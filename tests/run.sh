#!/usr/bin/env bash
set -u

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
UZ=${UZ:-$ROOT/uz}
TMP=$(mktemp -d "${TMPDIR:-/tmp}/uz-test.XXXXXX") || exit 1
trap 'rm -rf "$TMP"' EXIT

SRC="$TMP/src"
ARC="$TMP/archives"
OUT="$TMP/out"
PASS=0
FAIL=0
SKIP=0

mkdir -p "$SRC/sub" "$ARC" "$OUT"

printf 'alpha\n' > "$SRC/a.txt"
printf 'beta\n' > "$SRC/sub/b.txt"
printf 'space file\n' > "$SRC/hello world.txt"
head -c 4096 /dev/urandom > "$SRC/random.bin"

have() {
    command -v "$1" >/dev/null 2>&1
}

ok() {
    PASS=$((PASS + 1))
    printf 'PASS  %s\n' "$1"
}

bad() {
    FAIL=$((FAIL + 1))
    printf 'FAIL  %s\n' "$1"
}

skip() {
    SKIP=$((SKIP + 1))
    printf 'SKIP  %s\n' "$1"
}

require() {
    have "$1" || return 1
    return 0
}

extract_container() {
    name=$1
    archive=$2
    dest="$OUT/$name"
    rm -rf "$dest"
    if "$UZ" -q -o "$dest" "$archive" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$dest/src/a.txt" &&
       cmp -s "$SRC/sub/b.txt" "$dest/src/sub/b.txt" &&
       cmp -s "$SRC/random.bin" "$dest/src/random.bin" &&
       cmp -s "$SRC/hello world.txt" "$dest/src/hello world.txt"; then
        ok "$name extract"
    else
        bad "$name extract"
    fi

    if "$UZ" -l "$archive" >/dev/null 2>&1; then ok "$name list"; else bad "$name list"; fi
    if "$UZ" -t "$archive" >/dev/null 2>&1; then ok "$name test"; else bad "$name test"; fi
}

extract_bare() {
    name=$1
    archive=$2
    dest="$OUT/$name"
    rm -rf "$dest"
    if "$UZ" -q -o "$dest" "$archive" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$dest/bare.txt"; then
        ok "$name extract"
    else
        bad "$name extract"
    fi

    if "$UZ" -l "$archive" 2>/dev/null | grep -q 'bare\.txt'; then
        ok "$name list"
    else
        bad "$name list"
    fi
    if "$UZ" -t "$archive" >/dev/null 2>&1; then ok "$name test"; else bad "$name test"; fi
}

# ---- build fixtures -------------------------------------------------------

SZ=""
if have 7z; then SZ=7z
elif have 7za; then SZ=7za
elif have 7zz; then SZ=7zz
fi

if [ -n "$SZ" ]; then
    ( cd "$TMP" && "$SZ" a -tzip "$ARC/plain.zip" src >/dev/null 2>&1 )
    ( cd "$TMP" && "$SZ" a -t7z  "$ARC/plain.7z"  src >/dev/null 2>&1 )
    ( cd "$TMP" && "$SZ" a -tzip -ppw123 "$ARC/enc.zip" src >/dev/null 2>&1 )
    ( cd "$TMP" && "$SZ" a -t7z -ppw123 -mhe=on "$ARC/enc.7z" src >/dev/null 2>&1 )
    ( cd "$TMP" && "$SZ" a -v1k "$ARC/split.7z" src >/dev/null 2>&1 )
    ( cd "$TMP" && "$SZ" a -tzip -v1k "$ARC/split.zip" src >/dev/null 2>&1 )
else
    skip "zip/7z fixtures (7z unavailable)"
fi

if have tar; then
    tar -cf "$ARC/plain.tar" -C "$TMP" src
    tar -czf "$ARC/plain.tar.gz" -C "$TMP" src
    tar -cjf "$ARC/plain.tar.bz2" -C "$TMP" src
    tar -cJf "$ARC/plain.tar.xz" -C "$TMP" src
    if have zstd; then tar --zstd -cf "$ARC/plain.tar.zst" -C "$TMP" src; fi
    if have lz4; then tar -cf - -C "$TMP" src | lz4 -q > "$ARC/plain.tar.lz4"; fi
    if have lzip; then tar -cf - -C "$TMP" src | lzip -c > "$ARC/plain.tar.lz"; fi
    if have xz; then tar -cf - -C "$TMP" src | xz --format=lzma -c > "$ARC/plain.tar.lzma"; fi
