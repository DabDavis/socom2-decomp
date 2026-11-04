/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdlib/getenv_r.c (MAIN 0x1945F8-0x19475C), the toolchain's libc: _findenv_r and
 * _getenv_r. The environment is reached through the static p_environ (data.txt, 0x1CC458), under
 * the environment lock -- this newlib returns NULL for an unset environ without unlocking. */

#include "newlib.h"

extern char **environ;
extern int strncmp(const char *, const char *, size_t);
extern void __env_lock(struct _reent *);
extern void __env_unlock(struct _reent *);

#define ENV_LOCK __env_lock(reent_ptr)
#define ENV_UNLOCK __env_unlock(reent_ptr)

/* Only deal with a pointer to environ, to work around subtle bugs with shared
   libraries and/or small data systems where the user declares his own
   'environ'.  */
static char ***p_environ = &environ;

/*
 * _findenv --
 *	Returns pointer to value associated with name, if any, else NULL.
 *	Sets offset to be the offset of the name/value combination in the
 *	environmental array, for use by setenv(3) and unsetenv(3).
 *	Explicitly removes '=' in argument name.
 *
 *	This routine *should* be a static; don't use it.
 */

char *
_findenv_r (reent_ptr, name, offset)
     struct _reent *reent_ptr;
     register const char *name;
     int *offset;
{
  register int len;
  register char **p;
  const char *c;

  ENV_LOCK;

  /* In some embedded systems, this does not get set.  This protects
     newlib from dereferencing a bad pointer.  */
  if (!*p_environ)
    return NULL;

  c = name;
  len = 0;
  while (*c && *c != '=')
    {
      c++;
      len++;
    }

  for (p = *p_environ; *p; ++p)
    if (!strncmp (*p, name, len))
      if (*(c = *p + len) == '=')
	{
	  *offset = p - *p_environ;
	  ENV_UNLOCK;
	  return (char *) (++c);
	}
  ENV_UNLOCK;
  return NULL;
}

/*
 * _getenv_r --
 *	Returns ptr to value associated with name, if any, else NULL.
 */

char *
_getenv_r (reent_ptr, name)
     struct _reent *reent_ptr;
     const char *name;
{
  int offset;

  return _findenv_r (reent_ptr, name, &offset);
}
