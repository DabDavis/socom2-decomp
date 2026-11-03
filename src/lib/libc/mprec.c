/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/mprec.c (MAIN 0x195B48-0x196B5C), the toolchain's libc: David Gay's multiple-
 * precision helpers for dtoa and strtod (Pack_32, IEEE_8087 -- the EE is little-endian -- and
 * no Sudden_Underflow). The demo's object 0x3E2D58 holds the same seventeen in the same order.
 * __ULong and __Long are int here (long is 64-bit on the EE, the digits are words: Balloc's size
 * is 20 + 4 * 2^k). The tables (tens, bigtens, tinytens) are data.txt's __mprec_* rows.
 *
 * OPEN, the runtime compiler's double moves (README "The GCC front", round 4a: this toolchain's
 * 2.96 is not the runtime's, which never puts a DFmode register move in a delay slot): _ulp (the
 * target's return copy into v0 sits before `jr ra; nop`, here in the slot), _ratio (the same, 9
 * words with the frame), _d2b (5 words: y's spill fills the lo0bits call's slot here, the
 * target keeps the DFmode-derived word in a1). Every integer helper is byte-exact as written. */

#include "newlib.h"

typedef unsigned int __ULong;
typedef int __Long;
typedef struct _Bigint _Bigint;

union double_union
{
  double d;
  __ULong i[2];
};

#define word0(x) (x.i[1])
#define word1(x) (x.i[0])

#define Exp_shift  20
#define Exp_shift1 20
#define Exp_msk1    ((__ULong)0x100000L)
#define Exp_msk11   ((__ULong)0x100000L)
#define Exp_mask  ((__ULong)0x7ff00000L)
#define P 53
#define Bias 1023
#define Exp_1  ((__ULong)0x3ff00000L)
#define Ebits 11
#define Frac_mask  ((__ULong)0xfffffL)

#define Storeinc(a,b,c) (((unsigned short *)a)[1] = (unsigned short)b, \
((unsigned short *)a)[0] = (unsigned short)c, a++)

#define Bcopy(x,y) memcpy((char *)&x->_sign, (char *)&y->_sign, y->_wds*sizeof(__Long) + 2*sizeof(int))

#define	Balloc	_Balloc
#define	Bfree	_Bfree
#define	multadd	_multadd
#define	s2b	_s2b
#define	lo0bits	_lo0bits
#define	hi0bits	_hi0bits
#define	i2b	_i2b
#define	mult	_multiply
#define	pow5mult	_pow5mult
#define	lshift	_lshift
#define	cmp	__mcmp
#define	diff	__mdiff
#define	ulp 	_ulp
#define	b2d	_b2d
#define	d2b	_d2b
#define	ratio	_ratio
#define	tens __mprec_tens
#define	bigtens __mprec_bigtens
#define	tinytens __mprec_tinytens

extern _Bigint *Balloc(struct _reent *p, int k);
extern void Bfree(struct _reent *p, _Bigint *v);
extern int hi0bits(__ULong);
extern int lo0bits(__ULong *);
extern void *_calloc_r(struct _reent *, size_t, size_t);
extern double _ulp(double x);

_Bigint *
Balloc (ptr, k)
     struct _reent *ptr;
     int k;
{
  int x;
  _Bigint *rv ;

  if (ptr->_freelist == NULL)
    {
      /* Allocate a list of pointers to the mprec objects */
      ptr->_freelist = (struct _Bigint **) _calloc_r (ptr,
						      sizeof (struct _Bigint *),
						      16);
      if (ptr->_freelist == NULL)
	{
	  return NULL;
	}
    }

  if ((rv = ptr->_freelist[k]) != 0)
    {
      ptr->_freelist[k] = rv->_next;
    }
  else
    {
      x = 1 << k;
      /* Allocate an mprec Bigint and stick in in the freelist */
      rv = (_Bigint *) _calloc_r (ptr,
				  1,
				  sizeof (_Bigint) +
				  (x-1) * sizeof(rv->_x));
      if (rv == NULL) return NULL;
      rv->_k = k;
      rv->_maxwds = x;
    }
  rv->_sign = rv->_wds = 0;
  return rv;
}

void
Bfree (ptr, v)
     struct _reent *ptr;
     _Bigint * v;
{
  if (v)
    {
      v->_next = ptr->_freelist[v->_k];
      ptr->_freelist[v->_k] = v;
    }
}

