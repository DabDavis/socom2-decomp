/* libmpeg's types (Sony's SDK): the public sceMpeg and its callbacks as libmpeg.h declares them,
 * and the decoder's private state (sceMpeg.sys, 0x10C0 bytes: sceMpegCreate refuses a smaller
 * work area and hands the rest to the allocator). The private fields are named by what the code
 * does with them; their offsets are the target's. */

#ifndef SC_LIBMPEG_H
#define SC_LIBMPEG_H

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;
typedef unsigned long u_long;
typedef int u_long128 __attribute__((mode(TI)));

#define IPU_CMD		((volatile u_long *)0x10002000)
#define IPU_CMD_W	((volatile u_int *)0x10002000)	/* a command is written as a word */
#define IPU_CTRL	((volatile u_int *)0x10002010)
#define IPU_BP		((volatile u_int *)0x10002020)
#define IPU_TOP		((volatile u_long *)0x10002030)
/* the same registers through plain pointers: GCC folds a non-volatile access's constant address into
 * the instruction, which GNU as expands (`ld $4,0x10002030` -> `lui $4,0x1000; ld $4,0x2030($4)`, a
 * store through $at) -- the target's IPU_TOP reads and the BDEC error's IPU reset (GCC round 20a) */
#define IPU_TOP_NV	((u_long *)0x10002030)
#define IPU_CTRL_NV	((u_int *)0x10002010)
#define D3_CHCR		((volatile u_int *)0x1000B000)
#define D3_MADR		((volatile u_int *)0x1000B010)
#define D3_QWC		((volatile u_int *)0x1000B020)
#define D4_CHCR		((volatile u_int *)0x1000B400)
#define D4_MADR		((volatile u_int *)0x1000B410)
#define D4_QWC		((volatile u_int *)0x1000B420)
#define D4_TADR		((volatile u_int *)0x1000B430)
#define D9_CHCR		((volatile u_int *)0x1000D400)
#define D9_QWC		((volatile u_int *)0x1000D420)
#define D_ENABLER	((volatile u_int *)0x1000F520)

#define D_ENABLEW	((volatile u_int *)0x1000F590)

typedef struct {
	int width;
	int height;
	int frameCount;
	long pts;
	long dts;
	u_long flags;
	long pts2nd;
	long dts2nd;
	u_long flags2nd;
	void *sys;
} sceMpeg;

typedef enum {
	sceMpegCbError,
	sceMpegCbNodata,
	sceMpegCbStopDMA,
	sceMpegCbRestartDMA,
	sceMpegCbBackground,
	sceMpegCbTimeStamp,
	sceMpegCbStr
} sceMpegCbType;

typedef struct {
	sceMpegCbType type;
	char *errMessage;
} sceMpegCbDataError;

typedef struct {
	sceMpegCbType type;
	long pts;
	long dts;
} sceMpegCbDataTimeStamp;

typedef struct {
	sceMpegCbType type;
	u_char *header;
	u_char *data;
	u_int len;
	long pts;
	long dts;
} sceMpegCbDataStr;

typedef union {
	sceMpegCbType type;
	sceMpegCbDataError error;
	sceMpegCbDataTimeStamp ts;
	sceMpegCbDataStr str;
} sceMpegCbData;

typedef int (*sceMpegCallback)(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData);

typedef struct {
	sceMpegCallback func;
	void *data;
} MpegCb;

/* the work area's allocator (_alalc*) */
typedef struct {
	u_int base;		/* 0x0 */
	u_int size;		/* 0x4 */
	u_int cur;		/* 0x8 */
	u_int mark;		/* 0xC: what _alalcFree returns to */
} MpegAlloc;

/* libipu's DMA state, saved and restored around the decoder's own transfers */
typedef struct {
	u_int d4madr;
	u_int d4tadr;
	u_int d4qwc;
	u_int d4chcr;
	u_int d3madr;
	u_int d3qwc;
	u_int d3chcr;
	u_int ipubp;
	u_int ipuctrl;
} sceIpuDmaEnv;

