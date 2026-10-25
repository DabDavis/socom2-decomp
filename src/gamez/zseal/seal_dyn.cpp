/* seal_dyn.cpp -- the character's dynamics, CharacterDynamics read from its .rdr: the physics, the
 * aim and look limits, the cameras (SOCOM 1's unit, __sinit_seal_dyn.cpp; SOCOM 1's order, Load and
 * then the camera vector's template tail). OVL2 0x59BA80 (Load, after seal_create.cpp's GetSeal
 * stretch) to 0x59D6BF: Load, insert 0x59CF10, reserve 0x59D490, the deleter's destructor 0x59D640
 * and clear 0x59D6B0 -- SOCOM 1's swap, ~CharacterDynamics and the deleter's constructor are gone.
 * Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zrdr/zrdr.h"

extern "C" {
/* the world's scale (OVL1 data 0x3DFE10, data.txt's; znode.h does not declare it) */
extern float m_scale__Q23zdb6CWorld;
/* the first-person bobbing: the walk's and the crawl's amplitude and rate (OVL1 bss
 * 0x44D4E8-0x44D4F4, new in SOCOM II; the names ours) */
extern float data_0044D4E8;
extern float data_0044D4EC;
extern float data_0044D4F0;
extern float data_0044D4F4;
}

#define DEG2RAD 0.017453292f

/* a length in metres made world units (an inline: the product lands in the value's own register) */
static inline float WorldScale(float v)
{
    v *= m_scale__Q23zdb6CWorld;
    return v;
}

/* a camera pushed: include/vector's push_back reads the end through the deleter's data()
 * reference (SOCOM 1's DWARF: data() inside the deleter, <vector> line 923) -- MW then keeps
 * &data_ in a saved register across the pushes, as the target does */
static inline void PushCamera(std::vector<CharacterDynamics::CCameraParams> &v,
                              const CharacterDynamics::CCameraParams &p)
{
    v.push_back(p);
}

/* the dynamics read, once: each value under its tag, the angles in degrees made radians and their
 * cosines and sines kept, the falls' landing speeds solved, the wiggle, the bobbing and the throw
 * from their blocks, five cameras pushed (OVL2 0x59BA80; SOCOM 1's with the bobbing) */
