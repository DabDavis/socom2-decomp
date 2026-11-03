/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_intr.c: a count of the INTC handlers installed (OVL2 0x6311E8-0x6313D8), in SCEA's RT library,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). Between rt_crypt.c's ciphers and
 * rt_list.c; it has no $Header, no data and no SOCOM 1 counterpart, so the file name (between those
 * two in the alphabetical link order) and every name are ours. For each of the eleven interrupt
 * causes 0-10 it adds a handler that does nothing, removes it again and sums what the removal
 * returns -- the kernel's count of the handlers left on that cause. Its three functions stood as
 * matching m2c drafts (round 11b). */

extern int AddIntcHandler(int cause, int (*handler)(int), int next);
extern int RemoveIntcHandler(int cause, int hid);

int func_00631388(int nCause);
int func_00631390(int nCause, int nId, int *pnCount);

int func_006311E8(int *pnCount)
{
    if (pnCount == 0)
        return 2;
    func_00631390(0, AddIntcHandler(0, func_00631388, -1), pnCount);
    func_00631390(1, AddIntcHandler(1, func_00631388, -1), pnCount);
    func_00631390(2, AddIntcHandler(2, func_00631388, -1), pnCount);
    func_00631390(3, AddIntcHandler(3, func_00631388, -1), pnCount);
    func_00631390(4, AddIntcHandler(4, func_00631388, -1), pnCount);
    func_00631390(5, AddIntcHandler(5, func_00631388, -1), pnCount);
    func_00631390(6, AddIntcHandler(6, func_00631388, -1), pnCount);
    func_00631390(7, AddIntcHandler(7, func_00631388, -1), pnCount);
    func_00631390(8, AddIntcHandler(8, func_00631388, -1), pnCount);
    func_00631390(9, AddIntcHandler(9, func_00631388, -1), pnCount);
    func_00631390(10, AddIntcHandler(10, func_00631388, -1), pnCount);
    return 0;
}

/* the handler installed: it does nothing */
int func_00631388(int nCause)
{
    return 0;
}

/* removes the handler just added, summing the handlers the kernel reports left on its cause */
int func_00631390(int nCause, int nId, int *pnCount)
{
    int n;

    if (nId >= 0) {
        n = RemoveIntcHandler(nCause, nId);
        if (n > 0)
            *pnCount += n;
    }
    return 0;
}
