# zlib 1.1.4, as linked into SCUS_972.75

These files are **zlib 1.1.4, unmodified** (`zlib-1.1.4.tar.gz`, sha256
`9e3e973174f9910fd51539ef9ce94c86a3943d4f897fab8e9adf4b19e6a8291e`, from
<https://zlib.net/fossils/>), copyright Jean-loup Gailly and Mark Adler under the
zlib licence (the header of `zlib.h`). Only the inflate side is here: it is all the
loader links (`uncompress` decodes the two engine overlays at boot, `0x1C5DA0`).

Built by `../../sccheck.py` with the decomp's settled toolchain, every function the
image holds matches byte for byte: 21 functions at `0x1C6930`-`0x1CA3CC`. Absent from
the image, and so not scored: `inflateReset` (inlined into both callers by
`-inline auto`, then dead-stripped), `inflateSetDictionary`, `inflateSync`,
`inflateSyncPoint`, `inflate_set_dictionary`, `inflate_blocks_sync_point`,
`zlibVersion`, `zError` (never referenced).

This is the decomp's CALIBRATION: the source is certain, so every mismatch here was
the compiler or a flag, never a guess. It is how the compiler build (3.0b52/3.0b50)
and every flag in `scdiff.FLAGS` were settled.
