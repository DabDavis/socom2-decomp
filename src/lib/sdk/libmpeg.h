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
#define IPU_CTRL	((volatile u_int *)0x10002010)
#define IPU_BP		((volatile u_int *)0x10002020)
#define IPU_TOP		((volatile u_long *)0x10002030)
#define D3_CHCR		((volatile u_int *)0x1000B000)
#define D3_QWC		((volatile u_int *)0x1000B020)
#define D4_CHCR		((volatile u_int *)0x1000B400)
#define D4_QWC		((volatile u_int *)0x1000B420)
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

/* a reference picture's record (0x68 bytes, nine of them from 0x1E8) */
typedef struct {
	char pad0[0x28];
	int valid;		/* 0x28 */
	char pad2c[0x68 - 0x2C];
} MpegRef;

typedef struct {
	int isEnd;		/* 0x000 */
	int nFrames;		/* 0x004: frames decoded and not yet output */
	int started;		/* 0x008 */
	MpegCb cb[7];		/* 0x00C: by sceMpegCbType */
	void *work;		/* 0x044: 0x600 bytes from the allocator */
	int _048;		/* 0x048 */
	char pad04c[0x70 - 0x4C];
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
	int _0fc;		/* 0x0FC */
	int _100;		/* 0x100 */
	int _104;		/* 0x104 */
	MpegAlloc alloc;	/* 0x108 */
	int frameNo;		/* 0x118 */
	int ipuEnd;		/* 0x11C */
	int secondField;	/* 0x120 */
	char pad124[0x150 - 0x124];
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
	char pad18c[0x1B8 - 0x18C];
	MpegRef *ref[3][4];		/* 0x1B8 */
	MpegRef refs[9];		/* 0x1E8 */
	char pad590[0x818 - 0x590];
	int dirty;		/* 0x818: the last IPU command leaves the bit buffer stale */
	u_int _81c;		/* 0x81C */
	int _820;		/* 0x820 */
	long pts;		/* 0x828 */
	long dts;		/* 0x830 */
	u_int bits;		/* 0x838: the IPU's top 32 bits */
	int nbits;		/* 0x83C: of them, valid */
	u_int _840;		/* 0x840 */
	u_int _844;		/* 0x844 */
	int mpeg1;		/* 0x848 */
	int _84c;		/* 0x84C */
	int _850;		/* 0x850 */
	int _854;		/* 0x854 */
	sceMpeg *mp;		/* 0x858 */
	char pad85c[0x10A0 - 0x85C];
	int _10a0;		/* 0x10A0 */
	char pad10a4[0x10C0 - 0x10A4];
} MpegDec;

#endif