/* a reference picture's record (0x68 bytes, nine of them from 0x1E8) */
typedef struct {
	u_int buf;		/* 0x00: uncached */
	int width;		/* 0x04 */
	int height;		/* 0x08 */
	int mbw;		/* 0x0C */
	int mbh;		/* 0x10 */
	int _14;		/* 0x14 */
	long pts;		/* 0x18: _getPtsDtsFlags reads both (GCC round 30a) */
	long dts;		/* 0x20 */
	int valid;		/* 0x28 */
	int _2c, _30, _34, _38, _3c, _40;	/* 0x2C: the picture's flags, packed into sceMpeg.flags */
	int centre[6];		/* 0x44: copied to MpegDec._0b4 ... _0c8 (sceMpegDispCenterOffX/Y's) */
	int dispWidth;		/* 0x5C */
	int dispHeight;		/* 0x60 */
	int _64;		/* 0x64 */
} MpegRef;

/* a macroblock's record (0x140 bytes, two of them from 0x590): BDEC's output and what the motion
 * compensation reads (GCC round 20a) */
typedef struct {
	u_int mc;		/* 0x000: a 6 KB scratchpad buffer (uncached) */
	u_int coef;		/* 0x004: BDEC's 384 coefficients, 768 bytes of scratchpad */
	char pad008[0x13C - 0x008];
	int noCoef;		/* 0x13C: neither intra nor coded: the prediction alone */
} MpegMB;

