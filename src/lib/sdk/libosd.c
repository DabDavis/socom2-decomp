/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's ExecPS2 kernel patch (MAIN 0x1ACB28-0x1ACCA4): Sony's SDK 2.7, the demo's object
 * 0x164428 (C), its six functions in the same order. An early kernel cannot keep the OSD
 * configuration's version field (PatchIsNeeded writes 1 and reads it back); on one, InitExecPS2 copies
 * the patch's code image (osdsrc) to 0x80074000 in kernel mode and points the patched syscall slot
 * (-0x3BFE: relative to the table, ps2sdk's note) at it. Names are SOCOM 1's (s1:exact; its statics by
 * position, `setup@libosd`: README "The GCC front"). */

typedef unsigned int u_int;

typedef struct {
	u_int spdifMode:1;
	u_int screenType:2;
	u_int videoOutput:1;
	u_int japLanguage:1;
	u_int ps1drvConfig:8;
	u_int version:3;
	u_int language:5;
	int timezoneOffset:11;
} sceOsdConfigParam;

struct SyscallData {
	int syscall;
	void *function;
};

extern void GetOsdConfigParam(sceOsdConfigParam *p);
extern void SetOsdConfigParam(sceOsdConfigParam *p);
extern void FlushCache(int op);

extern unsigned char osdsrc[0x7A8];     /* the patch's kernel-mode code image (this object's .data) */

static int kCopy(u_int *dest, u_int *src, u_int size);

struct SyscallData SysExecPS2Entry[] = {
	{ 0x5A, kCopy },
	{ 0x5B, (void *)0x80074000 },
	{ -0x3BFE, 0 },
};

static void setup(int syscall, void *function)
{
	asm volatile ("li $3, 0x74\n\tsyscall");
}

static int Copy(void *dest, void *src, int size)
{
	asm volatile ("li $3, 0x5a\n\tsyscall");
}

static int kCopy(u_int *dest, u_int *src, u_int size)
{
	u_int i;

	size >>= 2;
	for (i = 0; i < size; i++)
		*dest++ = *src++;
	return 0;
}

static void *GetEntryAddress(int syscall)
{
	asm volatile ("li $3, 0x5b\n\tsyscall");
}

static int PatchIsNeeded(void)
{
	sceOsdConfigParam original, config;

	GetOsdConfigParam(&original);
	config = original;
	config.version = 1;             /* an early kernel cannot keep this field */
	SetOsdConfigParam(&config);
	GetOsdConfigParam(&config);
	SetOsdConfigParam(&original);
	return config.version < 1;
}

void InitExecPS2(void)
{
	u_int i;

	if (PatchIsNeeded()) {
		setup(SysExecPS2Entry[0].syscall, SysExecPS2Entry[0].function);
		Copy((void *)0x80074000, osdsrc, sizeof(osdsrc));
		FlushCache(0);
		FlushCache(2);
		setup(SysExecPS2Entry[1].syscall, SysExecPS2Entry[1].function);
		for (i = 2; i < 3; i++)
			setup(SysExecPS2Entry[i].syscall, GetEntryAddress(SysExecPS2Entry[i].syscall));
	}
}
