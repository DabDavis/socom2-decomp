# `decomp/` — matching decomp of SOCOM II (SCUS-97275 r0001)

**4,503 functions byte-exact** (2026-10-25, 11.16 % of the map's bytes): **1,352 hand-written** --
zlib 1.1.4 in MAIN (the calibration), the boot loader's overlay decode, the engine's zMath, ZAnim
(the command handlers, CZAnimMain/Set/Anim), zdb (CNode, CWorld and its save/load, the asset
manager, intersections), CCamera, the input classes, the sound system (989snd.c and zsnd), the
game states and app camera, the mission and network game, the entities and the compass, the HUD
and the C2D 2D-graphics family -- and **3,151 generated**, m2c's drafts that are byte-exact as
they stand (`src/auto/`, counted apart; see "The generated tier"). Every open function is
diagnosed in its source. Canary: `./test_scdecomp.py`.

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
| `sccheck.py` | builds `src/`, scores every function, holds `src/MATCHED.txt` (keyed by address, so a rename is not a regression; a regression is rc 1); an inline function's out-of-line copy that misses and whose map address lies outside its unit is `elsewhere` (the linked copy is another unit's) |
| `scd.py` | `show` / `list` / `where` / `census` |
| `test_scdecomp.py` | the canary: each check paired with a negative control that proves it can fail |
| `tools/decrypt_overlays.py` | your disc -> the two decrypted overlays (the target) |
| `tools/seed_symbols.py` | the sweep that seeded `symbols.txt` |
| `tools/identify.py` | compile a known-source TU, find each function in the image, verify, rename |
| `tools/callnames.py` | name placeholders off byte-exact functions' own call sites (eqcallnames.py) |
| `tools/m2c.py` | a target function as m2c-ready assembly (labels, callees by map name, `%hi/%lo` as `data_<VA>`), and m2c's draft |
| `tools/perm.py` | the permuter: reorders `// PERM_BEGIN/END` lines and `// PERM_DECL` names until a function matches |
| `tools/automatch.py` | compiles m2c's draft of every small function as it stands; `--emit` writes the hits to `src/auto/`; `--stale` redoes the drafts a rename left behind; a template-named row is drafted under its placeholder and renamed in the object (`/* sc: as=OLD NEW */`); a miss is redrafted with its callees' prototypes, then as C++ |
| `tools/sigs.py` | a function's parameters for m2c: the mangled name's list, or the argument registers live at its entry (backward dataflow over its blocks) |
| `tools/cxx.py` | C++ declarations that mangle back to a Metrowerks name, for drafts that must make member and virtual calls |
| `tools/s1names.py` | SOCOM 1's function names, out of the May 2002 demo's symbols and relocations |
| `tools/s1data.py` | SOCOM 1's global data names, through the functions s1names paired |
| `tools/dwarf1.py` | the demo's Metrowerks DWARF 1: units, signatures, class layouts, enums; `func` (one function's parameters, locals, blocks, source lines); `header` (compilable C++ declarations of a type and all it embeds, offsets annotated, holes -- MW's vtable pointer -- padded) |
| `tools/beta_tu.py` | which source file a function was in, out of the press beta's asserts |
| `tools/rename.py` | rename map rows by hand, keeping the note, refusing a taken name, listing the source still using the old one |
| `tools/s1units.py` | SOCOM II's translation units: its static initialisers against SOCOM 1's named `__sinit_<file>`s |
| `tools/unitscan.py` | where one hand-written unit could be two: its pinned and inlined pairs, the target's own calls, every cut that separates the first and none of the second |
| `tools/s1vtables.py` | virtual functions named slot by slot: SOCOM II's vtables found by their RTTI's class-name string, aligned with the demo's `__vt__` tables; `--holdout` measures it on names the map already has |
| `tools/try.py` | compile one unit with one textual substitution at a time and score one symbol, in a scratch copy of `src/`, built as sccheck builds it (the unit's own `sc:` directive: a GCC unit's toolchain and flags, `as=`, `overlay=`) |
| `tools/scperm.py` | the permuter: EQOA's tree-sitter mutations of one function, MW-compiled and alignment-scored (needs `.venv`) |
| `tools/ccsweep.py` | compiler identification for the GCC front: one source over every ee-gcc toolchain and flag set (and `//==` source variants), every function it defines scored |
| `tools/ccperm.py` | every order of a marked block of statements, for GCC's scheduled stores: which source order gives the target's |
| `tools/ccflags.py` | a GCC range's drafts recompiled with a flag added: a flag the vendor used loses none (how strict aliasing was told apart) |
| `tools/asmunit.py` | an assembler unit (`.s`) for a library object that was assembly: the rows transcribed (canonical instructions, labels, map names, %hi/%lo of the symbols the code addresses, the unit's own `.bss` labels), assembled by the toolchain named and scored until every function matches |
| `symbols.txt` | **the map** — `0xADDR SIZE name  # note`, kept by hand from here on |
| `data.txt` | the map of global data, the same format and namespace |
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
default is 3.0b52 (the nearer date). A function that separates them settles it (the float
code tried so far -- Slerp, the normalisers, the trig tables -- compiles the same under both,
and under 3.0b38).

**The flags, each by its own evidence** (`scdiff.FLAGS`):

Each flag is necessary: drop any one and zlib's score falls (3.0b52, the others held) -- but
`bottomup` and `level=5`, which zlib cannot see and SOCOM II's C++ needs.

| flag | without it | evidence |
|---|---|---|
| `-O4,p` | `-O3,p` 18; `-O4,s`, `-O4`, `-O3` 6; `-O2` 0 | the only level at 21/21 |
| `-sdatathreshold 0` | 18 | the target addresses zlib's 6-byte version string with `lui/addiu`; the default (8) puts it in `$gp` small data |
| `-inline auto,deferred` | `auto` 20, `deferred` 18, `on` 17 | `inflateReset` is absent from the image — inlined into both callers and stripped (`auto`); functions emit in **reverse definition order**, exactly as the image holds them (`deferred`) |
| `-inline bottomup` | (zlib alike) | inlining starts at the leaves of the call graph, so a callee arrives with its own callees in it: `CZAnimExVector::Free`'s `delete` inlines CZAnimEx's destructor inside `std::for_each`'s function object, and the recursive `CZAnim::Reset`, Pause and Resume inline themselves twice with their callees in the outer copies -- four functions that matched only with it, nothing lost |
| `-inline level=5` | (zlib alike) | a call whose callee's whole inline tree is at most five high inlines all of it; a taller one inlines only the callee's own body and calls what it calls. Five, not four: the deque's teardown destructor inlines five this-tested levels, CCompass::CreateAnchor inlines CCompassAnchor's constructor with its vector<CPnt3D>'s, CVisual's constructor CDynTexList's five levels, ~C2D four destructor levels and the deleter's iterator temporaries -- all impossible at 4, which had been fitted (2026-10-24) with every MSL header a level too shallow to compensate. The inline-level lane (2026-10-25) refitted the headers at 5 -- each change following SOCOM 1's MSL where its DWARF shows it (the free swaps, the capacity pair's swap, alloc_ptr, the node allocator) -- and every level-4 match holds but SaveNodeStates (below); level 6 needs one level more in five chains (see "Open, and next"). **A second limit cuts as the level does: `#pragma inline_max_size`** (default 256 units, measured: a store is ~8, a wrapper level ~4) -- a callee whose expanded body (its own inlining done) exceeds it is inlined only one level; a leaf always inlines. `new CZAnimSetSave` (179 units here) is that cut in the target, at any level |
| `-enum min` | 13 | `inflate`'s `mode` field is stored with `sb` |
| `-Cpp_exceptions off` | (C++ only) | `CMatrix::SetZero` is `j memset`: with exceptions on, MW keeps a frame around every C++ call, while C tail-calls either way; every C++ match held when it went on |

Driven through **wibo** (decompals' minimal Win32 loader, as decomp.me does): ~10 ms a
compile against wine's ~2 s, and the 2003 builds — which wait forever on a FlexLM licence
prompt under wine — just run. `SC_WINE=1` falls back to wine. `-gccinc` makes `-I` paths
serve `<...>` includes; `-nostdinc` keeps the host's headers out.

### MAIN's Metrowerks runtime: three groups, three flag sets

MAIN 0x180000-0x18DB90 is MW-built code outside the game's overlays (the MAIN runtime lane, round
1, 2026-11-03: none of it matched before, 81 rows after). A unit names its own build and flags in
its directive (`/* sc: compiler=mwcps2-3.0b52-030722; flags=... */`), as the GCC units do:

| group | units (src/main/) | build | flags | evidence |
|---|---|---|---|---|
| the game's own MAIN units | the pool unit zmalloc.cpp 0x1801E0-0x18194F (the name ours; its `__sinit` 0x1D4B00), MAIN's copies of zsys_memory.cpp (0x181950-0x181C1F) and zvid.cpp (0x181C20) | 3.0b52 | the game's | `__nwa__FUi` is a tail call (`j`), which needs exceptions off; `_delete_vec` exact on every 3.0 build; round 2: the pool unit's 17 rows and its initialiser at the game's flags |
| MW's C++ runtime | runtime/: gcc_wrapper.c 0x181C50, mwoverlay.c 0x181C90-0x181E2F, NMWException.cpp 0x181E30-0x18280F, global_destructor_chain.c 0x182810, __init_cpp_rts.c 0x182840, ptmf.c 0x1828A0; the exception-handling unit 0x182930-0x1845FF | 3.0b38, b50 or b52 | the game's plus `-Cpp_exceptions on`, deferred | `__destroy_arr`/`__destroy_new_array` match only on 3.0b22-020926 and 3.0b38+; `__initialize_cpp_rts`'s short-loop nops rule out 3.0b22-020926; with exceptions off `terminate`/`unexpected` become tail calls (9 words off); `,p` required; without `-sdatathreshold 0` `__register_global_object` is 8 words off; the image holds the PowerPC runtime's source order reversed |
| MSL C++'s prebuilt library | msl/: ios.cpp 0x184600, iostream.cpp 0x184C80, locale.cpp 0x18CF20-0x18DB8F (SOCOM 1's DWARF spans), with its own msl/msl.h | 3.0b38, b50 or b52 | `-O4,p -inline auto,bottomup,level=5 -enum min -Cpp_exceptions on -wchar_t on`, NOT deferred (round 2: the default small-data threshold, not 0 -- `locale::global`'s 8-byte static locale is `addiu a0,gp,-31928` and `__use_facet`'s facet ids `lw -31904(gp)`; round 1 had carried the runtime group's 0 over; and an explicit level: `~__locale_imp<1>` inlines the facet vector's three destructor levels only at level>=3 or with bottomup, plain auto calls it -- nothing tells the two apart, so the game's pair) | `showmanyc<wchar_t>` divides by 4 where SOCOM 1's divides by 2: a 4-byte `wchar_t`, which only 3.0b22-020926 and later give; `do_widen<wchar_t>`'s six-instruction loop rules out 3.0b22-020926; `getloc` keeps its frame (exceptions on); SOCOM 1's DWARF lines rise with the addresses (source order) |

So SOCOM II's prebuilt MSL is **not CodeWarrior 2.4's build**, though SOCOM 1's DWARF names that
install; and none of the 81 rows separates 3.0b38 from 3.0b50 or 3.0b52. The `.comment`'s
2.4.1.01 comes from none of these groups -- MW's assembler writes DWARF but no `.comment`, and
crt0.s (`_start`, `_exit`, `_root` 0x1801D0) is one MW-assembled object (SOCOM 1's DWARF has its
unit), which scdiff has no MW assembler path for -- GNU as gives the same bytes (round 2: src/main/crt0.s
through the assembler path, all three byte-exact; `_start` ends `Exit(main(argc, argv))`, so 0x1C4CC0 is
the boot ELF's main, unnamed only because OVL1's main.cpp holds the map's `main`). Rules measured on the way: with `-wchar_t on` a `wchar_t` is a 4-byte built-in from
3.0b22-020926 on yet loaded and stored 16-bit (`sh`/`lhu` with a 4-byte step); six-instruction loops
tell builds apart (3.0b38+ leave the delay slot empty and pad two nops, 3.0b22-020926 fills it); in
the non-deferred library an inline callee is inlined only if generated before its caller (`template
class fpos<int>;` goes first); each overlay links its own copy of non-template units as well as
templates (one source order reproduces MAIN's and OVL1's zsys_memory.cpp); the PowerPC runtime's
bodies compile byte-exact on MIPS once adapted to the ABI (the destructor flag `short`, 64-bit
`long`); MW inline `asm` functions reproduce an assembly runtime function byte-exact; a local copy of
a pointer parameter is numbered right after the parameters. The sweep (ccsweep extended to the MW
builds, adapting flags the older builds reject) is tools/mwsweep.py (round 2 moved it there; it
defaults to the unit's own `sc: flags=`).

Round 2 (2026-11-03, 136 of 141 in src/main: the pool unit zmalloc.cpp, ios.cpp whole, the
istream/ostream classes over a virtual basic_ios, crt0.s) measured: `~ios_base` matches only under
`#pragma opt_strength_reduction off` (the image recomputes `n*8` on every pass of the callback loop,
as SOCOM 1's copy does; `~__locale_imp`'s loop is strength-reduced, so it is not library-wide, and no
command-line option does it); MW 3.0 appends a function template instance's RETURN TYPE to its
mangled name (`min<i>__3stdFRCiRCi_RCi`, `..._RCQ23std8ctype<c>`), so names carried from SOCOM 1 for
template functions need the suffix; a virtual-base destructor thunk is `@offset@dispoffset@name`
(`@8@72@__dt__...`: a class with a virtual base keeps the base's displacement word just past the
base, +72 for basic_ios); `delete (--p->ref_ == 0 ? p : 0)` is the image's `movn` null-test merge;
case labels written ascending give a descending compare chain; `T *p = a = b = f();` keeps the
result in v0 for every use; in the non-deferred library an inline template defined before a caller
is always inlined, defined after it is called there and then everywhere else too (RefCountedPtr's
operator= defined after the streambuf instantiations keeps their `imbue` calls out of line); ios.cpp
(0x184600-0x184C7F) holds three functions the image dead-strips (failure's constructor, `imbue`,
`~_MSLstring`) -- `no-target` rows, two more in NMWException.cpp -- and they are what makes the unit
emit `~locale` and the RefCountedPtr<char> pair; 0x1D4B80 is iostream.cpp's initialiser (`__nInit`,
`__wInit` and their destructors). Open: `__nw__` (0x508 against 0x4E0) and func_001805A0 (0x500
against 0x4DC) -- the switch's compare chain is right but the image puts each case constant in a0 and
leaves every delay slot empty, ours uses v0; `do_length<wchar_t>` (0x38 against 0x3C) -- the image
calls `min<int>` from its first user and inlines it in the later ones, which no definition order or
inline flag reproduces both of. Not started: the filebuf bodies (26 rows), the exception-handling
unit (18), `__nInit`/`__wInit`, `ctype<wchar_t>`'s range maps, `__locale_imp::replace`, the rest of
locale.cpp, the logic_error family, the other three `__sinit` rows.

Round 3 (161 of 244 in the range: 27 more, all iostream.cpp -- the filebuf members for char and
wchar_t: imbue, sync, pbackfail, uflow, underflow, read1-3, write2, unshift, seekpos<wchar_t>;
`__nInit`/`__wInit`'s constructors and destructors; xsputn<char>; the standard streams' data rows)
measured: MW decides an inline when a template member is INSTANTIATED, not when it is emitted -- a
member instantiated mid-unit keeps calling a template already called out of line, one instantiated
at the unit's end (the vtables' pass) inlines it (build/scratch/rt-r3/t2.cpp) -- which is why the
FILE constructors and ~basic_filebuf stay open (the target's constructors inline the locale
pointer's destructor yet call its copy constructor); codecvt<wchar_t>'s do_out/do_in keep their
4-byte copy a loop only under `#pragma opt_unroll_loops off` (`-O3,p` and `-opt nounroll` change
nothing; every build fails them alike, so 3.0b38/b50/b52 stay undecided); a one-byte enum's bitmask
operator returns the enum, so the image masks again (`andi 0xff`); case labels in any order give the
compare chain in the reverse order; `!(a < b)` on two plain loads materialises a bool, `a <= b` with
the operand loaded first on the left branches directly; for 4-byte locals the later-declared gets
the lower address, a 1-byte local packs at the end; `ungetc(buf[--n], f)` recomputes the index each
pass, `--n;` alone is strength-reduced; MW's explicit instantiation takes single member functions
but not constructors or destructors, and none once the class is instantiated; msl.h's `_impure_ptr`
is `__attribute__((section(".data")))` (a plain 4-byte extern is $gp-addressed; the image reaches it
absolutely, 0x1CC750). THE `__sinit` ROWS, corrected: 0x1D4BF0 is iostream.cpp's (two init guards,
0x1D5628/0x1D562C, beside the codecvt<wchar_t>/ctype<wchar_t> ids -- SOCOM 1's guard-only shape),
0x1D4C20 locale.cpp's (a 128-entry table of shorts at 0x1CA420, two guards after ctype<char>'s id),
and 0x1D4B80, round 2's "iostream.cpp's", is a game MAIN unit's (it addresses its `__nInit` object
without $gp: every game unit including <iostream> builds one -- 25 more such initialisers in the
overlays). A try harness that honours a unit's own compiler and flags line is
build/scratch/rt-r3/var.py (tools/try.py takes scdiff's defaults, so the library units fail under
it). MAIN's zvid.cpp copy (0x181C20, marked `ovl-copy: zVid_Assert__FbUiPci`) still scores against
OVL1's row: sccheck takes a unit's home from its functions' named rows, all OVL1's for a unit that is
wholly another overlay's copy -- fixed by GCC round 11a's `/* sc: overlay=MAIN */`, which zvid.cpp now carries.

Round 4 (183 of 244: 22 more) wrote MW's EXCEPTION HANDLING whole -- three units, not one:
ExceptionDecode.cpp (0x182930-0x182A87, the two number decoders: in either other unit auto-inlining
expands them everywhere, UnwindStack growing from 0x844 to 0x1650), ExceptionHandler.cpp
(0x182A90-0x18411B, the table walk) and ExceptionMIPS.cpp (0x184120-0x1845F7, frames and registers --
FindExceptionRecord calls the two-store __FindExceptionTable out of line, so they are apart) -- all 18
at 3.0b52 with the game's flags plus `-Cpp_exceptions on`, deferred (the walk compiles 12 of 12 under
3.0b22-020926, b38, b50 and b52 alike and at most 2 under any 3.0.1 build: the builds stay unseparated);
and locale.cpp's locale::global, locale::classic, ctype<char>'s constructor and ~ctype<char> (round 1's
open). The library gains `-str readonly` (classic's "C" literal is addressed absolutely, not through
$gp; nothing else moves). Measured: MW emits an `asm` function at once, in source order, and defers C
functions to the unit's end in reverse; registers saved into a struct MW copies come out as a 21-pass
quadword copy only as one nested struct (a one-word nested struct goes through $f0 via an address
register); a type-punned store (`*(unsigned int *)&var = ...`) keeps a variable on the stack with its
dead stores and reloads, where an inline's out-parameter is promoted to a register; MW computes a value
as late as its first use, so a value the image computes early needs a variable of its own (a flag read
through a pointer before the pointer is reassigned is computed ahead of the calls, from a register
variable it sinks below them); `x ? x : 0` reproduces a do-nothing relocation test on a catch block's
type string; PowerPC's `#pragma exception_magic`/`__exception_magic` work here; a local declared after a
`try` sits below the catch's compiler-made CatchInfo -- declare it before; function-local statics
under `throw()` match directly (their guards and objects unchecked by scdiff); destructors referenced
only from exception tables never reach the image (the link dropped the tables): `no-target` rows for
them are expected. `__TransferControl` and `__throw` were 0x70/0x80 in the map -- the seed counted
padding -- and are 0x64/0x7C (SOCOM 1 agrees); 0x184340 is `__PopStackFrame`. Open: locale.cpp's own
string pieces (the constructor 0x18D2C0, marked ovl-copy of 0x2396F0; reserve 0x18D8C0, which throws
length_error; the range replace 0x18D3E0; assign(first, last) 0x18DB40; ~out_of_range 0x18D830) --
MSL's string internals and the logic_error family with exceptions on are modelled nowhere yet;
locale.cpp's `__sinit` 0x1D4C20 fills `__ctype_map` entries 9-127, the ones holding several mask bits:
the mask is likely a two-byte enum whose `operator|` is a function, which renames the wide ctype rows
(SOCOM 1's one-byte names); ~bad_exception is generated last where the image has it before __unexpected.

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
- **The game's units link in project order, which the initialiser tables record.** Every unit
  with dynamic initialisation has a `__sinit_<file>`; the MWo3 header's ctors range lists them in
  link order (OVL1 125, OVL2 16), and SOCOM 1's demo names its 296. `tools/s1units.py` pairs
  them (exact pairs, then by shared names; a shape alone is only a hint): zMath is four units,
  `zmath_main.cpp`, `zmath_matrix.cpp`, `zmath_pnt3d.cpp`, `zmath_pnt4d.cpp`, in that order.
  The thunks live in the data area and are compared like any function (all four zMath ones
  match, which pins the statics and their inline constructors -- CQuat's stores `w` first).
- **`-inline auto` inlines within a unit and never across**, so the image's inlining reads
  unit boundaries: `CPnt3D::Interp` is `Scale(1 - t, out); b->Scale(t, &tmp); out->Add(&tmp,
  out);` with both calls inlined, while CQuat's code in `zmath_main.cpp` calls `Scale`/`Add`.
- **Return types are not mangled but they move registers**: both `CPnt3D::Normalize`s return
  the length (kept in `$f0`, it fixes every other register; void, MW numbers them the other
  way round), a bit test returns `bool` (an `int` adds `andi 0xff`).
- **MW 3.0 mangles a function template's return type** (`_close_enough<f>__FRCfRCfRCf_b`);
  SOCOM 1's 2.4-era compiler did not, so every carried template-function name needs it.
  A 128-bit type mangles as `1` (`CVU::Code(u_long128 *)` is `Code__3CVUFP1`).
- **Hand-written VU0 code is a CodeWarrior `asm` function** (`asm void f(...) { ... }`), which
  reproduces it exactly, delay slots and `addi` included (`Vu0MatrixInverseWithUniformScale`
  and three point-transform loops, first try). GCC-style `asm("...")` strings are scheduled
  instruction by instruction with the C around them; an `asm { }` block is a barrier, but
  drops a repeated `qmtc2.i` -- `CBBox::GetIntersect`'s last two instructions are still open
  on exactly that.
- **VU0 code inside a C function costs -inline auto by its form** (autoprobe): a GCC-style
  `asm("..." : : "r"(a), ...)` counts each instruction AND each operand, an `asm { }` block
  (Metrowerks' syntax, naming the C variables) its instructions only. `CCamera::GetZ` is four
  locals, nine VU0 instructions and a return: 17 as GCC asm, 14 as a block -- and InsideMidPlane
  inlines it in the target, so it is a block. It must be **`asm volatile { }`**: a plain block
  lets MW see that `aPosPtr` is `&aPos` and fold it into the return (an instruction longer),
  where `volatile` (or GCC asm, which takes its operands as clobbered) keeps the pointer. SOCOM
  1's line table agrees -- each instruction of a block has its own line, while GCC asm strings
  give the first two one line. GCC-style asm binds its operands left to right, a block's named
  variables follow the arguments' right-to-left evaluation (`CMatrix::Transform`'s helper is
  GCC-style: &input is set up before &output).
- **SOCOM 1's CPnt3D has a written copy constructor and assignment; SOCOM II's has neither**
  (SOCOM 1 links out-of-line copies of both; adding them here regresses 12 matched functions).
  So a struct copy is the compiler's: an assignment from a pointer copies field by field and
  scalarises its target, while copy construction puts it in memory -- find_convex_hull_xz's
  swap assigns (`t = pnts[m];`), and `CPnt3D t(pnts[m])` misses.
- **An `s1:align` name is a prediction**: CCamera's FOV setters at 0x2911B0/0x2913F0 carried
  SOCOM 1's names by position, but the code that calls SOCOM 1's setters (zAnimCameraTick and
  Begin, CSaveLoad::Load) calls 0x2912C0/0x291490 in SOCOM II, with (f, true): those are SOCOM 1's
  setters with a flag added, and the pair that had their names are new wrappers taking degrees.
- **An inline call in an expression is expanded ahead of the expression**, out-of-line calls
  included: `if (GetMidPlane() > DistToCell - cell->GetRadius())` loads the plane before
  GetRadius is called and keeps it in `$f20` across the call, where `m_mid_plane` read as a
  member is loaded after the call (and `this` is kept instead). An evaluation order that only
  an accessor explains is an accessor (InsideMidPlane, both overloads).
- `long` is 64 bits (zlib's `uLong` argument is `lwu`-loaded into a 64-bit slot).
- Small params: a callee's prototype changes the caller's scheduling — the loader matched only
  once its DNAS callees took `unsigned int` lengths.
- **Scheduling is global: a mismatch far from the tail can be the tail's.** `zAnimRangeTestTick`
  left eleven delay slots unfilled and came out fourteen instructions long, all of them in its
  first half, until its six tests were written `if (d2 < r) return true;` falling through to
  one `return false` instead of `return d2 < r;`. Fix the return shape before the middle.
- **A member call evaluates its arguments before it loads `this`**; called through a cached
  local (`CZAnim *anim = ZAnim.m_CurAnim;` -- the ZAnim handlers all read it once), the
  register shapes follow. **A virtual call goes through t9**, a function pointer through v0.
- **Declaration order assigns the callee-saved registers**: `zAnimObjectTranslateStateTick`
  matched once `node`, `ref` and `moved` were declared ahead of `anim` (otherwise the same
  code with s1-s4 permuted).
- **MW scalarises a local struct unless something keeps it in memory**: a copy out of an
  inline that returns by value from an lvalue (`CNode::GetPosition`) stays on the stack,
  stored through one pointer; a named return value, the three-float constructor or a
  brace-initialised `PNT3D` (zeroed first) all differ.
- **`(bool)x == false` keeps the bool conversion** (an `sltu` before the branch) where `!x` and
  `x == 0` test the bit -- likely a bool accessor's result compared in the original.
- **`return true` compiles to `li`, a bool assigned `true` to `daddiu`** -- and an inlined bool
  function's `return true` becomes an assignment, so StateRunning (bool) is `li` out of line and
  `daddiu` inside PauseSequence. A switch's case tests run in reverse source order.
- **Inlining reads unit boundaries and declared sizes**: CZAnimMain::Unload calls
  CZAnimNameTable::Unload, which -inline auto inlines from the same unit -- so the name tables
  are a unit of their own. A function too big for -inline auto in the target's eyes but inlined
  there (CZAnimNameTable::AddName into Load) was written smaller: its two appends one inline
  helper. A function inlined everywhere and absent out of line was `inline` (NewAnimSet).
- **Two-byte copies show the type's alignment**: a halfword union copies with lhu/sh, a
  two-char struct bytewise (a command's id).
- **-inline auto takes a function of at most 15 statements, by MW's own count** (the
  `inline_max_auto_size` pragma's default; `tools/autoprobe.py` measures any shape): every
  statement of the lowered body but its labels -- an expression statement, a return (a void
  function's closing one too), each branch (`a || b` is two), the goto of an else or of a loop's
  entry, a switch -- counted before anything is inlined into the function. Code compiled to
  nothing still counts (`(void)0;` 1, `if (0) f();` 2; `;`, `{}`, `do {} while (0);` 0). So
  CZAnim::Running (10) and GetNodeRef (11) inline, but Start (11), StopAnims (12) and
  ExecuteSpecialSequence (15), never inlined in the target, would too: the original's had more
  statements (debug code?) or a pin. **The pragma state at a callee's definition decides
  whether it can be inlined at all**, so `#pragma auto_inline off` around those three
  definitions reproduces them (probe: build/scratch/probe/q.cpp). Calls that do not inline are
  unit-boundary evidence: LoadZAR calls GetAnimSet(int), which anim_main.cpp inlines, so the ZAR
  loaders are a unit of their own (zanim_zar.cpp).
- **Inlining is bottom-up to level 5** (`-inline bottomup,level=5`). At each call site, a callee
  whose whole inline tree is at most five functions high (it counts as one; an empty inline
  function counts too) is inlined entirely; a taller callee is inlined ONE level -- its own
  body, its callees called out of line, where each of those copies inlines by the same rule
  (probe: build/scratch/ctor/chain.cpp, chain2.cpp). So a deep MSL chain shows up as calls to
  tiny out-of-line copies -- the 78 three-word zeroing constructors in the image are mostly
  that -- and the header's levels must be the real ones (include/vector carries the counts it
  was fitted to). **The size limit cuts the same way**: a callee whose expanded body exceeds
  `#pragma inline_max_size` (256 units by default) is inlined one level whatever its height
  (`#pragma warn_notinlined on` makes MW name every inline call it leaves a call -- the quickest
  way to read heights off a real unit; build/scratch/lvl/ has notinl.py, ubisect.py -- a
  function's match range over inline_max_size -- and psize.py/csize.py for probes). Tail-code
  cuts in the target are often this limit, not a level: a list's insert expands to 253 units
  in an out-of-line push_back, 3 under. A function object's call inlines inside `std::for_each`, and a recursive
  function inlines itself twice, the outer copies carrying their own callees (Resume and Pause
  inline Running into the first two copies and Running's Source test into the outer one, then
  call the sibling accessor and the Source test -- hence those out-of-line copies).
- **MW evaluates a call's arguments right to left** (inlined constructors included): CZAnim::
  OnWeaponHitInit computes the root node before it loads the health's values, so CZAnimEx's
  constructor takes the root last.
- **Declaration order picks the argument registers too** -- the later-declared pointer takes the
  lower one: vector<CPnt3D>::reserve's copy loop matched only as `T *d = temp.data_; T *p =
  this->data_; T *last = p + this->size_;`. A loop whose end is re-read each pass (a member that
  a store might alias) is never unrolled; computed once into a local, MW unrolls it 8x (the
  vector reserves' copies). Nor is a loop whose counter is a parameter: the pod vector's range
  init walks a local copy of its `first` (`for (InputIterator p = first; ...)`), and only so is it
  unrolled as the target's is.
- **SOCOM 1's DWARF describes MSL's out-of-line template functions** -- 41 from `<vector>`,
  11 from `<list>`, 11 from `<algorithm>`, 18 from `msl_utility`, under `C:\apps\cw301\PS2
  Support\Msl\...` (CodeWarrior 3.01's headers): their signatures (a class iterator by value
  shows as a reference -- MW passes it by address when its copy constructor is user-declared),
  their registers and their header line numbers (`dwarf1.py func`). The shapes include/list
  copies come from there.
- **An inline function's string literal is MW's `@STRING@<function>` pool**, not local-bound so
  every unit emitting the function can share it; scdiff compares it by content as it does an
  anonymous literal (MSL's "Memory allocation failure" in the inlined allocate).
- **SOCOM 1's DWARF never says `bool`: MW's DWARF 1 writes it as `unsigned char`** (none of its
  77,784 members is bool; `std::basic_string::makeUnique` mangles a `b` its DWARF calls
  `unsigned char copy`). A DWARF `unsigned char` may be a bool: CZAnimMain::m_anim_complete is,
  by CZAnim::Tick returning it unconverted as the scheduler callback's bool.
- **A call in an argument list runs before the plain loads of the same call; a call made as a
  statement keeps a load written before it in place.** `GetName()` reads the index before
  fetching the set, so its set comes from a statement of its own (probe: build/scratch/probe).
- **A pointer vector's begin() is three high, and the walks show it**: begin()/end() convert
  the base's raw pointers through a member taking them by reference (include/vector), and
  begin() takes the base's pointer into a *const local* first. Three constraints fix that
  shape: SetDynamicLight's partial inline calls the base's begin() and then __iter, out of
  line (so begin() calls both); FindChild by name keeps &data_ in a register (so the base's
  begin() reads through data()'s reference); and SetDynamic and SetUseParentBBoxTree, which
  assign begin() to a declared iterator, inline it whole in their outer copies (so it is three
  high, not four -- passed straight to __iter, the call sat a level deeper; a plain local
  instead of a const one swaps two of SetDynamic's registers). A one-level begin() folded a
  walk's first test (begin against begin + size) into `n == 0`. An iterator *assigned* from
  begin() is a level deeper than one *initialised* from it. A class iterator's `!=` (as
  `!(==)`) materialises when it is the whole loop test and branches inside `&&`.
- **Template instances close a unit; an inline function's out-of-line copies follow the first
  function that calls them.** node_vector.cpp ends with std::remove_copy's instance (0x312480),
  node_main.cpp with its remove_copy and list instances (from 0x3176B0); but SetModeTree's
  iterator copy constructor, postfix ++, assignment, default constructor and its inline setter's
  copy (0x313450-0x3134BB) sit right after SetModeTree, mid-unit, in reverse of their first use
  -- which an earlier boundary misread as a unit's end (Create, inlining node_main.cpp's
  SetName, and the press beta's file references corrected it). An instance an earlier unit
  already emitted is not repeated (the linker keeps the first copy).
- **A call's argument set-up follows its parameter order**, so it can recover a prototype the
  callee's own code cannot (registers are allotted per class, ints and floats apart): zmath's
  two projectors take the matrix last, as SOCOM 1's CalcWorldToUV did -- only that order puts
  CNode::ApplyDecal's `mov.s $f13` ahead of the matrix's `addiu`; ScrollTexture's call to the
  texture effects takes InitScrollTexture's order. A value threaded through calls stays in v0
  (ScrollTexture passes each call's returned index to the next, and returns it).
- **-inline auto refuses a function holding a call that cannot inline whole**: without its call
  to SetDynamicLight (recursive, self-inlined twice), SetApplyClutterFlag inlines itself twice;
  with it, not at all, as in the target. SetApplyDecals and SetApplyMultipassEffects have no
  such call and count twelve statements, yet the target never inlines them (nor did SOCOM 1's
  beta, no asserts in either), so they are pinned with `#pragma auto_inline off`.
- **The vtable pointer sits where the first virtual is declared among the data members**:
  CGameState declares its virtuals first (vptr at 0), CGameMenu its members first (vptr at
  0x910), CNode after tag_NODE_PARAMS (0x60), CDI after its 12 bytes of parameters (0xc).
- **Small tells from the game-state and HUD units** (the agents' notes): a derived destructor
  that calls the base destructor in its body runs the base teardown twice, and both builds do
  (CCoreState); an empty `default:` or `break` leaves no block, so CLetterBox::Tick needs
  `default: return;`; narrowing an argument to an unsigned-char enum folds into an `lbu` at the
  call, while `lb` plus `andi 0xff` in the callee is an int parameter narrowed inside it; MW
  never loads a float constant from memory for a store, so a float read with `lwc1` from data
  is a variable; a `!flag` materialised (sltu, xori) inside `&&` is an inline returning
  `!m_loading`; a global pointer read twice is re-read after a char store, and taken into a
  local once it stays in a register (CCoreState::OnPush); MW calls a member object's virtuals
  through its vtable.
- **MW's inline budget, measured** (build/scratch/inl, mwtest.py; at level 4 when written): a
  callee whose inline tree is at most the level high inlines whole, a taller one only its own
  body (its calls stay calls). A recursive function's self-copies grow by one per level (a
  probe's r0 adds a copy each level from 3 to 8); a recursive auto-inline function of height H
  (itself and its callees) inlines itself only from level H (SelectNextObjective never does in
  the target: at level 6 it would). A
  function's height ignores calls made as arguments, but a call written as an argument of an
  inline call at the top is checked a level deeper (`f1(w2(p))` inlines w2 one level where
  `1 + w2(p)` inlines it whole). A recursive function inlines itself until the budget runs out
  (CNode::SetNodeEx: twice) and the innermost copies call the vector's pieces out of line.
  That fallback is for functions declared `inline`: a function MW takes under `-inline auto`
  (one defined in the unit) is inlined whole or not at all -- CNode::CreateInstance(name)
  inlines CreateInstance(model) only once the pointer list's push_back is short enough to keep
  the whole tree within the level.
  An assignment's right side nests a level too (`first = find(...)`): MSL's std::find tests
  `==`, not `!=` (which calls `==`), or the inlined std::remove in CNodeVector::Remove could not
  inline it whole. Measure with probes before theorising: three models of this died in one
  sitting.
- **A bool that only an inline function explains**: CModelVector::GetModel's predicate
  materialises `!strcasecmp(...)` (sltu, xori) and then branches on it -- what a call to an
  inline `bool` function compiles to, where the test written in the `if` branches on the call's
  result and a bool local adds an `andi 0xff` (CNodeNamed has that one: SOCOM 1's too).
- **A local the found iterator lives in**: `iterator it = std::find(...); return it != end();`
  schedules the temporaries' stores with their copies; `return std::find(...) != end();` does
  not (the four Exists).
- **Block order follows the early return**: `if (!archive) {...} else return 0; return 1;` and
  `if (a >= b && !Reserve()) return 0;` each put their return-0 block where the target has it.
- **The same inline written two ways serves different callers**: `CZAnim::GetArgPnt3D` must be
  a ternary (TranslateState, RangeTest), `CZAnimMain::CmdHdr` a ternary for BreakTick,
  GoToEndIf and FOR_CHILDREN -- while IfTick reads the current command the if-assign way.
- **Tells from round 3** (the camera, zoom and HUD lanes, and the motion script):
  - a byte enum (`-enum min`) is copied with `lb` but compared or indexed with `lbu`, so the
    load's sign tells an enum from an unsigned char (CAMVIEW, LENS_FX);
  - a byte field that is tested and then passed on wants `x == 0`: `!x` re-extends it with an
    `andi 0xff` for the call (zAnimObjectMotionScriptTick);
  - `if (x)` on a float emits `c.eq.s x,0`, x first; `x != 0.0f` puts the zero first;
  - a `b` to the very next instruction is an if/else whose trivial arm vanished -- `else step =
    *dT;` once the load is hoisted above the test (apply_script_frame);
  - `t = c; if (b) t = b;` puts the load in the branch's delay slot; `b ? b : c` is branchier;
  - `float s = a * b; f(s + c)` keeps `s + c`; as one expression MW emits `c + a*b` whichever
    order the source has. `amp = base; amp += k * r();` reads `base` before the call, a single
    expression after it;
  - MW drops an inlined callee's stores to a global that a later statement overwrites before
    any call (Reset's inlined SetLensFx), and reuses a value just stored to a global instead of
    reloading it, until a call (BeginShake);
  - an inline setter that returns the old value gives "read old, store new, store old"
    (`CAppCamera::SetCtrlView`);
  - at exactly the 15-statement limit the callee's shape counts: `if (!held) {...}` is one less
    than an early return, a ternary is one, and statements can move into a nested inline;
  - pin with `#pragma auto_inline off` every function the target calls that MW would inline,
    and the functions calling them too -- a pin does not carry over to the caller;
  - under `-inline deferred` definition order does not change what inlines (probes in
    build/scratch/hud/inl/);
  - an iterator ASSIGNED from a pointer list's begin() is copied bitwise (lwc1/swc1) and then
    lives in memory, re-read and re-stored at every step; one INITIALISED from begin() lives in
    a register. SOCOM 1's list: `begin()` is `iterator(tail()->next_)` through a node*
    constructor, `end()` is `iterator(tail())` through a node_base* one, and the base
    iterator's `==` is a free function in std (include/list now has the wrapper iterator, its
    one member `i_` at 0 as SOCOM 1's DWARF has it);
  - an inline accessor held in a local (`t = tail(); t->next_`) is computed first and shared
    with a later identical expression, which moves registers;
  - scdiff's "N differing" count inflates after a one-instruction shift: read the diff.
- **Tells from the ZAnim motion handlers** (round 4, the integrator's lane):
  - a frame's named objects are laid out in reverse declaration order, grouped by type (CQuat
    temporaries, return slots and locals below the CPnt3D ones); the from-to tick needed its
    `CQuat q` declared before `to`;
  - `p = a + b` gives operator+'s result a return slot of its own only when `p` is scalarised
    (only its fields read); an address-taken `p` is built in place;
  - a ternary initialising an object selects a pointer and copies after the join; the target's
    construction in each arm is an inline function returning by value (`FromToGoal`);
  - a byte enum compared with an enumerator zero-extends (`andi 0xff`), its truth test does not
    (ProcExpr's operator) -- with the round-3 `lb` copy, the enum's whole signature;
  - a switch's compare chain runs in REVERSE case order (cases 3, 2, 1 test 1, 2, 3);
  - `x = 0; if (p) x = q;` is if-converted, the test materialised (sltu, xori); CmdHdr's
    ternary ASSIGNED to a local declared before compiles to the target's if-assign shape (zero
    in the delay slot, then the load), while used straight in an expression or initialising it
    branches over the zero (ProcExpr). zAnimIfTick still resists: MW hoists each
    constant-address `m_cmd_*` load ahead of the CmdHdr diamond, whichever operand it is;
  - an `unsigned short` offset tested and then added wants `!= 0`: a truth test re-extends it
    (`andi 0xffff`) for the add; likewise a byte tested and then passed wants `== 0`;
  - MW's saved registers follow declaration order for most locals, but an anim pointer read
    from `ZAnim.m_CurAnim` takes the lowest wherever it is declared (the motion-script tick and
    the from-to begin, where the target gives it s2: still open);
  - an address-taken local's fields read through an inline's REFERENCE parameter come off the
    frame (`lwc1 268(sp)`); read directly they reuse an address MW hoisted into a saved
    register for the local's copies (the light tick's `SetPosition(const CPnt3D &)`);
  - a union's implicit copy (SOCOM 1's BLENDMODE: sceGsAlpha and the word) holds its source
    address from the block's start; a one-member class's copy loads the word directly.
- **Tells from round 4** (the game states and the HUD):
  - an inlined loop's compiler temporaries count as declared last: they take the lowest saved
    registers and push named values up (CMenuState::Tick);
  - a 16-byte value passed by value to an inline is copied twice: into the return's temporary
    (lq/sq), then into the parameter's (a 128-bit move, `paddub` with zero, and sq) -- SeeFarther;
    MW uses lq/sq only where it trusts 16-byte alignment (a global's start), element by element
    at other offsets of a struct of 4-aligned PNT4D;
  - a byte enum passed to an inline taking the same enum is loaded with lb and narrowed inside
    with andi; an unsigned char or int parameter folds the conversion into an lbu at the load;
  - a global passed to a bool parameter must itself be bool (an unsigned char adds an sltu);
    a test that re-reads members and materialises `&&` before branching is an inline bool;
  - MSL's basic_string destructor is never inlined: called with -1 for a member, 0 for a base;
  - a constructor too tall to inline whole is inlined one level, and its callees' out-of-line
    copies are emitted in that unit; MW 3.0 counts implicit constructors as inline levels, an
    empty user constructor too (probes in build/scratch/hud/p_ctor*.cpp) -- SOCOM 1's debug
    build expanded implicit constructors in place, so its code says nothing about heights;
  - the end iterator taken once into a local (`iterator i, e = end();`) keeps it in a saved
    register, as the target's list loops do; `i != end()` recomputes it from `this`. After a
    call, write `i = begin();` then `iterator e = end();` so `this` dies at the loop;
  - a list constructed before the vtable store is a base class (CLensFlareManager : list, vptr
    at 0xc); a member is constructed after it. A three-word vtable is one virtual function and
    no virtual destructor;
  - a hand unit's out-of-line copy supersedes a draft of the same name even when it misses:
    declare stub destructors non-inline so the unit emits nothing;
  - s1:calls names can slide one member where SOCOM II inserts a call: check each call's
    receiver offset against the member its constructor builds (three were shifted back).
- **Tells from the entity lane** (round 4; include/list now has the pointer list's const
  iterator, built from the base's by value -- a bitwise copy, the base having no copy
  constructor of its own):
  - a const pointer-list walk that ASSIGNS begin() makes four copies (store, bitwise, word,
    bitwise); one initialised from it makes two and then lives in a register;
  - SOCOM II's non-const walks over CEntity::m_list keep end() in a local; with end() in the
    loop test the sentinel and the first node trade registers;
  - walked through a reference local (`const CEntityList &list = GetEntityList()`) the list's
    address sits in a register and the first node is read off it; through the member itself it
    is addressed absolutely;
  - a reference-returning `*` keeps the element's address in a saved register across a call,
    a by-value one reloads it;
  - a static const member defined with its initialiser before a reader is folded into it;
    defined after, the reader loads the word;
  - `this` passed to a `const T &` parameter is homed on the stack at entry and every member
    access reads it back;
  - locals get stack slots in declaration order, the first declared at the highest address;
  - a bool returned through a local (`r = false; ... r = true;`) puts `move v0,zero` in the
    test's delay slot;
  - an inline virtual's out-of-line copy is linked once per overlay (CEntityCtrl::SetThrottle
    at 0x2A26E0 and 0x567340).
- **Tells from the network glue** (round 5, fts_remote.cpp):
  - plain global or member arguments load in parameter order; one that needs a conversion (an
    unsigned global to an int parameter, a uchar field to a ushort one) is evaluated earlier,
    and an inline accessor among the arguments is expanded before an out-of-line call in the
    same list (that is how the connection index lands in a saved register);
  - MW doesn't trust an unsigned short or unsigned char PARAMETER's extension: storing one into
    a wider field adds an `andi` -- which recovers SOCOM II's changed signatures;
  - `beqz a -> L; bnez b -> body; nop; nop; L: b epilogue` is `if (!(a && b)) return;`;
  - a byte loop zeroing a stack struct behind a null test of its address is MW's `= {0}`; a
    struct with constructors can't take it, so packets hold PNT3D, not CPnt3D;
  - a bool argument computed before the body's test was a local in the source; an inline bool
    compared `!= false` keeps its sltu before the branch, a plain truth test drops it;
  - a struct copy of eight unsigned shorts is a four-pass lh/sh loop; a constant eight-pass
    element loop unrolls into eight lhu/sh pairs;
  - CValve's inline setter, as both builds inline it: `state = VALVE_SAME; if (m_value != value)
    state = value == 0 ? VALVE_OFF : VALVE_ON;`, an int parameter, the enum compared against an
    enumerator.
- **Tells from CHUD** (round 5, hud_main.cpp):
  - three `==` tests on one variable joined by `||` fold into a single range test; the target's
    chain needs each test in its own bool local (PauseGame);
  - `bool b = p ? p->f == 0.0f : false;` materialises with `andi 0xff`; written with `&&` it is
    if-converted (SetupRespawnText);
  - a pointer handed through an inline that takes it by value stays in a saved register; only
    the copy goes to memory for push_back's reference (Flash, via CFlashFXVector::Add);
  - an owner read straight (`appCamera->m_bc`) takes its saved register after a local derived
    from it; read through the inlined accessor, the two swap (ZoomOut);
  - an int counter with the index narrowed to signed char is never strength-reduced; a signed
    char counter becomes a walking pointer (CZPauseTest::Draw);
  - a call made as an argument lets constant offsets be scheduled ahead of it; fetched into a
    local first, the call goes first (SetupSpectatingText);
  - a size-guarded `while` over class iterators gives the size test then the compare twice;
    `if (x) { ...; return; }` branches straight to the epilogue where an if/else as the last
    statement leaves a join block (Add2D);
  - a true function-local static matches, but scdiff leaves its BSS guard and object
    unchecked: placeholders hold them to their addresses (SetupGhostText).
- **Tells from the zdb world** (node_saveload, node_util, node_shadow, node_world):
  - a bool returned straight from a bool call tail-calls; kept in a local first it gets `jal`
    plus `andi 0xff` (LoadAssetLib);
  - `if ((p = f()) != 0)` branches on v0 with the copy in the delay slot; `p = f(); if (p)` and a
    declaration in the condition move first and test the copy;
  - a test made once before the loop's own test is `if (c) while (c) ...` (FilterAssetLibFolder);
  - GetChildren() taken into a reference inlines one recursion level deeper than
    `x.GetChildren().begin()` (find_node);
  - a value-initialised empty functor stores a zero byte; a user-declared empty constructor
    stores nothing;
  - MW hoists a read of an old bitfield above a store to another field of the same object, then
    reloads the object pointer for the later read-modify-write (GetAltitudes); an inline bool
    accessor's test reloads the byte for a following bitfield store (DeleteLandmark);
  - SOCOM II instantiates FreeVector on the base std::vector: the temporary's destructor has
    three this-tests, not four;
  - a member whose `this` is never read still passes its arguments in a1 onward
    (GetIntersectBatch).
- **Tells from the sound system** (989snd.c and zsnd/, round 6):
  - the EE sound API is Metrowerks code (16-aligned, MW's prologue, two C++-mangled senders):
    SOCOM 1's `989snd.c`, compiled with `#pragma cplusplus on`; its wrappers pack their
    arguments into a local int array;
  - a loop counter whose type differs from the inline accessor's parameter (unsigned into
    `Item(int)`, int into `operator[](unsigned)`) stops MW strength-reducing the element
    address (CreateSequence, the reverb loaders, the group's pick);
  - a virtual call on a global object is devirtualised: the target's vtable dispatch needs a
    reference to it (CSnd::Open, NetOpen);
  - an inline helper called with a constant keeps products like `0.0f * x` that MW folds when
    the expression is written out (FadeOut's `CalcVol(0.0f)`), and its by-value arguments are
    loaded before its body runs;
  - `n <= m_current` branches on `slt at`; `m_current >= n` materialises the test in v0;
  - `while (!(bool)f())` gives the `sltu; xori` pair before the branch;
  - an early `return;` in each arm explains a branch to the very next instruction
    (CSndJukebox::FadeOut);
  - vectors of float, int and unsigned int run on `__vector_pod<unsigned int>` as the pointer
    vectors do (`~vector<float>` is three this-tested levels, LoadCSndGroup pushes through the
    pod insert at 0x1FE340).
- **Tells from the mission and network game** (round 6: fts_player, fts_action, fts_mission,
  fts_net_seal):
  - a bit-field returned as `x ? true : false` compiles to sltu plus a nop where the ternary's
    blocks vanished; `!= 0` gives no nop;
  - `team > 31` branches on `slti at`; `team >= 32` puts the compare in a register;
  - an enum stand-in with a single zero enumerator is signed under -enum min and sign-extends
    its argument; with the real values (all non-negative) it is unsigned (andi 0xff);
  - a bool passed to an int inline parameter as `b ? 1 : 0` is re-extended for the store
    (LateJoiner);
  - a load written into a call's expression can be hoisted above the call into a saved
    register, where a load or call result written into a local first stays put; in one
    expression `now + n*1000` is emitted product first;
  - a list walk that keeps `&end_.next_` in a saved register reads the sentinel's next field
    through a reference; a walk that assigns its iterator wants `iterator i, e = end();`;
  - a switch value converted to a byte enum in a local before the guarding test gives the
    target's lb in the delay slot plus its re-extension;
  - a constructor whose row is still a placeholder cannot be spelled as `new T`: name the
    constructor first (ftsCreateSeal waited on four OVL2 names).
- **Tells from the C2D family and the weapon pages** (round 6: c2dbitmap.cpp, ztwod.cpp,
  hud_main.cpp's slots):
  - an implicit destructor inlined only one level still gets its full out-of-line copy
    emitted, even when nothing references it (0x1FDCE0);
  - a by-value class argument built by an inline constructor counts as an inline level; the
    copy constructor does not;
  - `t = a; t += b;` loads `a` straight into the register `t` is passed in, where `t = a + b`
    loads both operands into temporaries first (TickFade);
  - statements after an unsigned-to-float conversion's branch stay after it, which fixes the
    order of independent global stores (C2D::Init);
  - `SetColor(p.x, p.y, p.z)` through an inline float overload loads the components last to
    first, before any store;
  - `build/scratch/hud/pvar.py` compiles a probe once per macro value, with `PINC=` putting
    shadow headers first: a quick way to measure inline heights.
- **Tells from the asset manager and the input classes** (round 6: node_assetlib.cpp,
  zinput/, int_main.cpp):
  - a pointer-list walk that reads an element twice across a call needs the base deleter's
    iterator, the element cast out by value; the wrapper's reference-returning `*` keeps the
    element's address in a saved register;
  - a list-derived destructor matches only when defined inline in the class
    (`~CAssetList() { Clear(); }`): it then inlines ~list one level and calls ~__list_imp, like
    the implicit ~CEntityList; defined out of line it inlines all three levels;
  - removing through `__list_imp::remove`, one level shallower than the wrapper's remove,
    stacks its temporaries in the target's order;
  - a functor that writes out its null test (`if (!p) return 0; delete p; return 0;`) gives the
    target's two `move v0,zero`;
  - an inline accessor returning a byte enum makes the caller narrow its own enum argument into
    a fresh register;
  - members zeroed before a class's vectors belong in the mem-initializer list;
  - a one-pass loop over a static array (`NUM_PADS = 1`) keeps the array's lui/addiu form;
  - `x = a - b; x /= c - b;` computes the divisor first, `(a - b) / (c - b)` the dividend;
  - an explicit `p->~T()` passes -1; 0x3C866666 is 1.05/64, not 0.0164.
- **Tells from the sound system, round 2** (989snd.c's stream calls, CSndInstance's pool, the
  headset):
  - 3.0b52 reuses a value just stored to a global only when it comes from a ternary
    (`g = c ? 0 : 1`) or an assignment used as an expression; `g = !c; if (g == 1)` reloads it
    (SOCOM 1's `movz` there is the same ternary);
  - a function that stores new's result to a global keeps it in v0 only if it also returns it
    (Headset::Create); an inline setter's arguments load before its stores (ftsGetHeadset);
  - to keep a pushed pointer in a register, push a copy (`T *kept = p; v.push_back(kept);`):
    only the copy goes to memory;
  - a walk whose arms each step the iterator on emits three copies of the step (PostTick);
  - std::find compares `*first == value`: a target comparing `value == *first`, with first and
    last in the other registers, is a hand-written loop;
  - a constructor whose disabled test is an early return branches over its stores when
    inlined; written as an if-block it branches past them the other way;
  - operand order in a comparison carries through to `bne`: `mode == x` puts the parameter
    first;
  - SOCOM 1's symbol table names every 989snd.c global, and they are defined in that unit.
- **Tells from the network objects** (round 7: fts_net_persona/grenade/bomb/blather/game,
  fts_controller, fts_objectivecam):
  - reading a value into a local before an unrelated store changes the register choice
    (`n = x; t = 2.0f; x = n + 1` against `x++`, which merges the sum into the load's register);
  - a reference local to a global taken after a call reads the first member absolutely and the
    next through an address built in the delay slot; taken before the call, the address sits in
    a saved register;
  - a range test written as a ternary folds into one unsigned compare (sltiu); `if (in range)
    return t[i]; return 0;` keeps the two compares; `if (p) return p->x; return 0;` leaves a
    nop after `move v0,zero` where the ternary does not;
  - an inline bool compared `== false` keeps its sltu inside an && chain, `!` drops it; a
    returned && chain of plain tests builds each step in v0 and branches on it;
  - the network registration's field offsets need `T *p = &local; (int)&p->f - (int)p` (char *
    casts fold them to constants, int casts recompute the address each time);
  - a dead data load the target keeps needs a volatile read (MW 3.0 drops it and tail-calls);
  - `CPnt3D p(s.x, s.y, s.z)` loads z, y, x and stores x, y, z;
  - `int i = t - 1; f(i)` keeps the subtraction, `f(t - 1)` folds the -1 into the table address.
- **Tells from the C2D implementation** (round 7: c2dbitmap_poly, c2dpoly, c2dline, c2dstring,
  zui_2d):
  - float rows that must land on 16-byte offsets need a 16-aligned type (`sceVu0FVECTOR`):
    C2DPoly's arrays reach 0x30 after its PRIM word only that way;
  - an inline virtual's out-of-line copy comes with each unit that emits the vtable, the linker
    keeping the first: one class's slots can end up split between two units (C2DBitmapPoly's
    6/8 against 7/9);
  - `if (IsOn() != false)` (or a bool local) gives the target's `andi 0xff` after an
    out-of-line bool call; `if (IsOn())` branches on v0;
  - a test the target branches on directly must be written out each time: an inline bool
    function makes MW compute the result first;
  - `m_RGB[i < 3 ? i : 2]` keeps the ternary's empty arm as a `b`; `if (i >= 3) i = 2;` loses it;
  - a chained `a = b = c = K;` stores c first (C2DString's colour: z, y, x, then w);
  - a local array initialiser copied in with memcpy reproduces MW's lq/sq copy from anonymous
    data (C2DLine's UVs);
  - a bitfield read into a bool local adds an `sltu`, into an unsigned char local `andi 0xff`;
  - when a byte is tested twice, MW reuses the load only if the first test is also an inline
    accessor (CPlainBmp::Draw's `IsString()`).
- **Tells from the zdb world, round 3** (texture characters, GarbageCollect, AddChild, the DI
  reads):
  - a local initialised before a call and returned on the path that skips the rest takes a
    saved register; declared after that call it stays in v1 (CDI::Read);
  - a reference returned by an inline accessor (GetChildren()) stays in a saved register;
    writing the member directly rematerialises its address from `this`;
  - `V tmp(v); v.swap(tmp);` inside an inline template is inlined one level only, like
    FreeVector: the copy constructor, swap and destructor become calls (GarbageCollect);
  - to keep a load ahead of a call both arguments must be locals (`n = a[i]; lib = Find();
    f(lib, n);`); with only `n` a local MW sinks the load past the call;
  - pushing onto a pointer list through an inline that takes the pointer by value puts the
    target's temporary at the bottom of the frame; a named local copy lands among the named
    slots;
  - `if (A) { if (L) ... else ...; return; } loop` keeps the inner join block; nesting the loop
    as the else lets MW thread the jump;
  - float min/max helpers returning references select addresses; returning by value they select
    values with mov.s, as the target does;
  - MW 3.0 initialises a function-local static with a constant initialiser statically, with no
    guard (SOCOM 1's build had one);
  - inside an inlined bool function, `if (c) return true; return false;` materialises with
    daddiu/move.
- **Tells from zsys and the valves** (round 3 of the sound lane):
  - MW accepts a 128-bit literal (32 hex digits); a 64-bit constant fills only the low half
    (`pcpyld v1,zero,v0`);
  - an `asm volatile { sync }` block is a barrier (a constant loaded before it is loaded again
    after it); GCC-style `asm("sync")` lets MW keep it in a register;
  - MW moves an asm block's last instruction into a following call's delay slot; a trailing
    `nop` in the block keeps the slot empty;
  - `(T *)0x70000000 + i*n` adds the base through `$at` each time; as integer arithmetic MW
    keeps the base in a register;
  - `bool b = true;` when 1 is already in a register comes out as `andi rX,rY,0xff`;
  - an assert that becomes `beqz; li 1` in the delay slot was a ternary, `x ? p != 0 : true`;
  - SOCOM 1's two-begin()s-and-one-end() loop is `std::transform` of a container onto itself;
  - a function calling a small same-region function out of line, which -inline auto would
    inline within one unit, is in another unit (CHeap).
- **Tells from the game-state pool and the graph** (round 8: game_state.cpp, graph_tree.cpp):
  - a user-declared empty destructor keeps `delete p`'s null test under an enclosing `if (p)`;
    a trivial one lets MW fold it away;
  - a bool parameter stored into a 1-bit field compiles to `(b & 1) << n`, even for a constant
    argument;
  - `T *e = p;` with `&e` taken reloads every access through `e` from the stack, while `p`
    stays in a saved register;
  - an `&&` of two inline bool calls is materialised only when the `&&` is itself an inline
    bool function;
  - `return i == end ? 0 : *i;` drops the trailing nop `if (i == end) return 0; return *i;`
    leaves;
  - SOCOM 1 calls CGEdge's constructor and CGEdge::Delete out of line, as SOCOM II does: both
    need `#pragma auto_inline off`.
- **Out-of-class template members: never into a unit's own functions** (the entity lane's
  measurement, round 6): an MSL template member defined after its class and not declared
  `inline` is never inlined into the unit's own functions; it is inlined into tail code --
  implicit destructors, out-of-line template copies, static initialisers -- once one of the
  unit's own functions has instantiated it. Declared `inline` it behaves as an in-class member.
  So `__sinit_node_assetlib.cpp` inlines the list deleter's constructor (CAssetList's constructor
  instantiated it) where `__sinit_ent_main.cpp` calls it; and a range erase instantiated by the
  unit's own code (a clear, an erase, remove/remove_if) makes the tail's implicit list
  destructors inline ~list one level and call ~__list_imp (~CEntityList, ~CAssetList: proven by
  removing RemoveAll/PurgeEntities/DoRecycleEntity, after which ~CEntityList inlines the whole
  chain). `#pragma inline_depth`/`always_inline` act only through their state at the unit's end.
  ~C2D and ~CHUD's flare list inline four destructor levels plus the deleter's iterator
  temporaries -- five high: the level is 5. A range erase the unit instantiated counts in tail
  code, so the implicit list destructors there (~CEntityList, ~CAssetList, ~CAiEventList) are cut
  below ~list -- the erase frees through the node allocator, one level taller (include/list).
- **Tells from the HUD, round 8** (the MakePackets, the weapon selectors, the line map):
  - a cursor of a different pointer type than the packet (`sceVu0FVECTOR *` from the
    `u_long128 *`) keeps MW from substituting `p = packet` into later uses;
  - one `p++` per quadword: MW folds one step into the next access and materialises every
    second (steps of 32); a `p += 2` block folds whole into constant offsets;
  - `SCE_GIF_SET_TAG` with a variable EOP ORs `(eop<<15 | nloop | pre<<46)`, then `prim<<47`,
    then `nreg<<60`; with only the PRIM variable the constants fold into one word;
  - a brace-initialised PNT2D local is zeroed bytewise first: assign the fields instead;
  - MW devirtualises calls on a global object, but calls on a member of a global array element
    go through the vtable as written;
  - `if (!(a && b && c && d)) return false; return true;` lays the false block before the true
    one; `if (a && ...) return true; return false;` puts true first;
  - an inline accessor on a product's last factor makes it the right operand of the final
    multiply; `n > 1` branches on `sltiu at` with the delay slot filled where `n >= 2`
    materialises the test; `delete p` under its own `if (p)` keeps two null tests.
- **Tells from the menu's UI commands** (zanim_menu.cpp):
  - **name rows by a registration table when both builds have one**: SOCOM II's UICommandTable
    (0x3DD4D0, 208 records of 16 bytes {id, name, handler, parser}) and SOCOM 1's (149 of 12)
    pair the handlers by command name -- and caught an `s1:exact` mispairing (masked relocations
    let tiny "load a global, call with 0, return 1" wrappers pair wrongly: 0x27A250 is
    UIGetClanRanking, not GameStatePopCommand);
  - include/vector's pointer back() is a level too tall for FlushHistory/~CMenuStack/
    FlushMenuHistory: `push_back(v.back())` in an inline that is itself inlined fetches the
    element out of line; only `*(base end() - 1)` gives the target's inlining;
  - an inline method's `this` temporary keeps a member's address hoisted in a saved register
    across a loop (`m_history.Flush()`); the same loop written out recomputes it;
  - a loop test written as an inline accessor re-reads the member in the body; written out, the
    test's load is reused;
  - a table scan whose first record is never tested is `do { } while (table[++i].id)`;
  - `if (i >= 16) return 0; return a[i];` gives `sltiu v0`/`bnez`, the ternary `sltiu at`/`beqz`;
  - `name = !strcmp(name, X) ? f() : name;` puts the call's block after a branch where the `if`
    form falls through;
  - a load and a call read into two locals in that order keeps the load before the call, in a
    saved register (ForceMPLaunch);
  - a callback that returns true (`li v0,1`) frees v0 for a function-pointer call inside it;
  - scdiff leaves a function-local static's object unchecked: hold it by its placeholder.
- **Each overlay links its own template copies** (the AI lane): OVL2 is linked apart from OVL1, so
  a template instance or an inline's out-of-line copy exists once per overlay (the pointer list's
  erase: OVL1 0x1EAAF0, OVL2 0x50F990). The map keeps one name per function; the other overlay's
  copy stays a placeholder whose note says `ovl-copy: <name>`, and scdiff resolves a call from
  that overlay to it (`SymbolTable.address_in`). sccheck scores an instance a unit emits against
  the copy in the unit's own overlay (the overlay of the unit's own functions) and counts matches
  by address, so one name can match once per overlay: fts_remote.cpp's `_saturate<int>` is OVL1's
  0x2BD220 while the name stays on OVL2's 0x519290, and five OVL2 copies hand units already
  emitted (0x50F990, 0x50FAD0, 0x51BED0, 0x539B70, 0x53AAC0) matched the day this went in.
  automatch's superseded test follows the same rule. To diff one copy by hand: `./scdiff.py SRC
  NAME --target 0xADDR`. `tools/ovlcopies.py` finds unmarked copies (a placeholder's words equal to
  a named row's in another overlay, the link's address fields masked, every call to the same callee
  or its overlay's copy, one twin per overlay) and `--apply ADDRS` notes the ones vetted. THE WORDS
  DO NOT PROVE THE INSTANCE: template instances for different types share one body, and look-alike
  accessors coincide (CAiMaps::GetMapByIndex has CEntityList::GetEntityByIndex's 0x70 bytes; the
  CAiEventCallback list's destructor has ~CWepCallbackList's) -- of a first 32 marks, 24 were taken
  back to plain "the same words as" hints. Mark an instance that exists once in the program
  (basic_string<char>'s members, the pointer lists' list_deleter<const void *> base) or a class's
  own inline members found together (CAiMap's GetCell family in OVL1's map code, 0x20F480-0x2106A0).
- **Tells from the AI core** (ai_events.cpp, ai_astar.cpp, zchar/):
  - testing a field through an inline bool accessor makes MW load it again for the next
    statement; written out, the first load is reused;
  - a bitfield read from a halfword is promoted to int and compares signed, one read from the full
    word compares unsigned -- the same source gives both (CAiPlanner::dist);
  - an inline helper that returns through a local (`r = 0; if (c) r = ...; return r;`) gives the
    target's registers and add order; early returns do not;
  - `deleted = true;` before `delete t` keeps the flag in a saved register across the call; MW
    3.0 drops the delete's null test once the pointer has been dereferenced;
  - `va_arg(ap, EVENT)` reads a byte enum with `lb`; `(EVENT)va_arg(ap, int)` does not;
  - SOCOM II's CAiPlanner::reset is empty where SOCOM 1's cleared a byte: a name carried over from
    the vtable says nothing about the body.
- **Tells from the online lobby** (src/gamez/zonline/):
  - the lobby's units are fixed by the static initialisers in link order and by string copies --
    the same text at two addresses means two units ("Retrieving messages from server..." in the
    clan unit and the lobby unit);
  - two of three `==` tests on one variable joined by `||` fold into an unsigned range test plus
    one `==`; a `switch` gives a compare chain instead;
  - `x > -1` branches on `slti` as the target does; `x >= 0` gives `bltz`;
  - a local zeroed before an early-return test puts the zero in that test's delay slot;
  - an inline bool returning a negation, used as the whole test, is materialised (`sltu`,
    `xori`) before the branch;
  - the source order of a request's field assignments decides how its stores are scheduled;
  - an s1 "exact" pair with relocations masked cannot tell which global it passes: check the
    address (UIGameStatePopCommand passed theLobby -- it is GetClanRanking's handler);
  - the lobby's `uiv_*` members are read through `theLobby.` even inside member functions.
- **Tells from the SEALs** (src/gamez/zseal/):
  - a `neg.s` of a loaded constant needs a local (`float g = -98.0f; ... -g`); a literal
    `-(-98.0f)` or a `const` local folds;
  - MW 3.0 lays out SOCOM 1's guarded function-local statics as plain initialised data, 8 bytes
    apart, and scdiff compares them by content (the static is 0.707107f, not sqrt(1/2));
  - `ai::_max` is `a >= b ? a : b` (`a < b ? b : a` inverts the branch), and MW auto-inlines no
    template function, so `_max`/`_min` calls are jals in both builds;
  - a member load the target places before a branch needs a local taken first;
  - CSealCtrlCopy's OVL1 table shows which CSealCtrl virtuals are inline: any OVL1 address in a
    slot it does not override;
  - a vtable slot's name carried over from SOCOM 1 can land on a subclass's override when SOCOM
    1's base function was an empty stub of the same 8 bytes: check each class's own table
    (OnMissionStart/End are CRemoteCtrl's at 0x5E6270/80, CSealCtrl's at 0x597F50/0x597EB0);
  - a `jr t9` tail call can run on into the next function, and a nop before an epilogue can end
    a row early: the seed split three of the lane's functions in two.
- **Tells from the grid** (round 4 of the zdb world: grid_main.cpp, grid_clutter.cpp):
  - SOCOM II's CGrid threads its spare atoms through their Next pointers (a free list): SOCOM 1's
    m_AtomFreeCnt/m_AtomFreePtr are gone, every later member sits 8 bytes earlier, 0x6514 bytes;
  - MW expands `abs` inline when it comes through <stdlib.h>: a target calling MSL's abs
    (0x191428) needs it reached by address;
  - an unsigned compare written `b <= a` branches on `sltu at`; `a >= b` puts the test in v0;
  - a void function whose paths all end on `++member` allocates its temporaries from v1;
    returning the incremented value gives the target's v0;
  - `x = MAX(0, x); x = MIN(n - 1, x);` gives gridGetAtomBasePtr's clamp; written as ifs it
    becomes bgez;
  - reusing `len = 1.0f / len` instead of a new `inv` local swaps the FPRs to the target's order.
- **Tells from the HUD, round 9** (hud_weaponsel.h, the pause test, the clip helpers):
  - MW 3.0 deletes an empty element-destroy loop, so include/vector's one reverse loop
    (`while (first < last) (--last)->~T();`) serves trivially destructible and
    virtual-destructor elements alike (C2DPoly's deleter: clear and destructor);
  - a class vector's begin() through a `T *const` local keeps a walk's first test a compare
    (C2DTriFan::Draw), as the pointer vectors' does;
  - an implicit destructor tells an array (one `__destroy_arr` call) from a run of members
    (destroyed one by one): CWeaponSel's three selection backings are members;
  - a class member at offset 0 is destroyed with -1, a base class with 0;
  - `x = (signed char)(x + 1)` re-extends before adding; `x++` does not;
  - as the last test of a negated `&&`, `!(b <= a)` branches on `at` where `a < b` materialises
    into v0;
  - `ok = f(); return ok;` with int types keeps the target's layout of the last test; `return
    f();` inverts it, and a bool local adds an `andi`;
  - declaration order assigns float registers too: `float t;` declared first gave the target's
    `$f20`/`$f21`.
- **Tells from CGame and the deque** (round 9 of the states lane: include/deque, game_main.cpp):
  - include/deque: Metrowerks::cdeque's capacity/size/start/data blocks at 0, start_ 0x10, size_
    0x14, a constant block size of 128, the allocator an empty base after the 0x18 bytes;
    pointer deques run on deque<const void *> through a partial specialization (which needs a
    more specialized `X<const void *, A>`, or T = const void recurses into an incomplete base);
    SOCOM II's state stack is still SOCOM 1's m_Stack[16] -- the deque is the command queue;
  - MW puts an empty base that follows a non-empty base at the object's end, with zero size (the
    next member overlaps it); an empty base alone or listed first sits at offset 0;
  - compiling a constructor, MW emits out-of-line copies of every implicit destructor level of
    its members and bases, even with exceptions off and an empty body;
  - in the template tail an out-of-class member is inlined only if it was generated earlier in
    the tail, and an out-of-class template destructor's tail copy inlines at most four nested
    destructor levels;
  - an uninitialised local declared ahead of an initialised one takes the higher saved
    register (PopTo, Tick);
  - `for (;;) { if (a == b) break; ... }` keeps its test at the top and jumps back with a `b`;
    `while (a != b)` gets rotated;
  - std::allocator's constructor is defined after its class (`throw()`): the deque's
    constructors call it out of line (0x2CEE30).
- **include/string and the UI commands, round 2**: SOCOM II's MSL 3.0 basic_string read from the
  image -- 12 bytes, a long form {capacity|1, size, text} and a short form with the size in the
  first byte's upper 7 bits and the text in place (SOCOM 1's DWARF describes the older
  reference-counted string, so it could not supply the layout); the default constructor zeroes the
  three words through a register (`addiu v1,sp,N; sw zero,0(v1)` x3); the destructor is out of
  line (0x1E7EF0); assign/append/erase go by placeholders until MSL 3.0's names are known.
  - a CSaferStr local's destructor passes 0 (a base subobject), a plain std::string local -1;
  - a value returned through an inline function gets its saved register after the named locals;
    writing the out-of-line call directly gives declaration order;
  - a ternary between a byte enum's two enumerators passed to an enum parameter compiles to
    `li 1; movz`, as an int to `sltu`;
  - a member pointer handed on from a const reference parameter must be copied into a local for
    the target's 12-byte copy, or MW passes the reference straight through as a tail call;
  - reading a loop-invariant record field into a local before the loop frees the record's saved
    register for the iterator;
  - member-pointer constants are pooled per unit as `{0, -1, f}`.
- **Tells from the SEALs, round 2** (seal_body.cpp, seal_cfg.cpp, seal_env.cpp):
  - an inline's out-of-line copies after their first caller sit in exactly reverse first-use
    order (PostTick is followed by eight, CDynGrenade's three statics among them);
  - a unit that defines one of a class's virtuals also emits its implicit destructor, which then
    supersedes a matching draft; declare the destructor non-inline where the target calls it
    (CAiSAnimate);
  - `T *a; U *b = x; a = f();` swaps the two locals' saved registers compared with initialising
    `a` at its declaration;
  - a scaling written as its own statement (`d *= k; return d;`) is the only form that madds a
    blend into $f0 and loads the factor into $f1 after the mula (ComputeElevOfs);
  - CZSealBody is `: CEntity, CBody` -- CBody at 0x170, 0x90 bytes (its constructor at 0x28EDD0
    zeroes through 0x8C); every CAiState holds a CAiStateStack (`: list<CAiState*>`) at +8.
- **Tells from the kit and the AI states** (seal_aistate.cpp, seal_airesponse.cpp,
  seal_aifollow.cpp, remote_control.cpp, seal_squirm.cpp, seal_kit.cpp, seal_unit.cpp):
  - SOCOM II's CAiState table has 18 slots (SOCOM 1's ten, eight added, the destructor last at
    +0x4C), read off every SOCOM 1 override; ai/ai_states.h is the one declaration (zseal.h
    includes it), the base's answers inline: a unit that emits a state's table emits the copies
    its slots need;
  - a member destructor the target calls from every unit (not inlined a level) is an ordinary
    function defined in one unit (CAiStateStack's, seal_aistate.cpp);
  - a class's inline-virtual stub copies come with its table, so a unit's tail of stubs needs one
    of the class's virtuals defined non-inline in that unit;
  - `bool ok = false; switch ... ok = f(); break;` gives a `jal` then `andi 0xff` per case, with
    the zero in the delay slot (CRemoteCtrl::AllowCommand);
  - a struct copied with memcpy and read through a `bool` member returns without `sltu`
    (GetJumpFlag);
  - the GCC libraries start at OVL2 0x60FB50 (an empty `jal` delay slot, GCC's prologue order,
    8-aligned with no padding); the last Metrowerks function is 0x60FB30.
- **Tells from the AI core, round 2** (ai_util.cpp, ai_events.cpp, vehicle_rdr.cpp):
  - a function returning a value from each arm of nested float tests is merged into one exit;
    one result variable assigned in each arm keeps the target's separate branches
    (get_clock_direction);
  - `s += dT; if (s >= s)` gives the target's `c.lt.s $f0,$f0`: MW reuses the just-stored global
    for both operands;
  - an end iterator taken into a local before remove_if, the erase its own statement, keeps the
    list's sentinel in s0 (CAiEvent::Tick);
  - an explicit `template class std::__list_deleter<...>` makes its out-of-class constructor
    auto-inlinable; without it the initialiser calls it, as the target does;
  - an explicit instantiation of a function template (std::find) emits nothing; only a use does;
  - `sltu at,zero,count` before a branch is an index test against 0 (an `Item(0)`); a plain
    unsigned `count > 0` compiles to `bnez`;
  - an accessor taking an unsigned index stops MW strength-reducing a loop with an int counter
    (CVehicleRdr::Open);
  - labels after a `b` in hand-matched code: 487 padded to 8 bytes, 130 not, 117 delay slots
    filled from the target -- padding against filling depends on context, and blocks
    GetNextEvent, MakeNoise and MakeNoiseXZ.
- **Tells from the UI widgets and the file I/O** (zui_var.cpp, zui_tuners.cpp, src/gamez/zutil/):
  - a destructor that stores no table means the polymorphic object at offset 0 is a member, not
    a base (CMemCard holds its CMemCardIO);
  - `tries = ok ? 0 : tries - 1;` matches the memory card's retry loops; the if/else form leaves
    a nop before the loop branch;
  - `x > -10` branches on `slti at`, where `x >= -9` computes the test into v0 first;
  - `int end = pos + size;` written before an early return puts the add in the branch's delay
    slot (CBufferIO's freads);
  - `int s = b; a2 = a; b2 = s;` reproduces a load-load-store-store order (SelectPortSlot);
  - a materialised `&&` assigned inside an if/else comes from an inline bool helper
    (CFileIO::IsOpen);
  - `if (p) delete p;` gives the double null test around a virtual-destructor delete; a delete
    whose destructor has no name in the map can be written as the placeholder called with
    `(p, 1)`;
  - SOCOM II's CIO has three async slots after fwrite (0x30-0x38): table positions from SOCOM 1
    shift by three past them (0x39FEC0 was misnamed fwrite that way);
  - libmc's directory table: 64-byte aligned matches the out-of-line Exists and Open, but where
    Exists is inlined the target lays it out 16-aligned (GetUsedSpace and the reserve files, open).
- **Tells from the weapons** (src/gamez/zweapon/):
  - an inlined bool callee's `return true` becomes `daddiu`; IsM203Weapon and IsPrimary match only
    by calling a separate `if (...) return true; return false;` test, which out of line uses `li`;
  - in a pointer-list walk with end() in a local, the declaration order decides the registers
    (`it = begin(), e = end()` against `e` first), and an inlined copy can need the other order
    from its out-of-line copy (GetWeapon against GetWeaponName);
  - an iterator that is assigned (so it lives in memory) with the end in a register is
    `it = begin(); for (iterator e = end(); it != e; ++it)` (GetAmmo);
  - `m = c != 0.0f ? x / c : 0.0f;` puts the result in $f0 on both arms; `float s = 0; if (c) s =
    ...;` puts it in $f1 (the accuracy setters);
  - a reference local to a struct element (`T &cb = *it`) gives the target's `addiu a3,s2,8` in a
    walk over a list of structs (MakeWeaponEventCallbacks);
  - Recoil is CZFTSWeapon's key function: its table and implicit destructor come out in
    zwep_ftsweapon.cpp, first in the image; SOCOM 1's link order here is ztex_manager, zvid_init,
    vis_main, vis_decal, vis_texfx, zwep_explosive, zwep_global, zwep_projectile, zwep_weapon,
    zwep_ammo, and SOCOM II's CTurret sits before zvid_init.
- **Tells from the body animation** (src/gamez/zbody/: bone_math.cpp, body.cpp, si_object.cpp):
  - an explicit `(float)` cast on a float product flips MW's multiply operand order: `a->m_wt =
    (float)(scale * a->m_wt)` matches where no spelling without the cast does -- which is why
    m2c's `(f32)` drafts match;
  - a member read once into a local stays in a saved register across calls (CZBodyPart::Orient's
    node); RTList::Add's tail local, declared first, takes the higher argument register;
  - a single-exit if/else chain into a local gives the target's shared `jr ra` (LinearWeight);
    early returns add nops; `if (a >= b) w = 1; else w = a / b;` branches on `a < b` to the
    division;
  - `new` with a constructor whose body calls a too-tall inline calls the member constructors out
    of line in member order, then the inline (CZBodyPart: CPnt3D() is 0x1FB3C0, CQuat()
    0x28EB60);
  - a loop-scoped CPnt3D declared first at function scope takes the highest frame slot
    (PartInSpace);
  - CBody (0x90 bytes; zseal.h includes zbody.h): its table, `short m_state` at 4, the active-anim
    and blend pools (RTArray, 0x20 bytes each) at 0x10/0x30, the blend stack at 0x50, the
    part and anim vectors at 0x5C/0x68, the overlay at 0x74; zanim_menu.cpp runs to 0x2869C4
    (cdeque<const void *>'s instances close it), the body code from 0x2869D0 to 0x29035C.
- **Tells from the rdr reader** (src/gamez/zrdr/: zrdr.h, zrdr_main.cpp, zrdr_archive.cpp,
  zrdr_file.cpp, zrdr_pack.cpp; src/gamez/zsave/):
  - an inline const member called through a non-const pointer reloads a field the caller just
    tested; through a const pointer, or written out, it reuses the register;
  - MW folds a null test of a local array's address, even through an inline's parameter; written
    as `(int)p` the test stays (both builds' FindRdr keep it);
  - the target's bool-materialised tag test is `bool m = type == 3 ? strcmp(...) == 0 : false;`
    written in the caller; as an inline bool member it reloads the type;
  - `Item(unsigned)` with an int counter keeps the element address recomputed each pass, as the
    target does; matching types strength-reduce it;
  - a pointer-list walk whose element the target reloads after a call reads it through `*i.i_`,
    cast out by value; `i = begin();` written before `iterator e = end();` loads the first node
    ahead of the sentinel's address;
  - a placeholder constructor or destructor wrapped in an inline gains MW's null test: call the
    placeholder by hand where the member's would run;
  - a 16-bit bitfield in `type == 4 ? length : 0` compares signed; through an unsigned-returning
    accessor, unsigned;
  - _zrdr is `type:8 isclone:1 packed:1 nofree:1 unused:5 length:16` and a word union; zanim.h
    (and zui.h) still have their own `struct _zrdr` (Item(int), non-const accessors, no
    constructor). Measured: swapping zanim.h's for zrdr.h's breaks 4 units' builds and 14
    matched functions (ten menu handlers, CCamera::CmdParseParams, ParseTeamBlock,
    LoadSoundGroups, ~CRdrArchiveList's copy) -- the two families of code want different
    accessors (const or not, the index unsigned or int) and zrdr.h's constructor. Overloads cannot
    serve both (each side calls with int counters through non-const pointers); a unit that needs
    both headers waits on renamed accessor families.
- **Tells from the HUD, round 10** (hud_objectives.cpp complete, hud_ammo.cpp, hud_mpinfobox.cpp,
  hud_healthbar.cpp):
  - an inline bool written `if (i < 0 || i >= n) return false; ...; return true;` puts the false
    block first, as the target does (ResolveIndex, after SOCOM 1's line table); the `&&` form
    puts it after;
  - scdiff leaves a function-local static float unchecked but compares a file-scope static by
    content: prefer file statics;
  - a local that comes from an inline helper takes a lower saved register than the caller's
    named locals and earlier inlined loop temporaries (NextObjective's greying moved into an
    inline for `newindex` to land in s1);
  - MSL's `vector<bool>`: its destructor one null-tested level (`if (data_) operator
    delete(data_)`); an element read goes through SOCOM 1's `reference {vec, n}` to the
    out-of-line `test_bit`, the reference kept on the stack inside a loop, folded outside one;
  - a same-unit function the target calls out of line is pinned with `#pragma auto_inline off`
    (SetCurColor);
  - `if (own) tex = x;` before a call pads the join with a nop; a ternary in the call's argument
    does not;
  - include/string lacks `basic_string(const char *)` (its out-of-line copy is 0x2396F0):
    CRadioIcon_Dynamic::Tick waits on it.
- **Tells from the AI core, round 3** (char_type.cpp, ai_maps.cpp, the new ai_maptable.cpp):
  - string literals a loop passes on every pass, hoisted into saved registers at the function's
    start, were string locals declared at the top (match_name_to_chartype);
  - where the target loads the right operand of `a * b` first, write `a * (int)b` (AllocEx);
  - a bitfield ternary `invalid == 0 ? (TYPE)type : NONE` with a byte enum puts the valid arm
    first without materialising the bit; `!invalid` is normalised to the other order;
  - a base virtual called qualified and written in terms of the reference parameter shares its
    loads with the caller; through a member inline on the pointee it does not;
  - a jump-table switch of string returns regenerates mechanically: the case blocks in address
    order are the source's case order, the enum values come from the table
    (GetCharacterSoundTypeString's 85);
  - `!(s > NONE)` on a signed byte enum gives `slti`/`bnez` (GetSound);
  - open on include/vector, five functions in two lanes: the pointer vector's end() is one level
    too tall where GetByNameMod is inlined a level deeper (CCharacterType::Create,
    RdrFetch_CharacterType), and the target builds end() once and copies it for the erase after
    remove_if (CleanupTemporary, CWorld::DeleteLightMap, DeleteChild).
- **Tells from the SEALs, round 3** (pickup_main.cpp, seal.cpp's stats, seal_ai.cpp):
  - `x == 1 || x == 2` on consecutive values folds into one range test; writing the second as an
    inline returning `x == 2 ? true : false` keeps the two compares (the target folds three tests
    on one byte where MW folds two: 0x562960, 0x5629B0 open);
  - a loop written `i <= 4` branches on `slti at`; `i < 5` puts the test in v0;
  - `iterator i = begin(), e = end()` gives `e` the lower saved register, but for argument
    registers `e` must be declared first to get the lower one; declaring a walk's iterators
    before an unrelated float initialisation puts the begin load ahead of the float literal;
  - `s->x++; return s->x;` re-reads the counter, `return ++s->x;` does not; an int added to an
    unsigned short field gives the target's `andi 0xffff` only when cast `(unsigned short)n`;
  - CPickup is SOCOM 1's pickup_main.cpp (0x541650-0x542FD7: 0x542E50 is CPickup::Close), and
    seal.cpp starts at 0x542FE0; 0x540910-0x5414EF are string and vector instances closing the
    unit before it; zseal.h includes zchar.h for CHARACTER_SOUND.
- **Tells from the kit and the AI states, round 2** (seal_kit.cpp 56 of 57, the states' signals,
  seal_aivehicle.cpp):
  - `x > 0.0f` compiles to `c.le.s x,0; bc1f`; writing `!(x <= 0)` adds an xori;
  - after an early return a `while` jumps straight to its bottom test; the target's top test
    needs `if (c) do ... while (c)`;
  - a signed byte enum local assigned from a call is narrowed only where it is compared; a
    `signed char` local is narrowed at the assignment;
  - a local copy of a parameter takes the lowest saved register;
  - an inlined copy that misses only by label pads wants the inline written another way: `if
    (w) return ...; return 0;` and the ternary compile alike out of line but pad differently once
    inlined (GetIcon in GetCurIcon);
  - a switch whose cases share one body ends its chain with a `b`; an if-chain inverts its last
    test; the states' OnSignal switches list case 4 first (their chains test 3, 2, 1, 0, 4);
  - `return i != -1 ? f(...) : 0` gives SOCOM 1's beq/move shape; the if-return form inverts it;
  - kit.h's EQUIP_ITEM/FIREMODE/CZWeaponList/CZWeapon/CZFTSWeapon are size-only stand-ins whose
    offsets agree with zweapon.h's (m_maxfiremode 0x24, m_ID 0x7c, the zoom vector's size 0x284):
    a unit including kit.h and zweapon.h together waits on folding them into zweapon.h.
- **Tells from the DNAS client and the mission, round 10** (fts_patch.cpp, fts_dnas.cpp):
  - the OVL1 DNAS client is two units: the patch downloader (CSMDownloadPatch, 0x2CA330-0x2CC66F,
    its HTTP client DNAS.BIN's) and the authentication (CSMDNASAuthenticate, 0x2CC670-0x2CD66F);
    a member-pointer pool that starts over with the base's constants (Finish, Idle) marks the
    boundary; the DNAS.BIN calls are address placeholders with the argument shapes in the source;
  - a helper taking `const int &error` keeps a struct field's address in a saved register and
    reloads it after a call; bound to a register local it costs nothing, to a literal 0 it adds
    a stack temporary -- the target used a zeroed local;
  - `if ((err = f() < 0))` is the original's precedence slip: the error passed is the comparison
    (`slt a3,v0,zero`);
  - as a case's last statement, `if (c) { f(); return; } break;` keeps the target's jump to the
    epilogue, `if (c) f(); break;` drops it;
  - calling the base's inline virtual explicitly makes the second copy of the member pointer with
    `ld`/`sd`; a value compared against a global loaded with `ld` and passed to an int parameter
    through `dsll32/dsra32` is a 64-bit `long`;
  - inline bool setters called with `false` produce one `(0 & 1)` shared by both 1-bit stores;
  - include/string: `operator[]` (0x2CA6E0) and size() casting the short form back to its byte
    (0x2CA700 masks it), measured over every hand-written unit; zanim.h's CValve has
    SetValue(int) (0x2CA740), which several units still spell as helpers of their own.
- **Tells from the rdr parser and editor, the particles, the pipe** (zrdr_parse.cpp,
  zrdr_edit.cpp, src/gamez/zparticle/, src/gamez/zrender/):
  - keep `strtol`'s result `long` until the store: the conversion (`dsll32`/`dsra32`) lands in the
    store's arm (MakeUnion);
  - early returns from a float function hoist a constant into the last test's delay slot; one
    result variable assigned in every arm keeps the target's layout (GetOpacity);
  - a CPnt4D temporary bound to a const-reference parameter is optimised away unless the copy
    constructor copies through the base; a named local placed after the call then gives the
    target's stack slots (ColorSequenceAdd) -- SOCOM II's CPnt4D has a written copy constructor
    through PNT4D's (0x328D60), which zmath.h lacks (zparticle.h carries its own CPnt4D);
  - a string index written through an inline taking the text pointer into a local gives the
    target's `ptr + i` order; `c_str()[i]` gives `i + ptr`;
  - an inline's own local (erase()'s category byte) takes its stack slot in order of appearance,
    so a hand-written `erase(pos, n)` must be an inline too;
  - a function a unit defines first can sit after the out-of-line copies that follow its
    last-emitted function (zrdr_newarray);
  - a 64-bit GIF-tag constant written as one literal matches li/dsll32/li/or (FIFO_Signal);
  - `new T` as an inline call's argument is checked a level deeper, as its own statement it
    inlines whole (CRdrEditor::push) -- an inline height, to recheck at the corrected level.
- **Tells from the zdb render classes** (zvid_init.cpp, vis_mesh.cpp, vis_main.cpp, vis_decal.cpp):
  - a pointer the target keeps in v0 throughout and stores at the end is the return value (the
    mangled name does not carry it): declared void, GetTextureSelectBlock_ver2 spills a saved
    register;
  - `unsigned char *flag = &p->u8[2]` gives the target's address computed once and stored
    through; a cast on the base does not;
  - a loop count loaded from a halfword but declared `unsigned int` swaps the count's and the
    counter's saved registers into the target's order;
  - a single-case `switch` gives `beq; nop; b`;
  - a GS packet written through a cursor apart from the packet's start, one `p++` per quadword,
    gives SOCOM II's layout (SetGSReg, zVid_Scissor);
  - MW 3.0 inlines float-to-unsigned conversion where SOCOM 1 called `fptoui`;
  - zbody.h's RTArray serves the decal arrays too (the same 0x20 bytes).
- **Tells from the AI maps** (ai_maptable.cpp, ai_mapio.cpp):
  - a field compared with a parameter goes first in the `bne` only when read into a local first
    (`aid = x.m_actionId; if (aid == id)`); both spellings of `x.f == id` put the parameter first;
  - `i = m_index + (int)(x - m_start)` loads m_index before the subtraction; without the cast the
    subtraction is scheduled first;
  - a walk over `this->m_table` keeps `this` in a saved register only in a non-const member;
    through a cast `this` or a reference local MW keeps the table's address (GetLocation is
    non-const in SOCOM II);
  - each class-vector deleter destructor in ai_map.cpp's tail calls its own clear out of line,
    unlike include/vector's in-place destroy; SOCOM II's CAiMap::Read is 0x51AFE0, handing map
    versions 19-23 to 0x51AD90, 0x51AB50, 0x51A910, 0x51A6D0, 0x51A490.
- **Tells from the file I/O, round 2** (util_stable.cpp, util_sonyio.cpp, util_main.cpp):
  - a const member walking a pointer vector through a cast `this` hoists `&size_`/`&data_` into
    saved registers; the non-const member does not (CSTable::FindString is non-const in SOCOM II);
  - an empty loop stepping a class iterator with postfix `i++` puts its entry test at the end of
    the function (Relocate);
  - MW puts a constant temporary bound to a const-reference parameter in static data: `resize(n,
    0)` uses SOCOM 1's `@533`, SOCOM II's 0x3E0658;
  - `p = buf = malloc()` keeps v0; `buf = malloc(); p = buf;` reloads from the spill slot (Pack);
  - a literal passed on every loop pass inside an inline helper stays in a saved register only as
    a local of that helper (`const char *delim`);
  - `*s != 0` as a loop test lets the body's char compare reuse the loaded byte; a bare `*s`
    re-extends it (CFileIO::Open); `(bool)c && ...` materialises the char test inside a loop's
    `&&` (UTF8Strncpy);
  - `s = f(); if (!s) return s;` avoids the `move v0,zero` of `return 0`, and `s = p;` falling
    into a shared `return s` avoids a branch to the next instruction (CreateString);
  - newlib's strtok tail-jumps into strtok_r right after it: the seed had them as one 0xE8 row
    (split at 0x19AA20) -- a `j` to the next instruction is a tail call too.
- **Tells from the SEALs, round 4** (seal_create.cpp, seal_command.cpp, seal_ai.cpp's helpers):
  - Rfloat's random product through a local (`float r = K*rand(); return min + range*r;`) gives
    the join pad and the inlined copies' registers;
  - a by-value inline parameter with a `const float &` bound to it is stored on the stack and read
    back after the call, as the target does;
  - MW puts a derived class's virtuals that its primary base lacks after all its tables and calls
    them through the primary pointer (CZSealBody's +0x88/+0x8C/+0x90): tables can run past
    data.txt's sizes; CBody's has four slots (GetRootPart pure, BodySetState);
  - `switch (x) { case K: ...; break; default: ...; }` puts the case block first behind `beq`
    with a `b` to the default; an if/else falls through instead;
  - testing a field through an inline accessor makes MW load it again for an inlined callee's
    test of the same field;
  - `(unsigned int)(t - 1) < 3` written out gives the target's one range test (three `==` joined
    by `||` fold only two);
  - in a string switch, cases sharing the default's string are `case K: break;` before the
    common return, which keeps their compare or table slot;
  - `if (n == 0) s = 0; else s = f();` puts the zero arm first; a ternary or an if-assign puts the
    call first; `int r = 0; if (c) r = f(); return r;` sets the zero first, so the other path
    branches straight to the epilogue;
  - converting a pointer to a base at a non-zero offset adds a null test; passing `*this` to a
    reference parameter avoids it; a pointer taken into a local before a call keeps its load
    ahead of the call, in a saved register;
  - open on zmath.h: SOCOM II's CPnt3D operators are const members (`__pl__6CPnt3DCFRC6CPnt3D`
    0x557DC0, `__ml__6CPnt3DCFf` 0x557E10, the minus 0x557E90) where zmath.h declares free
    functions, and CPnt3D::Cross (0x592400) is undeclared.
- **Tells from the online lobby, round 2** (the lobby's lists, friends, clan requests):
  - MW 3.0 copies a class with a user-declared constructor member by member (arrays in loops,
    nested plain structs as words); a plain struct is copied as one block -- the lobby's
    whole-response copies need that constructor;
  - a non-inline function template's instances follow their first caller in reverse order of
    first use, each with its whole body inlined (ReleaseVector<T>);
  - an inline switch returning constants compiles each case to `li` returning `int`, to `daddiu`
    returning `unsigned char`;
  - a type tested twice with a zero in both delay slots is `if (t == 3) return t == 3 ? s : 0;
    return 0;`;
  - a local declared earlier takes the higher stack slot: the request is declared before the
    1 KB text buffer;
  - after 0x306670 come zMath, then a movie/IOP stretch (0x309550-0x30CE80: audioDec*,
    loadImage, CIOPManager); CZNetwork starts around 0x30CE80.
- **Tells from the body animation, round 2** (body.cpp):
  - a field saved around a VU0 write wants `*(float *)&p->m_node`; spelled
    `((float *)&p->m_pos)[3]` it keeps the address in a register;
  - a bool bitfield accessor tested with `!= false` gives the target's `sltu`; tested plainly it
    folds (SmoothAnims);
  - a vector walked through a saved `&member` register wants a reference local (`T &parts =
    m_body_parts;`); the plain member walk reads `this` each pass (OrientParts against
    OrientAuxParts); a non-const member keeps `this` in a saved register, the const one with a
    cast keeps the member's address (FindPart is non-const in SOCOM II);
  - fields loaded into float locals before a call keep the element pointer out of a saved
    register (Propagate); a pointer local to a stack vector (`CPnt3D *p = (CPnt3D *)&v`) keeps
    the address in a saved register for the calls that take it (GetVelocity);
  - two-case dispatch written `if / else if / else return` gives the target's test-body-test-body
    layout, a switch puts the tests first; two arms each ending in a call, then one shared
    `return false`, gives the shared `move v0,zero` tail (Tick);
  - named locals for two pushed `new` pointers take frame slots in declaration order
    (AddCallback); CBody's vector at 0x68 is the auxiliary parts, not the anims.
- **Tells from the zdb render classes, round 3** (zvid_init.cpp, vis_main.cpp, zwep_explosive.cpp):
  - a bool or unsigned char parameter compared `== false` gets an `andi 0xff` before the branch;
    written `!x` it does not;
  - a field zeroed right after the vtable store, before the member objects, is a derived-class
    member in the initializer list; a base-class constructor's store lands before the vtable
    store (CVisual's reference count);
  - storing through an unsigned view of a signed byte gives `li 255` where `li -1` would be the
    signed store;
  - MW pools identical constant temporaries bound to `const T &`, default arguments included,
    into one static per unit;
  - `f(r->x, r->y, r->w, r->h)` with f inlined loads all four fields before the first store;
    written out, the stores pin the loads; `w = global; r = call(); w->g(r)` keeps the global's
    load ahead of the call, `global->g(call())` puts the call first;
  - a same-unit stand-in for an out-of-class template member needs `#pragma auto_inline off`;
  - CVisual's constructor is byte-exact at -inline level=5 and not at 4 (CDynTexList's
    constructor five inline levels deep) -- one more for the corrected level;
  - CTurret's unit (veh_turret.cpp) runs 0x3A71A0-~0x3ACA5F, opening with its inline virtual
    overrides' copies; CVehicle's 0x3ACA60-~0x3AFB2F; zvid_init.cpp from 0x3AFBA0.
- **Tells from the AI states, round 3** (the states' Ticks and signals):
  - every state's Tick returns a byte enum, ending `return (AI_RESULT)(m_done = m_active ?
    AI_RUNNING : AI_DONE);` (`li 1; movz; andi; sw`);
  - one inline does all the `dsll32`/`dsrl32` pairs on m_flags (`|= 0x80000000; &= ~clear; |=
    set`, called with constants); `&= ~0` stays as a bare zero extension;
  - Rfloat's random factor taken into its own local gives the target's float-register order
    (CSealCtrlSquirm::Initalize); odds passed by value and read through a `const float &` local
    stay in the frame across rand();
  - spelling an argument `(char *)ai + K` stops MW sharing that address with a later field load;
  - a member added to a header class hides a free function of the same name in another unit;
  - `bool b = !(x == 0) && y == 4;` gives the target's `xor/sltiu/xori`;
  - CAiState::Create (0x5F6550) inlines all 30 state constructors, their out-of-line copies after
    it in reverse first-use order; CAiStateStack is 0x20 bytes in SOCOM II (SOCOM 1's
    controller state timer folded in at +0x18); CAiMapLoc's default constructor is empty in
    SOCOM II (0x516120) -- zentity.h's stand-in, which reads `invalid`, is not it (open, with
    the entity header).
- **Tells from the UI widgets, round 3** (zui_spec.cpp, zui_cleanup.cpp, zui_var.cpp, util_crc.cpp):
  - bitfields cleared in the mem-initializer list are each read, masked and written back as a
    whole word; assigned in the body, MW narrows the stores to bytes and halfwords (_zrdr's clone
    constructor);
  - a class whose first base has no vtable gets a vtable pointer of its own where its virtuals
    are declared: CUIVarManager declares its members before its virtuals;
  - `new T(spec.GetVarSpec())` swaps the new object's saved register with the argument's,
    against reading `spec.m_var` directly;
  - member initialisers that call (`m_valVar(zrdr_findstring(..))`, a float through an inline)
    run before the string member's zeroing, the target's order;
  - a pin does not carry through: MW still auto-inlines a function that calls a pinned one (each
    caller needed its own `#pragma auto_inline off`);
  - `!(bool)x` materialises sltu/xori/andi into a bool local; `x == 0` keeps the raw value and
    tests it later;
  - zui.h's UIVAR_LONGEVITY has SOCOM 1's DWARF names and values (UIVL_DORMANT 0, TEMPORARY 1,
    PERMANENT 2, PERSISTENT 3); zanim_menu.h still carries the old spellings (TEMPORARY 0,
    SESSION 1, PERMANENT 2) -- mapped by value where a unit moved over;
  - util_crc.cpp's CRC32String (0x3A2370) is the CRC-32 the sound system hashes cue names with.
- **Tells from the network, round 3** (zNetMediusLobby.cpp, zNetMediusChat.cpp, src/gamez/znet/):
  - a void leaf never takes v0 for a temporary: a leaf that uses v0 and leaves it to the end is
    returning it (AdvanceMissionSequence returns the next mission);
  - an int-array member copies through the FPRs (lwc1/swc1); seven named int members copy as
    lw/sw pairs (TypeRTimeFixStruct);
  - `if (err) return err; flag = 1; return err;` gives SOCOM 1's `beqz; b` layout (zNetJoin);
    `if (!err) flag = 1;` inverts it;
  - a bool argument to an inline setter is re-extended (andi) at each use: CValve wants a
    SetValue(bool) overload for those callers;
  - an `||` materialised before its branch is a bool local (`bool b = x || y; if (b)`); two loops
    over one range want their own `for (int i...)` counters (a shared one swaps the second's
    registers);
  - zNetDme.cpp starts at 0x30CDF0 (its initialiser builds theNetwork at 0x45A0C0, registering
    ~CZNetwork 0x30E410), zNetVoice.cpp is 0x30E490-0x31097F (the radio and USB headset);
    znet.h can become the one CZNetwork declaration.
- **Tells from the SEALs, round 5** (seal_dyn.cpp, AnimSet's readers, Slide):
  - in three repeated list loops MW gives the later-declared of list, count and counter the lower
    saved register; the target's registers rotate loop to loop, so each loop's declaration order
    follows them;
  - a member copy-constructed in an init list from a CPnt3D is ld/sd plus lwc1/swc1; from a PNT3D
    cast to a CPnt3D reference, three lwc1/swc1 through pointers;
  - the product lands in the operand's own register (`mul.s $f1,$f1,$f0`) only inside an inline
    that multiplies its parameter in place (`v *= s; return v;`); plain `*=`, `x = x * s` and a
    `(float)` cast all give $f0;
  - `a = b = c = 0.0f` zeroes c first; separate initialisers zero the first variable first;
  - reading a vector's end through a reference to `data_` keeps `&data_` in a saved register
    across repeated push_backs (CharacterDynamics::Load, through a local helper);
  - open on include/vector: the class `__vector_imp::insert` returns void (0x59CF10 sets no v0;
    one definition would serve 13 class-vector inserts, ~18 KB), the class push_back through a
    deleter data() reference (SOCOM 1's line 923), and the deleter's destructor in a unit's tail
    calling clear out of line (seal_dyn.cpp, ai_map.cpp).
- **Tells from the turret and the vehicle** (veh_turret.cpp, veh_vehicle.cpp):
  - an address taken from a parameter (an entry + 0x20) and passed to an inline that loops over
    it is recomputed every pass; held in a local in the caller first, it stays in a saved
    register (CVehicle::Spawn);
  - `if (max > -1 && count >= max) return 0;` inside an inline gives the target's "branch to
    body, branch to body, jump to end"; `if (max < 0 || count < max) {...}` does not
    (CTurret::Create);
  - `x = c && m ? m : x;` on a bool parameter narrows at the join (`andi` in the delay slot);
    `if (c && m) x = m;` narrows inside the arm (SetThermal);
  - the flag setter `f &= ~(on ? 0 : mask); f |= on ? mask : 0;` reproduces the `movz`/`nor`
    pattern; with a constant `true` its first half becomes a `dsll32`/`dsrl32` zero-extension;
  - a network test with the player in a local loads this entity's controller first; written
    inline, that load is hoisted above the call (PrePostTick against Tick);
  - veh_turret.cpp is 0x3A71A0-0x3ACA5F (initialiser 0x404240; its template tail from 0x3AB5F0:
    CTurretDef's constructor, resize, erase, insert), veh_vehicle.cpp 0x3ACA60-0x3AFB2F
    (initialiser 0x4043A0), a near-copy with 136-byte definitions and 688-byte vehicles;
  - open on include/vector: a class-vector resize would unblock both definition-vector Inits,
    the two template tails and ~CTurretDefVector (0x3AC9E0).
- **Tells from the AI core, round 5** (ai_astar.cpp, ai_mapio.cpp, ai_path.cpp):
  - to keep a second load of a member MW would share, read it through a const view of `this`
    (plain, int-cast and volatile reads all share it: read_v19's m_cellcount);
  - `(unsigned)(s - 1) > 2` branches on `sltiu at`; the negated `< 3` inline or `>= 3`
    materialises the test (CAiPath::Clear);
  - `size() != 0 &&` as a while test branches on the size directly; a bare `size() &&`
    materialises a bool (TickPathPlanner);
  - a packed value read through a reference parameter is reloaded after every store to an
    address-taken local: copy it into a local first (_fixupNode);
  - CAiMapLocVec's implicit constructor adds an inline level: CAiPathStats with one is too tall
    for CAiPath's constructor at level 5, a plain `std::vector<CAiMapLoc>` member inlines whole;
  - CAiPath is 0xA8 bytes in SOCOM II, CAiStar 0x14 (its open and closed lists became the unit's
    statics: ai_astar.cpp's initialiser 0x668540 builds them and path_queue).
- **Tells from the body animation, round 3** (bone_archive.cpp, bone_init.cpp, si_script.cpp):
  - `bool ok = f(); ...; loaded = ok;` gives the target's second `andi 0xff` at the end of the
    arm (zBoneLoadMap);
  - an inline taking a coordinate by const reference loads it where it is used; taken by value,
    before the body runs (ClipToBox);
  - a quotient taken into a local flips the next multiply's operand order; a `(float)` cast does
    not;
  - narrowing to an `unsigned char` local before a switch gives the one hoisted `andi 0xff` ahead
    of the byte stores (SetCallbackIndex);
  - a `const char *const &` inline parameter bound to a global re-reads the global at each use;
    a local passed to `new T(args)` and declared before the `new` is loaded before the allocation;
  - `unsigned short a : 10` followed by `unsigned char b : 2` puts b at byte+1, bits 2-3 (lhu for
    a, lbu for b, as the target reads them);
  - libvu0-style GCC asm (`mfc1 $8, %n` with an "f" operand and a $8 clobber) reproduces
    mfc1/nop/qmtc2; the buffers held in pointer locals give register-based stores;
  - `new T[n]` calls MSL's __construct_new_array (MAIN 0x182070); CScriptArchive is 0x108 bytes.
- **Tells from the sound and the valves, round 4** (zsnd_csndinstance, zsnd_csndgroup, valve_list.cpp ...):
  - `iterator e = end(); i = remove_if(begin(), e, p); erase(i, e);` gives the target's single
    end in s0; written as one expression, MW builds end() twice;
  - a vector walk whose iterators live in stack slots (copied with lwc1/swc1) is `std::for_each`
    with a functor (CValvePool::Free);
  - an unsigned counter compared `(int)i < n` stops MW strength-reducing `_zrdr::Item()`'s address
    (Parse, LoadCSndGroup); `len <= size - (int)(next - names)` branches on `slt at` with the
    target's registers;
  - a byte enum given real non-negative enumerators becomes unsigned and compares with `andi
    0xff` (VALVE_TYPE: ANY 0, PERM 1, TEMP 2, PERSIST 3);
  - bit-fields set in a constructor's initialiser list compile to one word read-mask-write each
    (CValve's constructor);
  - SOCOM II's CSndInstance::Tick adds a Doppler shift: the word at 0xC is the listener distance
    (a float), CalcVolPan returns the new one;
  - `989snd.c`'s name starts with a digit: sccheck counts its matches but lists them only with
    `-v`.
- **Tells from the states, round 11** (CGame::Tick, the bomb, the remote SEALs, the persona):
  - a byte enum copied into an enum-typed field loads with `lb`, into an `unsigned char` with
    `lbu`;
  - an `unsigned char` bitfield compared through a byte-enum cast gets `andi 0xff`; MW extracts
    any byte bitfield with dsll32/dsrl32, so those shifts do not mean a 64-bit field;
  - a ternary of enum constants cast as a whole (to an enum with no enumerators) sign-extends;
    casting each arm does not;
  - a switch returning through a result local (`bool r = false; ... r = true; break;`) puts
    `move v0,zero` ahead of its first test; `bool r = c ? f() : false;` gives `move` then
    `andi`, `if (c) r = f();` gives `andi` straight from v0;
  - in `id == f()->x` MW keeps `id` in a saved register across the call; taking `f()` into a
    block-local first makes it re-read the field;
  - `if (A && B) ... else if (C && D)` sends a failing B to C's test; nested ifs do not;
  - a caller that leaves an argument register unset shows the callee's last parameter does not
    exist (NetSendE3Index takes three in SOCOM II);
  - CGame::Tick: testing Init's bool `!= false` gives the target's `andi`, and the nops were
    label padding; the thread wait is `while (!flag)`.
- **Tells from the entity and the console, round 8** (ent_main.cpp, con_ps2.cpp, zentity.h):
  - SOCOM II stores through a pointer list's node at its offset where SOCOM 1 reused `*it`'s
    reference temporary: write the node's `data_` directly (RecycleEntity);
  - a ternary bool with an int `0` false arm puts the `andi` on the compare's arm only; with
    `: false` it lands at the join; `int n = c ? m : 0` gives the target's saved registers where
    the if-assign swaps them (CConsole::Tick);
  - `!(bool)f()` with `continue` materialises `sltu; xori` before the branch; `!= false` and
    `== false` do not (FindPacket);
  - two `new`s in a ternary each branch on v0 with the copy in the delay slot;
  - strlen in an argument walk: take the length into a local and add `(len + 1)` to recompute
    the element address after the call;
  - an inline setter's `this` copy avoids reloading an address-taken local between two field
    stores (AddHandler); con_ps2.cpp is `#pragma auto_inline off` throughout (the target calls
    every function of it out of line);
  - zentity.h: CAiMapLoc() is empty (SOCOM II's, 0x516120) with SOCOM 1's field-by-field copy
    constructor, CEntity initialising from `invalid`; CEntity's slot +0x0C is `const CMatrix
    *GetPosW() const`, +0x60/+0x68 take a CPnt3D * and return bool, +0x64 returns a node.
- **Tells from completing the MSL headers** (include/vector, include/string, zmath.h):
  - the class `__vector_imp::insert` is defined after SOCOM 1's lines 1428-1485 and returns
    void (five of the thirteen class inserts byte-exact; the two 8-byte-element ones one
    register off); the deleter has `T *&data()`, the class push_back/begin/end read through it,
    and its destructor is `clear(); if (data_) delete` with clear's loop in clear's own body
    (inlined into tail code clear came in one level only, its static helper called);
  - `resize(n)` is written (`insert(end(), n - size(), T())`, else an erase) but no instance
    matches yet: each waits on its element's constructor, and the 412-byte turret element's
    insert/erase call copy, copy_backward, fill_n and the copy constructor out of line
    (0x3ABAE0, 0x3AC460, 0x3AC200, 0x3AC6D0) where the header writes those loops inline; the
    CAiMapLoc instance (0x51C4A0) misses with the empty constructor too;
  - in a unit's template tail, out-of-class members are generated in rounds -- first those the
    unit's own code uses, then those those use -- and within a round in declaration order,
    later-declared class templates first; a tail function inlines another only if that one was
    generated before it;
  - inline calls in an argument list (`end()`, `size()`) are expanded ahead of an out-of-line
    constructor call in the same list;
  - at a join, `if (n) do ... while (--n)` gives the target's top-tested loop; `for`/`while` are
    entered at their bottom test;
  - `newsize > newcap` and `X <= xp` branch on the compare directly; the reversed forms put the
    result in a register first;
  - CPnt3D's `+ - *` are const members (SOCOM II's 0x557DC0, 0x557E10, 0x557E90), Cross
    declared; CPnt4D's written copy constructor still loses SeeFarther and stays out;
  - still open: include/deque's copy constructor (CRdrEditor() waits), list<T *>'s
    insert(iterator, T *const &)/postfix ++/iterator copy constructor (PathPlanExecute), the
    list clear keeping `&end_.next_` in a register (five functions), the iterator's `*` by value.
- **Tells from the zdb world, round 5** (grid_di.cpp, grid_clutter.cpp, int_main.cpp, node_world.cpp):
  - SOCOM II's pointer-list begin() builds its first node from a REFERENCE to the sentinel's
    link (`node_base *const &`): `&end_.next_` becomes a temporary MW computes once and reuses
    for a walk and the clear after it, in the lowest saved register -- the shape behind CValve's
    destructor, ~CZAmmoList, zNetFlushMessages and others; znode.h's stand-ins (PtrListFirst,
    ListBegin, ListEnd, ListClear) carry it until include/list does;
  - a constant stored into a bitfield through an inline setter is masked at run time (`(0 & 1)
    << n`); assigned directly, it folds;
  - taking a vector by reference before its size() test keeps the address in a saved register
    for the walk;
  - in an inline, `if (i == 0) return 0; return i <= n ? &d[i-1] : 0;` gives the target's two
    tests with a zero in each delay slot;
  - `if (!p) return false; return p->bit ? true : false;` puts the false block first; `!= 0`
    puts it last;
  - a float comparator written `if (a < b) return -1; if (a == b) return 0; return 1;` pads as
    the target does; `return a != b;` misses by the pads;
  - grid_di.cpp (SOCOM 1's name) runs ~0x2D2890-0x2D4FDB, closed by ~CStack's copy, its
    initialiser 0x401CC0; 0x2D2070-0x2D2830 look like the graph unit's tail.
- **Tells from the AI states, round 4** (CAiState::Create, Reset, Signal):
  - a function inlined whole in one caller and called out of line in another sits between two
    size limits: Reset expands to at most inline_max_size (Signal inlines it whole) while
    CAiState(AI_STATE) -- Reset plus the table store and the stack's constructor -- exceeds it
    (Create calls it out of line);
  - `a = b = c = 0.0f` stores c, b, then a, and costs fewer inline units than three statements;
  - a constructor inlined one level calls its members' constructors out of line, even empty ones
    (CPnt3D(), CAiMapLoc()): a member built by hand in the body matches only if it is the last
    one, or every later member is built by hand too;
  - an inline setter called with a constant can reuse the register a switch's compare chain
    loaded (`li v0,1` feeding `andi t1,v0,1`): the mask stays unfolded there, folds elsewhere.
- **Tells from the UI, round 4** (zui.cpp, the specs' CreateDisplayObj):
  - an abs written `f >= 0.0f ? f : -f` gives the target's `bc1t`-to-negate and a `b` over it;
    `f < 0.0f ? -f : f` if-converts to `bc1f` over a `neg.s`;
  - MW gives an override of a secondary base's virtual a new primary slot after all the tables
    (CPlainBmp::UpdateUIVar at +0x80), the derived class's new virtuals after it;
  - two struct members with inline constructors are built inline, an array of them in a loop;
  - an inline constructor too big to inline whole is inlined one level, calling its members'
    copies; a class wrapping it then calls it out of line (CWrappedText -> CScrollText);
  - declaring a defined virtual first among a derived class's non-inline virtuals makes the
    unit emit its table and implicit destructor; slot order still follows the base;
  - zui.cpp is 0x365200-0x36AF9F (CDblBufTextureRelocator's inline virtuals open it); the soft
    keyboard and the rdr-table code after 0x3A2510 use MSL's map (include/map since round 3 of
    the MSL headers).
- **Tells from the HUD, round 12** (hud_healthbar.cpp complete, hud_scorepopup.cpp, ~CHUD):
  - 8-byte-spaced float pools are function-local statics (a float literal is lui/ori/mtc1; a
    target loading constants with lwc1 there wants statics) -- write them as file statics, which
    scdiff compares by content;
  - MW 3.0 calls a global object's virtual through the vtable when its class inherits the
    function without overriding it (CScoreBmp, CHudBrightText); it devirtualises only on the
    declaring class;
  - two inline calls in one argument list expand right to left: an inline accessor's load in the
    last argument goes ahead of the first argument's `c_str()` ternary, which then compiles
    branchy;
  - a stand-in class declared only for one virtual must declare its destructor without defining
    it, or the unit's implicit copy supersedes a matching draft;
  - ~CHUD matches at level 5 with CHUD's texture lists as znode.h's zdb::CDynTexList (SOCOM 1's
    classes over the pointer vectors); the font stretch from 0x359740 is SOCOM II's rewritten
    font system ("faces", "glowtexture", "charoffsets", "Korean", "dropshadow") -- no SOCOM 1
    counterparts.
- **Tells from the SEALs, round 6** (the ladder/jump stretch, seal_anim.cpp, seal_cfg.cpp):
  - each CPnt3D local takes a 16-byte frame slot;
  - a byte-enum local is assigned with `li`, and comparing it with its zero enumerator gives `andi
    0xff` plus `sltu`; an `unsigned char` local is assigned with `daddiu` and its conversion to
    bool drops the `andi`; converting a bitfield to the enum before masking keeps the `andi 0xff`;
  - in a lookup loop, a byte local declared inside the loop, after the counter, takes s0 and the
    counter s1;
  - writing the random fraction out instead of calling the Rand01 inline gives the target's float
    registers for the constants (CBlink);
  - an uninitialised flag declared before the object pointer and set after a call keeps the
    higher register and the zero's position after the call (CanPlaceBomb);
  - a short field read into an `int` local before a switch is tested from the `lh`-loaded
    register with no re-extension;
  - CAiTimer lives in zseal/aitimer.h, included ahead of ai_states.h (its float constructor
    0x5EA550); seal_anim.cpp runs 0x575CF0-0x5781BF, seal_env.cpp from 0x59D6C0 to 0x5B818B.
- **Tells from the weapons, round 5** (zwep_ammo.cpp, zwep_projectile.cpp, zwep_flyout.cpp):
  - in a float function `if (!(x != 0.0f)) return s;` gives the target's `bc1f` over a `b`; `==
    0.0f` or an if-block fold into one `bc1t`;
  - `Dot(d, d)` through zmath.h's const-reference inline reads the fields off the frame; written
    out, their addresses stay in registers (IsInRange);
  - an inline search that reads size() into its own local reuses the caller's earlier load;
    passing the count as a parameter swaps two argument registers (ReleaseFlyout);
  - FreeVector on a CNodeVector calls CNodeVector's own constructor and destructor; the target's
    std::vector<CNode *>'s -- write `FreeVector<std::vector<CNode *> >(...)`;
  - a whole CBBox local assigned to a member keeps its six stores and copies; two CPnt3D locals
    assigned field by field fold away (CFlyout's constructor);
  - Init and Reset match only with every flag stored through an inline bool setter in the
    original order (`false` shares one `andi rX,zero,1`, `true` becomes `daddiu ...,1<<n`);
  - `if (!a || t == 1 || t == 2 || c) return; f(this);` reproduces the shared-`b` layout with
    the range test folded; nested ifs do not;
  - OVL1's last code unit is 0x3D4810-0x3D5947 (a pointer vector, CProjectileNodeEx, the flyout
    machine -- moved from between zwep_global and zwep_projectile in SOCOM 1).
- **Tells from completing the MSL headers, round 2** (include/list, include/vector):
  - a list's begin() reads the first node through a REFERENCE to the sentinel (`node_base &t =
    end_; iterator(t.next_)`), in the deleter, its destructor and the pointer list's wrapper alike:
    MW then computes `&end_.next_` once as a compiler temporary and keeps it in a saved register
    for every later begin() of the function -- a walk's, a clear()'s, the implicit destructor's
    (CValve's destructor, DeleteCallbacks, GenerateLightList, ~CZAmmoList, zNetFlushMessages ...);
    a reference to the link itself (`node_base *const &`) does it only in part;
  - a walk that re-reads its element after a call casts it out by value through the base
    iterator, `(T *)*it.i_`: the wrapper's `*` stays a reference, because CGNode::FindEdge reuses
    the value its first `*i` loaded (a by-value read never does, even with no call between) --
    no one form of the operator serves both; SerializeValves and RemoveTask(name) re-read the
    iterator itself after each call, as only an escaped iterator does (open);
  - a member declared before the class's own `typedef ... iterator` sees the base's iterator:
    list<T *>::insert must follow the wrapper's typedef;
  - SOCOM 1's remove_if over list<CSched_Task *> gives find_if's first/last s4/s5, SOCOM II's
    s5/s4 (0x29FE70, open): its two parameter copies are made in parameter order there;
  - vector<CAiMapLoc>::resize (0x51C4A0) is byte-exact through the header's resize(n) as it
    stands (the element's empty constructor builds nothing); CSched_Manager::Clear (0x34E480) is
    `if (size()) { std::for_each(begin(), end(), DeleteTask()); clear(); } ClearFreeList(false);`
    with CSched_Task's destructor inline (`if (m_name) _free(m_name, 0, 0); m_name = 0;`) and a
    value-initialised functor -- byte-exact as a probe (build/msl2/probe/sched.cpp), waiting on
    zanim.h's declarations; ai::PathPlanExecute is byte-exact as a probe over the new list members
    (build/msl2/probe/ppe2.cpp), waiting on CAiPathQueue's insert inline and the GetPriority/
    IsPlanning rows (0x510C20, 0x510C00).
- **Tells from the states, round 12** (fts_remote.cpp's messages, fts_net_blather.cpp):
  - when a strength-reduced loop's base is dead afterwards MW walks the base itself; the target's
    separate induction register needs an explicit `char *dst` cursor;
  - a value the target keeps in a saved register and stores only just before the call is a
    second local (`count = votes; f(&count)`); taking the computed value's address forces it into
    memory;
  - `a<<20 | b<<10 | c` evaluates c first; left-to-right conversions need locals (`int ax =
    f(x)<<20; int ay = f(y)<<10; int az = f(z); p = ax | ay | az;`); float arguments computed in
    source order need locals too (MW evaluates call arguments right to left);
  - `bool b = true; if (x != 1 && x != 2) b = false;` in an inline gives the target's `daddiu` in
    the first beq's delay slot; the `||` forms fold into a range test;
  - a same-unit stand-in for a template function (`_saturate<int>` as a plain function) must be
    pinned with `#pragma auto_inline off`: MW auto-inlines the plain function, never the template;
  - left for the next states round (the definitions switch with them): 0x2BAF30 ->
    E3IndexPacketHandler__FiiiPv, 0x2BA9D0 -> ChatPacketHandler__FiiiPv, 0x2BD220 ->
    `_saturate<i>__FPiRCiRCi_b` (SOCOM 1's call sites).
- **Tells from the movie, IOP and network units, round 4** (src/gamez/zmpeg/, zNetCnf.cpp, trc.cpp):
  - `own = p = 0;` into a bool compiles to `sltu v1,zero,zero`: the pointer-to-bool conversion is
    not folded (CMPEG::Uninit);
  - in a comparison MW evaluates the right operand first: `present + size > end` matched where
    `end < present + size` did not (nodataCallback);
  - GS display bitfields need 32-bit fields: `unsigned long` turns the DY update into
    ld/sd/daddiu/dsll (playPss);
  - `(unsigned char *)&s->hdr + n` gives `addu n,base` where the array decay gives `base,n`;
  - a `= {0}` pointer array zeroes bytewise, the loop count giving the array's size;
  - Sony's ezmpeg sample code reproduces nearly as written (three audiodec functions matched on
    the first try); the Sony units are split where tiny functions are called out of line
    (`-inline auto` would have inlined them within one unit);
  - zmpeg.cpp 0x309550, audiodec.cpp 0x309F60, playpss.cpp 0x30A860, ldimage.cpp 0x30B740,
    strfile.cpp 0x30B9D0, zNetCnf.cpp 0x30BD40, trc.cpp 0x30C760, zNetDme.cpp from 0x30CA50
    (SOCOM II's new CZNetwork functions sit ahead of SOCOM 1's first).
- **Tells from the AI core, round 6** (ai_map.cpp, PathPlanExecute):
  - bitfields set in a mem-initializer list give word read-mask-writes, body assignments byte
    stores; a layout-identical stand-in struct with such a constructor reproduces a missing
    constructor's temporary (GetMapRegion);
  - a static read twice across stores is reloaded after the first store; passed as arguments to
    an inline setter it is loaded once (GetDirVec);
  - an unsigned loop test `i <= 10` branches on `sltiu at`, `i < 11` puts the test in v0
    (GetClosestDir);
  - a walk inline too big to inline whole is inlined one level, its callees called out of line --
    even an empty constructor -- and a function-pointer argument becomes a direct `jal`
    (ResetCells);
  - fifteen statements plus the return exceed -inline auto's limit (CAiCell() calls Reset out of
    line); PathPlanExecute's queue insert is an inline CAiPathQueue::Insert taking the path by
    value;
  - ai_astar.cpp ends at 0x51557F (its tail the three statics' destructors), ai_map.cpp runs
    0x515580-0x51A48F; the map layer's own code sits after vehicle_rdr.cpp (0x53A730-0x53A9C0).
- **Tells from the AI states, round 5** (slot 0x34, CAiStateStack's operations):
  - a frame's temporaries are laid out by inline depth first, then in order of appearance (Set
    matched only once push_front sat two inline levels deep);
  - a value passed by reference to insert and read again after should come back as an inline's
    return value: one load after the call, kept in a register (Push);
  - inside an inline bool, `bool r = a || b; if (r) r = c && d; return r;` gives the target's
    layout (EscorteeJoinUnit); `return r && c && d` and the ternary do not;
  - an `unsigned char` call result stored into an `int` field with no second zero-extension wants
    an `int` local, which takes the one `andi` at the call;
  - SOCOM 1's PushAiState bool is a flags word in SOCOM II, and the states' Tick returns `int`;
    CAiState's slot 0x34 takes (CSealCtrlAi *, CAiEvent *, CZSealBody *, bool);
  - a lane's call to a base class's member that a newer header overload hides fails to build at
    merge -- name the base (`std::__list_deleter<const void *, ...>::insert(...)`).
- **Tells from the particles and the render init, round 3** (zparticle.cpp, zparticle_buf.cpp,
  zrndr_init.cpp):
  - MW unrolls a constant-trip loop 8x whatever the body; a 4x-looking unroll was an inner loop of
    four unrolled whole inside an outer loop left rolled (zRndrInit);
  - a CPnt3D copied from a matrix row loads z, y, x and stores x, y, z only through the
    three-float constructor (`p = CPnt3D(m[0], m[1], m[2])`); a struct or field copy interleaves;
  - a constructor too tall to inline whole is inlined one level, calling even CPnt4D's copy out of
    line; a member copy only declared makes the outer constructor short enough to inline whole,
    so give it its real height (Branch);
  - a free-list push `p->next = head ? head : 0;` gives the target's select, stored in both arms;
    the if/else stores zero directly (Recycle);
  - `T *p = m_head; if (!Empty()) ...; else p = 0;` reuses the head's load as the result where
    `return !m_head;` materialises (InstancePool::Pop); `bool r = false; if (a && b) r = true;
    return r;` puts `move v1,zero` in the first test's delay slot (List::Tick);
  - a by-value CPnt4D parameter is read off the frame only through a local copy (`CPnt4D q(p);`);
  - `delete p` of a class with no destructor emits no null test: a target with one was `if (p)
    delete p;` (~Source);
  - zparticle.cpp 0x3223D0-0x324C6F, zparticle_buf.cpp from 0x324C70, the instance unit from
    0x326F50, zrndr_init.cpp 0x335FD0-0x33622F; 0x321E00-0x3223CF is a vis unit's CLensFlare/
    CRipple code.
- **Tells from the SEALs, round 7** (~CSealCtrlAi, SEAL_AI_PARAMS):
  - an RTTI's base list gives a second base's offset: that base is constructed before the vtable
    store, and its inlined destructor sits behind a null test of `this + offset`
    (CSealCtrlAi : CSealCtrl, SEAL_AI_PARAMS at 0x250 -- its state stack at 0x41C, a CAiPath at
    0x46C, a route at 0x514, a CAiEvent at 0x628, a CAiEventQ at 0x650);
  - a caller that never sets $f12 shows the callee has no float parameter;
  - a product's member factor read through an inline accessor is loaded first (SetViewRange);
  - a signed square `if (r < 0) r = -(r*r); else r = r*r;` gives the target's two arms; a ternary
    or a separate result local if-converts;
  - a load the target puts in a null test's delay slot needs a local taken before the test;
  - a CPnt3D copied field by field gives load/store pairs through one register; copy
    construction loads all three fields first;
  - tools/perm.py fixed a locals-only register miss in one run (SealGetAltitude);
  - `ovl-copy:` notes are read from data rows too (scdecomp's address_in): an RTTI's overlay copy
    (0x65C220 for zdb::CNodeEx's) resolves a `dynamic_cast` in OVL2.
- **Tells from completing the MSL headers, round 3** (include/tree, include/map, include/vector):
  - SOCOM II's red-black tree is SOCOM 1's MSL tree (CW 3.0.1's `tree`, its DWARF and symbol
    table): __red_black_tree<1> holds the links (left_ 0, right_ 4, parent_ 8 with the colour in
    bit 0, set = red; the value at 0xC) and the static rotate_left/rotate_right/balance_insert --
    one copy per overlay (OVL1 0x211030/0x210FA0/0x210D80, OVL2 0x58DB20/0x58DA90/0x58D870); the
    tree is size 0, anchor 4 (left_ the root), comparator byte 8, leftmost 0xC; the map is its
    tree alone. insert_node_at's message is "tree::insert length error\n";
  - hud_linemap.cpp's maps (SOCOM 1's g_mapLineProps, g_mapPolyProps: std::map<int, LINE_DATA>)
    make its tail byte-exact from source, 12 rows; the soft keyboard's nine and find<int> are
    byte-exact as probes (build/tree/); the shapes that settled them are in include/tree's comment
    (the grandparent local in balance_insert, prev/p/n's declaration order, `size() > max_size() -
    1`, destroy's self-inlining against the tree's own accessors);
  - MW 3.0 folds `x >= 0xFFFFFFFF`, `x == 0xFFFFFFFF` and `!(x < 0xFFFFFFFF)` into one `bne -1`;
    only `x > max - 1` keeps the target's `sltiu -1`;
  - a member template's mangled name carries its return type (`find_or_insert<i,9LINE_DATA>
    __...FRCi_RQ23std19pair<Ci,9LINE_DATA>`): SOCOM 1's compiler left it out;
  - `true`/`false` bound to pair's const references are pooled constants loaded with lbu
    (insert_one 0x38EC10: `lbu v1,1160(at)`);
  - a by-value empty comparator copied into a member is a byte copy (`lbu; sb`); the map's
    value_compare(Compare) leaves its member unset, as the image's constructors copy nothing;
  - the class vector's copy constructor zeroes the deleter's words and calls the out-of-class range
    init<const T *>(first, last, forward_iterator_tag) (the particle sequences' 0x328FC0,
    0x3290D0, 0x329220, byte-exact); MSL checks the length on the size there, not the distance;
  - still open: the map constructor's allocator temporary (the image builds none, include/memory's
    allocator() is out of line for the deque), the tree's erase (0x306030), the CSaferStr trees
    (the string's out-of-line compare 0x2DEE30), the deque's copy constructor chain (mapped in
    include/deque), pop_back's 8 bytes, remove_if's swapped registers.
- **Tells from the HUD, round 13** (hud_scorepopup.cpp's spectators, ztex_manager.cpp's tail):
  - a walk's end test through an inline member (`CEntityItr::More`) gives the constructor's
    `begin()` temporaries the higher stack slots; written straight into the loop test, `end()`'s
    get them (GetTeamMembers);
  - a struct returned by value goes straight into the return slot only through a constructor
    (`return IPNT2D(x, y)`); a named local is built on the stack and copied (GetTagIPNT2D);
  - a member updated across a store through another field of the same type is reloaded; the
    target's single load is a local, and its declaration order against the element pointer picks
    their registers;
  - `int i` indexing a pointer vector's `operator[](unsigned)` keeps the `sll` on every pass;
    `unsigned i` strength-reduces it;
  - MW 3.0 gives the first-declared float local the highest saved register (SOCOM 1's compiler
    gave it the lowest), and reusing one set of locals across blocks re-colours the later blocks;
  - 8-byte-spaced statics that read zero in the image (0x3E5718, 0x3E5720) are set by the unit's
    initialiser;
  - ztex_manager.cpp's share from 0x357000 (CGSTexBuffer's Setup/Add/RelocateAll, CDynTexList's
    Adds) is SOCOM 1's order; the unit itself starts near 0x3557xx, so the two halves become one
    file once the rest is written.
- **Tells from completing the MSL headers, round 13** (the instances other lanes waited on):
  - include/list's range and single erases DESTROY each element (its destructor called with -1) before freeing the
    node -- pointer lists unaffected; the list erase re-reads the iterator's node after the element's destructor
    call (ai_group.cpp's step-list erase 0x60EA90 matches; the single erase 0x60EB60 matches as a probe and counts
    once CAiGroup::Tick 0x60C3E0, its only caller, is written);
  - include/string gains find_first_of/find_last_of/find_first_not_of/find_last_not_of in all their forms, at()
    and operator== (12 zrdr_parse.cpp rows named, CRdrIO::Open now calling them as members): the character set
    is walked by its own pointer beside the counter (indexing `s[i]` does not give the target's code), and the
    not-found exit of the *_not_of searches is a `goto` out of the inner loop; the list splice 0x33E790 was
    vis_texfx.cpp's copy already;
  - MW 3.0b52 compiles the SFINAE `is_class` test (`int U::*`: enums, scalars, pointers and member pointers 0,
    structs 1) -- include/msl_utility has it (and std::min, moved from include/string), and include/deque destroys
    class elements through it, scalar ones not: the chat deque's clear 0x2F5A30 matches;
  - with `destroy(begin(), end())` through inline wrappers end's make_iterator runs first (the target's order),
    written `destroy(make_iterator(0), make_iterator(size_))` begin's does, in the same stack slots; the chat
    iterator's subtraction helper stays a call in the destroy even as an in-class member;
  - open (measurements in include/deque's comment): every make_iterator (best 0xB8/0x100 against the target's
    0x130/0x138 -- the target keeps the block iterator in memory), the chat's range destroy 0x2F5B10 (0x20C vs
    0x1F8: a helper generated too late), the chat teardown 0x2F62B0 (it needs a destroying class between
    __deque_imp and __deque_buf, moving the iterator into __deque_buf and renaming nine rows), the lobby deque's
    pieces; 0x32FB90, 0x3323F0, 0x332560, 0x3320E0, 0x333120 are code-exact over a stand-in element and wait for
    the world lane's real 28-byte record (a byte and two string-like members).
- **Tells from completing the MSL headers, round 12** (THE INLINE LEVEL, measured header by header and not
  adopted: is SOCOM II built at `-inline level=8` with MSL deeper than include/*? -- build/scratch/msl-r12/
  table.txt and its logs, probes build/p12/, untracked):
  - MW's level stops at 8: the integrator's whole-tree logs at 8 and 9 are byte-identical; a chain probe at 8
    inlines an 8-high callee whole and a 9-high one one level;
  - SOCOM 1's DWARF and SOCOM II's image agree on the vector's capacity pair: SOCOM 1's ~vector<T *> copies
    call compressed_pair_imp<allocator<Ui>,Ui,1>::second() then first() out of line (0x1735A0, 0x173590 --
    the pod's destructor `if (data_) alloc().deallocate(data_, cap())`, <vector> 230-233), SOCOM II's
    func_002B55E0 (a list of actions deleted) calls its OVL1 copies 0x2B5750/0x2B5740 the same way, and
    zui_hangul.cpp's deleter clear 0x393C60 calls the char pod's 0x392750/0x392740 five this-tests deep: a
    destructor chain cut at its LEAVES, which the one-level model does not produce (open);
  - modelled (compressed_pair_imp<A, Ui, 1> deriving from the allocator: the class constructor 6 high and
    its swap 6, the pod's constructor 6 and destructor 6, 95 units), level 8 gives 6761 over every
    hand-written unit (master's headers: 6746 at 8, 6800 at 5), +15 and nothing lost: ~CCell, ~CHUD,
    ~CVisual, ~CGSTexBuffer, doAddBuffer, SaveNodeStates with its four constructors, st_core.cpp's FreeCues
    path, ai_path.cpp's TrimVector -- and every level-5 witness holds at 8 (CreateAnchor,
    __sinit_compass.cpp, CVisual(), ~CGame, ~C2D, CCharacterType(), the chat deque). ~CVisual's cut is a
    SIZE cut there: CDynTexList's two member chains come to 259 units, while ClearGroups' group destructor,
    the same height (8) with one chain (157 units), inlines whole -- no height separates the two, only size;
  - NOT CLOSED: a class FreeVector is then 7 high and 250 units and inlines whole at 8, where the target
    calls its four pieces (FreeTables, DestroyPool, ~MapCompass, Close, WipePktBuf: 25 rows), as the
    pointer TrimVector (GarbageCollect). They need the class swap 8 high and the pod's 8 (or FreeVector past
    256 units): two and three forwarding levels no DWARF or image shows. With them (a measurement, branch
    commit 69f18387): 6788 at 8 (+27, nothing lost), 6739 at 7, 6672 at 6, 6624 at 5 -- still under 6800;
  - the list's sentinel through SOCOM 1's tail() (<list> 174) over the end pair's second() (853) -- the
    image's 0x2D2530 and 0x207560 -- in begin()/end()/~__list_deleter alone loses 32 at 8 (the walks'
    shared &end_.next_: ~CValve, DeleteCallbacks, GenerateLightList, ~CCell, ClearEffectors, ~CSealUnit, the
    remove_if copies) and brings back none of the target's list cuts (~CEntityList, ~CAssetList,
    ~CAiEventList, valve_list.cpp's iterator pieces, SelectNext/PreviousObjective through GetObjective's
    walk): SOCOM 1's whole list shape (size_ and end_ both compressed pairs) is a round of its own;
  - unexplained at 8 by any header tried (26 rows against level 5): the string constructor's chain (main.cpp's
    CGameState(const char *), SetDefaultMenu, CSaferStr(), COurGame() -- SOCOM 1's DWARF has the older
    string), placement new and the element copies (C2DPoly's copy and vector<C2DPoly>::reserve, CInit<Us,0>(),
    CJamoStr()), zui_skb.cpp's ShutdownBegin/SwitchTo, ~CWaterRippleManager, ~CTextureFXManager, ~CZNetwork,
    RTArray<CDecal>'s destructor and the list family above;
  - the vector range assigns are MSL's do_assign (SOCOM 1's symtab; include/deque already spelled the deque's
    so): renamed with rows 0x259D20, 0x393FB0, 0x53F370, 0x53F730 -- no codegen change (7100 before and after).
- **Tells from the UI, round 12** (C2DFont through c2dfont.h, CSkbFonts(), the vector<CSkbKey> instances):
  - MW's implicit operator= copies a union as its first member: a class that copy-constructs with lb/lhu but assigns
    with lbu/lh has UNION members -- CSkbKey's, which unlocked its vector's erase, resize, init and do_assign;
  - a ternary bool argument (`p ? false : true`) passed to an inline setter is recomputed at every use after the
    setter's own loads, written `!p` it is computed once ahead of them;
  - a loop whose element address is the induction variable (base+offset stepping) belongs in a member function of
    the array's owner (CSkbTexLists::Free, CSkbFonts::Unload), where FreeVector must be written out (called, it is
    one inline level too deep); through a const item pointer with const accessors the type byte loads once for
    Type()/String()/Integer(), non-const accessors reload it; `if (!name) to = 0; else to = Find(name);` lays the
    zero arm out before the search;
  - zui_skb.h includes the HUD's c2dfont.h (C2DFont(), ~C2DFont, CSkbFonts() byte-exact; the 0x64-byte stand-in
    gone); the hangul composition types moved to a new zui_hangul.h; 0x3503F0 is vector<CIOPManager::CRegistrant>'s
    range erase (renamed by the integrator: util_main.cpp's copy and UnRegisterRebootCallback match with it);
  - open: CreateSoftKeyboard (the target's size, ~100 instructions shifted: one register short -- the target
    recomputes the key's address each pass), insert<CSkbKey> 0x38F1F0 (8 short), the grid's clear 0x38FEF0 and the
    pieces' clear 0x3929C0 (the element's destructor one inline level deeper in the original), the vector<vector<
    CSkbKey>> pieces 0x38EDC0/0x38F640/0x38F7D0; CSkbLook() 0x38E790 is code-exact but for 0x38D130, ~vector of a
    0x2C-byte {CSaferStr, two rects} -- m_sources[4]'s real element; CUIVariable's UIVarMixin base (zui_2d.h's, the
    HUD lane's) is not modelled yet (moving it ahead of zui.h is not additive).
- **Tells from the AI core, round 16** (the ai_maps.cpp block, CAiMap::CreateGraph and its walk):
  - a pointer passed to push_back by reference lives in memory: copying it into a second local first (`CAiMap *m =
    map; push_back(m);`) keeps the original in a saved register (ReadMapSet);
  - one assigned iterator reused for a second walk splits into a later live range whose stack slot falls below the
    begin() temporaries -- a second named iterator does not (UpdateActionTables); `iterator last = end();` taken
    before a walk gives begin a0 and end a1, `i != end()` swaps them; a walk's end held in a function-level local
    spills the counter and `this` as the target does;
  - early `return 0`s through a result local (`found = 0` ... `found = best`) let MW zero v0 once at entry
    (FindLinkEdge); an inline bool taking map ids by value loads them once before its `||` chain, by reference it
    reloads them; a list reached through a reference local gets its end() as two adds, read straight off the owner
    MW folds the offset;
  - the out-of-line copies of a walk inlined one level reveal its CONSTNESS: copies distinct from an earlier unit's
    non-const ones are the const overloads (CreateGraph's ComputeLength walk; include/vector gains the class vector's
    const begin()/end() and its const_iterator's postfix ++, additively);
  - data: AiMaps is 0x2E0 (ReadMapSet copies 512 bytes to +0xDD); 0x2D0930 CGraph::CreateNode never reads a1 and both
    AI callers leave it unset -- `CreateNode__6CGraphFv`, the states lane's row and unit to change together;
  - open: UpdateNameTables 0x52B820 (0x324 vs 0x350: the target keeps both loops' entry tests in out-of-line blocks,
    MW folds them); find_path waits on heap templates include/algorithm lacks (0x5132B0, 0x513460, 0x515270 are
    SOCOM 1's push_heap, pop_heap and __insert_heap_down over the A* node; 0x5121C0/0x5123A0 its helpers).
- **Tells from the kit and AI states, round 17** (ai_group.cpp's leftovers, eight CT* task parsers):
  - an inline that copies a name into a block wants the destination as a local (`char *d = m_str;`) -- written in
    place MW folds the address into each branch, the target computes it once before the null test; a member
    TASK_ENT's `this` stays in a saved register across AddName only when an inline Create sets it up;
  - byte-enum locals stored into word fields sign-extend at the store (dsll32/dsra32 0x18: CTStance's parse, its
    two fields words -- +8 the peek, +0xC the stance);
  - A STACK POINT'S FIELD ADDRESSES: MW keeps the addresses of a stack CPnt3D's y and z in registers where the target
    reads them straight off the stack (`optimization_level 3` and `opt_common_subs off` change nothing) -- it holds
    four functions in three lanes: ai_group.cpp's LookAt 0x60DCF0 and FaceXZ 0x60CAD0, camera.cpp's LookAt 0x29BC90
    (the same code as 0x60DCF0), seal_env.cpp's FindDeathAnimTypeCQB; whoever cracks it closes all four;
  - MW strength-reduces a secondary counter's index into a register of its own in a loop driven by an iterator, the
    target shifts the counter inside the loop (CAiGroup::Tick 0x60C3E0: right size, 285 instructions of register
    shifts; `#pragma opt_strength_reduction off` drops the register but loses the hoisted &p.y/&p.z the target keeps);
  - open too: AddMember 0x60D020 (4: the Is() materialisation that keeps CAiStateStack::Pop(ai, AI_STATE) open),
    act_camera::Tick 0x60DB60 (the target loads a point's three floats before storing any into the matrix's
    translation row). The MSL lane's round 13 list erase made both step-list erases match from ai_group.cpp.
- **Tells from the SEALs, round 15** (ComputeTurnThrottle, ClearGoal, the grenade state, the alert update):
  - CSealCtrl's vtable slot 0x9C is SOCOM 1's non-virtual ComputeTurnThrottle made virtual: CSealCtrl's 0x56C220 and
    CSealCtrlAi's 0x56C340 inline one shared body and share its function-local statics (0x650110-0x650120);
  - an inlined early-out written `m_goal_type > type` puts `slt at` in the caller's delay slot, `type < m_goal_type`
    computes it into v0 after a nop (out of line both compile alike); `p = c ? q : p;` gives a `beq` to the
    assignment plus a `b` over it, `if (c) p = q;` one `bne`; `case 0: case 1:` sharing a body ends the compare
    chain with a `b` where `||` does not;
  - a constant returned or assigned as an enum is `li`, as an unsigned char `daddiu`, and a byte-enum result tested
    `!= 0` keeps its `andi`; in an inline `if (!a || !b || x != f()) return 0; return p;` gives the zero arm's `b
    join; move s,zero`; matrix elements read straight (`m.m_matrix[2][0]`) and through `operator()` give opposite
    madd operand orders; a call argument taken into a local first moves the `this` copy after it; a float
    compare's arm order follows the source (`x >= 0.0f ? 1.0f : -1.0f` is the target's `bc1t`);
  - 3.0b50 and 3.0b38 compile SetGoalDir and SetAiBit2 exactly as 3.0b52 (no build separation there);
  - open: SetGoalDir 0x56D920 (8: the target reloads x after the sqrt, ours keeps it -- the matched
    CPnt3D::Normalize keeps it too, so the sqrt is no barrier) and SetAiBit2 (the same kind), 0x56D530 (12: the
    entity in t3 vs t2), 0x55F7F0/0x55F940 (27 each: the result flag numbered below the parameters -- not a plain
    local?); not reached: 0x557190, 0x55AC20, 0x5784A0, the unclaimed 0x5781C0-0x579D9F, the data candidates.
- **Tells from the game states, round 19** (st_shutdown.cpp and st_reboot.cpp, DisplayMessage, Purge, C2DMessage_Q()):
  - MW pools identical literals within a unit, so a duplicated literal in adjacent code marks a unit boundary:
    0x22EA50-0x22EBCF (st_shutdown.cpp) and 0x22EBD0-0x22EDAF (st_reboot.cpp) each carry their own copy of the
    literals both use; a unit's code strings are laid out in emission order;
  - a byte-enum parameter is tested with `lbu` and passed on with `lb`, a `char` parameter tested with `lb` -- the
    message type is a byte enum (MSGTYPE, ours: the three DisplayMessage rows are `...UcUc7MSGTYPE`);
  - `for (iterator i = next = begin(); i != end(); i = next)` was the only one of 21 forms whose loop-entry test
    compares the register (a while loop, `iterator i = next` or a separate begin() reload the iterator's slot); `int
    i` over a pointer vector's operator[] keeps the per-pass `sll` (SelectConfig, as the HUD's round 13 found); a
    1-bit bool field set false is `andi zero,1` shifted, set true a pre-shifted `daddiu`;
  - SOCOM 1's C2DMessage_Q members sit 0x2E0 further on in SOCOM II, its bools bits of the byte at 0x4DC;
    LobbyPlayerInfo is 0xE8 bytes; 0x2A9680 is not ~vector<CLadderData> (it transforms a box's eight corners:
    withdrawn to a placeholder) -- 0x2A96B0/0x2A9680/0x2A9550 are fts_mission.cpp's copies of shared inlines whose
    first caller 0x2A9260 (CMission's world bboxes) brings them; CUIVariable has UIVarMixin as a base at 0x8 and its
    own vptr at 0x10 (0x2B1530's RTTI: zui.h's "m_8 table pointer" is the base's vptr);
  - open: 0x2C6350 (1: the byte-enum family's `lb; andi 0xff` that MW folds to one `lbu`); not written: 0x2B1530
    (a ~CUIVariable copy), 0x2B1390 (vector<CUIVariable>::resize), 0x2C5E70, 0x2CD8A0/0x2CD9F0 (an unidentified unit,
    mostly static destructors).
- **Tells from the weapons, round 14** (zvid_SetVideoMode, CMesh's destructor and Clone, the turret, CVehicleDef::Parse):
  - MW's scheduler reorders independent constant stores within a switch arm: SetVideoMode's 480p arm matched only
    with the aspect stores written ahead of the `= 18` store, though the image stores the 18 first;
  - a union copies as its FIRST member: a bit-field struct goes through an FPR (`lwc1`/`swc1`), an `unsigned int`
    `lw`/`sw`, an `unsigned char` `lbu`; MW 3.0b52 refuses member lookup into an anonymous struct nested in an
    anonymous union ("undefined identifier") -- znode.h's tag_VISUAL_HEAD names its struct (`m_8s`);
  - a constant passed into an inline's float parameter folds `-limit` into a literal, a local initialised inside
    the inline keeps the `neg.s`; a `strlen` whose result is unused stays in the code (CVehicleDef::Parse's string
    fields -- likely an assert compiled out); `if ((p = f()) != 0)` branches on v0 with the copy in the delay slot;
  - zvid.h's `_zvid_public` padding is named fields (renderBuf2, psm, iMinZ/iMaxZ, textureBase/EndAddr, videoMode,
    hblnkRate, pcrtcDo, pcrtcOffset, modeOffset, flag90/91, ztest_on/off at 0x94/0x98) and zVid's data row is 0xB0
    (dmaFrameRestore at 0xA0, lodLevel at +0xB0); CMesh's 0x68/0x70 are m_original/m_owner;
  - open: zVid_Init 0x3B1200 (11, same size: two zero byte stores at 0x7C/0x7D sink to the 0x90 flag stores and the
    fade alpha's 255 lands in v0 -- a 23x23 placement sweep, bool/char/bitfield/volatile fields and chained
    assignments moved nothing), CMesh::AddDecal 0x3B8550 (0x11C vs 0x124: both arms of the pool choice padded with a
    nop in the target; the name and parameter types ours); not reached: 0x3B8680 (a 0x5A0 VU0 decal builder),
    GetDamage, HandleDelayedDetonate, InitializeWeapons (about ten CZWeapon accessors to name first), the turret's
    Fire 0x3A8D20 and stretch, CTurretDef::Parse (0x8D0), the VU chain's rest, SetBuffer, the vid stack's deque
    copy chain (0x3B83D0 its iterator ++).
- **Tells from the HUD, round 22** (zui_2d.cpp's renderable strings and ZAnim commands; C2DFont's real layout):
  - with a secondary base MW lays the vtable out as: the primary base's slots, the secondary base's part, the
    class's own new virtuals, then the primary slot of an override of a secondary-base virtual -- calls through
    the primary vptr reach past the secondary part (CRenderableString: Update +0xA0, UpdateUIVar +0xA4); SOCOM 1's
    vtable names had slid one class (0x379C50/0x379B10 are CWrappedText's, CRenderableString's own parser and tick
    0x36FE40/0x36FCC0);
  - `v /= 100.0f` on a float local keeps the quotient in the dividend's register, `x / 100.0f` does not; the
    compare chain's `li v0,1` can serve a case's `return true` when cases return directly, a bool local gives
    `daddiu` and an `andi 0xff` on the base call's result; `!(a <= (float)fabs(d))` gives the target's c.le/bc1t,
    `fabs(d) < a` c.lt with the other polarity; a member loaded before a virtual call and kept in a saved register
    (`lines * GetSpacing()`'s left operand) wants its own local;
  - C2DFont is 0x64 bytes (src/gamez/ztwod/c2dfont.h, the name ours): a map<unsigned short, C2DGlyph> at 0 (the
    glyph a 0x30-byte 16-aligned record -- a 16-aligned map value moves the tree node's data from 0xC to 0x10), a
    vector<zdb::CTexHandle *> at 0x10, SOCOM 1's scalars from 0x20, a CPnt3D at 0x48, a CSaferStr at 0x58 -- its
    constructor 0x38E590, destructor 0x369EB0 and CSkbFonts() 0x38E550 are code-exact in a probe
    (build/scratch/hud-r22/font/probe.cpp); it cannot live in ztwod.h (zui_skb.h reaches ztwod.h and defines its
    own C2DFont); 0x1FEAC0/0x1FE760/0x1FB3F0 are the glyph map's tree constructor, destroy and map();
  - leads: the radio command menu's global 0x413120 (initialiser 0x3FF4A0, constructor 0x237390, destructor
    0x237580) and a second at 0x4144F0 (destructor 0x230620), its handlers from 0x22EDB0 referenced by the menu
    builder func_002344E0; the font unit 0x359740-0x35D4xx (Unload 0x35A650, the glyph tree's clear 0x35C800 and
    swap 0x35C840, the hangul composition 0x3599D0, an int-keyed map's operator[] 0x35C6B0).
- **Tells from the kit and AI states, round 16** (ai_group.cpp written as a unit, 0x60C190-0x60EDFF):
  - a for_each whose functor inlines a member walk is cut to one level even at level 5: its iterator pieces, the
    functor call and the implicit copy for the return go out of line, their copies following the caller
    (steplist::Stop and stopper::operator());
  - a 4-byte functor zeroed with one `sw zero` has a declared constructor -- value-initialising `F()` without one
    zeroes it byte by byte in a loop; a member that is a class over a std container adds one this-test in its
    owner's destructor (~CAiGroup: 4/4/3); a byte-enum member returned or copied loads with `lb`;
  - `new T(..., f(x))` calls new first and evaluates f(x) under the null test; a walk's end taken into a local
    keeps the sentinel in a saved register, and the element read into a local before the compare puts the field
    first in the `bne` (that closed Find, open since round 10); a sibling that tail-jumps to a function shows it was
    never inlined -- pin it; a flag setter fed a global byte takes an unsigned int, not a bool;
  - 0x60C170 is seal_aiparams.cpp's; 0x60BF10 is an out-of-line copy of a bool setter on zdb::DiIntersect's byte-2
    bit 0, after its only caller 0x60BC40 in ai_target.cpp; 0x5EF9B0 is OVL2's copy of the pointer lists' deleter
    constructor (marked `ovl-copy` by the integrator: the step-list insert 0x60EBF0 matches with it);
  - open: RemoveMember 0x60CE40 (6, the entity and SetGroup's inlined `this` swapped), TickStep 0x60C220 (11, the
    walk's begin and end swapped), Start 0x60CBD0 (5: the camera flag re-normalised before it is negated),
    remove_if 0x60E9A0 (6, the README's swapped-copies list remove_if), the step list's erases 0x60EA90/0x60EB60
    (the target destroys each element before freeing it, include/list's general erase does not -- the MSL lane's);
    not written: Tick 0x60C3E0, AddMember 0x60D020, act_camera::Tick 0x60DB60 with 0x60DCF0/0x60DE30, 0x60CAD0.
- **Tells from the AI core, round 15** (FloodFill and the walker unit's list tail, ResetCells, GetLocationDelta):
  - under bottom-up inlining a call through an inline one level deeper is expanded later, so its temporaries get
    the LOWER stack slots: a wrapper fixed GetLocationDelta (its two lookups by cell) and FloodFill (pop_front as an
    inline over `erase(begin())`);
  - a cell walk taking its region as a pointer set only inside a branch (`if (!pr) { GetMapRegion(r); pr = &r; }`)
    keeps the pointer in a register and shares no loads through it -- what ResetCells (open since round 6) needed;
    Tile and FloodFill inline the same walk, and two functions sharing an inline's constant pool (0x64F9D0) inline
    the same function;
  - MW drops the last store into a never-read local array unless something reads it afterwards (a read whose value
    is unused is enough: FloodFill's direction-order table); a comparison against a member puts the field first in
    the `beq` only when it is read into a local first (ValidateCells); a local at sp+0 of a 16-byte frame is 12
    bytes or larger, a PNT2D lands at sp+8 (GetInterpolationCells);
  - `inside && cell->IsInside() ? 3 : cell->GetShadow()` must be an if/else (the ternary lays the 3 out after the
    call); making "navyseals" a string local like the other two group names fixes RdrFetch_CharacterType's first
    call's argument order; CAiMap::name::ValidateName is out of line in SOCOM II (ai.h declares it,
    ai_maptable.cpp defines it); neighbor__6CAiMap 0x64F910 is SOCOM 1's direction-step table;
  - open: CAiMap::Tile 0x536230 (0x8BC against 0x8B4: its four cell walks byte for byte, the target storing and
    reloading the flags at plain stack offsets where this build puts the field's address in a register; nine
    spellings in its comment). Leads: CreateGraph 0x52EEE0 (its unit then emits the copies 0x52F2F0/0x52F330), the
    unnamed 0x52A8C0-0x52BDF0 (the demo's ai_maps.cpp: CreateGraph, ValidateLinks, ReadMapSet, GetSporeLocation,
    the table updates, ExtendPath), 0x52C5C0 (RegisterPathPlan by place: 0x52C5B0 its three-argument wrapper).
- **Tells from the UI, round 11** (zui_skbview.cpp's toggles, sounds, bitmaps and cursor; CSkbTexLists()):
  - an int-array member is block-copied through the FPRs one array at a time: two `int[2]` give
    load-load-store-store twice, one `int[4]` loads all four first, named ints copy with lw/sw (CSkbRect is two
    int pairs);
  - `if (!HasNode(a) || !HasNode(b)) return;` through a bool inline makes the body reload the pointer, a plain
    member test keeps it in a0; `bool on = !Shown(); if (p) p->Set(on);` puts the sltu/xori/andi ahead of the null
    test; a byte-enum ternary taken into a local is chosen before the call's other ternary, left in the argument
    list after it;
  - `T *arr = obj->m_arr; &arr[i]` adds the index first, `&obj->m_arr[i]` the object first; a string member's
    size() read through a reference keeps the string's own address in a register; locals initialised from
    c_str() ahead of a call keep those addresses in saved registers; in a while loop `n++` written before the
    call lands in the call's delay slot;
  - the keyboard's pad is CPad (byte 0xD L1's state); its lists at 0x184 hold `vector<zdb::CTexHandle *>`;
    CSkbKeyProps is {x, y, w, h, CButtonSpec * 0x10, C2DButton * 0x14, CNode * 0x18}; 0x3E04D0 is a pooled
    `&C2DButton::SetActive` (slot 0x50); zui_spec.h repeats zinput.h's PAD_BUTTON/KEY_STATE enums, so zui_skb.h
    carries a CPad view behind `#ifndef ZINPUT_H` (one shared header for the enums would end it);
  - open: SkbForgetDisplay 0x396F30 (6 short: the target enters the inner loop at its bottom test through two
    jumps, every form tried a do-while), SkbTextFits 0x396D40 (13: saved registers numbered differently, its
    `return true` a bool assignment -- an inline?), SkbTypeOrBack 0x399C20 (12 with `while (HasChar(p)) { n++;
    p += CharLen(p); }`, measured not applied); CSkbFonts() 0x38E550 waits on C2DFont's real constructor
    (0x38E590: 0x64 bytes, a glyph map at 0, built through 0x1FEAC0 -- the 2D lane's).
- **Tells from the online lobby, round 11** (the Medius names respelled, the lobby's Tick, the chat callbacks, the
  friend lists, zNetConnect):
  - MSL's deque blocks hold AT LEAST 8 elements: include/deque's buf_size is `sizeof(T) * 8 <= 512 ? 512 /
    sizeof(T) : 8` -- the chat's 112-byte lines go 8 to a block (pop_front 0x2F51E0 keeps 16, the private-line
    callbacks' inlined front() divide the start by 8, where 512 / 112 would give 4); MediusChatMessage is the only
    element in the tree past 64 bytes, so the minimum moves nothing else;
  - MW's CSE treats a field read through a const view and through a non-const one as different expressions
    (Tick needed zrdr.h's const `Real()` for the float setter to reload the type byte);
  - a user-declared non-inline destructor counts as the unit's own function and never inlines an out-of-class
    template member; an implicit destructor's tail copy does (~CLobbySaveModule, ~basic_string) --
    ~CLobbySaveModule is defined in zNetMediusUniverse.cpp, inside that unit's span, by image order;
  - stack slots follow declaration order across scopes: per-case buffers declared at function scope ahead of a
    shared buffer take the top slots; a per-case base and counter, counter declared last, give base s1 and
    counter s0 in every case;
  - `if ((err = f()) != 0)` branches on v0 with the copy in the delay slot, `err = f(); if (err)` on the copy; a
    compare materialised in v0 before a ternary's branch is a bool local (`bool fits = n < 1000; fits ? a : b`);
    an initialisation the target places after a loop's guard means an explicit guard around a do-while;
  - on a class iterator `(*i).f` and `i->f` schedule a call's arguments differently; one `&&` chain with both
    event calls in else-arms lets the inline event's null test branch straight to the epilogue, nested ifs
    with `return` leave a jump block (AddFriend); a ternary passed to a `char`/`unsigned char` parameter adds
    an extension, to an `int` parameter it matches (cbFilterChat, the server-message callback);
  - open: func_0030BD40 (2: the counter's zero ahead of the loop guard), the lobby's Open 0x2E0030 (0x410,
    unwritten: CZOnlineLobby::Open(host, port), called from UILobbyConnect), the deque instances whose callers
    are all written now (their sizes against include/deque in the MSL queue), the IgnoreItem class insert
    0x3022F0 (0x78C against 0x554); func_00300740 is declared bool in both callers' units, as defined.
- **Tells from the weapons, round 13** (vis_vu.cpp's VU chain, GetChainData, the vid stack):
  - SOCOM II's vid stack is `std::stack<void *>`: its out-of-line empty() (`xor v0,v0,zero; sltiu`) and top()
    (the deque's back() over 128-element blocks inlined) match 0x321F40/0x321EF0, renamed by the integrator;
    znode.h forward-declares CVidStack and vis_vu.cpp/vis_main.cpp define it -- <stack> cannot go in znode.h
    because zrdr.h defines a std::stack of its own and the two clash; GetVid is six inline levels high, so
    its callers inline it one level;
  - MW refuses `break` as an asm mnemonic: the intrinsic is `__I_break(0)`, and the game's trap is
    `__I_break(0); __I_nop();` -- inlined one level it pads `break; nop; nop`, inlined whole `break; nop`;
  - a command helper that returns `p + 1` lets a caller step a cursor whose address is taken without
    reloading it after the helper's stores;
  - `!(p + n >= end)` through an inline bool gives `sltu; xori; xori; bnez`, written inline as
    `!(p + n < end)` it branches on the sltu; an int flag is `li` stored unmasked, a bool flag `daddiu` plus
    `andi 0xff`; a bool result set at the top of its if block (`r = true;` before the work, else `r = false`)
    puts the daddiu ahead of the loop; a 1-bit field read is `dsll32`/`dsrl32` where a mask is `andi`;
  - a 15-statement function in the same unit is auto-inlined into its caller (VuProjectedMap needed `#pragma
    auto_inline off`); the detail struct is SOCOM 1's zdb::tag_DETAIL_PARAMS (CVisual keeps its DETAIL typedef);
  - VuUpdate's tail calls even 8-byte helpers out of line (0x3B5250-0x3B52C0 sit right after it): its inline
    budget runs out partway -- a THIRD limit, probably `inline_max_total_size`, not measured yet;
  - open: VuLightMap 0x3B54E0 (5: the loop test's end() colours the size and the iterator word the other way
    round), CopyBuffer 0x3C36D0 (8 bytes long: the switch's second test `beq 8; b out` where the target
    branches straight into kind 8's code, the block copy's two loads ahead of the length); not written:
    VuUpdate, 0x3B5F20 (VuBatchProgram's command list), 0x3B41D0, 0x3B6870, 0x3B6B20, 0x3B7170, 0x3B76B0,
    0x3B78D0, SetBuffer, the vid stack's deque copy chain.
- **Tells from the world, rdr and sound, round 3** (zrndr_pipe.cpp on the real texture classes, zrndr_postfx.cpp):
  - a function template DECLARED ahead of its definition (`template <class V> inline void FreeVector(V &);`)
    is no longer inlined one level, even at calls written after the definition (ztex_manager.cpp's four
    FreeVector users all missed): include the defining header instead; and a template the header's inline
    bodies call must be defined before them, or their copies call it out of line;
  - in `a = b = x;` the store to `b` comes first (`CAlpha()` needed `m_next = m_visualList = ...`); MW keeps
    the source order of constant stores to `this` (`CPipeEffect()`'s sixteen matched only in the target's);
  - a header inline whose tree is taller than the level, used by two units (`CPipe::Flush`,
    `CGameMenu::Clear`), is inlined one level in both, and its callees' copies sit after the FIRST caller in
    link order; a unit's statics defined in its initialiser's order (`<iostream>`'s first) make its
    `__sinit` byte-exact (`__sinit_zrndr_pipe.cpp`, `__sinit_zrndr_postfx.cpp`);
  - zrndr_pipe.cpp ends at 0x33CD4F (its template tail 0x33C2A0-0x33CCCF, ~CFlareSystem 0x33CCD0 last);
    0x33CD50-0x33D22F is a unit of its own, SOCOM II's effect at thePipe+0x688 (named zrndr_postfx.cpp and
    CPipeEffect by us), whose initialiser 0x402F80 follows the pipe's; zsav_module.cpp starts at 0x33D230;
    the pipe interrupt's test at 0x1C is the packet vector's capacity;
  - 0x1FB3D0/0x1FB300 are vector<CTexHandle *>'s constructor and destructor, not vector<CAssetLib *>'s (the
    font code 0x35A650 builds and destroys FreeVector's temporary through them beside the pipe's own clear
    and swap copies; Add2D, the old names' basis, inlines its CLibList's) and 0x1FDDA0 vector<CTexPalette
    *>'s destructor -- renamed by the integrator, which made CDynTexList::Free 0x33BAA0 byte-exact;
  - open: the delayed nodes' class insert 0x33C5C0 (109, registers only: the in-place end pointer in s6
    where the target has s0 -- include/vector's class-insert end register, the MSL lane's); 0x1EA580, a
    pointer list's clear() out of line in an unwritten unit's tail, is unnamed (CAlpha::Free calls it).
- **Tells from the weapons, round 12** (vis_texfx.cpp whole, a new unit vis_ripple.cpp, GetTextureSelectBlock):
  - vis_ripple.cpp (0x3C0CF0-0x3C356F, its initialiser 0x404B30; vis_main.cpp starts at 0x3C3570) is
    SOCOM II's water ripples, with no SOCOM 1 counterpart: a manager global (theWaterRippleManager
    0x4B5170) that CCoreState::Tick ticks and the ZAnim RIPPLE/RIPPLE_NODE_GROUP handlers feed -- its
    class, function and global names are ours; vis_texfx.cpp is 0x3BFB60-0x3C0CEF, and RTPool is
    global-namespace (SOCOM 1's `26RTPool<...>`, not `Q23zdb`);
  - a bool result compared `!= false` (or `== false`) gets `andi 0xff` before the branch, `== true`
    compares against 1 (Update, the manager's Tick); an inline `bool Lifts() { return m_bit; }` tested
    `!= false` turns the bit into a bool with `sltu` first, tested plainly it folds away;
  - a point written and read through a `float *` set before a call keeps its stores and reloads the
    other pointer's fields after each store (SetLevel); through a `CPnt3D *` parameter it lives in
    registers;
  - GetBBox passed as the argument of an inline overlap test is inlined one level, its recursion left a
    call; called inside an inline that also stores, it is called whole;
  - `push_back(this)` in a constructor forces `this` into memory; through an inline `Add(T *r)` the copy
    is a parameter of its own and `this` stays in a register;
  - erasing through an inline wrapper creates the erase's temporaries after an inlined clear()'s, the
    target's stack-slot order -- a lead that MSL's list<T *> has an inline erase(iterator);
  - a callee's float/pointer parameter interleaving shows in the caller's argument set-up: a1, a2, f12,
    a3, f13, f14 is (ptr, ptr, float, ptr, float, float) (the ripple constructors' signatures);
  - `CPnt3D d; d = a - b;` makes two copies (ld/sd, then lwc1/swc1), initialising makes one; `dist *= k;
    dist += c;` keeps the sum in dist's own register before a `cosf` call, one expression computes it
    straight into $f12;
  - a nearest-so-far test is two arms with one body, `if (!best) {X} else if (d < min) {X}`: `!best ||
    d < min` lets MW merge the arms; `(x pair) && (z pair)` branches past both z tests at once when the x
    pair fails;
  - include/list gains the sized members (resize, insert(n), init(n), advance_to, splice) and list<T
    *>::resize(n, x = 0), additive;
  - open: the list's resize in vis_texfx.cpp (7: the inlined erase's two node registers swapped) and
    insert(position, n, x) (the temporary's destructor inlined where the target calls ~__list_deleter);
    Gather 0x3C3120 (3, a delay slot around the live-ripple test); ClearGroups 0x3C2A10 (0xD8 against
    0xF8: the target inlines the group's whole destructor at the delete, six levels high here with the
    node vector's destructor alone five, so MW calls clear() and the vector destructor -- another
    inline-depth witness, "Open, and next"); Flatten 0x3C0CF0 (0x260 against 0x274: the target
    multiplies 32767.5 by 1.0 and 0.0 at run time); Ripple 0x3C2CB0 (57: the target keeps `this` in s8
    and reaches the vertex through one pointer register).
- **Tells from the AI core, round 14** (CCharacterType's Open, Parse, constructor, InitMultiplayerMessages):
  - an inline setter taking a class BY VALUE builds the parameter on the stack (four floats, then two) and
    assigns member by member; a plain assignment or a returned copy is scalarised (the constructor's
    SetVoiceFont);
  - a float array copied into a CPnt3D member as `*(CPnt3D *)arr` loads the owner once and reads elements
    1-2 through computed addresses; field-by-field stores reload the owner before each store (Open's gear
    offset and rotation);
  - `return t == 3 ? strcmp(a, b) == 0 : false;` in an inline bool gives `xor v0,v0,zero; sltiu; andi
    0xff` before the branch, the if/else and `&&` forms do not (Open's ':' test); `state = p ? 1 :
    -state;` leaves the join unpadded where the if/else pads it;
  - a count read through a reference-taking min is reloaded; min's result taken into a reference before
    `T *p = data_` keeps the copy loop unrolled 8x, `p` declared first does not (the class assign);
  - -inline auto inlined the 12-statement AddAnimSetName into Parse although its push_back calls an
    out-of-line insert: it needs `#pragma auto_inline off` (round 13's "the insert keeps it out" was
    never measured);
  - SOCOM II's HEALTH_PARAMS is three float[6] arrays (health, armour, maxima; the implicit assignment
    copies each four then two); zchar.h's MultiplayerMsg is SOCOM 1's DWARF layout; include/vector gains,
    additively, the class `__vector_imp`'s operator=, its out-of-class range assign and a
    reference-taking `__vector_min`;
  - OVL2 copies marked by words and calls: 0x540D00 of the template replace 0x1EE780 (the SEALs' string
    constructor copy 0x58CE70 calls it at +0x54 where OVL1's 0x2396F0 calls 0x1EE780 -- 0x58CE70 matches
    with the mark), 0x541240 of 0x1EF400, 0x528C30/0x529030 of the 4-byte pod's operator=/assign;
  - NAMING, open: the class and pod range assigns are spelled `assign` (include/vector, row 0x259D20,
    0x53F370, 0x53F730), but SOCOM 1's symtab calls every one of them `do_assign` (`do_assign<PC3LOD>`
    0x24B2E0, `do_assign<PC14CCharacterWeap>` 0x1D7700, the pod's `do_assign<PCUi>` 0x1FC880) -- the MSL
    headers' next round renames them;
  - open: the LODs' class insert 0x540410 (0x590 against 0x500: the target calls the reallocation
    temporary's destructor 0x540A50 out of line, ours inlines it -- the class inserts' generation-order
    question); the LOD deleter's clear 0x540990 and destructor 0x540A50 are emitted but miss (our clear
    calls the string destructor out of line where the target inlines it; both rows stay matched as drafts).
- **Tells from the SEALs, round 14** (CEntity's slots 0x5C/0x64, seal_ai.cpp's small rows, seal_cfg.cpp's lookups):
  - A RECURSIVE INLINE'S OUT-OF-LINE COPY INLINES ITSELF EXACTLY AS MANY TIMES AS THE INLINE DEPTH (7, 8
    and 9 give that many copies): CSealUnit::GetTop's copy (0x561D20) shows eight, so seal_ai.cpp carries
    `#pragma inline_depth(8)` (the state at the unit's end acts); every function the unit matched at 5
    holds at 8, as does every unit including zseal.h, where 8 also matches veh_vehicle.cpp's copy of
    zdb::CNode::GetBBox (0x262550, the other self-recursive inline, open at 5) -- evidence that the
    original's level is 8 with deeper MSL headers ("Open, and next");
  - `return s ? InlineBool(s) : false;` puts the `andi 0xff` on the true arm (0x559000); a byte-enum field
    read into a local keeps an inlined `&&` as branches, compared straight MW if-converts it;
  - a float constant read from data for a compare is a variable too (190.0f at 0x64FE08 in 0x561B90);
  - a member factor passed as an inline's argument is loaded ahead of the inline's test (0x572240);
  - a callee inlined only one level calls every one of its own callees out of line, trivial accessors
    included (AnimSet's name accessor from 0x58C4B0 and 0x5784A0): 0x5788C0/0x578920 inline AnimSet::Find
    one level with nine iterator and name pieces out of line, where our include/vector's Find is 4 high
    and inlines whole -- an MSL depth question;
  - CEntity's slot 0x5C is `bool (CZSealBody *, CTurret *)` (0x5A88C0 calls it with the SEAL and the entity
    when it is a turret) and 0x64 SOCOM 1's GetFirepointNode(const char *), virtual in SOCOM II;
    zbody.h's siNumBodyObjects is `unsigned int` (si_script.cpp's definition with it); zseal.h's members
    split out of padding: CZSealBody's m_carry 0xEAC (SOCOM 1's 0xDA8), CSealUnit's m_parent 0x60 (0x3C);
  - open: 0x572240 (0xBC against 0xC4: the target reads the range at 0x6F4 again for the product, seven
    shapes in its OPEN note); 0x58CE70, the string constructor's OVL2 copy, misses only its call to
    0x540D00 until that row carries `ovl-copy: func_001EE780` (the AI core's round 14 marks it);
    0x5781C0-0x579D9F is unclaimed, probably seal_body.cpp's head.
- **Tells from the world, rdr and sound, round 2** (ztex_manager.cpp written whole on SOCOM 1's classes,
  SetupSpecialTextures):
  - MW's scheduler reorders independent float stores to globals: TexBufParams' four defaults written
    0-1-2-3 land 0-1-3-2 in the image, and writing them in the image's order misses by 18 (CTexManager());
  - a `u_long128 *` cursor stepped `p++` per quadword gets one `addiu +32` after each pair of quadwords;
    `p += 2` after four doubleword stores folds every step into the store offsets;
  - libgraph's `SCE_GIF_SET_TAG` builds its constant once and two tags in one function share it; a
    hand-folded `x | 0x8000 | (2 << 58)` ORs the low part into x before widening it to 64 bits;
  - `x > K` and `i <= 13` compare into `at`; `x >= K+1` and `i < 14` compare into v0;
  - for a function's first store to go through a0, declare the next local after that store; declared
    before it, the store goes through the saved copy of `this`;
  - a materialised `a.empty() && b.empty()` is a bool local, not a test written inside the `if` (Load);
  - a value pushed by reference is stored to the stack where it is set; a copy made at the push
    (`T *p = t; push_back(p)`) is stored only at the call (Hookup);
  - a select's register-address word is written before its data word (SetupSpecialTextures); an Exists
    with a local found iterator stores each temporary next to its copy (CDynTexList::Add(CTexPalette *));
  - ztex_main.cpp ends at 0x35597F; ztex_manager.cpp runs 0x355980-0x357E8F (CBatchRelocator's slots at
    its head stay drafts), moved from ztwod/ to src/gamez/ztex/ and written on znode.h's CTexture and
    node_world.h's real CGSTexBuffer/CTexManager (the HUD lane's stand-in classes are gone); MakeChain
    takes no count (`FP1`, not `FP1i`: SOCOM II's Load never sets a2 and the body never reads it);
    0x356B90's s1:align name `swap` was wrong (ReservePktBuf);
  - open: UpdateTextureSelects 0x355DE0 (11, registers only: both loop-end tests swap a0/a1, and the
    palette test wants its operands in one order for the loads and the other for the branch); Hookup
    0x3571D0 (42, registers only: this s3, lib s2, tex s1, i s0 against the target's s2/s1/s0/s4; six
    declaration orders change nothing). The lane's scratch (callers.py, the caller scanner; open.py) is
    build/scratch/wld-r2/.
- **Tells from the kit and AI states, round 15** (seal_ik.cpp, ~CSealUnit, the aim parameters):
  - single-bit `unsigned int` bitfields written one at a time compile to lbu/sb: splitting the 3-bit
    field was enough for TurnSetup 0x5B8270 (round 14's unsigned-char guess was not needed);
  - an unsigned byte-enum member passed straight to a parameter of the same enum loads with `lb`; cast
    to its own type, `(E)m`, it loads with `lbu`;
  - member lists that are classes over std::list (SOCOM 1's slist/ulist/clist) give four this-tests per
    member in an implicit destructor, as ~CAiSAnimate;
  - `if (p) {...} else {...; return false;} return f();` puts the shared call's block last
    (SetAimPointW); returning early inside the if puts it right after the if body;
  - a recursive function's out-of-line iterator copies sit BEFORE it in the image (0x5ED880-0x5ED9A0
    precede GetSealCount, as 0x2D22E0 precedes CGTree::Destroy); GetSealCount (0x1A8 vs 0x1F8) waits
    on include/list's depth: the pointer-list wrapper's begin()/end() as `iterator(base::begin())`
    and a two-level size() (0x5E8A80 a const reference to size_, SOCOM 1's compressed_pair first());
  - seal_ik.cpp is 0x5B8270-0x5B82CF, SOCOM 1's seal_ik.cpp position between seal_ex.cpp and
    seal_kit.cpp; 0x5EBDC0 is CAiState's implicit operator= (the stack's 0xC-0x1B copied as one block
    through the FPU: a nested struct), its list assignment an MSL chain include/list does not model
    (0x5EF7F0, 0x5EF470, 0x5EF9B0, 0x5EF8D0, 0x5EF950, 0x510110 -- seal_unit.cpp's header has it).
- **Tells from the world, rdr and sound, round 1** (ztex_main.cpp, zrdr_file.cpp, CSndVocalQueue::Add):
  - bitfields set in a constructor's initialiser list compile to one word read-mask-write each, with
    `(0 & mask) << shift` left unfolded; assigned in the body they fold to byte stores (CRdrFile());
  - a member read through an inline `const` accessor is reloaded after the caller's own test of it;
    `T *r = 0; if (m) r = ...` puts the zero in the test's delay slot (Create, Resolve);
  - through a bool setter `true` folds `(1 & 1) << n` to a daddiu constant while `false` stays masked
    at run time; a 64-bit bitfield narrowed through `(int)` is sign-extended before a halfword store
    (Read's palette id); an inline called with a constant null keeps its dead arm behind an
    unconditional branch (~CTexture's SetName(0)); the last arm of an if/else-if chain written as a
    ternary saves two words (po2);
  - STAND-INS, the original's spelling unknown: SetGifPtrs, CTexture::Read and CSndVocalQueue::Add
    match only under a function-scoped `#pragma opt_propagation off` (a constant kept in a register,
    a forwarded copy kept in memory; every plain form tried was 3-5 instructions off; unit-wide it
    loses others, and swept over 14 units with open functions only Add gained -- five other optimiser
    pragmas gained nothing: build/scratch/wld-r1/propsweep.py); Resolve needs a `volatile` store to keep
    the flags' unchanged write-back;
  - ztex_main.cpp (the press beta's assert names it) starts at 0x354510, straight after valve_main.cpp,
    in SOCOM 1's order; its boundary with ztex_manager.cpp falls in 0x355A70-0x355DE0; znode.h's
    CTexture is 0x6C bytes (m_MIP2, m_reverse_miplist, m_loadimage); the HUD lane's ztwod/ztex_manager.cpp
    still carries its own CTexture (m_addr = m_gsaddr, m_name = m_AssetLib, m_palid = m_palID, m_size =
    m_gsWords, m_mips = m_reverse_miplist).
- **Tells from the AI core, round 13** (ai_perception.cpp, char_type.cpp's sounds, LODs and references):
  - the no-op `lw; dsll32 0; dsrl32 0; sw` before a bit set is CAiPath::SetFlags' `x &= ~clear; x |= set`
    called with a clear of 0 (the scan 0x60AA20 does it six times);
  - a lerp `lo*(1-x) + hi*x` read straight off a table element swaps the madd operands; reading lo and
    hi through a `const T &` element reference keeps the target's order (the scale through the same
    reference moves its load);
  - `Item((unsigned int)i)` on Item(int) stops pointer stepping; `Item((unsigned int)(i + 1))` computes
    i+1 after the count, `Item(i + 1)` before it;
  - `n += p ? strlen(p) : 0` leaves both joins unpadded, the if/else form pads them as the target
    does; `played = true;` written before a call fills the jal's delay slot and orders the float
    constants f12 first, f13 last; a float field taken into a local hoists its load above nested
    tests;
  - ai_perception.cpp runs 0x53ABE0-0x53B00B (its .data 0x64FA70-0x64FC3C between the walker unit's
    and char_type.cpp's); the voice-font getters 0x53B010-0x53B06F close either it or char_type.cpp
    (identical bytes); 0x53C940 is SOCOM 1's CCharacterGear::CreateInstance (same body, same place).
- **Tells from the online lobby, round 10** (nine units' static initialisers, the lobby's state queue):
  - defining a unit's globals in order gave seven of the lobby units' `__sinit_` byte-exact at once
    (zNetMedius, Room, Clan, Ladder, Lobby, Friends, LANLobby; Chat and Dme followed);
  - a global whose registered destructor is not its vtable's destructor is a member of an outer
    object: two this-tests, the member's destructor inlined one level (theAcctDB inside
    CZUniversInformationServer -- level-6 evidence, "Open, and next");
  - an initialiser calling a queue's tiny members out of line means the global's constructor was cut
    to one level by its size; `#pragma inline_max_size` acts through its state at the unit's END
    (the chat unit sets 100 for the whole unit) and dead code adds no size;
  - a derived list whose destructor body is `clear();` stores the size in the first this-test's
    delay slot; the deleter's clear inlines into a list's destructor only once the unit's own code
    calls `list.clear()` (which calls the deleter's clear out of line);
  - a vector's resize calling memset for its blank element means the element has a memset
    constructor; returning a member unconverted wants it declared `bool` (`unsigned char` adds an
    sltu); `p[i + k]` through a per-case local pointer stays base-plus-index, while `bytes = p + k`
    is strength-reduced into a walking pointer;
  - 0x2F62B0 is the chat unit's tail, so zNetMediusLobby.cpp starts at 0x2F6340 (corrected by round 11: the chat
    deque's instances run to 0x2F6CF7 -- 0x2F6340, 0x2F6600, 0x2F6A70, 0x2F6BC0 are called only from them --, so
    zNetMediusLobby.cpp starts at 0x2F6D00).
- **Tells from the HUD, round 21** (st_mpexit.cpp written whole, reco.cpp's gsapi calls by name):
  - a switch's compare chain runs its labels in reverse source order and lays the blocks out in
    source order: Tick's target order 1, 3, 5, 6, 4 is `case 4: break; case 6; case 5: case 3; case 1;`
    with the shared code after the switch;
  - a value loaded before the field it is added to came from an inline accessor (UpdateCareerRecord,
    30 instructions to a match); one load of a global pointer serving several int stores is an
    inline member on that object (written out, the global is reloaded after every store);
  - a reference local declared before a call keeps the member's address in a saved register, declared
    after it the offset folds into each access; a list walk keeping its begin and end temporaries in
    the frame is a const_iterator assigned from a const view's begin();
  - `bool b = false; int i;` and the reverse order in one block give different saved registers
    (ComputeNewRating, 15 to 4); a bitfield cleared with an unfolded `andi v1,zero,1` comes from an
    inline function's bool parameter; unused parameters show only at the call site;
  - a record's size can be read from two stack frames: CareerRecord is 0xE8 bytes (fts_net_persona.cpp's
    LobbyPlayerInfo likely 0xE8, not 0xF0); 0x22EA50-0x22EDA0 (CShutdownState's and CRebootState's
    constructors, tables 0x405AE0/0x405B10) is a game-states unit inside the HUD range.
- **Tells from the kit and AI states, round 14** (seal_unit.cpp, SetAiEvent, CalcAimGoal):
  - a bare float test `len ? a / len : 1.0f` gives `c.eq.s len,0`; `len != 0.0f` swaps the operands;
  - a helper with a function-local static inlined into its only caller wants `static inline`: `static`
    alone still emits a copy, `inline` alone makes the static external (compared by name, not content);
  - `r = 0; if (inlineBool(...)) r = v;` keeps the result in a saved register across the call, the
    ternary does not; `s = (E)m; if (m == 3) s = f();` puts the narrowing in the compare's delay slot;
  - grouped cases listed 3, 4, 0 then 1, 2 give the compare chain 2, 1, 0, 4, 3; a recursive
    auto-inline candidate (15 statements or fewer) inlines itself unless pinned;
  - 0x5E6C90's second walk tests against begin() -- an original bug (`beq s3,s3`), kept;
  - zseal.h includes ai/ai.h (CSealCtrlAi::SetAiEvent returns the event kind) and CalcGoalDir returns
    bool (CalcAimGoal keeps its result).
- **Tells from the SEALs, round 13** (small rows: CAiSAnimate's list instances, CAiSGrenade::OnSignal, the blender):
  - MW 3.0b52 refuses an explicit instantiation of a class template's member function ("illegal
    explicit template specialization"); `template class X<T>;` compiles but emits every member, so
    list instances need their real callers;
  - a class deriving from std::list<T> adds one this-test level to an inlined member destructor (four
    tests, not three: ~CAiSAnimate); a stand-in for an out-of-class template member's copy must be
    pinned with `#pragma auto_inline off`, or the unit's own callers inline it;
  - `unsigned int` bitfields read or written one at a time come out as `lbu`/`sb` (CEntity's
    m_isAlive, the 0x264 bits) -- CPickup's lw/sw is not universal;
  - `bool b = !on && bit;` computes `!on` (sltu/xori) before branching; `if (!on && bit) b = true;`
    branches on the register (0x561E20); a global loop bound's load is never hoisted out of a loop
    storing through a pointer, even `const` (so AnimPop's count in a3 is something else);
  - seal_ai.cpp's template tail runs to 0x576117 (CAiSAnimate's list instances); seal_anim.cpp starts
    at 0x576120 (CGame::StopEngine calls it where SOCOM 1 called ~CSealAnim); OVL1's
    siNumBodyObjects (0x3DE1E8) behaves as `unsigned int` in SOCOM II.
- **Tells from the AI core, round 12** (ai_mapio.cpp's tail, vehicle_rdr.cpp's entry, char_type.cpp's weapon list):
  - a class vector's resize matches through include/vector once the element's default constructor is
    right: body assignments give byte stores in statement order, bitfields in a mem-initializer list
    word read-mask-writes;
  - a placement new followed by a call in an insert is MSL's construct(), not an assignment: with a
    user-declared constructor the implicit copy constructor goes out of line -- the vehicle entry's
    class insert 0x534630 matches that way (a lead for the class inserts' end register);
  - `(EQUIP_ITEM)*(const char *)&w.m_wepId` lets remove_copy_if share one `lb` between the compare and
    the element copy; a function too long for -inline auto inlines once its body goes through an
    inline accessor (`Item(i)->Int()`, ParseTeams);
  - an `unsigned` loop counter through `operator[](unsigned)` is strength-reduced, an `int` counter
    keeps the per-pass index; `if (p != 0)` branches directly where a bare `if (p)` in a for/break
    search is materialised first; a name-table loop indexes from the table's start (`names[i]`, i =
    1..2);
  - char_type.cpp starts at 0x53B070 at the latest (InsertWeapon inlines SetupCharacterWeapon);
    ai_mapio.cpp's tail runs 0x51B6C0-~0x52416F; 0x534D40 is the entry's copy constructor, not `__as__`.
- **Tells from the UI, round 10** (the keyboard's vector<char> instances and assign(s, n) through their callers):
  - an end iterator copied to a stack slot, then read with the step folded into the access (`lhu
    -4`), is an iterator passed by value to an inline (zui_skb.h's SkbPrev): every back()/end()-1
    form scalarises it;
  - an element built directly in push_back's temporary, its field addresses hoisted out of the loop,
    is `push_back(T(a, b))`; a helper returning T by value adds a copy; a pooled true/false constant
    read with `lbu` into a hidden result is `std::pair<bool, bool>(...)`'s const-reference constructor;
  - a materialised `!x` (sltu, xori) inside an `&&` needs an inline `return !(bool)x;` -- the cast in
    the condition is simplified away; a byte-enum is assigned unnarrowed (movz) and narrowed with
    `andi 0xff` at each read, an unsigned char at the assignment;
  - `return n >= 2 ? (unsigned short)x : 0;` gives `slti v0`, `bnez` and an `andi 0xffff` at the join;
    `(T *)(buf + i * sizeof(T))` adds `this` first, `(T *)buf + i` the index first;
  - include/string has `basic_string(const charT *, size_type)`, the char vector pop_back(); the
    hangul element vectors' destructor chain is one level deeper in the original than these headers
    (six this-tests inlined there) -- about 15 hangul rows wait on it (the MSL lane's).
- **Tells from the game states, round 18** (two initialisers, the controller's stick and button readers):
  - an array static of a class with an inline empty constructor gives an `__sinit` that tail-calls
    `__construct_array` with the constructor's out-of-line copy and no destructor (fts_message.cpp);
  - a static's registered destructor inlines its whole chain only if nothing in the unit's own code
    requested its out-of-line copy first -- FreeVector's temporary does (st_core.cpp's
    ~vector<CMovieCue>, 0x1EF8C0, a draft's match);
  - `bool b = !A() || !B();` over inline bools materialises both negations, `daddiu b,zero,1` in the
    first branch's delay slot; a switch case assigning a ternary joins the arms before its break;
  - `return c ? -x : x;` leaves no nop before `jr ra`, `if (c) x = -x; return x;` does; chained early
    returns ending in `return 0.0f` land the later branch on the zero's padding nop (a result local
    avoids it);
  - the 0x437Dxx words are fields of theMission (0x4364E0): 0x437D08 is CMission::m_subtitles
    (+0x1828); 0x1E91C0-0x1EB06F matches SOCOM 1's database.cpp (testLoad, testUnload), unwritten.
- **Tells from the weapons, round 11** (vis_data.cpp, CZMarkList::AddNewMark):
  - a user-declared out-of-line empty destructor keeps a band's member list teardown at its
    definition point, and -inline auto then refuses it to the vector's clear (~CPreLight_band);
  - a constant-initialised function-local `static float` is read with `lwc1` from data, a literal
    is built with `lui`/`ori` (ColConvTo3D); storing a constant in a member and reading it back stops
    constant folding, an inline parameter is folded;
  - `a >= b` materialises into a register where `b <= a` uses `at`; `m_index > m_first` loads m_first
    first; `!found & (q <= m_end)` (bitwise) gives a computed-and-ANDed loop test, `&&` branches;
  - an inline helper's parameter order sets its argument load order and its FPRs (`RandRange(hi,
    lo)`); `k * rand()` held in a local fixes the multiply's operand order; `(n << 4) + (unsigned
    int)p` puts the scaled index first in the `addu`, every pointer form the base;
  - a `return;` at the end of the last if-block emits the target's `b epilogue; nop`; `CPnt3D v = a -
    b;` puts operator-'s temporary below the named copy; `m_a = m_b = x` stores m_b first; a copy
    constructor `{ *this = b; }` makes the vector insert construct through `if (p) p->operator=(x)`;
  - vis_data.cpp runs 0x3BA300-0x3BD99F (SOCOM 1's 0x310460-0x312DDF; no assert names it);
    znode.h's *_Object stubs are SOCOM 1's DWARF shape (a std::vector<band> plus m_bandMax).
- **Tells from the HUD, round 20** (reco.cpp's Load/Tick/callbacks, __sinit_hud_mpweaponsel.cpp, zui_2d.cpp):
  - MW knows what an already-generated function of the same unit clobbers: Load keeps `lang` in a0
    across a call to a pinned lookup defined after it (generated first); the target's second call
    sets no argument at all;
  - a jump-table switch needs every case of the enum's range written, empty ones included -- with
    only the two real cases it compiles to a compare chain; the case blocks lie in source order;
  - under `-enum min` a `-1` enumerator makes the enum a signed byte: SOCOM 1's 4-byte
    `ALIGNMENT_HACK = -1` enums need 0xFFFFFFFF or 0x7FFFFFFF;
  - `g.m_v.push_back(x)` on a global folds to absolute addresses; the target's loads through the
    vector's own address come from an accessor returning a reference (`g.GetPath().push_back(x)`);
  - as an `&&`'s last test, unsigned `n > 2` branches on `at`, `n >= 3` materialises into v0;
  - SOCOM 1's vtable names in the C2D family can be one slot off: check them against ztwod.h's slot
    order (0x36E6B0 is SetNormal, +0x54; SetCanHandleInput is 0x377380, +0x4C).
- **Tells from the kit and AI states, round 13** (seal_unit.cpp, SetAimDirR, CTPaint::Tick, seal_aievent.cpp):
  - a bool masked in v0 (`andi v0,v0,0xff`) live to the epilogue is the function's return value:
    SetAimDirR returns its callee's bool, which SOCOM 1's mangled name never showed;
  - passing a member's address as `(T *)((char *)this + K)` stops MW keeping it in a saved register
    for later stores to that member (the existing tell, confirmed for stores);
  - a single-element list walk `if (!found) { walk; if (!found && flag) walk; }` gives the target's
    jump straight to the return after the first loop; two separate ifs do not (IsMemberOfUnit);
  - a pointer-list member constructed in an out-of-line constructor calls OVL2's deleter-constructor
    copy (0x50FFA0) out of line; a `list<FT_COMMAND>` member makes the unit emit its own copy;
  - units: seal_kit.cpp starts at ~0x5B82D0 (SOCOM II's CZKit additions at the source's end);
    seal_aievent.cpp runs 0x5F8D40-0x5FADEF in SOCOM 1's order.
- **Tells from the SEALs, round 12** (seal_env.cpp's look, blink and aim functions):
  - a CQuat copied into a pointed-to member through an inline taking it by const reference gives the
    target's registers; the plain assignment swaps them (ApplyBlinkAnim, ApplyEyeLookAnim);
  - a local that looks unneeded can be one SOCOM 1 passed twice: SetEyeLookDir's `cur` needs its own
    saved register; a float test's arm order follows the source (`if (x >= 0.0f) A else B` puts A on
    the fall-through);
  - when every true test of an `||` chain must reach one `return true` block, fold the last flag test
    into the same condition -- a second `if` makes its own block (IsThrowing);
  - `int top = m_count - 1; m_frames[top]` keeps the subtraction as an index; inline, the -1 folds
    into the offset (AnimPop);
  - SOCOM II holds SOCOM 1's heap looks inside the SEAL: SOCOM 1's "pointer member + 0x10" becomes
    `this + K`; 0x575CF0 (a list range-erase instance) looks like seal_ai.cpp's template tail, not
    seal_anim.cpp's first row.
- **Tells from the UI, round 9** (zui_skb.h, zui_skbview.cpp, zui_skbkeys.cpp):
  - an RTTI class-name string shows the namespace: SOCOM II's "CBmpWithNode" (0x3FAEA8) lacks SOCOM 1's
    `@unnamed@zui_skb_cpp@::` prefix -- the keyboard's classes come from a shared header (zui_skb.h);
  - a static of a class holding `U m[n]` registers that class's implicit `(this, flag)` destructor;
    a static `T[n]` gets a bare `__arraydtor$N`, a `T[4][10]` is flattened with no per-row helpers;
  - a virtual call on a static CRenderableString goes through the table (no devirtualisation);
    `on ? 1 : 0` into an `unsigned char` folds to the bool, a byte-enum keeps `li`/`movz`; a float
    taken into a local before a call fixes `add.s`'s operand order;
  - units: zui_hangul.cpp is 0x3903A0-0x3941CF (its tail ends on the two vector destructors its
    initialiser 0x403770 registers), zui_skbview.cpp runs to ~0x398ADF, zui_skbkeys.cpp ~0x398AE0-
    0x39BBBF (it inlines none of its own small wrappers).
- **Tells from the online lobby, round 9** (zNetMediusClan.cpp complete, the lobby's callbacks):
  - an unsigned operand moves v0/v1: StoredClanInfo's leader id unsigned closed IsLeaderOfClan (open
    since round 1); an unsigned stat addend gives the target's registers where int operands swap them;
  - a counter that must survive a call is assigned before it (`int n = 8;` ahead of std::sort lands
    `li s0,8` in the call's delay slot); a switch with only `case 0` and `default` gives `beqz` plus
    `b` with the zero case first, where a ternary gives a select;
  - walking `_zrdr::Item(i)` items, test `line->type == 3` on the bit-field directly, sharing one load
    with ZString's test; `strlen` into an int local before an int sum keeps the target's `addu` order;
  - a struct field the target reads past where the header ends shows a missing SDK field
    (MediusGetClanInvitationsSentResponse's ResponseTime before EndOfList).
- **Tells from the kit and AI states, round 12** (anim.cpp's name lookups, seal_kit.cpp's items and fire modes):
  - a switch's compare chain tests its cases in reverse source order: list them 2, 1, 0 for the
    target's 0, 1, 2 chain; an empty `case 0: break;` keeps its own test;
  - `bit ? 1 : 0` folds to `sltu` whatever its type; the target's `li 1; movz` plus a sign extension
    is a signed byte-enum local assigned in an if/else;
  - a field read in place several times shares one load in a temporary; taken into a named local it
    shifts every temporary of an inlined walk (NextFireMode, 21 instructions);
  - the same accessor with a signed bound (`slt`) is a different inline from its out-of-line copy
    (`sltu`); written out at the call site the saved registers s1/s2 swap;
  - a callee of at most 15 statements too big to inline whole keeps its callers out of line: writing
    the callee (SetItemByIndex, 20 statements) removed RemoveItem's and SetItem's pins;
  - SOCOM II's zdb::IntersectStruct has a constructor zeroing m_poly and m_node (node_world.h does
    not declare it); the statics defined in order after <iostream> give __sinit_seal_kit.cpp;
  - a template copy proven by its caller can be emitted by an explicit instantiation and scored
    through an ovl-copy note (ai::_max<unsigned char> 0x5DEE70; anim.cpp's string-map find 0x58D090
    and string compares 0x58D250/0x58D740).
- **Tells from the HUD, round 19** (mp_rating.cpp, reco.cpp):
  - a list walk `i = begin(); iterator e = end(); for (; i != e; ++i)` (iterator assigned, end
    initialised) keeps `this + 4` in `this`'s register (mp_rating.cpp's walks);
  - push_back taking the pointer's address made MW number the parameters last; passing it by value
    through an inline wrapper put them first again (AddPlayer);
  - a bool local returned from an `int` function adds `andi 0xff`, from a `bool` function it is a
    plain `move`; a bool local just assigned true (`daddiu`) is reused as the value of a one-bit
    field set true in the same block (CReco::Init); `found == false` as a loop test keeps `andi;
    bnez`, `!found` drops it;
  - indexing a two-slot member array (`m_teams[i]`) gives the target's registers, a named walking
    pointer moves them;
  - units: mp_rating.cpp (SOCOM II's Elo ratings, CRatingTeam/CRatingMatch) runs 0x239770-0x23A2DF,
    reco.cpp (the voice recogniser's wrapper, the 0x40-byte theReco 0x414680) 0x23A2E0-0x23B98F with
    its initialiser 0x3FF7B0 -- the cut by what the code touches (the recogniser's fields at 0x2C/0x30).
- **Tells from the game states, round 17** (SetMovie and its cue sort, fts_message.cpp):
  - a function calling trivial accessors -- even leaves -- out of line holds a big EXPLICIT-inline helper
    MW inlines one level: the helper's callees become calls, their copies following the caller in
    reverse order of first use (SetMovie's ReadSubtitles, 0x1EDE70-0x1EDF3C);
  - zsys.h's inline FreeVector reached through a one-level helper is emitted with the unit's own code
    right after its caller, and requests the deleter's clear and destructor in the tail's first round
    (the class vector's reserve and insert then inline their temporary's teardown);
  - a sort comparator with no store and no constructor call is a default-initialised POD local; its
    slot below the iterators' says they were named locals built in place and declared before it;
  - `name = f(); if (!name) name = K;` puts the copy in the branch's delay slot, a ternary branches
    twice; an override passing its own bool argument on leaves a2 untouched at the call -- an unset
    argument register can be a pass-through, not a missing parameter (C2DMessageString::SetString);
  - include/vector's class vector has `reserve(n)` forwarding to __vector_imp's (0x1EDF10, a `j` to
    0x1EF190, as clear()'s copy 0x1ED000); fts_message.cpp (ours) runs 0x2B5830-0x2B794F after
    fts_action.cpp, as in SOCOM 1's symtab.
- **Tells from the AI core, round 11** (FreeTables, AI_PARAMS::Merge, the character type's packing):
  - an int counter indexing a member array (`m_start[i]`) is strength-reduced; `m_start[(unsigned
    int)i]` keeps the target's per-pass recompute (startvec::Load);
  - indexing `packed[n]` makes a cursor variable numbered last (s0); stepping the parameter itself
    keeps the parameter's register (PackWeapons); pinning an inlined callee fixed its caller's
    registers too (KorusGetPackedWeapons once PackWeapons was pinned);
  - one counter shared by two loops gives the target's swapped counter/cursor registers in the second
    (FreeTables) -- the opposite of the "two loops want their own counters" tell;
  - `(a & m) || !(b & m) ? x : y` gives two loads, a nested ternary three (Merge);
  - open: CAiMap::lines::Find 0x52FD90 (begin() before the end, registers), ResetGears 0x53F0B0 (an
    inner loop entered through a block after the outer loop, the SortNearToFar pattern), the pod resize
    copy 0x53AA50 (the tail's resize inlines DeleteLayers' erase whatever the declaration order).
- **Tells from the SEALs, round 11** (RTList<ACTIVE_ANIM>'s copies, seal_env.cpp's weapon parenting, DoSetStance):
  - in an inline helper `r = 0; if (c) r = (unsigned)f;` leaves the float-to-unsigned conversion's join
    a block of its own (+1 nop); the ternary, if/else and early-return forms merge it (0x543B90);
  - an `unsigned int` bitfield read through a byte-enum accessor gives the target's `andi 0xff`
    (0x57F150); a callback's `void *` data cast through an inline is numbered after a reference
    local, cast inline in the body the two swap saved registers (the pistol callbacks);
  - from a `bool` function `return p ? (a || b) : false;` adds `andi 0xff`, `if (p) return a || b;
    return false;` does not (0x5A77E0); `s = r != 0 ? s / r : 0` keeps nops around the `div.s`, the
    if/else form drops them (0x5A56D0);
  - in a switch case `if (!c) x = A; else {...}` puts the single assignment on the fall-through, `x =
    A; if (c) {...}` falls into the body (DoSetStance);
  - template members called as such (`Begin()`/`Next()`) make the first OVL2 unit in link order that
    needs them emit the instances (seal_anim.cpp, 0x5780E0/0x578110, ovl-copy notes);
  - zseal.h's body parts sit at SOCOM 1's offsets +0x7C (m_spinelo 0x2FC ... m_pistol 0x354).
- **Tells from the weapons, round 10** (the weapon event callbacks, the decal arrays, vis_vu.cpp's helpers):
  - an MW `asm` function loads a global's address with `la reg, sym` (an `la` in a branch delay slot
    puts its `lui` there); `%hi()`, `@ha`, `@l` and `lo()` are all refused;
  - a list element with an empty user constructor is copied member by member by the insert; with an
    implicit one it is a block copy through the FPRs (WEAPON_CALLBACK_STRUCT);
  - `erase(remove_if(begin(), e, p), e)` matches written `iterator e = end(); iterator i =
    std::remove_if(begin(), e, p); erase(i, e);` -- both erase copies made after the call (a lead for
    the erase(remove(...), end()) family);
  - `CPnt4D t(...); return t;` builds on the stack and copies out; `return CPnt4D(...)` builds
    straight into the return slot;
  - an element whose copy constructor resets a member shows a constant store at the start of every
    reserve/insert move (CMeshLOD's copy stores 1e12, SOCOM 1's FLT_MAX);
  - units: vis_decal.cpp's tail ends at 0x3BFB5F (vis_texfx from 0x3BFB60); an unwritten unit starts at
    0x3BA300 (ColConvTo3D, CVisData, the *_Object readers; SOCOM 1's 0x310460); vis_vu.cpp is probably
    SOCOM 1's vis_main.cpp -- its tail holds CVisual::m_stack_vid's deque copy-constructor chain
    (0x3B7DB0-0x3B83D0; the 0x3B7DB0 row, named ~stack, is the deque copy constructor) and its
    initialiser 0x404570 sits between zvid_init.cpp's and the decal pools'.
- **Tells from the kit and AI states, round 11** (seal_kit.cpp's weapon modes and indices, seal_sensors.cpp, ai::test):
  - `x > 3` compiles to `slti at,x,4; bnez at`, `x >= 4` puts the test in v0; an `||` arm written
    `return delay;` keeps its own `b end` block, an else-if chain assigning a result merges it;
  - `r = 0.5f; r *= a + b;` in an if/else gives the false arm's trailing nop and the half-first
    operand order (GetFireDelay); function-local `static float` constants match 8-byte-spaced data by
    content;
  - an inline that modifies its parameter copies it at entry: the copy taken into a local after the
    first test lands where the target has it (kit.h's wrap, GetAmmoIndex);
  - stores through `T *p = &arr[i]` re-read a global loop bound each pass (no unrolling); indexed
    stores into a separate global unroll 8x;
  - MW 2.4 inlined template functions MW 3.0 calls out of line: an explicit instantiation emits the
    instance until its caller is written (ai::test<int>/<float> 0x5DECB0/0x5DED70 for ai::PARAM's
    Tick 0x5D4770);
  - seal_kit.cpp ends in SOCOM 1's order (sealSaveFireModes, CSavedFireModeInfo(), the CAiWepHint
    statics), seal_sensors.cpp starts at 0x5C9400; __sinit_seal_kit.cpp is 0x668DB0 (named).
- **Tells from the UI, round 8** (util_loc.cpp, zui_hangul.cpp, zui_skbkeys.cpp, zui_skbview.cpp):
  - a null-safe string compare `a ? (b ? strcasecmp(a, b) : 1) : -1` gives the target's branches;
    the if/return form is 8 bytes longer; `h < 0 ? -h % 17 : h % 17` gives two divides, `abs(h) % 17`
    a movz and one;
  - a find returning `*i` from inside the loop keeps its result in a0, a result local set before the
    loop costs a saved register; `if ((e = f()) != 0)` gives `beqz v0; move a0,v0`;
  - an inline helper's by-value argument is loaded before the helper's own test (Load/Reload's
    `(*i)->m_name` ahead of `s_useZar`); a vector-subclass member adds one this-test to the
    destructor but one level to the constructor (Unload against Load: the bigger one kept);
  - an int table element passed to an `unsigned short` parameter loads with `lhu`; a char passed to
    an `int` parameter is sign-extended again; `(a != 0) ^ (b != 0)` puts the xor in v0;
  - MW names an unnamed namespace after the COMPILED file, not the header declaring it: zui_skb.cpp's
    classes used by the keyboard units 0x3903A0-0x39BBB0 (three units by their initialisers 0x403770,
    0x403800, 0x404110) must come from a shared header -- the next UI round moves them out;
  - util_loc.cpp runs 0x3A3100-0x3A59BF (its template tail from 0x3A4560, the twelfth std::sort).
- **Tells from the SEALs, round 10** (seal_body.cpp's blend and jump families, the hostile anchor, seal_cfg.cpp):
  - an inline accessor with an UNSIGNED index called with an int counter keeps the target's per-pass
    `sll; addu` on an RTStack walk (0x57DBB0, 0x57DC20);
  - two identical address expressions either side of a call are shared through a saved register;
    spelling the first differently (`&s->m` against `(char *)s + K`) makes MW recompute it, as the
    target does (0x57F310 and the other pops);
  - `if (!InlineBoolWithAnd()) x = ...; else x = false;` keeps the `&&` materialised (sltu) with the
    held arm last; a ternary or `!= false` does not (UpdateHostileAnchor 0x543030);
  - a call's result taken into a block-local before a float-zero store gives `sw zero`; stored through
    the call's result directly, MW hoists 0.0f into $f20 (0x57F690);
  - a short argument read through an inline accessor loads ahead of the call's other arguments
    (0x5802B0);
  - an unnamed unit function nothing calls can be a helper the unit inlines everywhere: write it and
    have its siblings call it (0x588B40);
  - `if (a || b) return false; return f();` leaves the false block right after the test (0x57F7D0);
    a single negated test lets MW move it to the end (0x57F530, open).
- **Tells from the HUD, round 18** (st_mpexit.cpp's stat leaderboard, hud_bitmapreticule.cpp, zui_2d.cpp's command pieces):
  - a side test written `if (b) { if (!A()) continue; } else if (!B()) continue;` gives the target's
    `bnez v0,ok; b skip`; the goto form inverts it;
  - a parser returning its new record or its base parser's result through one local
    (`cmd = Base::Parse(rdr); return cmd;`) matches; returning each directly is 4 bytes short;
  - a derived GetZAnimCmdType takes its base's count as `Base::GetZAnimCmdType(0) + n` (the null
    name folds the inlined base chain to its count); its fallthrough `Base::GetZAnimCmdType(name)`
    inlines the whole chain;
  - tools/perm.py compiles its copy as cand.cpp: an unnamed-namespace function is
    `...__18@unnamed@cand_cpp@...` with `--target`, and its `-I` paths must be absolute;
  - SOCOM II's BitmapReticule texture tables hold 11 entries (SOCOM 1's 10); hud_bitmapreticule.cpp
    ends with hudStartDefuseTimer at 0x21932F as in SOCOM 1 (~BitmapReticule now matches from the unit,
    no longer a draft-only false regression); the unwritten unit between hud_mpinfobox and
    hud_mpweaponsel spans ~0x239770-0x23B98F (its global a 0x40-byte object storing 0xDEADBEEF at 0x34).
- **Tells from the online lobby, round 8** (zNetMediusFile.cpp complete, the ladder list, the friend invitations):
  - MW never inlines `std::make_pair` here, by reference or by value; `std::pair<K, V>(key, v)`
    inlines and reproduces the target's two string copies;
  - an iterator declared first, `iterator e = end();` taken next and then `i = find(k);` assigned,
    lives in memory, so the dereference re-reads it as the target does; initialised from find, MW
    reuses the compared value; `i != end()` inside an inline helper cuts end() to an out-of-line
    `tail()` -- end() taken into a local first stays inline;
  - a buffer address kept in a saved register across calls needs a local (`char *id =
    req.MessageID;`, declared after `err`); a request declared inside an inline helper takes a lower
    stack slot than one in an earlier-expanded inline;
  - a function returning a bool call's result while it has an address-taken local is no tail call
    (return type `bool` matches, `int` adds `andi 0xff`); an out-parameter's stack slot shows its
    type's size (a two-word result moved the local from sp+60 to sp+56);
  - an inline constructor cut to one level calls even its members' empty user-declared
    constructors: dropping the member's constructor fixed it, its implicit assignment staying out of
    line member by member;
  - two `unsigned int` bit-fields totalling 16 bits take two bytes (a following char array starts at
    2); a struct with an odd-length char array is copied member by member only when assigned (`T c;
    c = *p;`), `T c = *p;` block-copies;
  - include/tree has `swap(__tree &)` out of class like clear, include/map `swap(map &)`; open on the
    MSL side: the inline allocator() (ReleaseFileTransfers, and the out-of-line ~__tree and
    ~pair<std::string, CLobbyFileIO *> the image lacks), the compressed-pair tree swap 0x305BE0.
- **Tells from the game states, round 16** (fts_action.cpp, CGTree::FindNode, the graph destructors, CMsgEditor):
  - a member list whose destructor tests `this` four times before the erase is a class of its own
    over `list<T *>` (CGNodeList, CGEdgeList: ~CGNode, ~CGRoute, ~CGraph);
  - a `std::transform` op written as an inline function and passed by pointer inlines fully, with
    no out-of-line copy; a functor passed by value leaves an `sb zero` temporary; MW 3.0 takes a
    function template's address only through an explicit cast;
  - for an unsigned length `len > 1 &&` branches on `sltiu at`, `len >= 2 &&` materialises the test;
  - `T *a; bool b; U *c = f(); a = ...; b = true;` gives `c` the highest number while `a` and `b`
    are still computed after the call;
  - `buf[len] = c; buf[++len] = 0;` keeps a separate `len + 1`, where `buf[len + 1]` folds into the
    previous store's address; virtual calls on member objects stay virtual;
  - hud_main.cpp's `theMission.m_messages4` (+0x1060) is a CMsgEditor, not a C2DMessage_Q;
    __sinit_fts_player.cpp (0x400330) is emitted byte-exact but for ~CSpawners (0x2B4030, open);
    CGTree::Destroy's inner self-inlined copy is one inline level too shallow (it wants SOCOM 1's
    list shape: tail() 0x2D2530, the node/sentinel constructors 0x2D2520/0x2D2510).
- **Tells from the AI core, round 10** (CAiMaps::Tile/SetOccupied, the overlay walks, the look/cover finders):
  - a const-walk inline called on a global object (`ai::overlays.Contains`) lets MW fold the constant
    `this` and drop each pass's store of end(); walking through `const T *l = this;` keeps the
    target's per-pass store (SetOccupied, 0x53A500);
  - a postfix `i++` adds one inline level: it made the open-cell helper six high, so it was cut to
    one level (_find_look, _find_cover);
  - a field read twice without a name merges into a temporary numbered after every named local
    (_tile_soil's flags); summing through a const reference to a local keeps MW from sharing that
    load with a later read (FindLookLoc's d.z);
  - an inline cut to one level makes the caller call even iterator default constructors and
    assignments out of line, and their copies sit right after it (SetRegionType, 0x529D30-0x529ED7);
  - `m_regions[type].end()` in a loop test keeps the size's and the data's addresses in two saved
    registers, a reference local only the vector's;
  - open: GetRandomLoc 0x529EE0 (the target pads a nop after each random draw), SetRegionType
    (s1/s2), std::remove 0x529D70 (include/algorithm's shape, 0x1C short).
- **Tells from the kit and AI states, round 10** (anim.cpp, seal_ex.cpp, seal_ailines.cpp, ai_group.cpp):
  - a const inline accessor reached through a non-const parameter reloads the field the caller just
    tested; the parameter copied into a `const T *` local reuses the load (ftmask_create);
  - a postfix `i++` on a class iterator gives the rotated loop (its setup block at the bottom),
    prefix `++i` does not (CloseAll); an inline accessor as a loop bound is re-read every pass
    (CAnimTypeRec::Count());
  - an `int` callee result summed into an int needs an `int` return type -- a `bool` adds `andi 0xff`
    at the call (MarkState);
  - a walk stepping the group pointer itself through the pairs keeps one cursor register, a separate
    pairs pointer takes another; in `if (a == b)`, the element read into a local first decides the
    `bne`'s operand order (Contains);
  - a state whose constructor is inlined one level calls a real CAiTimer member's constructor out of
    line, so the unit emits the copy (CAiTimer(float) 0x5EA550);
  - units: seal_ex.cpp starts at 0x5B8190; anim.cpp (0x5E0030-0x5E622F) holds CAnimTypeRec and its
    600-record pool; seal_ailines.cpp runs from ~0x5FF2F0 to seal_look.cpp (SOCOM 1's ai::MoveTo at
    0x600070); CAiState(AI_STATE)'s copy 0x5EA5D0 and CAiMapLoc's copy constructor 0x5EA7E0 come only
    with CSealUnit::IssueCommand (its static CAiSBreach built where CAiStateStack() is not defined).
- **Tells from the SEALs, round 9** (seal.cpp's small rows, seal_ai.cpp's stance setters):
  - the prologue copies parameters into their saved registers in reverse declaration order, floats
    included: the order of the moves places a float among the int parameters where no call site
    shows it (SetDesiredStance: (stance, float, bool));
  - two chained member-pointer calls are two `__ptmf_scall`s through frame copies of the pointers;
    typed `extern const` placeholders for the constants pass their addresses as the target does;
    a static list walked through a const view keeps the iterator temporaries in the frame (0x545D80);
  - a member vector walked through a local pointer keeps the member's address in a register across
    the loop -- passed to a static inline helper the offset folds back (0x544B60); a reference local
    to a global list keeps its address in a saved register across a loop of calls (0x543930);
  - a call's result ANDed with a member loads the call's side first only through a local; written
    inline MW loads the other side first in either operand order (0x544FC0, 0x5434B0);
  - a byte-enum member is copied with `lb`, a signed-char member cast to the enum with `lbu`
    (RestoreAiMode); an unsigned short count converted to float through unsigned int needs an
    unsigned int local, or MW masks it again after the test (0x5458A0);
  - a result assigned and returned inside the then-block keeps that block first; `if (!b) return A;
    return B;` puts it last (InStance);
  - CSealStats lives in zseal.h now (m_stats at 0x58C, before m_accuracy as in SOCOM 1); st_mpexit.cpp
    still keeps a copy of its own.
- **Tells from the integrator's zar units** (zar_search.cpp, zar_key.cpp):
  - SOCOM 1's debug allocator calls name units: its `_calloc(n, s, "zar_search.cpp", line)` (SOCOM II
    passes 0, 0) put the fetches in a unit of their own -- a SOCOM 1 function that allocates gives
    its file's name;
  - `size <= key->m_size` branches straight on `slt at`; `key->m_size >= size` materialises the
    test (`slt v0`, `bnez v0`) -- the weapons round 9 tell with the operands the other way;
  - `return open ? f(...) : 0;` from a bool function, an int ternary, gives `andi 0xff` on the
    call's path, `move v0,zero` on the other and `sltu` after the join (SOCOM 1's typed fetches too);
  - `if ((size = f()) > 0)` branches on the call's result (`blez v0`, the copy in the delay slot);
    `int size = f(); if (size > 0)` branches on the copy;
  - a parameter copied to a local (`CIO *file = io;`) is numbered after the parameters: it takes the
    saved register a later parameter would (CKey::Read: s1, where io takes s2);
  - an inline whose by-value parameter it passes by reference (InsertKey(key) -> push_back(key))
    keeps the caller's variable in a saved register and copies it to the frame; push_back(key)
    written directly keeps the variable itself in the frame;
  - an auto-inline function inlined by one caller (CKey::Read inlines InsertKey) and called out of
    line by an equally small other (Insert(const char *, ...)) is a unit boundary: zar_key.cpp
    (CKey's) splits off zar_search.cpp at 0x25AE00, the fetches calling FindKey out of line too;
  - zar_main.cpp starts below the GCC libraries' nominal 0x258000: their 8-aligned rows end at
    0x256FB8, and from 0x256FD0 on it is MW code (CZAR's buffer writer, NewKey 0x2571D0,
    WriteDirectory ~0x257590); CZAR grew four words at 0x20 and 0x2C-0x34 and its TAIL one, so
    m_tail sits at 0x38 and m_root at 0x9C.
- **Tells from the HUD, round 17** (st_mpexit.cpp's stat machinery, four static initialisers):
  - a pointer-to-member constant passed by value to an inline is copied into a local (ld/lwc1 then
    sd/swc1); `(p->*pfn)()` calls __ptmf_scall with t9 = &pfn; `pfn1 == pfn2` compiles to
    `!__ptmf_cmpr(&pfn1, &pfn2)` (MAIN 0x1828E0, named);
  - an array of pointer-to-members, even a const one, is initialised at run time by the unit's
    initialiser (c_arrStatFuncs); the accessors it points at are emitted where their address is first
    taken, in reverse of the array order;
  - two short fields compared: load them into int locals first (`int a = s1.x, b = s2.x; if (a != b)
    return a < b;`) -- compared straight, MW loads the right operand first;
  - computed file statics defined in address order reproduce an initialiser exactly (scdiff compares
    .data variables by content; the bss statics' order is the computation order);
  - a walk starting on an element already examined is `for (;;) { it.Advance(); if (!it.More()) break;
    ... }`;
  - include/iostream holds the `__nInit`/`__wInit` objects ~40 initialisers build first; 0x219090
    (~BitmapReticule) is a third draft-only false regression under `--files '^(?!auto/)'` (the unit's
    span now reaches its initialiser); initialisers found: st_mpexit 0x3FE860, hud_bitmapreticule
    0x3FE490, hud_ammo 0x3FF610, hud_mpinfobox 0x3FF680, hud_mpweaponsel 0x3FF7F0, hud_linemap 0x3FE190,
    hud_actionbitmap 0x3FE550; 0x22EDB0-0x237xxx is SOCOM II's radio command menu (TCM, VOTE, MUTE).
- **Tells from the weapons, round 9** (CZItemStash, CVehicle(), the vehicle initialisers, vis_vu.cpp):
  - a class-iterator walk with a postfix `i++` gets its end hoisted into a preheader after the loop
    (entered with a `b`, the entry test repeated there); `++i` re-reads the vector's data every pass
    (CZItemStash::Count and Clear match with the postfix step);
  - `if (i > Count()) return;` branches straight to the epilogue on `slt at`; `if (Count() >= i)
    {...}` materialises the test and keeps a join;
  - 1-bit fields set in a mem-initializer list are word read-mask-writes (`true` folded to `daddiu
    1<<n`, `false` left as `(0 & 1) << n`); set in the body or through inline setters, MW narrows a
    global's accesses to lbu/sb -- what fixed both vehicle initialisers (the debug masks);
  - a pointer-vector slot read and cleared through the pod's data() reference keeps `&data_` in a
    saved register across calls; operator[] reads data_ straight and keeps `this` (DereferenceDecals);
  - a square taken into a local before a run of stores keeps its loads ahead of those stores;
    `if (a == -1 || !b || !c) return;` gives the `bnez -> body; b exit` layout;
  - ~CVehicle wants the cells as a vector subclass (a third this-test), which makes the region's
    constructor six levels high and costs CVehicle's constructor -- the constructor kept;
  - vis_vu.cpp (ours: 0x3B36F0?-0x3B7FDF, CVisual's decal and VU half, SOCOM 1's vis_main.cpp's first
    half -- its stack destructor 0x3B7DB0 and initialiser 0x404570 suggest it may be the real
    vis_main.cpp).
- **Tells from the UI, round 7** (util_options.cpp, util_ssm.cpp, the HDD icon writers):
  - a local bitfield struct cleared in the mem-initializer list gives the word-wide read-mask-write;
    clearing it in the body narrows the stores to sb/sh;
  - std::map's no-argument constructor passes a bare stack slot for the allocator: the allocator's
    constructor has to be inline (in include/memory since the MSL headers' round 7; util_options.cpp's
    explicit specialisation is gone);
  - the reverse switch-test order holds for grouped cases: tests 4, 2, 3, 1 come from `case 1: case 3:
    case 2: ... case 4:`;
  - a type byte read again after a whole-word mask test reloads only through a byte member in a union;
    `(unsigned char)word` reuses the word load with andi;
  - `T *p; bool ok = false; p = argv[++i];` gives the target's scheduling where an initialised
    declaration does not; `while ((bool)f() && ...)` gives the sltu/xori pair before the branch;
  - a stack header can be larger than what is written (the PS2ICON3D header: 0x220 bytes, 0x200 written);
  - a class's vtable is emitted where its only non-inline virtual is (CSimpleStateMachine::Start);
    ~CSimpleStateMachine needed its queue in a class of its own for the fourth this-tested level;
  - include/queue (std::queue: its default constructor copies a default container) and a general
    deque<T> (buf_size 512 / sizeof(T)) are in; units: util_options.cpp 0x3A2510-0x3A30FF (the
    start-up options, a map<string, OPTION_INFO>), util_ssm.cpp from 0x3A59D0, util_loc.cpp
    (CLocTable).
- **MW's saved-register order is a NUMBERING** (the task-handler lane's round 9, seal_tasks.cpp): not
  interference degree or use counts -- adding 20 temporaries or 16 uses of a variable never moves
  it. Variables are numbered parameters first, then the function's named locals in declaration
  order, then inlined functions' named variables in expansion order, then temporaries; the highest
  number gets s0; the first non-parameter variable read (a null test) moves to the end. A plain
  pointer cast of a parameter (`T *cmd = (T *)hdr;`) makes a variable of its own, numbered just after
  the parameters; the same cast through an inline (`template <class T> T *TaskCmd(...)`) makes it the
  parameter's register. The 13 open task handlers want the block numbered where a variable declared
  between the inlined find's counter and its list would be -- no C++ shape tried does that (the
  variants are in taskFindParam's comment); tools/perm.py's best was 8 of 14. An inline Create()
  whose member inline stores then calls keeps that member's `this` (cmd plus an offset) in a saved
  register across the call; a byte-enum call result tested `!= 0`, declared uninitialised ahead of a
  later flag, gives the andi before the branch (CTInState::Parse).
- **Tells from the online lobby, round 7** (the room and ladder units, zNetMediusFile.cpp):
  - a function handed to std::transform by address becomes a direct, never-inlined `jal` with the
    element in a0 (DeleteRoom); a functor's operator() would have `this` in a0;
  - a destructor testing `this` three times for a pointer vector means the object holds std::vector
    itself (BriefingRoom : vector<RoomGame *>, the channel-list record a second base at 0xC); a derived
    list class adds a fourth test;
  - zsys.h's FreeVector<std::vector<T *> > reproduces the out-of-line constructor/clear/swap/
    destructor calls (BriefingRoomDB::Release);
  - structs with odd-length char arrays are assigned member by member, user constructor or not;
  - when two plain loads are compared, the left one is loaded first (`field > global` loads the
    field first);
  - a clamp written as the call argument `x - 2 <= 0 ? 1 : x` matches; into a local, `s = x; if (x <=
    0) s = 1;`;
  - `err = f(); flag = 1; Check(err);` puts the flag store in Check's delay slot; `int c =
    strcasecmp(...); return c < 0;` keeps the target's empty delay slot;
  - CLobbyFileIO's 15 slots are CIO's (zutil.h) in the vtable at 0x406760; the file API's map is
    `map<string, CLobbyFileIO *>` (freadAsync and Close insert the Medius key with `this`, the transfer
    callbacks find it and read node+0x18); requests: MSL's std::sort (eight room/ladder sorts and the
    chat unit's), c_str() on a string member read off the containing object (UpdateLadderBestDisplay),
    the deque copy constructor (SOCOM II's circular-buffer iterator is four words: current, data,
    data+start, data+capacity).
- **Tells from completing the MSL headers, round 5** (the string trees, list, the class vector):
  - the string-keyed trees' compare is MSL's `basic_string::compare(const basic_string &)` (0x2DEE30,
    every caller tests the sign) and `compare(pos1, n1, s, n2)` (0x2DF030, SOCOM 1's
    CFUiUiPCcUi); `operator<` is inline. The Medius file API's `map<string, CLobbyFileIO *>`
    (0x450070) tail, the material unit's `map<string, int>` and the IOP module table (keyed by
    std::string, not CSaferStr: a CSaferStr key's implicit copy is one inline level more) are
    byte-exact as probes; include/tree has increment, balance_erase, erase, erase_one, clear()
    out of class, include/map erase and a no-argument map();
  - list remove_if takes begin() before end() and finds the run's end with a for loop breaking on the
    first refused element: `for (++j; j != e; ++j) if (!pred(*j)) break;` (0x20CB50, its OVL2 copy,
    0x2D2540); the pointer list's begin() through base::begin() measured -1 and is not adopted;
  - the class vector's end() takes `data() + size` into a const local (a walk then re-reads the data
    pointer each pass: CMesh::SetLOD, GetRouteIndex, GetRegionContaining), its operator[] reads the
    data pointer into a const local and keeps the index (AddLOD's `m_lods[size() - 2]`), its iterator
    has operator->; the scalar vectors have iterators, begin(), end(), erase (CZItemStash::Compact);
  - include/string's assign(const char *), append and replace(pos, n1, s) are out of class (seven
    units' tails emit them byte-exact); its default constructor stays two inline levels high
    (four cost zui_spec.cpp's GetControlSpec), so fts_state.h keeps its own string; its destructor
    is no longer pinned (it only ever mattered for units' own functions);
  - MW does not merge two adjacent is_long() tests (write one if/else); in tail code, within one
    generation round, later-declared members are generated first (declaring erase, erase_one, find in
    that order makes erase_one inline find and call erase); a user-declared copy constructor on the
    tree iterator makes MW pass it by address; a reference local taken before a test keeps a member's
    address in a saved register across a call; a loop descending through a reference stores it every
    step, through a local once; `get_bit() == false` gives sltu then bnez (a branch) or xor/sltiu (a
    value), `== true` li 1/bne;
  - git merged two identical RTArray::GetOldestUsed definitions silently (two lanes added it): a
    merge can duplicate an additive member -- the build catches it.
- **Tells from completing the MSL headers, round 11** (MW's colouring replayed offline, where MW's
  temporaries come from; no header change gained a match without losing one -- the measurements are in
  include/vector's insert, include/algorithm's remove_if, include/deque's deque() and include/list):
  - MW'S COLOURING REPLAYS EXACTLY OFFLINE: build/p11/igreplay.py (untracked, with the round 10 tools
    it builds on) reruns simplify/colour from an igdump record -- K = 25, simplify's ascending sweeps,
    the spill pick MW's own (`spillCost / degree` as a float, the first least in a descending-index list,
    mweppc's SpillCode.c: reads add the block's loop weight, twice for a register not rematerialisable;
    igdump now reads each node's cost +0xC and each block's weight +0x24) -- and reproduces every
    colouring of the probes and of st_core.cpp (61); igmap.py reads the TARGET's register for each
    virtual register by aligning the pcode with the final code (the cue insert: 71 vregs, no conflict),
    so a candidate graph is scored against the target before any source is written (igexp.py);
  - the class insert's end: the target's colouring needs e numbered after the loop optimiser's else-loop
    cursor and before the CSE pass's n*8 -- a window no declared local reaches, while m, i and both j's
    must stay declared locals; the turret's insert needs e before m, CMeshLOD's after it. So the end is a
    declared local that small elements (copy_backward inlined) replace by a later temporary; the source
    that does it is not found (include/vector's insert comment lists eleven shapes);
  - WHERE MW'S TEMPORARIES COME FROM: tmpdump.py breaks on CParser_GetUniqueID (0x42F590 in 3.0b52, the
    `@N` counter at 0x5841AA) and records each creator's return addresses: the inliner, then the loop
    optimiser (one cycle of temps per loop: an induction temp replaces a cursor whose initial value is a
    copy -- @2517, the else loop's i), a hoisting pass, the class scalariser (iterator objects, in IR order
    of first appearance), CSE (size_, n*8, (m-n)*8 here), then one late pass; a temp's vreg number is its
    creation order, which is the colouring order of every node that is not deferred;
  - MW'S DWARF DROPS A LOCAL IRO ABSORBED: `int b = inline_f(a);` leaves no `b` in the DWARF (folded into
    the inline's return temp) while a merely dead local (the insert's xp) stays listed, 3.0b22 and 3.0b52
    alike -- so SOCOM 1's insert list (i, j, newcap, newsize, tmp, j, i, k, j, xp, t1) lacking an end and
    an m says those were absorbed, not that the source had none; it also lists locals in plain
    declaration order with nested scopes flattened, and never an inlined function's;
  - copies merge into the earlier variable: a copy `@N = v` whose source dies merges into whichever has
    the lower VarRecord index (e keeps copy_backward's parameter temp; that temp keeps a CSE temp when no
    e exists); PCode copy propagation cannot remove a copy whose destination is read-modify-written;
  - the list remove_if instances' swapped copies are the order MW BUILDS argument temporaries (left to
    right) against the order it EVALUATES plain arguments (right to left): out of line (valve_list.cpp
    0x352500, ai_events.cpp 0x50F670) the stack slots agree and only the copies' order differs;
  - a class element's deque() calls only the buffer's constructor while its copy constructor calls
    allocator<const void *>() at +0x18 (lobby 0x2E0D90/0x2E07B0, chat 0x2F6230, the state machine
    0x3A6410/0x3A5E50); `public Allocator` as __deque_imp's second base gains the default constructors
    and loses both copy constructors;
  - not reached: SOCOM 1's list shape for CGTree::Destroy (request 3), CRegistrant's element copy,
    CMaterial_band's tail order, the IgnoreItem insert/erase and the three reserves.
- **Tells from completing the MSL headers, round 10** (MW's register allocator read out, the 4-byte pod's
  operator= and assign):
  - READING MW'S REGISTER ALLOCATOR: gdb runs the compiler under wibo (break on wibo's
    `thunk_kernel32_CreateFileA`, then software breakpoints in the PE). In mwccps2 3.0b52 the coloring
    driver calls buildinterferencegraph 0x4BC7B0, simplifygraph 0x4C4480 and colorgraph 0x4C47F0 (exit
    0x4C4962); the graph is an array of 30-byte nodes at `[0x5B63C4]` (next +0, the variable's Object +4,
    index +0x10, colour +0x12, flags +0x14, degree +0x16, an edge list +0x1A whose records hold both
    ends at +0xC/+0xE and each end's next at +4/+8), the class byte at 0x5B7F5D (0 = GPR),
    used_virtual_registers at 0x5B6880, the blocks at `[0x5B639C]` (first instruction +0x14; opcode
    +0x28, operand count +0x2A, 22-byte operands from +0x3C, register at +4) -- the release build's
    `#pragma debug_listing` writes nothing (its listing function is a bare `ret`). The MSL lane's
    build/p10/igdump.py (gdb Python) dumps every coloring: each virtual register's name (an Object's
    HashNameNode, the string at +0xA), degree, neighbours, push order, colour and the pre-rewrite code;
    igshow.py, igcode.py and igvar.py (a header variant's effect on one variable's degree) read it;
  - the allocator is the decompiled PowerPC backend's (doldecomp/mweppc Coloring.c): virtual registers
    are numbered arguments, then named locals in declaration order, then front-end temporaries
    (`@N`: the inliner's and optimizer's -- an IR live-range split shows as one), then code-generation
    temporaries; simplify pushes in ascending order every node of degree below K and colour pops the
    stack, each node the lowest free volatile register (v0 first where it is free, then v1, a0 ...),
    a nonvolatile (s0 up) only when none is; K = 25 (32 less zero, at, k0, k1, gp, sp, ra -- fp is
    allocatable), so a node with 25 or more neighbours is deferred and coloured BEFORE the rest. That is
    MW 3.0's "first-declared local gets the highest saved register" and every one-off register tell;
  - the class insert's in-place end (include/vector's insert comment): e is a named local deferred at
    degree >= 25, so it takes v1 ahead of the loop temporaries; the ELEMENT moves its degree by the GPR
    temporaries its copy and assignment need at the four sites e spans (float members go through the
    FPRs and add none). Here e has 30 neighbours for CMovieCue; the target's float-first 8-byte entries
    need 24 or fewer and TypeRemoteGuy 25 or more, so the original template gives e 6-9 fewer
    neighbours with identical instructions -- the shape that does is still open;
  - include/vector's 4-byte pod has the char pod's out-of-class operator= (0x259AF0) and an assign with
    init's word loop and MSL's "vector::assign length error" (0x259D20; the char pod's 0x393FB0 says
    "vector length error"): both byte-exact as a probe, rows named -- what an implicit assignment of a
    class holding pointer vectors calls (vector<LOD>'s insert 0x35E100, the UI variables' erase, the soft
    keyboard's element vectors, zar_main.cpp's 0x259250, itself an unnamed class insert of a 24-byte
    element holding two pointer vectors); the UI lane's zui_hangul.cpp emits both byte-exact;
  - a code-exact instance can wait on its callee's row alone: AcctRecord's insert 0x2FE3E0 matched once
    its copy constructor's row (0x2FEAD0) carried the name zNetMediusUniverse.cpp emits.
- **Tells from completing the MSL headers, round 9** (the 1-byte pod vector<char>, basic_string::assign(const char *, n)):
  - MW reuses a pointer difference only when both are the same expression: the target recomputes
    `last - first` after an allocation, so its two lengths come from different expressions;
  - a move helper taking its length into a local (`unsigned int n = last - first`) lets MW fold
    `(old + k) - old` to `k`; inline-helper parameters of type `const T *` given `T *` arguments block
    reuse of the caller's `e - position`, plain `T *` parameters allow it;
  - `k = position - data_` declared before `T *old = data_` gives the target's numbering; testing
    capacity through capacity() while the growth reads capacity_ keeps the target's second load;
  - in a unit's tail a class vector's move helpers inline the string's copy and assignment only if the
    unit's own code generated them first;
  - include/vector's char pod (moves through memcpy/memmove/memset) gives the keyboard unit A's seven
    instances byte-exact as probes (0x3923E0, 0x392A30, 0x392600, 0x393D30, 0x393D80, 0x393F00,
    0x393FB0) and assign(const char *, n) 0x39B620 through the template replace 0x1EE780 -- they count
    once the UI writes their callers; unit B's 0x2C-byte element (CSaferStr plus two rects) and the
    element-vector chain 0x393DD0/0x393E40/0x394090 are recorded open.
- **Tells from completing the MSL headers, round 8** (the erase/remove family, include/algorithm's
  remove and remove_if, the class insert's end register):
  - THE erase(remove(...), end()) FAMILY IS THE CALLERS' FORM, not a header's: MW builds an argument
    list's temporaries LEFT TO RIGHT but evaluates it RIGHT TO LEFT, so in the one-line idiom erase's
    end() always lands in a slot below remove's temporaries and is copied into its parameter before
    the call (const-reference forwarding in erase moves the copies but not the slot, and loses
    CGrid::Destroy). The target's slots read `iterator e = end(); iterator i = std::remove(begin(), e,
    x); erase(i, e);` -- the end declared first takes the highest slot, the new end the next, both of
    erase's copies follow the call (DeleteLightMap, CTurretList::Remove, CTargetList::RemoveTarget,
    CWorld::DeleteChild, CNodeVector::Remove; CleanupTemporary and RemoveInvalidPoints as probes) --
    or with a separate `end()` for remove_if's last (RemoveEmptyGames, RemoveUncheckedGames: six
    slots, two of them ends);
  - std::remove has remove_if's ternary shape (`first = find(...); ForwardIterator i = first; return
    first == last ? first : remove_copy(++i, ...);`): the dead copy of begin() into remove's own first,
    `i = first` scheduled ahead of the test, the two results joined through an address -- the early
    return was 0x18 off in every instance (the AI core's remove<CAiMapLoc> 0x529D70 byte-exact so);
  - remove_if searches with `find_if<ForwardIterator, Predicate &>(first, last, pred)` (libc++'s
    form, MSL C++'s sibling): the predicate is read in place where a by-value find_if copied it
    (delshadow's 0x320800). The list instances stay 6 instructions off: MW numbers inlined copies in
    creation order, a by-value find_if creates last's copy first, and the target made the list
    iterator's copies in parameter order but the vector iterator's right to left -- a const-reference
    search copying into locals serves the lists and loses the vectors (build/p8/rif1.cpp);
  - the class vector's range erase reads its iterators through pointer references, as the pointer
    vector's does (RemoveTarget: MW copies a named iterator into the parameter only when its address
    is taken); resize is declared AHEAD of erase (a unit whose own code instantiates erase otherwise
    has resize<CAiPath::point> inline it: RemoveInvalidPoints and 0x528430 byte-exact together);
  - MW's DWARF 1 lists a function's locals in plain declaration order, nested scopes flattened in
    source order (3.0b52 and 3.0b22 alike): SOCOM 1's class insert declares i, j, newcap, newsize,
    tmp, j, i, k, j, xp, t1 -- no end local. The in-place end's register is per ELEMENT across the
    image's 29 class inserts (include/vector's insert comment): still open.
- **Tells from completing the MSL headers, round 7** (include/deque's copy constructor chain, include/memory):
  - THE TAIL'S GENERATION ORDER, measured (probes build/p7/ord*.cpp): the unit's own functions are
    taken in reverse definition order (their emission order), each one's template requests generated
    before the next one's; within that, out-of-class members are generated class by class, later-
    declared class templates first, each class's members in DECLARATION order, in passes until no
    request is left; a member inlines another only if that one was generated before it. So `c` inlines
    `b` when b is declared ahead of c, whatever order a caller calls them in (ord7), and two members
    requested by the same caller in the same pass never inline each other the other way (ord4);
  - MW lays an empty base listed first at offset 0 and a later one at the object's end (build/p7/lay.cpp);
  - the deque's blocks are a compressed pair (SOCOM 1's compressed_pair over compressed_pair_imp) of
    the allocator and the block buffer: ~__deque_buf tests `this` five times after its loop as the
    image's does; the copy constructors (rdr 0x333B30, util_ssm 0x3A5E50, the lobby's 0x2E07B0, the vis
    unit's 0x3B7DB0) are `base(x.alloc())` (the pair at 0), the base's constructor from an allocator
    (a rebound allocator temporary, an empty buffer from it, the pair copy-constructed from both), the
    allocator base, x.begin(), x.end() and init; begin()/end() return a named make_iterator() result
    (`return make_iterator(0);` of the same type is a tail call); the pointer deque's default and copy
    constructors are out of class -- CGame()'s queue calls deque<CGameStateChangeCmd *>() (0x2CEE00,
    renamed from __deque_imp<PCv>()), CRdrEditor() the stack's temporary (0x3340F0);
  - the allocator's copy and converting (rebind) constructors are out of class, as SOCOM 1's; its
    default constructor is inline except for the const-pointer allocators (a partial specialisation):
    allocator<const void *> is the empty base the deques and buffers call out of line (0x2CEE30), and
    std::map's no-argument constructor builds its allocator temporary as a bare stack slot
    (ReleaseFileTransfers 0x305640, the soft keyboard's map() 0x38E740 byte-exact);
  - a user-declared copy constructor on the buffer's iterator makes MW pass the by-value iterators by
    address (not the target's);
  - OVL2's 0x58D090 is std::map<std::string, int>::find (ovl-copy of 0x2DED30); the kit lane's name
    lookups are `if (name) { i = m.find(name); if (i != m.end()) return (*i).second; } return -1;` (an
    early `if (!name) return -1;` lays the -1 out first) -- 0x5E4F20 byte-exact as a probe
    (build/p7/kitfind.cpp);
  - still open (include/deque's comment): the generation order the image's copy chain implies (its
    base constructor calls the temporary's destructor 0x2CEE90, the buffer's copy constructor calls
    its begin() and the allocator base while its constructor from an allocator inlines both -- no
    declaration order tried gives both), the default constructors' middle level 0x334120 (rdr, vis,
    lobby) that game_main.cpp's 0x2CEE00 inlines, init's parameters copied twice around two empty
    temporaries (0x333BB0, 0x3A5ED0), the buffer's begin()/end() built in memory (0x334030,
    0x333F90), ~__deque_buf's loop set-up (0x140 against 0x138); the tree swap's anchor address
    (0x305BE0: MW hoists `&tail()` into the test's delay slot where the target computes it in each
    arm, after the parent word's load); the class vector's insert/erase, std::remove and remove_if
    (requests 3 and 4) not reached.
- **Tells from completing the MSL headers, round 6** (include/algorithm's sort, include/vector's
  iterator arithmetic and range erase, the ladder's names):
  - std::sort is SOCOM 1's MSL sort (CW 3.0.1's `sort`/`__sort132`/`__selection_sort`, with
    `min_element`, `iter_swap` and the static `shuffle`, -4 in data, stepped -4..4 twice a pass) over
    SOCOM II's class iterators: each instance is three functions in its unit's template tail, a
    comparator object by value inlined into all three, a function pointer called. Each instance has its
    own `@LOCAL@sort<...>@shuffle` in .data -- data.txt names them, or the instance's relocations
    cannot resolve. Twelve instances in the image, all byte-exact as written or as probes: the room
    unit's games and rooms (0x2E4A90, 0x2E5150), the ladder's two (0x2EC260, 0x2EC7E0) and
    ai_maptable.cpp's names (0x5312D0) as their units emit them; st_mpexit.cpp's seals (0x226060),
    st_core.cpp's film cues (0x1EE000), the chat unit's room players (0x2F1980), the clan unit's members
    (0x2E9B40, by LadderStat highest first) and ent_target.cpp's targets (0x2A12E0, with swap<CTarget>
    0x2A1880 and the iterator's operator* 0x2A19B0 out of line) as probes (build/p6/), their rows named
    (the comparators of the unwritten callers' named ours in the rows' notes); util_loc.cpp's 0x3A4560
    (12-byte entries by their halfword id) is the twelfth, unprobed;
  - the shapes the sweeps settled: `if (len <= 1) return;` (`< 2`: 2 instructions), `len *= 3;` before
    the second pick (`3 * len / 4`: 16 bytes more), and the iterator's `last - 1` a copy stepped by `-=`
    (MSL's own arithmetic: built as `iterator(m_p - n)`, every short-range sort is 6 instructions off).
    That operator- folds the step into a load, so the class vector's back() steps the end pointer itself
    (`*(end().m_p - 1)`: CAiPath::PopPoint and CTargetList::AddTarget compute the address first);
  - SOCOM 1's sorts took raw pointers (`sort<CZSealBody **, ...>`): over a raw pointer st_mpexit.cpp's is
    0x3B4 bytes, the target's 0x598 is the vector iterator's -- an s1 template name is a prediction;
  - a comparator's test written in operator() itself loads `*--m` before `*first`; through an inline
    function of its own the target's order (the room's GameLess over GameSortsBefore);
  - CAiMap::name's four bytes after the name are two (the names' sort copies them lbu/sb);
  - c_str() on a member: `(*i)->m_name.c_str()` keeps the string's own address in a register (item +
    0x8C, then +1, +8) and no form of c_str() itself moves it (eight tried); read through an inline
    accessor on the item (LadderItem::GetName), the inlined c_str() folds the offset into the item's as
    the target does (UpdateLadderBestDisplay, LadderList::Find 0x2EBC20);
  - the pointer vector's range erase reads its iterators through pointer references (their addresses
    taken: MW copies a named iterator into the parameter instead of passing it straight, DeleteLayers
    byte-exact). Still open: erase(remove(...), end()) -- the target builds end() once and copies it
    into erase's parameter after remove's call (include/vector's erase comment: a const-reference level
    forwarding to it gets the order, 4 instructions off, but loses CGrid::Destroy);
  - still open: the deque's copy constructor chain and pop_back, the map constructor's allocator
    temporary (an inline allocator() costs the deque's out-of-line 0x2CEE30), the class vector insert's
    end register, remove_if's find_if copies (0x5F8AF0).
- **Tells from the HUD, round 16** (the weapon selector's constructor and OnOpen, three initialisers):
  - the seed reads a virtual tail call (`jr t9`) as a jump table and merges the next function when only
    a vtable references it: a "dead `jr ra`" after a tail call is a function of its own (0x36B1F0 and
    0x37BF40 split, the AddToRdr writers; a map-wide scan found no others);
  - a copy into a saved register that dies early means the variable is reused at a later call (MW
    folds that use into the same register: PlaceWeaponText's seal read again at the third call);
  - `return;` closing the last block inside an `if` gives `b epilogue; nop` over the pad; an error block
    that must sit ahead of the body wants `if (a && b) { ...; if (!c) { err; return; } } else { err;
    return; }` then the body (OnOpen);
  - defining a unit's dynamically initialised statics in order makes MW emit its `__sinit_<file>`
    byte-exact, reusing a just-stored value for the next initialiser (hud_weaponsel, hud_teamnames,
    hud_teammatenames);
  - `int len; const char *s = ...; len = strlen(s);` gives `s` the lower saved register;
  - A THUNK IS EMITTED WITH A VTABLE THAT USES IT: when CCheckBox overrode UpdateUIVar, zui_2d.cpp
    stopped emitting C2DButton's thunk 0x37D0E0 (C2DButton's own table goes with ~C2DButton's
    unit); defining CHandlerButton's destructor in zui_2d.cpp (its table 0x407700 keeps the
    button's slot, as CRadioStrip's does) brought it back, with @192@~CHandlerButton. A partial
    `--files` check cannot see a function that stops being emitted -- only the full check does.
- **Tells from the weapons, round 8** (CMesh's decals and LODs, CZProjectileList, the decal pools):
  - MW 3.0 pads a short loop's head with nops when the found block ends in `break`; with `return`
    inside it gets no padding (RemoveMeshDecal, RemoveDecal);
  - a local initialised from a member after the tests that read it takes the register those tests
    loaded it into; declared before them it lands elsewhere (GetNextFreeProjectile);
  - SOCOM II's class-vector operator[] reads the data pointer through begin() and keeps the index as
    its own value (AddLOD's `m_lods[size()-2]`); a class-vector walk re-reads the data pointer each
    pass (CMesh::SetLOD, 2 off: include/vector's class end() lets MW hoist the whole end) -- both
    requests to the MSL lane, with the class iterator's missing operator->;
  - `d = a(); d += b();` keeps a two-call float sum in the result's own register (GetDamage);
  - SOCOM II inlines CZProjectileList's FreeIdx and FindLastUsed and keeps no copy; 0x3BF9B0-0x3C356F
    (where vis_texfx.cpp should be) is ~0x3BC0 bytes against SOCOM 1's 0xAD0 -- mostly new code with
    no vtables, CTextureFX's Tick/ScrollTexture/engine init among it.
- **Tells from the UI, round 6** (the soft keyboard, CSaferStr : std::string):
  - zui_spec.h's CSaferStr is include/string's basic_string now (its copies and destructor implicit,
    +3); its operator=(const char *) returns nothing -- the out-of-line copy 0x22DCC0 keeps no `this`
    for a result (returning `CSaferStr &` lost it at the merge);
  - a file read and freed around a block the target keeps in a stack slot is an object holding the
    file, with a destructor that frees it (ZuiInit);
  - `!f()` of an int call materialises (sltu, xori) inside an `&&` only through an inline bool
    returning the negation (OnEnterKey);
  - a member pointer to a virtual function is pooled as {0, slot offset, vptr offset}:
    `&C2DButton::SetNormal` is {0, 0x54, 0xC}, stored identically at 0x3E0468 and 0x3E0478;
  - basic_string's destructor is inlined in tail code (resize<CDesignDlgObj>) and called in a unit's
    own functions (~CDesignDlgObj), so include/string must not pin it (a request to the MSL lane);
  - units: zui_skb.cpp 0x38B180-0x39039F (ending on its `@128@~CBmpWithNode` thunk, as SOCOM 1's;
    initialiser 0x403730); 0x3903A0 on is another keyboard unit (the Korean input rules, the props
    parsers; 0x403800 builds its eight CBmpWithNode globals); 0x3A3100-0x3A5500 localisation
    (LOC.rdr, LanguageChoice); 0x3A59C0-0x3A5CB0 thread helpers; 0x3A5CC0-0x3A71A0
    CSimpleStateMachine over cdeque<const void *>.
- **The inlining mode, measured** (a measurement lane, after the sound lane's zsnd_queue.cpp):
  - no MW mode both emits a unit in reverse and auto-inlines only earlier definitions (17 builds,
    3.0b22-020926 to 3.0.1b210): `-inline ...,deferred` (= `#pragma defer_codegen on`) emits in
    reverse and inlines in both directions; non-deferred emits in source order and inlines only
    earlier-defined callees; mwldps2 keeps object order, so the image's reverse order is the
    compiler's; `defer_codegen` toggled mid-unit flushes the pending functions, it combines nothing;
  - the image itself refutes "earlier only": C2DBitmap::SetColor(fff) (0x364930) is inlined both by a
    caller below it (0x364900) and by the constructor above it (0x364C50) -- only a deferred inliner
    does that; master's flags stay (non-deferred over the tree: 3,818 of 4,214 against 3,999);
  - CLUSTERED PINS CAN MEAN A UNIT BOUNDARY: auto-inlining never crosses units, so a cut that every
    pinned pair straddles and no inlined pair does is a candidate split. build/scratch/inlmode/
    split_scan.py (and split_best.txt, its results over the 60 pinned units) found such a cut in 33;
    strong ones: zsnd_csndsequence.cpp at 0x349C70 (7 of 7 pins), hud_objectives.cpp at 0x206F90
    (6/6), graph_tree.cpp at 0x2D12D0 (5/5), int_main.cpp at 0x2DC660 (5/5), zrdr_edit.cpp at
    0x333540 (8/9), body.cpp at 0x28F190 (14/18). Each needs the template-placement check:
    ai_mapio.cpp's 20/20 cut at 0x51AEF0 fails because read_v19-v23's ReadCellTable instances close
    the range above it;
  - THE SPLIT LANE (2026-10-25) adopted four, each scoring as before with its pins gone:
    zsnd_csndsequenceentry.cpp 0x349A70-0x349C6F | zsnd_csndsequence.cpp (the press beta's two
    zsnd_csndsequence.cpp asserts are CreateSequence's and ~CSndSequence's); graph_main.cpp
    0x2D04F0-0x2D12FF | graph_tree.cpp 0x2D1300-0x2D288F (its initialiser 0x401C80 builds the tree
    pool, ~CGTreePool 0x2D2830 last, as SOCOM 1's __sinit_graph_tree.cpp); int_main.cpp |
    int_shapes.cpp 0x2DC7A0-0x2DDC2F (diNorm's initialiser 0x401D80, byte-exact as
    __sinit_int_shapes.cpp); grid_main.cpp | grid_cell.cpp 0x2D79D0-0x2D7E9B (CCell). New names ours;
    the half SOCOM 1 names (initialiser, asserts) keeps the old one. tools/unitscan.py rescans one
    unit (pinned pairs, inlined pairs, the target's own calls, every cut);
  - the scan's cut is only the lowest address of a feasible range; the exact one comes from other
    evidence: an inlined pair a draft shows (~CGRoute, 0x2D1210, inlines Clear: graph_tree's cut is
    above it), a call only a boundary explains (SetCDIBBox calls CDI(), which inlines into any caller
    of its unit: int_main's cut is SetCDIBBox's first word, and SOCOM 1's int_main.cpp asserts end at
    DiIntersect(n) just below), the data a half alone uses (diNorm), whole classes;
  - SOCOM 1 is -inline deferred as well (zcam_main.cpp's DWARF lines fall as addresses rise), so an
    EMPTY function SOCOM 1 calls out of line is a unit boundary there: its CGrid::AddClutter and
    DeleteClutter call CCell's empty stubs (0x2170E0, 0x2170F0). The other way round, what SOCOM 1
    inlines is one unit there: zrdr_edit.cpp's wrappers inline push and the add*node in SOCOM 1, and
    SOCOM II's push calls addstringnode inside any upper unit -- no split;
  - rejected: body.cpp's cut falls inside its own tail (RTList's Add/Remove and _close_enough<f>
    above it are SetBlendAnims' and Normalize's below; the 14 are calls to the flag helpers
    0x28F190/0x28F1B0, which sit in the tail among the instances: out-of-class template members,
    likely); valve_list.cpp's remove_if (0x352500, above) is Close's, Reset's and Uninit's (below);
    con_ps2.cpp and ai_util.cpp inline no own function anywhere -- a unit without auto-inlining, not
    two units; zcam_main.cpp's pinned pairs are all in SOCOM 1's DWARF zcam_main.cpp;
    seal_control.cpp's "cut" is Cross's copy after its first caller; ai_map.cpp's pin is its
    initialiser's (tail code); zanim_anim.cpp and node_assetlib.cpp explain 2 of 5; the small ones
    (zsnd_csnd, node_saveload, zsav_manager, zrdr_archive, game_state, game_main, fts_action,
    fts_net_bomb, fts_net_platform, seal_create) pin one or two callers' calls to 0x88-0x1F0-byte
    callees -- their statement counts explain them as well, and SOCOM 1 calls the same callee out
    of line where it has one (RemoteBombPostUpdateCallback, PreCreateSeal, LoadPalettes_PS2);
  - compiled-out statements (debug prints) still count toward the inline limit: DumpQueue/DumpFree
    keep their pins;
  - generation order changes code as well as inlining: zlib's inflate_trees_dynamic loses 8 bytes
    non-deferred, its static callee huft_build already generated.
- **Tells from the kit and AI states, round 8** (seal_tasks.cpp, the task commands):
  - SOCOM II's own command table names the task handlers: CSealCtrlAi::RegisterCommands (0x5DE320)
    makes 34 AddCmd calls, 92 Parse/Begin/Tick/End handlers named after SOCOM 1's class for the same
    command name (CTCreate, CTDrain, CTGroup, CTTurret ours: SOCOM 1 has no such commands);
  - ONE REGISTER, 13 HANDLERS: every handler that uses its command block after the inlined find
    misses on that block's register alone -- the target puts it in s1, between the find's list (s0)
    and its counter (s2), every spelling tried here gives s4 (CTHold, CTWait, CTStance, CTPursue,
    CTMacro, CTGroup, CTLookAt, CTGoto); cracking it should close most of ~60 handlers;
  - an inlined find ending on `if (!(bool)p) break; ... return 0;` gives the target's materialised
    (sltu, xori) exit after the loop; `if (!p) return 0;` branches inline; declaring the counter
    before the list gives the list s0, the counter s1;
  - `seal ? SealAi(seal) : 0` with the call in an inline of its own keeps the result in both arms
    (`b; move s0,v0` / `move s0,zero`); straight in the ternary MW puts the zero in the delay slot;
  - fields a parser reads before its allocation come from an inline Create(): its by-value arguments
    are evaluated before its body (CTStopAll, CTComm);
  - in scripts, test scdiff's verdict as a line starting `  MATCH`: relocation lines say MISMATCH;
  - units by SOCOM 1's initialisers: seal_kit, seal_sensors (0x5C9400-0x5CDBCF), seal_tasks
    (0x5CDBD0-~0x5DEEBF), seal_weapon (probably from ~0x5DEEC0), anim.cpp (0x5E0030-0x5E622F, AnimSet:
    the demo has its DWARF), remote_control, seal_unit, seal_aistate, seal_aievent, seal_aimoves,
    seal_aifollow, seal_squirm, seal_ailines, seal_handsig, seal_aiscan, seal_airesponse; 0x60C170-
    ~0x60EDFF is SOCOM II's CAiGroup with act_camera/act_zanim/act_anim/act_sound.
- **Tells from the AI core, round 9** (the line callbacks, SetCellLink and the links, char_type.cpp):
  - an inline is cut to one level only when its tree is 6 or more high: links::Remove inlined whole
    until Find compared through a `link == CAiMapLoc` operator, one level more; under such a cut even
    CAiMapLoc's implicit assignment (0x52F3C0) and a mem-initializer constructor are called out of
    line;
  - in `b > a ? b - a : a - b` MW loads `a` first, in `a < b ? ...` it loads `b` first: the
    comparison's right operand is evaluated first;
  - the soil test written out in the caller reuses its load for the next compare; through
    CAiCell::IsTraversable it reloads (_TravelLineCallback);
  - two inline calls compared directly are evaluated left to right; locals in the target's order fix
    it; `float h = 0.0f; float cover = h;` gives the target's mtc1 then mov.s;
  - a bit-test `||` materialised and then branched on comes from an inline bool helper;
  - 0x530980 is the links' back(): its old s1:exact name (CAiPath::point's back) matched only because
    the two compile to identical code -- another case of words not proving the instance;
  - units: SetCellLink's out-of-line copies follow it (0x530980-0x530BCF); ai_maptable.cpp's tail runs
    _min<int> 0x5311C0, a sort helper 0x5311E0, the sort 0x5312D0, erase(T *) 0x532000; the walker
    unit's tail 0x539BC0-0x539E9F is the list<CAiMapLoc> pieces (FloodFill's); _find_look/_find_cover
    (open: the target inlines the open-cell helper one level, as if a depth budget were spent from the
    call site).
- **Tells from the states, round 15** (fts_missionlist.cpp, CGraph::BuildTree, st_core.cpp's strings):
  - a function calling dozens of trivial pieces out of line while inlining others is inlining big
    helpers one level: BuildTree needed three (the graph's reset, the queue's pop and update);
  - a helper's named locals take stack slots ahead of its call temporaries;
  - under deferred inlining `#pragma dont_inline on` around one function does nothing;
    `#pragma inline_max_total_size(N)` acts, but cuts calls deep inside the chain (not the target's
    pattern);
  - a plain function template's instance follows its first caller, its own callees inlining by the
    normal rules (FreeVector<cue vector> 0x1ECF60);
  - `c = f(); c += a*b;` emits `f + a*b`; `f() + a*b` emits `a*b + f`;
  - `bool b = (x & M) ? p == q : false;` branches on the raw AND with zero in the delay slot; the `&&`
    form converts the AND to bool first;
  - an inline find returns through a result local (`m = 0; ... m = *i; break; ... return m;`) to inline
    into callers as the target does;
  - `delete` of a class with a derived-string member gives two null tests (delete's and the
    destructor's); an extra `if (p)` makes three;
  - include/list gained the base iterator's postfix `operator++(int)` (additive); the pointer list's
    begin() written `iterator(base::begin())` (FindNode, Destroy) and list's remove_if shape (0x2D2540,
    0x20CB50) are header requests;
  - units: fts_missionlist.cpp (SOCOM II-only: the multiplayer mission list, then CMsgEditor, up to the
    patch downloader at 0x2CA330); graph_tree.cpp runs to 0x2D2830 (BuildTree, its copies, FindNode,
    Destroy, then the queue list's template code).
- **Tells from the sound, round 5** (zsnd_queue.cpp, zsnd_positions.cpp, CSched_Manager::Tick):
  - zsnd_queue.cpp's calls looked as if the original auto-inlined only functions defined earlier in
    the unit; the measurement that followed (below, "The inlining mode, measured") found it was a UNIT
    BOUNDARY: CSndQueue/CSndQueueEntry (zsnd_queue.cpp, 0x34C560-0x34CD6F) and CSndVocalQueue
    (zsnd_vocalqueue.cpp, 0x34CD70-0x34D17F) are two units -- split, they match with 2 pins instead
    of 8 and CSndQueue::Open (0x34CC00) is byte-exact;
  - an empty walk with a postfix step, `for (i = begin(); i != end(); i++) ;`, gives the loop with its
    entry test after the body (all four empty walks first time; the prefix form does not);
  - `(p = new T)->field = x;` stores through new's result before reloading `p` when p is later pushed
    by reference; a pushed copy, `T *q = p; v.push_back(q);`, gives the target's reload into the next
    stack slot; a pushed copy taken before a call is saved ahead of it and reloaded after, taken
    after the call the original stays in a saved register;
  - a vtable slot pointing into the middle of a map row means the seed merged a tail call with the
    function after it (0x34CD10, CSndQueueEntry::CanPlay, split from 0x34CCF0);
  - units: zsnd_positions.cpp 0x34BD40-0x34C55F (initialiser 0x403370, tail destructor 0x34C4F0);
    zsnd_queue.cpp 0x34C560-0x34CD6F and zsnd_vocalqueue.cpp 0x34CD70-0x34D17F; 0x34D180-0x34E06F is a separate unit (two tables of 12-byte
    records, closed by their reserve at 0x34DED0; CSnd::Init builds its global at 0x48DC30);
    zsys_sched.cpp starts at 0x34E070; 0x34BCA0-0x34BD10 belongs to the headset unit.
- **Tells from the weapons, round 7** (the turret's template tail, CZWeapon's sounds/timers/AI params):
  - a class temporary in an argument list whose type has a user destructor (even an empty inline
    one) is built before the inline end()/size() are expanded; without a destructor MW expands
    those first;
  - a user-declared copy assignment on a member's member makes the enclosing class's implicit
    assignment a real function, called out of line once the outer assignment passes
    inline_max_size; left implicit it is a block copy;
  - a class with a user-declared constructor gets a member-by-member implicit copy constructor while
    its implicit assignment stays a block copy -- the copy constructor and the assignment can copy the
    same member differently (CTurretAiParams); an empty user constructor on a vector element makes
    insert copy it member by member (CZDecalDef);
  - a tail instance an earlier unit already emitted is deduplicated by name, so a unit's own use of a
    vector destructor can be invisible in the image (zwep_weapon.cpp's DestroyMarkList calls
    0x2CDB30, an earlier unit's copy);
  - DEAD-STRIPPED STAND-INS: an insert inlines the element destructor and clear only if the unit's
    own code generated them first; where no such caller is left in the image, a minimal stand-in for
    the dead-stripped original (no row: it scores as no-target) restores the order --
    DeadTurretDefinitionsKill made the turret's insert byte-exact (veh_turret.cpp), and
    DeadVehicleDefinitionsKill gives 0x3AF040 the target's size (its end-pointer register still open);
  - veh_turret.cpp's tail starts at resize 0x3AB8C0; CTurretDef() (0x3AB5F0) is an ordinary function
    right after Parse, as in SOCOM 1.
- **Tells from the HUD, round 15** (the weapon selector's states and scrolling, zui_2d.cpp complete):
  - an inline's side-effect-free argument used once on each path is substituted into each arm, so
    SOCOM 1's `Increment(2*(475*dT), ...)` computes the product in both arms, as the target; the
    operand order needed `2.0f * (float)(475.0f * dT)`;
  - a CPnt3D struct copy reloads its source; written field by field, the first field reuses the load
    a preceding compare made;
  - `(f(4, pad) == 1) != false` is the one form tried that materialises an `== 1` test (andi; xori;
    sltiu) inside an `&&` chain;
  - where the target keeps a sum's non-product operand first, that operand must be in a local;
    `int x, y; y = ...; x = ...;` gives the target's saved registers where initialised declarations
    in either order swap them;
  - an inline const accessor reuses the type byte its caller tested only through a const view (`const
    CUIVariable *v = var`); through the non-const parameter it reloads it;
  - a secondary vtable sits at the primary table's size, so stand-in classes need the real slot
    counts (C3StateButton 29, C2DString 34, C2DBitmap 26);
  - THIS-ADJUSTING THUNKS have MW's names `@<offset>@<target>` (`j target; addiu a0,a0,-offset`):
    zui_2d.cpp's classes emit `@192@__dt__9CCheckBoxFv` and its kin for their secondary bases, and
    naming the rows 0x37D020-0x37D0E0 so made eight of them byte-exact (a thunk's row named after the
    function it jumps to and the offset it subtracts);
  - zui_2d.cpp's tail order: ~CCheckBox, ~CHProgressBar, ~CSliderBar, ~CHandlerButton,
    C3StateButton::Tick, ~CTeleType, then the thunks.
- **Tells from the online lobby, round 6** (the account save module, the universe list, the LAN lobby):
  - a temporary is laid out below the named locals: the target's struct sitting below an inner
    block's object was `push_back(T(args))`;
  - walking `Item(unsigned)` with an int counter recomputes each address, and dropping the
    strength-reduction register moved Add's whole register allocation;
  - a class vector's one-element erase is an out-of-class `T *erase(T *)` behind an inline iterator
    wrapper (a class return would pass a hidden return slot) -- include/vector has it now, beside the
    pod vector's;
  - copy construction of a record whose members have constructors goes member by member while its
    implicit assignment stays a block copy (AcctMission: no constructor of its own, its name member a
    class with one);
  - `&r->m_missions[i]` keeps an advancing base and adds the member offset each pass; a pointer
    cursor advances the member's address instead;
  - `!x` passed to an int parameter gives sltu/xori/andi, `x == 0` sltiu;
  - units: the save module zNetMediusUniverse.cpp 0x2FAFC0-0x30066F (CZUniversInformationServer::
    CAcctDB, a CSaveModule and a CLobbySaveModule by its RTTI; PasswordSecurity 0x2FAF50 ends the
    lobby unit); the friends unit ends at 0x30372F (its tail: reserve 0x3030B0, resize 0x303250, the
    72-byte erase 0x303490); zNetLANLobby.cpp 0x303730-0x30460F (initialiser 0x4024F0); the Medius
    file API 0x304610-0x30661F (CLobbyFileIO, vtable 0x406760, initialiser 0x402570) waits on the
    string-keyed map's compare 0x2DEE30.
- **Tells from the kit and AI states, round 7** (the kit's item vector, the event state, the callouts):
  - a state destructor with byte-array members matches with the member destructors called by hand in
    reverse order; a vector's goes behind three nested `if (v)`, and MW keeps all three tests;
  - in `if (A && B) ... else if (C && D)` where B is a materialised `&&`, write B as an inline bool:
    the `&&` stays materialised and a failing B still falls through to C;
  - a field the target loads again for a third test, after one shared load for the first two, is
    tested through an inline accessor;
  - the remainder pass of an unrolled loop over a struct array folds the field offset into the
    relocation unless each element goes through a pointer local (`T *c = &a[i]; c->f = 0;`);
  - `*p = c ? enumLocal : K` reuses the enum local's register for K; an if/else does not;
  - a byte-enum call result passed on unmasked and tested with andi: the callee's return and the
    receiving parameter the same unsigned byte enum, compared `== ENUM_0`;
  - SOCOM 1's CAiTrigger() inlines one level, as the target needs, only when Rfloat's range
    constructor is inline (out of line, the default limit inlines it whole);
  - MW lowers a switch over {2, 4, 5, 12} to a compare chain; the target's bit test (`(unsigned)(k-2)
    < 11 && (1 << (k-2)) & 0x40D`) has to be written out;
  - reading Rfloat-shaped members through a cast keeps their address in a saved register; declare them
    with the right type;
  - units: ai_target.cpp (0x609DF0-0x60BF3F: CheckLineOfFire and CAiTarget, where SOCOM 1 has them),
    seal_weapon.cpp (~0x5DF370-0x5E0023, CZSealBody's weapon functions before AnimSet's), the
    callout table's functions in seal_airesponse.cpp; 0x5EA550 (CAiTimer(float)) and 0x5EA5D0 are
    copies CSealUnit::IssueCommand emits.
- **Tells from the rdr lane, round 4** (the save manager, CPipe's overlay and FIFO, the flare system):
  - `buf[i] ^= key` on an unsigned char key sign-extends it, `c ^ key` zero-extends it; `key ^= key +
    c` masks the sum before the xor, `key = key ^ (key + c)` does not (SaveDecrypt; SaveEncrypt is
    open on the key's copy masked first and the raw key sign-extended);
  - a member's address kept in a saved register across a loop needs a local pointer taken inside the
    size guard; through the member, an inline's pointer parameter or an inline member's `this`, MW
    adds the offset at every call;
  - frame slots follow declaration order (Load and Save declare the header first, above the buffer
    and the archive); a comparator that is an inline's local sits below begin()'s temporary, the
    caller's own local above it;
  - `ok = true;` written before a call lands in that call's delay slot;
  - a constant loop of 14 is unrolled 7x, not whole: write the stores out when the target has all
    fourteen;
  - converting a float to an unsigned char inlines MW's float-to-unsigned check; through `(int)` it
    is the plain `cvt.w.s`;
  - reading an interrupt's command through a volatile pointer reloads a field after its test;
  - a local cannot be named `list` inside a member of a class derived from std::list, nor `zar` where
    namespace zar is in use: MW fails to parse;
  - include/string's c_str() written short form first (`!is_long() ? s_.data_ : (const charT *)w_[2]`)
    makes CSaveManager::Load (0x33D6D0) byte-exact (measured on a shadow include/ only);
  - zrndr_pipe.cpp opens with the flare system (0x336230, the object at 0x488F10); 0x33C2A0-0x33CD4F
    looks like its template tail (a vector of 112-byte elements); zsav_manager.cpp ends with SOCOM 1's
    list sort<less<const void *> > and merge (0x33E510-0x33E7EF); a further unit runs
    0x33E7F0-0x33F04F (helpers UICharacterProfile calls).
- **Tells from the AI core, round 8** (the path's point vector, SmoothPath, the visibility tests):
  - a chain of `==` tests joined by `||` becomes a bit test; the target's two materialised ors come
    from explicit bools, the outer bool declared first and the value last;
  - literals passed to a `const float &` (`_saturate(&v, 0.0f, 1.0f)`) get a per-function constant
    pool in data, 8 bytes apart; a function inlined into another shares that pool, so a shared pool
    reveals the inlining (0x538D10 uses 0x538E30's);
  - a trim temporary's copies show its type by this-test count: `std::vector<T>` has two, the
    `points` subclass three;
  - chance odds stay in the frame across rand() only with the Rfloat-shaped random; the plain
    `lo + span * (k * rand())` keeps them in a saved float register;
  - if a unit's own code instantiates a class vector's erase, the tail's resize inlines it; the
    target's resize still calls erase, so the original MSL probably declares resize ahead of erase
    (RemoveInvalidPoints kept out of the build under `#if 0` for it);
  - SOCOM II's CAiPath::point is 8 bytes with a written copy constructor (cell, width, flags copied,
    the rest zeroed); CAiMaps::GetCellCenter returns the point; the walker unit holds CAiMaps
    functions too (0x5352D0, 0x538980-0x538EF0).
- **Tells from the states, round 14** (the graph's BuildRoute/PopulateTree, the objective cams):
  - an rvalue (an accessor's result) passed to a list insert binds a temporary whose stack slot comes
    after the call's outer temporaries (BuildRoute's `insert(it, leaf->GetNode())`); a named local
    or the member itself gets the wrong slot;
  - an element address the target recomputes every pass needs an unsigned index; an int index is
    strength-reduced into a saved register (0x2AF360);
  - `CMissionCam();` written inside a constructor's body builds and destroys a temporary (both builds
    do it in CMissionObjectiveCam());
  - an end iterator declared before the `for` takes the higher saved register (PopulateTree);
  - zNetRegisterSeal (0x6C0 bytes) matched on the first try once TypeSealState's fields were laid out
    from its 38 registrations (0x90 bytes);
  - graph_tree.cpp continues past BuildTree (CGQueue's push 0x2D1DA0 and find 0x2D1DE0, the
    CGQueueAtom list's remove 0x2D1E30); BuildTree and SetMovie (0x1EDA60) call ~30 trivial MSL
    pieces out of line, as if compiled with inlining off (untested); 0x2C8760-0x2CA32F is a SOCOM
    II-only multiplayer mission list unit; st_core.cpp's tail 0x1EE000-0x1EF8C0 holds std::sort and
    MSL string members (0x1EEB40 assign(const char *), 0x1EEB70 an append through replace,
    0x1EF390 replace(pos, n, s)); 0x440B80 is "ExtraStringTables", so 0x361BC0, 0x3A3900 and
    0x3A4460 are the string-table manager's.
- **Tells from the SEALs, round 8** (CPickup's Close/Create/NetworkCreate, the AI sound statics):
  - an empty functor passed by value needs a user-declared empty constructor; without one MW
    zero-stores its byte on the stack (CPickup::Close's std::transform);
  - fields stored through a single load of a pointer that lives in memory (its address taken for an
    insert) come from an inline member; written as `p->` statements, MW reloads `p` after each store;
  - MW uses byte access for a bitfield only when its container is `unsigned char`: `unsigned int`
    bitfields use lw/sw even inside one byte (CPickup's flags);
  - a bool read from a bitfield and tested later (`sltu` kept) needs an inline accessor written
    `? true : false`; `return bit;` folds the test away;
  - in a swap, `t = a; a = b; b = t;` against the reverse decides both the float registers and the
    store order;
  - deriving a member class from CBits adds a second this-test to an inlined `delete`; a stand-alone
    class of the same layout does not;
  - 0x540270-0x541640 is char_type.cpp's tail (SOCOM 1's order puts it right before
    pickup_main.cpp); 0x5B8F20, 0x5B8FC0, 0x5B8FF0 are CZKit methods on the vector at kit+0x254, before
    seal_kit.cpp (from 0x5B9020).
- **Tells from the UI, round 5** (the button specs' copies, the ActionItem and CDesignDlgObj vectors):
  - the UI classes copy as `T(const T &o) : a(0), b(0) { *this = o; }` (ActionItem, CDesignDlgObj);
    a vector's range init builds each element that way and calls the assignment out of line;
  - an implicit copy constructor too big to inline whole is inlined one level in each Copy, its
    members' copies called; its own out-of-line copy, emitted where a derived class needs it,
    inlines everything;
  - loads written into separate call statements are not hoisted above the earlier calls: the four
    state names read into locals first match InitializeButton (both builds keep them in saved
    registers);
  - `next = erase(i); delete obj; i = next;` keeps erase's result in a saved register, as the target;
  - a string member of a global read through a reference local builds its address once; written
    straight, MW addresses each word absolutely;
  - a same-unit function the target always calls out of line needs `#pragma auto_inline off`
    (InitializeButton, or the CreateDisplayObjs regress; the template tail's CDesignDlgObj
    assignment);
  - include/vector gained the pod vector's single erase(T *) (SOCOM 1's 0x1DEF80, SOCOM II's
    0x28F220) and the pointer vector's erase(iterator); the class erase(first, last) wants MSL's
    destroy loop after the copy (`for (T *p = first + n; p < e; ++p) p->~T();`) -- erase<ActionItem>
    (0x383E00) byte-exact in a probe, left to the MSL round;
  - ~ActionItem (0x3890C0) is zui_cleanup.cpp's first-defined function, the last of its own code in
    the image; after it the unit holds only the inline-virtual copies and implicit destructors, in
    vtable order.
- **Tells from the online lobby, round 5** (the lobby's state handlers, the chat, the room requests):
  - all 16 state handlers share one switch on the byte flag, every case empty but ENTRY: it compiles
    to the compare chain 4, 3, 2, 1, 0 plus a `b`; the handlers are named by the slot the table at
    0x2ED460 registers them in, against SOCOM 1's Open;
  - an int-sized enum (a 0x7FFFFFFF extent) assigned a ternary of two enumerators gives `li 1;
    movz`; an int gives `sltu` (JoinType, ChatToggle);
  - a byte-enum local is assigned with `li` and compared through `andi 0xff`; passed to a parameter
    of the same enum it is not re-extended, an int parameter adds the andi (cbChatMessage's colour);
  - `case 0:` re-assigning the initial value keeps its own `b` block; a bare `break` is threaded
    straight to the join;
  - a valve's 16-bit value read through an int-returning accessor serves the test and a following
    sum with no mask; read as the bitfield or an unsigned short member, the sum adds `andi 0xffff`;
  - a struct of plain members with a user constructor (NetConnectionInfo) copies member by member:
    a nested plain struct as FPR words, an int array in an eight-pass two-word loop, char arrays
    bytewise;
  - CopyField's null test of the destination is not folded for a member array of a local struct
    (four functions); a source-only test or a plain memcpy fits (zonline.h's older note said the
    local array's test folds away);
  - a member function called with a0 unset is static (PasswordSecurity); a request declared inside an
    `if` block takes a lower stack slot than case-scope locals declared before it (declared at case
    scope in the target's order: AccountLoginStateHandler);
  - the chat unit opens with the state machine (the table 0x2ED460, handlers through 0x2EFE50), then
    the player-info pages, then the chat; functions its own callers call out of line are pinned with
    `#pragma auto_inline off` (0x2F3FF0, 0x2F4FC0, 0x2E3AA0, 0x2E2920, FindRoom); the Medius client
    calls (OVL2 0x647548-0x649960, GCC-built) are named from SOCOM 1's call sites.
- **Tells from the HUD, round 14** (the weapon selectors' Inits, the team names, InitHUD_Compass):
  - a guarded function-local static that stores a constant had a non-constant initialiser: MW 3.0
    initialises `= 48.0f` or a `const float` statically, with no guard; an inline call returning 48
    reproduces the guard;
  - a float immediate that is not the nearest float to its decimal is a folded product: 0x3F5AE147
    is `0.9f * 0.95f`, where `0.855f` gives ...148;
  - several loops in one function: a shared function-scope counter against separate `for` counters
    re-colours the later loops; a counter declared ahead of an initialised local takes the higher
    saved register;
  - a reference local whose address is pushed lives in a stack slot and is reloaded before every
    store through it; storing through an inline that takes the pointer keeps it in a register;
  - MW forwards a constant just stored to a member into a later expression only if that expression
    comes before the register is reused (CInGameWeaponSel's backdrop: its top computed before its
    left);
  - hud_mapcompass.cpp starts at 0x2113D0 (the three anchor accessors before Tick in the image, so
    defined last); hud_teamnames.cpp runs 0x220D00-0x221BF0; RefreshStatics follows
    ~CZObjectiveList (defined last in hud_objectives.cpp); st_mpexit.cpp (CMPExitState,
    0x223290-0x2265F7) sits inside the HUD's range.
- **Tells from the zdb world, round 6** (CDI(), the grid's traversal boundary, znode.h's stand-ins):
  - znode.h's list stand-ins (PtrListFirst, ListBegin, ListEnd, ListClear) are gone: include/list's
    begin()/clear()/insert() give the same code in all six of their users;
  - CDI() inlines SOCOM 1's DI_PARAMS constructor (tag_DI_HEAD's, with eleven flag setters);
  - the &size_/&data_ register order of a local vector walked after a call turns on whether the
    pod vector's end() reads through data(): `return data() + size_;` gains CGrid::Destroy,
    GenerateLandmarkList, DeleteChildren and AddPalette but makes CCharacterType::Create one
    inline level too tall (measured over every hand-written unit; the pointer vector's end() through
    __base::data() gains the same four and loses find_node, whose target calls the pod's end() out
    of line) -- not applied, an MSL round's to settle;
  - a swap reloads the element just tested only if it reads it through a different type (a
    `CClutter *` swap reuses the tested value);
  - "the inner loop entered through a block placed after the function's end" is not reproduced by
    any for, while, counter or test form tried (SortNearToFar, CGrid::Create);
  - grid_di.cpp's static stack is `CStack(false)` (zmath.h's CStack(bool): the initialiser clears
    the owner byte and pushes nothing), which makes __sinit_grid_di.cpp byte-exact; 0x306620 is
    CFRect(const CPnt3D *, const CPnt3D *), top and bottom from the corners' z;
  - try.py substitutes files under src/ only: to probe a header under include/, compile against a
    shadow copy of include/ placed first (build/scratch/world/incprobe.py).
- **Tells from the body animation, round 4** (ReadMotionRdr, the blend stack, BodyAddAnim):
  - an inline accessor returning a member's address makes MW hoist that address out of a loop,
    into a preheader placed after the loop's bottom test, the loop entered with a `b`; written as
    `p->member`, the address is recomputed every pass;
  - an inline wrapping an out-of-line call gives its result the lowest saved register, below
    another inline's `this` temporary (PushBlend); ReadMotionRdr (0xE98 bytes) closed on one such
    inline, `CZSIScriptCommon::LoopTime()`, whose `this` copy takes s0;
  - `mul.s $f1,$f1,$f0` with the first operand loaded after a call is a by-value inline parameter
    multiplied in place (`t *= s; return t;`, called as `f(member, call())`);
  - two vectors walked in step: both cursors class iterators, the walked vector's initialised
    first (BodyAddAnim);
  - a bool set true/false in an if/else puts `daddiu` in the branch's delay slot; a float
    comparison returned as `if (c) return true; return false;` gives the target's label pads where
    `return c;` does not;
  - MW puts a division's dividend in $f1 and its divisor in $f0 whatever the spelling (ThisFoot and
    CBlendOverlay::Tick want them the other way round: open);
  - inline costs measured by `#pragma inline_max_size` bisection: CPnt3D's implicit assignment ~49
    units, CQuat's ~30, a user destructor 17, an empty transform over a pointer vector 80 --
    CBody::Destroy fits only at 299 units, so the original's copies or MSL pieces were leaner;
  - OVL1's copy of the pod vector's resize (0x28B0E0) is scored from source only once its row
    carries `ovl-copy:` naming MAIN's copy (0x18B050): an instance is credited to the copy in the
    unit's own overlay (see "Each overlay links its own template copies").
- **Tells from the states, round 13** (fts_net_platform.cpp, the seal unit's tail, the pool vector):
  - an 8-byte two-int element is copied through the FPRs in a class vector's insert; any
    user-declared constructor, even an empty one, gives the target's lw/sw (both 8-byte-element
    inserts of the round);
  - a vector that reserves at construction is a derived class whose inline constructor calls
    `reserve(n)`: the initialiser inlines it, and the implicit destructor tests `this` four times
    (CZSealState::CRemoteGuyList, CZPlatformState::CPendingList);
  - an element copied by insert/erase moves a byte member bytewise; an 8-bit field of an int unit
    (`unsigned int m_cmd : 8`) moves the whole word, while accesses still narrow to bytes;
  - a value read through an inline accessor takes its saved register after the named locals
    (RemoteGameCreationCallback's s0/s1);
  - an element reference taken through an inline `operator[]` lands after a named local initialised
    from it; through the raw data pointer it keeps declaration order (CreateCompassNavPoints);
  - `f()->m = 1` loads the constant ahead of the call into a saved register; taking the result into
    a local first keeps it after the call;
  - `_zrdr::Item(int)` lets MW walk an int-counter loop's element address; an unsigned index gives
    the target's per-pass recompute;
  - SOCOM 1's FilterNavPointString layout comes only from `if (i < 0 || i > 23) r = 0; else r = i +
    14; return r;`;
  - fts_net_platform.cpp (0x2C7910-0x2C875F, the turrets' network objects, CZPlatformState after its
    "PlatformState" registration -- the names ours) sits between fts_missioncam.cpp and a unit from
    0x2C8760.
- **Tells from the weapons, round 6** (the def vectors, the flyout machine, the decal sets, CZMarkList):
  - giving an element class its real layout and an inline constructor made include/vector's resize
    and erase byte-exact for it (CVehicleDef: 0x3AE750, 0x3AE800 at 0x840 bytes); the class
    inserts stay open everywhere (0x3AF040 8 bytes short, 0x3D3D90 16), and they call the
    temporary's destructor where the target inlines it -- the template tail's generation order;
  - computing a count into a local before zeroing the counter puts the zero in the next call's
    delay slot (both def-vector Inits);
  - a loop with its own loop variable gets a preheader, and MW hoists a stored constant into it
    (PlacedExplosive::PostTick);
  - an inline bool `return !p;` used as a loop test is sltu/xori; an inline returning a bitfield
    compared `== false` gives andi/sltu;
  - an assignment inside the second operand of `||` delays that load until after the first test;
  - `r = 1 + rand() % 990; r /= 10;` builds the sum in the result's register;
  - two loops sharing one counter declared after the count reproduce the second loop's registers;
  - clearing each array element through a pointer local reloads the array base for every element
    (the target's unrolled shape);
  - assigning through an inline accessor returning a reference keeps the add base-first on every
    statement; an inline setter with a by-value argument loads it before the free;
  - matrix diagonal stores keep source order only when written one matrix at a time; CMatrix's
    default constructor is the target's unit-matrix call (no SetIdentity);
  - a global's address taken between two tests lands in those tests' delay slots;
  - seal_env.cpp defines its own CZDecalDef and sees zweapon.h through zseal.h, so CZDecalDef,
    CZDecalSet and CZMarkList live in zwep_weapon.cpp (0x3D0470-0x3D17CF), not the header;
    zwep_weapon.cpp's template tail runs through 0x3D4260 and zwep_ammo.cpp starts at 0x3D4380;
    the placed-explosive pointer vector (0x3D4810-0x3D4B7F) opens the flyout unit.
- **Tells from the kit and AI states, round 6** (CAiStateStack::Tick, SEAL_AI_PARAMS(), the wrappers):
  - a virtual call whose caller never sets $f12 proves the slot takes no float, even when the same
    function sets $f12 for other calls: SOCOM II's state Tick (slot 0x10) is `int Tick(CSealCtrlAi
    *)` (the states read the m_dT static) and slot 0x14 `int Virtual14(CSealCtrlAi *, CAiState
    *sub)` -- 31 rows re-mangled;
  - reading a list's front through `*begin()` lets a later front reuse the `&end_.next_` an erase's
    begin() kept in a saved register (Pop);
  - an auto-inline candidate (15 statements or fewer) too big to inline whole stops every caller
    from being auto-inlined; pinning it (`#pragma auto_inline off`) frees them (PushAiState inlines
    into 0x5F51A0 once the stack's Push is pinned);
  - a unit can declare its own layout-compatible class so members a shared header keeps as byte
    stand-ins get real constructors, built in declaration order (seal_aiparams.cpp);
  - declaring a member class's destructor, even undefined, changes the enclosing constructor (with
    ~CAiPath() declared the route's address stays in s1 across its path's construction);
  - an array of 8 CAiMapLoc (an empty inline constructor) compiles to an empty counted loop; a byte
    enum copied into a local loads with lb;
  - `if (ok && cond) { ... } return ok;` around the rest of a function gives two branches straight
    to the shared return;
  - still open: the `Is()` built whole in a register with an explicit false arm (Pop(ai, AI_STATE),
    PopTo, 0x5F43B0) -- when the second operand is an out-of-line call MW drops the first operand's
    andi; 0x5B818C-0x5B901F is SOCOM 1's seal_ex.cpp and seal_ik.cpp (CSealEx::OnWeaponHit at
    0x5B8260).
- **Tells from the AI core, round 7** (ai_maps.cpp, the map walkers, CAiMapLayer, CAiMap::GetCell):
  - one-level inlining can be a size cut, not a height: CAiMaps::GetCell inlines CAiMap::GetCell one
    level only once GetCell's expanded body passes inline_max_size -- measured by prepending
    `#pragma inline_max_size(N)`: 244-248 units with the index in an expression, past 256 with the
    scanline and column in locals;
  - `if (r != 0)` on a pointer local branches; a bare `if (r)` was materialised (sltu, xori) inside a
    list walk;
  - walking a list through a const view of `this` builds the const begin()/end() iterators in stack
    slots, as the target does; the non-const walk folds them away; taking end() into a local before
    the walk keeps the sentinel in a saved register;
  - a separate inline `operator!=` for CAiMapLoc gives the target's sltu; `!(a == b)` gives sltiu
    with the branches inverted;
  - reading a zeroed struct field after the call through a const accessor stops MW holding that
    field's address in a saved register;
  - a vector subclass with an out-of-line destructor adds one this-test, and the owner's destructor
    calls it with -1 (~CAiMap for the layers);
  - ~CAiPath (0x5281B0) matches only with the stats' visited cells as CAiMapLocVec, which makes
    CAiPath() one inline level too tall at 5: one more level-6 candidate;
  - units: ai_path.cpp's tail runs to 0x5291EF; ai_maps.cpp 0x5291F0-0x52EE9F (template tail from
    0x52EB40), ai_maptable.cpp follows; the walker unit ai_mapwalk.cpp ~0x5350C0-0x539BBF (tail
    from 0x539B70); the look and cover searches 0x539EB0-0x53A72F; the layer code (ai_maplayer.cpp)
    0x53A730-0x53AA4F; initialisers 0x668680 (ai_path.cpp), 0x668710 (ai_maps.cpp, AiMaps at
    0x66A1B0), 0x6687D0 (empty, probably ai_vismap.cpp's).
- **Tells from completing the MSL headers, round 4** (include/vector, include/algorithm):
  - SOCOM II's MSL builds every class-vector element through an inline construct() and moves them
    through std::copy, copy_backward and fill_n, each an inline wrapper over an inline helper: an
    element whose copy or assignment expands past inline_max_size cuts them to calls (placement new
    0x2FECA0 and the copy constructor before it, the helpers' copies in the unit's tail), a small one
    inlines all of it -- a stand-in turret definition with scalar members reproduces 0x3AB5F0-0x3AC6DF's
    whole call structure and order (build/m4/p/turret2.cpp, in the MSL lane's worktree); SOCOM 1's
    insert of the same vector calls the copy constructor and assignment directly;
  - the class erase destroys the elements left past its new end, n (the count after the range) a
    signed int (the UI lane's measurement); MW 3.0 deletes the loop for trivial elements;
  - whether an insert calls its temporary's destructor, inlines it, or inlines it with clear is the
    unit's tail generation rounds (probes build/m4/p/g1.cpp, g3.cpp): a template member is inlined only
    if generated before, requests are generated in rounds (own code's first), and within a round the
    deleter's members come before __vector_imp's. The image's tail order does not show the rounds:
    vehicle_rdr.cpp's clear and destructor follow its insert though generated first (the linker lays
    a tail out depth first). veh_vehicle.cpp, veh_turret.cpp and zwep_weapon.cpp inline both with no
    caller of either left: a dead-stripped own function that clears and destroys the vector does it
    (appended to veh_vehicle.cpp, 0x3AF040 gets the target's size and tail);
  - the in-place branch's end pointer register is still open (CVehicleDef, CRegistrant against the
    float sequence entries and CAiMapLoc, opposite ways; include/vector lists what was tried);
    CRegistrant wants a user-declared constructor first (99 -> 38 lines off);
  - an inline accessor level can be paid back elsewhere: the pod's end() through data() (+4) made the
    pointer vector's end() a level too tall for CCharacterType::Create until it took the base's end
    into a const local, as begin() does;
  - a constructor's inline cost can grow without a level: the pod constructor's zero held in a local
    (the same three stores) pushes CScriptArchive's constructor past inline_max_size
    (__sinit_bone_init.cpp byte-exact), where the stores in a helper cut CHUD(), CCell(), CVisual()
    and __sinit_compass.cpp too.
- **The class vector's deleter destructor and clear are out of class** (include/vector): MSL
  3.0.1 defines both after the class body, not inline (SOCOM 1's DWARF lines 939 and 949), so MW
  emits them in each unit's template tail and inlines them only in tail code the unit has
  instantiated them for (the rule above). vector::clear() and __vector_imp::clear() inline around
  a call to the deleter's clear (CZAnimSetSave::Free, ~C2DTriFan, CEntity::SetUpdateTargetList --
  0x26D2F0 is that clear for CPnt3D, not __vector_imp's), and an implicit destructor chain over a
  class vector stops at the deleter's destructor (~vector and ~__vector_imp inline, the deleter's destructor is a call:
  ~CSndSequence, ~CEntity, CCompass::DeleteAnchor, SOCOM 1's ~CEntity alike), while
  zanim_set.cpp's Unload counts it and the class reserves inline it into their temporary's
  destruction. Probes in build/scratch/vec/ (ip.py, vscore.py; the c*.cpp variants record the
  rules measured on the way: non-template code never inlines an out-of-class template member,
  explicit instantiation makes one an ordinary auto-inline function) -- but measure such changes
  on the real units, which the probes did not predict. SOCOM 1's DWARF names two MSL installs:
  `C:\apps\cw301` (the game's headers) and CodeWarrior 2.4's (the prebuilt MSL library: locale,
  iostreams) -- but SOCOM II's prebuilt library is a 3.0b38+ build ("MAIN's Metrowerks runtime").
- **Tool notes**: scdiff accepts the image's one non-zero pad word (0xFFFFFFFF at 0x1E70F4,
  after main); when a renamed row's re-draft misses, `automatch --stale` now keeps the old draft
  and renames the function it defines (an `sc: as=` line for a template name) instead of
  deleting it -- three matches had been lost that way and restored by hand. A
  hand-written unit's inline copy supersedes a draft only when it is byte-exact (sccheck and
  automatch alike): a copy that misses leaves the draft's match standing, as the linker keeps
  a strong definition over the copies.

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

**Two DNAS clients call DNAS.BIN, not OVL2, and the seed split OVL2 at those calls.**
`OVERLAY/REL/DNAS.BIN` (plaintext, `extracted/overlays/disc/OVERLAY/REL/`, mapped 1:1 from
0x4C5380) is loaded into the OVL2 slot for the authentication. Two pieces of code call into it:
MAIN's boot-loader client (0x1C59C0-0x1C6200, four functions) and OVL1's DNAS client
(0x2CA340-0x2CD360, fifteen functions). Their jal targets in 0x4C5380-0x568180 name DNAS.BIN
functions, and every one that lands mid-function in r0001's OVL2 lands on a DNAS.BIN prologue.
The seed made a row at every jal target, so it cut r0001 OVL2 functions there. Eighteen cuts are
merged back, each row's note saying so:
- the SEAL lane: DecrZoomState, IncrZoomState, SetCurZoomState and six more;
- the AI-core lane: 0x534630, 0x534D40, 0x539D00;
- the integrator: 0x4C6B50-0x4C7718, six.

`build/scratch/seal/fallthru.py` finds rows that run into the next one, and
`build/scratch/seal/ovl2_splits.txt` lists what it found; 0x610010 and 0x643018 there are not
DNAS targets. Scoring either client needs nothing new: a placeholder resolves by the address in its
name when no row carries it (`scdiff._sym_address`), so a call spelled `func_00544550` compares
equal with the target's jal into DNAS.BIN -- spell DNAS.BIN's functions that way, never by the
OVL2 name that covers the address.

### Names from SOCOM 1 (`tools/s1names.py`)

SOCOM 1's May 13 2002 demo (`extracted/socom1-demo-020513/SCUS_972.05`, sha256 `00c9cee7...`,
from the owner's copy of the Hidden Palace prototype) is unstripped: 9,703 named, sized
functions, `.relmain` (115,142 relocations) and Metrowerks DWARF 1 (`tools/dwarf1.py`: 959
units, class layouts, enums). Every carried name says how it was found in its note, and `s1names.py` never renames a row
that hand-written source defines or that already carries an `s1~` verdict:

| note | how | count |
|---|---|---|
| `s1:exact` | same size, identical with only the fields the demo's own relocations name masked; unique BOTH ways; no call whose target both builds name differently (a single agreeing call breaks a tie between wrappers) | 973 |
| `s1:callee` | called from the same instruction of an exact pair, by every such call site alike | 137 |
| `s1:gap` | between two exact anchors, equal counts, sizes within 35 %, paired in order | 74 |
| `s1:align` | the same stretch with unequal counts, anchored on every name: a dynamic programme on instruction-skeleton similarity (>= 0.5, sizes within 2x, a margin over any rival) | 851 |
| `s1:calls` | the same sequence of known callees and the same strings, unique, both Metrowerks | 277 |
| `s1:order` | by hand: same position in the same unit, signature checked against the code | 23 |
| `s1:ref` | taken by address (a handler, a callback) where SOCOM 1 takes that function's (`tools/s1data.py`) | 31 |
| `s1:unit` | a static initialiser paired with SOCOM 1's `__sinit_<file>` (`tools/s1units.py`) | 7 |
| `s1:vtable` | the same slot of the same class's vtable (`tools/s1vtables.py`): tables found by the RTTI's class-name string, aligned by a dynamic programme anchored on exact names (and on a destructor writing its own table), other pairs scored on opcode similarity (>= 0.5, sizes within 2x), each pair forced by a 0.03 margin, every table inheriting the function agreeing | 504 |
| `s1~` | judged and NOT applied: a name landing on several rows (static copies of SDK stubs; identical template instances), a signature that changed, or a name the relocation bug gave | 73 |

2,957 of the map's 13,370 rows are named (141 of the rows are the static initialisers). A weaker tag is upgraded when the exact pass later
confirms the same name.

**The vtables check the weaker passes.** `s1vtables.py` anchors only on exact names and scores
every `align`/`calls`/`gap`/`order`/`ref` name like a placeholder, so the alignment can disagree
with it: it confirms 90 and contradicted three, each then read in the code. `clear__tree<...CSnd*>`
(align) stores CSnd's own vtable and calls `operator delete` -- it is `CSnd::~CSnd`;
`CBatchRelocator::StartBatchReloc` (align) passes a `CTexHandle *` through as the demo's
`AddToBatch` does; `CZPauseTest::~CZPauseTest` (calls) is slot 0 of CUIMapCtrl's table, and
CZPauseTest's own table has its real one. Held out one at a time, the 49 exact names in vtables
are all re-derived (opcode similarity; s1names' register-keeping skeleton gets 40 and leaves 9:
one inlined level more re-allocates every register -- CNode::UpdateBBox scores 0.09 with
registers, 0.60 without). Tables that disagree with each other are withheld (the C2D, CAiState
and CEntity families changed shape and their many identical stubs could slide), and a name two
SOCOM II functions would take -- an inline's out-of-line copies, one per overlay -- is an `s1~`.
The tables themselves are in `data.txt` (`__vt__`/`__RTTI__`, 436 rows).

**The relocation keying bug (fixed 2026-10-24).** `.relmain`'s `r_offset` is already a virtual
address; the first version added `main`'s base to it, so every "exact" comparison masked words
1 MB away from the real relocation fields. Only functions with no relocations, or whose stray
mask happened to cover the differing words, matched: 383 instead of 941. Three names it gave
were unproven or wrong and are back to placeholders with `s1~` notes -- `SetStance`, a
`return f(x);` wrapper that masks like every other, and `DoSetStance`, named through it, which
builds strings with `strlen` where the demo's queries AI states. `test_scdecomp.py` now holds
every `R_MIPS_26` to a j/jal and the wrapper to "not exact".

Against socom-unzipped's independent table, 704 of 712 shared addresses agree (the align
pass alone: 163 of 165, its two others role labels, `cull` and `flush`); the eight
others are its own labels (`netbEx*`, `_StartThread`, a command-string guess) or Ghidra's
syscall-slot names, against SOCOM 1's real symbols -- e.g. both `AddDmacHandler` stubs load
syscall 18, and the call site says which one the code calls.

**A non-exact name carries SOCOM 1's SIGNATURE, which may have changed.** The two projector
builders are SOCOM 1's `zdb::CalcWorldToUV` by behaviour, but SOCOM II passes the matrix
third and two floats where SOCOM 1 passed a `CTexture *` and the matrix fifth -- so their
rows keep placeholders with an `s1~` note. When source for a `gap`/`calls` name compiles to
a different mangled name, rename the row to what the source defines. (An earlier note said
the projectors were absent from the 2002 builds: they were in another unit, `zdb`'s.)

### Global data names (`data.txt`, `tools/s1data.py`)

`data.txt` maps global data the way `symbols.txt` maps functions (same row format, one shared
namespace; the differ resolves a name from either, and a TU-local static the data map names
resolves by address rather than by content). `tools/s1data.py` fills it from the demo: every
function pair s1names made addresses the same globals, so the references are paired -- at the
same instruction for an exact pair; otherwise by how each target is used (the sorted access
kinds of its references), because two compilers order the accesses differently and the demo
reaches small data through `$gp` where SOCOM II never does. A name is written when every
proposal agrees both ways and an exact pair, a uniquely-used target, or two functions support
it: 215 globals (`theGame`'s neighbours, `CStack::m_top`, `exptbl`, `vu0_CodeSegment`, libcdvd's
state), and 25 functions named because SOCOM 1 takes their address at the same place (the
camera's ZAnim command handlers, interrupt handlers). m2c drafts use these names.

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
| the EE-GCC compilers | `toolchains/ee-gcc*/` | EQOA's (`~/claude-eqoa/decomp/toolchains/`, decompme/compilers' `ee-gcc*`): a symlink per directory; only `ee-gcc2.9-991111` and `ee-gcc2.96` are used (README "The GCC front") |
| objdump/readelf | `$PS2DEV/ee/bin/` | ps2dev (default `~/ps2dev/ps2dev`) |

`$SOCOM_ROOT` defaults to this directory's parent — set it when working from a clone
of the published repository rather than inside claude-socom.

## Publishing

`decomp/` is mirrored to a **public** repository, github.com/DabDavis/socom2-decomp
(created 2026-10-24, public since 2026-10-24), as `git subtree split --prefix=decomp` of
claude-socom's history:
the work happens here and `tools/publish.sh` pushes the committed state (`--dry-run`
to look first). It refuses a tracked line that looks private, as claude-eqoa's
`_publish.py` does.

## Working in parallel

Agents (or people) can work at once in git worktrees of claude-socom, each on its own units, with
the integrator merging their branches into master. The tools take turns where they share state:
`tools/try.py` builds in `$SC_TRY_WORK` (one scratch tree each), scdiff names its object after
the source's whole path and its process, `tools/rename.py` changes the map under a lock, and
`./sccheck.py --files REGEX --build-dir DIR` builds and scores only the units named, into a
private directory, its regression check covering only them. Only the integrator runs the whole
check, `--baseline` and `tools/automatch.py` (after a merge: `--stale`, twice if it reports
superseded drafts), and only the integrator publishes. A change under `include/` is proved on
every hand-written unit (`--files '^(?!auto/)'`) before it is committed. A worktree needs
`extracted/` and `decomp/toolchains/` linked in from the main tree.

`--files '^(?!auto/)'` leaves out the drafts, so a hand-written inline copy that only its draft
matches reports as a regression there (0x32C5F0, ~CRdrArchiveList's copy, 0x3340F0, the rdr editor's deque
constructor -- zrdr_edit.cpp's copy is 0x30 against 0x28 since the MSL headers' round 7 -- 0x1EF8C0, st_core.cpp's
~vector<CMovieCue> (0x60 against 0x80: generated early by FreeVector's temporary), 0x510080, OVL2's
__list_imp destructor that seal_aistate.cpp's missing copy now reports "elsewhere", and, since the
online lane's round 10 renamed their rows, the lobby and chat queues' 0x2E0D90, 0x2E0DC0, 0x2F6230
and ~CLobbySaveModule's copy 0x2FD070 (each draft carries an `sc: as=` line) -- 0x29FF60 and
0x2D02B0 match from hand source since the MSL headers' completion): rerun with that draft's file
included before believing one.

## The generated tier: `src/auto/`

`tools/automatch.py` hands every unnamed 16-aligned overlay function up to 0x100 bytes to
m2c, compiles the draft **as it stands** with the settled toolchain, and keeps it only if
it is byte-exact with every relocation resolved -- the same `scdiff.compare` hand-written
source must pass. 2,335 of 7,007 did on the first pass (61 % of those up to 0x40 bytes, 33 %
overall); with the corrections below, 3,068 did by 2026-10-24 (149 from callee prototypes, 37
as C++), and the automatch lane's fixes (below) added 424 in one run over everything up to 0x400
on 2026-10-25 (5,835 tried) -- 3,151 stand now, hand-written source superseding the rest. They
live one per file in `src/auto/<segment>/<VA>.c` (two drafts' prototypes can disagree),
are counted apart by `sccheck.py`, and are **not finished source**: placeholder names,
`void *` and `M2C_FIELD` offsets, C rather than the game's C++, and no TU grouping. They are:

- a regression guard for the toolchain: 3,151 functions across both overlays compile on
  every check, so a wrong flag or build fails loudly (one build and one flag set match all
  of them -- the overlays were built with the toolchain zlib settled, throughout);
- a correct starting body for whoever writes the real TU;
- disposable: hand-written source supersedes them (sccheck skips a generated function the
  hand tier defines, or emits a byte-exact inline copy of, and says so), and
  `automatch.py --emit` deletes superseded files.

Data references in a draft are `data_<VA>` placeholders (BSS included) or `data.txt` names,
so the relocation check verifies them; none reads a literal address.

What the generator corrects in m2c's output before compiling (each measured on the near
misses -- drafts that compile at the right size and miss by an instruction or three):

- **parameter gaps**: m2c names a parameter by its register and leaves out the unused ones,
  so `f(s32 arg0, s32 arg2)` would pass `arg2` in a1; the skipped ones are put back;
- **tail calls**: m2c reads a `jal`'s delay slot but not a `j`'s, losing an MI thunk's
  `this` adjustment or a loaded argument; the exported assembly spells `j f; <delay>` as
  `<delay>; jal f; jr ra`, with self-moves for argument registers passed through untouched;
- **narrow fields**: a byte stored as 0xFF is `li 255` to an unsigned char and `li -1` to a
  signed one; a miss is retried with `s8`/`s16` made unsigned;
- **template names**: a row whose name is no C identifier is drafted under its placeholder
  and renamed in the object (`/* sc: as=OLD NEW */`);
- **callee prototypes** (`tools/sigs.py`): m2c sees a call's arguments only in registers the
  caller writes, so a destructor handing `this` on untouched in a0 and writing a1 = 0 is
  drafted as `f(0)`. A miss is redrafted with its callees declared in an m2c context file:
  the parameters a mangled name lists (MW marks no static member -- `this` is read off the
  code), or else the argument registers live at the callee's entry, by backward dataflow;
- **member and virtual calls as C++** (`tools/cxx.py`): MW evaluates a member call's
  arguments before it loads `this`, where the C call of the mangled name loads `this` first;
  and a virtual call goes through t9, a function pointer -- all C has -- through v0. A
  remaining miss is redrafted as C++: each non-static member function it calls declared in
  a class that mangles back to its name (scopes as namespaces) and called as a member; each
  m2c virtual call (a function pointer two loads from the object) made on a declared class
  with the vtable pointer at that offset (MW puts it after the class's own data) and a slot
  per offset. The draft's functions stay `extern "C"` under the map's names, in `<VA>.cpp`.

The differ also stopped refusing the last function of a unit that an 8-aligned GCC object
follows without MW's 16-byte padding (131 drafts had every byte right).

**Measuring it** (`tools/amsurvey.py`, 2026-10-25): a fixed sample -- every unmatched 16-aligned
overlay candidate up to 0x100 bytes, 3,624 functions -- run through automatch's own `attempt()`,
every draft's verdict kept, the closest miss and its diff saved, the near misses classified by
cause (`near`, `classify`, `show`, `cmp`). The generator as it stood matched **4** of them; 1,032
(28 %) had no draft MW would build, 173 no draft at all. Each correction below was measured
there, one at a time, in this order (`SC_AM_<NAME>=0` switches one off):

- **a field off a `char *` base, uncast** (`SC_MEMBER`; 4 -> **201**): m2c's
  `M2C_FIELD(p, T *, off)` casts its base to `char *` even when `p` already is one (every
  `void *` is, here), and **any cast on the base makes MW keep the field's address in a
  register** -- `addiu v1,a0,0x73C; swc1 $f12,0(v1)` where the original's member access stores
  `swc1 $f12,0x73C(a0)` -- and moves registers besides (probes: an int base, an `int *` base, a
  redundant `(char *)` and a cast to a struct pointer all do it; only uncast `char *`
  arithmetic and a real struct pointer do not). A field whose base the draft's declarations
  make a `char *` (a declared one, a `char **` field, a `char *` plus an int, a call returning
  one) is spelled `SC_MEMBER`, which adds the offset as it is; m2c's spelling stays the
  fallback form, and the bool and C++ stages run on each form;
- **repairing what MW refuses** (201 -> **273**): m2c writes GNU C, where an int passed or
  assigned as a pointer, a pointer of another type, a call disagreeing with the prototype m2c
  took from another call, or a load through an int are warnings -- errors to MW. A draft that
  will not build is repaired from MW's own diagnostics, a round at a time, each repair a
  conversion the code makes anyway: a cast to the type the message names, an extern or
  function pointer declared without parameter types when every parameter is an int or a
  pointer (probed: the call compiles the same), `#pragma mpwc_relax`, a word load through an
  int made through an `int *`, the 128-bit typedefs (build failures 1,032 -> 445);
- **three more spellings of a virtual call** (273 -> **329**): the vtable pointer at 0
  (`*obj`); a member object's virtual, called on `obj + K` with the vtable read from obj at
  VPTR (its vtable pointer VPTR - K into the member); and a call m2c gives no arguments, which
  passes the object on in a0 untouched -- m2c cannot see it (a destructor's `Release()`);
- **arguments five to eight** (`tools/m2c.py`; 329 -> **395**, m2c refusals 173 -> 65, none lost): objdump spells registers 8-11 `t0`-`t3`,
  O32's names; the EE passes arguments five to eight in them, and m2c's EE ABI calls them
  `a4`-`a7` (its `t0`-`t3` are registers 12-15, not even temporaries). Spelled `t0`, m2c read a
  fifth parameter as an unset register and refused the function, or dropped a fifth argument;
- **tail calls through a register**, **bool** and three variants (`tools/m2c.py`, automatch;
  measured together: 395 -> **456**): m2c takes any `jr` but ra's for a jump table, so a virtual tail call (`lw t9,8(t9);
  jr t9`) is exported as `jalr; jr ra` like the `j` tail call; a bool flag is tested as itself
  (`flag != 0` narrows it again with `andi 0xff`), a function returning only 0 and 1 is drafted
  as bool, and one whose `true` the target materialises with `daddiu` returns through an
  assigned bool (`return true` is `li`, an assigned true `daddiu`); `x == 0` as `!x` (`sltu,
  xori` against `xor, sltiu`), an int plus an offset past 16 bits as a pointer sum (MW builds
  the offset in `$at` for a pointer, v0 for an int), and m2c's byte offsets from an `M2C_UNK`'s
  address summed as bytes (C scales them by 4: `sll 4` for the target's `sll 2`).

Remaining near misses (of the 309 within four words before the virtual-call and later fixes: 145 register-only, 40 of them virtual calls since fixed; 31 operand order; 20 stack slots; 18 `!x`; 13 `M2C_UNK` sums; 12 `$at`; 10 FPR registers; 8 movn/movz; 8 bool), ranked: register allocation with the same instructions
(argument registers, v0/v1 and mixed), operand order and scheduling (a struct copy m2c writes
field by field, `c + a*b`), a local's stack slot (m2c declares a 4-byte `M2C_UNK` for a
12-16-byte object, which MW then places at the top of the frame), `sltiu $at` range checks (a
`switch` m2c wrote as ifs), movn/movz. 12 % of the sample saves its s-registers with `sd` and
never `sq` -- GCC's prologue, not MW's: none of the 3,641 byte-exact functions does, and none of
those 428 has ever matched (a filter the generator could apply).

## The GCC front: the prebuilt libraries

About 868 KB of the image -- 3,672 rows, 22.1 % of the map's bytes -- is not Metrowerks code: libraries linked in
prebuilt, built by their vendors with Sony's EE-GCC. The code says which: GCC aligns a function
to 8 (MW to 16 -- the map's `unaligned` notes), saves s-registers with `sd` (MW with `sq`), uses
branch-likely and, from 2.96, sibling calls. A unit names its toolchain in its directive (its
first three lines are read; the first to give a key wins):

    /* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */

and scdiff builds it with that toolchain's own driver and GNU assembler -- EQOA's ee-gcc builds,
linked into `toolchains/` ("Inputs"): natively for the Linux i386 ones, through wibo for the
PE32 ones (byte-identical to wine's objects, 0.02 s a compile against 4.6 s; only
`ee-gcc3.2-030210-beta2` will not run there, importing `cygwin_internal`). MW stays the default
and no MW unit changed: `--files '^(?!auto/|lib/)'` gives the same rows, line for line, with the
tools before and after (4,068 byte-exact on 2026-10-25, 4,249 on 2026-11-02 after master's merge).

**The compilers, library by library** (`tools/ccsweep.py`: one source over every toolchain and
flag set, every function it defines scored; the probes are `build/scratch/gcc/p*.c`):

| range | library | toolchain, flags | evidence |
|---|---|---|---|
| MAIN 0x18DB90-0x191410, 0x1A13D0-0x1AD088, 0x1B3970-0x1C4CC0, 0x1CCD78-0x1CE2A4 | Sony's SDK 2.7: libcdvd, libdma, libgraph, libipu, libkernl, libmc, libmpeg, libmrpc, libpad2, libscf, libvu0 ... | `ee-gcc2.9-991111-01-dtls13010` `-O2 -G0` (991111 compiles their C the same) | libvu0's inline asm: 32 of 32 only under the dtls13010 build, whose GNU as fills the delay slots 991111's leaves empty and encodes vsqrt as 2.96's does not (below); `memclr` is byte-exact only under 2.9-ee-991111 and its -01, a and dtls rebuilds: 990721 lacks the short loop's nops (8s), every 2.95.x fills the delay slot (2), 2.96 misses by 4, 3.2 by 7; `sceDmaGetChan` is 12s under 2.96, `sceCdSearchFile` 6s |
| MAIN 0x191410-0x1A13D0, 0x1AD088-0x1B3970 | newlib's libc, libgcc's fp-bit, libm: the toolchain's own runtime | `ee-gcc2.96` `-O2 -G0` | `atof`, `atol`: a sibling call under a frame it no longer needs (`sd ra; ...; ld ra; j strtol`) -- 2.96's; no 2.9x or 2.95.x makes a sibling call at all (6s, 5s); libgcc2's `__muldi3` 2.96 only (2.95.3 19 words, 2.9 29). Not quite this 2.96: the runtime's compiler never fills a delay slot with a double's register move (round 4a) |
| OVL1 0x1E7F58-0x1E8FD4 | the microphone input (`openmic`, `getsamples`, `Sid*`, over lgAud) and Sony's libezcnf/libezctl (1.0 beta) | `ee-gcc2.96` `-O2 -G0 -fno-strict-aliasing` | found 2026-11-02, the only run outside the first ranges that saves s-registers with `sd` and never `sq`; `closemic`, `SidClose` byte-exact under 2.96 only (every other toolchain 9-13 words off); `SidGetSamples` reloads its read position after storing through `out` -- byte-exact only without strict aliasing |
| OVL1 0x2430F0-0x243840 | the recogniser's heap (`heap_*`, ScanSoft's) | `ee-gcc2.96` `-O2 -G0` **`-funroll-loops`** (the unit carries the recogniser's `-fno-strict-aliasing` too, untold) | SearchFor's loop unrolled by four: byte-exact only with the flag (without: 50s) -- round 3a |
| OVL1 0x243840-0x248350 | lgAud (the USB headset, liblgaud 1.08.002), SCEA's libnetb/inet | `ee-gcc2.96` `-O2 -G0` | `lgAudASync`, `sceBInetControl`, func_00246CE0: 2.96 only (2.9-991111 15s/10s/6s, 2.95.3-136 15s/6/6s, 3.2 11s/9s/=) |
| OVL1 0x2484A0-0x253510, 0x256F48-0x256FD0 | Nellymoser's speech codec: the encoder (SaseEncVad) and the host-callback setters | `ee-gcc2.96` **`-O2 -G8`** | five functions byte-exact only so: their scalars and float literals are `$gp`-relative (small data in MAIN's 0x1D5300-0x1D55C0, `.lit4`); at -G0 every one misses |
| OVL1 0x253510-0x256F48 | Nellymoser's decoder (SaseDec) | `ee-gcc2.96` `-O2 -G0` | round 12a: the decoder reaches the same host callbacks and instance count by lui and an offset, never $gp (sase_decapi.c 5 of 5; two drafts matched at -G0 at once) |
| OVL2 0x4C5400-0x50DC50 | ScanSoft's speech recogniser (asrspi\*, casr\*, mtx\*) | `ee-gcc2.96` `-O2 -G0 -fno-strict-aliasing -funroll-loops` | the loop unrolling: ContextActivateParams (below, round 2b); `casrStoreSegmentation` keeps its `short` store above the load of the next node: byte-exact only without strict aliasing, and none of the library's drafts is lost by the flag; the sibling calls -- func_004CCC50, ConstructMel, func_004F7038 -- 2.96 only (every 2.9x and 2.95.x 5s-15s); its leaf functions do not tell 2.9x, 2.95.x and 2.96 apart. `mtxConstruct` makes no sibling call because 2.96 declines one where the return types differ |
| OVL2 0x60FB50-0x64F720 | Medius, DME and the rt_\* libraries (SCEA) | `ee-gcc2.96` `-O2 -G0` | rt_comm_get_channel, rt_memory_register_callbacks, rt_msg_client_change_state, rt_msg_set_byte: all four only under 2.96 -O2 (2.9x 5/=/9s/6, 2.95.x 4/=/10s/5, 3.2 =/5/3/=); strict aliasing stays on -- two sibling drafts of one shape (0x61E7F0, 0x61E858) match only with it: thin, one shape |

-O1 misses wherever it was tried and -O3 compiles these as -O2 does; everything is C (no mangled
name in any range). Sony's SDK libraries were built with 2.9-ee-991111 (November 1999), the
rest -- and the runtime the game linked -- with 2.96. **Strict aliasing is per vendor**: this
2.96 enables it at -O2, ScanSoft's and the mic library's builds turned it off, SCEA's Medius kept
it, and so did lgAud (25 of its functions lose without it, round 5a); the other libraries show no
function either way (`tools/ccflags.py LO HI
FLAGS` recompiles a range's drafts with a flag added: a flag the vendor used loses none).
Outside these ranges no row saves its s-registers with `sd` and never `sq`: amsurvey's 428 such
functions (above) were theirs.

**What scdiff learned about GNU objects** (gated on `.reginfo`, which MW never writes):
TU-local references go against SECTION symbols (`.rodata`+N for each string), so a `%lo` is
paired with its `%hi` by the relocation table's order -- GNU as writes each `%hi` just before
its first `%lo` -- not by name (two pairs of one section interleave under one name); a static
callee is `R_MIPS_26` against `.text`+N, resolved by the function at that offset (through the
map, else by the same distance as in the target); a local symbol's GPREL16 and LITERAL fields
are less the object's gp0 (`.reginfo`'s, 0x4000 from ee-gcc), which the linker adds back, and
`R_MIPS_LITERAL` (the -G8 literal pool) is checked as GPREL16 is. sccheck counts GCC's weak
`.gnu.linkonce` copies as inline copies (binding 13 is MW's alone).

**The generated tier** (`tools/automatch.py`'s `GCC_RANGES`): a row in a GCC range -- any
alignment, MAIN included -- gets m2c's GNU C draft (`mipsee-gcc-c`) compiled with its range's
toolchains (MAIN's interleaved SDK and newlib: both, the range's own first), as it stands and in
GCC's own variants -- m2c's single exit as the vendors' guard clauses (2.96 returns straight into
v0: a third of the Medius near misses), a void draft ending in a call as returning its result,
byte steps on `M2C_UNK` and typed pointers, a frame buffer m2c left undeclared -- then redrafted
told its callees' and its own parameters by dataflow (`sigs.prototype`). MW's own corrections
(the member spelling, the repair from MW's diagnostics, bool, C++) answer MW's quirks and are not
tried. `tools/m2c.py` spells `$gp`-relative accesses `%gp_rel(sym)`. A draft carries its
toolchain's directive. A draft GCC refuses is repaired as MW's are, from GCC's errors (never its
warnings): a load through an int (`*(i * 4 + M2C_FIELD(p, s32 *, 4))`) becomes an `s32` load, a
call disagreeing with the prototype m2c took from another call unprototypes the callee -- the
two were ~420 of the ~520 GCC drafts that would not build. The runs (`--max-size 0x400 --emit
--no-prune`, at most 2 jobs; the repair's column is a second run over the first's misses):

| library | rows tried | byte-exact, first run | build failures | m2c refuses | + after the GCC repair | drafts kept |
|---|---|---|---|---|---|---|
| MAIN: Sony's SDK, newlib | 1,062 | 199 (18.7 %) | 108 | 282 | +1 (build failures 84) | 194 |
| OVL1: heap, lgAud, libnetb, the codec | 346 | 41 (11.8 %) | 58 | 14 | +3 (28) | 38 |
| OVL1: the microphone input, libezcnf/libezctl | 30 | 8 (26.7 %) | 8 | 0 | 0 | 5 |
| OVL2: ScanSoft's recogniser | 581 | 159 (27.4 %), +4 under `-fno-strict-aliasing` | 129 | 52 | +6 (57) | 165 |
| OVL2: Medius, DME, rt_* | 1,386 | 447 (32.3 %) | 90 | 102 | +4 (re-run stopped at 0x62AF40) | 447 |

849 GCC drafts, 39,284 bytes, stand.

**Hand-written** (`src/lib/<group>/`, C as the vendors wrote it, SOCOM 1's names): 55 functions
byte-exact of 57 -- `sdk/libdma.c` (5), `libc/stdlib.c` (abs, atof, atoi, atol), `ovl1/sid.c`
(the microphone, 7), `ovl1/ezcnf.c` (ezNetCnfGetCombination, ezNetCnfGetDefault, named by their
own messages; libezcnf is 2.96 too: 991111 30s/24s, 2.95.3-136 30s/22s, 3.2 26s/29s),
`ovl1/lgaud.c` (3), `ovl1/libnetb.c` (2), `ovl1/nelly.c` (the codec at -G8, 5), `asr/mtx.c` (5),
`asr/cb.c` (9 of 11: casrDoSecondPass and cbGenderDisable are OPEN, diagnosed in place),
`asr/extaudio.c` (2), `asr/ctxutil.c` (3), `medius/rt_msg.c` (all 8 byte accessors). Each unit's
directive carries its library's toolchain and flags; a draft it supersedes is deleted, as for MW.

**Which library objects were assembly -- the demo says.** The demo's symbol table keeps every
input object's `.text` SECTION symbol, and GCC marks each object it compiled with a local
`gcc2_compiled.` (and `__gnu_compiled_c`) at that same address: an SDK or newlib object without
the marker was assembled (`./scd.py s1 --units LO HI` lists them, statics marked; `--callers NAME` finds a demo function's callers; `tools/ccsweep.py UNIT --unit` scores a unit under its own directive, one function a line -- the three the 2b lane asked for). In MAIN's libraries
there are exactly three kinds: libkernl's syscall-stub object (138 stubs), libkernl's TLB/debug
exception handlers (one object, three functions), and newlib's 13 EE string routines (memchr,
memcmp, memcpy, memmove, memset, strcat, strchr, strcmp, strcpy, strlen, strncat, strncmp,
strncpy, one object each). **Everything else is C** -- libvu0 too: its object carries the marker,
so its functions are inline `asm` in C, as Sony shipped libvu0.c, not assembly. Of MAIN's 288 open
rows holding a syscall/sync/COP0/COP2/MMI/lq/sq instruction, 154 are those assembler objects; the
rest are C with inline asm or C whose 128-bit moves GCC made itself (qsort, __kernel_rem_pio2).
The same boundaries give each C unit's grouping: a `src/lib` unit is one demo object, its
functions in the demo's order.

**The assembler path** (2026-11-02): `sccheck` and `scdiff` take `src/**/*.s`. An assembler unit
names its toolchain on one of its first three lines (`# sc: compiler=ee-gcc2.9-991111`), and is
built by that toolchain's driver, which hands a `.s` straight to its own GNU as (`as -G0 -EL -O2
-mabi=eabi`: 2.9-ee-991111b under 991111, 2.10-ee-001003-1 under 2.96, 2.9-ee-991111 under the
dtls13010 build); its object (`<name>.s.o`) is scored like any unit's, each `.type x,@function`
symbol against its row, every relocation resolved. Metrowerks has no such path (a `.s` without an
ee-gcc toolchain is a build failure). `tools/asmunit.py` writes one from the image: canonical
instructions (`objdump -M no-aliases`, numeric registers) under `noreorder`/`noat`/`nomacro`,
branches to labels, j/jal to map names, each lui and its %lo uses as %hi/%lo of the symbol there,
the object's own zeroed data as local `.bss` labels; then it assembles the unit and scores it
until every function matches (an instruction the assembler spells differently becomes a `.word`
-- none has needed to). scdiff resolves a GNU object's `.bss+N` reference by the local label at N
when the data map names it (zeroed data has no content to compare). Proved on 2026-11-02: every MW
hand-written unit gives the same 6,335 rows with master's (97eb839) tools and these, every GCC unit (src/lib
and the GCC drafts: 1,031 rows) the same rows but one, which the `.bss` resolution now checks (rt_time_get_time_string_ptr's `szTime.0`: unchecked before, ok now). The assembler objects: `sdk/syscall.s` (138 of 138,
byte-identical to the demo's slot for slot -- eight slots named from it, `s1:slot`),
`sdk/tlbhandler.s` (_kTLBException, _kExitTLBHandler, _kDebugException: the map's one 0x360 row
split at the demo's offsets; its save area and saved hi/lo/sa/epc words named from the demo's
locals, all 27 relocations ok), `libc/*.s` (the 13 string routines, under 2.96's as; five differ
from the demo's own copies only in one short loop's tail -- `sb; addiu; bne; nop` here, `sb; nop;
bne; addiu` there: one source, two assemblies).

**Sony's SDK: ee-gcc 2.9-ee-991111-01, the dtls13010 build.** libvu0 tells the 991111 builds
apart, which no C probe did: inline asm leaves delay-slot filling to GNU as (reorder mode), and
991111's as (2.9-ee-991111b) leaves the return's slot empty after an `sq` (sceVu0CopyVector) and a
loop branch's after an `addi` (sceVu0MulMatrix) -- 9 of 32 functions 3s-5s -- where the target has
them filled; 2.96's as (2.10) fills them but encodes `vsqrt Q,vf5x` with its fsf field set
(0x4A2503BD, the target 0x4A0503BD: 2 functions 1 word); 3.2 misses the three Clip functions;
`ee-gcc2.9-991111-01-dtls13010` (its as reports 2.9-ee-991111) gives all 32. libdma, libkernl's
deci2/intr/cache objects compile the same under 991111 and dtls13010, so the new SDK units name
dtls13010. And 2.9-991111 does make sibling calls: libkernl's iSyncDCache (`j _sceSDC`, the mask
in the slot) and SyncDCache (`j EIntr` under a torn-down frame) compile so under both builds (2.96
misses); what is 2.96's alone is atof's kind, the sibling call under a frame it keeps.

**Round 2a** (2026-11-02, by hand, under the toolchains above): libvu0's 32 leaf functions
(`sdk/libvu0.c`; RotMatrix, CopyVectorXYZ, LightColorMatrix stand as drafts, Camera/
NormalLight/ViewScreen/DropShadow, C around calls, open); libkernl's `deci2.c` (10), `intr.c`
(9), `cache.c` (6), `dintr.c` (EIntr; DIntr OPEN: the target reloads the EIE mask inside its
loop); `sdk/libscf.c` (5 of 10, the T10K/OSD getters; five OPEN on a v0/v1 choice and a movz);
newlib's libm objects `libm/s_copysign.c`, `s_isinf.c`, `s_isnan.c`, `sf_copysign.c`,
`sf_fabs.c` (2.96 only; every 2.9x and 2.95.x 10-15 words off; `s_fabs.c` OPEN: the target leaves
its return's slot empty after the copy into v0). With the assembler units: 222 byte-exact of the 229
functions written (154 of them assembler), four superseding drafts that matched (GetRomName, IsT10K, iSyncDCache, iInvalidDCache: `automatch.py --emit` drops them).

**Round 3a** (2026-11-02, by hand): libkernl's last five C objects -- `sdk/tlbfunc.c` (18 of 19),
`initsys.c` (9), `libosd.c` (6, the ExecPS2 patch), `exit.c` (9), `alarm.c` (5); OVL1's
`ovl1/lgaud.c` (16: the RPC wrappers, PrepareForReboot, the async end callbacks), `ovl1/heap.c`
(heap_GetVersion, SearchFor); newlib from newlib's own text -- `libc/strrchr.c`, `strstr.c`,
`strcasecmp.c`, `strncasecmp.c`, `strtok_r.c`, `rand.c` (srand, rand), `abort.c`, `assert.c`, the
eight reentrant syscall wrappers (`closer.c` ... `writer.c`) and `strtod.c` (strtod; strtof OPEN):
86 functions byte-exact of 88 written. What they showed:

- **A static several objects define is `NAME@UNIT` in the map** (`Copy@tlbfunc`, `kCopy@exit`):
  libkernl links five objects with static `Copy`/`kCopy`/`setup`/`GetEntryAddress` syscall
  wrappers. A GNU object's local function `NAME` scores as the row `NAME@UNIT` (UNIT the source's
  stem) when the map has one (`scdiff.qualify_statics`; sccheck, ccsweep and scdiff apply it); a
  static the map names once stays bare. Drafts spell such a row `func_<VA>` (`as=` renames them).
  Proved: master's tree scores the same rows with master's tools and these, 5,108 hand-written and
  843 GCC-draft rows.
- **991111's sibling call is the void-context call's only**: `InitTLB() { if (...) InitTLB32MB();
  else _InitTLB(); }` compiles to two `j`s, the target's `return InitTLB32MB(); return _InitTLB();`
  to jals under the frame -- the opposite of 2.96, which tail-calls a returned call.
- **A register variable lives across calls unsaved**: InitTLB32MB keeps its index in t9 across
  every `_SetTLBEntry` (`register int i asm ("$25")`; GCC gives a pseudo a saved register), and
  returns it shifted. The syscall wrappers are C: `asm volatile ("li $3, N\n\tsyscall")` in a body
  with no return statement gives the stub's four words.
- **An inline-asm function can end without its own delay slot**: libsif's sceSifWriteBackDCache
  ends in its asm's `jr ra; nop` and then GCC's `jr ra`, whose slot is the next object's first word;
  the seed had cut both words into 4-byte rows (merged back: the function is 0xB0, sceSifInitRpc
  starts at 0x1A6368, both as the demo's).
- **OVL1's heap is ScanSoft's** (the demo links it inside the recogniser): SearchFor's search is
  unrolled by four, byte-exact only with `-funroll-loops` (-O2 -G0: 50s). automatch's GCC_RANGES give
  it the recogniser's flags, and the recogniser's own range gains `-funroll-loops`.
- **newlib compiles as written**: every newlib function written from newlib's source matched as it
  stands, its R5900 short-loop nops GNU as's; `_read_r`'s `(_ssize_t)read(...)` cast (a long from an
  int) is newlib's own and the target's extra move. The reent wrappers use the system layer's global
  `errno` (0x1E6640), newlib undefining the macro there. `abort` ends at a noreturn `_exit` (crt0's,
  `Exit(0)`), where the demo's older newlib loops back.
- **Statement shapes the vendors used**: lgAud folds the reply's device bits in two statements
  (`devicechanged &= ~2; devicechanged |= ...`; one expression swaps the registers) and assigns
  through a test (`if ((*hint = devicechanged) == 1)`); libkernl's kFindAddress walks its parameter
  itself (a local copy moves the early exit), supplement_crt0 sets its semaphores in field order.
- **Open, diagnosed in place**: InitTLB32MB (5 words: the last loop's first EntryHi is computed after
  the exit test in the target -- a giv's preheader -- and before it here), strtof (its delay slots are
  empty in the target; -fno-delayed-branch, -O1 and -fno-schedule-insns2 do not reproduce it).

**Round 4a** (2026-11-03, by hand): 53 rows newly byte-exact (14,512 bytes), 65 functions written
byte-exact of 72 (twelve supersede matched drafts) -- libdma whole (`sdk/libdma.c`, 19 of 19: 11 new, three drafts superseded);
libgcc, a new group (`src/lib/libgcc/`: libgcc2.c's DImode objects, `libgcc2.h` and a unit per
`L_` object, 5 of 8; fp-bit, `fp-bit.h` and a unit per object, 18 of 18); newlib's strtol.c and
strtoul.c, the printf family (printf, fprintf, fiprintf, sprintf, vsprintf, sscanf with its static
eofread), stdio.c's four cookie functions, vfprintf.c's and vfiprintf.c's statics and wrappers
(their `_r` bodies not written); libscf's time helpers (7 of 11). 18 rows named, each by body, call
or the demo's position (vfprintf, _vfprintf_r, vfiprintf, _vfiprintf_r, __svfscanf, __sinit,
__sfvwrite, fflush, __sread/__swrite/__sseek/__sclose, strtoul, `__sprint@`/`__sbprintf@` for both
printf objects, sceScfGetGMTfromRTC). No tool changed. What they showed:

- **The runtime's compiler never puts a double's register move in a delay slot.** libgcc's
  __fixdfdi, __fixunsdfdi and __floatdidf have every instruction right but the call slots whose only
  candidate is a DFmode GPR move (`move s0,a0; jal dpcmp; nop`), while an int move fills the same kind
  of slot (__floatdidf's second litodp). The compiler that built the runtime gave a soft-float DFmode
  move a length of two instructions (mips.md's movdf_internal2), which reorg never puts in a slot;
  toolchains/ee-gcc2.96 gives it one. No flag changes it (-msingle-float, -mhard-float, -mfp32, -mips3,
  -mlong64, -mabi=o64; -fno-delayed-branch hands the slots to GNU as, which fills them too). It is
  strtof's and fabs's open cause as well (rounds 2a, 3a): three objects, one diagnosis. And
  __fixunsdfdi converts its unsigned DImode by the older expand_float (__floatdidf, then +2^64 when
  negative; this 2.96 halves first): the runtime's GCC is a 2.96 whose integer code is the
  toolchains' exactly (every DImode division and multiply byte-exact) but not this build.
- **libgcc2.c compiles as GCC shipped it**, with longlong.h's MIPS `umul_ppmm` (multu; mflo/mfhi)
  and its C `udiv_qrnnd`/`count_leading_zeros` -- so `UDIV_NEEDS_NORMALIZATION` and a static
  `__clz_tab` in every division object (the demo's symbols keep four LOCAL copies) -- and the
  USItype `__ll_B` (`1L` makes the remainder's shift 64-bit: 309 words). __muldi3 is 2.96's only
  (2.95.3-136 19 words, every 2.9 29).
- **fp-bit is not the stock text**: no rounding (the guard bits are shifted off), denormals flushed
  both ways, no NaN or infinity in single precision (the EE's FPU has neither: only a fraction
  overflowing the all-ones exponent saturates), a double's all-ones exponent unpacked with
  normal_exp EXPBIAS and an infinity's fraction IMPLICIT_1; the multiply is the older text, its
  redundant `& 0xffffffff` kept. Written from the binary in fp-bit's shape, all 18 byte-exact.
- **Sony's CHCR is set field by field**: libdma's transfers assign tD_CHCR's bitfields (MOD, DIR,
  STR) -- the compiler merges them into one load and store with signed masks (`li -13`) -- where an
  explicit `& ~0x100` on a u_int is a zero-extended constant (`lui; ori 0xfeff`); a u_int compared
  with 0xffffffff is `lui; ori` too, an int with -1 `li`. A volatile store never fills a call's delay
  slot (sceDmaReset's D_STAT acknowledge does, so its second store is through a plain pointer), and
  sceDmaReset's six stores schedule as the target's in one order of 720 (`tools/ccperm.py`).
- **dtls13010 tells itself apart by a broken sibling call**: libcdvd's cdvd_exit and libscf's addhour
  and sceScfGetLocalTimefromRTC end `jal f; nop; jr ra; nop` after the epilogue (a call that returns to
  itself, harmless only because nothing reaches it); 991111, its -01 and a rebuilds and 990721 miss
  cdvd_exit by 36-37 words (the wrong size), dtls13010 by 5. libscf's adddate and subdate, though,
  are byte-exact as written under 2.9-990721 and every 2.95.x and not under dtls13010 (the target
  reloads t->year after the day/month stores), while those miss GetRomName -- one of the two sources
  is not yet the vendor's.
- **newlib's stdio compiles as newlib wrote it**, its FILE on the stack for the string forms (`_flags
  __SWR|__SSTR`, `_w`/`_bf._size` INT_MAX), CHECK_INIT before every `_r` tail call; the repo's MW
  `include/stdarg.h` gives GCC's EABI `va_list` too (a pointer into the GPR save area).
- **Open, diagnosed in place**: __fixdfdi, __fixunsdfdi, __floatdidf (above); libscf's frombcd (the
  target's two-operand `mult; mflo` against the R5900's three-operand form), adddate, subdate
  (above), AdjustTime (branch order and a `j` sibling call where the target falls through and ends
  in the jal form).

**Round 5a** (2026-11-03, by hand, OVL1's libraries first): 74 functions written byte-exact of 79 --
57 rows newly byte-exact, 17 matched drafts superseded -- and 37 rows named. `ovl1/lgaud.c` (27 of 28:
Enumerate, Open, Read, Write, WriteVag, GetMixer, SetMixer, ARead, AWrite, AWriteVag, AEnumHint new),
`ovl1/heap.c` (9 of 9: Realloc, Alloc, Calloc, Free new; GetMemInfo, Open, Close over their drafts),
`ovl1/sid.c` (the microphone whole, 14 of 14: openmic, getsamples, SidInit, SidAdvanceBuffer new),
`ovl1/libnet.c` (new: Sony's sample inet client over libmrpc, OVL1 0x245AD8-0x247050, 33 of 33, 24
new), `ovl1/libnetb.c` (10 of 11: sceBInetCreate, Close, IP2Address, libnetb_trans_data,
libnetb_alloc_bufs new), `ovl1/libnetb_udp.c` (new: libnetb's UDP ring, the demo's object 0x424DF8,
2 of 5), `ovl1/nelly.c` (8 of 8, 3 new), and in MAIN `sdk/inet_ntop.c` (new: BIND's inet_ntop.c in the
network library, inet_ntoa/inet_ntop/inet_ntop4/inet_ntop6, 4 of 4 as BIND wrote them -- under the
SDK's 2.9 builds only, 2.96 17-196 words off). Named: lgAud 7 (their own messages), the heap 5 (the
C library call each wraps), SidIsRecording, libnet 17 (`s1:order`: the demo's libnet object 0x4223A0
holds them in the same order, and the RPC numbers agree), libnetb 3, BIND's four (inet_ntoa by its own message '[inet_ntoa error]', the rest by body). What they showed:

- **lgAud keeps strict aliasing** (`-fno-strict-aliasing` loses 25 of its matched functions), and so
  does libnet: SCEA's and Sony's 2.96 libraries are built at plain `-O2 -G0`.
- **The R5900's short loops are GNU as's**: lgAudInit's halts are `for (;;);` and its bind delay
  `while (i--);` -- as pads each to the target's five nops (2.96's as, 2.10), so the nops are no
  source's; and its delay count is reset before the bind call (it lives across the call, in s0).
- **SCEA checks each argument with its own `return 2`** (libnetb_trans_data, the UDP ring): one
  hoisted `li v0,2` and every test branching to the epilogue; a combined `a || b || c` splits the
  returns. Sony's libnet combines its two size checks (`size < 0 || room < size`) where SCEA would not.
- **A value computed and never used can be the source**: libnet's get-string helper computes its
  siblings' `room` and never uses it -- dead, but it gave the target its registers (an unused local
  in the other place did too, which was the hint).
- **2.96 inlines an `__inline__` helper into a caller, the result through its own register**:
  heap_Realloc's null-block path is heap_Alloc's body (`move v1,s1 ... move v0,v1`); heap_Alloc itself
  is written out (through the helper: 12 words).
- **A struct parameter whose address is taken is copied where the source copies it**: sceBInetClose
  takes `&n` for rt_memory_free; the target stores the local copy in the RPC call's delay slot, which
  only `LIBNETB *n; ... n = s;` right before the call gives.
- **Statement orders, measured** (`tools/ccperm.py`): lgAud's request stores (handle, size, channel),
  heap_Free's counters, SidInit's three fields, the codec allocator's, sceBInetIP2Address's bytes.
- **Open, diagnosed in place**: lgAudInit (32 words, all registers: &cdUA and dealloc take each
  other's s2/s3), sceBInetGetBuildTimestamp (the frame: six saved registers in the target),
  libnetb_udp_init_buffer, libnetb_udp_create_space, libnetb_get_valid_data_header_ptr.

**The tools, round 5a** (the 3b lane does not change them): `tools/amsurvey.py` drives automatch's GCC
attempts too (its wrapper refused the range's toolchain argument and every GCC row crashed): over
OVL1's 247 open library rows it found 49 right-size near misses, 23 within five words, most of them
written by hand above; `tools/automatch.py` retries a GCC draft with every callee whose value it
discards declared void (m2c's M2C_UNK callee sets v0, which keeps the result register out of the
allocation after the call -- the draft's `lui v1` where the target's is `lui v0`): over MAIN's 345 open
library rows up to 0x100, 0 -> 4 byte-exact (__sinit, sceSifExitRpc, _sceFsWaitS, _sceMpegFlush), none
lost; `tools/ccperm.py` writes its variants file per process (two lanes at once) and shows the nearest
orders when none matches. automatch over MAIN's library ranges (`--range 18DB90 1C4CC0` and `1CCD78 1CE2A4`,
`--max-size 0x400 --emit --no-prune`, 2 jobs): 896 tried, 44 hits -- 40 of them rows hand-written
source already matches (automatch's `hand_defined()` reads `build/obj`, which a lane's private
`--build-dir` checks never fill: they were deleted) and the 4 the void-callee variant gained, kept
as drafts (m2c refused 283 of the 896, 83 drafts would not build). No tool that scores changed: scdiff.py and sccheck.py are
master's, so every MW and GCC unit scores as before, row for row (`./sccheck.py --files
'^(?!auto/)'`: scdiff.py, sccheck.py, scdecomp.py and `include/` are byte-identical to dc0c477's, so no
unit can score differently; every `src/lib` unit: 820 byte-exact of 862, each miss OPEN in place).

**Round 6a** (2026-11-03, MAIN first): 124 functions byte-exact of 132 written -- 102 rows newly
byte-exact (19,776 bytes), 22 matched drafts superseded -- and 71 rows named (69 `s1:order`: the
demo's newlib objects sit in the same alphabetical link order, each checked by its calls; 2 by body).
`sdk/inet_bsd.c` (33 of 33) and `sdk/inet_addr.c` (5 of 8): the socket library at 0x1C0948; newlib
(`libc/`, 71 of 74): findfp.c, exit, fclose, fflush, fgets, fwalk, fwrite, refill.c, rget, wbuf,
wsetup, fread, fseek, ftell, makebuf, ungetc, flags, fopen, puts, the `_r` forms of printf/sprintf/
vsprintf, mprec.c (14 of 17), signal.c, `_getpid_r`, getenv_r, envlock, locale, mbtowc_r, strcspn,
qsort, fvwrite, bsearch, bcopy, `_calloc_r` and `setjmp.s` (an assembler unit: newlib's
machine/mips/setjmp.S, sq of every saved GPR); libkernl's `sdk/iopheap.c` (10 of 11) and
`sdk/loadfile.c` (5 of 6 written). data.txt gains `__mprec_tens`/`bigtens`/`tinytens`, `p_environ`
and `__ps2_klibinfo__`. What they showed:

- **The socket layer over libinet was built at -O0** with the SDK's own toolchain: 33 BSD-name
  wrappers (socket, bind, connect, the receive calls with a 600000 receive timeout first ...) with
  -O0's frame pointer and spilled arguments, byte-exact only under the dtls13010 build (991111, -01,
  a, 990721 12 words off, 2.95.x 14, 2.96 11, 3.2 11) -- its GNU as fills the calls' delay slots. A
  struct in_addr passed or returned by value goes through a local copy of the library's own address
  type (-O0 keeps the store and load). The address routines are FreeBSD's inet_aton (strtoul per
  part) and 4.4BSD's class functions at -O2 under dtls13010, **with 32-bit types**: this compiler's
  `long` is 64 bits and the BSD text's `u_long` would be doublewords. Its isxdigit tests an
  uninitialised variable (the target hoists `(u_char)(t5 - '0') < 10` of a register nothing sets).
- **newlib compiles as written, and is newlib 1.9's text** -- each difference from the text first
  tried was read off the target and is a later 1.9 line, not a compiler effect: lflush's
  parenthesised test, `CHECK_INIT(fp)` in __swsetup, `mode[1] &&` before the '+' in __sflags,
  ftell's and fseek's flushes, getenv_r's static `p_environ` (NULL returned without unlocking),
  `_sprintf_r`/`_vsprintf_r` calling vfprintf. The whole `struct _reent` is now in `libc/newlib.h`
  (`_atexit` 0x148, `_sig_func` 0x1D4, `__sglue` 0x1D8, `__sf[3]` 0x1E4), with struct stat (st_mode
  at 4). mprec's `__ULong` is a word (Balloc's size is 20 + 4 * 2^k); `_ulp`, `_ratio`, `_d2b` are
  the runtime compiler's DFmode delay slots again (round 4a).
- **The SDK's statics have the demo's names**: its symbol table keeps every object's LOCAL labels
  (`cd`, `rdata`, `sdata`, `_lih_data`, `_bind_check`, `_lfwildcard`, `_senddata`, `_lfversion`), and
  the loader compares the server's version with `__ps2_klibinfo__`+12 ("PsIIlibkernl2710", another
  object's global). The IOP heap's queries return unsigned (failure is lui/ori 0xffffffff).
- **A goto loop keeps an invariant in the loop**: GCC 2.9's loop optimiser hoists only from
  while/do/for, so DIntr's mask reload (open since round 2a) appears with `again: ... if (sr &
  0x10000) goto again;` -- 14 words to 7, the rest a register pair and the duplicated `res = 0`
  return (not adopted: still open).

**The tools, round 6a**: `scdiff.gnu_static_size` -- a GNU object relocates its statics against the
section, so `local_object_ok` sized a `.data` scalar by its heuristics, and an int of -1 followed by
a pointer read as a 5-byte string (loadfile's `_bind_check`, then `_lfwildcard`: a false WRONG
REFERENCE); plain data now takes its LOCAL OBJECT symbol's size, an object holding a relocation keeps
the old extent. Proved: every MW hand-written unit (`--files '^(?!auto/|lib/)'`, 5,015 rows) gives
byte-identical output under master's scdiff and this one; every GCC unit (src/lib and the 713 GCC
drafts after merging master d75bccc: 1,279 rows) the same rows but sceSifLoadFileReset, WRONG ->
match. Found on the way, left for its lane: the recogniser's FillLanguageVirtualTable is checked
against one byte of its table (the first slot is null, so the heuristic reads a string); checked to
the symbol's size, one slot names another address. automatch was not run this round.

**The tools, round 7a: a referenced object is compared whole.** `scdiff.local_extent` finds the
TU-local object a reference lands in -- MW's own symbol, a GNU object's sized LOCAL label
containing the offset (a table too, and a reference into its middle), a label without a size by its
data.txt row -- and `local_object_ok` compares all of it, at the target address the reference's
offset implies. A word relocated inside it resolves through the CANDIDATE's own function at that
offset, by that function's map address (`table_word_ok`; the same distance from the referencing
function only when the map does not name it): language.c's four OPEN functions compile to other
sizes, so by distance 73 of _LangVirtualTable's 79 slots would move, by name all are right (the 7c
lane checked them by hand: FillLanguageVirtualTable was no false positive). A pointer into the
object's own data (a string table) is checked by what it points at, one level deep; a static the
data map names by its address AND its content (MW's were by address only; a GNU one, relocated
against its section, by content only); an unnamed GNU literal at least over the width its load
reads (a float whose first byte is zero is not a one-byte string). Proved over every unit (`--files
.`, 3,094: hand-written MW and GCC, every draft): the 10,965 output lines are identical under
master's scdiff and this one -- no verdict changed in either direction -- while 982 references in
224 functions are compared over more bytes: 891 data-map-named MW statics now by content (88 in
.data/.rodata, the .bss ones where the image holds that memory), 60 MW references at an offset into
an object, 11 GNU tables (FillLanguageVirtualTable 1 -> 316 bytes, FillContextV2VirtualTable 1 ->
84, InitAlarm and InitTLBFunctions 2 -> 64, InitSystemCallTableAddress 2 -> 16) and 20 GNU literals
(the codec's `.lit4` floats 1 -> 4). `test_scdecomp.py`'s `test_gnu_table_is_compared_whole` pairs
both with a corrupted image (a slot +8; the codec's 1.40234375f, 0x3FB38000, as 0x3FB48000): each
was a MATCH under master's scdiff, each fails now. Still by address only: a GLOBAL a unit defines
(its content is data, scored nowhere). Re-proved after merging master 7ba1b5ba (7b's and 7c's units
in): 11,110 lines, one verdict changed -- **asrspi_OpenEngine, WRONG REFERENCE -> MATCH**: its
`g_APIRecogCallBacks` (eight callbacks, slot 3 null) was read as its first three relocated words
and checked by distance, and asrspi_ActivateWords, open and 311 words off in size, sits between the
function and the callbacks; by name all eight slots are right (checked apart, slot by slot). A
false WRONG on master's tools, not a hole in these.

**Round 7a** (2026-11-03, by hand; the tools above first): 32 rows newly byte-exact (8,870 -> 8,902 on
master's tools, 8,903 on these), 13 matched drafts superseded, 12 rows and 4 data rows named. libkernl's `sdk/loadfile.c` whole (18 of 18: `_lf_bind`, open since round
6a, and the six module/ELF/IOP-memory calls), libmc whole but one (`sdk/libmc.c`, 27 of 28: 21 new;
12 rows named by the demo's order, each checked by its RPC number -- sceMcInit, sceMcEnd,
mcHearAlarm, `mcDelayThread@libmc` (OVL2's copy at 0x62C180 holds the bare name), sceMcChdir,
sceMcFormat, sceMcDelete, sceMcFlush, sceMcSetFileInfo, sceMcRename, sceMcUnformat,
sceMcGetEntSpace; data.txt gains the demo's labels buffFileInfo, sifParamFname, sifParamNext and the
newer release's semaidDelay), and five libgraph objects (`sdk/gs_*.c`, `sdk/libgraph.h`: 5 of 6 --
sceGsPutDispEnv, sceGszbufaddr, sceGsSyncV, sceGsSetDefAlphaEnv, sceGsGetGParam over its draft).
What they showed:

- **A constant-size memcpy is GCC's block move**: loadfile's 252-byte argument copy (the run-time
  alignment test, an `ld`/`sd` loop and an `ldl`/`ldr` one) and libmc's 64-byte file-info copy
  (`ldl`/`sdl` both sides: a `char *` source makes both unaligned) are `memcpy` calls with a constant
  size, never a struct assignment (aligned `ld`/`sd`).
- **A result read before a store that may alias it**: loadfile's module calls return `_senddata.id`
  read into a local before `*result` is stored (`int *` may alias the static).
- **Block layout follows the source's nesting**: `_lf_bind`'s bind loop sits inside
  `if (_bind_check < 0)`, its success path returning from inside the loop -- the delay loop is laid
  out last and the first test jumps to the final `return 0`; libmc's null-argument cases come last
  as `if (argp != 0) {...} else`.
- **libmc is a later release than the demo's** (functions 4-16 bytes apart, a second semaphore for
  the delay alarm): its rows had no exact names; the demo's order and the RPC numbers (ps2sdk's libmc
  reconstructs the same protocol and command layouts) name them.
- **Sony's sceGsGParam is 16-aligned**: only then does GCC fold sceGszbufaddr's `InterMode == 1 &&
  FFMode == 0` into one 64-bit load and a 0x0000FFFF0000FFFF mask; libgraph's demo objects hold one
  function each.
- **Open, diagnosed in place**: sceMcWrite (the head split's `buff - 16` is reassociated here, kept in
  the target), sceGsResetGraph (4 words, a v0/v1 swap).

**Round 10a** (2026-11-03, by hand, MAIN's densest library first): Sony's libmpeg, five of the
demo's six objects -- 46 rows newly byte-exact (7,072 bytes), 31 matched drafts superseded, 77
functions byte-exact of 82 written, 17 rows named. `sdk/libmpeg.h` (the public sceMpeg and its
callback data as Sony's libmpeg.h declares them; the decoder's private state, sceMpeg.sys, 0x10C0
bytes, its offsets checked by `MPEG_AT` typedefs), `sdk/mpeg_api.c` (the demo's object 0x1593B8:
sceMpegInit ... _sceMpegFlush, the allocator, the picture loop; 35 of 36), `sdk/mpeg_sysbit.c`
(7 of 7, the system-stream bit reader), `sdk/mpeg_seq.c` (0x159F10: the resets, the error path,
_sendDataToIPU's DMA chain, the sequence header and extensions; 17 of 19), `sdk/mpeg_defcb.c`
(2 of 2) and `sdk/mpeg_dec.c` (the decoder object's bitstream layer and header parsers, 16 of 18;
its motion compensation and picture output are not written). Named `s1:order`, each body checked
against the demo's: sceMpegAddBs, sceMpegSetDefaultPtsGap, sceMpegResetDefaultPtsGap,
sceMpegSetImageBuff, sceMpegDispWidth/Height, sceMpegDispCenterOffX/Y, sceSetPtm,
_alalcSetDynamic, _initSeqAgain, the four extension stubs (by their own messages too),
_defStopDMA, _defRestartDMA. What they showed:

- **libmpeg is 991111's**: sibling calls in void context only (`_sysbitInit`'s `j _sysbitFlush`,
  `_pictureHeader`'s `j _updateTempTackData`), loops aligned to 8 (the padding nop at a loop's
  head), every function byte-exact under the dtls13010 build as written.
- **The IPU's idle wait and command write are macros**, expanded in the bit readers as well as
  standing as functions (`_waitIpuIdle`, `_sendIpuCommand`): the expansions keep no constant in a
  saved register where the functions do. The command's `>> 28` index into `_isDirty` is `srl` or
  `sra` by the command expression's own type at each site.
- **A value fold would distribute is its own statement**: `(size ? 3 : 0) << 28` folds into a
  ternary of shifted constants and a branch, `ps != (c ? 2 : 1)` into two compares -- the target's
  movn means the value was computed first (`_sendDataToIPU`'s tag id). And `ret = ret &&
  _decPicture(d)` after `ret = _updateRefImage(d, 0)` routes the result through a temporary (the
  target's `move s2,v1` at the join): `ret` in its own expression is not a safe target.
- **Field extraction order shows in the registers**: `_sequenceExtension` takes the extension's
  fields from the low bits up (bit_rate, vertical, horizontal, chroma) before its first check.
- **Stores, measured** (`tools/ccperm.py`, 14 functions): with independent stores at a function's
  end the last source store is often scheduled first and the delay slot takes the next-to-last --
  but not always; measuring was quicker than predicting every time.
- **Open, diagnosed in place**: _decodeOrSkipField (the constant 1 lives in s1 across the calls and
  feeds three movn's), _initSeq (right size, the reallocation block's schedule), _initRefImages
  (the target multiplies by 384 once per picture, GCC CSEs every spelling), _ipuVdec (IPU_TOP read
  as `lui; ld 0x2030(reg)`), _extensionAndUserData (both start codes hoisted into saved registers).

**The tools, round 10a**: `tools/ccperm.py` keeps the probe's own directory on the include path
(its variants are compiled under build/ccsweep, so a probe's `#include` of a header beside it was
lost and every order failed to build). No tool that scores changed: scdiff.py, sccheck.py,
scdecomp.py and `include/` are byte-identical to master's (3daf64e5), so no unit can score
differently. automatch was not run this round.

**Round 11a** (2026-11-03, by hand; the tools below first): 46 functions byte-exact of 57 written --
39 rows newly byte-exact (9,514 -> 9,553), 7 matched drafts superseded -- and 3 rows and 1 data row
named. libmpeg's reference interpolators (`sdk/mpeg_dec.c`, 35 of 38: the sixteen `_rix_`/`_ri0_`,
`_copyAddRefImage`, `_ipuSetMPEG1`, `_dmVector`); libkernl's SIF RPC object (`sdk/sifrpc.c`, 13 of 23),
its thread helpers whole (`sdk/thread.c`, 5 of 5), its DECI2 console (`sdk/tty.c`, 4 of 7) and its
console output object's writers and entry points (`sdk/kprintf.c`, 5 written of 8: kputchar,
deci2Putchar, serialPutchar, kprintf, scePrintf). Named: `_sceRpcFreePacket`, `_request_end`
(s1:order, the demo's sifrpc object; libmrpc's copies are 0x1BCF00/0x1BCF20), `_request_rdata`
(s1:ref: cmd 0x8000000C's handler, a later release's), data `_data_table` (the demo's label). What
they showed:

- **libmpeg's interpolators are inline asm in noreorder mode**: written for reorder mode (each filled
  slot handed back to GNU as) 13 of 16 refill as the target under the dtls13010 build's as and
  2.96's -- none under 991111's (10-51 words off), libvu0's tell again -- but four clear a flag in the
  slot of the branch that tests it (`bnez t8; move t8,zero`), which reorder mode cannot write.
  `_copyRefImage` carries its clamp constant `_maxval` behind `.align 4` (lq needs 16 bytes): its nops
  follow the object's own layout, so it is OPEN until the four C functions before the interpolators
  are written (byte-exact with 8 bytes put before `_rix_000`).
- **A function-local static is `NAME.N` in the demo** (InitThread's `stack.6`): this compile numbers it
  `stack.9`, so its data row would not resolve; the reference stays unchecked. A bare static label
  resolves through the data map by its bare name -- a `.bss` table libmrpc's copy already names
  (`_packet_buffer`) cannot be named for sifrpc's without qualifying data labels too.
- **Sony's libkernl is not ps2sdk's reconstruction**, near as it is: the thread helpers return the
  id they queued (ps2sdk 0), test it unsigned and read the running thread with an inlined syscall
  -47; the console's queue is `{size, count, rp, wp, buf}` advanced by peek-then-done calls;
  `_request_end` is a switch with an empty third case (0x8000000C), which gives the target's case tree
  (0x8000000A at its root: two cases alone make a chain); the console writers take an int.
- **Volatile, field by field**: the console's `tinfo.socket` is read back after its store
  (sceTtyInit) and `writing` is spun on (sceTtyWrite); `read_queue` is plain -- sceTtyRead is
  byte-exact only so.
- **Open, diagnosed in place**: sifrpc's ten share one cause first -- the target never hoists a load
  above a store through another structure, which this build does under strict aliasing; the unit
  keeps strict aliasing (without it _request_bind comes to 9 and _request_rdata to 8, but four matched
  functions are lost), so the vendor's packet field types are not yet these; sceTtyInit (16:
  the failure return jumps to the shared epilogue), sceTtyHandler, sceTtyWrite, _copyRefImage (above).

**The tools, round 11a**: (1) **a unit names its overlay**: `/* sc: overlay=MAIN|OVL1|OVL2 */` --
each function scores against its row in that overlay (the named row there, else the row an
`ovl-copy:` mark names), `no-target` when the overlay has neither, an unknown overlay a build
failure. MAIN's zvid.cpp (0x181C20, marked `ovl-copy: zVid_Assert__FbUiPci`) then scores at its own
address, and a MAIN unit defining `main` does once 0x1C4CC0 carries `ovl-copy: main` (tested with
the mark: it scores against the 0xCF8 row). (2) **`tools/try.py` builds as sccheck does**
(`sccheck.build_one`, now given the tree's root): the unit's toolchain and flags, an assembler unit,
`as=`, `NAME@UNIT` statics, `overlay=`; `-n` reads `\n` and `\t` in a substitution (the MAIN runtime
lane's var.py folded in). (3) **`scdiff.code_label`**: a GNU object's `.text`+N reference at a
local label the data map names resolves by its address (libmpeg's `_maxval`), whatever the unit's
layout before it; the canary's `test_gnu_code_label_resolves_by_the_map` moves the label as its
negative. Proved over every unit (`--files .`, 2,889 units, 12,038 lines) under master's scdiff,
sccheck and scdecomp and these: identical but `_copyAddRefImage`'s `_maxval`, unchecked -> ok; no
verdict changed. automatch over this lane's GCC ranges (`--range 18DB90 1C4CC0`, `1CCD78 1CE2A4`,
`1E7F58 1E8FD4`, `243000 256FD0`, `--max-size 0x400 --emit --no-prune`, 2 jobs): 526 tried, 0 hits
(m2c refused 44, 99 drafts would not build).

**Round 12a** (2026-11-03, by hand, half of it OVL1's codec): 73 rows newly byte-exact -- 71 by hand (of
74 functions written or rewritten), 2 drafts -- and 12 rows named. MAIN: libscf's getters and AdjustTime (`sdk/libscf.c`
18 of 21: +6), libdbc (`sdk/libdbc.c`, new: Sony's DBC client under libpad2, 7 of 9 -- sceDbcSetWorkAddr,
DeleteSocket, InitSocket, ResetSocket, GetDeviceStatus, 0x1904F8, the compiled-out debug printf).
OVL1, Nellymoser's codec, a unit per source file the asserts name (58): `ovl1/sase_encsc.c` (EncSC.c's
setters, 11 of 11), `sase_encapi.c` (the encoder API, 12 of 13), `sase_parapi.c` (the parameter block's
API, 13 of 13), `sase_decsc.c` (4 of 4), `sase_decapi.c` (the decoder API, 5 of 5), `sase_ldpda.c` (4),
`sase_libvect.c` (3), `sase_codepv.c` (3), `sase_libquan.c` (1), `sase_misc.c` (libsnd.c's release,
QP0SC3.c's dequantiser). Named (12, `sceDbc*`): libdbc is not in SOCOM 1 (it links libpad), and each
RPC wrapper prints its own name in its error message. What they showed:

- **Sony's getters keep one exit**: libscf's five open since round 2a store their value in one `ret` on
  every path (`if (IsT10K()) ret = gConfig.X; else { ...; ret = p2.x; } return ret;`) -- a `return` per
  path builds the T10K path's address in v1 where the target has v0, loads the field straight into v0,
  and selects GetTimeZone's 540 by branch where the target has movz. AdjustTime tests `m >= 0` first (its
  else loop laid out last, entered by the bltzl) and sums into a variable of its own: expand_binop puts
  the target first when it is an operand, so `min += t->minute` and `min = t->minute + min` both give
  `addu s0,s0,v0` against the target's `addu s0,v0,s0`.
- **adddate and subdate are GCSE's**: byte-exact under dtls13010 with `-fno-gcse` -- this build's GCSE
  removes the t->year reload the target keeps -- while GetRomName needs GCSE (it keeps gRomName's %hi in
  s1 and builds the address from it twice: 28 words off without). Their source differs from these
  somewhere still (round 4a's 990721/2.95.x match was the same effect: those builds have no such GCSE).
- **The codec's asserts name its objects**: `__assert("../../SaseEncVad/source/EncSC.c", 876, "pEncMemBlk !=
  NellyNull")` is newlib's `assert` in the vendor's file at that line; a `#line 876
  "../../SaseEncVad/source/EncSC.c"` before the assert gives __FILE__ and __LINE__ exactly, the
  expression text is the macro's `#e`. 27 source files are named so (libquan.c 23 functions, libtran.c
  19, LDPDA.c 15, PtchCand.c 12, EncSC.c 12, ...); the entry points carry none (a handle check: a 64-bit
  magic 0xface, the kind -- 10 the encoder, 11 the decoder -- then 6100/6110/6115).
- **The codec is two libraries built apart**: the encoder (SaseEncVad, 0x2484A0-0x253510) reaches its
  scalars and .lit4 floats through $gp (-G8); the decoder (SaseDec, 0x253510-0x256F48) reaches the same
  host callbacks (MAIN 0x1D55A8-0x1D55C0) by lui and an offset -- built at -G0 (automatch's ranges now
  split there: two decoder drafts matched at once, 0x254250 and 0x256970).
- **This GCC's decimal-to-float conversion is not the vendor's**: `0.011747969314455986f`, the exact
  value of the target's 0x3C407A8E, converts to 0x3C407A8D here (so does its exact decimal expansion, 30
  digits); a hex float (`0x1.80f51cp-7f`) gives the bits exactly. The codebooks are written so;
  QP0SC3's constant as a decimal between the target's float and the next.
- **Shapes, measured**: a setter's changed-flag store comes first in the source (the scheduler reverses
  the pair); a flag-only setter reads the parameter block before its assert, as a declaration's
  initialiser (the load sits in the assert branch's delay slot); a callee whose value is discarded is
  declared returning int (the free callback's pointer is then loaded into v1 after it, as the target's
  is -- round 5a's void-callee variant the other way round); the decoder's destructor leaves the
  caller's pointer as it was where the encoder's zeroes it.
- **Open, diagnosed in place**: frombcd (the product allocated to LO -- mulsi3's two-operand
  alternative -- in eleven spellings, 64-bit ones too), adddate, subdate (above); sceDbcCreateSocket and
  sceDbcReceiveData (the buffer's base held in a saved register from the first store; a fourth saved
  register here), the encoder's func_0024F888 (the coded buffer's address born while the handle is
  live: three saved registers in the target, two here).

**The tools, round 12a**: (1) **`scdiff.lit_entry`: a literal-pool entry is compared alone** -- a GNU
`.lit4`/`.lit8` reference was compared as an unnamed literal over up to 16 bytes, but the object's pool
holds its constants in first-use order and the image's in the link's, so a right constant read its
neighbours (QP0SC3's dequantiser: a false CONTENT-MISMATCH); it is now compared over its own 4- or
8-byte entry. Narrower than before, so no verdict could turn from a match to a miss. Proved over every
unit (`--files .`, 10,243 functions scored) under master's (74a85b36) scdiff and this one: the 9,976
lines are identical but func_00250C68's, WRONG REFERENCE -> match (9,774 -> 9,775); the canary's
`test_gnu_literal_is_its_own_entry` pairs it with a corrupted entry (fails) and a corrupted neighbour
(still matches). sccheck.py, scdecomp.py and `include/` are master's. (2) **automatch's GCC ranges**: the codec split by its -G (the encoder and the host-callback
setters -G8, the decoder -G0, the UDP ring's last two with libnetb), Sony's SDK rows tried under the
dtls13010 build first (991111's C compiles the same but where that build's sibling call shows), and
`build/automatch/verdicts.txt` lists each row's verdict. **Why GCC drafts do not build** (the lead
from round 11a): over this lane's 463 placeholder rows up to 0x400, m2c's GNU C as automatch prepares
it builds for 404, m2c refuses 18, and 41 fail: an undeclared frame base -- `sp` (13) or a slot
`spXX` (6) where `stack_locals` declares none (no locals below the saves), `saved_reg_*` (2) -- an
int dereference the repair misses (10), an argument count disagreeing with another call (5), a parse
error on m2c's cast spelling (6); automatch's own tally (99 "build", round 11a's too) counts besides
the named rows the ranges hold (sceCd*, sceSif*: GCC ranges try rows with real names) and every
exception `try_one` swallows. So the drafts mostly build and miss: the near misses are what to
classify next (`tools/amsurvey.py near`). automatch over the four ranges (`--max-size 0x400 --emit
--no-prune`, 2 jobs): 445 tried, **2 hits** (both decoder rows the -G0 split opened), 99 build
failures, 42 m2c refusals.

**Round 13a** (2026-11-03, by hand, the network clients and libcdvd first): 53 functions byte-exact of
55 written -- 52 rows newly byte-exact, PowerOffCB over its draft -- and 15 rows named. OVL1: libnet
whole (`ovl1/libnet.c` 44 of 44: the Recv/Send family, Control, the interface, routing and name-server
calls, sceLibnetInitialize/Terminate), libnetb (`ovl1/libnetb.c` 20 of 22: RecvFrom, SendTo,
GetAvailBytesToRead, the newer 2-byte receive and send with their wrappers, the connection-state check,
both prepare_async calls), libezcnf (`ovl1/ezcnf.c` 6 of 6: the bind and three RPC calls) and libezctl
(`ovl1/ezctl.c`, new, 6 written). MAIN: libcdvd's first object (`sdk/libcdvd.c` 17 of 18 with
`sdk/libcdvd.h`: the delay thread, the N-command callback, its thread and the read interrupt, the
semaphores, the power-off trio, both pre-checks, sync) and four of its one-function objects
(`sdk/cd_disktype.c`, `cd_geterror.c`, `cd_poweroff.c`, `cd_clock.c`) and `cd_mmode.c`. Named: by their
own messages sceLibnetInitialize, ezNetCnfGetCount, ezNetCnfGetCombinationList,
ezNetCnfGetNetcnfifData (and ezNetCtlSetProperties/SetCombination/GetStatus with s1:order); s1:order
libnetb_prepare_async_receive/send (RPC 100/101), initEzNetCtl, exitEzNetCtl, ezNetCtlSetAutoMode,
UpInterface, DownInterface (the demo's libezctl object 0x425EA0 row for row: sizes and RPC numbers 2-6
agree, the demo reaching its flag by $gp); sceLibnetTerminate by the demo's libnet_term's body. What
they showed:

- **libcdvd's shared words are volatile, one by one**: the callback number and its copy, the
  semaphore ids, the busy flag and the callback thread's cleared word are never reordered nor put in a
  delay slot (a volatile load also stays out of a call's slot: `lw a0,id; jal iSignalSema; nop`), the
  thread id is plain (sceCdInitEeCB reads it once for both uses). The pre-checks compare `id !=
  PollSema(id)` (read twice).
- **The caller's $gp is one asm statement**: sceCdCallback stores `$gp` with `sw $28, %0`; the
  callbacks swap it with `move %0,$28; lw $28,%1` in ONE statement -- two statements let the saved
  word's `lui` follow the move (_sceCd_Poff_Intr), the target computes it first.
- **A local initialised at its declaration is the target's early address**: sceCdMmode's send buffer
  and every libezcnf/libezctl call hold the buffer's address in s0 from the entry (the store still goes
  through `%lo(sym)(s1)`: GCC folds the known sum back). And sceCdMmode reads its RPC number and sizes
  from three separate initialised words (34, 4, 4; one `lui` each, so not a struct) where the demo's
  used immediates.
- **2.96's `while (i--)` steps by 4 in one function and by 1 in another**: libnet's init and term
  delays (`i = 0x10000; while (i--);`) compile to a count-down by 4 here and in the demo, lgAud's by 1;
  as written, both match -- the spelling is not to be read off the step.
- **SCEA's wrappers return their success value first**: libnetb's RecvFrom/SendTo and the 2-byte pair
  lay the error path (`err ? err : -501`) after the success path only with `if (f(...) == 0) return
  len;` first; their out-locals' frame slots follow the declarations (measured: `len, port, flags`).
- **Open, diagnosed in place**: sceBInetSend (8 words, registers: both DMA descriptors take their two
  loaded words in the other registers in every store order; an `__inline__` helper gives each transfer
  its own descriptor, frame 80), func_0018E9E8 (a newer libcdvd accessor: the target's switch is an
  equality chain with the bodies after it, this build makes a jump table for eight cases in a range of
  20).

**The tools, round 13a**: `tools/automatch.py --obj-dir DIR` (repeatable) reads a lane's private
`sccheck.py --build-dir` objects with build/obj's, so `hand_defined()` sees what the lane's own source
defines -- round 5a's 40 hits on rows hand-written source already matched (the drafts then deleted by
hand) do not recur. automatch does not score: scdiff.py, sccheck.py, scdecomp.py and `include/` are
byte-identical to master's (046cfe21), so no unit can score differently. automatch over this lane's
ranges (`--max-size 0x400 --emit --no-prune --jobs 1 --obj-dir` the lane's `src/lib` build): MAIN
`18DB90 1C4CC0` 233 tried, 0 hits (62 build failures, 26 m2c refusals), `1CCD78 1CE2A4` 6 tried, 0
hits, OVL1 `1E7F58 1E8FD4` 12 tried, 0 hits, `243000 256FD0` 145 tried, 0 hits (27 build failures, 9
refusals): 396 tried, none byte-exact, and the hand-written rows (CB_DelayTh, sceCdSync,
sceCdGetDiskType ...) were not retried. What is left to the generator in these ranges is near misses.

**Next**: the libraries by hand, module by module, as their vendors wrote them (each `src/lib`
unit's header says what it covers); GCC's scheduler moves independent stores, and which source
order gives the target's is easier measured than predicted (`tools/ccperm.py PROBE
FUNCTION` tries every order of a marked block). libkernl's thread, tty/printf and SIF objects
(sifcmd, sifrpc, the libmrpc copy), fileio, libgraph, libipu/libmpeg, libmc, libpad (its version
is not the demo's: its rows are unnamed), libcdvd (its first object is dtls13010's: the sibling call
above; it reads its semaphores and callback number through volatile and plain globals the target
schedules apart), newlib's stdio bodies (_vfprintf_r, __svfscanf, findfp.c, fflush, __sfvwrite,
mallocr) and libm's fdlibm functions (the double moves above will hold some open) are open; the
network library at 0x1C0948-0x1C4CC0 (inet_aton at -O2 beside RPC stubs built at -O0: frame pointer,
spilled arguments) holds BIND's inet_ntop.c (round 5a: Sony's SDK toolchain, inet_ntop's returned
calls jal-and-return); in OVL1 lgAudInit (registers), libnetb's Open, Recv/RecvFrom, Send/SendTo,
GetAvailBytesToRead, recv_from/send_to and three helpers, its UDP ring's three, libnet's Recv/Send
family, Control, the interface and routing calls, and the codec (round 5a's survey: 0x251348,
0x251A98 -- a switch on the codec's mode the target tests as an if-chain -- and 0x2533B0, a
soft-float compare, are the nearest). automatch over OVL1's three library ranges (round
3a, `--max-size 0x400`) found nothing new: 298 tried, 0 hits; `tools/amsurvey.py near` lists the
near misses now that it drives GCC rows (round 5a). Still open: the strict-aliasing setting
of the SDK, newlib and the codec (no function yet tells), the 991111 rebuilds -01 and
a (no probe separates them from 991111), and the generated tier's GCC near misses, not yet
classified as amsurvey classified MW's.

### The recogniser and Medius by hand (round 2b)

**SOCOM 1's demo lists every GCC object.** Its symtab keeps GCC's `gcc2_compiled.` marker at
the start of each object file, and the objects' static functions as LOCAL symbols: a library's
units -- their spans, order and the names of their statics -- read straight off it (62 objects
in the recogniser, 0x336EF0-0x378890; 112 across Medius/DME/RT). SOCOM II keeps the order
within an object (GCC emits in source order), so a placeholder at the same position, with a
body that does what SOCOM 1's does, takes the name (`s1:order`). SOCOM 1's Medius objects
interleave with MW code, so a whole-range alignment wanders there; pair object by object.

- **ScanSoft built the recogniser with `-funroll-loops` too**: ContextActivateParams' 18-step
  loop is unrolled by 3 (2.96 picks 3 when 4 does not divide a constant count) -- 81s without the
  flag, byte-exact with it; all 165 recogniser drafts keep matching with it (ccflags) and every
  `src/lib/asr` unit carries it. SOCOM 1's copy is unrolled the same way.
- **What a call returns moves registers around it.** The host's notify callback returns
  nothing: declared `int`, the call sets v0 and the constant stored before it moves from v1 to
  v0. `_tosErrorLog` returns its code as an `unsigned char`, so every `return _tosErrorLog(...)`
  is a jal, not a sibling call (2.96 declines one when the modes differ; `unsigned int` against
  `int` does not stop it, `long` adds a `move v0,v0`).
- **The RT library is built per file**: rt_time is byte-exact only with
  `-fno-optimize-sibling-calls` (rt_time_get_base_diff's return of a call is a jal, even through
  a variable). The flag loses 42 of 112 drafts in 0x62C000-0x645000 and 31 of 104 in DME, but
  1 of 232 in the Medius client: a unit's directive says it, not the library's.
- **Shapes 2.96 decides**: a guard clause puts the return value in the branch's delay slot
  (the callbacks); a struct passed by value goes through the stack (`ldl`/`ldr` pairs: the 8-byte
  word selection and safe handles, the 24-byte NetAddress EABI passes by reference); `strcpy` of
  a literal is a block move; the vendor writes `c->params->x` where a local would hoist loads
  (ContextGetAction, ContextSetParam), and `list[n]` where strength reduction makes the pointer.
- **Open, diagnosed in place**: asrspi_cbResult (two identical null-result blocks the target
  keeps and 2.96 merges), casrGetLastError/Warning (2.96's if-conversion of SOCOM 1's plain
  magic-number guard into `li 11; movz` -- no shape tried reproduces it), casrImportContext, the
  error translator at 0x4C6078; round 1's casrDoSecondPass and cbGenderDisable.

Hand-written this round: `asr/asrspi_cb.c` (6 of 7), `asr/context.c` (8 of 9), `asr/gsapi.c`
(8), `asr/cb.c` (+3), `asr/tos.c` (2), `medius/rt_msg.c` (+12: the unsigned accessors and eight
network-order twins SOCOM 1 did not have), `medius/rt_time.c` (6), `medius/mediusutil.c` (2),
`medius/mediusapi.c` (4) -- 51 functions, 47 of them new to the map's byte-exact rows (four
supersede matching drafts: asrspi_cbAgc, ContextSetCtxData, rt_msg_get_u2byte,
iCreateAndAllocateMessageID).

**Round 3b: whole objects, by template.** Most of SCEA's open rows are a few shapes repeated --
a request wrapper, a locked wrapper -- so a shape written once and a generator reading each
function's parameters off its disassembly (`build/scratch/gcc/genapi.py`, `genlock.py`) wrote
whole objects at once; every function is still compiled and compared as any hand-written one.

- **The Medius request**: strings terminated (`iTerminateString`, in order, before the guards or
  after them -- SessionBegin/End), `GlobalInitFlag == 1` (else -15) and `MyConnectionIndex >= 0`
  (else -16), the callback kept in the request's own slot, `NetSendMessage(64, index, 0xFFFF,
  1 or 4, PacketType..., sizeof *request, request)`, -5 if the send fails. The values are the
  library's own names: `MediusGetErrorCodeString` (0x64A5D8) switches over every MediusErrorCode.
- **SOCOM 1's callback slots name the requests.** Each request stores its callback in its own
  global, and SOCOM 1's symtab names them (`CreateClanCallbackFunctionPtr` ..., data 0x5B1A3C and
  0x43B958): SOCOM II's slots are SOCOM 1's in the same order with SOCOM II's new requests
  inserted, so a function's position in the object and the slot it stores give its name twice
  over (44 names). Two names the online lane gave by call site sit on requests new in SOCOM II
  where SOCOM 1's function is another row: `MediusGetClanMemberList` (0x6468C8; SOCOM 1's is
  0x646708, by slot) and `MediusLadderPosition` (0x64B1B8; SOCOM 1's is 0x64A018). The game's
  source calls both by those names, so the map keeps them.
- **Each object declares `GlobalInitFlag` as it reads it**: the API object, which defines it,
  reads a halfword (SOCOM 1's symbol is 2 bytes); the clan and file objects read a word. That
  and the file API's callback slots (after the clan's in .bss, apart from the API's in .data)
  cut the file API out as its own object.
- **The RT library's object order is alphabetical by source file** (case-insensitive): the
  CVS `$Header:` strings in .data run audio.c, codec.c, DmeLocal.c, DmeMessage.c, Dme.c, grid.c,
  LargeInt.c, md5_platform.c, RSA.c, rt_audio.c, rt_comm, rt_crypt.c, rt_msg_client.c, rt_p2p.c,
  rt_udp.c, rt_upnp.c, SHA1.c, SHA1_platform.c, StreamMedia.c, and the code follows (rt_circ_buf,
  rt_comm, ..., rt_msg_client, rt_msg, rt_mutex, rt_time ...).
- **SOCOM II's DME and RT APIs are locked wrappers**: the public function tests that the
  library is up, locks its mutex, calls the internal function (the body SOCOM 1's same-named
  function has) and unlocks; SOCOM 1's DME has no such layer. In rt_msg_client the started test
  compares the flag with the result variable's first value (`int result = 1; if (started !=
  result)`): the target keeps the 1 in the result's register (ccsweep, 1 of 7 shapes).
- **Store order by permutation**: the universe-information requests keep five globals; one
  order of 120 schedules as the target does (`tools/ccperm.py`).
- **Names corrected**: 0x6473F0 is `MediusGetBuildTimeStamp` (it formats the Medius library's
  own date, beside "Medius Client Library Version 1.50.0013"); the s1:exact `NetGetBuildTimeStamp`
  it carried is SOCOM 1's byte-identical DME twin, 0x620518 here (Dme.c's date).

Hand-written in round 3b: `medius/mediusclan.c` (30 of 30, new), `medius/mediusapi.c` (+74, 78
of 78), `medius/mediusfile.c` (13 of 13, new), `medius/dme.c` (78 of 78, new),
`medius/rt_msg_client.c` (24 of 24, new), `asr/gsapi.c` (+3, 11 of 11) -- 222 functions, none
with a matching draft. Open, diagnosed in place: gsapi's engine accessors (SOCOM 1's
CheckEngineHandle inlined comes within 11 words; the target zeroes the result before the tests).
Next in the same vein: rt_msg_client's internal functions, DME's internals behind each wrapper
(SOCOM 1's same-named bodies), and gsapi's remaining wrappers (handle test, a context loop).

**Round 4b: the RT objects, DME's internals, the globals named.**

- **Medius's globals carry SOCOM 1's names in `data.txt`**: `GlobalInitFlag` (0x656188, 2 bytes)
  and `MyConnectionIndex` (0x65618C), referenced at the instructions SOCOM 1's MediusSessionBegin
  references its own (133 and 132 references), and 69 request callback slots (`s1:slot`: SOCOM
  1's slot of the same position, its request storing it as SOCOM 1's does, each with its count):
  44 of the API object's (.data, 0x6561C0-) and 25 of the clan object's (.bss, 0x686A5C-). The
  two slots whose function names were disputed took SOCOM 1's names once the integrator settled
  them (round 5b: 0x656278 MediusLadderPositionCallbackFunctionPtr, 0x686AC0
  GetClanMemberListCallbackFunctionPtr -- SOCOM 1's 0x43BA08 and 0x5B1AA0, between the same
  neighbours). The units dropped their `#define`s.
- **The link order is the .data `$Header:` order, but not alphabetical**: DmeLocal.c,
  DmeMessage.c, *then* Dme.c. Each object's functions address its own .data, so a function's
  object reads off the `$Header` span its references land in (`build/scratch/gcc/dataobj.py`):
  audio.c 0x60FF00-0x612630; an object without a `$Header` -- SOCOM 1's first DME object,
  NetInitializeClients .. NetClientLeaves, 15 rows, 0x6126A0-0x6132B8; codec.c to 0x613FD0;
  DmeLocal.c (SOCOM 1's Dme.c bodies) from 0x613FD0 past 0x61CCA8; DmeMessage.c around 0x61F518;
  Dme.c from 0x6204B8 (NetGetBuildTimeStamp, then the locked wrappers); grid.c from 0x6250D0;
  md5_platform.c at 0x62B168.
- **SCEA's shapes, each measured**: one guard clause per test, each returning its constant (an
  `||` of them makes 2.96 build a shared return block the target lacks: rt_mutex); a test whose
  else-block the target carries in a branch-likely's delay slot was written the other way round
  (`if (used != 0) {...} else *p = 0;`, `if (ok) { ...; return 0; } return ERR;`); a count bumped
  in each branch, which 2.96 cross-jumps into one store; a call's result returned through a
  variable after a join is no sibling call (func_0061BEB8).
- **Per-object flags again**: rt_memory.c is built without sibling calls (rt_memory_align_alloc
  returns rt_memory_get_aligned_address's result through a jal: 14 words off with them), as is
  the recogniser's user-word object (DoLabelStop/DoLabelStart: 10 and 6 off); rt_mutex,
  rt_circ_buf (normalize ends in `j`) and DmeLocal.c (DmeFree is `j rt_memory_free`) keep them.
- **The build stamps** (MediusGetBuildTimeStamp, NetGetBuildTimeStamp, rt_msg_client's): each
  object's own `__DATE__`/`__TIME__`, spelled out (a rebuild's would be today's); the month table
  is a 38-byte literal -- its last byte is copied from .rodata, so the vendor's ends in an explicit
  `\0`; the date and time are local pointers, set after DME's and the client's argument test.
- **Open, diagnosed in place**: rt_circ_buf's nine (after `result = getter(cb, &n); if (result)
  return result;` the target tests n in v0, 2.96 here in v1 -- every shape tried; free's and use's
  start update is byte-exact only under `-fno-sched-interblock`, which no other unit takes), four
  of DmeLocal.c's and cbDoUserWordTraining (2.96 folds `if (f()) return 1; return 0;` into `sltu`).

Hand-written in round 4b: `medius/rt_mutex.c` (5 of 5) and `rt_mutex_platform.c` (6 of 6),
`medius/rt_memory.c` (8 of 8), `medius/rt_circ_buf.c` (7 of 16), `medius/dmelocal.c` (15 of 19),
`asr/userword.c` (4 of 5), and MediusGetErrorCodeString, MediusSetClientAddrList,
MediusGetBuildTimeStamp, NetGetBuildTimeStamp, rt_msg_client_get_build_timestamp in their units
-- 50 functions, 40 new to the map's byte-exact rows (ten supersede matching drafts); 15 rows
named (rt_circ_buf's 14 by SOCOM 1's order, FindFreePingSlot).

**Round 5b: DmeMessage.c and the rest of DmeLocal.c, typed.**

- **DmeMessage.c is SOCOM 1's third DME object, grown** (0x61DAA8-0x6204B8): RegisterDMEmessages
  (0x61FBC8) takes the handlers' addresses in its order, and SOCOM 1's object pairs with them by
  callees and returned sizes -- GetFreeNetObjectBlock and FindFreeDataStreamBlock first (SOCOM 1's
  bodies, the first now with a count guard), then ClientConnects, ServerVersion, ClientLeaves,
  ClientUpdate, RequestServers, ServerResponse, RequestOwnership. A handler takes (connection,
  unused, source client, message) and returns the size it consumed, -1 without a message.
  Pairing a handler's callees also names DmeLocal.c's internals (RemoteClientConnects is
  0x614B18, NetGetObject's body 0x614EC8, DmeFree 0x614888).
- **The records are typed now** (`medius/dmelocal.h`, after SCEA's DmeLocal.h): a world (0x450:
  256 client-record pointers, then this client's index at 0x400, the session master, its name,
  the server's version ...), a client record (0x2C), an object (class, status, owner, creator).
  func_0061F410 matches only with the world as a struct; dmelocal.c's matches hold typed.
- **SCEA's guards, measured**: a getter's result and its pointer are tested together --
  `if (result != 0 || p == 0) return result;` (closed round 4b's func_0061C8E8: 2.96 makes movn of
  every other shape); separate guards where the null pointer has its own code; a handler ends in
  one `return SIZE;` after nested tests, which frees v0 for temporaries (RequestOwnership); a local
  initialised inside the block that needs it (closed func_006147E0); a table indexed in place, not
  through a pointer local (closed func_0061C6C8).
- **Where 2.96 puts a local in the frame tells its scope**: an array or struct gets its slot where
  it is declared, a scalar when its address is first taken, slots rising in that order -- so a
  reply struct declared in the block that sends it sits above the outer block's flag
  (RequestServers), and a two-byte message is an array (func_0061A240).
- **Open, diagnosed in place**: ClientConnects (2 words: the port global reloaded in a bnezl's
  slot), ClientUpdate (19: the 36 it returns kept live in v0), 0x61F7B0 (3: two loads' order),
  func_00614758 (round 4b's), 0x617610 (7: store order, past ccperm's reach), 0x6189C8 (1: beq
  against beql), 0x61D108, 0x61D320.

Hand-written in round 5b: `medius/dmemessage.c` (11 of 14, new), `medius/dmelocal.c` (+27, 42 of
47, three of round 4b's four OPENs among them), the two callback slots in mediusapi.c and
mediusclan.c -- 38 functions new to the map's byte-exact rows (no draft superseded); 16 rows named (s1:order: SOCOM 1's
DmeMessage.c and Dme.c objects by position and callees), 2 data rows.

**Round 5c: the recogniser, object by object.**

- **The two data tables name every accessor** (`s1:slot`): FillContextV2VirtualTable and
  FillLanguageVirtualTable each copy a LOCAL table of SOCOM 1's (`_ContextV2VirtualTable`, 21
  words; `_LangVirtualTable`, 79 words) whole into the caller's (a struct assignment: 2.96's
  ldl/ldr block move); SOCOM II's tables are the demo's slot for slot -- the same length, the
  same null slots -- so each static takes the demo's name from its slot, not from its size or
  position: 0x508AD0 is `_langGetpErgodicStates` (it had the align pass's
  `_langGetpErgodicPhenomes`), 0x508B90 is `_langGetpCodeBook` (the align pass saw no pair), and
  the language file's section walkers are in another order than the demo's. The tables are
  statics of their units; scdiff resolves the `.data` reference by data.txt's name.
- **SOCOM 1's CheckEngineHandle is an inline function in SOCOM II** (the open engine accessors
  of round 3b): `static __inline__`, the array base read into a local first and the result
  set to 0 before the tests, `ok = e->engine != 0` when the engine is a live slot (null, below
  the base, not 108-aligned, past the last) -- byte-exact in five gsapi calls. An early-return
  inline keeps a 0 per failing test; a one-expression one loads the base after the null test.
- **ScanSoft's shapes, each measured**: one guard per test, even with the same code twice
  (safeh_HandleCheck: the `||` builds one shared code); `if (x < n) return 0; return 1;` for a
  boolean the target computes with `sltiu` (`!(x < n)` and `== 0` give `xori`); an engine-array
  loop takes the element's address into a local before testing it (indexing gives the operands
  of the address sum the other way round); a pointer kept as a local before an offset is added
  (`m = Dims + 8; return m + n` -- added at once, 2.96 reassociates).
- **Open, diagnosed in place**: the language file's offset chains (`_langGetpErgodicStates` 4
  words, Phenomes 6, Penalty 18, `_cdbGetpProbMatrix` 14: the same instructions, the sums in
  another order -- 45 spellings in `build/scratch/gcc/spec_*.py`), `_cdbGetCodeBookSize` (the
  target reloads the second codebook's size inside the smoothing test) and the six codebook
  walkers waiting on it, `_contextGetpSyntax` (a copied register), fastallo's allocation (p in a
  saved register to a single return), gsapi_NotifySetEnergyInterval and EngineSetUserData (the
  inlined check zeroes twice there).

Hand-written in round 5c: `asr/contextv2.c` (20 of 21, new), `asr/language.c` (64 of 76, new;
`_ConvPhenomesToStates` moved there from ctxutil.c), `asr/critsec.c` (5 of 5), `asr/safeh.c` (2
of 2), `asr/osspi.c` (4 of 4), `asr/fastallo.c` (2 of 3), `asr/gsapi.c` (+12, 21 of 23) -- 22
functions new to the map's byte-exact rows (4,016 bytes), the rest superseding matching drafts;
101 rows named (65 `s1:slot`, 30 `s1:order`, 3 `s1:align`, 3 `s1:calls`).

**Round 6b: the RT objects, whole.** The RT library's objects in 0x62C000-0x640000 read off SOCOM 1's
in order; SOCOM II grew most of them (a third channel kind, a connect test, an interrupt mask), so
each function is written from the binary in the vendor's shape and SOCOM 1's copy is the check
where it is s1:exact.

- **The unit list, by link order**: `rt_comm.c` (0x62C958, the channel is an allocated 0x18
  block whose address is its id), `rt_comm_platform2.c` (0x62DA90, sockets over libnetb),
  rt_comm_platform (0x62EA58, forwarders; drafts), `rt_crypt.c` (0x62EBF8, IsNullRSAKey first),
  ..., `rt_mt.c` (0x638CF0: Cokus's MT19937 as SCEA kept it -- no `$Header`, the name is the
  alphabetical link order's, between rt_msg.c and rt_mutex.c), setNewSeed_platform's object
  (0x639058), `rt_time_platform.c` (0x63D4E0).
- **Narrow return types stop sibling calls**: SOCOM II's internal functions return a type
  narrower than the API's `int` (rt_msg_client_map_comm_error, rt_msg_client_is_valid_connection,
  the platform's interrupt mask 0x639568), so `return f(...)` from an int function is a `jal` with
  nothing after it (the callee promotes): ccsweep, the void and int declarations give `j`.
- **A call returned after a guard on its own result is no sibling call** (rt_comm_metric_add_data:
  `result = f(); if (result != 0) return result; return 0;`), where `return f();` and a result
  variable both give `j`.
- **A leaf's loop sits inside its positive test**: `if (p != 0) { for (...) if (...) return 0; }
  return 1;` (IsNullRSAKey/IsNullRCKey); the guard-clause spelling gives the leaf a second `jr ra`.
  `i = 64; while (i-- > 0)` is the target's bgtz on the old index (func_0062FDD8); a signature's
  null inputs are one `||` (KM_DigiSign/Verify); nested `if (n > 1000) {...}` keeps the branch the
  store-flag fold would remove (rt_comm_metric_update/print).
- **A clamp passed as an argument**: `f(..., n > 4096 ? 4096 : n, ...)` to a libnetb parameter
  declared `unsigned short` gives 2.96's `sltiu; xori v0,v0,0; movz` in the argument register; a
  clamped local (int or short) re-extends.
- **Float literals are the vendor's short spellings**: `0.015734f` and `0.003390842f` give the
  target's 0x3C80E496/0x3B5E38E3 through 2.96's parser, where the exact decimals of those bits
  come out an ulp low (`build/scratch/gcc/fconst.py` compiles spellings and reads the bits).
- **Aggregate alignment shows in a struct copy**: the RC cipher state (0x10C) is word-aligned --
  KM_CopyKeys moves its last word with `lw/sw`, a byte array gives `lwl/lwr`.
- **Names**: 0x62CEF8 was not rt_comm_receive (the align pass's pair; it sends on the third channel
  kind) -- 0x62CFA0 is, by SOCOM 1's callees; rt_comm's own rows, the platform/platform2
  forwarders (startup, shutdown, create, send, receive, get_local_ip) by the calls into them, and
  the MT rows by order and Cokus's constants. 0x62CB68 is rt_comm's set_default_options by
  SOCOM 1's create (it calls it at the same point), but the map gives that name to 0x634958
  (`s1:exact`, a six-instruction body in rt_msg_client's object): left a placeholder.
- **Open, diagnosed in place**: KM_Create (8: a delay-slot choice), func_0062F818 (8 bytes: the
  result's zero set in each branch), rt_comm_platform2_receive_from (41s: flags and port in the
  other frame order), rt_msg_client_is_valid_connection_id (1: beqz against beqzl);
  setNewSeed_platform (the EE timers read with `lw; dsll32; dsra32` -- no spelling tried gives it).

Hand-written in round 6b: `medius/rt_comm.c` (33 of 33, new), `medius/rt_comm_platform2.c` (17 of 18,
new), `medius/rt_crypt.c` (22 of 24, new), `medius/rt_mt.c` (6 of 6, new), `medius/rt_time_platform.c`
(6 of 6, new), `medius/rt_mutex.c` (+2), `medius/rt_msg_client.c` (+8 of 9 internals) -- 94
functions byte-exact, 77 new to the map's byte-exact rows (17 supersede matching drafts); 27 rows
named (`s1:order` 18, `s1:calls` 9), one name withdrawn (0x62CEF8, `s1~`); 12,976 bytes.

**Round 6c: the queues, the scheduler's small functions, 117 names.**

- **SOCOM 1's call sites name the callees** (`build/scratch/gcc/callvotes.py`): every map row
  carrying a demo name has its callee sequence aligned with the demo function's; where ours calls
  a placeholder and the demo a named function, that is a vote. 75 names whose votes agree with the
  object order (`_tosControl` 87 sites, `_tosGetProfileU32` 54, `qEnQueueOne` 21, `ConvErr` 21,
  `tosBaseBlockConstruct` 25 ...), and where asr_align.txt was a row off (CalcAccuracyCtxInfo/
  CreateMenuMasks, GenderDataCompute/GenderSelection, qFreeUnusedElemements) the demo's order
  checked by hand; the queue and tos objects' other 20 rows by order, each with its callers
  checked against the demo's. The HUD lane's five gsapi leads (from CReco against SOCOM 1's reco)
  hold against SOCOM 1's gsapi object -- `gsapi_Init`, `gsapi_LanguageLoadBuffer`,
  `gsapi_EngineOpen`, `gsapi_EngineStop` and the error translator, `gsapi_ErrorGetString` -- and
  17 more of the object's rows and extaudio's three follow by callee sequence and position.
- **gsapi_ErrorGetString is SOCOM 1's, moved**: the translator at the end of the context object
  (the extaudio object between it and gsapi's) is SOCOM 1's gsapi_ErrorGetString with a bit-7
  test. Its jump back to the top is a tail-recursive `return gsapi_ErrorGetString(0x80CC007F)`,
  which 2.96 turns into the jump -- written last, after the table walk, as the target lays it out.
- **Inlining, three ways**: SOCOM 1's static `qFreeElem` is a `static __inline__` (no copy left);
  `qDeQueueOne` is a public `__inline__`, inlined into qCheckDeQueueOne's loop as SOCOM 1's is, its
  own copy still byte-exact; so is `_tosErrorLog` (inlined into tosBaseBlockConstruct, its log
  record still built on the stack).
- **ScanSoft's shapes, each measured**: the element copied out before the walk moves on (`p = e;
  e = e->next; qFreeElem(q, p)` -- the inline's parameter copy); a counter of its own per loop
  (sharing one moves the outer loop's registers or its unrolling); `count -= n` reused as the
  bound; `err |= !ok` (an `if` around `err |= 1` is if-converted to a movz the target lacks); a
  free list tested before it is read (`if (*list) { e = *list; ...}`); a table indexed twice,
  not through a local pointer (the address sum's registers swap); the positive test around a body
  (`if (ok) { ...; return q; } return 0;`); a parameter copied to a local before a default is
  taken (`size = elemsize` or the profile's).
- **Open, diagnosed in place**: qEnQueue (byte-exact as written without `-funroll-loops`; 2.96
  unrolls its loop by two where the target's stays rolled, while the object's other counted
  loops are unrolled -- something in the vendor's body stops the unroller: two empty `asm
  volatile` statements do, 4 words off), tosBaseBlockConstruct (16 words: the inlined logger's
  code computed inside the `error == 0` test in the target).

Hand-written in round 6c: `asr/queue.c` (16 of 17, new: SOCOM 1's queue object whole),
`asr/tos.c` (+5: the base block's three input-queue commands, `_tosGetProfileU32`,
`_tosGetProfileFloat`; 7 of 8 written), `asr/context.c` (+1, gsapi_ErrorGetString) -- 22
functions new to the map's byte-exact rows (6,752 bytes); 117 rows named (95 `s1:calls`/
`s1:order` from call sites and object order, 22 by the gsapi object's callees and order).

**Round 7b: DME's client table, the Medius core, Dme.c's own functions.**

- **The option setters were a row off, and rt_comm_set_default_options is 0x62CB68.** rt_comm_create
  calls 0x62CB68 where SOCOM 1's calls rt_comm_set_default_options (SOCOM 1's body, the grown 12-byte
  options cleared first); the `s1:exact` 0x634958 is SOCOM 1's six instructions met again in
  rt_msg_client -- its UDP setter's internal, `rt_msg_client_iudp_set_default_options` (the wrapper
  0x6335E8 is called by DME's UDP enabler, then rt_msg_client_enable_udp, as SOCOM 1's NetEnableUDP
  does), and the names beside it moved too: 0x6348E0 is `iset_default_connect_options` (SOCOM 1's
  constants in a block grown 64 -> 76), 0x633558 `set_default_connect_options` (DME's
  NetConnect_ClientServerTCP twin, 0x617788, calls it). No static of the same name: no `NAME@UNIT`.
- **`clients.c` is SOCOM 1's first DME object** (0x6126A0-0x6132B8, between audio.c and codec.c, no
  `$Header`: the name keeps the link order), NetInitializeClients .. NetClientLeaves in order plus
  the session master's hand-over (0x613158). **DME's API names go to Dme.c's locked wrappers**, as
  NetGetObject and NetSendMessage already did: zNetGetGameClient calls NetGetClient's wrapper
  (0x622A20) where SOCOM 1's calls NetGetClient, so the four wrapped bodies (0x6126E8, 0x612750,
  0x612AB0, 0x612D80) keep placeholders and the nine unwrapped take SOCOM 1's names.
- **A call inside a loop statement is never a sibling call** (2.96's calls.c: stmt_loop_nest_empty):
  NetFreeClient's DmeFree is a jal only as `do { DmeFree(...); } while (0)` -- a statement macro's
  expansion -- where SOCOM 1's tail-called it and NetFreeClientPointers still does; void, int,
  nested, guarded and local-pointer spellings all give `j`. The first explanation for a jal where
  2.96 could tail-call, after a narrower callee return type.
- **An enum is its own alias class**: 0x613158 loads the new master's index (`int *`) across the
  store to the client's state -- only with the state an enum (`dmelocal.h`'s DmeClientState); as an
  int the store orders the load.
- **Identical if/else arms leave their test behind**: MediusLocalConnectCallback keeps the status
  test's `li v1,1; lw v0,4(a0)` after cross-jumping merged two arms that both store the index (no
  dead-code pass runs after jump2) -- byte-exact only so.
- **The Medius core is SOCOM 1's object, slimmed**: the queued request is a `switch` with JoinGame's
  case first (the compare tree tests 18, then 44; the bodies follow source order) and `memcpy` from a
  `void *` (the tail word moved with `lwl/lwr`: a struct assignment's would be `lw/sw`); the
  keep-alive keeps its timestamp but no longer sends the client-state request; a new update
  (0x647AC0) drops the connection on a server flag. MediusGetCallbackStatusString is the enum's
  switch in declaration order, 40 cases over 1005 values: the balanced tree byte-exact as written
  (its strings read off the target by running the tree over every value, `emu_switch.py`).
- **SCEA's shapes, measured**: the success path nested where the target puts an error code in a
  branch's delay slot and falls into the epilogue (NetGetClient's list test, NetSendMyClientUpdate's
  state test after a plain guard on the client, NetClose's last steps -- which also gives the
  target's s0/s1); stores in argument order (Dme.c's registration tables, 1 of 6) and an index taken
  into a local before the caller's pointer is written (one load of the count for every table).

Hand-written in round 7b: `medius/clients.c` (15 of 15, new), `medius/mediusapi.c` (+26: the core,
24, MediusGetLastNetUpdateError and MediusGetCallbackStatusString), `medius/dme.c` (+8:
NetSetDefaultInitializeParams, NetInitialize, NetClose, 0x623DA8, the four registration functions),
`medius/rt_msg_client.c` (+2, the option setters) -- 51 functions byte-exact, 34 rows new (7,596
bytes), 17 drafts superseded.
51 rows named (30 `s1:order` in the Medius and client objects, 16 `s1:calls`/`s1:order` DME API
wrappers and callees, the 5 option setters, each with its evidence), 12 data rows (MyClientIsJoined,
ActiveNetObjectCount, KeepAliveInterval/Timestamp, CurrentSessionKey, LastDMENetError, four callback
slots, geMediusProcessingState, geQueuedRequestType: SOCOM 1's names at the same accesses).

**Round 7c: the SPI object and the resource object, whole.**

- **The asrspi object is SOCOM 1's at 0x33A768, two functions moved**: `_engineFree` follows
  `_engineCalloc` (SOCOM 1's object starts with it) and `asrspi_GainSet` (0x4CBC68) follows
  `asrspi_Stop`. Every entry point checks the handle (null 0x80040083, magic 11111 0x80040001),
  takes the engine's busy flag under its critical section (0x80040003 when held), checks the
  states the call is allowed in through `g_StateError`, calls casr and turns its error through
  `ConvErr`. Two exits leave the flag set, as the target does (SetParamList's 0x80040013,
  Start's 0x80040017).
- **The state check is an inline per count of states, not a macro**: the target loads every
  allowed state into its own register before the first test (`li v0,1; li a0,2; li a1,5; li a2,4;
  beq ...`) -- integrate.c copies an inline's constant arguments into pseudos at the call -- and
  a macro's `s == 1 || s == 2` folds into a range test (`addiu; sltiu`). The test is
  `if (s != a && s != b) return g_StateError[s]; return 0;` (the `==` form lays the zero first).
- **A static the code reads must be defined above its first use**: `lw rt,g_StateError(rs)` is a
  GNU as macro; for a symbol not yet defined in the file as puts it in a variant frag (it might
  be small data) and will not swap it into the branch's delay slot -- the target's
  `b; lw` came out `lw; b; nop` with the table defined at the end.
- **The converters are inlines with out parameters**: ConvResultKind, ConvSampleFormat and
  TranslateParamId `return` their error from a switch's default and store the value through a
  pointer; ConvAbnorm's and TranslateParamId's variables stay in memory (`move a2,sp; sw v0,0(a2)`),
  ConvResultKind's and ConvSampleFormat's go back to registers. A switch on an unsigned value (an
  enum) tests 0 by equality, which gives the target's 1, 0, 2, 4 chain where an `int` takes `slti`.
- **No sibling call from a function with a stack slot**: 2.96's sibcall pass gives up when
  `frame_offset` is nonzero, so ConvErr, intAcquisition, asrspi_Stop, the session-data and word
  functions `jal ConvErr` where the frameless entry points `j ConvErr` -- and two such calls
  cross-jump into one (intAcquisition).
- **Shapes, each measured**: intAcquisition's chunk loop is a `goto` loop (`for`/`while` rotate
  the exit test or invert the last-chunk test, 20 words; returning 0 from its top takes v1 for the
  flag); ActivateCtx's state result has its own variable (the ConvErr result takes another
  register); the rsrc lookup keeps `err = 0` to a single `return err` (its s0) with the other
  exits returning directly; Start's four stores in one order of 24 (`tools/ccperm.py`).
- **The tables**: data.txt names the asrspi object's .rodata from SOCOM 1's symtab
  (`UswVersionCode`, `g_ApiVersion`, `_sizeof_asrspi_engine`, `g_StateError`, the same values) and
  the rsrc object's (`g_NullResource` beside `g_lhNullHandle`). `g_APIRecogCallBacks` (eight
  callbacks) is referenced through `.rodata` and so checked by content, each callback at the same
  distance from asrspi_OpenEngine as in the target: OpenEngine reads R until every function
  between is the right size. The integrator's table check of FillLanguageVirtualTable and
  FillContextV2VirtualTable: every slot names the map's function at the target's word (79 of 79,
  21 of 21), resolved through the object's own function at each offset; by distance
  _LangVirtualTable shifts 73 slots only because four OPEN functions of language.c are the wrong
  size.
- **Open, diagnosed in place**: asrspi_ActivateWords (0x600 against 0x5A0: m2c's shape, its
  exits share more code in the target), and with it asrspi_OpenEngine's table reference.

Hand-written in round 7c: `asr/asrspi.c` (41 of 43, new: SOCOM 1's asrspi object whole),
`asr/rsrc.c` (3 of 3, new: rsrc_Open, rsrc_Close, InitRsrcFunction, SOCOM 1's FreeRsrc inlined)
-- 44 functions byte-exact, 36 new to the map's byte-exact rows (14,768 bytes), eight superseding
matching drafts (ASRSPICBAGC, ASRSPICBASKCURRENTGAIN, asrspi_GetLngGenders, asrspi_GetVersion,
asrspi_GetUswVersion, _engineMalloc, _engineCalloc, _engineFree); 9 rows named (asrspi_GainSet
`s1:calls`, the other eight `s1:order`), 5 data rows (`s1:data`).

**Round 8c: the engine's processing blocks.** The front end is a chain of blocks the scheduler
(tos.c) runs, one demo object each, every one the same four functions: a constructor
(`tosBaseBlockConstruct` with the block's size, a sibling call under a vestigial frame), an init
(reads the block's profile, sizes its queue elements, allocates), a process (one input frame, or
a null frame while flushing) and a control (a command: 255 destroys). `asr/tosblk.h` is the
blocks' view of the scheduler: the record's head (scheduler, id, queue tables, the three
functions, data from +40).

- **The constructor's slots name the statics.** The demo marks Process/Control/Init LOCAL, so
  they are `static` with prototypes at the top; the row the constructor passes in
  `tosBaseBlockConstruct`'s process (or init) slot takes the demo's name of that slot, the
  object's order agreeing (`s1:order`). The align map was a row off at some object edges
  (ProcessGenderFilter precedes the constructor in the demo; the subsampler object starts with
  Subsampler_Process): follow the demo's object list.
- **A `static __inline__` defined after one caller and before another**: GCC 2.96 inlines only a
  body it has already seen, so the VQ's FreeCodeBooks is a real call in LoadCodeBooks and inlined
  into FreeVq1500, its argument (`v->b.tos`) read before the test -- one source, both shapes.
- **One division for a test and a quotient**: InitUndersampler takes `n / factor` into a local
  before the input size is stored; the test's `div` then gives the quotient and only its zero
  trap stays in the success path (a store between reloads the factor -- `-fno-strict-aliasing`
  -- and divides again). Two `unsigned short` rates divide unsigned (2.96 shortens it), an int
  signed. A division's zero trap can vanish: the spellings that divided before the test kept no
  trap after the first (mips.md's div_trap prints nothing when it is anti-dependent on an
  earlier trap in its block), the target's only one sits in the success path.
- **ScanSoft's shapes, measured**: a store and its pointer's increment as two statements
  (`*h->pos = bin; h->pos++;`: one expression keeps `&h->pos` in a register); a pointer of its
  own per loop (one shared pointer moves every register); `for (sum = *p++; p < group; p++)` for
  a group sum; a loop bound taken into a local (`end = s->end`) where the unroller would copy the
  register; an index initialised at its declaration (zeroed in the prologue); the found-node
  branch first (`if (p != 0)`) and case bodies in source order; store orders by
  `tools/ccperm.py` (the delay line's reset, the median's reset -- the same five stores in two
  functions).
- **Literals**: the EE's largest float is 0x7FFFFFFF (no infinity), `6.80564694e+38f` through
  2.96's parser (the VQ's FLT_MAX); `0.001` comes out an ulp low (0x3F50624DD2F1A9FB) and `0.1f`
  0x3DCCCCCC, both the target's.
- **Open, diagnosed in place**: ProcessStationarity (50 words, the right size: the first block's
  registers), GetLabel (0x46C, the VQ's two-stage search: both outer loops unrolled by two, each
  distance loop's first step outside, pointers strength-reduced -- the break after the sum is the
  target's). Of language.c's twelve only `_cdbGetCodeBookSize` was retried, its sum factored as
  the target computes it (`(dim + nbr_in_sec_search) * first_size`, three spellings): still 32s.

Hand-written in round 8c: `asr/delayblk.c` (4 of 4), `asr/lookahd.c` (4 of 4), `asr/gendfilt.c`
(4 of 4), `asr/undersmp.c` (4 of 4), `asr/stacker.c` (4 of 4), `asr/median.c` (5 of 5),
`asr/station.c` (3 of 4), `asr/vq.c` (8 of 9), `asr/slhisto.c` (8 of 8, with SOCOM II's three
session functions) -- 44 functions byte-exact, 26 new to the map's byte-exact rows (6,336 bytes),
18 superseding matching drafts; 11 rows named (ProcessGenderFilter, ProcessUndersampler,
InitUndersampler, ProcessStacker, ProcessMedian, ProcessStationarity, GetLabel,
IdleVq1500Process, ControlVq1500Process `s1:order`; FreeVq1500 `s1:calls`; SlidingHisto_Free by
body).

**Round 8b: RT's leftovers, the crypto objects, three Medius names settled.**

- **The online lane's three questions, against SOCOM 1's symtab** (the demo names every
  `PacketType*` global and every `MediusUniqueCallback*ResponseHandler`, each handler returning the
  size of its response): 0x6492D8 keeps `MediusBuddyGetPermission` -- SOCOM 1's function of that name
  sends `PacketTypeAddToBuddyListConfirmationRequest` and its slot's handler is
  `...AddToBuddyListConfirmationResponseHandler` (64 bytes, as here; the request grew 44 -> 48);
  0x649060 keeps `MediusSendChatMessage` -- SOCOM 1's shape exactly and the library's only 112-byte
  request (a direct message is a chat message with its type and target); 0x649400 was not
  MediusGetPolicy -- 38 bytes (no policy word) and a 72-byte response (the GetBuddyList response's
  size) -- it is the game's GetIgnoreList's request, `MediusGetIgnoreList`, with
  `MediusAddToIgnoreList`/`MediusRemoveFromIgnoreList` (0x6494A0/0x649540, by their callers' status
  strings), and **MediusGetPolicy is 0x6495E0** (44 bytes, a 288-byte response as SOCOM 1's). Two
  callback slots moved with it (data.txt 0x656240, 0x65624C). A request's size and its handler's
  size pair a new request with SOCOM 1's more surely than the slot order did.
- **SOCOM II's RT keeps SOCOM 1's objects under new layouts**: the RC cipher's counters are words
  ahead of the table (SOCOM 1's were bytes after it), LargeInt's tests walk up from word 0 and
  IsGreater and its siblings are gone, KM_Initialize has a third key pair, rt_time_calc_diff gives a
  zero span where SOCOM 1 failed, and rt_time_platform_get_time_string's RTC read is its own
  function with milliseconds kept (gCurTimeDate, gbTimeDateSet: SOCOM 1's names).
- **The title's glue object is built without scheduling**: KM_GetSoftwareID and
  KM_GetSoftwareKeyPair (SOCOM 1's object 0x420FC0, the keys its function-local statics
  `pubKey.0`/`priKey.1`) are byte-exact at `-O1 -G0` (or -O2 with both scheduling passes off) and
  miss at -O2 by 2 and 67 words on every ee-gcc toolchain -- a per-object flag again, the first -O1.
- **Shapes, each measured**: a conditional expression where the target if-converts (`nTimeout =
  b == 1 ? 0 : 10000` -- the `if` form keeps the constant 1 across the call in a saved register and
  branches on it: rt_comm_platform2_create); `while (!bFound)` around a search whose found block
  jumps back to the flag test (the interface search); a date copied out field by field in date
  order, year first (0x63D8E8: a struct assignment is ldl/ldr, the target lw/sw); RC4's j summed on
  `S[i]` and the swap through its own byte temporary (the target's `andi` on the swapped byte);
  counters cleared i, j, n (ccperm, one of six); a salt indexed off its own stack slot; LargeInt's
  IsEqual through plain word pointers (index-first address sums), lintGetBit's mask first.
- **Open, diagnosed in place**: RCEncrypt (15 words) and RCDecrypt (36), registers only;
  LargeInt's two carry functions 0x626D28 (17) and 0x626D78 (28s), the loop's block layout.

Hand-written in round 8b: `medius/rt_time_platform.c` (+3, 9 of 9), `medius/rt_time.c` (+1, 7 of 7),
`medius/rt_crypt.c` (+1, KM_Initialize: 23 of 25), `medius/rt_comm_platform2.c` (+2, 19 of 20),
`medius/rc.c` (2 of 4, new), `medius/largeint.c` (8 of 10, new), `medius/kmsoftware.c` (2 of 2, new)
-- 19 functions byte-exact, 16 new to the map's byte-exact rows (4,364 bytes), 3 superseding
matching drafts (IsEven, lintSetBit, lintClearBit); 17 rows named or corrected (rt_time_platform_
get_time_string `s1:calls`, KM_Initialize, the four RC and seven LargeInt functions `s1:order`,
MediusGetPolicy moved, the three ignore-list requests by call), 4 data rows.

**Round 9c: the casr object, whole** (resumed after a power cut: the first agent's casr.c batch,
11 of 13, re-scored and committed, then the round carried on).

- **casr.c and cb.c were one object**: SOCOM 1's casr object (0x348248) is SOCOM II's
  0x4D8398-0x4DF46C -- this round's casr.c the head, rounds 1 and 2b's cb.c the tail, interleaved
  in the image (casrGetSNR between casrGetSignalLevel and casrGetState) and sharing statics. cb.c
  is merged into casr.c, image order. The object is built with `-fno-optimize-sibling-calls`
  (casrStartUserWordTraining returns its call through a jal, 18s with sibling calls, and the
  object holds no `j`); cb.c's functions score the same with it. The four rows after it
  (0x4DF470-0x4E0D00) have no partner in SOCOM 1's object.
- **SOCOM 1's statics, inlined**: casrFreeResult is a `static __inline__` (in cbTOSRecognised
  and cbDoSecondPassIfEnabled, no copy left); casrGet/SetParamNoClearErrorLog are inlined into
  the second passes (last_error restored in the branch's slot, both paths) yet keep their copies
  further on -- GNU `extern __inline__` bodies at the top, the real functions at their place (GNU C
  allows the redefinition; 2.96 inlines only a body it has seen). cbDoSecondPassIfEnabled moved up
  to after cbIsoWordRecognised; SOCOM 1's CalcWordConfidence is no row here.
- **Two calls, not one with a chosen argument**: `_tosControl(c->event, fmt != 0 ? 106 : 107, ...)`
  is if-converted into a movn (71s); the target's two calls in an if/else share one tail after
  cross-jumping (casrInitSampleFormat; casrStart's 8/6/1; the second passes' context constructors,
  where one result variable copies v0 aside: 82 words).
- **The host's callbacks take (ms, info)**: SOCOM 1's and ours leave a2/a3 as they came -- a frame
  count in a3 is a register choice, not an argument; cbTSAfterStartDetected adds its frame offset
  before setting its flag (5 words the other way).
- **Literals**: 2.96's parser lands an ulp below even an exact decimal for CalcSentenceConfidence's
  two slope constants (76.9f gives the target's 0x4299CCCC); they are spelled just above the
  target's floats. SOCOM II calls expf where SOCOM 1 calls exp.
- **Shapes, each measured**: the allocation first and `else p = 0` (casrGetActiveWords: the null
  first leaves the slot unannulled); a bitmap byte taken into a local per outer iteration (the store
  through p reloads it per bit); the next write position computed inside the reallocation block
  before its stores, PRE supplying the copy's size (casrReadLabelInQueue -- the demo orders its loads
  the same); a switch's case bodies in the target's order (casrGetParam: 1, 2, 3, 4, 5, 15, 18, 16,
  6, 7, 8, 9, 10, 17, 11, 13, 12, 14); a local pointer for a struct copy's destination whose address
  is used again (casrGetSessionData: with `s->lng` 2.96 folds the data pointer to s+152, 87s); one
  float variable for x and the result (CalcSentenceConfidence: two move every register, 17 words).
- **The user-word DP blocks** (dpgenuw.c, dpscoreuw.c, extevdp.c; SOCOM 1's objects 0x352770,
  0x3539F0, 0x354AD0): `x += c ? (cache[i] = f()) : cache[i]` gives the target's add in both arms
  with x loaded before the test (a local and if/else does not); `if (c == 0) r.f = 100; else r.f =
  x;` gives the constant arm falling through (fold turns the `?:` round); small inline helpers whose
  argument is loaded before their test (FreeCells/FreeBacktrace's early `lw tos`; CheckAlloc's
  `unsigned char` return, the `andi` at the join); a loop bound through the block is reloaded and
  the loop not unrolled -- the target's is a local; `e->chunks[n++] = chunk; e->nchunks = n;` (n + 1
  does not increment in place); `bt = node` after the node's stores (before them CSE stores through
  bt). ccsweep's and ccperm's probes qualify a `NAME@UNIT` static by the file's stem: a probe must be
  `<dir>/<unit>.c` (helpers in build/scratch/gcc/h1/). The call-site vote's ControlDpScoreUw at
  0x4E5668 is ControlDpGenUw (its constructor's control slot); func_004E5538 (ControlDpGenUw's
  command 23, split off) and func_004E6BC0 (uncalled) are SOCOM II's own.
- **Open, diagnosed in place**: UpdateExevStatus (1 word: `bnez` there, `bnezl` here),
  casrGetSessionData (38 words, the right size: which stores go through s+8), cbDoSecondPassIfEnabled
  (9: the result's register), casrInitSegmentation (44s: the row pointer), and the earlier opens
  (casrGetLastError/Warning, casrImportContext, casrDoSecondPass, cbGenderDisable).

Hand-written in round 9c: `asr/casr.c` (44 of 53, new; cb.c's 12 matched functions merged in) --
24 functions new to the map's byte-exact rows (6,960 bytes), 8 superseding matching drafts
(cbTSDetected, cbSpellRecognised, casrSet/GetParamNoClearErrorLog, cbGainRequest, cbAskCurrentGain,
cbTOSAbnormalCond, CopyContextInfo); 19 rows named (casrCreateResult, cbIsoWordRecognised,
cbDoSecondPassIfEnabled `s1:calls`; casrGetSessionData, cbTSDetected, cbTSAfterStartDetected,
cbStartDetected, cbTOSRecognised, cbSpellRecognised, cbNothingRecognised, cbSpellNothingRecognised,
the NoClearErrorLog pair, cbRegisterOutputQueue, cbGainRequest, cbAskCurrentGain, cbTOSAbnormalCond,
Set_EndBackTraceMask_In_StateDict, CopyContextInfo `s1:order`); `asr/dpgenuw.c` (7 of 8; InitViterbi
OPEN, the wrong size), `asr/dpscoreuw.c` (11 of 11), `asr/extevdp.c` (5 of 5), all new -- 17 rows new
(6,616 bytes), 6 superseding matching drafts, 9 rows named (`s1:order`, `s1:slot`). DynProgUserWords,
SendResultUserWords and DynProgScoreUserWords are not written.

**Round 9b: the voice library (rt_audio), audio.c and grid.c by hand.** SOCOM 1 has no voice
library (nor codec, grid or md5 objects), so its names are placeholders and the records' fields
ours; the units say what each function does.

- **Where the two objects start.** audio.c begins at 0x60FB50, not 0x60FF00: the eight rows
  before 0x60FF00 address audio.c's one `.data` word (0x651160, just before clients.c's) and call
  audio.c's own functions. grid.c begins at Dme.c's end, 0x6244B0, not at its first data
  reference (0x6250D0): every rt_audio source carries its `$Header` (audio.c, codec.c, grid.c,
  rt_audio.c, each with its headers'), none sits between Dme.c's and grid.c's, the one `.data`
  word between Dme.c's statics and LargeInt.c's is grid.c's random seed (0x6244B0, the C library's
  LCG), and these functions and grid.c's call each other both ways.
- **The records** (`audio.c`, `grid.c`): the session (0xD8: message counters, the codec's
  context and six handles, the groups' ring, the peers' ring, flags, this client's id and group,
  the socket, the connect callback); a peer (a ring node with its state, keep-alive time and
  tries, a 32-byte self-description at 0x20 -- two addresses and ports, group, id, flags --
  and its decoders at 0x840-0x85C); a group (0x84: id, a 64-byte name, a queue in the list object
  at 0x629DA0, a member table of 0x14-byte members that doubles when the group has no maximum).
- **A row that carried the next function's alignment nop**: 0x60FFF8 was 0x18 where its code is
  0x14 -- the only one of the Medius range's 702 odd-length functions cut so (resized, the map's
  fix; no scorer change).
- **Types decide where an address is computed**: audio.c calls the clock (0x62C070) undeclared --
  implicitly `int` -- and a timer stored from it of another type makes 2.96 convert and hoist the
  timer's `lui/addiu` into a saved register before the call; the same type gives the target's
  `lui; sw %lo` after it (a static or an extern alike).
- **Shapes, each measured**: one combined `||` guard where separate guards hoist the -1 above the
  prologue's saves (the connect); `for (;;)` with a `break` where a do-while rotates an `n > 0`
  test to the loop's top (the codec read loop); a result variable apart from a call's length,
  zeroed at the top, which keeps two callback calls from cross-jumping (the decode); `n == f()`
  for the target's `xor` operand order; an elapsed time taken into a local before the state test
  that uses it twice; the connect state an enum (its own alias class, so its store schedules
  past the callback pointer's load); a group create that returns its group (void keeps the
  result in v1 and stores it branch-likely); an index zeroed at its declaration lands in the
  prologue, assigned after the guards where the target zeroes it there; a member's peer taken
  into a local (one load for the copy and the call).
- **Open, diagnosed in place**: 0x611180 (13: the peer and its state swap s0/s1, every spelling
  tried), 0x612518 (14: the socket's event switch -- the table from 0 and sibling calls right,
  the error code's copy to a1 hoisted to the entry), 0x60FC90 (52s: the frame length in v0, the
  target's v1), 0x626140 (9: the capacity test's second branch sense).

Hand-written in round 9b: `medius/audio.c` (57 of 60, new: 0x60FB50-0x6126A0 but 0x6110D8, which
stands as a matched draft, 0x6115C0 and 0x610C20), `medius/grid.c` (28 of 29, new: 0x6244B0-0x624C60
and 0x626110-0x626B10 but 0x626240, 0x626658, 0x6264C0; 0x624830, 0x624DC0 and grid.c's three
large functions 0x6250D0-0x625690 not written) -- 85 functions byte-exact, 51 new to the map's
byte-exact rows (9,600 bytes), 34 superseding matching drafts; 2 data rows (nSeed, nTimerStart,
ours); one row resized (0x60FFF8). No tool changed.

**Round 10c: the gsapi object whole, and the small block functions.**

- **A redefined `extern __inline__` is output at the end of its file, and inlined in its new form
  after it.** SOCOM 1's gsapi object ends with a real CheckEngineHandle; SOCOM II's callers inline
  it, and the map's func_004C8C38 (after StopListening, called by nothing) is SOCOM 1's function --
  so gsapi.h carries the GNU `extern __inline__` body and gsapi.c the real one, which 2.96 defers
  to the end of the object as an inline's out-of-line copy (a probe: defined anywhere, it comes out
  last). Defined in the source just before gsapi_NotifySetEnergyInterval, its own early-return body
  is what the two functions after it inline -- the result zeroed again after the base test, the
  round-3b diagnosis of NotifySetEnergyInterval and EngineSetUserData, open since: both byte-exact
  now, with every earlier caller keeping the extern inline's single-result form. And the two
  SOCOM II functions after CheckEngineHandle (0x4C8CB8, 0x4C8D10) are therefore another object:
  `gsapi2.c` (ours), inlining the header's form.
- **The gsapi object's data are SOCOM 1's globals** (`data.txt`, `s1:data`): AsrSpiSignalCallBacks
  and AsrSpiRecogCallBacks (the asrspi_cb object's .data: the engine start sets or clears the
  energy and SNR callbacks), MAXRECOGNIZERS and DEFAULTENERGYINTERVAL (4 and 500, the shorts
  func_004C8CB8 resets), the .rodata's szGSAPIVersion ("Game Speech API V2.02.01 for Sony
  PlayStation2", SOCOM 1's "GAME ASDK ... 1.00.02") and TranslateParamTable (18 asrspi ids, 255
  none: ActivateDefaultParams and the translator read it).
- **The vendor reads its default parameters through a null pointer**: gsapi_LanguageLoadBuffer
  copies the engines' defaults from a block it never sets (`lw v1,8(zero)`, values from 12).
- **ScanSoft's shapes, measured**: a guard on the stored status, then `return status = f()`
  (RestartEngine; the `>= 0` test with one return reloads); the start's two calls with the error
  copied in the `< 0` arm (`err = status`) so the join's own test is not threaded; nested tests
  with one `return err` where a call must not be a sibling call (EngineSetImmediateContext:
  guard clauses tail-call ContextActivate); a `switch (state) { case 0: break; case 1: ... }`
  where `== 0` returned early lets CSE drop the target's `move v0,zero` (ProcessAudio); a block's
  control `break`s to one shared `return 1`, its error path returning its own (the FFT, window
  controls); stores by `tools/ccperm.py` (the engine open's five, ResetGender's four, the
  window's three).
- **Names, each a call site or a slot**: Subsampler_Reset (SOCOM 1's Voicing_Reset tail-calls it
  at the same point, the body its own with the fields 4 bytes lower), InitVoicing,
  Voicing_MaintainNoiseEner, Voicing_AddSignal (one caller each, as SOCOM 1's), ProcessFFTMod,
  InitFFTMod, ProcessWindow, InitWindow, CombinerProcess (their constructors' slots, SOCOM 1's
  LOCALs in the same objects).
- **Open, diagnosed in place**: gsapi_EngineStop (2 words longer: GCSE's PRE keeps the stop's
  e->audio load for the external-audio branch -- byte-exact only under
  `-fno-expensive-optimizations`, which costs two other functions), gsapi_LanguageLoadBuffer (37,
  the right size: the copy loop's source pointer starts at 12 where here 12 is the loads'
  displacement), func_004C8D10 (11: id and gGSAPI's address swap s1/s2). Tried without result:
  casrGetLastError's movz through the handle inline, six caller shapes (C4, the nested `?:`,
  comes out the right size).

Hand-written in round 10c: `asr/gsapi.c` (47 of 49: 22 new, the two round-3b opens closed, 3 over
their drafts -- gsapi_GetNumContextsLoaded, func_004C8890, TranslateGsapiParamId2AsrSpiParamId),
`asr/gsapi2.c` (1 of 2, new: func_004C8CB8 moved from gsapi.c), `asr/voicing.c` (2 of 2 written,
new: Voicing_Reset, Voicing_Free over its draft), `asr/gender.c` (2: ResetGender, ConstructGender
over its draft), `asr/fftmod.c` (2, new), `asr/window.c` (2: ControlWindow, ConstructWindow over its
draft), `asr/vad.c` (ConstructVad), `asr/mtx.c` (+1, imtxInit) -- 31 rows new to the map's
byte-exact rows (7,500 bytes), 6 superseding matching drafts; `src/lib/asr` 362 of 396 (325 of 358
before). 10 rows named, 6 data rows. No tool changed (ccperm's `-I` gap was worked round with an
absolute include; master's round 10a fixed it).

**Round 10b: the objects after the RT library, the voice library's lists, rt_msg_client whole.**

- **What 0x63DC90-0x645710 holds, by the `.rodata` each row reads**: rt_udp.c from 0x63DC90 (its
  version at 0x656040, its jump tables at 0x665840/0x665880), **rt_upnp.c from 0x642330** (its
  first function returns `gpszVersion`, "rt_upnp version: 1.01.0007", the string just before its
  `$Header`; '%u.%u.%u.%u' after it), a three-function table object at 0x642968, SHA1.c to 0x643168
  (0x643140 reads its `.rodata`), **StreamMedia.c 0x643168-0x6451E8** (DME's media streams: its two
  switch tables follow its `$Header`; it alone reads 0x6560C8/0x6560CA and the session table at
  0x676480), then a fragment-buffer object at 0x6451F0 (64 slots of 512 bytes) before the Medius
  client. They are not Medius API objects: every call they make goes to rt_comm, rt_time, DmeLocal.c
  or the voice library's API (rt_audio.c's forwarders, 0x62BDF0-0x62C060).
- **md5.c is RFC 1321 with a byte count** (0x629B38: MD5Init's four constants): SCEA counts input
  in bytes and makes the bits only in MD5Final, and the transform and Encode are md5_platform.c's,
  called at 0x6641D0/0x664F80 where no map row is (a placeholder resolves by its address). The
  update's index must be zeroed right after the first copy -- it then lives across the transforms
  in s4 and the last copy is shared; zeroed after the loop, 2.96 keeps it in a3 and splits the copy.
  The names are placeholders: RFC 1321's are a prediction, not this library's evidence.
- **SCEA's statics carry SOCOM 1's names**: every SOCOM 1 RT object keeps a `char *gpszVersion`, a
  byte `gbInitialized` and a short `gnInitCount` (its symtab's LOCAL OBJECTs, rt_msg_client's at
  0x43D7C8-0x43D7CE); StreamMedia.c's .data is that byte and short. A GNU `.data` static is checked
  by content only, so the units use SCEA's names and data.txt holds them as `NAME@unit`
  (`gbInitialized@streammedia`); a `.bss` static must carry its data.txt name (scdiff.bss_local).
- **SCEA's shapes, measured**: a boolean query as one `&&` after its guards (a guard per test is
  if-converted to `xor; sltiu` -- StreamMedia's 0x644060 and 0x644390, the matched draft's shape);
  a `void *` user-data field, its own alias class, whose load schedules above the `int` stores into
  the callback's record (three callbacks); an argument read back from the field just stored
  (`pUpnp->nServerPort`: CSE gives the stored register and 2.96 zero-extends it again, `andi`);
  a 4-byte address and a 2-byte port unpacked with `memcpy` into locals that stay in the frame;
  a chained `a->x = a->y = a` (no reload between the two stores); a ring walk as `if (p) do {...}
  while (p);` with the head in a local; a call's error returned from inside the positive test
  (`if (ok) { r = f(); if (r) return r; } return 0;` keeps the final `move v0,zero`); a test whose
  `return 41` cross-jumps backwards into an earlier failure block gets GNU as's five short-loop nops.
- **round 6b's OPEN rt_msg_client_is_valid_connection_id closed**: with is_valid_connection (its
  callee) defined above it and the connection state an enum (compared unsigned: `sltiu`), the
  beqz/beqzl choice comes out as the target's.
- **Open, diagnosed in place**: StreamMedia.c's 0x6437E0 (21 default stores, 17 orders tried),
  0x644610 and 0x644EA0 (5 words each: a store order, a register pair), 0x644D20 (the 1 kept in v0
  for a test and three returns; nine shapes), 0x6448A0 and 0x6450B0 (not iterated).

Hand-written in round 10b: `medius/streammedia.c` (33 of 39, new), `medius/rt_upnp.c` (21 of 21,
new), `medius/md5.c` (3 of 3, new), `medius/peer.c` (21 of 21, new: the voice library's peer rings
and member lists, 0x629DA0-0x62A3B0), `medius/rt_msg_client.c` (+17 and the OPEN closed: 53 of 53)
-- 96 functions byte-exact, 76 new to the map's byte-exact rows (11,216 bytes), 20 superseding
matching drafts; 4 data rows (gbInitialized@streammedia, gnInitCount@streammedia,
gpszVersion@rt_upnp by SCEA's convention; gapStreamSessions, ours). No row renamed; no tool changed.

**Round 11b: rt_udp.c, rt_p2p.c and the RT objects between them.**

- **What 0x6311E8-0x63CF20 holds**: after rt_crypt.c's ciphers an interrupt-handler object
  (0x6311E8, drafts) and **rt_list.c** (ours, 0x6313D8-0x6322C8: a message list in a ring buffer --
  an 8-byte head per message, an empty message always ending the chain -- the lists rt_udp keeps per
  endpoint and per connection); after rt_mutex_platform **rt_p2p.c** (0x6395B8-0x63C700, its
  `$Header`; a peer table over one rt_udp endpoint of 256 connections and the key manager, id -1)
  and **rt_table.c** (ours, 0x63C700: a rows-by-columns word table, .data 0x656000-0x656010); then
  rt_time. **rt_udp.c** is SCEA's reliable UDP ("rt_udp version: 01.02.0048", a .data array): an
  endpoint of 0x54 bytes on its own channel or one of eight shared ones (`gapUdp`, by the slot at
  0x48), 0x90-byte connections with a TCP-like RTT estimator (srtt, rttvar, an RTO of at most 500
  ms), 12-byte headers (flags, slot, length, id, sequence) and a 4-byte trailer. SOCOM 1's
  rt_rel_udp keeps none of these bodies: every name is a placeholder.
- **A statement macro again stops sibling calls**: rt_p2p's three relay handlers are jals only as
  `do { f(...); } while (0)` (2.96 tail-calls a void function's last call to an int function
  otherwise) -- round 7b's NetFreeClient finding, met a second time.
- **A narrow return type, a third time**: rt_list's end-marker writer returns `unsigned char`, so
  the clear returns its result through a jal; and KM_Cleanup (rt_crypt.c, 0x630508) is void, as its
  sibling KM_Destroy: declared int, rt_p2p's destroy takes v1 for the record it reloads after it.
- **Shapes, each measured**: a two-way choice as `if (n == 0) {...; return 0;} if (n == 1) {...;
  return 0;} return 1;` (an if/else lets the two bodies' tails cross-jump; a switch moves every
  block); a result kept in a saved register to one `return result` across a call (0x641230); a value
  read once into a local before three class tests (KM_Cleanup's id: the struct field reloads in
  every test otherwise); a row index for a fixed table (`aSlots[i]`, i < 32), which 2.96 turns into
  the target's count-down loop, where a walking pointer moves the registers; the RTO as `n = srtt +
  MAX(rttvar, 1); if (n > 500) n = 500;`; header stores by `tools/ccperm.py` (five functions).
- **An unrotated search loop**: rt_p2p's peer searches test the bound at the top, recompute
  `i * 40` and hoist only the count -- `for (i = 0; ; i++) { if (i >= n) break; ... }` gives that
  loop where a plain `for` is rotated and strength-reduced (the guards before it still differ).
- **Open, diagnosed in place**: rt_udp's create (the allocation laid out after the option copy) and
  raw send, rt_p2p's searches and create, rt_list's init, rt_table's five accessors (the result
  constant held in v0 from the entry), four functions 2-3 words off on argument scheduling.

Hand-written in round 11b: `medius/rt_udp.c` (36 of 44, new), `medius/rt_p2p.c` (22 of 29, new),
`medius/rt_list.c` (15 of 16, new), `medius/rt_table.c` (1 of 6, new), `medius/rt_crypt.c` (+1,
KM_Cleanup: 24 of 26) -- 75 functions byte-exact, 58 new to the map's byte-exact rows (7,860
bytes), 17 superseding matching drafts; 4 data rows (gapUdp, gnTableCols, gnTableRows, gnTableBufSize, ours).
No row renamed; no tool changed.

**Round 11c: the scheduler, the subsampler, round 10c's half-written blocks.**

- **A public `__inline__` is output at its object's end, in declaration order.** The subsampler's
  and the voicing record's Init each hold Reset's body inlined (2.96 inlines only a body it has
  seen), so Reset is a public `__inline__` defined above Init; 2.96 defers an inlinable
  function's own copy to the object's end. With Free an `__inline__` too, declared after Reset,
  the copies come out Reset, Free -- the image's order in both objects; SOCOM 1's subsampler
  (Free, then Reset) is Reset alone deferred.
- **GNU extern inlines keep their copies in place** (tos.c): the input-queue commands are inlined
  into tosBaseBlockControl and the id lookups into tosCallControlFunc, from `extern __inline__`
  bodies above them; the real functions stay where the image has them. ConstructBlock, earlier in
  the object, calls tosGetQueuePtr out of line -- the bodies are not visible from the object's top.
- **An inlined helper's returns give one result tested once**: tosInit's failures each set 1 in
  one register and jump to a single `andi; beqz` (the success path sets 0 and falls into it) -- a
  `static __inline__` with a `return 1` per failure and a final `return 0`, called in a `&&`.
- **ScanSoft's shapes, measured**: the positive test with `else ret = 0` (tosBaseBlockControl:
  `if (!enabled) { ret = 0; break; }` makes reorg steal the slot from the target, the target fills
  from the fall-through); a list walk testing `*l` in its condition and taking `c = *l` after
  (_tosControl: a `c = *l` in the condition is threaded into the error call); a pointer and a
  `char` index of their own per table walk, the id compared first (`id == r[0]`: DestructQueue);
  the host's 24-byte callback block copied through a pointer (a by-value struct is copied to the
  frame first); `*s->pos++ = x << s->shift1` as one expression (`&s->pos` kept in a register);
  `*a = *b = x * c` (b stored first); `acc += *s->mid; *out++ = acc >> s->shift`; the error block
  of `if (coef == 0)` entered by `goto` from the earlier allocation tests (Subsampler_Init keeps it
  mid-function, the success path after it).
- **Doubles from 2.96's parser land an ulp low too**: three of the subsampler's 23 taps are spelled
  an ulp above the target's bits (`build/scratch/gcc/dconst.py ADDR COUNT` compiles candidate
  spellings and reads them back); a local `double h[23] = {...}` is the block move from `.rodata`
  at Init's top; one function's two uses of `1e-15` are two pool entries, as the target's.
- **window.c's object is built with `-ffast-math`**: InitWindow's Hamming loop is unrolled by two
  and the peeled first step calls `cosf(0.0f)` -- the `0 * 2pi / (n - 1)` folded away, which 2.96
  does only without IEEE semantics (8 words longer without); nothing else in the object changes.
- **The four rows after the casr object (0x4DF470-0x4E0D00) are the training object's head**:
  three are called only by TrainSmoothCounts (0x4E0D00), the fourth only by DoUserWordTraining,
  and 0x4E0050 ends in `j heap_Free` -- the casr object, built with -fno-optimize-sibling-calls,
  holds no `j` (TrainFree, the same object's, ends in one too).
- **One word, three functions**: tosCallControlFunc, cbDoUserWordTraining and round 9c's
  UpdateExevStatus each miss by a branch-likely: reorg takes a `move v0,...` from the branch's
  target into its slot and annuls it here (`bnezl`) where the target's does not (`bnez`), the
  fall-through in each a call that sets v0. Every source spelling tried leaves it.
- **Open, diagnosed in place**: Subsampler_CheckImpulseResponse (49 words, 0x16C against 0x174:
  the centre tap's test and the odd taps' loop share one tail, cross-jumped the other way round
  here), tosCallControlFunc and cbDoUserWordTraining (above; cbDoUserWordTraining was 10 words:
  the flag test and the last call as one `&&` keep the final branch 2.96 folded into `sltu`).

Hand-written in round 11c: `asr/subsmp.c` (4 of 5, new: the trigger's subsampler), `asr/tos.c` (+7:
DestructQueue, DestructArchitecture, _tosControl, tosBaseBlockControl, tosInit, tosBaseBlockDestruct,
DisabledProcess over its draft; 14 of 16), `asr/voicing.c` (+3: InitVoicing,
Voicing_MaintainNoiseEner, Voicing_AddSignal; 5 of 5), `asr/fftmod.c` (+1, InitFFTMod; 3 of 3
written), `asr/window.c` (+2, ProcessWindow and InitWindow; 4 of 4 written), `asr/nbestdp.c`
(2, new: ExtendBacktraceStack, and ScoreCompare@nbestdp over its draft), `asr/isowdp.c`
(ScoreCompare@isowdp, new), `asr/train.c` (TrainFree, new: the training object's span and head),
`asr/acne.c` (4 of 5, new: Reset, NormalizeAndEnQ, InitAcne, ConstructAcne over its draft; its two
21-float 11 kHz tables statics checked by content; SOCOM 1's FreeResourcesAcne a `static __inline__`
inlined into ControlAcne, which is OPEN: 48 words, the right size, the session pointer and the
array size swap s0/s1 over five shapes) -- 22 rows new to the map's byte-exact rows (11,280 bytes),
three matched drafts superseded (DisabledProcess, ScoreCompare@nbestdp, ConstructAcne);
`src/lib/asr` 387 of 424. gender.c waits on a shape: InitGender inlines ResetGender while
ControlGender calls it twice, so ResetGender is a `static __inline__` between them whose copy
exists only once ControlGender (0x58C) is written. Named: Subsampler_Process,
Subsampler_CheckImpulseResponse, ScoreCompare@isowdp, ScoreCompare@nbestdp, ConstructAcne
(`s1:order`), SetAndControlQSizes (`s1:calls`: SOCOM 1's tosInit calls it at the same point),
smtDestruct (`s1:calls`: SOCOM 1's FreeResourcesAcne tail-calls it where ControlAcne's inlined copy
calls 0x4F80A8). No tool changed.

**Round 12c: gender.c's half, the DCT lift, the pitch window, the bnez/bnezl word settled as a
toolchain question.**

- **Three block objects are built with `-ffast-math`** -- window.c's (round 11c), and now the DCT
  lift's and the gender classifier's. ProcessDCTLift's DCT stores `c * p` over the 0 it has just
  stored in each row's first preconditioned step (`0 + x` folded, which 2.96 does only without IEEE
  semantics: 78 words longer without); GenderDataCompute inlines `sqrtf` as a bare `sqrt.s` (no
  errno test and library call) and folds each distance loop's first step (`d = t * t`, 362 words
  off without). The objects' other functions compile the same either way, and so do the pitch DP
  and pitch window objects: a unit's directive says it, not the library's.
- **A static `__inline__` between two callers, as round 11c predicted**: ControlGender (above it)
  calls ResetGender twice, InitGender (below it) holds its body, and its own copy, deferred, comes
  out after ConstructGender -- all three byte-exact, ResetGender's match kept.
- **An inlined function's `unsigned char` return is zero-extended once, at the join**: command 1
  of ControlGender (the codebook's dimensions, then `_tosErrorLog(g, 102)` or the language kept) is
  a `static __inline__` returning the scheduler's error; the target's one `andi` at the join of its
  two returns, which the error arm's `b` copies into its slot, comes only so (an `unsigned char`
  local assigned in both arms needs none; `(unsigned char)err == 0` on an int also matches, the
  inline is the vendor's shape). The `andi` on GetCompStart's result is an int accessor stored
  through `(unsigned char)` into a short.
- **A variable incremented in place keeps its register**: GenderSelection's 1-based gender is
  `best++` on both paths -- the target's `addiu s2,s2,1` in three branch-likely slots; `best + 1`
  takes another register (21-40 words) -- and the fixed path's three stores in one of two orders
  of six (`tools/ccperm.py`). Its byte table is SOCOM 1's function-local `ControlID_Disable.27`.
- **Two variables where one value has two uses**: InitDCTLift copies n into a second saved
  register for the coefficient matrix's allocation (`nrows`; one variable, 35 words); integers
  reach its double arithmetic through `float` (`(float)i * PI`: the unsigned halving trick's
  SFmode target, then `fptodp`), and PI is spelled twice -- two pool entries, as SOCOM 1's.
- **Shapes, each measured**: a block-local `t` for a squared difference (a function-level one takes
  another FPR for `t * t`, 65 words); a count read through its field (`g->f80++`, then
  `1.0f / g->f80`: a local keeps it in another register); an index table read as `*idx` with
  `idx++` at the iteration's end (`idx[k]` and `*idx++` move the count-down loop's registers); a
  dictionary entry as a pointer local (`e = dict + *p; *e |= 2`: indexing gives the address sum's
  operands the other way round, 62 words); a list walked as `for (; bt->next != 0; bt =
  bt->next)` (a `next` variable copies the test's load differently); a score's nonzero arm first.
  2.96 leaves Set_EndBackTraceMask_In_StateDict's setting loop rolled where SOCOM 1's compiler
  unrolled both. The pitch window's Hamming doubles are plain `0.54`, `0.46`, `2 * 3.14159...`
  (2.96 lands each an ulp low, on the target's bits); ProcessPitchDP copies the block header
  through a second pointer (`TOSBLOCK *b = &p->b`: the target's `move s6,s0`); an early exit that
  must reach a shared `return ret` is a `goto` (a second `return ret` is constant-propagated to
  `return 0`: FlushPitchDP); PostDP's index offset parenthesised, `k + (lookahead - overlap)`.
- **THE bnez/bnezl WORD IS A TOOLCHAIN QUESTION, NOT A FLAG** (REFUTED by round 13c below: the callee's definition order decides it -- this paragraph's sweep stands as a measurement): every optimisation switch this
  cc1 (2.96-ee-001003-1, the only 2.96 build in this tree or EQOA's) knows -- thread-jumps,
  cse-follow-jumps/skip-blocks, gcse(-lm/-sm), rerun-cse-after-loop, rerun-loop-opt,
  expensive-optimizations, regmove, strength-reduce, peephole(2), caller-saves, force-mem/addr,
  defer-pop, schedule-insns(2), sched-interblock/spec/spec-load, optimize-register-move,
  reorder-blocks, branch-count-reg, function-cse, move-all-movables, reduce-all-givs,
  delayed-branch, align-jumps/labels, optimize-comparisons (`build/scratch/gcc-r12c/bnez_sweep.sh`) --
  toggled one at a time on tosCallControlFunc, cbDoUserWordTraining and UpdateExevStatus leaves
  each at its one word or worse. The target's reorg puts the branch target's `move v0,...` in an
  unannulled slot because the fall-through's call kills v0; this build's resource scan does not
  see it. With round 4a's runtime compiler (a 2.96 whose integer code is these toolchains' but
  not this build), the second sign of a later SCE 2.96 release; the three stay OPEN until one is
  found.
- **Data** (`s1:data`): g_SpeechMeanInit11kHz (0x659230) and g_NoiseFloors11kHz (0x659288), SOCOM
  1's LOCALs, 21 floats each, SOCOM 1's to the ulp (12 an ulp low: 2.96's parser on the same
  decimals) after the same 200.0f and 50; Smoother1600Info11kHz (0x65A3E0), SOCOM 1's GLOBAL,
  three {table, 21, 10} entries, its tables at the same distances (acne.c spells it by name).
- **Open, diagnosed in place**: GenderDataCompute (93 words, the right size: the frame and
  codeword walkers take each other's register in both stages -- declaration order and scope, the
  setup's 24 orders, while/for and new second-stage pointers tried), and the pitch DP's six
  (Actual_DP and InitPitchDP: one more value kept live in the target; BackTracking, ProcessPitchDP,
  InitJumpFactors: registers only; ControlPitchDP: ten independent stores, past ccperm's reach).

Hand-written in round 12c: `asr/gender.c` (+3: ControlGender, InitGender, GenderSelection; 5 of 6
written, ProcessGender not), `asr/dctlift.c` (4 of 4, new), `asr/pitchwin.c` (5 of 5, new),
`asr/pitchdp.c` (3 of 9, new), `asr/casr.c` (+1, Set_EndBackTraceMask_In_StateDict; 45 of 54),
`asr/dpgenuw.c` (+1, SendResultUserWords; 8 of 9) -- 17 functions byte-exact, 13 rows new to the
map's byte-exact rows (7,868 bytes), four superseding matched drafts (ConstructDCTLift,
ControlDCTLift, ConstructPitchWindow, ConstructPitchDP). 8 rows named (`s1:order`, each with the
constructor's slot or its one caller: InitDCTLift, ConstructDCTLift, ProcessDCTLift, PostDP,
ControlPitchDP, InitJumpFactors, InitPitchDP, InitPitchWindow), 3 data rows. No tool changed.

**Round 12b: rt_msg_client's head and SOCOM 1's bodies, the ciphers, five small objects.**

- **rt_msg_client.c starts at 0x6325B8, not 0x632620** (the brief's boundary, checked): 0x6325B8 keeps
  its state in the object's own .data (0x655630, after gbMutexInitialized and the 0x87654321 word;
  rt_memory.c's callbacks end at 0x65560C) and is rt_udp.c's first function again (0x63DC90) with
  key-manager id 0 for rt_udp's -1; 0x632610 returns the .data head 0x655610, the pointer to
  "rt_msg_client version: 1.08.0204" -- SOCOM 1's object opens with rt_msg_client_get_version_string_ptr,
  the same three words (`s1:order`).
- **SOCOM 1's same-named bodies are the quickest wins**: the startup, shutdown and ishutdown are SOCOM
  1's sizes exactly and matched as read; iupdate_all and create_send_buf_space grew a little. Shapes,
  measured: a positive test around the shutdown's mutex teardown and the space's refill (`if (result
  == 0 && count == 0) {...} return result;` -- guard clauses there invert the branch); the table's
  pointer cleared before its count (store order); a `for` alone where an outer `if (n != 0)` adds a
  second entry test; an `int` result for an `unsigned char` callee (no `andi` after it).
- **The ciphers are SOCOM 1's KM_Encrypt and KM_Decrypt grown** (`s1:order`, the object's last two)
  by a fourth key: the title's software key pair (kmsoftware.c, tag 0x80000000), each with a twin over
  a key master and key chain already found (0x630938, 0x630DF0 -- the encrypt twin reads the chain's
  id through a null chain in its last test, as the target does) and an RSA re-check (0x631008: LargeInt's
  0x629034 over copies). KM_Encrypt keeps its header in s5 from the entry to one `return nHeader` (the
  branches nested; early returns lose the register); KM_Decrypt's cases are laid out 0xE0000000,
  0x80000000, 0x60000000, 0x20000000, 0, then 0x40000000 with the default (bodies in source order), its
  hash check inside `if (nLength != 0 && nResult == 0)`; the id test is `nID == -1 || (nID &
  0x80000000) == 0` (`>= 0` folds the pair into one slti).
- **Five objects without a $Header, named by the link order** (ours): `rt_intr.c` (0x6311E8, between
  rt_crypt.c and rt_list.c: an INTC handler added and removed for causes 0-10, the kernel's counts
  summed -- round 11b's three matched drafts as a unit), `rt_thread.c` (0x63CD18, between rt_table.c and
  rt_time.c: three ReferThreadStatus queries; a struct copied out inside the positive test, the
  guard-clause spelling schedules the return value first), `rt_window.c` (0x642968: round 10b's
  three-function table object is five functions -- a 0x4010-byte table of 64 slots of 256 bytes, and
  its byte count, a leaf whose walk must `break` to one `return`: returning from inside the loop gives
  a second `jr ra`), `rt_frag.c` (0x6451F0, the 64-slot fragment buffer before the Medius client: its
  create and destroy are rt_window.c's over another layout) and `sha1.c` (SHA1.c's init: the initial
  context by memcpy, GCC's inline block move with its run-time alignment test, and the byte-order
  probe tail-called).
- **An error block the compiler keeps apart**: in rt_udp's send 0x63E928, its raw send 0x63EBA0 and
  rt_msg_client_iget_metrics, a `return` after a test is a block of its own in the target -- the test
  branches over a fall-through `b` to the epilogue -- where 2.96 merges it into a branch to the
  epilogue (round 11b's "raw send" diagnosis). An empty `__asm__ volatile ("" ::: "memory")` in the
  block reproduces it: iget_metrics comes to 1 word, the raw send to 10 (from 33s), and 0x63E928 is
  byte-exact with a second one after its walk's next-message call (the target reloads the pointer it
  just stored before the loop's test). No flag does either (-fno-gcse, -fno-cse-follow-jumps,
  -fno-rerun-cse-after-loop, -fno-expensive-optimizations, -fno-force-mem, -fno-thread-jumps), nor a
  volatile pointer (127s), `if (0) f();`, `(void)0`, a label or an empty loop in the block. rt_udp.c
  carries it as `RT_UDP_BARRIER()`, a stand-in marked as one: SCEA's construct there is not known.
- **Open, diagnosed in place**: rt_udp's 0x63ECC8 (93s: the receive -- the flag test reads the
  header's first word, `lw; andi 6`, which `(*(unsigned int *)&header & 6) == 6` gives; the loop's
  message pointer lives in a0 where the target's is v0), rt_crypt's 0x630DF0 (21: the key master and
  data pointer swap s1/s2; setting the hash after the null test gives the registers and moves its
  store, 9), rt_msg_client_iget_metrics (37s, above), the UDP poll under SOCOM 1's
  rt_msg_client_iudp_disable_heartbeat (9: SOCOM II's body, not the name's; the flag's reload takes v1
  for v0), rt_frag's store 0x645290 (13: the 29-bit sequence in a0 where the target reuses a3).

Hand-written in round 12b: `medius/rt_msg_client.c` (+7: func_006325B8, rt_msg_client_get_version_string_ptr
over its draft, startup, shutdown, ishutdown, iupdate_all, create_send_buf_space; 60 of 62),
`medius/rt_crypt.c` (+4: KM_Encrypt, 0x630938, KM_Decrypt, 0x631008; 28 of 31), `medius/rt_udp.c` (+1,
0x63E928; 37 of 46), `medius/rt_intr.c` (3 of 3, new, over its drafts), `medius/rt_thread.c` (3 of 3,
new), `medius/rt_window.c` (4 of 4, new), `medius/rt_frag.c` (2 of 3, new), `medius/sha1.c` (2 of 2,
new, 0x643140 over its draft) -- 26 functions byte-exact, 21 rows new to the map's byte-exact rows
(5,464 bytes), 5 superseding matching drafts; 3 rows named (rt_msg_client_get_version_string_ptr,
KM_Encrypt, KM_Decrypt: `s1:order`). No tool changed.

**Round 13c: ProcessGender, casrOpen, the user-word callbacks; the bnez/bnezl word is the callee's
definition.**

- **THE bnez/bnezl WORD IS NOT A TOOLCHAIN QUESTION** (rounds 11c and 12c held it open as one): a
  conditional branch whose fall-through calls a function the object has ALREADY COMPILED keeps the
  branch target's `move v0,...` in a plain slot (`bnez`, the target's); with the callee only
  declared, reorg annuls it (`bnezl`). cbDoUserWordTraining (open since round 4b) matched with no
  change to itself once ExitUserWordTraining was written above it, and tosCallControlFunc once
  ConstructBlock was (a stand-in body above it gives byte-exact too). So a one-word `bnezl` against
  a target `bnez` before a call means: write the callee earlier in the same unit. UpdateExevStatus
  stays open: its fall-through holds no call (the callees defined in the unit do not move it).
- **A switch on an unsigned value tests by equality** (round 7c's rule, met twice more):
  ProcessGender's state and cbGenderDisable's mode (open since round 1: "tests 1, 0, 2 with no
  range check") give the target's 1, 0, 2 chain unsigned and a `slti 2` range test as ints.
  ProcessGender's state is an enum (filling, chosen, draining).
- **An inline's local initialiser is evaluated before its first test** -- ProcessGender's frame-in
  helper reads the write position as a declaration initialiser (the target's load in the input
  test's slot) in two of its three inlined copies; the chosen state tests the input at the call
  as well, so the inline's own test folds away and the load follows it. SOCOM 1's ProcessGender has
  the same three placements, so it is the source's shape. The level is read into a local before
  the fixed-gender test (the first `in[0]` load then serves it when the copy loop is skipped), the
  ring's new position is a conditional expression (a copy in a0, where an `if` updates p in place).
- **casrOpen's six tables**: tosInit takes SOCOM 1's architecture GLOBALs -- IniFile, InitialProcs,
  QueueWriters, QueueReaders, ControlTable (data.txt `s1:data`: the architecture object is SOCOM
  1's laid out alike, 0x1FA7A0 higher, ControlTable's STD_ table of 21 state lists included) -- and
  a local TOSCB initialised with the four scheduler callbacks (cbStateHandler,
  cbInitArchitectureQueues, cbRegisterInputQueue, cbRegisterOutputQueue: the anonymous .rodata
  block the ldl/ldr copy reads, checked by content). The host's eight callbacks are copied whole
  (a struct: casr.h's CASRCBTABLE), the log head and the callback info pair are one-element arrays
  of their records (every `c->event`, `c->info` use stays a pointer).
- **The training utterances are records**: three at +980 of {frames, the segmentation's length, the
  segmentation} (round 9c's seginfo was their second word); the user-word callbacks index them
  through a local index (cbUserWordResultAvailable: a record pointer or a reloaded index move every
  register); the accepted callbacks store the count, the flag, then the state (ccperm: one order
  fits both). 2.96 splits each such address into the context plus 4 or 12 and 976 -- the target's
  -- from plain field accesses.
- **InitUserWordTraining's setup** (command 15) is the language's user-word phonemes -- their count
  (a local of its own, stored after the second accessor's call), _LangVirtualTable's slot 46
  (_langGetpPhenUserWordTraining) and slot 15 (_langGetUWSilencePhenome), also kept at +1016.
- **ConstructBlock's registration helpers are inlines** returning the scheduler's `unsigned char`
  (an `andi` at each join): an output queue takes the block's element size when it has none, an
  input queue's reader is numbered down from the queue's count of readers; the scheduler's own
  record goes through the host's callbacks at the block's control/process slots.
- **Open, diagnosed in place**: SendPartOfLabelsToEngine (36s: SOCOM 1's order; the target's rotated
  loop keeps its error blocks in place and reloads the queue pointer in the preheader -- seven loop
  and guard shapes tried), ConstructBlock (149: the output count's narrowing, two saved registers),
  and the earlier opens (GenderDataCompute, pitchdp.c's six, UpdateExevStatus, the language file's
  offset chains, the casr object's large functions).

Hand-written in round 13c: `asr/gender.c` (+1, ProcessGender; 6 of 7), `asr/casr.c` (+4: casrOpen,
cbGenderDisable closed, cbTrain and cbDTMFDetected over their drafts; 49 of 57), `asr/userword.c` (+8:
InitUserWordTraining, cbUtteranceNotAccepted, cbUtteranceAccepted, cbLastUtteranceAccepted,
cbUserWordResultAvailable, ExitUserWordTraining, cbDoUserWordTraining closed; SendPartOfLabelsToEngine
OPEN; 11 of 12), `asr/tos.c` (+1: tosCallControlFunc closed; ConstructBlock OPEN; 15 of 17) -- 13
functions byte-exact, 11 rows new to the map's byte-exact rows (3,780 bytes), two superseding
matched drafts (cbTrain, cbDTMFDetected); `src/lib/asr` 417 of 460 (405 of 449 before). 4 rows named (`s1:order`,
each body checked: cbUtteranceNotAccepted, cbUtteranceAccepted, cbLastUtteranceAccepted,
cbScoreUserWordResultAvailable), 5 data rows (`s1:data`). No tool changed.

**Round 13b: DmeLocal.c, the densest object, by SOCOM 1's bodies.** SOCOM 1's Dme.c object (0x418B58,
115 functions) is SOCOM II's DmeLocal.c in the same order, most bodies grown: a row at SOCOM 1's
position is read beside the demo's function (NetGetBufferStatus, NetSetSendAggregationInterval,
NetIpStringToBinary, FilterTargetClientBitmask, SendPacket, the NetSendMessage* family, NetConnect,
RemoteClientEvent, FindFreeLocalObject, NetAddToDataStream ...), and the rows after SOCOM 1's object
end (0x61C450 on) are the peer layer's callbacks, which the transport's startup (0x616C98) passes to
rt_p2p by address -- six rows at once.

- **SOCOM 1's two-path functions share one final `return 0`**: the server's connection goes through
  the peer layer (0xFFFF or 0) and the others through rt_msg_client, written `if (p2p) { if (f()) return
  37; } else { ...; } return DME_OK;` -- a join, so the zero is materialised (`move v0,zero`) where guard
  clauses let 2.96 if-convert or reuse a known zero. The same for a body inside a positive test
  (`if (!bStarted) {...} return DME_OK;`, the startup) and for a callback's `!=` test around its send.
- **The known-zero v0 is cse-follow-jumps'**: func_00614758 (round 4b's OPEN) is byte-exact under
  `-fno-cse-follow-jumps` alone, and func_00615B48 comes from 65s to 1 word -- 2.96's CSE follows the
  jump past `return 37` with v0 still the call's result and deletes the target's copy into the return
  register, so reorg cannot put the constant in the branch's slot. Not the vendor's flag: it reallocates
  func_00614168 (s0/s1) and costs six of the unit. The target's CSE follows jumps but keeps a copy into
  v0 -- a toolchain difference, with the runtime's DFmode slots (round 12c's bnez/bnezl word, once counted
  here too, is the callee's definition order -- round 13c).
- **Shapes, each measured**: SOCOM 1's comparison operands as written (`anDelay[j] > anDelay[j + 1]`
  loads a[j] first; `nPort == entry.nPort`); a clamp as a guard (`if (n < 1) n = 1;` -- the ternary
  if-converts the other way); a sub-record's address taken once before its tests (NetConnect's kinds);
  an index sum into its own variable before the status test (FindFreeLocalObject); the ternary port
  default as an if/else on `== 0`; a clear loop with its own counter (a temporary, not the outer index);
  a range guard as two (`< 0`, then `>= 256` -- one `||` folds to an unsigned test); the bandwidth copy
  takes the byte counters first (register order says it); a void callback where the target's early
  exits leave v0 as the test's register.
- **A record's types reach other functions**: adding a `char` array to DmeObject (the named object's
  name, 0x0C) or retyping the stream slot's `char` array as bytes drops func_0061C7A8's reload; the
  records stay as the code proves them and the name is spelled by its offset.
- **Open, diagnosed in place**: func_0061D530 and func_00617560 (the link-lost reports: the target loads
  the world's field and the host word ahead of all five stores; 120 orders, an array and a struct),
  SendPacket (SOCOM 1's bytes: `li -1; xor` where 2.96 folds `nor`, the 0xFFFF test if-converted the
  other way; nine shapes), func_00615B48 (the known-zero v0, above), func_00616290 (`p + 12 != 0` folded
  to `p != -12`), func_0061A118 (two sends cross-jump here), func_0061B538 (the byte message's frame
  slot), func_0061C9E8 (the ping table's address forms, first shape).

Hand-written in round 13b: `medius/dmelocal.c` (+37 written, 71 of 84): 29 functions byte-exact, all new
to the map's byte-exact rows (6,588 bytes; `src/lib/medius` 781 -> 810 of 878) -- func_006142A8,
func_00614380 (the delay medians), func_006144A0, func_00614590, NetGetBandwidthInfo, func_006148E0,
func_00614F58, func_00615390, func_00615558, func_00615660, func_006157E0, func_006163A0,
func_00616748, func_00616C10, func_00616C98, func_00616DA0, func_00616ED0, func_00617690,
func_006186B0, func_0061ABD8, func_0061B650, func_0061CBB8, func_0061CEF0, func_0061CFF0,
func_0061D650, func_0061D7A8, func_0061D898, func_0061D948, func_0061D9F8. No row renamed (the bodies
keep placeholders: DME's API names belong to Dme.c's locked wrappers, round 7b, and 0x6209B0/0x620A88
-- NetGetBufferStatus's and NetSetSendAggregationInterval's wrappers by their bodies -- are left for a
call-site check); no tool changed.

## Open, and next

The work runs in lanes (see "Working in parallel"), each an address range with its own units;
their open functions are diagnosed in their sources (`grep -rn "OPEN" src/ --include=*.cpp`).
What is open across lanes, and what comes next:

- **SaveNodeStates' size cut** (zanim_set.cpp, the one level-4 match level 5 loses, with its
  three vector-constructor copies): `new CZAnimSetSave` is inlined one level in the target, while
  CreateAnchor inlines CCompassAnchor's constructor (a class vector inside) and AddAnimEx `new
  CZAnimExVector` whole -- no height does both at any level, so it is the inline_max_size cut:
  CZAnimSetSave() expands to 179 units here and must pass 256, CCompassAnchor() (197) must not.
  The original's vector constructors were fatter than include/vector's (~26 units more in each
  pointer vector's); the shape that adds them unseen. CPnt3D's copy constructor (0x387F90,
  vis_mesh.cpp's) was matched only through grid_main.cpp's level-4 mis-inlining of GetMidPoint;
  at level 5 no unit emits it (a draft can take it).
- **Level 6, or 8?** Two SOCOM II structures need a level more than 5: ~__list_deleter written
  `erase(begin(), end())` (OVL2's ~__list_imp copy 0x510080 and ent_main's tail call begin() and
  end() out of line) inlines whole in ~C2D only at 6, and SOCOM 1's push_back chain through the
  base's push_back (ai_events.cpp's two copies, 0x50F810/0x50FE80, whose map names are swapped)
  keeps CreateInstance(model) auto-inlined only at 6 (and costs the stack slots of Add2D,
  CZHudMissionCams::Init and GetLoadedLibRefs at any level). Measured at 6 with both: ~C2D,
  CreateInstance x2, Release, SetIncludeInTargetList and end()'s copy match, but every chain
  fitted to 5 needs one level more -- the deleter's swap (the capacity pair as compressed_pair
  over compressed_pair_imp), TrimVector's pieces, fts_state.h's string constructor, the erase's
  node allocator (end_.first() through the pair), SelectNext/PreviousObjective's callees (their
  self-inlining starts at 6) -- 27 functions to refit (build/scratch/lvl/w1_l6.txt). The
  original's MSL is deeper than these headers everywhere SOCOM 1's DWARF shows it (compressed
  pairs in the pod, the list's size and end node, alloc() accessors), so a level of 6 or more with
  the real depths is likely; 5 is what these headers fit. A third witness (the online lane, round
  10): `__sinit_zNetMediusUniverse.cpp` 0x4022D0 matches at `-inline level=6` and not at 5 once
  theAcctDB is modelled as the member at offset 0 of an outer CZUniversInformationServer (the
  destructor it registers, 0x300570, is not CAcctDB's vtable's) -- open at 5 (0xC4 vs 0x144).
  MEASURED OVER THE WHOLE TREE (2026-11-03, master 914fce13, `SC_FLAGS` with level=N, everything
  else as now): level 5 gives 9,295 byte-exact, 6 gives 9,246 (56 lost, 5 gained), 7 9,240, 8 and 9
  9,241 (64 lost, 14 gained). The five that every level from 6 gains are non-recursive:
  ClearGroups 0x3C2A10 (the weapons' round 12: the group's whole destructor inlined at the delete,
  six levels here), WipeoutTextureAssetCharacters, `__sinit_zNetMediusUniverse.cpp`,
  `__sinit_zui_hangul.cpp` and vector<SKBCOMPLETE>'s resize; from 8 on, zdb::CNode::GetBBox
  (0x262550, the self-recursive inline) matches too -- at 9 as well, so it bounds the level from
  below only, while CSealUnit::GetTop's copy (the SEALs' round 14: seven, eight and nine self-copies
  at depths 7, 8 and 9) asks for exactly 8 (seal_ai.cpp carries `#pragma inline_depth(8)`). What the
  higher levels lose is what headers too shallow predict: vector constructors, clears and deleter
  swaps, a list iterator's operators and FreeVector inlined away ("gone": 37 at 8), destructors that
  now inline one level more (24), and a handful of the game's small constructors (CGameState(const
  char *), CSaferStr(), C2DPoly's copy, CJamoStr()) -- so level 8 with the original's deeper MSL
  stays the likely answer, and moving to it is a header refit (the MSL lane), not a flag flip. The
  lists: build/scratch/lvl/summary_2026-11-03.txt. THE REFIT, TRIED (the MSL headers' round 12, "Tells
  from completing the MSL headers, round 12"): SOCOM 1's compressed pair under both vector capacities,
  DWARF- and image-backed, recovers 15 of level 8's losses at no cost there (6761 against level 5's
  6800 over every hand-written unit) and leaves 39 open; the FreeVector/TrimVector cuts (27 rows) need
  swap levels nothing shows, the list's (about 10) SOCOM 1's whole list shape, and the strings,
  element copies and four destructors (about 15) evidence not found. The flag stays at 5, the headers
  at their level-5 shapes, seal_ai.cpp's pragma with them.
- **MSL pieces not modelled yet**: the deque's copy constructor chain's generation order (its
  structure is in since round 7: include/deque's comment; back() is in, pop_back() 8 bytes short), the tree's erase
  (0x306030), the CSaferStr trees (0x3A2D60-0x3A2F60 and OVL2's string-keyed map, through the
  string's out-of-line compare 0x2DEE30), zrdr_file.cpp's pointer-keyed map (0x32D780)
  (include/tree), the list instances of std::remove_if (0x29FE70, 0x3C6560, 0x352500, 0x50F670,
  0x5F8AF0: find_if's two iterator copies in parameter order -- include/algorithm), the class
  vector's destroy for elements with a virtual
  destructor (C2DPoly's deleter), __vector_imp::resize out of line (CPool), and the pod vector's
  end() reusing begin()'s load (AddPalette, ~CAssetLib, DeleteChildren; the zdb world's round 6
  measured `return data() + size_;`: +4, but CCharacterType::Create one level too tall).
- **zMath** (0x306670-0x30954C, 47 of 54): CMatrix::SetRotate, CQuat::Align, CBits::Resize,
  CPnt3D::Reflect, CBBox::GetIntersect and the two DistToVectors. **zcamera**: CCamera::Update
  (13 instructions, the near plane's VU0 temporaries' stack offsets). **ZAnim**: the big handlers
  (OBJECT_MOTION tick/begin, PARTICLE_SOURCE, CallAnimBegin, InitCommands) and apply_script_frame
  ($f20/$f21), ComputeRunTime, OBJECT_MOTION_FROM_TO's begin, SCROLL_MOTION.
- **Unclaimed ranges** for the next lanes: OVL1 0x280000-0x293000 (CBody, CSequence,
  CZSIScript, CZBodyAnim -- SOCOM 1 names them, no DWARF), 0x322000-0x33F000 (Particle, CPipe,
  zrdr, CSaveModule/CSaveManager), 0x37D000-0x408480 (CBufferIO/CFileIO/CMemCardIO, CZWeapon,
  CZProjectileList, the UI specs, CTurret); OVL2 0x5B8000-0x610000 (CZKit, CRemoteCtrl, the AI
  state family CAiS*, CSealCtrlSquirm). The prebuilt GCC libraries are opened: see "The GCC
  front" for the compilers, the generated tier's GCC drafts and what is next there.
- **Unit boundaries behind pins**: the inlining-mode question is answered (no such mode; "The
  inlining mode, measured") -- of the pinned units' candidate cuts (build/scratch/inlmode/
  split_best.txt) the split lane adopted zsnd_csndsequence.cpp, graph_tree.cpp, int_main.cpp and
  grid_main.cpp and turned down the rest of its list with reasons (same section);
  hud_objectives.cpp (the HUD lane's) is unmeasured. Units with no auto-inlined pair at all
  (con_ps2.cpp, ai_util.cpp) may want a file-wide `#pragma auto_inline off` instead of their pins.
- **`tools/scperm.py`** (EQOA's tree-sitter mutations driven by MW) has not yet closed a
  function: its finds so far trade correctness for score.
- Tell 3.0b50 from 3.0b52 (MAIN's runtime does not: "MAIN's Metrowerks runtime"); port
  eqrefcheck's data-reference audit to absolute addresses. MAIN's runtime is calibrated (2026-11-03):
  next, the pool unit's big functions (`_strdup`/`_free`/`__nw__`/`__dl__`, func_001805A0), the
  exception-handling unit, the MSL filebuf bodies, crt0.s, the four `__sinit` rows at
  0x1D4B00-0x1D4C2F (the runtime units', outside its range).

## Credits and licence scope

- **socom-unzipped** (Scotho, GPL-3.0): the overlay decryption harness this tree runs from
  `reference/` (never copied in), the r0001 Ghidra function table used as a cross-check, and the
  SOCOM 1 name carry. Their research notes saved days.
- **decompme/compilers** and **decomp.me** for the compilers and the wibo invocation;
  **decompals/wibo**.
- **zlib 1.1.4** (Gailly & Adler, zlib licence) is vendored unmodified in `src/zlib/`.
- This tree carries **no game code or data**: the target is produced from your own disc.
