/* libgraph's types, as its units use them (Sony's SDK 2.7, MAIN 0x1A13D0-0x1A3058): the global
 * parameters sceGsGetGParam returns, the GS's privileged registers (64-bit, uncached at
 * 0x12000000) and the register-pair blocks the sceGsSetDef* functions fill. A GS register's
 * bitfields are spelt here as whole u_long words: the code stores and loads them whole. */

typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;
typedef int u_long128 __attribute__((mode(TI)));

typedef struct {
	short sceGsInterMode;		/* 0: SCE_GS_NOINTERLACE, 1: SCE_GS_INTERLACE */
	short sceGsOutMode;
	short sceGsFFMode;		/* 0: SCE_GS_FIELD, 1: SCE_GS_FRAME */
	short sceGsVersion;		/* the GS revision, CSR's REV */
	volatile int (*sceGsVSCfunc)(int);
	int sceGsVSCid;
} sceGsGParam __attribute__((aligned(16)));

extern sceGsGParam *sceGsGetGParam(void);

#define	GS_PMODE	((volatile u_long *)0x12000000)
#define	GS_SMODE2	((volatile u_long *)0x12000020)
#define	GS_DISPFB1	((volatile u_long *)0x12000070)
#define	GS_DISPLAY1	((volatile u_long *)0x12000080)
#define	GS_DISPFB2	((volatile u_long *)0x12000090)
#define	GS_DISPLAY2	((volatile u_long *)0x120000A0)
#define	GS_EXTDATA	((volatile u_long *)0x120000C0)
#define	GS_BGCOLOR	((volatile u_long *)0x120000E0)
#define	GS_CSR		((volatile u_long *)0x12001000)

typedef struct {
	u_long pmode;
	u_long smode2;
	u_long dispfb;
	u_long display;
	u_long bgcolor;
} sceGsDispEnv;

typedef struct {
	u_long alpha1;
	u_long alpha1addr;
	u_long pabe;
	u_long pabeaddr;
	u_long texa;
	u_long texaaddr;
	u_long fba1;
	u_long fba1addr;
} sceGsAlphaEnv;

#define	SCE_GS_ALPHA_1	0x42
#define	SCE_GS_PABE	0x49
#define	SCE_GS_TEXA	0x3b
#define	SCE_GS_FBA_1	0x4a

/* The GIF A+D register addresses and the register words, as Sony's libgraph.h builds them: each
 * field cast to u_long and shifted to its place (no field masked). */
#define	SCE_GS_PRIM	0x00
#define	SCE_GS_RGBAQ	0x01
#define	SCE_GS_XYZ2	0x05
#define	SCE_GS_TEST_1	0x47

#define	SCE_GS_PRIM_SPRITE	6

#define	SCE_GS_SET_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
	((u_long)(prim) | ((u_long)(iip) << 3) | ((u_long)(tme) << 4) | \
	 ((u_long)(fge) << 5) | ((u_long)(abe) << 6) | ((u_long)(aa1) << 7) | \
	 ((u_long)(fst) << 8) | ((u_long)(ctxt) << 9) | ((u_long)(fix) << 10))
#define	SCE_GS_SET_RGBAQ(r, g, b, a, q) \
	((u_long)(r) | ((u_long)(g) << 8) | ((u_long)(b) << 16) | \
	 ((u_long)(a) << 24) | ((u_long)(q) << 32))
#define	SCE_GS_SET_XYZ(x, y, z) \
	((u_long)(x) | ((u_long)(y) << 16) | ((u_long)(z) << 32))
#define	SCE_GS_SET_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
	((u_long)(ate) | ((u_long)(atst) << 1) | ((u_long)(aref) << 4) | \
	 ((u_long)(afail) << 12) | ((u_long)(date) << 14) | ((u_long)(datm) << 15) | \
	 ((u_long)(zte) << 16) | ((u_long)(ztst) << 17))

