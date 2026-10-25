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
matches reports as a regression there (0x32C5F0, ~CRdrArchiveList's copy, and 0x510080, OVL2's
__list_imp destructor that seal_aistate.cpp's missing copy now reports "elsewhere" -- 0x29FF60 and
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
- **Level 6?** Two SOCOM II structures need a level more than 5: ~__list_deleter written
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
  the real depths is likely; 5 is what these headers fit.
- **MSL pieces not modelled yet**: the deque's copy constructor (the rdr lane's 0x333B30 and its
  pieces, mapped in include/deque; back() is in, pop_back() 8 bytes short), the tree's erase
  (0x306030), the CSaferStr trees (0x3A2D60-0x3A2F60 and OVL2's string-keyed map, through the
  string's out-of-line compare 0x2DEE30), zrdr_file.cpp's pointer-keyed map (0x32D780) and the map
  constructor's allocator temporary (the image passes an unconstructed stack slot: `__sinit` and
  the out-of-line map() copies) (include/tree), the class vector's destroy for elements with a virtual
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
