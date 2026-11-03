/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's console character output (MAIN 0x1A5280-0x1A5368): Sony's SDK, the demo's object
 * 0x15CC80 (C), both of its functions -- kputchar writes one character to the EE's SIO (waiting
 * while the transmit FIFO is full), deci2Putchar collects a line in linebuf and hands it to kputs
 * at a newline or when it is full. Names are SOCOM 1's (s1:exact; count and linebuf the demo's
 * labels); both static, as in the demo's object, and deci2Putchar takes its character as an int. */

extern void kputs(char *);

extern int count;
extern char linebuf[128];

static int kputchar(int c)
{
	while (*(volatile int *)0x1000F130 & 0x8000)
		;
	*(volatile char *)0x1000F180 = c;
	return c;
}

static void deci2Putchar(int c)
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