_Bigint *
multadd (ptr, b, m, a)
     struct _reent *ptr;
     _Bigint * b;
     int m;
     int a;
{
  int i, wds;
  __ULong *x, y;
  __ULong xi, z;
  _Bigint *b1;

  wds = b->_wds;
  x = b->_x;
  i = 0;
  do
    {
      xi = *x;
      y = (xi & 0xffff) * m + a;
      z = (xi >> 16) * m + (y >> 16);
      a = (int) (z >> 16);
      *x++ = (z << 16) + (y & 0xffff);
    }
  while (++i < wds);
  if (a)
    {
      if (wds >= b->_maxwds)
	{
	  b1 = Balloc (ptr, b->_k + 1);
	  Bcopy (b1, b);
	  Bfree (ptr, b);
	  b = b1;
	}
      b->_x[wds++] = a;
      b->_wds = wds;
    }
  return b;
}

_Bigint *
s2b (ptr, s, nd0, nd, y9)
     struct _reent * ptr;
     const char *s;
     int nd0;
     int nd;
     __ULong y9;
{
  _Bigint *b;
  int i, k;
  __Long x, y;

  x = (nd + 8) / 9;
  for (k = 0, y = 1; x > y; y <<= 1, k++);
  b = Balloc (ptr, k);
  b->_x[0] = y9;
  b->_wds = 1;

  i = 9;
  if (9 < nd0)
    {
      s += 9;
      do
	b = multadd (ptr, b, 10, *s++ - '0');
      while (++i < nd0);
      s++;
    }
  else
    s += 10;
  for (; i < nd; i++)
    b = multadd (ptr, b, 10, *s++ - '0');
  return b;
}

int
hi0bits (x)
     register __ULong x;
{
  register int k = 0;

  if (!(x & 0xffff0000))
    {
      k = 16;
      x <<= 16;
    }
  if (!(x & 0xff000000))
    {
      k += 8;
      x <<= 8;
    }
  if (!(x & 0xf0000000))
    {
      k += 4;
      x <<= 4;
    }
  if (!(x & 0xc0000000))
    {
      k += 2;
      x <<= 2;
    }
  if (!(x & 0x80000000))
    {
      k++;
      if (!(x & 0x40000000))
	return 32;
    }
  return k;
}

int
lo0bits (y)
     __ULong *y;
{
  register int k;
  register __ULong x = *y;

  if (x & 7)
    {
      if (x & 1)
	return 0;
      if (x & 2)
	{
	  *y = x >> 1;
	  return 1;
	}
      *y = x >> 2;
      return 2;
    }
  k = 0;
  if (!(x & 0xffff))
    {
      k = 16;
      x >>= 16;
    }
  if (!(x & 0xff))
    {
      k += 8;
      x >>= 8;
    }
  if (!(x & 0xf))
    {
      k += 4;
      x >>= 4;
    }
  if (!(x & 0x3))
    {
      k += 2;
      x >>= 2;
    }
  if (!(x & 1))
    {
      k++;
      x >>= 1;
      if (!x & 1)
	return 32;
    }
  *y = x;
  return k;
}

_Bigint *
i2b (ptr, i)
     struct _reent * ptr;
     int i;
{
  _Bigint *b;

  b = Balloc (ptr, 1);
  b->_x[0] = i;
  b->_wds = 1;
  return b;
}

_Bigint *
mult (ptr, a, b)
     struct _reent * ptr;
     _Bigint * a;
     _Bigint * b;
{
  _Bigint *c;
  int k, wa, wb, wc;
  __ULong carry, y, z;
  __ULong *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
  __ULong z2;

  if (a->_wds < b->_wds)
    {
      c = a;
      a = b;
      b = c;
    }
  k = a->_k;
  wa = a->_wds;
  wb = b->_wds;
  wc = wa + wb;
  if (wc > a->_maxwds)
    k++;
  c = Balloc (ptr, k);
  for (x = c->_x, xa = x + wc; x < xa; x++)
    *x = 0;
  xa = a->_x;
  xae = xa + wa;
  xb = b->_x;
  xbe = xb + wb;
  xc0 = c->_x;
  for (; xb < xbe; xb++, xc0++)
    {
      if ((y = *xb & 0xffff) != 0)
	{
	  x = xa;
	  xc = xc0;
	  carry = 0;
	  do
	    {
	      z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
	      carry = z >> 16;
	      z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
	      carry = z2 >> 16;
	      Storeinc (xc, z2, z);
	    }
	  while (x < xae);
	  *xc = carry;
	}
      if ((y = *xb >> 16) != 0)
	{
	  x = xa;
	  xc = xc0;
	  carry = 0;
	  z2 = *xc;
	  do
	    {
	      z = (*x & 0xffff) * y + (*xc >> 16) + carry;
	      carry = z >> 16;
	      Storeinc (xc, z, z2);
	      z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
	      carry = z2 >> 16;
	    }
	  while (x < xae);
	  *xc = z2;
	}
    }
  for (xc0 = c->_x, xc = xc0 + wc; wc > 0 && !*--xc; --wc);
  c->_wds = wc;
  return c;
}

