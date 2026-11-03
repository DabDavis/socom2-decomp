/* ai_perception.cpp (OVL2 0x53ABE0-0x53B00B, the name ours): how well a viewer perceives a
 * target, new in SOCOM II -- six factors (the range first), each valid by a bit, weighted per
 * range band through two static tables. The AI's target scan (OVL2 0x60AA20) builds one on its
 * stack, sets the range and the other factors and asks for the score; the turret's and the
 * sensors' code set the maximum range first (OVL1 0x3A82C0, OVL2 0x5C9B30).
 * Defined here in reverse of the image's order (-inline deferred). */

/* the weights per range band: six factors and the score's scale (normalised in place) */
static float s_maxRange = 1000.0f;
static float s_invMaxRange = 0.001f;

/* a factor's response per band: lo at zero, hi at full, the factor scaled first */
struct PERCEPTION_CURVE {
    float lo;
    float hi;
    float scale;
};

static PERCEPTION_CURVE s_curves[6][4] = {
    {{6.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
    {{1.0f, 3.0f, 0.02f}, {0.0f, 5.0f, 0.02f}, {-0.5f, 1.5f, 0.02f}, {-0.5f, 0.5f, 0.02f}},
    {{0.5f, 1.5f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
    {{0.5f, 1.0f, 1.0f}, {0.5f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}},
    {{0.5f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {-1.0f, 0.5f, 1.0f}, {-1.0f, 0.5f, 1.0f}},
    {{-1.5f, 1.0f, 1.0f}, {-1.5f, 1.0f, 1.0f}, {-1.5f, 0.5f, 1.0f}, {-1.5f, 0.0f, 1.0f}},
};

static float s_weights[4][7] = {
    {1.0f, 1.0f, 1.0f, 0.1f, 0.0f, 0.5f, 1.1f},
    {0.75f, 0.75f, 0.5f, 0.25f, 0.1f, 0.5f, 1.0f},
    {0.25f, 0.35f, 0.25f, 0.25f, 0.1f, 0.75f, 1.0f},
    {0.25f, 0.25f, 0.25f, 0.25f, 0.1f, 0.75f, 1.0f},
};

static float s_bands[3] = {100.0f, 0.7f, 0.3f};

static float s_lastRange;

class CAiPerception {
public:
    CAiPerception(unsigned int viewer, unsigned int target);
    void SetRange(float range);
    float Compute();
    static void SetMaxRange(float range);
    static void NormalizeWeights();

    unsigned int m_viewer;   // 0x0: the viewer's entity id
    unsigned int m_target;   // 0x4: the target's
    unsigned char m_band;   // 0x8
    float m_factor[6];   // 0xc
    unsigned int m_valid;   // 0x24

    /* valid bits cleared, then set (CAiPath::SetFlags' shape: a clear of none leaves the
     * target's zero extension, dsll32/dsrl32, before the set) */
    void SetValid(unsigned int set, unsigned int clear)
    {
        m_valid &= ~clear;
        m_valid |= set;
    }
    /* a factor and its valid bit (the target scan sets five this way) */
    void SetFactor(int i, float v)
    {
        m_factor[i] = v;
        SetValid(1 << i, 0);
    }
};

void CAiPerception::NormalizeWeights()
{
    for (int i = 0; i < 4; i++) {
        float sum = 0.0f;

        for (int j = 0; j < 6; j++)
            sum += s_weights[i][j];
        float inv = 1.0f / sum;

        for (int j = 0; j < 6; j++)
            s_weights[i][j] *= inv;
    }
}

void CAiPerception::SetMaxRange(float range)
{
    s_maxRange = range;
    s_invMaxRange = 1.0f / range;
}

CAiPerception::CAiPerception(unsigned int viewer, unsigned int target)
{
    m_viewer = viewer;
    m_target = target;
    m_band = 2;
    m_valid = 0;
    NormalizeWeights();
}

void CAiPerception::SetRange(float range)
{
    float x = (s_maxRange - range) * s_invMaxRange;
    int band;

    if (range <= s_bands[0]) {
        x = range / s_bands[0];
        band = 0;
    } else if (!(x < s_bands[1])) {
        band = 1;
    } else if (!(x < s_bands[2])) {
        band = 2;
    } else {
        band = 3;
    }
    s_lastRange = x;
    m_band = band;
    SetFactor(0, s_lastRange);
}

float CAiPerception::Compute()
{
    float w[6] = {0};
    float m[7] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    if (m_factor[1] <= 0.1f && m_factor[4] < 1.0f && m_factor[5] <= 0.25f && m_factor[2] <= 0.5f) {
        m[6] = 0.25f;
        m[0] = 0.5f;
    } else if (!(m_factor[2] < 1.0f)) {
        m[6] = 2.0f * m_factor[2];
    }
    if (m_band == 0 && 1.0f == m_factor[5])
        m[6] = 1000.0f;

    float sum = 0.0f;

    for (int i = 0; i < 6; i++) {
        if ((1 << i) & m_valid) {
            float x = m_factor[i] * s_curves[i][m_band].scale;
            /* lo and hi through a reference: written out, MW multiplies (1 - x) by lo where the
             * target multiplies lo by it (the scale read through it too loses the load order) */
            const PERCEPTION_CURVE &c = s_curves[i][m_band];

            w[i] = c.lo * (1.0f - x) + c.hi * x;
            sum += m[i] * (w[i] * s_weights[m_band][i]);
        }
    }
    float r = sum * s_weights[m_band][6];

    if (!(r <= 0.0f))
        r *= m[6];
    return r;
}
