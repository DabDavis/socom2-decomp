/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's DECI2 calls (MAIN 0x1A4B80-0x1A4D5C): Sony's SDK 2.7, the demo's object 0x15C580 (C:
 * gcc2_compiled.), its ten functions in the same order. Each packs its arguments into a word array
 * and makes the Deci2Call system call (syscall.s) with the request's number: 1-4 the protocol calls,
 * -5..-9 their Ex forms, 16 kputs. Names are SOCOM 1's (s1:exact); the protocol, the handler and
 * the request block (userarea, passed uncached) are libkernl's sceDeci2Open parameters. */

typedef unsigned int u_int;
typedef unsigned short u_short;

extern int Deci2Call(int request, u_int *param);
extern char userarea[];

int sceDeci2Open(u_short protocol, void *opt, void (*handler)(int event, int param, void *opt))
{
	u_int arg[4];

	arg[0] = protocol;
	arg[1] = (u_int)opt;
	arg[2] = (u_int)handler;
	arg[3] = (u_int)userarea | 0x20000000;
	return Deci2Call(1, arg);
}

int sceDeci2Close(int s)
{
	u_int arg[1];

	arg[0] = s;
	return Deci2Call(2, arg);
}

int sceDeci2ReqSend(int s, char dest)
{
	u_int arg[2];

	arg[0] = s;
	arg[1] = dest;
	return Deci2Call(3, arg);
}

int sceDeci2Poll(int s)
{
	u_int arg[1];

	arg[0] = s;
	return Deci2Call(4, arg);
}

int sceDeci2ExRecv(int s, void *buf, u_short len)
{
	u_int arg[3];

	arg[0] = s;
	arg[1] = (u_int)buf;
	arg[2] = len;
	return Deci2Call(-5, arg);
}

int sceDeci2ExSend(int s, void *buf, u_short len)
{
	u_int arg[3];

	arg[0] = s;
	arg[1] = (u_int)buf;
	arg[2] = len;
	return Deci2Call(-6, arg);
}

int sceDeci2ExReqSend(int s, char dest)
{
	u_int arg[2];

	arg[0] = s;
	arg[1] = dest;
	return Deci2Call(-7, arg);
}

int sceDeci2ExLock(int s)
{
	u_int arg[1];

	arg[0] = s;
	return Deci2Call(-8, arg);
}

int sceDeci2ExUnLock(int s)
{
	u_int arg[1];

	arg[0] = s;
	return Deci2Call(-9, arg);
}

int kputs(char *s)
{
	u_int arg[1];

	arg[0] = (u_int)s;
	return Deci2Call(16, arg);
}
