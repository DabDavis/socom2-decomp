/* libcdvd's EE side (MAIN 0x18DB90-0x18FB50): Sony's SDK -- the types, calls and the demo's globals
 * (data.txt) its objects share. The callback numbers, semaphore ids, the busy flag and the callback
 * thread's cleared word are volatile (the target never reorders them nor puts them in delay slots). */
#ifndef LIBCDVD_H
#define LIBCDVD_H

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

extern int CreateSema(struct SemaParam *sema);
extern int DeleteSema(int sema_id);
extern int WaitSema(int sema_id);
extern int iSignalSema(int sema_id);
struct ThreadParam {
	int status;
	void *entry;
	void *stack;
	int stackSize;
	void *gpReg;
	int initPriority;
	int currentPriority;
	u_int attr;
	u_int option;
	int waitType;
	int waitId;
	int wakeupCount;
};

extern int SetAlarm(u_short time, void (*handler)(int, u_short, void *), void *common);
extern int DIntr(void);
extern int EIntr(void);
extern int sceCdSync(int mode);
extern int scePrintf(const char *fmt, ...);
extern void ExitDeleteThread(void);
extern int GetThreadId(void);
extern int ReferThreadStatus(int thread_id, struct ThreadParam *info);
extern int CreateThread(struct ThreadParam *thread);
extern int StartThread(int thread_id, void *arg);
extern int ChangeThreadPriority(int thread_id, int priority);
extern int SignalSema(int sema_id);
extern void *sceSifRemoveCmdHandler(u_int cid);
extern void sceSifAddCmdHandler(u_int cid, void (*handler)(void *), void *data);
extern int sceSifCallRpc(void *cd, u_int fno, u_int mode, void *send, int ssize, void *recv, int rsize,
                         void (*end)(void *), void *end_param);
extern int sceSifCheckStatRpc(void *cd);

extern volatile int sceCdCbfunc_num;            /* the demo's globals (data.txt) */
extern volatile int sceCdCbfunc_number;
extern volatile int cb_semid;
extern volatile int _sceCd_ncmd_semid;
extern volatile int _sceCd_scmd_semid;
extern volatile int data_001CA870;              /* a third command semaphore */
extern volatile int _sceCd_c_cb_sem;
extern int data_001CA854;                              /* the callback thread is running */
extern void (*sceCdCbfunc)(int);
extern int data_001D5F84;                       /* the caller's $gp, for the callback thread */
extern int SCE_CD_debug;
extern int my_thid;
extern volatile int data_001D5F94;
extern struct ThreadParam data_001D5FA0;        /* the caller's status */
extern struct ThreadParam data_001D5FD0;        /* the callback thread's parameters */
extern int _icmd_bind;
extern int Init_seq;
extern void (*data_001D5F88)(void *);           /* the power-off callback, its $gp and argument */
extern int data_001D5F8C;
extern void *data_001D5F90;
typedef struct {
	int rpcd[4];
	u_int command;
	void *buff;
	void *cbuff;
	void (*func)(void *);
	void *para;
	void *serve;
} sceSifClientData;

extern sceSifClientData _sceCd_cd_ncmd;         /* the N- and S-command RPC clients */
extern sceSifClientData _sceCd_cd_scmd;
extern int ncmd_sema_keep_cmd;
extern int scmd_sema_keep_cmd;
extern int _ncmd_bind;
extern int _scmd_bind;
extern int PollSema(int sema_id);
extern void sceSifInitRpc(u_int mode);
extern int sceSifBindRpc(sceSifClientData *cd, u_int num, u_int mode);
extern int sceCdSyncS(int mode);
extern int data_001CA8C0[];                     /* the N commands' receive buffer */
extern char _sceCd_ncmdsdata[];
extern char _sceCd_scmdrdata[];
extern char data_001CBE80[];                    /* the S commands' send buffer */

typedef struct {
	u_char stat;
	u_char second;
	u_char minute;
	u_char hour;
	u_char pad;
	u_char day;
	u_char month;
	u_char year;
} sceCdCLOCK;

extern int data_001CC3E8;                       /* sceCdMmode's request: 34, 4, 4 */
extern int data_001CC3EC;
extern int data_001CC3F0;
extern void sceSifWriteBackDCache(void *ptr, int size);
extern int _sceCd_scmd_prechk(int cmd);

#endif