_Bigint *
pow5mult (ptr, b, k)
     struct _reent * ptr;
     _Bigint * b;
     int k;
{
  _Bigint *b1, *p5, *p51;
  int i;
  static const int p05[3] = {5, 25, 125};

  if ((i = k & 3) != 0)
    b = multadd (ptr, b, p05[i - 1], 0);

  if (!(k >>= 2))
    return b;
  if (!(p5 = ptr->_p5s))
    {
      /* first time */
      p5 = ptr->_p5s = i2b (ptr, 625);
      p5->_next = 0;
    }
  for (;;)
    {
      if (k & 1)
	{
	  b1 = mult (ptr, b, p5);
	  Bfree (ptr, b);
	  b = b1;
	}
      if (!(k >>= 1))
	break;
      if (!(p51 = p5->_next))
	{
	  p51 = p5->_next = mult (ptr, p5, p5);
	  p51->_next = 0;
	}
      p5 = p51;
    }
  return b;
}

_Bigint *
lshift (ptr, b, k)
     struct _reent * ptr;
     _Bigint * b;
     int k;
{
  int i, k1, n, n1;
  _Bigint *b1;
  __ULong *x, *x1, *xe, z;

  n = k >> 5;
  k1 = b->_k;
  n1 = n + b->_wds + 1;
  for (i = b->_maxwds; n1 > i; i <<= 1)
    k1++;
  b1 = Balloc (ptr, k1);
  x1 = b1->_x;
  for (i = 0; i < n; i++)
    *x1++ = 0;
  x = b->_x;
  xe = x + b->_wds;
  if (k &= 0x1f)
    {
      k1 = 32 - k;
      z = 0;
      do
	{
	  *x1++ = *x << k | z;
	  z = *x++ >> k1;
	}
      while (x < xe);
      if ((*x1 = z) != 0)
	++n1;
    }
  else
    do
      *x1++ = *x++;
    while (x < xe);
  b1->_wds = n1 - 1;
  Bfree (ptr, b);
  return b1;
}

int
cmp (a, b)
     _Bigint * a;
     _Bigint * b;
{
  __ULong *xa, *xa0, *xb, *xb0;
  int i, j;

  i = a->_wds;
  j = b->_wds;
  if (i -= j)
    return i;
  xa0 = a->_x;
  xa = xa0 + j;
  xb0 = b->_x;
  xb = xb0 + j;
  for (;;)
    {
      if (*--xa != *--xb)
	return *xa < *xb ? -1 : 1;
      if (xa <= xa0)
	break;
    }
  return 0;
}

_Bigint *
diff (ptr, a, b)
     struct _reent * ptr;
     _Bigint * a;
     _Bigint * b;
{
  _Bigint *c;
  int i, wa, wb;
  __Long borrow, y;		/* We need signed shifts here. */
  __ULong *xa, *xae, *xb, *xbe, *xc;
  __Long z;

  i = cmp (a, b);
  if (!i)
    {
      c = Balloc (ptr, 0);
      c->_wds = 1;
      c->_x[0] = 0;
      return c;
    }
  if (i < 0)
    {
      c = a;
      a = b;
      b = c;
      i = 1;
    }
  else
    i = 0;
  c = Balloc (ptr, a->_k);
  c->_sign = i;
  wa = a->_wds;
  xa = a->_x;
  xae = xa + wa;
  wb = b->_wds;
  xb = b->_x;
  xbe = xb + wb;
  xc = c->_x;
  borrow = 0;
  do
    {
      y = (*xa & 0xffff) - (*xb & 0xffff) + borrow;
      borrow = y >> 16;
      z = (*xa++ >> 16) - (*xb++ >> 16) + borrow;
      borrow = z >> 16;
      Storeinc (xc, z, y);
    }
  while (xb < xbe);
  while (xa < xae)
    {
      y = (*xa & 0xffff) + borrow;
      borrow = y >> 16;
      z = (*xa++ >> 16) + borrow;
      borrow = z >> 16;
      Storeinc (xc, z, y);
    }
  while (!*--xc)
    wa--;
  c->_wds = wa;
  return c;
}

