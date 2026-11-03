/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions on */
/* __init_cpp_rts.c -- Metrowerks' C++ runtime: run a static initialiser table (MAIN 0x182840).
 * MAIN's mwInit (mwoverlay.c) passes the linker's __static_init table and the exception table's
 * bounds, an overlay's entry its own table and nothing; the bounds go unused here. The file name
 * is ours (no DWARF covers the runtime).
 *
 * The short loop's nops (`sltu v1; nop; bnez; nop`) are a compiler's: byte-exact under 3.0b38,
 * 3.0b50, 3.0b52, 3.0b22-020716 and 3.0.3-020716, two words off under 3.0b22-020926 -- one of the
 * calibration's discriminators (README "MAIN's Metrowerks runtime"). The walking pointer is a
 * local copied from the parameter: walking the parameter itself swaps s0 and s1. */

typedef void (*voidfunc)(void);

void __initialize_cpp_rts(voidfunc *ctor, voidfunc *end, void *eti_start, void *eti_end)
{
    voidfunc *p;

    for (p = ctor; p < end; p++)
        (*p)();
}
