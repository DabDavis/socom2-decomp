/* The boot loader's overlay decode (MAIN 0x1C5DA0): one MWo3 overlay out of
 * APACHE00.ZDB, through DNAS.BIN's codec, inflated to its slot (docs/storage.md).
 * The original file name is unknown; the functions it calls keep placeholder names
 * until their own TUs are written. */

typedef unsigned long uLong;    /* 64-bit under this compiler: uncompress's 4th argument is lwu-loaded */

int func_001C5EC0(const char *name, void *buf);                        /* read a ZDB entry -> its length */
int func_00539D00(unsigned int len, void *buf, int *key);              /* DNAS.BIN: container header */
int func_00539D50(unsigned int len, int key, void *buf);               /* DNAS.BIN: body decrypt -> length */
int func_00534848(unsigned int len, void *buf, unsigned int *clen);    /* DNAS.BIN: content setup */
int func_00535018(unsigned int len, unsigned int clen, void *buf);     /* DNAS.BIN: content run -> 0 */
void func_004C73F0(void);                                               /* DNAS.BIN: before the second payload */
int uncompress(void *dest, uLong *destLen, const void *source, uLong sourceLen);
void func_00181D80(void *dest, int size);

int func_001C5DA0(const char *name, void *dest, void *buf, int second)
{
    uLong destLen = 0x5000000;
    int ok = 0;
    int len;
    unsigned int clen;
    int key;

    if ((len = func_001C5EC0(name, buf)) > 0) {
        key = 0;
        if (func_00539D00(len, buf, &key) >= 0) {
            if ((len = func_00539D50(len, key, buf)) > 0) {
                if (func_00534848(len, buf, &clen) == 0) {
                    if (func_00535018(len, clen, buf) == 0) {
                        if (second)
                            func_004C73F0();
                        if (uncompress(dest, &destLen, buf, clen) == 0) {
                            if (destLen != 0) {
                                func_00181D80(dest, destLen);
                                ok = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return ok;
}
