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
- **MW's inline budget, measured** (build/scratch/inl, mwtest.py): a callee whose inline tree is
  at most 4 high inlines whole, a taller one only its own body (its calls stay calls). A
  function's height ignores calls made as arguments, but a call written as an argument of an
  inline call at the top is checked a level deeper (`f1(w2(p))` inlines w2 one level where
  `1 + w2(p)` inlines it whole). A recursive function inlines itself until the budget runs out
  (CNode::SetNodeEx: twice) and the innermost copies call the vector's pieces out of line.
  That fallback is for functions declared `inline`: a function MW takes under `-inline auto`
  (one defined in the unit) is inlined whole or not at all -- CNode::CreateInstance(name)
  inlines CreateInstance(model) only once the pointer list's push_back is short enough to keep
  the whole tree four high.
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
  Still unexplained: ~C2D and ~CHUD's flare list inline four destructor levels plus the deleter's
  iterator temporaries, which every probe says costs a fifth.
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
  that overlay to it (`SymbolTable.address_in`).
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
  iostreams).
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
DNAS targets. Scoring either client will need DNAS.BIN's functions resolved apart from OVL2's, as
`ovl-copy` does for template copies.

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
matches reports as a regression there (0x29FF60, 0x2D02B0): rerun with those drafts' files
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

## Open, and next

The work runs in lanes (see "Working in parallel"), each an address range with its own units;
their open functions are diagnosed in their sources (`grep -rn "OPEN" src/ --include=*.cpp`).
What is open across lanes, and what comes next:

- **The class vector's constructor** (include/vector): CCompass::CreateAnchor and CEntity's
  constructor inline a vector<CPnt3D>/CTargetList member's constructor whole -- three levels --
  while zanim_set.cpp's SaveNodeStates needs `new CZAnimSetSave` (a pointer vector base and three
  vector members) to inline only its implicit constructor, which wants four, and AddAnimEx pins
  the pointer vector's at three. No out-of-class piece explains it yet: every piece of the chain
  is inlined into a unit's own function somewhere, which out-of-class members never are.
- **The list destructor's height** (include/list): ~C2D and ~CHUD's flare list inline four
  destructor levels plus the deleter's iterator temporaries, which every probe says costs a
  fifth; the cut that ~CEntityList and ~CAssetList show is explained (a range erase the unit's own
  code instantiated).
- **MSL pieces not modelled yet**: deque<const void *> (CGame's state stack -- the states lane),
  the class vector's destroy for elements with a virtual destructor (C2DPoly's deleter),
  __vector_imp::resize out of line (CPool), and the pod vector's end() reusing begin()'s load
  (AddPalette, ~CAssetLib, DeleteChildren).
- **zMath** (0x306670-0x30954C, 47 of 54): CMatrix::SetRotate, CQuat::Align, CBits::Resize,
  CPnt3D::Reflect, CBBox::GetIntersect and the two DistToVectors. **zcamera**: CCamera::Update
  (13 instructions, the near plane's VU0 temporaries' stack offsets). **ZAnim**: the big handlers
  (OBJECT_MOTION tick/begin, PARTICLE_SOURCE, CallAnimBegin, InitCommands) and apply_script_frame
  ($f20/$f21), ComputeRunTime, OBJECT_MOTION_FROM_TO's begin, SCROLL_MOTION.
- **Unclaimed ranges** for the next lanes: OVL1 0x280000-0x293000 (CBody, CSequence,
  CZSIScript, CZBodyAnim -- SOCOM 1 names them, no DWARF), 0x322000-0x33F000 (Particle, CPipe,
  zrdr, CSaveModule/CSaveManager), 0x37D000-0x408480 (CBufferIO/CFileIO/CMemCardIO, CZWeapon,
  CZProjectileList, the UI specs, CTurret); OVL2 0x5B8000-0x610000 (CZKit, CRemoteCtrl, the AI
  state family CAiS*, CSealCtrlSquirm). The prebuilt GCC libraries (Sony's libnetb/inet and the
  USB-audio lgAud in OVL1 0x243000-0x258000, the speech recogniser at OVL2's start, the Medius/DME
  network libraries from 0x610000) are not MW code: EQOA's EE-GCC toolchains could match them, a
  separate effort.
- **`tools/scperm.py`** (EQOA's tree-sitter mutations driven by MW) has not yet closed a
  function: its finds so far trade correctness for score.
- Tell 3.0b50 from 3.0b52; port eqrefcheck's data-reference audit to absolute addresses. The
  MSL runtime in MAIN (`_delete_vec` and friends) is MW-built library code -- a second calibration
  set once its source is pinned down.

## Credits and licence scope

- **socom-unzipped** (Scotho, GPL-3.0): the overlay decryption harness this tree runs from
  `reference/` (never copied in), the r0001 Ghidra function table used as a cross-check, and the
  SOCOM 1 name carry. Their research notes saved days.
- **decompme/compilers** and **decomp.me** for the compilers and the wibo invocation;
  **decompals/wibo**.
- **zlib 1.1.4** (Gailly & Adler, zlib licence) is vendored unmodified in `src/zlib/`.
- This tree carries **no game code or data**: the target is produced from your own disc.
