#!/usr/bin/env python3
"""Decrypt SOCOM II r0001's two engine overlays off the disc: the decomp's TARGET.

    decomp/.venv/bin/python decomp/tools/decrypt_overlays.py \
        "SCUS-97275 (1.02).iso" reference/socom-unzipped/tools_py

The engine is not in SCUS_972.75 (its overlay segments are FileSiz 0, docs/storage.md);
it ships DNAS-encrypted and zlib-compressed in RUN/RAW/APACHE00.ZDB. This runs the
disc's OWN decoder under Unicorn, using socom-unzipped's harness
(github.com/Scotho/socom-unzipped, GPL-3.0, cloned into reference/ which git ignores):

  1. dnas_selfdecrypt.decrypt  -- DNAS.BIN carries 131 self-encrypting code blocks;
                                   each is decrypted by running its own cipher core.
                                   THIS is what stopped our cmd/ovlextract: it ran the
                                   blocks still encrypted, and the init walked into IOP RPC.
  2. decrypt_apache            -- the loader's four calls (0x539d00, 0x539d50, 0x534848,
                                   0x535018) with sceSif*/libcdvd answered in HLE, then inflate.

Every input and output is checked against the digests socom-unzipped pins for the r0001
disc (tools_py/disc_to_elf_expected.json), so a wrong disc or a drifted harness refuses
rather than handing the decomp a target that is not the shipped code.

Writes <out>/ftscore.bin (OVL1, loads at 0x1E7000) and <out>/zsealetc.bin (OVL2,
0x4C5380), both MWo3 images; <out> defaults to extracted/overlays/ (git-ignored).
Needs unicorn==2.1.4 (decomp/.venv). Takes a few minutes.
"""
import hashlib
import json
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

WANT = ('/SCUS_972.75', '/OVERLAY/REL/DNAS.BIN', '/RUN/RAW/APACHE00.ZDB')


def iso_files(iso_path, want):
    """{path: bytes} for the wanted files, read with a minimal ISO 9660 walk."""
    iso = open(iso_path, 'rb')

    def sec(n, cnt=1):
        iso.seek(n * 2048)
        return iso.read(cnt * 2048)

    pvd = sec(16)
    if pvd[1:6] != b'CD001':
        raise SystemExit(f'{iso_path}: no ISO 9660 volume descriptor at sector 16')

    def walk(lba, size, path=''):
        d = sec(lba, (size + 2047) // 2048)
        off = 0
        while off < size:
            n = d[off]
            if n == 0:
                off = (off // 2048 + 1) * 2048
                continue
            rec = d[off:off + n]
            elba, esz = struct.unpack_from('<I', rec, 2)[0], struct.unpack_from('<I', rec, 10)[0]
            name = rec[33:33 + rec[32]]
            if name not in (b'\0', b'\1'):
                p = path + '/' + name.decode('ascii', 'replace').split(';')[0]
                if rec[25] & 2:
                    yield from walk(elba, esz, p)
                else:
                    yield p, elba, esz
            off += n

    root = pvd[156:156 + 34]
    out = {}
    for p, lba, size in walk(struct.unpack_from('<I', root, 2)[0], struct.unpack_from('<I', root, 10)[0]):
        if p in want:
            iso.seek(lba * 2048)
            out[p] = iso.read(size)
    missing = set(want) - set(out)
    if missing:
        raise SystemExit(f'{iso_path}: not on the disc: {sorted(missing)}')
    return out


def check(label, data, expect):
    got = hashlib.sha256(data).hexdigest()
    if len(data) != expect['size'] or got != expect['sha256']:
        raise SystemExit(f'{label}: {len(data)} B sha256 {got}, expected {expect["size"]} B {expect["sha256"]}'
                         ' -- not the r0001 disc, or the harness drifted')
    print(f'ok  {label}  {len(data)} B  {got[:16]}')


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    iso_path, tools_py = sys.argv[1], os.path.abspath(sys.argv[2])
    out = os.path.abspath(sys.argv[3]) if len(sys.argv) > 3 else os.path.join(ROOT, 'extracted', 'overlays')
    expected = json.load(open(os.path.join(tools_py, 'disc_to_elf_expected.json')))

    files = iso_files(iso_path, WANT)
    for p in WANT:
        check(p.lstrip('/'), files[p], expected['inputs'][p.lstrip('/')])

    # the harness reads a disc TREE; give it one holding only the three files it opens
    game = os.path.join(out, 'disc')
    for p in WANT:
        dst = os.path.join(game, *p.strip('/').split('/'))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, 'wb').write(files[p])

    sys.path.insert(0, tools_py)
    import dnas_selfdecrypt
    import decrypt_apache

    variants = {int(k, 16): {'keytab': int(v['keytab'], 16), 'core': int(v['core'], 16)}
                for k, v in expected['dnas']['variants'].items()}
    dec, meta = dnas_selfdecrypt.decrypt(files['/OVERLAY/REL/DNAS.BIN'], variants)
    if len(meta) != expected['dnas']['blocks']:
        raise SystemExit(f'DNAS.BIN: {len(meta)} self-encrypted blocks, expected {expected["dnas"]["blocks"]}')
    check('DNAS.dec.bin', dec, expected['dnas']['output'])
    open(os.path.join(game, 'OVERLAY', 'REL', 'DNAS.dec.bin'), 'wb').write(dec)

    for path in decrypt_apache.decrypt_package(game, os.path.join(game, 'RUN', 'RAW', 'APACHE00.ZDB'), out):
        check(os.path.basename(path), open(path, 'rb').read(), expected['overlays'][os.path.basename(path)])
    print(f'wrote {out}/ftscore.bin and {out}/zsealetc.bin')


if __name__ == '__main__':
    main()