typedef struct {
	int isEnd;		/* 0x000 */
	int nFrames;		/* 0x004: frames decoded and not yet output */
	int started;		/* 0x008 */
	MpegCb cb[7];		/* 0x00C: by sceMpegCbType */
	void *work;		/* 0x044: 0x600 bytes from the allocator */
	int _048;		/* 0x048 */
	sceIpuDmaEnv dmaEnv;	/* 0x04C */
	int ptsGapSet;		/* 0x070 */
	long ptsGap;		/* 0x078 */
	int _080;		/* 0x080 */
	long _088;		/* 0x088 */
	int _090;		/* 0x090 */
	int modeI;		/* 0x094: sceMpegSetDecodeMode's three */
	int modeP;		/* 0x098 */
	int modeB;		/* 0x09C */
	int nI;			/* 0x0A0: pictures of each type seen */
	int nP;			/* 0x0A4 */
	int nB;			/* 0x0A8 */
	int frameBase;		/* 0x0AC */
	int csc;		/* 0x0B0: 1 for sceMpegGetPicture (colour-converted), 0 for RAW8 */
	int _0b4;		/* 0x0B4 */
	int _0b8;		/* 0x0B8 */
	int _0bc;		/* 0x0BC */
	int _0c0;		/* 0x0C0 */
	int _0c4;		/* 0x0C4 */
	int _0c8;		/* 0x0C8 */
	int dispWidth;		/* 0x0CC */
	int dispHeight;		/* 0x0D0 */
	int firstStructure;	/* 0x0D4 */
	u_int imageBuf;		/* 0x0D8: uncached */
	int imageX;		/* 0x0DC */
	int imageY;		/* 0x0E0 */
	int imageSize;		/* 0x0E4 */
	int brokenLink;		/* 0x0E8 */
	char pad0ec[0xF0 - 0xEC];
	long ptm;		/* 0x0F0 */
	int ptmSet;		/* 0x0F8 */
	u_int frameBuf[3];	/* 0x0FC: the three pictures' memory */
	MpegAlloc alloc;	/* 0x108 */
	int frameNo;		/* 0x118 */
	int Fault_Flag;		/* 0x11C: mpeg2decode's: a VLC the IPU could not decode, an error */
	int secondField;	/* 0x120 */
	int horizontal_size;	/* 0x124 */
	int vertical_size;	/* 0x128 */
	int mb_width;		/* 0x12C */
	int mb_height;		/* 0x130 */
	int bit_rate;		/* 0x134 */
	int vbv_buffer_size;	/* 0x138 */
	int progressive_sequence;	/* 0x13C */
	int chroma_format;	/* 0x140 */
	int matrix_coefficients;	/* 0x144 */
	int display_horizontal_size;	/* 0x148 */
	int display_vertical_size;	/* 0x14C */
	int picture_coding_type;	/* 0x150 */
	int full_pel_forward_vector;	/* 0x154 */
	int forward_f_code;		/* 0x158 */
	int full_pel_backward_vector;	/* 0x15C */
	int backward_f_code;		/* 0x160 */
	int f_code[2][2];		/* 0x164 */
	int picture_structure;		/* 0x174 */
	int top_field_first;		/* 0x178 */
	int frame_pred_frame_dct;	/* 0x17C */
	int concealment_motion_vectors;	/* 0x180 */
	int repeat_first_field;		/* 0x184 */
	int progressive_frame;		/* 0x188 */
	int frame_centre_horizontal_offset[3];	/* 0x18C */
	int frame_centre_vertical_offset[3];	/* 0x198 */
	int closed_gop;			/* 0x1A4 */
	int broken_link;		/* 0x1A8 */
	int _1ac;			/* 0x1AC */
	int dcr;			/* 0x1B0: BDEC's DC reset (the intra DC predictors' reset) */
	int quantizer_scale_code;	/* 0x1B4: BDEC's QSC, the slice's or the macroblock's */
	MpegRef *ref[3][4];		/* 0x1B8 */
	MpegRef refs[9];		/* 0x1E8 */
	MpegMB mb[2];		/* 0x590: the current one mb[_810] */
	int _810;		/* 0x810 */
	int _814;		/* 0x814 */
	int dirty;		/* 0x818: the last IPU command leaves the bit buffer stale */
	u_int _81c;		/* 0x81C */
	int _820;		/* 0x820 */
	long pts;		/* 0x828 */
	long dts;		/* 0x830 */
	u_int bits;		/* 0x838: the IPU's top 32 bits */
	int nbits;		/* 0x83C: of them, valid */
	u_int load_intra_quantiser_matrix;	/* 0x840 */
	u_int load_non_intra_quantiser_matrix;	/* 0x844 */
	int mpeg2;		/* 0x848: a sequence_extension was seen */
	int _84c;		/* 0x84C */
	int _850;		/* 0x850 */
	int _854;		/* 0x854 */
	sceMpeg *mp;		/* 0x858 */
	char pad85c[0x880 - 0x85C];
	char tags[0x10A0 - 0x880];	/* 0x880: _sendDataToIPU's DMA tags, from the first 16-byte boundary */
	int _10a0;		/* 0x10A0 */
	char pad10a4[0x10C0 - 0x10A4];
} MpegDec;

/* the layout, checked where the code says it */
#define MPEG_AT(T, f, off) typedef char _at_##T##_##f[(int)&((T *)0)->f == (off) ? 1 : -1]
MPEG_AT(sceMpeg, sys, 0x40);
MPEG_AT(MpegDec, work, 0x44);
MPEG_AT(MpegDec, ptsGapSet, 0x70);
MPEG_AT(MpegDec, alloc, 0x108);
MPEG_AT(MpegDec, picture_coding_type, 0x150);
MPEG_AT(MpegDec, picture_structure, 0x174);
MPEG_AT(MpegDec, ref, 0x1B8);
MPEG_AT(MpegDec, mb, 0x590);
MPEG_AT(MpegDec, _810, 0x810);
typedef char _at_MpegMB_size[sizeof(MpegMB) == 0x140 ? 1 : -1];
MPEG_AT(MpegDec, dirty, 0x818);
MPEG_AT(MpegDec, mp, 0x858);
MPEG_AT(MpegDec, _10a0, 0x10A0);
typedef char _at_MpegDec_size[sizeof(MpegDec) == 0x10C0 ? 1 : -1];

#endif
