/* libgraph's types, as its units use them (Sony's SDK 2.7, MAIN 0x1A13D0-0x1A3058): the global
 * parameters sceGsGetGParam returns, the GS's privileged registers (64-bit, uncached at
 * 0x12000000) and the register-pair blocks the sceGsSetDef* functions fill. A GS register's
 * bitfields are spelt here as whole u_long words: the code stores and loads them whole. */

typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;

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
