/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, PackSC.c (OVL1 0x24FAE8-0x24FBE8): a frame's 17 quantiser indices packed
 * into the packet, each at its own bit width (a static const table of shorts, compared whole),
 * through BitPackC.c's packer (sase_bitpackc.c) over a packer block on the stack. Asserting as
 * the codec does (file and line by `#line`, as in sase_encsc.c). Written in the GCC lane's round
 * 18a; the block's stores in one of the three orders of 720 that give the target's schedule and
 * registers (tools/ccperm.py), the plainest. The packer's prototype here takes an unsigned char
 * (the caller's lbu of each int index) where its definition takes an int. Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96,
 * -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	unsigned char *pcBuffer;	/* 0x00 */
	unsigned short usBufSize;	/* 0x04 */
	short sPad;
	int i8, iC;
	unsigned short *pusBytePos;	/* 0x10 */
	unsigned char *pcBitPos;	/* 0x14 */
} BitPackMblk;

extern void func_002484A0(BitPackMblk *hBitPack, unsigned char cData, short cDataWidth);

static const short asBitWidth[17] = { 6, 4, 8, 5, 5, 5, 4, 5, 3, 5, 2, 5, 0, 5, 0, 1, 1 };

void func_0024FAE8(int *piQuanIndex, unsigned char *pchPacketBuffer, unsigned short *psByteOffset,
		   unsigned char *pchBitOffset, int iBufSize, int iFrame)
{
	BitPackMblk bp;
	int i;

#line 99 "../../SaseEncVad/source/PackSC.c"
	assert(piQuanIndex != NellyNull && pchPacketBuffer != NellyNull);
	assert(psByteOffset != NellyNull && pchBitOffset != NellyNull);
	bp.usBufSize = iBufSize;
	bp.pusBytePos = psByteOffset;
	bp.pcBitPos = pchBitOffset;
	bp.pcBuffer = pchPacketBuffer;
	piQuanIndex[16] = (unsigned short)iFrame;
	piQuanIndex[15] = 0;
	for (i = 0; i < 17; i++)
		func_002484A0(&bp, piQuanIndex[i], asBitWidth[i]);
}
