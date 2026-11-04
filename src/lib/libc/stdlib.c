/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdlib conversions (MAIN 0x191428-0x1914D4), from the libc.a of the ee-gcc 2.96
 * toolchain the game was linked with -- not Sony's SDK build (README "The GCC front": atof and
 * atol are 2.96's sibling calls under a leftover frame, which 2.9-ee-991111 never emits).
 * long is 64-bit on the EE: atoi's (int) is the dsll32/dsra32 pair after strtol. */

extern double strtod(const char *s, char **end);
extern long strtol(const char *s, char **end, int base);

int abs(int i)
{
    return (i < 0) ? -i : i;
}

double atof(const char *s)
{
    return strtod(s, 0);
}

int atoi(const char *s)
{
    return (int)strtol(s, 0, 10);
}

long atol(const char *s)
{
    return strtol(s, 0, 10);
}
