/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libscf (MAIN 0x1BDE40-0x1BE178): Sony's SDK system-configuration getters, the demo's object
 * 0x3AA6F8 (C), in its order; the rest of the object (sceScfGetLocalTimefromRTC and its helpers)
 * is not written yet. On a T10K development kit (ROMVER's fifth byte 'T') the values come from the
 * block sceScfSetT10kConfig stored; otherwise from the OSD configuration (GetOsdConfigParam, and
 * GetOsdConfigParam2's second byte for the date/time formats, version 1 and up). Names are SOCOM
 * 1's (s1:exact; gConfig and gRomName are the demo's locals at the same references). */

typedef unsigned int u_int;
typedef unsigned char u_char;

typedef struct {
	u_int spdifMode:1;
	u_int screenType:2;
	u_int videoOutput:1;
	u_int japLanguage:1;
	u_int ps1drvConfig:8;
	u_int version:3;
	u_int language:5;
	int timezoneOffset:11;
} sceOsdConfigParam;

typedef struct {
	u_char reserved:4;
	u_char summerTime:1;
	u_char timeNotation:1;
	u_char dateNotation:2;
} sceOsdConfigParam2;

typedef struct {
	short TimeZone;
	u_char Aspect;
	u_char DateNotation;
	u_char Language;
	u_char Spdif;
	u_char SummerTime;
	u_char TimeNotation;
} sceScfT10kConfig;

extern void GetOsdConfigParam(sceOsdConfigParam *p);
extern int GetOsdConfigParam2(void *p, int size, int offset);
extern int sceOpen(const char *name, int mode);
extern int sceRead(int fd, void *buf, int n);
extern int sceClose(int fd);

extern sceScfT10kConfig gConfig;
extern char gRomName[];

static char *GetRomName(void)
{
	int fd;

	if (gRomName[0] == 0) {
		fd = sceOpen("rom0:ROMVER", 1);
		if (fd >= 0) {
			sceRead(fd, gRomName, 14);
			sceClose(fd);
		}
	}
	return gRomName;
}

static int IsT10K(void)
{
	if (gRomName[0] == 0)
		GetRomName();
	return gRomName[4] == 'T';
}

/* OPEN, with sceScfGetTimeZone, GetDateNotation, GetSummerTime and GetTimeNotation (2, 8, 4, 5, 5
 * words): on the T10K path the target builds gConfig's address in v0 (`lui v0; lbu v0,N(v0)`),
 * where this builds it in v1 as sceScfGetAspect and GetSpdif (byte-exact) do in both -- the five
 * have more code on the other path; GetTimeZone selects 540 with movz (`p.version ? tz : 540`
 * computed both ways), which no if/else spelling tried gives; the date/time formats load
 * GetOsdConfigParam2's byte into v1 and shift it into v0. */
int sceScfGetLanguage(void)
{
	sceOsdConfigParam p;

	GetOsdConfigParam(&p);
	if (IsT10K())
		return gConfig.Language;
	GetOsdConfigParam(&p);
	if (p.version == 0)
		return p.japLanguage;
	return p.language;
}

void sceScfSetT10kConfig(sceScfT10kConfig *config)
{
	gConfig = *config;
}

int sceScfGetAspect(void)
{
	sceOsdConfigParam p;

	if (IsT10K())
		return gConfig.Aspect;
	GetOsdConfigParam(&p);
	return p.screenType;
}

int sceScfGetSpdif(void)
{
	sceOsdConfigParam p;

	if (IsT10K())
		return gConfig.Spdif;
	GetOsdConfigParam(&p);
	return p.spdifMode;
}

int sceScfGetTimeZone(void)
{
	sceOsdConfigParam p;

	if (IsT10K())
		return gConfig.TimeZone;
	GetOsdConfigParam(&p);
	if (p.version == 0)
		return 540;
	return p.timezoneOffset;
}

int sceScfGetDateNotation(void)
{
	sceOsdConfigParam p;
	sceOsdConfigParam2 p2;

	if (IsT10K())
		return gConfig.DateNotation;
	GetOsdConfigParam(&p);
	if (p.version == 0)
		return 0;
	GetOsdConfigParam2(&p2, 1, 1);
	return p2.dateNotation;
}

int sceScfGetSummerTime(void)
{
	sceOsdConfigParam p;
	sceOsdConfigParam2 p2;

	if (IsT10K())
		return gConfig.SummerTime;
	GetOsdConfigParam(&p);
	if (p.version == 0)
		return 0;
	GetOsdConfigParam2(&p2, 1, 1);
	return p2.summerTime;
}

int sceScfGetTimeNotation(void)
{
	sceOsdConfigParam p;
	sceOsdConfigParam2 p2;

	if (IsT10K())
		return gConfig.TimeNotation;
	GetOsdConfigParam(&p);
	if (p.version == 0)
		return 0;
	GetOsdConfigParam2(&p2, 1, 1);
	return p2.timeNotation;
}
