/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's locale/locale.c (MAIN 0x194760-0x194844), the toolchain's libc: only the "C" locale
 * (no MB_CAPABLE), the static lconv. The demo's object 0x3E1A70 holds the same four. */

#include "newlib.h"

#define	CHAR_MAX 127

extern int strcmp(const char *, const char *);

struct lconv
{
  char *decimal_point;
  char *thousands_sep;
  char *grouping;
  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char int_frac_digits;
  char frac_digits;
  char p_cs_precedes;
  char p_sep_by_space;
  char n_cs_precedes;
  char n_sep_by_space;
  char p_sign_posn;
  char n_sign_posn;
};

static const struct lconv lconv =
{
  ".", "", "", "", "", "", "", "", "", "",
  CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
  CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
};

char *
_setlocale_r (p, category, locale)
     struct _reent *p;
     int category;
     const char *locale;
{
  if (locale)
    {
      if (strcmp (locale, "C") && strcmp (locale, ""))
	return 0;
      p->_current_category = category;
      p->_current_locale = locale;
    }
  return "C";
}

struct lconv *
_localeconv_r (data)
     struct _reent *data;
{
  return (struct lconv *) &lconv;
}

char *
setlocale (category, locale)
     int category;
     const char *locale;
{
  return _setlocale_r (_REENT, category, locale);
}

struct lconv *
localeconv ()
{
  return _localeconv_r (_REENT);
}
