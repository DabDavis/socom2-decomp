# `decomp/` — matching decomp of SOCOM II (SCUS-97275 r0001)

**2,383 functions byte-exact** (2026-10-24): **48 hand-written** -- zlib 1.1.4 in MAIN (the
calibration), the boot loader's overlay decode, and the engine's zMath (CQuat, the
projector matrices, CPnt3D/CPnt4D) -- and **2,335 generated**, m2c's drafts that are
byte-exact as they stand (`src/auto/`, counted apart; see "The generated tier"). Four
functions are open, each diagnosed in its source. Canary: `./test_scdecomp.py`.

This is the start of a function-by-function, byte-exact decomp of the game, built the way
`~/claude-eqoa/decomp` (github.com/DabDavis/eqoa-decomp) is: offline Python tooling, a
hand-kept symbol map, every function verified against the shipped bytes **and** every call
and data reference resolved to the address the shipped code uses.

```bash
./test_scdecomp.py                    # the canary: run first, it refuses a wrong disc
./sccheck.py                          # build src/ and score every function (rc 1 on a regression)
./sccheck.py --wrong -f GetWeapon     # with the instruction diff of each mismatch
./scd.py show 0x307520                # disassemble a target function, callees named
./scdiff.py src/engine/quat.cpp MakeZRot__5CQuatFf     # one function, one TU
./tools/identify.py SRC.c ... --rename                 # name a known-source TU's functions
./tools/callnames.py --rename                          # name callees off matched call sites
```

| File | What |
|---|---|
| `scdecomp.py` | the image (MAIN + the two MWo3 overlays, digests pinned), ELF reading, the map, disassembly |
| `scdiff.py` | the compiler driver (wibo + mwccps2) and the differ: masks the object's own relocation fields, then **resolves every relocation** — calls by map address, `%hi/%lo` pairs by value, string literals by text, switch tables by target |
| `sccheck.py` | builds `src/`, scores every function, holds `src/MATCHED.txt` (a regression is rc 1) |
| `scd.py` | `show` / `list` / `where` / `census` |
| `test_scdecomp.py` | the canary: each check paired with a negative control that proves it can fail |
| `tools/decrypt_overlays.py` | your disc -> the two decrypted overlays (the target) |
| `tools/seed_symbols.py` | the sweep that seeded `symbols.txt` |
| `tools/identify.py` | compile a known-source TU, find each function in the image, verify, rename |
| `tools/callnames.py` | name placeholders off byte-exact functions' own call sites (eqcallnames.py) |
| `tools/m2c.py` | a target function as m2c-ready assembly (labels, callees by map name, `%hi/%lo` as `data_<VA>`), and m2c's draft |
| `tools/perm.py` | the permuter: reorders `// PERM_BEGIN/END` lines and `// PERM_DECL` names until a function matches |
| `tools/automatch.py` | compiles m2c's draft of every small function as it stands; `--emit` writes the hits to `src/auto/` |
| `symbols.txt` | **the map** — `0xADDR SIZE name  # note`, kept by hand from here on |
| `src/` | the reconstructed source; `MATCHED.txt` (the baseline), `ABSENT.txt` (dead-stripped) |
| `include/` | minimal libc headers in our own words |

## The target: the disc, not the update

The target is **r0001**, the disc as shipped — build id `SOCOM 2 r0001 17:22:21 Oct 11 2003`,
`SYSTEM.CNF` `VER = 1.02`. It is **not** r0004 (Nov 3 2004), the online update that every
EE dump in `memory-dumps/` holds and that `symbols/*.sym` were built from — **those
addresses are r0004's**; socom-unzipped measured only ~2,931 functions at the same address
in both builds. Any owner of the disc can reproduce r0001 bit for bit; r0004 is per-console
encrypted on a memory card.

The image is three pieces of one address space:

| piece | base | source | code |
|---|---|---|---|
| MAIN | `0x00100000` | `SCUS_972.75`'s one non-empty LOAD | `0x180000-0x1CA3D0`, `0x1CCD78-0x1CE2A0`, `0x1D4B00-0x1D4F90` |
| OVL1 | `0x001E7000` | `ftscore.bin` (FTSCore) | `0x1E7040-0x3D5980`; VU1 microcode `0x3D5980-0x3DBFF8` |
| OVL2 | `0x004C5380` | `zsealetc.bin` (ZSealEtc) | `0x4C53C0-0x64F778` |

