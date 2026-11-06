/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's interrupt mask (MAIN 0x1AC860-0x1AC8C0): Sony's SDK 2.7, the demo's object 0x164160
 * (C), DIntr then EIntr. DIntr clears Status.EIE and loops until the write has taken (di is not
 * guaranteed to before an interrupt is taken: the R5900's errata loop, its head aligned by GCC);
 * both return whether interrupts were enabled. Names are SOCOM 1's (s1:exact). */

/* The target reloads the 0x10000 mask inside the loop (`mfc0 v0; lui v1,1; and v0,v0,v1; bnez v0`): a goto loop's,
 * since GCC 2.9's loop optimiser hoists only from while/do/for (round 6a; ps2sdk's spelling with a do loop is 16
 * words off), its head aligned by an `.p2align 3` (the nop before `di`). The disabled path is res = 0 in an else (round
 * 43a). The two blocks' opposite register pairs are the allocators' (GCC round 44a, open since round 2a): `res = eie !=
 * 0` is the ARM's first statement, reorg's fill of the branch's slot, so eie is referenced in a second block -- a GLOBAL
 * pseudo: local-alloc gives the first block's mask v0 and global-alloc gives eie, alive across it, v1; the loop reads SR
 * into a second variable, LOCAL to its block, which local-alloc ranks above its mask: v0, the mask v1. Written before the
 * test, eie is local too and takes v0, the first block's pair swapped (5 words; round 43a's 13 declaration orders never
 * moved the statement). */
int DIntr(void)
{
	int eie, res, sr;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	if (eie) {
		res = eie != 0;
		asm (".p2align 3");
	again:
		asm volatile ("di");
		asm volatile ("sync.p");
		asm volatile ("mfc0 %0, $12" : "=r" (sr));
		sr &= 0x10000;
		if (sr)
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
