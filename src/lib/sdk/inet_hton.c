/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O1 -G0 */
/* The socket library's byte-order pair (MAIN 0x1C1308-0x1C1348): htonl and htons (their rows keep
 * placeholders: no symbol names this library, it is not in the SOCOM 1 demo), the object before
 * inet_addr.c's address routines. GCC round 31a settled it as a separate object built at -O1:
 *
 *  - Both compute in strict source order -- the result in v0, one temporary in v1 reused for each
 *    term, the last `or` in the return's slot -- which -O2's schedulers never leave: under -O2 the
 *    andi is hoisted above the first sll (htonl 2 words, htons 3), under -O1 (no scheduling pass on
 *    this compiler) or -O2 -fno-schedule-insns -fno-schedule-insns2 the order is the source's.
 *  - htons's temporary is v1 only with its terms in explicit registers: every plain C spelling (an
 *    expression, two temporaries, one reused) puts its second term in the dying a0 on all thirteen
 *    ee-gcc toolchains at -O1 and at -O2 without the schedulers (3 words each,
 *    build/scratch/gcc/v_htons2.c); `register u_int r asm ("$2"), t asm ("$3")` -- the idiom of
 *    Sony's InitTLB32MB (README "The GCC front", round 3a) -- gives the target's under dtls13010 at
 *    -O1 and nowhere at -O2 with the schedulers (3 words); 2.96 misses by 7 (v_htons.c). htonl is
 *    byte-exact either way at -O1 (as one reused temporary, round 30a's finding, or with the same
 *    two registers): it is written as htons is.
 *  - The reading is BSD's hand-written MIPS htonl.S/htons.S (v0 the result, v1 the term, the `or`
 *    in `j ra`'s slot) in C operand order; an assembler object would give the same bytes, and the
 *    demo, which does not link this library, cannot tell the two apart. A C object at -O1 needs no
 *    assumption the rest of the library does not already make: its objects were built at -O2
 *    (inet_addr.c, inet_sock.c) and -O0 (inet_bsd.c) -- per-object optimisation levels. */

typedef unsigned int u_int;

u_int func_001C1308(u_int x)
{
	register u_int r asm ("$2");
	register u_int t asm ("$3");

	r = x << 24;
	t = x & 0xff00;
	t <<= 8;
	r |= t;
	t = x >> 8;
	t &= 0xff00;
	r |= t;
	t = x >> 24;
	r |= t;
	return r;
}

u_int func_001C1330(u_int x)
{
	register u_int r asm ("$2");
	register u_int t asm ("$3");

	r = x << 8;
	r &= 0xff00;
	t = x >> 8;
	t &= 0xff;
	r |= t;
	return r;
}