void CharacterDynamics::Load(const CRdrFile &rdr)
{
    float tmp;
    CPnt3D aim;

    if (m_loaded)
        return;
    m_loaded = true;
    zrdr_findreal(&rdr, "gravity", &m_gravity, 1);
    zrdr_findreal(&rdr, "DamageToNewtons", &m_DamageToNewtons, 1);
    zrdr_findreal(&rdr, "jump_factor", &m_jumpFactor, 1);
    zrdr_findreal(&rdr, "land_fall_rate", &m_landFallRate, 1);
    zrdr_findreal(&rdr, "land_hard_fall_rate", &m_landHardFallRate, 1);
    zrdr_findreal(&rdr, "ground_touch_distance", &m_groundTouchDistance, 1);
    zrdr_findreal(&rdr, "max_slope", &m_maxSlopeCos, 1);
    m_maxSlopeCos = cosf(DEG2RAD * m_maxSlopeCos);
    zrdr_findreal(&rdr, "step_height", &m_stepHeight, 1);
    zrdr_findreal(&rdr, "vertical_blast_boost", &m_blastBoostY, 1);
    m_fallDist[0] = 1.3f;
    m_fallDist[1] = 2.6f;
    m_fallDist[2] = 8.0f;
    zrdr_findreal(&rdr, "FALLING_DAMAGE_LIGHT", &m_fallDist[0], 1);
    zrdr_findreal(&rdr, "FALLING_DAMAGE_HEAVY", &m_fallDist[1], 1);
    zrdr_findreal(&rdr, "FALLING_DAMAGE_DEATH", &m_fallDist[2], 1);
    m_fallDist[0] = WorldScale(m_fallDist[0]);
    m_fallDist[1] = WorldScale(m_fallDist[1]);
    m_fallDist[2] = WorldScale(m_fallDist[2]);
    float g = theCharacterDynamics.m_gravity;

    for (int i = 0; i < 3; i++)
        m_landSpeed[i] = g * sqrtf(2.0f * m_fallDist[i] / g);
    zrdr_findreal(&rdr, "water_factor_slope", &m_waterFactorSlope, 1);
    zrdr_findreal(&rdr, "min_water_factor", &m_minWaterFactor, 1);
    zrdr_findreal(&rdr, "fb_accel", &m_fbStiffness, 1);
    zrdr_findreal(&rdr, "lr_accel", &m_lrStiffness, 1);
    zrdr_findreal(&rdr, "throt_exp", &m_throtExp, 1);
    m_throtScale = 1.0f / (exp_tl(m_throtExp) - 1.0f);
    if (zrdr_findreal(&rdr, "stand_turn_factor", &tmp, 1))
        m_standTurnFactor = tmp;
    zrdr_findreal(&rdr, "turn_maxrate", &m_turn_maxRate, 1);
    zrdr_findreal(&rdr, "lower_x_accel", &m_accelXMin, 1);
    zrdr_findreal(&rdr, "upper_x_accel", &m_accelXMax, 1);
    zrdr_findreal(&rdr, "lower_z_accel", &m_accelZMin, 1);
    zrdr_findreal(&rdr, "upper_z_accel", &m_accelZMax, 1);
    if (zrdr_findreal(&rdr, "init_aim_pitch", &tmp, 1))
        m_initAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "max_aim_pitch", &tmp, 1))
        m_maxAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "min_aim_pitch", &tmp, 1))
        m_minAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "max_aim_yaw", &tmp, 1))
        m_maxAimYaw = DEG2RAD * tmp;
    m_cosMaxAimPitch = cosf(m_maxAimPitch);
    m_cosMinAimPitch = cosf(m_minAimPitch);
    m_cosMaxAimYaw = cosf(m_maxAimYaw);
    if (zrdr_findreal(&rdr, "prone_max_aim_pitch", &tmp, 1))
        m_proneMaxAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "prone_min_aim_pitch", &tmp, 1))
        m_proneMinAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "prone_max_aim_yaw", &tmp, 1))
        m_proneMaxAimYaw = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "prone_zoom_delta_yaw", &tmp, 1))
        m_proneZoomDeltaYaw = DEG2RAD * tmp;
    m_cosProneMaxAimPitch = sinf(m_proneMaxAimPitch);
    m_cosProneMinAimPitch = sinf(m_proneMinAimPitch);
    m_cosProneMaxAimYaw = cosf(m_proneMaxAimYaw);
    if (zrdr_findreal(&rdr, "max_zoom_aim_pitch", &tmp, 1))
        m_maxZoomAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "min_zoom_aim_pitch", &tmp, 1))
        m_minZoomAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "max_zoom_aim_yaw", &tmp, 1))
        m_maxZoomAimYaw = DEG2RAD * tmp;
    m_cosMaxZoomAimPitch = cosf(m_maxZoomAimPitch);
    m_cosMinZoomAimPitch = cosf(m_minZoomAimPitch);
    m_cosMaxZoomAimYaw = cosf(m_maxZoomAimYaw);
    if (zrdr_findreal(&rdr, "prone_max_zoom_aim_pitch", &tmp, 1))
        m_proneMaxZoomAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "prone_min_zoom_aim_pitch", &tmp, 1))
        m_proneMinZoomAimPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "prone_max_zoom_aim_yaw", &tmp, 1))
        m_proneMaxZoomAimYaw = DEG2RAD * tmp;
    m_cosProneMaxZoomAimPitch = sinf(m_proneMaxZoomAimPitch);
    m_cosProneMinZoomAimPitch = sinf(m_proneMinZoomAimPitch);
    m_cosProneMaxZoomAimYaw = cosf(m_proneMaxZoomAimYaw);
    if (zrdr_findreal(&rdr, "max_look_pitch", &tmp, 1))
        m_maxLookPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "min_look_pitch", &tmp, 1))
        m_minLookPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "max_look_yaw", &tmp, 1))
        m_maxLookYaw = DEG2RAD * tmp;
    m_cosMaxLookPitch = cosf(m_maxLookPitch);
    m_cosMinLookPitch = cosf(m_minLookPitch);
    m_cosMaxLookYaw = cosf(m_maxLookYaw);
    if (zrdr_findreal(&rdr, "max_eye_look_pitch", &tmp, 1))
        m_maxEyelookPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "min_eye_look_pitch", &tmp, 1))
        m_minEyelookPitch = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "max_eye_look_yaw", &tmp, 1))
        m_maxEyelookYaw = DEG2RAD * tmp;
    m_cosMaxEyelookPitch = cosf(m_maxEyelookPitch);
    m_cosMinEyelookPitch = cosf(m_minEyelookPitch);
    m_cosMaxEyelookYaw = cosf(m_maxEyelookYaw);
    if (zrdr_findreal(&rdr, "max_blink_rate", &tmp, 1))
        m_maxBlinkRate = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "max_blink_angle", &tmp, 1))
        m_maxBlinkAngle = DEG2RAD * tmp;
    if (zrdr_findreal(&rdr, "pitch_rate", &tmp, 1))
        m_pitchRate = tmp;
    if (zrdr_findreal(&rdr, "turn_throttle_a", &tmp, 1))
        m_turnThrottleA = tmp;
    if (zrdr_findreal(&rdr, "turn_throttle_b", &tmp, 1))
        m_turnThrottleB = tmp;
    if (zrdr_findreal(&rdr, "pitch_throttle_a", &tmp, 1))
        m_pitchThrottleA = tmp;
    if (zrdr_findreal(&rdr, "pitch_throttle_b", &tmp, 1))
        m_pitchThrottleB = tmp;
    if (zrdr_findreal(&rdr, "camera_roll", &tmp, 1))
        m_cameraRoll = tmp;
    _zrdr *block = zrdr_findtag(&rdr, "CAMERA_WIGGLE");

    if (block && block->type == ZRDR_ARRAY) {
        zrdr_findreal(block, "Amplitude", &m_cam_wiggle.amplitude, 1);
        zrdr_findreal(block, "Duration", &m_cam_wiggle.duration, 1);
        zrdr_findreal(block, "Rate", &m_cam_wiggle.rate, 1);
    }
    block = zrdr_findtag(&rdr, "BOBBING_FIRSTPERSON");
    if (block && block->type == ZRDR_ARRAY) {
        float f;

        if (zrdr_findreal(block, "Walk_Amplitude", &f, 1))
            data_0044D4E8 = f;
        if (zrdr_findreal(block, "Walk_Rate", &f, 1))
            data_0044D4F0 = f;
        if (zrdr_findreal(block, "Crawl_Amplitude", &f, 1))
            data_0044D4EC = f;
        if (zrdr_findreal(block, "Crawl_Rate", &f, 1))
            data_0044D4F4 = f;
    }
    block = zrdr_findtag(&rdr, "THROW_PARAMS");
    if (block) {
        zrdr_findreal(block, "abort_threshold", &m_throw_params.m_abort_threshold, 1);
        zrdr_findreal(block, "max_distance_stand", &m_throw_params.max_distance_stand, 1);
        zrdr_findreal(block, "max_distance_crouch", &m_throw_params.max_distance_crouch, 1);
        zrdr_findreal(block, "toss_power_threshold", &m_throw_params.toss_power_threshold, 1);
        if (zrdr_findreal(block, "toss_aim_threshold", &m_throw_params.toss_aim_threshold, 1))
            m_throw_params.toss_aim_threshold = sinf(DEG2RAD * m_throw_params.toss_aim_threshold);
    }
    if (zrdr_findreal(&rdr, "zoom_factor", &tmp, 1))
        m_zoomFactor = tmp;
    if (zrdr_findreal(&rdr, "zoom_rate", &tmp, 1))
        m_zoomRate = tmp;
    m_cam_params.reserve(6);
    float side = 0.0f, height = 0.0f, dist = 0.0f;

    if (zrdr_findreal(&rdr, "cam_back_side", &tmp, 1))
        side = tmp;
    if (zrdr_findreal(&rdr, "cam_back_height", &tmp, 1))
        height = tmp;
    if (zrdr_findreal(&rdr, "cam_back_dist", &tmp, 1))
        dist = tmp;
    {
        CPnt3D offset(side, height, dist);

        if (!zrdr_findPNT3D(&rdr, "cam_back_aim", &aim)) {
            aim.x = 0.0f;
            aim.y = 20.0f;
            aim.z = 0.0f;
        }
        PushCamera(m_cam_params, CCameraParams(offset, aim));
    }
    side = height = dist = 0.0f;
    if (zrdr_findreal(&rdr, "cam_side_side", &tmp, 1))
        side = tmp;
    if (zrdr_findreal(&rdr, "cam_side_height", &tmp, 1))
        height = tmp;
    if (zrdr_findreal(&rdr, "cam_side_dist", &tmp, 1))
        dist = tmp;
    {
        CPnt3D offset(side, height, dist);

        if (!zrdr_findPNT3D(&rdr, "cam_side_aim", &aim)) {
            aim.x = 0.0f;
            aim.y = 20.0f;
            aim.z = 0.0f;
        }
        PushCamera(m_cam_params, CCameraParams(offset, aim));
    }
    side = height = dist = 0.0f;
    if (zrdr_findreal(&rdr, "cam_first_side", &tmp, 1))
        side = tmp;
    if (zrdr_findreal(&rdr, "cam_first_height", &tmp, 1))
        height = tmp;
    if (zrdr_findreal(&rdr, "cam_first_dist", &tmp, 1))
        dist = tmp;
    {
        CPnt3D offset(side, height, dist);

        if (!zrdr_findPNT3D(&rdr, "cam_first_aim", &aim)) {
            aim.x = 0.0f;
            aim.y = 20.0f;
            aim.z = 0.0f;
        }
        PushCamera(m_cam_params, CCameraParams(offset, aim));
    }
    side = height = dist = 0.0f;
    if (zrdr_findreal(&rdr, "cam_peekl_side", &tmp, 1))
        side = tmp;
    if (zrdr_findreal(&rdr, "cam_peekl_height", &tmp, 1))
        height = tmp;
    if (zrdr_findreal(&rdr, "cam_peekl_dist", &tmp, 1))
        dist = tmp;
    {
        CPnt3D offset(side, height, dist);

        if (!zrdr_findPNT3D(&rdr, "cam_peekl_aim", &aim)) {
            aim.x = 0.0f;
            aim.y = 20.0f;
            aim.z = 0.0f;
        }
        PushCamera(m_cam_params, CCameraParams(offset, aim));
    }
    side = height = dist = 0.0f;
    if (zrdr_findreal(&rdr, "cam_peekr_side", &tmp, 1))
        side = tmp;
    if (zrdr_findreal(&rdr, "cam_peekr_height", &tmp, 1))
        height = tmp;
    if (zrdr_findreal(&rdr, "cam_peekr_dist", &tmp, 1))
        dist = tmp;
    {
        CPnt3D offset(side, height, dist);

        if (!zrdr_findPNT3D(&rdr, "cam_peekr_aim", &aim)) {
            aim.x = 0.0f;
            aim.y = 20.0f;
            aim.z = 0.0f;
        }
        PushCamera(m_cam_params, CCameraParams(offset, aim));
    }
    if (zrdr_findreal(&rdr, "cam_tether_stiff", &tmp, 1))
        m_cameraTetherStiffness = tmp;
    if (zrdr_findreal(&rdr, "cam_look_dwell", &tmp, 1))
        m_cameraLookDwell = tmp;
    if (zrdr_findreal(&rdr, "cam_net_pos_smooth", &tmp, 1))
        m_cameraNetPosSmooth = tmp;
    if (zrdr_findreal(&rdr, "cam_net_release_rad", &tmp, 1))
        m_cameraReleaseRad = tmp;
    if (zrdr_findreal(&rdr, "cam_peek_decay_rate", &tmp, 1))
        m_cameraPeekDecayRate = tmp;
    zrdr_findreal(&rdr, "min_running_reload_speed", &tmp, 1);
    m_running_reload_speed = WorldScale(tmp);
    zrdr_findreal(&rdr, "min_running_chgweapon_speed", &tmp, 1);
    m_running_chgweapon_speed = WorldScale(tmp);
    zrdr_findreal(&rdr, "low_climb_height", &tmp, 1);
    m_lowHeight = WorldScale(tmp);
    zrdr_findreal(&rdr, "med_climb_height", &tmp, 1);
    m_medHeight = WorldScale(tmp);
    zrdr_findreal(&rdr, "high_climb_height", &tmp, 1);
    m_highHeight = WorldScale(tmp);
    zrdr_findreal(&rdr, "min_stand_height", &tmp, 1);
    m_minStandHeight = WorldScale(tmp);
    zrdr_findreal(&rdr, "min_jump_height", &tmp, 1);
    m_minJumpHeight = WorldScale(tmp);
}
