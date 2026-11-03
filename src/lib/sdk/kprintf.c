/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's console output (MAIN 0x1A5280-0x1A5C20): Sony's SDK, the demo's object 0x15CC80 (C), in
 * its order -- the character writers (kputchar to the EE's SIO, waiting while its transmit FIFO is
 * full; deci2Putchar collecting a line in linebuf for kputs at a newline or when full;
 * serialPutchar, a newline as CR LF), then ftoi and printfloat (%f: the value scaled into
 * [0.1, 1) by tens, its first six digits printed as "0.%d" and the exponent after), _printf (the
 * formatter, writing through the _putchar hook: not written yet), kprintf (through the current
 * hook) and scePrintf (through deci2Putchar for the call, the hook restored after). Names are
 * SOCOM 1's (s1:exact; count, linebuf and _putchar the demo's labels); the first five are
 * statics, as in the demo's object, and the writers take their character as an int (no sign
 * extension).
 *
 * ftoi rounds a double's bit pattern to an int -- but printfloat hands it `d * 1e6` converted to
 * an unsigned long (__fixunsdfdi: its parameter is a u_long, so the call converts), so it reads an
 * integer's bits as a double: Sony's, as the target has it. Measured shapes: the exponent shifted
 * into its variable and the bias subtracted in place, the mantissa masked and its hidden bit or'd
 * in as two statements, the parameter itself reused for it (the target's `move a1,a0`: 16 shapes
 * tried, build/scratch/gcc/gen_kp2.py); printfloat's ftoi call is a statement of its own (the
 * format's address is formed after the call), and its loops are entered at their tests (the
 * exit test calls dpcmp, which jump.c will not duplicate). */

#include <stdarg.h>

extern void kputs(char *);
extern int _printf(const char *fmt, va_list ap);

extern int count;
extern char linebuf[128];
extern int (*_putchar)(int);
int kprintf(const char *fmt, ...);

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

typedef unsigned long u_long;

static int ftoi(u_long x)
{
	long e;

	e = (x << 1) >> 53;
	e -= 1075;
	if (e < -53)
		return 0;
	if (e >= 13)
		return 9999;
	x = (x << 12) >> 12;
	x |= 1UL << 52;
	if (e < 0) {
		e = -e;
		x >>= e - 2;
		if ((x & 3) == 3)
			x = (x >> 2) + 1;
		else
			x >>= 2;
	} else
		x <<= e;
	return x;
}

static void printfloat(double d)
{
	int e, n;

	e = 0;
	if (d < 0.0) {
		d = 0.0 - d;
		_putchar('-');
	}
	if (d < 0.1) {
		while (d < 0.1) {
			d *= 10.0;
			e--;
		}
	} else if (d >= 1.0) {
		while (d >= 1.0) {
			d /= 10.0;
			e++;
		}
	}
	n = ftoi(d * 1000000.0);
	kprintf("0.%d", n);
	if (e >= 0)
		kprintf("e+%d", e);
	else
		kprintf("e%d", e);
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