else
    skip "tar fixtures (tar unavailable)"
fi

if have gzip; then gzip -c "$SRC/a.txt" > "$ARC/bare.txt.gz"; fi
if have bzip2; then bzip2 -c "$SRC/a.txt" > "$ARC/bare.txt.bz2"; fi
if have xz; then
    xz -c "$SRC/a.txt" > "$ARC/bare.txt.xz"
    xz --format=lzma -c "$SRC/a.txt" > "$ARC/bare.txt.lzma"
fi
if have zstd; then zstd -q -c "$SRC/a.txt" > "$ARC/bare.txt.zst"; fi
if have lz4; then lz4 -q -c "$SRC/a.txt" > "$ARC/bare.txt.lz4"; fi
if have lzip; then lzip -c "$SRC/a.txt" > "$ARC/bare.txt.lz"; fi

if have ar; then
    ar rcs "$ARC/sample.ar" "$SRC/a.txt"
fi

if [ -f "$ARC/plain.tar.gz" ]; then
    head -c 80 "$ARC/plain.tar.gz" > "$ARC/corrupt.tar.gz"
fi

# ---- containers -----------------------------------------------------------

for f in plain.zip plain.7z plain.tar plain.tar.gz plain.tar.bz2 plain.tar.xz \
         plain.tar.zst plain.tar.lz4 plain.tar.lz plain.tar.lzma; do
    if [ -f "$ARC/$f" ]; then
        extract_container "$f" "$ARC/$f"
    else
        skip "$f (fixture unavailable)"
    fi
done

# ---- bare streams ---------------------------------------------------------

for f in bare.txt.gz bare.txt.bz2 bare.txt.xz bare.txt.zst bare.txt.lz4 \
         bare.txt.lzma bare.txt.lz; do
    if [ -f "$ARC/$f" ]; then
        extract_bare "$f" "$ARC/$f"
    else
        skip "$f (fixture unavailable)"
    fi
done

# ---- rar (optional, needs a sample) --------------------------------------

if [ -n "${UZ_TEST_RAR:-}" ] && [ -f "${UZ_TEST_RAR}" ]; then
    RAR=$UZ_TEST_RAR
    if "$UZ" -l "$RAR" >/dev/null 2>&1; then ok "rar list"; else bad "rar list"; fi
    if "$UZ" -t "$RAR" >/dev/null 2>&1; then ok "rar test"; else bad "rar test"; fi
    rm -rf "$OUT/rar"
    if "$UZ" -q -o "$OUT/rar" "$RAR" >/dev/null 2>&1 &&
       [ -n "$(find "$OUT/rar" -type f -print -quit)" ]; then
        ok "rar extract"
    else
        bad "rar extract"
    fi
else
    skip "rar (set UZ_TEST_RAR=/path/to/file.rar)"
fi

# ---- split archives -------------------------------------------------------

