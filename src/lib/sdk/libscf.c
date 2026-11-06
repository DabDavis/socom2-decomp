/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libscf (MAIN 0x1BDE40-0x1BE530): Sony's SDK system-configuration getters and RTC time helpers,
 * the demo's object 0x3AA6F8 (C), whole, in its order (18 of 21 byte-exact). On a T10K development kit (ROMVER's fifth byte 'T') the values come from the
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

/* The getters keep one exit: the value goes to `ret` on every path (round 12a) -- a `return` per
 * path builds the T10K path's gConfig address in v1 and GetOsdConfigParam2's field straight into v0,
 * and selects GetTimeZone's 540 with a branch where the target has movz. */
int sceScfGetLanguage(void)
{
	sceOsdConfigParam p;
	int ret;

	GetOsdConfigParam(&p);
	if (IsT10K())
		ret = gConfig.Language;
	else {
		GetOsdConfigParam(&p);
		if (p.version == 0)
			ret = p.japLanguage;
		else
			ret = p.language;
	}
	return ret;
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
	int ret;

	if (IsT10K())
		ret = gConfig.TimeZone;
	else {
		GetOsdConfigParam(&p);
		if (p.version == 0)
			ret = 540;
		else
			ret = p.timezoneOffset;
	}
	return ret;
}

int sceScfGetDateNotation(void)
{
	sceOsdConfigParam p;
	sceOsdConfigParam2 p2;
	int ret;

	if (IsT10K())
		ret = gConfig.DateNotation;
	else {
		GetOsdConfigParam(&p);
		if (p.version == 0)
			ret = 0;
		else {
			GetOsdConfigParam2(&p2, 1, 1);
			ret = p2.dateNotation;
		}
	}
	return ret;
}

int sceScfGetSummerTime(void)
{
	sceOsdConfigParam p;
	sceOsdConfigParam2 p2;
	int ret;

	if (IsT10K())
		ret = gConfig.SummerTime;
	else {
		GetOsdConfigParam(&p);
		if (p.version == 0)
			ret = 0;
		else {
			GetOsdConfigParam2(&p2, 1, 1);
			ret = p2.summerTime;
		}
	}
	return ret;
}

int sceScfGetTimeNotation(void)
{
	sceOsdConfigParam p;
	sceOsdConfigParam2 p2;
	int ret;

	if (IsT10K())
		ret = gConfig.TimeNotation;
	else {
		GetOsdConfigParam(&p);
		if (p.version == 0)
			ret = 0;
		else {
			GetOsdConfigParam2(&p2, 1, 1);
			ret = p2.timeNotation;
		}
	}
	return ret;
}

/* The RTC's time, as libcdvd's sceCdReadClock gives it: BCD, Japan standard time. The object's
 * last eleven (round 4a): tobcd, convertfrombcd, converttobcd, addhour, subhour, sceScfGetGMTfromRTC
 * (the map's func_001BE4E0, named at the demo's position), sceScfGetLocalTimefromRTC byte-exact;
 * addhour's and GetLocalTimefromRTC's last calls are dtls13010's sibling call after the epilogue
 * (`jal f; nop; jr ra; nop`, as libcdvd's cdvd_exit: every other 2.9 build misses that by 30+ words).
 *
 * AdjustTime (round 12a) tests `m >= 0` first -- the else branch's loop is laid out last, entered by
 * the bltzl -- adds 60 before each subhour, and sums into its own variable: `min += t->minute` (or
 * `min = t->minute + min`, which expand_binop swaps to the target's operand) gives `addu s0,s0,v0`.
 *
 * adddate and subdate (GCC round 42a; open since round 4a, 41 and 37 words off) read the year into a
 * local first, for the leap test: written `(t->year & 3) == 0`, dtls13010's GCSE carried that load to
 * the century test and the increment (the target reloads t->year there after the day and month
 * stores) -- why they were byte-exact only under -fno-gcse, 2.9-990721 or a 2.95.x, while the object's
 * GetRomName needs this GCSE (gRomName's %hi held in s1, the address built twice from it). Read into
 * `u_char year`, the later test reads t->year again, as the demo's identical code does.
 *
 * OPEN: frombcd (5 words: the target multiplies with MIPS's two-operand `mult; mflo` -- the product
 * allocated to LO, mulsi3_mult3's second alternative -- this the R5900's three-operand `mult`; (x >> 4)
 * * 6 in eleven spellings, 64-bit ones included. Round 42a: not another CPU's mulsi3 -- -mcpu=r4000's
 * would be `mult; mflo` as one pattern, but this compiler and 2.96 then synthesise the 6 from shifts;
 * -mips3 is unchanged). */
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

static u_char tobcd(u_char x)
{
	return x + (x / 10) * 6;
}

static u_char frombcd(u_char x)
{
	return x - (x >> 4) * 6;
}

static void convertfrombcd(sceCdCLOCK *t)
{
	t->year = frombcd(t->year);
	t->month = frombcd(t->month);
	t->day = frombcd(t->day);
	t->hour = frombcd(t->hour);
	t->minute = frombcd(t->minute);
	t->second = frombcd(t->second);
}

static void converttobcd(sceCdCLOCK *t)
{
	t->year = tobcd(t->year);
	t->month = tobcd(t->month);
	t->day = tobcd(t->day);
	t->hour = tobcd(t->hour);
	t->minute = tobcd(t->minute);
	t->second = tobcd(t->second);
}

static void adddate(sceCdCLOCK *t)
{
	char days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	u_char year = t->year;

	t->day++;
	if ((year & 3) == 0)
		days[1] = 29;
	if (days[t->month - 1] < t->day) {
		t->day = 1;
		t->month++;
		if (t->month == 13) {
			if (t->year == 99)
				t->year = 0;
			else
				t->year++;
			t->month = 1;
		}
	}
}

static void subdate(sceCdCLOCK *t)
{
	char days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	u_char year = t->year;

	t->day--;
	if ((year & 3) == 0)
		days[1] = 29;
	if (t->day == 0) {
		t->month--;
		if (t->month == 0) {
			if (t->year == 0)
				t->year = 99;
			else
				t->year--;
			t->month = 12;
		}
		t->day = days[t->month - 1];
	}
}

static void addhour(sceCdCLOCK *t)
{
	if (++t->hour == 24) {
		t->hour = 0;
		adddate(t);
	}
}

static void subhour(sceCdCLOCK *t)
{
	if (t->hour == 0) {
		t->hour = 23;
		subdate(t);
	} else
		t->hour--;
}

static void AdjustTime(sceCdCLOCK *t, int min)
{
	int m;

	convertfrombcd(t);
	m = t->minute + min;
	if (m >= 0) {
		while (m > 60) {
			addhour(t);
			m -= 60;
		}
	} else {
		while (m < 0) {
			m += 60;
			subhour(t);
		}
	}
	t->minute = m;
	converttobcd(t);
}

void sceScfGetGMTfromRTC(sceCdCLOCK *rtc)
{
	AdjustTime(rtc, -540);
}

void sceScfGetLocalTimefromRTC(sceCdCLOCK *rtc)
{
	int tz = sceScfGetTimeZone();
	int st = sceScfGetSummerTime();

	AdjustTime(rtc, -540 + tz + st * 60);
}
