/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* MAIN 0x1BE530-0x1BE698, between libscf and libusbkb: two calls over libdbc's sockets -- a profile
 * received (command size 2, type 2, 3, 1) and two buffers sent concatenated behind their lengths
 * (command size 11, type 1, 3, 1, through sceDbcSendData2). The shapes of Sony's libvib, the DBC
 * pad's actuator library that came with libpad2 (sceVibGetProfile(socket, profile),
 * sceVibSetActParam(socket, profile_size, profile, data_size, data)); not in SOCOM 1's demo, and no
 * string or caller names them, so the rows keep their placeholders. Its own object (GCC round 30a):
 * libpad2 (0x1BD588) passes the same receive command as an int, its low word sign-extended, where
 * this object passes the 8-byte command struct whole (no truncation before either call). Both
 * byte-exact (the receive as first written): the send's `size = 40` comes before the command's
 * fields (built first, its 64-bit mask goes to t2 and schedules past the prologue: 6 words) and
 * the profile's length is stored between the two copies (it fills the second call's slot; 1 of
 * 60 orders with the declarations, build/scratch/gcc/v_vib5b0*.py). */

typedef unsigned char u_char;
typedef unsigned long u_long;

/* a DBC command word (libpad2.c's PAD2CMD): its 14-bit size, 2-bit type and two bytes */
typedef struct {
	u_long size : 14;
	u_long type : 2;
	u_long f16 : 8;
	u_long f24 : 8;
} sceDbcCmd;

extern int sceDbcReceiveData(int s, sceDbcCmd cmd, int *size, char *buf);
extern int sceDbcSendData2(int s, sceDbcCmd cmd, int *size, char *buf);
extern void *memcpy(void *d, const void *s, unsigned int n);

int func_001BE530(int socket, char *profile)
{
	sceDbcCmd cmd;
	int size;
	int r;

	cmd.size = 2;
	cmd.type = 2;
	cmd.f16 = 3;
	cmd.f24 = 1;
	r = sceDbcReceiveData(socket, cmd, &size, profile);
	if (r >= 0)
		return size;
	return r;
}

int func_001BE5B0(int socket, int profile_size, u_char *profile, int data_size, u_char *data)
{
	sceDbcCmd cmd;
	int buf[12];
	int size;

	size = 40;
	cmd.size = 11;
	cmd.type = 1;
	cmd.f16 = 3;
	cmd.f24 = 1;
	buf[1] = data_size;
	memcpy((char *)&buf[2], data, data_size);
	buf[0] = profile_size;
	memcpy((char *)&buf[2] + data_size, profile, profile_size);
	return sceDbcSendData2(socket, cmd, &size, (char *)buf);
}