for spec in 'split7:split.7z.001' 'splitzip:split.zip.001'; do
    name=${spec%%:*}
    first=${spec#*:}
    if [ ! -f "$ARC/$first" ]; then
        skip "$name (fixture unavailable)"
        continue
    fi
    if "$UZ" -l "$ARC/$first" >/dev/null 2>&1; then ok "$name list"; else bad "$name list"; fi
    if "$UZ" -t "$ARC/$first" >/dev/null 2>&1; then ok "$name test"; else bad "$name test"; fi
    rm -rf "$OUT/$name"
    if "$UZ" -q -o "$OUT/$name" "$ARC/$first" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$OUT/$name/src/a.txt"; then
        ok "$name extract"
    else
        bad "$name extract"
    fi
done

# ---- unknown format fallback ---------------------------------------------

if [ -f "$ARC/sample.ar" ]; then
    if "$UZ" -l "$ARC/sample.ar" >/dev/null 2>&1; then ok "unknown list"; else bad "unknown list"; fi
    if "$UZ" -t "$ARC/sample.ar" >/dev/null 2>&1; then ok "unknown test"; else bad "unknown test"; fi
    rm -rf "$OUT/unknown"
    if "$UZ" -q -o "$OUT/unknown" "$ARC/sample.ar" >/dev/null 2>&1 &&
       [ -n "$(find "$OUT/unknown" -type f -print -quit)" ]; then
        ok "unknown extract"
    else
        bad "unknown extract"
    fi
else
    skip "unknown fallback (ar unavailable)"
fi

# ---- passwords ------------------------------------------------------------

if [ -f "$ARC/enc.zip" ] && [ -f "$ARC/enc.7z" ]; then
    rm -rf "$OUT/enczip" "$OUT/enc7z"
    if "$UZ" -q -p pw123 -o "$OUT/enczip" "$ARC/enc.zip" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$OUT/enczip/src/a.txt"; then
        ok "encrypted zip extract"
    else
        bad "encrypted zip extract"
    fi
    if "$UZ" -q -p pw123 -o "$OUT/enc7z" "$ARC/enc.7z" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$OUT/enc7z/src/a.txt"; then
        ok "encrypted 7z extract"
    else
        bad "encrypted 7z extract"
    fi
    if "$UZ" --password=pw123 -t "$ARC/enc.7z" >/dev/null 2>&1; then
        ok "--password=PW"
    else
        bad "--password=PW"
    fi
    "$UZ" -t -p wrong "$ARC/enc.zip" >/dev/null 2>&1
    [ $? -eq 2 ] && ok "wrong password rc" || bad "wrong password rc"
    if have timeout; then
        timeout 3 "$UZ" -t "$ARC/enc.zip" >/dev/null 2>&1
        [ $? -eq 2 ] && ok "encrypted zip without -p fails fast" || bad "encrypted zip without -p fails fast"
        timeout 3 "$UZ" -t "$ARC/enc.7z" >/dev/null 2>&1
        [ $? -eq 2 ] && ok "encrypted 7z without -p fails fast" || bad "encrypted 7z without -p fails fast"
    else
        skip "encrypted without -p (timeout unavailable)"
    fi
else
    skip "encrypted archives (7z unavailable)"
fi

# ---- overwrite semantics --------------------------------------------------

if [ -f "$ARC/plain.zip" ]; then
    rm -rf "$OUT/forcezip"
    "$UZ" -q -o "$OUT/forcezip" "$ARC/plain.zip" >/dev/null 2>&1
    printf 'changed\n' > "$OUT/forcezip/src/a.txt"
    "$UZ" -q -o "$OUT/forcezip" "$ARC/plain.zip" >/dev/null 2>&1
    if [ "$(cat "$OUT/forcezip/src/a.txt")" = "changed" ]; then
        ok "zip skips existing without -f"
    else
        bad "zip skips existing without -f"
    fi
    "$UZ" -q -f -o "$OUT/forcezip" "$ARC/plain.zip" >/dev/null 2>&1
    if cmp -s "$SRC/a.txt" "$OUT/forcezip/src/a.txt"; then
        ok "zip overwrite with -f"
    else
        bad "zip overwrite with -f"
    fi
fi

if [ -f "$ARC/bare.txt.gz" ]; then
    rm -rf "$OUT/bareforce"
    "$UZ" -q -o "$OUT/bareforce" "$ARC/bare.txt.gz" >/dev/null 2>&1
    printf 'changed\n' > "$OUT/bareforce/bare.txt"
    "$UZ" -q -o "$OUT/bareforce" "$ARC/bare.txt.gz" >/dev/null 2>&1
    if [ "$(cat "$OUT/bareforce/bare.txt")" = "changed" ]; then
        ok "bare stream skips existing without -f"
    else
        bad "bare stream skips existing without -f"
    fi
    "$UZ" -q -f -o "$OUT/bareforce" "$ARC/bare.txt.gz" >/dev/null 2>&1
    if cmp -s "$SRC/a.txt" "$OUT/bareforce/bare.txt"; then
        ok "bare stream overwrite with -f"
    else
        bad "bare stream overwrite with -f"
    fi
fi

# ---- atomic bare output ---------------------------------------------------

if have gzip; then
    gzip -c "$SRC/random.bin" > "$TMP/big.bin.gz"
    head -c 200 "$TMP/big.bin.gz" > "$TMP/trunc.gz"
    rm -rf "$OUT/partial"
    mkdir -p "$OUT/partial"
    "$UZ" -q -o "$OUT/partial" "$TMP/trunc.gz" >/dev/null 2>&1
    if [ $? -eq 2 ] && [ ! -e "$OUT/partial/big.bin" ] &&
       [ -z "$(find "$OUT/partial" -name '*.tmp.*' -print -quit)" ]; then
        ok "failed bare extraction leaves no file"
    else
        bad "failed bare extraction leaves no file"
    fi
    printf 'OLD\n' > "$OUT/partial/big.bin"
    "$UZ" -q -f -o "$OUT/partial" "$TMP/trunc.gz" >/dev/null 2>&1
    if [ $? -eq 2 ] && [ "$(cat "$OUT/partial/big.bin")" = "OLD" ]; then
        ok "failed -f keeps previous file"
    else
        bad "failed -f keeps previous file"
    fi
else
    skip "atomic bare output (gzip unavailable)"
fi

# ---- paths ----------------------------------------------------------------

if [ -f "$ARC/bare.txt.gz" ]; then
    rm -rf "$OUT/symlink"
    mkdir -p "$OUT/symlink"
    printf 'target\n' > "$OUT/symlink/target"
    ln -s target "$OUT/symlink/bare.txt"
    "$UZ" -q -f -o "$OUT/symlink" "$ARC/bare.txt.gz" >/dev/null 2>&1
    if [ $? -eq 2 ] && [ "$(cat "$OUT/symlink/target")" = "target" ]; then
        ok "symlink not followed"
    else
        bad "symlink not followed"
    fi
fi

if [ -f "$ARC/plain.zip" ]; then
    rm -rf "$OUT/mkdirfile"
    mkdir -p "$OUT/mkdirfile"
    cp "$ARC/plain.zip" "$OUT/mkdirfile/foo.zip"
    touch "$OUT/mkdirfile/foo"
    "$UZ" -q "$OUT/mkdirfile/foo.zip" >/dev/null 2>"$OUT/mkdirfile/err"
    if [ $? -eq 2 ] && grep -qi 'not a directory' "$OUT/mkdirfile/err"; then
        ok "destination not a directory"
    else
        bad "destination not a directory"
    fi
fi

if [ -f "$ARC/plain.tar.gz" ]; then
    rm -rf "$TMP/default"
    mkdir -p "$TMP/default"
    cp "$ARC/plain.tar.gz" "$TMP/default/demo.tar.gz"
    ( cd "$TMP/default" && "$UZ" -q demo.tar.gz >/dev/null 2>&1 )
    if cmp -s "$SRC/a.txt" "$TMP/default/demo/src/a.txt"; then
        ok "default destination"
    else
        bad "default destination"
    fi
fi

if [ -f "$ARC/plain.zip" ]; then
    rm -rf "$OUT/nested" "$OUT/positional"
    if "$UZ" -q -o "$OUT/nested/a/b" "$ARC/plain.zip" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$OUT/nested/a/b/src/a.txt"; then
        ok "nested destination"
    else
        bad "nested destination"
    fi
    if "$UZ" -q "$ARC/plain.zip" "$OUT/positional" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$OUT/positional/src/a.txt"; then
        ok "positional destination"
    else
        bad "positional destination"
    fi
    rm -rf "$OUT/outopt"
    if "$UZ" -q --output="$OUT/outopt" "$ARC/plain.zip" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$OUT/outopt/src/a.txt"; then
        ok "--output=DIR"
    else
        bad "--output=DIR"
    fi
fi

if [ -f "$ARC/plain.tar.gz" ]; then
    mkdir -p "$OUT/dashdata" "$OUT/dash"
    cp "$ARC/plain.tar.gz" "$OUT/dashdata/-dash.tar.gz"
    if ( cd "$OUT/dashdata" && "$UZ" -q -o "$OUT/dash" -- -dash.tar.gz ) >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$OUT/dash/src/a.txt"; then
        ok "leading dash with --"
    else
        bad "leading dash with --"
    fi
fi

if [ -f "$ARC/plain.tar.xz" ] && have xz; then
    long="$TMP/long"
    for i in $(seq 1 70); do long="$long/segment-abcdefghij-$i"; done
    mkdir -p "$long/dest"
    cp "$ARC/plain.tar.xz" "$long/very-long-archive-name.tar.xz"
    cp "$ARC/bare.txt.xz" "$long/very-long-file-name.txt.xz"
    if "$UZ" -q -o "$long/dest" "$long/very-long-archive-name.tar.xz" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$long/dest/src/a.txt"; then
        ok "long path tar.xz"
    else
        bad "long path tar.xz"
    fi
    if "$UZ" -q -o "$long/dest" "$long/very-long-file-name.txt.xz" >/dev/null 2>&1 &&
       cmp -s "$SRC/a.txt" "$long/dest/very-long-file-name.txt"; then
        ok "long path bare xz"
    else
        bad "long path bare xz"
    fi
fi

# ---- diagnostics ----------------------------------------------------------

rm -rf "$OUT/quiet"
if [ -f "$ARC/plain.tar.gz" ]; then
    "$UZ" -q -o "$OUT/quiet" "$ARC/plain.tar.gz" >"$OUT/q.out" 2>"$OUT/q.err"
    if [ $? -eq 0 ] && [ ! -s "$OUT/q.out" ] && [ ! -s "$OUT/q.err" ]; then
        ok "quiet mode"
    else
        bad "quiet mode"
    fi
fi

if [ -f "$ARC/enc.7z" ]; then
    "$UZ" -v -q -p pw123 -l "$ARC/enc.7z" >"$OUT/v.out" 2>"$OUT/v.err"
    if grep -q 'uz: exec:' "$OUT/v.err" &&
       grep -qF '***' "$OUT/v.err" &&
       ! grep -q 'pw123' "$OUT/v.err"; then
        ok "verbose password masking"
    else
        bad "verbose password masking"
    fi
fi

if [ -f "$ARC/corrupt.tar.gz" ]; then
    "$UZ" -t "$ARC/corrupt.tar.gz" >"$OUT/c.out" 2>"$OUT/c.err"
    if [ $? -eq 2 ] && [ -s "$OUT/c.err" ]; then
        ok "corrupt errors visible"
    else
        bad "corrupt errors visible"
    fi
    "$UZ" -q -t "$ARC/corrupt.tar.gz" >"$OUT/cq.out" 2>"$OUT/cq.err"
    if [ $? -eq 2 ] && [ ! -s "$OUT/cq.err" ]; then
        ok "quiet suppresses corrupt errors"
    else
        bad "quiet suppresses corrupt errors"
    fi
fi

"$UZ" >/dev/null 2>&1
[ $? -eq 1 ] && ok "usage rc" || bad "usage rc"
"$UZ" --not-an-option >/dev/null 2>&1
[ $? -eq 1 ] && ok "unknown option rc" || bad "unknown option rc"
if [ -f "$ARC/bare.txt.gz" ]; then
    PATH=/nonexistent "$UZ" -t "$ARC/bare.txt.gz" >/dev/null 2>&1
    [ $? -eq 3 ] && ok "missing tool rc" || bad "missing tool rc"
else
    skip "missing tool rc (fixture unavailable)"
fi
"$UZ" -h >/dev/null 2>&1
[ $? -eq 0 ] && ok "help" || bad "help"

VER=$(sed -n 's/.*UZ_VERSION "\([^"]*\)".*/\1/p' "$ROOT/unizip.h")
if [ -n "$VER" ] && "$UZ" -V | grep -q "uz $VER"; then
    ok "version"
else
    bad "version"
fi

printf '\nPASS=%d FAIL=%d SKIP=%d\n' "$PASS" "$FAIL" "$SKIP"
[ "$FAIL" -eq 0 ]
