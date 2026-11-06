/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's interrupt mask (MAIN 0x1AC860-0x1AC8C0): Sony's SDK 2.7, the demo's object 0x164160
 * (C), DIntr then EIntr. DIntr clears Status.EIE and loops until the write has taken (di is not
 * guaranteed to before an interrupt is taken: the R5900's errata loop, its head aligned by GCC);
 * both return whether interrupts were enabled. Names are SOCOM 1's (s1:exact). */

/* OPEN: 14s here (and under every 2.9x build). The target reloads the 0x10000 mask inside the
 * loop (`mfc0 v0; lui v1,1; and v0,v0,v1; bnez v0`) and returns through one register on both
 * paths (`beqz v1,out; sltu a0,zero,v1` ... `out: move a0,zero; jr ra; move v0,a0`), where GCC
 * hoists the mask out of this loop and returns early: the mask is not a loop invariant GCC could
 * see -- the original's test must sit inside the asm (or behind a volatile), with the loop head
 * aligned by an `.p2align 3` (the nop before `di`). ps2sdk's spelling (the mask in C, an
 * asm(".p2align 3")) gives 16s, a single exit 14s. Round 6a: the reload is a goto loop's -- GCC
 * 2.9's loop optimiser hoists only from while/do/for -- and `if (eie) { asm(".p2align 3");
 * again: di; sync.p; mfc0; if (sr & 0x10000) goto again; }` gives 7s with the mask in the loop
 * (build/scratch/gcc/v_dintr.c); what remains is the loop's v0/v1 pair and the target's own
 * `move a0,zero; jr ra; move v0,a0` for the disabled path. GCC round 43a: that path is res = 0 in an
 * else, returned through the one `return res` (the `sltu` before the branch, reorg's slot fill, and
 * `move a0,zero` on the else: the target's) -- 3 words, the right size: only the loop's pair is left
 * (the target reads SR into v0 and the mask into v1, the AND into v0; here SR v1, mask v0, the AND
 * into the mask's v0; `eie &= 0x10000; if (eie)` in the loop keeps eie's v1 throughout, 4; a second
 * variable for the loop's SR, 5: build/scratch/gcc/r43/v_dintr.py). */
int DIntr(void)
{
	int eie, res;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	res = eie != 0;
	if (eie) {
		asm (".p2align 3");
	again:
		asm volatile ("di");
		asm volatile ("sync.p");
		asm volatile ("mfc0 %0, $12" : "=r" (eie));
		if (eie & 0x10000)
			goto again;
	} else
		res = 0;
	return res;
}

int EIntr(void)
{
	int eie;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	asm volatile ("ei");
	return eie != 0;
}
