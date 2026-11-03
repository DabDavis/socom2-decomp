/* gcc_wrapper.c -- Metrowerks' names for the double compares, over the GCC soft-float runtime's
 * dpcmp (MAIN 0x181C50-0x181C8F). SOCOM 1's DWARF names the file, `C:\apps\cw301\PS2
 * Support\gcc_wrapper.c` -- CodeWarrior 3.0.1's PS2 support, built with the game (it carries
 * debug information, as the game's units do and the prebuilt runtime does not) -- and its lines:
 * each body is one return (lines 100-102, 76-78). SOCOM II strips _dpfeq (SOCOM 1's 0x142180,
 * after _dpflt). Byte-exact under every 3.0 build; the game's compiler and flags. */

int dpcmp(double a, double b);

int _dpfgt(double left, double right)
{
    return dpcmp(left, right) > 0;
}

int _dpflt(double left, double right)
{
    return dpcmp(left, right) < 0;
}