double
ulp (_x)
     double _x;
{
  union double_union x, a;
  register __Long L;

  x.d = _x;

  L = (word0 (x) & Exp_mask) - (P - 1) * Exp_msk1;
  if (L > 0)
    {
      word0 (a) = L;
      word1 (a) = 0;
    }
  else
    {
      L = -L >> Exp_shift;
      if (L < Exp_shift)
	{
	  word0 (a) = 0x80000 >> L;
	  word1 (a) = 0;
	}
      else
	{
	  word0 (a) = 0;
	  L -= Exp_shift;
	  word1 (a) = L >= 31 ? 1 : 1 << (31 - L);
	}
    }
  return a.d;
}

double
b2d (a, e)
     _Bigint * a;
     int *e;
{
  __ULong *xa, *xa0, w, y, z;
  int k;
  union double_union d;
#define d0 word0(d)
#define d1 word1(d)

  xa0 = a->_x;
  xa = xa0 + a->_wds;
  y = *--xa;
  k = hi0bits (y);
  *e = 32 - k;
  if (k < Ebits)
    {
      d0 = Exp_1 | y >> (Ebits - k);
      w = xa > xa0 ? *--xa : 0;
      d1 = y << ((32 - Ebits) + k) | w >> (Ebits - k);
      goto ret_d;
    }
  z = xa > xa0 ? *--xa : 0;
  if (k -= Ebits)
    {
      d0 = Exp_1 | y << k | z >> (32 - k);
      y = xa > xa0 ? *--xa : 0;
      d1 = z << k | y >> (32 - k);
    }
  else
    {
      d0 = Exp_1 | y;
      d1 = z;
    }
ret_d:
#undef d0
#undef d1
  return d.d;
}

_Bigint *
d2b (ptr, _d, e, bits)
     struct _reent * ptr;
     double _d;
     int *e;
     int *bits;
{
  union double_union d;
  _Bigint *b;
  int de, i, k;
  __ULong *x, y, z;
#define d0 word0(d)
#define d1 word1(d)
  d.d = _d;

  b = Balloc (ptr, 1);
  x = b->_x;

  z = d0 & Frac_mask;
  d0 &= 0x7fffffff;		/* clear sign bit, which we ignore */
  if ((de = (int) (d0 >> Exp_shift)) != 0)
    z |= Exp_msk1;
  if ((y = d1) != 0)
    {
      if ((k = lo0bits (&y)) != 0)
	{
	  x[0] = y | z << (32 - k);
	  z >>= k;
	}
      else
	x[0] = y;
      i = b->_wds = (x[1] = z) ? 2 : 1;
    }
  else
    {
      k = lo0bits (&z);
      x[0] = z;
      i = b->_wds = 1;
      k += 32;
    }
  if (de)
    {
      *e = de - Bias - (P - 1) + k;
      *bits = P - k;
    }
  else
    {
      *e = de - Bias - (P - 1) + 1 + k;
      *bits = 32 * i - hi0bits (x[i - 1]);
    }
  return b;
}
#undef d0
#undef d1

double
ratio (a, b)
     _Bigint * a;
     _Bigint * b;
{
  union double_union da, db;
  int k, ka, kb;

  da.d = b2d (a, &ka);
  db.d = b2d (b, &kb);
  k = ka - kb + 32 * (a->_wds - b->_wds);
  if (k > 0)
    word0 (da) += k * Exp_msk1;
  else
    {
      k = -k;
      word0 (db) += k * Exp_msk1;
    }
  return da.d / db.d;
}

extern const double tens[];

double
_mprec_log10 (dig)
     int dig;
{
  double v = 1.0;
  if (dig < 24)
    return tens[dig];
  while (dig > 0)
    {
      v *= 10;
      dig--;
    }
  return v;
}
