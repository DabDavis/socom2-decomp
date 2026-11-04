/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's signal/signal.c (MAIN 0x1984E8-0x1987CC), the toolchain's libc: the reentrancy block's
 * table of 32 handlers, signal and raise over it. The demo's object 0x3E5198 holds the same eight
 * in the same order. */

#include "newlib.h"

#define	EINVAL 22
#define	NSIG 32

typedef void (*_sig_func_ptr) (int);

#define	SIG_DFL ((_sig_func_ptr)0)	/* Default action */
#define	SIG_IGN ((_sig_func_ptr)1)	/* Ignore action */
#define	SIG_ERR ((_sig_func_ptr)-1)	/* Error return */

extern int _kill_r(struct _reent *, int, int);
extern int _getpid_r(struct _reent *);

int
_init_signal_r (ptr)
     struct _reent *ptr;
{
  int i;

  if (ptr->_sig_func == NULL)
    {
      ptr->_sig_func = (_sig_func_ptr *)_malloc_r (ptr, sizeof (_sig_func_ptr) * NSIG);
      if (ptr->_sig_func == NULL)
	return -1;

      for (i = 0; i < NSIG; i++)
	ptr->_sig_func[i] = SIG_DFL;
    }

  return 0;
}

_sig_func_ptr
_signal_r (ptr, sig, func)
     struct _reent *ptr;
     int sig;
     _sig_func_ptr func;
{
  _sig_func_ptr old_func;

  if (sig < 0 || sig >= NSIG)
    {
      ptr->_errno = EINVAL;
      return SIG_ERR;
    }

  if (ptr->_sig_func == NULL && _init_signal_r (ptr) != 0)
    return SIG_ERR;

  old_func = ptr->_sig_func[sig];
  ptr->_sig_func[sig] = func;

  return old_func;
}

int
_raise_r (ptr, sig)
     struct _reent *ptr;
     int sig;
{
  _sig_func_ptr func;

  if (sig < 0 || sig >= NSIG)
    {
      ptr->_errno = EINVAL;
      return -1;
    }

  if (ptr->_sig_func == NULL && _init_signal_r (ptr) != 0)
    return -1;

  func = ptr->_sig_func[sig];
  if (func == SIG_DFL)
    return _kill_r (ptr, _getpid_r (ptr), sig);
  else if (func == SIG_IGN)
    return 0;
  else if (func == SIG_ERR)
    {
      ptr->_errno = EINVAL;
      return 1;
    }
  else
    {
      ptr->_sig_func[sig] = SIG_DFL;
      func (sig);
      return 0;
    }
}

int
__sigtramp_r (ptr, sig)
     struct _reent *ptr;
     int sig;
{
  _sig_func_ptr func;

  if (sig < 0 || sig >= NSIG)
    {
      return -1;
    }

  if (ptr->_sig_func == NULL && _init_signal_r (ptr) != 0)
    return -1;

  func = ptr->_sig_func[sig];
  if (func == SIG_DFL)
    return 1;
  else if (func == SIG_ERR)
    return 2;
  else if (func == SIG_IGN)
    return 3;
  else
    {
      ptr->_sig_func[sig] = SIG_DFL;
      func (sig);
      return 0;
    }
}

int
raise (sig)
     int sig;
{
  return _raise_r (_REENT, sig);
}

_sig_func_ptr
signal (sig, func)
     int sig;
     _sig_func_ptr func;
{
  return _signal_r (_REENT, sig, func);
}

int
_init_signal ()
{
  return _init_signal_r (_REENT);
}

int
__sigtramp (int sig)
{
  return __sigtramp_r (_REENT, sig);
}
