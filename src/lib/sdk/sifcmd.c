/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's SIF command layer (MAIN 0x1A5C20-0x1A6368): Sony's SDK, the demo's object 0x15D620
 * (C), its sixteen functions in the same order -- the two system command handlers (the IOP's
 * change-address and set-register commands), the soft registers, sceSifInitCmd (the receive
 * buffer, the system table, SIF0's DMA handler, the IOP handshake), the handler tables, the send
 * (one or two DMA descriptors: the extra data, then the packet) and the SIF0 interrupt handler
 * that copies each packet out and dispatches it. Names are SOCOM 1's (s1:exact), the statics' the
 * demo's labels of this object's .bss (_pckt_buffer, _send_buffer, _csdata, sif0_handleid,
 * _data_table, _sys_buffer, soft_reg); the layouts are Sony's sifcmd.h, checked by the code (and
 * ps2sdk's reconstruction, binary compatible: its struct cmd_data is _data_table -- but its
 * interrupt handler's `(size + 30) >> 4` is the target's signed `(size + 15) / 16`, its packet
 * copy 8 quadwords where the target's frame holds 7, and the target's sceSifSetCmdBuffer and
 * sceSifSetSysCmdBuffer return the table they replace).
 *
 * Shapes, measured: _sceSifSendCmd fills its descriptor's source and destination before the
 * header's (one order of 5040: tools/ccperm.py, which can now see it -- `--stem`); a void
 * function's last calls are 991111's sibling calls only when every path ends in a call and its
 * own `return` (sceSifInitCmd: an early `EIntr(); return;` otherwise leaves the two
 * sceSifSendCmd calls that fall into the epilogue as jal; build/scratch/gcc/p_sib2.c); the
 * receive buffer's header is volatile (the target's `lbu; andi 0xff` of psize), and so is the
 * copied packet's (fno reloaded on each path).
 *
 * sceSifInitCmd byte-exact since GCC round 83: both clears count UP to 32 -- loop.c reverses
 * them (s0 down from 31), and the counter's final value 32, live across EIntr and FlushCache, is
 * the SIF0 bit stored back (the s0 the OPEN note read as a variable for the bit).
 * OPEN: _sceSifCmdIntrHdlr (8, right size: one int is the size, the quadword count and the id --
 * the count tested `blez`, since a pseudo set more than once has no nonzero_bits for combine to
 * prove it positive; the packet indexed and the receive buffer walked, so the count's copy comes
 * before the giv's init; left: the fno test in v1 and the handler in a2, where this takes v0/v1). */

typedef unsigned int u_int;
typedef unsigned int u_long128 __attribute__((mode(TI)));

typedef struct {
	u_int psize:8;
	u_int dsize:24;
	u_int dest;
	u_int fno;
	u_int opt;
} sceSifCmdHdr;

typedef void (*sceSifCmdHandler)(void *, void *);

typedef struct {
	sceSifCmdHandler func;
	void *data;
} sceSifCmdData;

typedef struct {
	sceSifCmdHdr chdr;
	void *newaddr;
} sceSifCmdCSData;

typedef struct {
	sceSifCmdHdr chdr;
	int rno;
	u_int value;
} sceSifCmdSRData;

typedef struct {
	u_int data;
	u_int addr;
	int size;
	int mode;
} sceSifDmaData;

typedef struct {
	void *pktbuf;			/* 0x00: the receive buffer, uncached */
	void *sendbuf;			/* 0x04 */
	void *iopbuf;			/* 0x08: the IOP's receive buffer */
	sceSifCmdData *systbl;		/* 0x0C */
	int systbl_len;			/* 0x10 */
	sceSifCmdData *usrtbl;		/* 0x14 */
	int usrtbl_len;			/* 0x18 */
	int *sreg;			/* 0x1C */
} sceSifCmdState;

extern int DIntr(void);
extern int EIntr(void);
extern void FlushCache(int);
extern void sceSifSetDChain(void);
extern void isceSifSetDChain(void);
extern int AddDmacHandler(int, int (*)(int), int);
extern int RemoveDmacHandler(int, int);
extern int EnableDmac(int);
extern int DisableDmac(int);
extern u_int sceSifGetReg(u_int);
extern int sceSifSetReg(u_int, u_int);
extern u_int sceSifSetDma(sceSifDmaData *, int);
extern u_int isceSifSetDma(sceSifDmaData *, int);

void sceSifWriteBackDCache(void *ptr, int size);
u_int sceSifSendCmd(u_int fno, void *pkt, int psize, void *src, void *dest, int size);
static int _sceSifCmdIntrHdlr(int ch);

static u_int _pckt_buffer[32] __attribute__((aligned(64)));
static u_int _send_buffer[16] __attribute__((aligned(64)));
static sceSifCmdCSData _csdata __attribute__((aligned(64)));
static int sif0_handleid;
static sceSifCmdState _data_table;
static sceSifCmdData _sys_buffer[32];
static int soft_reg[32];
static int _cmd_init_check = 0;

static void _set_sreg(sceSifCmdSRData *sr, sceSifCmdState *d)
{
	d->sreg[sr->rno] = sr->value;
}

static void _change_addr(sceSifCmdCSData *cs, sceSifCmdState *d)
{
	d->iopbuf = cs->newaddr;
}

int sceSifGetSreg(int rno)
{
	return soft_reg[rno];
}

int sceSifSetSreg(int rno, int value)
{
	soft_reg[rno] = value;
	return value;
}

sceSifCmdState *sceSifGetDataTable(void)
{
	return &_data_table;
}

void sceSifInitCmd(void)
{
	int i;

	DIntr();
	if (_cmd_init_check) {
		EIntr();
		return;
	}
	_cmd_init_check = 1;
	_data_table.pktbuf = (void *)((u_int)_pckt_buffer | 0x20000000);
	_data_table.sendbuf = (void *)((u_int)_send_buffer | 0x20000000);
	_data_table.iopbuf = 0;
	_data_table.systbl = _sys_buffer;
	_data_table.systbl_len = 32;
	_data_table.usrtbl = 0;
	_data_table.usrtbl_len = 0;
	_data_table.sreg = soft_reg;
	for (i = 0; i < 32; i++) {
		_sys_buffer[i].func = 0;
		_sys_buffer[i].data = 0;
	}
	for (i = 0; i < 32; i++)
		soft_reg[i] = 0;
	_sys_buffer[0].func = (sceSifCmdHandler)_change_addr;
	_sys_buffer[0].data = &_data_table;
	_sys_buffer[1].func = (sceSifCmdHandler)_set_sreg;
	_sys_buffer[1].data = &_data_table;
	EIntr();
	FlushCache(0);
	if (*(volatile u_int *)0x1000E010 & 0x20)
		*(volatile u_int *)0x1000E010 = 0x20;
	if (!(*(volatile u_int *)0x1000C000 & 0x100))
		sceSifSetDChain();
	sif0_handleid = AddDmacHandler(5, _sceSifCmdIntrHdlr, 0);
	EnableDmac(5);
	if ((_data_table.iopbuf = (void *)sceSifGetReg(0x80000000)) != 0) {
		_csdata.newaddr = _pckt_buffer;
		sceSifSendCmd(0x80000000, &_csdata, sizeof _csdata, 0, 0, 0);
		return;
	} else {
		while (!(sceSifGetReg(4) & 0x20000))
			;
		_data_table.iopbuf = (void *)sceSifGetReg(2);
		sceSifSetReg(0x80000000, (u_int)_data_table.iopbuf);
		sceSifSetReg(0x80000001, (u_int)&_data_table);
		_csdata.chdr.opt = 0;
		_csdata.newaddr = _pckt_buffer;
		sceSifSendCmd(0x80000002, &_csdata, sizeof _csdata, 0, 0, 0);
		return;
	}
}

void sceSifExitCmd(void)
{
	DisableDmac(5);
	RemoveDmacHandler(5, sif0_handleid);
	_cmd_init_check = 0;
}

sceSifCmdData *sceSifSetCmdBuffer(sceSifCmdData *db, int size)
{
	sceSifCmdData *old;

	old = _data_table.usrtbl;
	_data_table.usrtbl = db;
	_data_table.usrtbl_len = size;
	return old;
}

sceSifCmdData *sceSifSetSysCmdBuffer(sceSifCmdData *db, int size)
{
	sceSifCmdData *old;

	old = _data_table.systbl;
	_data_table.systbl = db;
	_data_table.systbl_len = size;
	return old;
}

void sceSifAddCmdHandler(u_int fno, sceSifCmdHandler func, void *data)
{
	if (fno & 0x80000000) {
		_data_table.systbl[fno & 0x7fffffff].func = func;
		_data_table.systbl[fno & 0x7fffffff].data = data;
	} else {
		_data_table.usrtbl[fno].func = func;
		_data_table.usrtbl[fno].data = data;
	}
}

void sceSifRemoveCmdHandler(u_int fno)
{
	if (fno & 0x80000000)
		_data_table.systbl[fno & 0x7fffffff].func = 0;
	else
		_data_table.usrtbl[fno].func = 0;
}

static u_int _sceSifSendCmd(u_int fno, int mode, sceSifCmdHdr *pkt, int psize, void *src, void *dest, int size)
{
	sceSifDmaData dmat[2];
	int count;

	if (psize < 16 || psize > 112)
		return 0;
	count = 0;
	if (size > 0) {
		count = 1;
		pkt->dsize = size;
		dmat[0].data = (u_int)src;
		dmat[0].addr = (u_int)dest;
		pkt->dest = (u_int)dest;
		dmat[0].size = size;
		dmat[0].mode = 0;
		if (mode & 4)
			sceSifWriteBackDCache(src, size);
	} else {
		pkt->dsize = 0;
		pkt->dest = 0;
	}
	dmat[count].data = (u_int)pkt;
	dmat[count].addr = (u_int)_data_table.iopbuf;
	dmat[count].size = psize;
	pkt->fno = fno;
	pkt->psize = psize;
	dmat[count].mode = 0x44;
	count++;
	sceSifWriteBackDCache(pkt, psize);
	if (mode & 1)
		return isceSifSetDma(dmat, count);
	else
		return sceSifSetDma(dmat, count);
}

u_int sceSifSendCmd(u_int fno, void *pkt, int psize, void *src, void *dest, int size)
{
	return _sceSifSendCmd(fno, 0, pkt, psize, src, dest, size);
}

u_int isceSifSendCmd(u_int fno, void *pkt, int psize, void *src, void *dest, int size)
{
	return _sceSifSendCmd(fno, 1, pkt, psize, src, dest, size);
}

static int _sceSifCmdIntrHdlr(int ch)
{
	u_long128 pkt[7];
	sceSifCmdState *dt;
	volatile sceSifCmdHdr *h;
	volatile sceSifCmdHdr *c;
	int size, i;

	EIntr();
	dt = &_data_table;
	h = (volatile sceSifCmdHdr *)dt->pktbuf;
	if ((size = h->psize) == 0)
		return 0;
	size = (size + 15) / 16;
	h->psize = 0;
	{ u_long128 *s = (u_long128 *)h; for (i = 0; i < size; i++) pkt[i] = *s++; }
	isceSifSetDChain();
	c = (volatile sceSifCmdHdr *)pkt;
	if (c->fno & 0x80000000) {
		size = c->fno & 0x7fffffff;
		if (size < dt->systbl_len) {
			if (dt->systbl[size].func)
				dt->systbl[size].func(pkt, dt->systbl[size].data);
		}
	} else {
		size = c->fno;
		if (size < dt->usrtbl_len) {
			if (dt->usrtbl[size].func)
				dt->usrtbl[size].func(pkt, dt->usrtbl[size].data);
		}
	}
	asm volatile ("sync\n\tei");
	return 0;
}

/* Written back 64-byte line by line, eight lines a turn after the odd ones; its own `jr ra` ends
 * it, and the asm leaves noreorder on so that GCC's return is assembled with no delay slot: the
 * next object's first word is the one the image holds there. */
void sceSifWriteBackDCache(void *ptr, int size)
{
	asm volatile (
	"	.set noreorder\n"
	"	lui $25, 0xffff\n"
	"	ori $25, $25, 0xffc0\n"
	"	blez $5, 9f\n"
	"	addu $10, $4, $5\n"
	"	and $8, $4, $25\n"
	"	addiu $10, $10, -1\n"
	"	and $9, $10, $25\n"
	"	subu $10, $9, $8\n"
	"	srl $11, $10, 6\n"
	"	addiu $11, $11, 1\n"
	"	andi $9, $11, 7\n"
	"	beqz $9, 2f\n"
	"	srl $10, $11, 3\n"
	"1:	sync\n"
	"	cache 0x18, 0($8)\n"
	"	sync\n"
	"	addiu $9, $9, -1\n"
	"	nop\n"
	"	bgtz $9, 1b\n"
	"	addiu $8, $8, 64\n"
	"2:	beqz $10, 9f\n"
	"3:	addiu $10, $10, -1\n"
	"	sync\n"
	"	cache 0x18, 0($8)\n"
	"	sync\n"
	"	cache 0x18, 64($8)\n"
	"	sync\n"
	"	cache 0x18, 128($8)\n"
	"	sync\n"
	"	cache 0x18, 192($8)\n"
	"	sync\n"
	"	cache 0x18, 256($8)\n"
	"	sync\n"
	"	cache 0x18, 320($8)\n"
	"	sync\n"
	"	cache 0x18, 384($8)\n"
	"	sync\n"
	"	cache 0x18, 448($8)\n"
	"	sync\n"
	"	bgtz $10, 3b\n"
	"	addiu $8, $8, 512\n"
	"9:	jr $31\n"
	"	nop\n");
}
