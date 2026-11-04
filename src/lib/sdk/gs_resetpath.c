/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsResetPath (MAIN 0x1A1570): the demo's object 0x14E0F8 (C), one function. Resets
 * the paths to the GS: VIF1 (FBRST RST, ERR's ME0 mask), VU1 (FBRST's bit 9 through the
 * coprocessor's vi28), VIF1's registers through its FIFO (init_vif_regs: STCYCL 4/4, STMASK 0,
 * STMOD 0, MSKPATH3 0, BASE 0, OFFSET 0, ITOP 0) and the GIF (CTRL RST). Name: SOCOM 1's
 * (s1:exact; the local static the demo's `init_vif_regs.3`, data.txt's row). */

typedef unsigned int u_int;
typedef int u_long128 __attribute__((mode(TI)));

#define VIF1_FBRST	((volatile u_int *)0x10003C10)
#define VIF1_ERR	((volatile u_int *)0x10003C20)
#define VIF1_FIFO	((volatile u_long128 *)0x10005000)
#define GIF_CTRL	((volatile u_int *)0x10003000)

void sceGsResetPath(void)
{
	static u_int init_vif_regs[8] __attribute__((aligned(16))) = {
		0x01000404, 0x20000000, 0x00000000, 0x05000000,
		0x06000000, 0x03000000, 0x02000000, 0x04000000,
	};

	*VIF1_FBRST = 1;
	*VIF1_ERR = 2;
	__asm__ __volatile__(
		"sync\n"
		"cfc2 %0, $vi28\n"
		"ori %0, %0, 0x200\n"
		"ctc2 %0, $vi28\n"
		"sync.p"
		: : "r" (0));
	*VIF1_FIFO = *(u_long128 *)&init_vif_regs[0];
	*VIF1_FIFO = *(u_long128 *)&init_vif_regs[4];
	*GIF_CTRL = 1;
}
