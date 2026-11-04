/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, BitPackC.c (OVL1 0x2484A0-0x248718): the bit packer -- a field of up to
 * eight bits appended at the stream's bit position, the byte position wrapping at the buffer's
 * size. Asserting as the codec does (file and line by `#line`, as in sase_encsc.c). Written in
 * the GCC lane's round 18a: the packer 0x2484A0. 0x2485D0 (max.s/madda.s: the R5900's float
 * accumulator, assembly or a body of inline asm, as the dot product 0x248F60) is not written.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

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

void func_002484A0(BitPackMblk *hBitPack, int iData, int cDataWidth)
{
	unsigned short *pusBytePos;
	unsigned char *pcBitPos, *pc;
	unsigned short usBufSize;
	short sData;

#line 110 "../../SaseEncVad/source/BitPackC.c"
	assert(cDataWidth<=8);
	sData = iData;
	pusBytePos = hBitPack->pusBytePos;
	pcBitPos = hBitPack->pcBitPos;
	pc = hBitPack->pcBuffer + *pusBytePos;
	usBufSize = hBitPack->usBufSize;
	if (*pcBitPos == 0)
		*pc = sData;
	else
		*pc |= sData << *pcBitPos;
	*pcBitPos += cDataWidth;
	if (*pcBitPos > 8) {
		pc++;
		if (++*pusBytePos >= usBufSize) {
			*pusBytePos = 0;
			pc = hBitPack->pcBuffer;
		}
		*pc = sData >> (8 - (*pcBitPos - cDataWidth));
		*pcBitPos -= 8;
	} else if (*pcBitPos == 8) {
		*pcBitPos = 0;
		if (++*pusBytePos >= usBufSize)
			*pusBytePos = 0;
	}
}
