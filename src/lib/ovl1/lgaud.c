/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* OVL1's prebuilt libraries before the speech codec (0x2430F0-0x248350): the recogniser's heap,
 * lgAud (the USB headset's audio, liblgaud.a 1.08.002, over SIF RPC to lgaud.irx) and SCEA's
 * libnetb/inet, all ee-gcc 2.96 at -O2 -G0 (README "The GCC front"; 2.96's sibling calls keep
 * the frame they no longer need: lgAudASync, sceBInetControl). */

extern int sceSifCheckStatRpc(void *cd);
extern char data_00414980[];                    /* lgAud's sceSifClientData */
extern int func_002464A8(int, int, int, int);

struct binet_ctl {
    int a, b;
    int arg;
};

int func_00243618(unsigned int *info)           /* the heap's version query */
{
    if (info == 0)
        return 0x800B0183;
    *info = 0x01000001;
    return 0;
}

int lgAudASync(int mode)
{
    if (mode == 0) {
        while (sceSifCheckStatRpc(data_00414980) != 0)
            ;
        return 0;
    }
    return sceSifCheckStatRpc(data_00414980);
}

int sceBInetControl(int s, int cmd, struct binet_ctl *p, int len)
{
    if (p == 0)
        return -512;
    return func_002464A8(s, cmd, p->arg, len);
}
