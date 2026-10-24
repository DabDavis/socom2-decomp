#!/usr/bin/env bash
# Push decomp/ to its PUBLIC GitHub repository, github.com/DabDavis/socom2-decomp, as main.
#
#   decomp/tools/publish.sh            # scan, split, push
#   decomp/tools/publish.sh --dry-run  # scan and split only
#
# claude-socom stays the one place the work happens; the repository is `git subtree
# split --prefix=decomp` of it, which re-derives the same commits every time, so a
# push only ever appends. It pushes COMMITTED history only -- commit first.
#
# The repository is public, so before pushing it refuses any tracked decomp/ line that looks
# private (a home path, a mail address, a LAN address, a token), as claude-eqoa's
# _publish.py does. The tree carries no game code or data by construction: the target is
# built from the owner's disc and nothing under extracted/ or toolchains/ is tracked.
set -euo pipefail
cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"

REMOTE=decomp
URL=git@github.com:DabDavis/socom2-decomp.git
git remote get-url "$REMOTE" >/dev/null 2>&1 || git remote add "$REMOTE" "$URL"

PRIVATE='/home/[a-z]|[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[a-z]{2,}|\b(10|192\.168|172\.(1[6-9]|2[0-9]|3[01]))\.[0-9]+\.[0-9]+\.[0-9]+|gh[op]_[A-Za-z0-9]'
ALLOWED='noreply@anthropic|madler@|jloup@|gzip\.org|zlib\.net|git@github\.com:'
if hits=$(git ls-files -z decomp | xargs -0 grep -n -I -E "$PRIVATE" | grep -v -E "$ALLOWED"); then
    echo "refusing to publish; private-looking lines:" >&2
    echo "$hits" >&2
    exit 1
fi

SPLIT=$(git subtree split --prefix=decomp HEAD 2>/dev/null)
echo "decomp/ at $(git rev-parse --short HEAD) -> split $(git rev-parse --short "$SPLIT"): $(git log -1 --format=%s "$SPLIT")"
[ "${1:-}" = "--dry-run" ] && exit 0
git push "$REMOTE" "$SPLIT:refs/heads/main"
