# `decomp/` — matching decomp of SOCOM II (SCUS-97275 r0001)

**3,178 functions byte-exact** (2026-10-24, 4.19 % of the map's bytes): **110 hand-written** --
zlib 1.1.4 in MAIN (the calibration), the boot loader's overlay decode, the engine's zMath
(CQuat, the projector matrices, CPnt3D/CPnt4D) and 37 of ZAnim's command handlers -- and
**3,068 generated**, m2c's drafts that are byte-exact as they stand (`src/auto/`, counted
apart; see "The generated tier"). Ten functions are open, each diagnosed in its source. Canary: `./test_scdecomp.py`.

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
| `tools/s1vtables.py` | virtual functions named slot by slot: SOCOM II's vtables found by their RTTI's class-name string, aligned with the demo's `__vt__` tables; `--holdout` measures it on names the map already has |
| `tools/try.py` | compile one unit with one textual substitution at a time and score one symbol, in a scratch copy of `src/` |
| `tools/scperm.py` | the permuter: EQOA's tree-sitter mutations of one function, MW-compiled and alignment-scored (needs `.venv`) |
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
`bottomup` and `level=4`, which zlib cannot see and SOCOM II's C++ needs.

| flag | without it | evidence |
|---|---|---|
| `-O4,p` | `-O3,p` 18; `-O4,s`, `-O4`, `-O3` 6; `-O2` 0 | the only level at 21/21 |
| `-sdatathreshold 0` | 18 | the target addresses zlib's 6-byte version string with `lui/addiu`; the default (8) puts it in `$gp` small data |
| `-inline auto,deferred` | `auto` 20, `deferred` 18, `on` 17 | `inflateReset` is absent from the image — inlined into both callers and stripped (`auto`); functions emit in **reverse definition order**, exactly as the image holds them (`deferred`) |
| `-inline bottomup` | (zlib alike) | inlining starts at the leaves of the call graph, so a callee arrives with its own callees in it: `CZAnimExVector::Free`'s `delete` inlines CZAnimEx's destructor inside `std::for_each`'s function object, and the recursive `CZAnim::Reset`, Pause and Resume inline themselves twice with their callees in the outer copies -- four functions that matched only with it, nothing lost |
| `-inline level=4` | (zlib alike) | a call whose callee's whole inline tree is at most four high inlines all of it; a taller one inlines only the callee's own body and calls what it calls. `new CZAnimExVector` (a vector of pointers, three constructors deep) inlines whole in `CZAnimSet::AddAnimEx`, while `new CZAnimSetSave` (four high or more under its vector of CPnt3D) inlines its own implicit constructor and calls its four vectors' -- and `delete` does the same in `CZAnimSet::Unload` (CZAnimExVector's four nested `this` tests inline, CZAnimSetSave's vectors' destructors are calls); at level 5 and up (or none) SaveNodeStates, Unload and CZAnimSetSave::Free inline everything and the out-of-line copies they call vanish; level 3 loses ten others (the ZAR key walks, LoadZAR, AddAnimEx) |
| `-enum min` | 13 | `inflate`'s `mode` field is stored with `sb` |
| `-Cpp_exceptions off` | (C++ only) | `CMatrix::SetZero` is `j memset`: with exceptions on, MW keeps a frame around every C++ call, while C tail-calls either way; every C++ match held when it went on |

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
- **Inlining is bottom-up to level 4** (`-inline bottomup,level=4`). At each call site, a callee
  whose whole inline tree is at most four functions high (it counts as one; an empty inline
  function counts too) is inlined entirely; a taller callee is inlined ONE level -- its own
  body, its callees called out of line, where each of those copies inlines by the same rule
  (probe: build/scratch/ctor/chain.cpp, chain2.cpp). So a deep MSL chain shows up as calls to
  tiny out-of-line copies -- the 78 three-word zeroing constructors in the image are mostly
  that -- and the header's levels must be the real ones (include/vector carries the counts it
  was fitted to). A function object's call inlines inside `std::for_each`, and a recursive
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
  vector reserves' copies).
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
- **MW folds `begin == begin + n` into `n == 0` unless the iterator converts**: walks over a
  vector hold a const_iterator made from begin(). A class iterator's `!=` (as `!(==)`)
  materialises when it is the whole loop test and branches inside `&&`.
- **Block order follows the early return**: `if (!archive) {...} else return 0; return 1;` and
  `if (a >= b && !Reserve()) return 0;` each put their return-0 block where the target has it.
