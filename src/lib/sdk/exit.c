/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's program exits (MAIN 0x1ACCA8-0x1ACF44): Sony's SDK 2.7, the demo's object 0x1645A8 (C),
 * its nine functions in the same order. Each exit first copies the file name and up to fifteen
 * arguments into the kernel's argument area (_kExecArg, found by InitTLBFunctions) through a byte-copy
 * kCopy run in kernel mode, then restores the default TLB (TerminateLibrary) and makes the kernel
 * call. Names are SOCOM 1's (s1:exact; its statics by position, `Copy@sdk/exit` -- the stem exit is libc's too: README "The GCC front"). */

typedef unsigned int u_int;
typedef unsigned char u_char;

struct SyscallData {
	int syscall;
	void *function;
};

extern char **_kExecArg;
extern int strlen(const char *s);
extern int InitTLB(void);
extern int _ExecPS2(void *entry, void *gp, int argc, char **argv);
extern void _LoadExecPS2(const char *name, int argc, char **argv);
extern void _Exit(int status);
extern void _ExecOSD(int argc, char **argv);

static int kCopy(u_char *dest, u_char *src, u_int size);

static struct SyscallData SysEntry = { 0x5A, kCopy };

static int Copy(void *dest, const void *src, int size)
{
	asm volatile ("li $3, 0x5a\n\tsyscall");
}

static int kCopy(u_char *dest, u_char *src, u_int size)
{
	u_int i;

	for (i = 0; i < size; i++)
		*dest++ = *src++;
	return 0;
}

static void setup(int syscall, void *function)
{
	asm volatile ("li $3, 0x74\n\tsyscall");
}

static char *SetArg(const char *filename, int argc, char **argv)
{
	char **arg = _kExecArg;
	char *name, *ptr;
	int len, i;

	ptr = (char *)(arg + 16);
	name = ptr;
	setup(SysEntry.syscall, SysEntry.function);
	if (argc > 15)
		argc = 15;
	Copy(arg++, &ptr, 4);
	len = strlen(filename) + 1;
	Copy(ptr, filename, len);
	ptr += len;
	for (i = 0; i < argc; i++) {
		Copy(arg++, &ptr, 4);
		len = strlen(argv[i]) + 1;
		Copy(ptr, argv[i], len);
		ptr += len;
	}
	return name;
}

static void TerminateLibrary(void)
{
	InitTLB();
}

int ExecPS2(void *entry, void *gp, int argc, char **argv)
{
	SetArg("", argc, argv);
	TerminateLibrary();
	return _ExecPS2(entry, gp, argc, &_kExecArg[1]);
}

void LoadExecPS2(const char *filename, int argc, char **argv)
{
	char *name;

	name = SetArg(filename, argc, argv);
	TerminateLibrary();
	_LoadExecPS2(name, argc, &_kExecArg[1]);
}

void Exit(int status)
{
	TerminateLibrary();
	_Exit(status);
}

void ExecOSD(int argc, char **argv)
{
	SetArg("", argc, argv);
	TerminateLibrary();
	_ExecOSD(argc, &_kExecArg[1]);
}
