/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* logadd: the recogniser's log addition in half-log units (OVL2 0x4F5F20-0x4F610C, SOCOM 1's object
 * at 0x3629D0: LogAdd, HLnOnePlusExpHFloat), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM
 * 1's. ln(1 + e^x) is read off fLogOnePlusExp_lut (data.txt): 128 steps a unit above -0.5, 32
 * below, 0 below -9.5. SOCOM 1's LogAdd came first and inlined the table lookup; here the lookup
 * is a plain function above it and LogAdd calls it (0x4F6000: SOCOM 1's NormalizeAndEnQ calls
 * LogAdd where ours calls this row, s1:calls). */

extern const float fLogOnePlusExp_lut[353];

float HLnOnePlusExpHFloat(float x)
{
    if (x > -0.5)
        return fLogOnePlusExp_lut[(int)(x * 128.0 + 352.5)];
    if (x < -9.5)
        return 0;
    return fLogOnePlusExp_lut[(int)(x * 32.0 + 304.5)];
}

/* ln(e^a + e^b) in half-log units: the larger plus twice ln(1 + e^(-|a - b| / 2)) */
float LogAdd(float a, float b)
{
    float t;

    if (b < a) {
        t = HLnOnePlusExpHFloat((b - a) * 0.5);
        return a + t * 2.0;
    }
    t = HLnOnePlusExpHFloat((a - b) * 0.5);
    return b + t * 2.0;
}