- **The overlays are encrypted on the disc** (`RUN/RAW/APACHE00.ZDB`, docs/storage.md).
  `tools/decrypt_overlays.py` runs the disc's own decoder under Unicorn with
  **socom-unzipped's harness** (github.com/Scotho/socom-unzipped, GPL-3.0, cloned into
  `reference/`, which git ignores). The insight our `cmd/ovlextract` lacked: DNAS.BIN itself
  carries **131 self-encrypting code blocks**, each decrypted by running its own cipher core
  first; then the four loader calls run with sceSif/libcdvd answered in HLE. About ten minutes.
  Every input and output is checked against the digests socom-unzipped pins for this disc:
  `ftscore.bin` 2,233,472 B `cf09a7fa…`, `zsealetc.bin` 1,723,520 B `cdb3fa8e…`.
- **The MWo3 header is 0x40 bytes and code begins right after it** — FTSCore's first word is
  a prologue at `0x1E7040`, and the boot enters ZSealEtc at `0x4C53C0`. The text range is
  `[base+0x40, base+0x40+text)`, ending on FTSCore exactly at its first float constant.
  (socom-unzipped's research/05 says 0x80; it is not.)
- **OVL1's text ends in VU1 microcode** (lower/upper NOP pairs `8000033C`/`000002FF`); the
  sweep stops at it. Ghidra's 39 KB "function" at `0x3D6750` is that microcode.
- **MAIN is mostly not ours**: Sony's SDK (GCC-built, 8-byte aligned, tables inside `.text` —
  `PsIIlibipu`, `PsIIlibkernl`), MSL, zlib 1.1.4, and the boot loader. The `__sinit_*` static
  initialisers sit after rodata at `0x1D4B00`, as Metrowerks places them.

## The compiler: Metrowerks CodeWarrior for PS2, build 3.0b52 (or 3.0b50)

**Settled by measurement, not by the ELF's `.comment`.** `SCUS_972.75` (and SOCOM 1's beta)
carry `MW MIPS C Compiler (2.4.1.01)`; socom-unzipped took that string at its word and
rejected a matching decomp because "mwccps2 2.4.1.01 is a discontinued commercial product"
(their research/89). Two things are wrong with that:

