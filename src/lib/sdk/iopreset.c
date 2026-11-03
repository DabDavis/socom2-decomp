/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's IOP reset object (MAIN 0x1ABD28-0x1ABFD8): Sony's SDK, the demo's object 0x163620
 * (C), its four functions in the object's order, all s1:exact. sceSifResetIop sends the IOP the
 * reset command (0x80000003) in a 104-byte packet DMA'd to the IOP's command buffer (SIF register
 * 0x80000000) and, once the DMA is queued, clears the SIF flags the IOP sets as it comes back up;
 * sceSifRebootIop resets it through "rom0:UDNL " and the image's path. The packet is the demo's
 * static `rdata` (data.txt rdata@iopreset: libkernl's IOP heap object has an rdata of its own). */

typedef unsigned int u_int;

typedef struct {
	u_int psize:8;
	u_int dsize:24;
	u_int dest;
	u_int fno;
	u_int opt;
} sceSifCmdHdr;

typedef struct {
	sceSifCmdHdr chdr;
	int size;
	int flag;
	char arg[80];
} sceSifCmdResetData;

typedef struct {
	u_int data;
	u_int addr;
	int size;
	int mode;
} sceSifDmaData;

extern void sceSifStopDma(void);
extern u_int sceSifGetReg(u_int);
extern u_int sceSifSetReg(u_int, u_int);
extern u_int sceSifSetDma(sceSifDmaData *, int);
extern void sceSifWriteBackDCache(void *, int);
extern int sceSifInitRpc(u_int);
extern void sceSifExitRpc(void);
extern void sceResetttyinit(void);
extern int scePrintf(const char *, ...);

static sceSifCmdResetData rdata __attribute__((aligned(64)));

int sceSifResetIop(const char *arg, int mode)
{
	sceSifDmaData dmat;
	u_int dest;
	int i;

	sceSifStopDma();
	dest = sceSifGetReg(0x80000000);
	rdata.flag = mode;
	for (i = 0; arg[i] != 0; i++)
		rdata.arg[i] = arg[i];
	rdata.chdr.dest = 0;
	rdata.size = i;
	rdata.chdr.fno = 0x80000003;
	rdata.chdr.dsize = 0;
	rdata.chdr.psize = sizeof(rdata);
	dmat.data = (u_int)&rdata;
	dmat.addr = dest;
	dmat.size = sizeof(rdata);
	dmat.mode = 0x44;
	sceSifWriteBackDCache(&rdata, sizeof(rdata));
	sceSifSetReg(4, 0x40000);
	if (sceSifSetDma(&dmat, 1)) {
		sceSifSetReg(4, 0x10000);
		sceSifSetReg(4, 0x20000);
		sceSifSetReg(0x80000002, 0);
		sceSifSetReg(0x80000000, 0);
		return 1;
	}
	return 0;
}

int sceSifIsAliveIop(void)
{
	if (sceSifGetReg(4) & 0x10000)
		return 1;
	return 0;
}

int sceSifSyncIop(void)
{
	if (!(sceSifGetReg(4) & 0x40000))
		return 0;
	sceResetttyinit();
	return 1;
}

int sceSifRebootIop(const char *img)
{
	char param[80];
	const char *s = "rom0:UDNL ";
	const char *p;
	char *d;

	/* the end pointer offset first: `p - img + 11` adds after the subtraction (2 words short) */
	for (p = img; *p; p++)
		;
	if ((u_int)(p + 11 - img) > 80) {
		scePrintf("too long parameter '%s'\n", img);
		return 0;
	}
	sceSifInitRpc(0);
	sceSifExitRpc();
	d = param;
	while (*s)
		*d++ = *s++;
	while (*img)
		*d++ = *img++;
	*d = 0;
	return sceSifResetIop(param, 0);
}
