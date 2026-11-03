/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's console output (MAIN 0x1A5280-0x1A5C20): Sony's SDK, the demo's object 0x15CC80 (C), in
 * its order -- the character writers (kputchar to the EE's SIO, waiting while its transmit FIFO is
 * full; deci2Putchar collecting a line in linebuf for kputs at a newline or when full;
 * serialPutchar, a newline as CR LF), then ftoi and printfloat (%f: the value scaled into
 * [0.1, 1) by tens, its first six digits printed as "0.%d" and the exponent after), _printf (the
 * formatter, writing through the _putchar hook), kprintf (through the current hook) and
 * scePrintf (through deci2Putchar for the call, the hook restored after). Names are
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
 * exit test calls dpcmp, which jump.c will not duplicate).
 *
 * _printf runs with interrupts masked (DIntr; EIntr when they were on) and knows %o %x %d %u %c %s
 * and %e/%f (both printfloat, of a float read from its 8-byte slot -- lwc1 -- and 0 printed as
 * "0"), the length letters l (a doubleword) and h, and a zero pad %0N/%0NN (one or two digits, at
 * most 31: the buffer filled with '0's from 31 - N, the number printed from whichever starts
 * first); a '-' goes out before the padding, an empty string as "(null)", any other letter is
 * skipped. Its numbers are 64-bit (dsrl, __udivdi3/__divdi3; each digit's andi 0xff is
 * truncdiqi2). Measured (build/scratch/gcc/pf/, 0 of ~40 shapes until these): the hook is `void
 * (*)(int)` -- an int-returning hook keeps each call's v0 live, so every reload of the hook after
 * a call takes v0 where the target alternates v0/v1 through the "(null)" run; the second pad
 * digit is read before the first is tested (its `lb` fills the test's delay slot, which no load
 * from the fall-through may), the first tested as `(unsigned char)width < 10` (andi 0xff) and the
 * second as an int range, the tens summed as `width * 10 + (c - '0')`; the pad pointer is
 * `&buf[31 - width]` (`buf + 31 - width` folds to buf - (width - 31)); the string has a pointer of
 * its own, copied to p for the loop; the unsigned conversions use an unsigned long (sltiu on the
 * hex digit) and %d a long; and the fmt++ after the switch is the plain character's too (so its
 * else is laid out before the switch's default). */

#include <stdarg.h>

extern void kputs(char *);
int _printf(const char *fmt, va_list ap);

extern int count;
extern char linebuf[128];
extern void (*_putchar)(int);
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

extern int DIntr(void);
extern int EIntr(void);

int _printf(const char *fmt, va_list ap)
{
	char buf[32];
	char *p, *pad, *s;
	long v;
	unsigned long u;
	int lng, width;
	int c;
	int intr;
	float f;

	intr = DIntr();
	while (*fmt) {
		pad = 0;
		lng = 0;
		if (*fmt == '%') {
		next:
			fmt++;
			switch (*fmt) {
			case '0':
				width = fmt[1] - '0';
				c = fmt[2];
				if ((unsigned char)width < 10) {
					if ((unsigned)(c - '0') < 10) {
						width = width * 10 + (c - '0');
						if (width >= 32)
							width = 31;
						fmt += 2;
					} else
						fmt++;
					pad = &buf[31 - width];
					while (width > 0) {
						buf[31 - width] = '0';
						width--;
					}
				}
				goto next;
			case 'l':
				lng = 'l';
				goto next;
			case 'h':
				lng = 'h';
				goto next;
			case 'o':
				if (lng == 'l')
					u = va_arg(ap, long);
				else if (lng == 'h')
					u = (unsigned short)va_arg(ap, int);
				else
					u = (unsigned int)va_arg(ap, int);
				p = buf + 31;
				*p = 0;
				if (u == 0)
					*--p = '0';
				else
					while (u) {
						*--p = (u & 7) + '0';
						u >>= 3;
					}
				if (pad && pad < p)
					p = pad;
				while (*p)
					_putchar(*p++);
				break;
			case 'x':
				if (lng == 'l')
					u = va_arg(ap, long);
				else if (lng == 'h')
					u = (unsigned short)va_arg(ap, int);
				else
					u = (unsigned int)va_arg(ap, int);
				p = buf + 31;
				*p = 0;
				if (u == 0)
					*--p = '0';
				else
					while (u) {
						if ((u & 15) < 10)
							*--p = (u & 15) + '0';
						else
							*--p = (u & 15) + 'a' - 10;
						u >>= 4;
					}
				if (pad && pad < p)
					p = pad;
				while (*p)
					_putchar(*p++);
				break;
			case 'd':
				if (lng == 'l')
					v = va_arg(ap, long);
				else if (lng == 'h')
					v = (short)va_arg(ap, int);
				else
					v = va_arg(ap, int);
				p = buf + 31;
				*p = 0;
				if (v == 0)
					*--p = '0';
				else {
					if (v < 0) {
						v = -v;
						_putchar('-');
					}
					while (v) {
						*--p = v % 10 + '0';
						v /= 10;
					}
				}
				if (pad && pad < p)
					p = pad;
				while (*p)
					_putchar(*p++);
				break;
			case 'u':
				if (lng == 'l')
					u = va_arg(ap, long);
				else if (lng == 'h')
					u = (unsigned short)va_arg(ap, int);
				else
					u = (unsigned int)va_arg(ap, int);
				p = buf + 31;
				*p = 0;
				if (u == 0)
					*--p = '0';
				else
					while (u) {
						*--p = u % 10 + '0';
						u /= 10;
					}
				if (pad && pad < p)
					p = pad;
				while (*p)
					_putchar(*p++);
				break;
			case 'e':
			case 'f':
				f = va_arg(ap, float);
				if (f == 0)
					_putchar('0');
				else
					printfloat(f);
				break;
			case 's':
				s = va_arg(ap, char *);
				if (*s == 0) {
					_putchar('(');
					_putchar('n');
					_putchar('u');
					_putchar('l');
					_putchar('l');
					_putchar(')');
				} else {
					for (p = s; *p; p++)
						_putchar(*p);
				}
				break;
			case 'c':
				v = (char)va_arg(ap, int);
				_putchar(v);
				break;
			}
		} else
			_putchar(*fmt);
		fmt++;
	}
	if (intr)
		EIntr();
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