- **The same inline written two ways serves different callers**: `CZAnim::GetArgPnt3D` must be
  a ternary (TranslateState, RangeTest), `CZAnimMain::CmdHdr` a ternary for BreakTick,
  GoToEndIf and FOR_CHILDREN -- while IfTick reads the current command the if-assign way.

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
source must pass. 2,335 of 7,007 did on the first pass (61 % of those up to 0x40 bytes, 33 %
overall); with the corrections below, 3,068 do (the last run, over everything up to 0x400: 149
from callee prototypes, 37 as C++). They
live one per file in `src/auto/<segment>/<VA>.c` (two drafts' prototypes can disagree),
are counted apart by `sccheck.py`, and are **not finished source**: placeholder names,
`void *` and `M2C_FIELD` offsets, C rather than the game's C++, and no TU grouping. They are:

- a regression guard for the toolchain: 3,068 functions across both overlays compile on
  every check, so a wrong flag or build fails loudly (one build and one flag set match all
  of them -- the overlays were built with the toolchain zlib settled, throughout);
- a correct starting body for whoever writes the real TU;
- disposable: hand-written source supersedes them (sccheck skips a generated function the
  hand tier defines and says so), and `automatch.py --emit` deletes superseded files.

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

Not yet: a `bool` (MW's C has none -- `daddiu v0,zero,1` against `li`); the C++ drafts could
declare one now.
The differ also stopped refusing the last function of a unit that an 8-aligned GCC object
follows without MW's 16-byte padding (131 drafts had every byte right).

## Open, and next

- **zMath (OVL1 0x306670-0x30954C, 4 units): 47 of 54 functions byte-exact.** Open:
  `CMatrix::SetRotate` (callee-saved registers right, two scratch temporaries and one `madd`
  operand order not), `CQuat::Align` (right size; where MW keeps `&half.y` in an `s` register),
  `CBits::Resize` (the 8x-unrolled clear loop saves one register fewer), `CPnt3D::Reflect`
  (a by-value operator chain MW keeps four copies of), `CBBox::GetIntersect` (above), and the
  two `DistToVector`s (written; far).
- **ZAnim's command handlers (`zanim_cmd_tick.cpp`): 49 of 54 written are byte-exact.** Open:
  `zAnimIfTick`, `zAnimLODTestTick`, `zAnimObjectOpacityFromToTick`, TRUE_COLOR_SCALE and
  UPDATE_SHADOW_MAP_LIGHTING, each diagnosed in its source. Unwritten: SCROLL_MOTION's tick (an
  inlined VU0 transform), the motion handlers and the big ones (Light, ParticleSource, Camera,
  Sound, CallAnimBegin).
- **CZAnimMain (`anim_main.cpp`, `zanim_zar.cpp`), the name tables (`zanim_names.cpp`), CZAnimSet
  (`zanim_set.cpp`) and CZAnim (`zanim_anim.cpp`): all written byte-exact but five.**
  Open: SequenceRunning, GetCmd, GetAnimSetIndex, Open (registers in the inlined NewAnimSet --
  the permuter found nothing in Open's own text), CZAnim::LoadNodeNames (a strength-reduced
  index). Unwritten: the wildcard search, the rest of CZAnim. MSL's vector reserve (pod and class)
  and at() match from include/vector; insert does not yet (the pod one 8 bytes short:
  build/scratch/ins/BEST-pod-insert-0x418.txt -- the target reloads the capacity in the growth
  path, where every form tried keeps it).
- **`CZWeaponList::GetWeapon` (`0x3C4B40`)** -- 2 instructions from exact with an `int`
  parameter, but SOCOM 1's mangled name says `EQUIP_ITEM`, and every enum-typed variant sinks
  the zero-extension into the loop preheader.
- **`tools/scperm.py`** (EQOA's tree-sitter mutations driven by MW, ~195 candidates a second
  on 6 cores) has not yet closed a function: its finds so far trade correctness for score.
- Tell 3.0b50 from 3.0b52; port eqrefcheck's data-reference audit to absolute addresses.
- The MSL runtime in MAIN (`_delete_vec` and friends) is MW-built library code -- a second
  calibration set once its source is pinned down.

## Credits and licence scope

- **socom-unzipped** (Scotho, GPL-3.0): the overlay decryption harness this tree runs from
  `reference/` (never copied in), the r0001 Ghidra function table used as a cross-check, and the
  SOCOM 1 name carry. Their research notes saved days.
- **decompme/compilers** and **decomp.me** for the compilers and the wibo invocation;
  **decompals/wibo**.
- **zlib 1.1.4** (Gailly & Adler, zlib licence) is vendored unmodified in `src/zlib/`.
- This tree carries **no game code or data**: the target is produced from your own disc.