1. **The 2.4.1.01 compilers were never missing** — decompme/compilers ships four
   `mwcps2-3.0b22-*` builds that report exactly that string (EQOA's toolchain tarball had them).
2. **And the string does not name the compiler that built this code.** zlib 1.1.4 is linked
   into MAIN; its source is certain, so it is a calibration nobody has to guess at. Compiled
   **unmodified**, its 21 functions in the image score, per build (all 26 PS2 builds in
   decompme/compilers fetched; three pairs are byte-identical duplicates):

| build | reports | zlib functions byte-exact |
|---|---|---|
| `mwcps2-3.0b52-030722` | 3.0.0 | **21 / 21** |
| `mwcps2-3.0b50-030527` | 3.0.0 | **21 / 21** |
| `mwcps2-3.0b38-030307` | 3.0.0 | 20 (misses `huft_build`) |
| `mwcps2-3.0b22-020926` | 2.4.1.01 | 17 (misses `adler32`'s short-loop nops, `inflate_fast`, `inflate_codes`, `huft_build`) |
| `mwcps2-3.0b22-011126/020123/020716` | 2.4.1.01 | 6 |
| `mwcps2-3.0.1b44 … b87` (2003) | 3.0.0 | 5 |
| `mwcps2-2.4-001213` | 2.4.1.01 | 0 |

So the game was built with a 3.0.0-reporting build from mid-2003 — three months before the
Oct 11 2003 build — and the `.comment` comes from something else in the link (a prebuilt
runtime object is the likely carrier). **3.0b50 and 3.0b52 are not yet told apart**; the
default is 3.0b52 (the nearer date). A function that separates them settles it.

**The flags, each by its own evidence** (`scdiff.FLAGS`):

Each flag is necessary: drop any one and zlib's score falls (3.0b52, the others held).

| flag | without it | evidence |
|---|---|---|
| `-O4,p` | `-O3,p` 18; `-O4,s`, `-O4`, `-O3` 6; `-O2` 0 | the only level at 21/21 |
| `-sdatathreshold 0` | 18 | the target addresses zlib's 6-byte version string with `lui/addiu`; the default (8) puts it in `$gp` small data |
| `-inline auto,deferred` | `auto` 20, `deferred` 18, `on` 17 | `inflateReset` is absent from the image — inlined into both callers and stripped (`auto`); functions emit in **reverse definition order**, exactly as the image holds them (`deferred`) |
| `-enum min` | 13 | `inflate`'s `mode` field is stored with `sb` |

Driven through **wibo** (decompals' minimal Win32 loader, as decomp.me does): ~10 ms a
compile against wine's ~2 s, and the 2003 builds — which wait forever on a FlexLM licence
prompt under wine — just run. `SC_WINE=1` falls back to wine. `-gccinc` makes `-I` paths
serve `<...>` includes; `-nostdinc` keeps the host's headers out.

## What the source has to reproduce

- **Function order inside a TU is REVERSE definition order** (`-inline deferred`): the image's
  `inftrees.c` reads fixed, dynamic, bits, huft_build — the source defines them backwards. So
  the image's order is a direct read of the source's order, reversed.
- **TUs link in alphabetical file order** (zlib: adler32, infblock, infcodes, inffast, inflate,
  inftrees, infutil, uncompr, zutil — the unreferenced ones simply absent). Whether the game's
  own TUs follow the same rule is the next thing to measure; if so, TU boundaries can be read
  off the image.
- **Every Metrowerks function starts on a 16-byte boundary, padded with zero words.** Sony's
  and the Medius middleware are prebuilt GCC (8-aligned) and out of scope, like EQOA's runtime
  tail. 16-aligned rows in the map, an UPPER bound on the code that is ours (half of GCC's
  8-aligned functions land on 16 by chance): MAIN 868 (178 KB), OVL1 6,904 (1.94 MB), OVL2
  3,615 (1.31 MB) — against 8-aligned MAIN 497, OVL1 196, OVL2 1,149.
- `long` is 64 bits (zlib's `uLong` argument is `lwu`-loaded into a 64-bit slot).
- Small params: a callee's prototype changes the caller's scheduling — the loader matched only
  once its DNAS callees took `unsigned int` lengths.

## The map: `symbols.txt`

Seeded by `tools/seed_symbols.py` (EQOA's reach-tracking sweep plus three fixes, each a test:
a far `j` is a tail call, an infinite loop ends at the next 16-byte prologue, `0xFFFFFFFF` between
functions is padding). Against socom-unzipped's independent Ghidra table for the same image:
**13,045 of 13,228 starts agree**, 12,731 exactly on both ends; the rest sit mostly in the SDK.
SIZE is exact (the delay slot included, padding not). From here on the map is **kept by
hand**: rename a `func_<VA>` to the mangled name its source defines. A placeholder resolves by
the address in its name, so source can call `func_001C5EC0` before that function is written.
Data symbols go in the same file (`inflate_mask`). `src/ABSENT.txt` lists what the source
defines and the image does not hold.

### Names from SOCOM 1 (`tools/s1names.py`)

SOCOM 1's May 13 2002 demo (`extracted/socom1-demo-020513/SCUS_972.05`, sha256 `00c9cee7...`,
from the owner's copy of the Hidden Palace prototype) is unstripped: 9,703 named, sized
functions, `.relmain` (115,142 relocations) and Metrowerks DWARF 1 (`tools/dwarf1.py`: 959
units, class layouts, enums). Every carried name says how it was found in its note:

| note | how | count |
|---|---|---|
| `s1:exact` | same size, identical with only the fields the demo's own relocations name masked | 383 |
| `s1:callee` | called from the same instruction of an exact pair | 2 |
| `s1:syscall` | a kernel stub with the same syscall number | 6 |
| `s1:gap` | between two exact anchors, equal counts, sizes within 35 %, paired in order | 116 |
| `s1:calls` | the same sequence of known callees and the same strings, unique, both Metrowerks | 212 |
| `s1:order` | by hand: same position in the same unit, signature checked against the code | 7 |

Against socom-unzipped's independent table, 375 of 379 shared addresses agree; the four
others are its own labels (`netbEx*`, `_StartThread`) against SOCOM 1's real names.

**A non-exact name carries SOCOM 1's SIGNATURE, which may have changed.** The two projector
builders are SOCOM 1's `zdb::CalcWorldToUV` by behaviour, but SOCOM II passes the matrix
third and two floats where SOCOM 1 passed a `CTexture *` and the matrix fifth -- so their
rows keep placeholders with an `s1~` note. When source for a `gap`/`calls` name compiles to
a different mangled name, rename the row to what the source defines. (An earlier note said
the projectors were absent from the 2002 builds: they were in another unit, `zdb`'s.)

The July 9 2002 press beta (`SCUS_971.34`) is stripped but kept its asserts:
`tools/beta_tu.py` turns them into `tu:` hints -- which source file a function was in.

## Inputs (none of them in git)

| input | at | from |
|---|---|---|
| the disc | `$SOCOM_ROOT/SCUS-97275 (1.02).iso` | your own SOCOM II (SCUS-97275) |
| `SCUS_972.75` | `$SOCOM_ROOT/extracted/` | the disc |
| `ftscore.bin`, `zsealetc.bin` | `$SOCOM_ROOT/extracted/overlays/` | `.venv/bin/python tools/decrypt_overlays.py "<iso>" reference/socom-unzipped/tools_py` (needs `unicorn==2.1.4`: `python3 -m venv .venv && .venv/bin/pip install unicorn==2.1.4`) |
| the compilers | `toolchains/mwcps2-*/` | decompme/compilers releases (`gh release download compilers -R decompme/compilers -p 'mwcps2-*'`) |
| wibo | `toolchains/bin/wibo` | github.com/decompals/wibo releases (1.2.0) |
| objdump/readelf | `$PS2DEV/ee/bin/` | ps2dev (default `~/ps2dev/ps2dev`) |

`$SOCOM_ROOT` defaults to this directory's parent — set it when working from a clone
of the published repository rather than inside claude-socom.

## Publishing

`decomp/` is mirrored to a **private** repository, github.com/DabDavis/socom2-decomp
(created 2026-10-24), as `git subtree split --prefix=decomp` of claude-socom's history:
the work happens here and `tools/publish.sh` pushes the committed state (`--dry-run`
to look first). It refuses a tracked line that looks private, as claude-eqoa's
`_publish.py` does.

## The generated tier: `src/auto/`

`tools/automatch.py` hands every unnamed 16-aligned overlay function up to 0x100 bytes to
m2c, compiles the draft **as it stands** with the settled toolchain, and keeps it only if
it is byte-exact with every relocation resolved -- the same `scdiff.compare` hand-written
source must pass. 2,335 of 7,007 do (61 % of those up to 0x40 bytes, 33 % overall). They
live one per file in `src/auto/<segment>/<VA>.c` (two drafts' prototypes can disagree),
are counted apart by `sccheck.py`, and are **not finished source**: placeholder names,
`void *` and `M2C_FIELD` offsets, C rather than the game's C++, and no TU grouping. They are:

- a regression guard for the toolchain: 2,335 functions across both overlays compile on
  every check, so a wrong flag or build fails loudly (one build and one flag set match all
  of them -- the overlays were built with the toolchain zlib settled, throughout);
- a correct starting body for whoever writes the real TU;
- disposable: hand-written source supersedes them (sccheck skips a generated function the
  hand tier defines and says so), and `automatch.py --emit` deletes superseded files.

Data references in a draft are `data_<VA>` placeholders (BSS included), so the relocation
check verifies them; none reads a literal address.

## Open, and next

- **`CZWeaponList::GetWeapon` (`0x3C4B40`)** — 2 instructions from exact with an `int`
  parameter, but SOCOM 1's mangled name says the parameter is `EQUIP_ITEM`, and every
  enum-typed variant sinks the parameter's zero-extension into the loop preheader where the
  target computes it before the loop guard. A permuter's job (EQOA's `eqperm.py` is the model).
- **SOCOM 1's beta, `SCUS_972.05`** (May 2002, "SOCOM:2002 Public Beta") ships its full
  `.symtab` (9,703 functions), Metrowerks DWARF 1 (`.debug`, 5 MB) and `.line` — **types,
  struct layouts and translation-unit names**. Not on this machine. It is the single biggest
  accelerator available: it names the files this tree should have, and every shared engine
  class's layout.
- Tell 3.0b50 from 3.0b52.
- Measure the game's TU order (alphabetical?) and read TU boundaries off the image.
- Port the permuter; port eqrefcheck's data-reference audit to absolute addresses.
- The MSL runtime in MAIN (`_delete_vec` and friends) is MW-built library code — a second
  calibration set once its source is pinned down.

## Credits and licence scope

- **socom-unzipped** (Scotho, GPL-3.0): the overlay decryption harness this tree runs from
  `reference/` (never copied in), the r0001 Ghidra function table used as a cross-check, and the
  SOCOM 1 name carry. Their research notes saved days.
- **decompme/compilers** and **decomp.me** for the compilers and the wibo invocation;
  **decompals/wibo**.
- **zlib 1.1.4** (Gailly & Adler, zlib licence) is vendored unmodified in `src/zlib/`.
- This tree carries **no game code or data**: the target is produced from your own disc.
