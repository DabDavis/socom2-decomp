/* CQuat -- quaternions (OVL1). Names are SOCOM 1's (SCUS_972.05's symbols, carried by
 * exact body fingerprint); the file name is a placeholder until SOCOM 1's debug info
 * names the translation unit. Layout: x, y, z, w (MakeZRot writes sin to +8, cos to +12). */

extern "C" float sinf(float);
extern "C" float cosf(float);

class CQuat {
public:
    float x, y, z, w;

    void MakeXRot(float angle);
    void MakeYRot(float angle);
    void MakeZRot(float angle);
};

/* The three are emitted in reverse definition order (-inline deferred), so the image's
 * Z, Y, X at 0x307520/0x307580/0x3075E0 means the source defines X, Y, Z. */

void CQuat::MakeXRot(float angle)
{
    float half = 0.5f * angle;
    x = sinf(half);
    w = cosf(half);
    y = 0.0f;
    z = 0.0f;
}

void CQuat::MakeYRot(float angle)
{
    float half = 0.5f * angle;
    y = sinf(half);
    w = cosf(half);
    x = 0.0f;
    z = 0.0f;
}

void CQuat::MakeZRot(float angle)
{
    float half = 0.5f * angle;
    z = sinf(half);
    w = cosf(half);
    x = 0.0f;
    y = 0.0f;
}
