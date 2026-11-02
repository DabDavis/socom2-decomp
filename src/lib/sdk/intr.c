/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's interrupt-controller calls (MAIN 0x1A45F0-0x1A4820): Sony's SDK 2.7, the demo's object
 * 0x15BFF0 (C), its nine functions in the same order. The thread-context forms mask interrupts
 * around the system call when they were enabled (Status.EIE, bit 16: DIntr/EIntr), the interrupt-
 * context forms (`i`) only order the write with a sync. Names are SOCOM 1's (s1:exact). */

extern int _DisableIntc(int ch);
extern int _EnableIntc(int ch);
extern int _DisableDmac(int ch);
extern int _EnableDmac(int ch);
extern int _iEnableIntc(int ch);
extern int _iDisableIntc(int ch);
extern int _iEnableDmac(int ch);
extern int _iDisableDmac(int ch);
extern void DIntr(void);
extern void EIntr(void);

int QueryIntrContext(void)
{
	int status;

	asm volatile ("mfc0 %0, $12" : "=r" (status));
	return (status ^ 1) & 1;            /* Status.EXL clear: thread context */
}

int DisableIntc(int ch)
{
	int eie, ret;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	if (eie)
		DIntr();
	ret = _DisableIntc(ch);
	asm volatile ("sync");
	if (eie)
		EIntr();
	return ret;
}

int EnableIntc(int ch)
{
	int eie, ret;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	if (eie)
		DIntr();
	ret = _EnableIntc(ch);
	asm volatile ("sync");
	if (eie)
		EIntr();
	return ret;
}

int DisableDmac(int ch)
{
	int eie, ret;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	if (eie)
		DIntr();
	ret = _DisableDmac(ch);
	asm volatile ("sync");
	if (eie)
		EIntr();
	return ret;
}

int EnableDmac(int ch)
{
	int eie, ret;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	if (eie)
		DIntr();
	ret = _EnableDmac(ch);
	asm volatile ("sync");
	if (eie)
		EIntr();
	return ret;
}

int iEnableIntc(int ch)
{
	int ret = _iEnableIntc(ch);
	asm volatile ("sync");
	return ret;
}

int iDisableIntc(int ch)
{
	int ret = _iDisableIntc(ch);
	asm volatile ("sync");
	return ret;
}

int iEnableDmac(int ch)
{
	int ret = _iEnableDmac(ch);
	asm volatile ("sync");
	return ret;
}

int iDisableDmac(int ch)
{
	int ret = _iDisableDmac(ch);
	asm volatile ("sync");
	return ret;
}
