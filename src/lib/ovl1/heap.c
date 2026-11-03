/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The speech recogniser's heap (heap_*), OVL1 0x2430F0-0x243840: ScanSoft's, the demo's object
 * 0x378348 (C, nine functions, there inside the recogniser), prebuilt with ee-gcc 2.96 at -O2 -G0
 * and, as the recogniser itself, -funroll-loops (README "The GCC front": SearchFor's loop is unrolled
 * by four, which no other flag set gives). Results are the library's codes (0x800B0183: a null
 * argument). Names are SOCOM 1's (s1:exact/callee/order). Not yet written: heap_Alloc, heap_Calloc,
 * heap_Realloc, heap_Free, heap_GetMemInfo (the generated tier holds what matched). */

typedef unsigned int u_int;

struct heap_list {
	u_int count;
	int *items;
};

int heap_GetVersion(u_int *version)
{
	if (version == 0)
		return 0x800B0183;
	*version = 0x01000001;
	return 0;
}

int SearchFor(struct heap_list *list, int item)
{
	u_int i;

	if (list != 0 && item != 0) {
		for (i = 0; i < list->count; i++)
			if (item == list->items[i])
				return i;
	}
	return -1;
}
