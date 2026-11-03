/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, CodePv.c (OVL1 0x248718-0x2488A0): the Pv parameter's two scalar codebooks
 * (64 levels, and the 8-level midpoint book right after it in .rodata, each compared whole) and the
 * coder's quantise, dequantise and two-stage index over libquan.c (sase_libquan.c). The tables are
 * hex floats: this GCC's decimal conversion is not the vendor's (sase_misc.c), so a decimal of the
 * vendor's precision need not give the target's bits; the hex form does exactly. 0x2487E8 is not
 * written. Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96,
 * -O2 -G8). */

extern int func_0024B720(const float *pfCodeBook, int iLength, float fValue);
extern float func_0024B7E8(const float *pfCodeBook, int iLength, int iIndex);

static const float fPvTable[64] = {
	-0x1.56d7f0p+1f, -0x1.d29d62p+0f, -0x1.62393ep+0f, -0x1.12d9cep+0f,
	-0x1.84e7eep-1f, -0x1.e06d92p-2f, -0x1.8b3e56p-3f, 0x1.8a6330p-4f,
	0x1.596bfcp-2f, 0x1.19d648p-1f, 0x1.7f531cp-1f, 0x1.ed4046p-1f,
	0x1.30156ep+0f, 0x1.693ab4p+0f, 0x1.a9d5e4p+0f, 0x1.ed2598p+0f,
	0x1.15fedap+1f, 0x1.2d077cp+1f, 0x1.422b50p+1f, 0x1.57ed9cp+1f,
	0x1.6cb274p+1f, 0x1.821c3ep+1f, 0x1.98f4c6p+1f, 0x1.b147f0p+1f,
	0x1.ca6a2ap+1f, 0x1.e2cfa6p+1f, 0x1.f8cf94p+1f, 0x1.07985ep+2f,
	0x1.12fbf4p+2f, 0x1.1dce30p+2f, 0x1.289110p+2f, 0x1.34aa2ap+2f,
	0x1.3f1da2p+2f, 0x1.4a538ep+2f, 0x1.54eb06p+2f, 0x1.605dbep+2f,
	0x1.6af112p+2f, 0x1.756dacp+2f, 0x1.804e92p+2f, 0x1.8a56f6p+2f,
	0x1.950634p+2f, 0x1.9f6fb8p+2f, 0x1.a9f1bep+2f, 0x1.b5355cp+2f,
	0x1.c06cb4p+2f, 0x1.cc0a24p+2f, 0x1.d74c54p+2f, 0x1.e33d28p+2f,
	0x1.eea4f8p+2f, 0x1.fa2156p+2f, 0x1.02c068p+3f, 0x1.08967ep+3f,
	0x1.0ea7e8p+3f, 0x1.14df4cp+3f, 0x1.1be6dcp+3f, 0x1.23528ap+3f,
	0x1.2ae138p+3f, 0x1.32d9c2p+3f, 0x1.3b718ap+3f, 0x1.44d29ap+3f,
	0x1.4e8556p+3f, 0x1.5a01b8p+3f, 0x1.674e62p+3f, 0x1.773b2ep+3f,
};
static const float fPvMidTable[8] = {
	0.0f, 0x1.24dd2ep-3f, 0x1.24dd2ep-2f, 0x1.b74bc6p-2f, 0x1.245a1cp-1f, 0x1.6d9168p-1f, 0x1.b6c8b4p-1f, 0x1.000000p+0f,
};

int func_00248718(float fPv)
{
	return func_0024B720(fPvTable, 64, fPv);
}

float func_00248738(int iIndex)
{
	return func_0024B7E8(fPvTable, 64, iIndex);
}

int func_00248760(float fPv, float fRef, float fMid)
{
	int iIndex, iFlag;
	float fErr0, fErr1;

	iIndex = func_0024B720(fPvMidTable, 8, fMid);
	fErr0 = fPv - fRef;
	fErr0 *= fErr0;
	fErr1 = fPvMidTable[iIndex] - fRef;
	fErr1 *= fErr1;
	iFlag = 0;
	if (fErr1 < fErr0)
		iFlag = 1;
	return iIndex + iFlag * 8;
}
