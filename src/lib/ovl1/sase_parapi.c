/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder: a parameter block hung off the encoder's memory block (OVL1
 * 0x252748-0x252BF0) -- its constructor and destructor, and the API's accessors for its fields,
 * each behind the handle checks of sase_encapi.c (6100 a bad handle, 6110 not the encoder; 6000 out
 * of memory, 6120 a release failed). Placeholders: the codec's names are not in SOCOM 1. Built as
 * nelly.c is (ee-gcc 2.96, -O2 -G8: the host's allocator and free callback are $gp small data). */

typedef struct SaseHandle SaseHandle;

typedef struct {
	SaseHandle *owner;
	int f4;
	int f8;
	char pad12[8];
	short f20;
	short f22;
	unsigned short f24;
	unsigned short f26;
	unsigned short f28;
	unsigned char f30;
} EncParamBlk;

typedef struct {
	char pad0[76];
	EncParamBlk *params;
} EncMemBlk;

struct SaseHandle {
	long magic;
	long f8;
	long f16;
	unsigned char type;
	EncMemBlk *blk;
};

extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern void (*data_001D55AC)(void *);           /* the host's free callback */
extern int func_00253420(void *p, int n, float rate);
extern int func_00252EF8(void *p);
extern int func_002534D0(void *p);
extern int func_00252F70(void *p);

int func_00252748(SaseHandle *h)
{
	EncMemBlk *blk;
	EncParamBlk *p;
	int ret;

	if (h == 0 || h->magic != 0xface)
		return 6100;
	blk = h->blk;
	p = data_001D55A8(1, 32);
	blk->params = p;
	if (p == 0)
		return 6000;
	if (func_00253420(&p->f4, 160, 8000.0f) != 0)
		return 6000;
	ret = func_00252EF8(&p->f8);
	if (ret != 0)
		return 6000;
	p->owner = h;
	p->f20 = 0;
	p->f30 = 0;
	return ret;
}

int func_002527F8(SaseHandle *h)
{
	EncParamBlk *p;

	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	p = h->blk->params;
	if (func_002534D0(&p->f4) != 0)
		return 6120;
	if (func_00252F70(&p->f8) != 0)
		return 6120;
	if (p != 0)
		data_001D55AC(p);
	return 0;
}

int func_00252880(SaseHandle *h, unsigned short v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	h->blk->params->f26 = v;
	return 0;
}

int func_002528D0(SaseHandle *h, unsigned short *v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	*v = h->blk->params->f26;
	return 0;
}

int func_00252920(SaseHandle *h, unsigned short v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	h->blk->params->f24 = v;
	return 0;
}

int func_00252970(SaseHandle *h, unsigned short *v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	*v = h->blk->params->f24;
	return 0;
}

int func_002529C0(SaseHandle *h, unsigned short *v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	*v = h->blk->params->f28;
	return 0;
}

int func_00252A10(SaseHandle *h)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	h->blk->params->f30 = 1;
	return 0;
}

int func_00252A60(SaseHandle *h)
{
	EncParamBlk *p;

	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	p = h->blk->params;
	p->f30 = 0;
	p->f20 = 0;
	return 0;
}

int func_00252AB0(SaseHandle *h, short v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	h->blk->params->f22 = v;
	return 0;
}

int func_00252B00(SaseHandle *h, short *v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	*v = h->blk->params->f22;
	return 0;
}

int func_00252B50(SaseHandle *h, short v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	h->blk->params->f20 = v;
	return 0;
}

int func_00252BA0(SaseHandle *h, short *v)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	*v = h->blk->params->f20;
	return 0;
}
