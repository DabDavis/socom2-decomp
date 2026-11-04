/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/vfscanf.c (MAIN 0x19DA58-0x19E7E4), the toolchain's libc: __sccl, the %[...] scanset
 * builder, as newlib wrote it (1.8.2 and 1.9.0 alike); the demo's vfscanf object holds the same
 * pair, __svfscanf then __sccl. Not written: __svfscanf (0xC8C) -- not newlib's text as 1.8.2 or
 * 1.9.0 has it: it calls atol and sprintf where both releases (and the demo's 0xB08 copy) convert
 * integers through the ccfn pointer (strtol/strtoul) and floats with atof; the toolchain's own
 * modified vfscanf.c, not yet reconstructed. */

#define u_char char
#define _CONST const

/*
 * Fill in the given table from the scanset at the given format
 * (just after `[').  Return a pointer to the character past the
 * closing `]'.  The table has a 1 wherever characters should be
 * considered part of the scanset.
 */

u_char *
__sccl (tab, fmt)
     register char *tab;
     register u_char *fmt;
{
  register int c, n, v;

  /* first `clear' the whole table */
  c = *fmt++;			/* first char hat => negated scanset */
  if (c == '^')
    {
      v = 1;			/* default => accept */
      c = *fmt++;		/* get new first char */
    }
  else
    v = 0;			/* default => reject */
  /* should probably use memset here */
  for (n = 0; n < 256; n++)
    tab[n] = v;
  if (c == 0)
    return fmt - 1;		/* format ended before closing ] */

  /*
   * Now set the entries corresponding to the actual scanset to the
   * opposite of the above.
   *
   * The first character may be ']' (or '-') without being special; the
   * last character may be '-'.
   */

  v = 1 - v;
  for (;;)
    {
      tab[c] = v;		/* take character c */
    doswitch:
      n = *fmt++;		/* and examine the next */
      switch (n)
	{

	case 0:		/* format ended too soon */
	  return fmt - 1;

	case '-':
	  /*
	   * A scanset of the form [01+-] is defined as `the digit 0, the
	   * digit 1, the character +, the character -', but the effect of a
	   * scanset such as [a-zA-Z0-9] is implementation defined.  The V7
	   * Unix scanf treats `a-z' as `the letters a through z', but treats
	   * `a-a' as `the letter a, the character -, and the letter a'.
	   *
	   * For compatibility, the `-' is not considerd to define a range if
	   * the character following it is either a close bracket (required by
	   * ANSI) or is not numerically greater than the character we just
	   * stored in the table (c).
	   */
	  n = *fmt;
	  if (n == ']' || n < c)
	    {
	      c = '-';
	      break;		/* resume the for(;;) */
	    }
	  fmt++;
	  do
	    {			/* fill in the range */
	      tab[++c] = v;
	    }
	  while (c < n);
#if 1			/* XXX another disgusting compatibility hack */
	  /*
	   * Alas, the V7 Unix scanf also treats formats such
	   * as [a-c-e] as `the letters a through e'. This too
	   * is permitted by the standard....
	   */
	  goto doswitch;
#else
	  c = *fmt++;
	  if (c == 0)
	    return fmt - 1;
	  if (c == ']')
	    return fmt;
#endif

	  break;


	case ']':		/* end of scanset */
	  return fmt;

	default:		/* just another character */
	  c = n;
	  break;
	}
    }
  /* NOTREACHED */
}