typedef struct {
	u_long testa;
	u_long testaaddr;
	u_long prim;
	u_long primaddr;
	u_long rgbaq;
	u_long rgbaqaddr;
	u_long xyz2a;
	u_long xyz2aaddr;
	u_long xyz2b;
	u_long xyz2baddr;
	u_long testb;
	u_long testbaddr;
} sceGsClear;

/* A GIF tag, field by field (the image transfers set NLOOP, EOP, NREG and REGS0 after clearing it
 * whole: one 128-bit store, then each word's fields read-modify-written -- flow folds the field
 * stores of one word into one). */
typedef struct {
	u_long NLOOP:15;
	u_long EOP:1;
	u_long pad16:16;
	u_long id:14;
	u_long PRE:1;
	u_long PRIM:11;
	u_long FLG:2;
	u_long NREG:4;
	u_long REGS0:4, REGS1:4, REGS2:4, REGS3:4, REGS4:4, REGS5:4, REGS6:4, REGS7:4;
	u_long REGS8:4, REGS9:4, REGS10:4, REGS11:4, REGS12:4, REGS13:4, REGS14:4, REGS15:4;
} sceGifTag __attribute__((aligned(16)));

#define	SCE_GIF_CLEAR_TAG(tp)	(*(u_long128 *)(tp) = (u_long128)0)
#define	SCE_GIF_PACKED_AD	0x0e

#define	SCE_GS_BITBLTBUF	0x50
#define	SCE_GS_TRXPOS		0x51
#define	SCE_GS_TRXREG		0x52
#define	SCE_GS_TRXDIR		0x53
#define	SCE_GS_FINISH		0x61

#define	SCE_GS_SET_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
	((u_long)(sbp) | ((u_long)(sbw) << 16) | ((u_long)(spsm) << 24) | \
	 ((u_long)(dbp) << 32) | ((u_long)(dbw) << 48) | ((u_long)(dpsm) << 56))
#define	SCE_GS_SET_TRXPOS(ssax, ssay, dsax, dsay, dir) \
	((u_long)(ssax) | ((u_long)(ssay) << 16) | ((u_long)(dsax) << 32) | \
	 ((u_long)(dsay) << 48) | ((u_long)(dir) << 59))
#define	SCE_GS_SET_TRXREG(rrw, rrh)	((u_long)(rrw) | ((u_long)(rrh) << 32))
#define	SCE_GS_SET_TRXDIR(xdr)		((u_long)(xdr))

#define	SCE_GS_XYOFFSET_1	0x18
#define	SCE_GS_PRMODECONT	0x1a
#define	SCE_GS_SCISSOR_1	0x40
#define	SCE_GS_DTHE		0x45
#define	SCE_GS_COLCLAMP		0x46
#define	SCE_GS_FRAME_1		0x4c
#define	SCE_GS_ZBUF_1		0x4e

#define	SCE_GS_SET_FRAME(fbp, fbw, psm, fbmask) \
	((u_long)(fbp) | ((u_long)(fbw) << 16) | ((u_long)(psm) << 24) | ((u_long)(fbmask) << 32))
#define	SCE_GS_SET_ZBUF(zbp, psm, zmsk) \
	((u_long)(zbp) | ((u_long)(psm) << 24) | ((u_long)(zmsk) << 32))
#define	SCE_GS_SET_XYOFFSET(ofx, ofy)	((u_long)(ofx) | ((u_long)(ofy) << 32))
#define	SCE_GS_SET_SCISSOR(scax0, scax1, scay0, scay1) \
	((u_long)(scax0) | ((u_long)(scax1) << 16) | ((u_long)(scay0) << 32) | ((u_long)(scay1) << 48))

/* the one-bit registers the drawing environment sets field by field (a read-modify-write) */
typedef struct { u_long AC:1; u_long pad1:63; } sceGsPrmodecont;
typedef struct { u_long CLAMP:1; u_long pad1:63; } sceGsColclamp;
typedef struct { u_long DTHE:1; u_long pad1:63; } sceGsDthe;

