/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions on */
/* mwoverlay.c -- the C++ runtime's start-up and MWo3 overlay entry (MAIN 0x181C90-0x181E23): run
 * MAIN's static initialisers, and make a loaded MWo3 overlay live -- invalidate the I-cache, clear
 * its .bss, run its static initialisers. The file name is ours (no DWARF covers it).
 *
 * SOCOM 1's runtime has the first of these as `mwInit` (0x144890, the same four linker symbols, at
 * the end of its runtime); SOCOM II's sits before NMWException.cp with the two overlay functions
 * the boot loader uses (loader.c's func_001C5DA0 calls func_00181D80 after inflating an overlay).
 * An MWo3 header (README "The target") holds the .bss size at +0x14 and the overlay's static
 * initialiser table at +0x18/+0x1C. Defined in reverse of the image (-inline deferred): the loader
 * inlines the entry, as the image's does. C; the runtime's flags (README "MAIN's Metrowerks
 * runtime") -- C code tail-calls under either -Cpp_exceptions. */

typedef void (*voidfunc)(void);

typedef struct {
    char magic[0x14];
    unsigned int bss_size;      /* 0x14 */
    voidfunc *sinit_start;      /* 0x18 */
    voidfunc *sinit_end;        /* 0x1C */
} MWo3Header;

extern voidfunc __static_init[];
extern voidfunc __static_init_end[];
extern char __exception_table_start__[];
extern char __exception_table_end__[];

void __initialize_cpp_rts(voidfunc *ctor, voidfunc *end, void *eti_start, void *eti_end);
void *memset(void *, int, unsigned int);
int FlushCache(int);
int sceOpen(const char *filename, int flag, ...);
int sceClose(int fd);
int sceRead(int fd, void *buf, int nbyte);
int sceLseek(int fd, int offset, int where);

/* SOCOM 1's mwInit: MAIN's own static initialisers */
void func_00181E00(void)
{
    __initialize_cpp_rts(__static_init, __static_init_end, __exception_table_start__, __exception_table_end__);
}

/* an MWo3 overlay in memory at `ovl`, `size` bytes of it loaded */
void func_00181D80(void *ovl, int size)
{
    char *p = (char *)ovl;      /* its own variable, numbered after the parameters: s2 (size s3) */
    MWo3Header *hdr = (MWo3Header *)ovl;
    unsigned int bss = hdr->bss_size;

    FlushCache(2);
    if (bss) {
        p += size;
        memset(p, 0, bss);
    }
    __initialize_cpp_rts(hdr->sinit_start, hdr->sinit_end, 0, 0);
}

/* an MWo3 overlay read whole from a file to `ovl`: 1 when it is live */
int func_00181C90(const char *path, void *ovl)
{
    int ok = 0;
    int size = 0;
    int fd = sceOpen(path, 1);

    if (fd >= 0) {
        size = sceLseek(fd, 0, 2);
        sceLseek(fd, 0, 0);
    }
    if (size > 0)
        size = sceRead(fd, ovl, size);
    if (fd >= 0)
        sceClose(fd);
    if (size > 0) {
        func_00181D80(ovl, size);
        ok = 1;
    }
    return ok;
}
