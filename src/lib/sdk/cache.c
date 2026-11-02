/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's data-cache maintenance (MAIN 0x1A4370-0x1A45EC): Sony's SDK 2.7, the demo's object
 * 0x15BD70 (C), its six functions in the same order. _sceSDC and _sceIDC walk both ways of the 8 KB
 * data cache by index (64-byte lines): each line's tag is loaded (cache 0x10, TagLo), and a line
 * inside [start, end] is written back and invalidated (0x14) or only invalidated (0x16). The
 * thread-context forms mask interrupts around the walk; the `i` forms call it straight. Names are
 * SOCOM 1's (s1:exact). */

typedef unsigned int u_int;

extern int DIntr(void);
extern int EIntr(void);

void _sceSDC(u_int start, u_int end)
{
	int i;
	u_int tag;

	for (i = 0; i < 4096; i += 64) {
		asm volatile ("sync\n\tcache 0x10, 0(%0)\n\tsync" : : "r" (i));
		asm volatile ("mfc0 %0, $28" : "=r" (tag));
		tag = (tag & 0xfffff000) + i;
		if (!(tag < start) && !(end < tag))
			asm volatile ("sync\n\tcache 0x14, 0(%0)\n\tsync" : : "r" (i));
		asm volatile ("sync\n\tcache 0x10, 1(%0)\n\tsync" : : "r" (i));
		asm volatile ("mfc0 %0, $28" : "=r" (tag));
		tag = (tag & 0xfffff000) + i;
		if (!(tag < start) && !(end < tag))
			asm volatile ("sync\n\tcache 0x14, 1(%0)\n\tsync" : : "r" (i));
		asm volatile ("sync");
	}
}

void SyncDCache(void *start, void *end)
{
	int eie;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	if (eie)
		DIntr();
	_sceSDC((u_int)start & 0xffffffc0, (u_int)end & 0xffffffc0);
	if (eie)
		EIntr();
}

void iSyncDCache(void *start, void *end)
{
	_sceSDC((u_int)start & 0xffffffc0, (u_int)end & 0xffffffc0);
}

void _sceIDC(u_int start, u_int end)
{
	int i;
	u_int tag;

	for (i = 0; i < 4096; i += 64) {
		asm volatile ("sync\n\tcache 0x10, 0(%0)\n\tsync" : : "r" (i));
		asm volatile ("mfc0 %0, $28" : "=r" (tag));
		tag = (tag & 0xfffff000) + i;
		if (!(tag < start) && !(end < tag))
			asm volatile ("sync\n\tcache 0x16, 0(%0)\n\tsync" : : "r" (i));
		asm volatile ("sync\n\tcache 0x10, 1(%0)\n\tsync" : : "r" (i));
		asm volatile ("mfc0 %0, $28" : "=r" (tag));
		tag = (tag & 0xfffff000) + i;
		if (!(tag < start) && !(end < tag))
			asm volatile ("sync\n\tcache 0x16, 1(%0)\n\tsync" : : "r" (i));
		asm volatile ("sync");
	}
}

void InvalidDCache(void *start, void *end)
{
	int eie;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	if (eie)
		DIntr();
	_sceIDC((u_int)start & 0xffffffc0, (u_int)end & 0xffffffc0);
	if (eie)
		EIntr();
}

void iInvalidDCache(void *start, void *end)
{
	_sceIDC((u_int)start & 0xffffffc0, (u_int)end & 0xffffffc0);
}
