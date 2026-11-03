/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's console output (MAIN 0x1A5280-0x1A5C20): Sony's SDK, the demo's object 0x15CC80 (C), in
 * its order -- the character writers (kputchar to the EE's SIO, waiting while its transmit FIFO is
 * full; deci2Putchar collecting a line in linebuf for kputs at a newline or when full;
 * serialPutchar, a newline as CR LF), then ftoi, printfloat and _printf (the formatter, writing
 * through the _putchar hook: not written yet), kprintf (through the current hook) and scePrintf
 * (through deci2Putchar for the call, the hook restored after). Names are SOCOM 1's (s1:exact;
 * count, linebuf and _putchar the demo's labels); the first five are statics, as in the demo's
 * object, and the writers take their character as an int (no sign extension). */

#include <stdarg.h>

extern void kputs(char *);
extern int _printf(const char *fmt, va_list ap);

extern int count;
extern char linebuf[128];
extern int (*_putchar)(int);

static int kputchar(int c)
{
	while (*(volatile int *)0x1000F130 & 0x8000)
		;
	*(volatile char *)0x1000F180 = c;
	return c;
}

static int deci2Putchar(int c)
{
	if (count >= 126) {
		count = 0;
		linebuf[127] = 0;
		kputs(linebuf);
	}
	if (c == '\n') {
		linebuf[count] = c;
		linebuf[count + 1] = 0;
		count = 0;
		kputs(linebuf);
		return;
	}
	linebuf[count++] = c;
}

static void serialPutchar(int c)
{
	if (c == '\n') {
		kputchar('\r');
		kputchar('\n');
		return;
	}
	kputchar(c);
}

int kprintf(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = _printf(fmt, ap);
	va_end(ap);
	return ret;
}

int scePrintf(const char *fmt, ...)
{
	va_list ap;
	int (*old)(int);
	int ret;

	old = _putchar;
	_putchar = deci2Putchar;
	va_start(ap, fmt);
	ret = _printf(fmt, ap);
	va_end(ap);
	_putchar = old;
	return ret;
}
