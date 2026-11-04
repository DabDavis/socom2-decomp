/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdlib/abort.c (MAIN 0x191410), the toolchain's libc: one object, as in the demo. _exit is
 * noreturn, so the loop leaves no branch behind. */

extern int raise(int sig);
extern void _exit(int status) __attribute__ ((noreturn));

void abort(void)
{
	while (1) {
		raise(6);               /* SIGABRT */
		_exit(1);
	}
}