/* A drawing environment (context 1): eight A+D pairs. */
typedef struct {
	u_long frame1;
	u_long frame1addr;
	u_long zbuf1;
	u_long zbuf1addr;
	u_long xyoffset1;
	u_long xyoffset1addr;
	u_long scissor1;
	u_long scissor1addr;
	sceGsPrmodecont prmodecont;
	u_long prmodecontaddr;
	sceGsColclamp colclamp;
	u_long colclampaddr;
	sceGsDthe dthe;
	u_long dtheaddr;
	u_long test1;
	u_long test1addr;
} sceGsDrawEnv1;

extern short sceGszbufaddr(short psm, short w, short h);

/* FRAME_n's and DISPFB's frame-buffer page, the field the double buffer moves (the other
 * functions store these words whole) */
typedef struct { u_long FBP:9; u_long pad09:7; u_long FBW:6; u_long pad22:2; u_long PSM:6;
		 u_long pad30:2; u_long FBMSK:32; } sceGsFrame;
typedef struct { u_long FBP:9; u_long FBW:6; u_long PSM:5; u_long pad20:12; u_long DBX:11;
		 u_long DBY:11; u_long pad54:10; } sceGsDispfb;

/* Two display and drawing environments, each drawing one with its GIF tag (and a clear). */
typedef struct {
	sceGsDispEnv disp[2];
	sceGifTag giftag0;
	sceGsDrawEnv1 draw0;
	sceGsClear clear0;
	sceGifTag giftag1;
	sceGsDrawEnv1 draw1;
	sceGsClear clear1;
} sceGsDBuff;

extern void sceGsSetDefDispEnv(sceGsDispEnv *disp, short psm, short w, short h, short dx, short dy);
extern int sceGsSetDefDrawEnv(sceGsDrawEnv1 *dp, short psm, short w, short h, short ztest, short zpsm);
extern int sceGsSetDefClear(sceGsClear *cp, short ztest, short x, short y, short w, short h,
			    u_char r, u_char g, u_char b, u_char a, u_int z);

#define	SCE_GIF_IMAGE		2

#define	SCE_GS_PSMCT32		0
#define	SCE_GS_PSMCT24		1
#define	SCE_GS_PSMCT16		2
#define	SCE_GS_PSMCT16S		10
#define	SCE_GS_PSMT8		19
#define	SCE_GS_PSMT4		20
#define	SCE_GS_PSMT8H		27
#define	SCE_GS_PSMT4HL		36
#define	SCE_GS_PSMT4HH		44
#define	SCE_GS_PSMZ32		48
#define	SCE_GS_PSMZ24		49
#define	SCE_GS_PSMZ16		50
#define	SCE_GS_PSMZ16S		58

extern void scePrintf(const char *fmt, ...);

/* Host to GS local memory: a GIF tag of four A+D pairs, then an IMAGE tag for the data. */
typedef struct {
	sceGifTag giftag0;
	u_long bitbltbuf;
	u_long bitbltbufaddr;
	u_long trxpos;
	u_long trxposaddr;
	u_long trxreg;
	u_long trxregaddr;
	u_long trxdir;
	u_long trxdiraddr;
	sceGifTag giftag1;
} sceGsLoadImage __attribute__((aligned(16)));

/* GS local memory to host: VIF1's NOP, MSKPATH3, FLUSHA and DIRECT, a GIF tag, five A+D pairs. */
typedef struct {
	u_int vifcode[4];
	sceGifTag giftag;
	u_long bitbltbuf;
	u_long bitbltbufaddr;
	u_long trxpos;
	u_long trxposaddr;
	u_long trxreg;
	u_long trxregaddr;
	u_long finish;
	u_long finishaddr;
	u_long trxdir;
	u_long trxdiraddr;
} sceGsStoreImage __attribute__((aligned(16)));
