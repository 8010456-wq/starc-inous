#!/usr/bin/env bash
# ============================================================================
#  Xin / 辛  ——  one-command build + self-test
# ----------------------------------------------------------------------------
#  Usage:
#      STARC_CC=/path/to/starcc.exe bash build.sh
#
#  Produces (in ./dist, all PE32+ .sc binaries):
#      xin_pack.sc    rule compiler  : text list  -> rules.qrb
#      xin_core.sc    engine + bench : rules.qrb   -> per-domain verdicts
#      xin_hosts.sc   hosts compiler : rules.qrb   -> hosts_block.txt
#      xin_serve.sc   local HTTP API : 127.0.0.1:8353/check?domain=...
#
#  Requires: StarC compiler (starcc), bash, coreutils (grep/awk/sed/sort).
#  No gcc, no Python, no Node, no third-party libraries.
# ============================================================================
set -e

# --- locate project root (works from any cwd) -------------------------------
ROOT_RAW="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$ROOT_RAW" && pwd -W 2>/dev/null || echo "$ROOT_RAW")"
ROOT="${ROOT//\\//}"

# --- locate StarC compiler --------------------------------------------------
CC="${STARC_CC:-D:/starc/starcc_v13/starcc.exe}"
if [ ! -f "$CC" ]; then
  echo "ERROR: StarC compiler not found: $CC"
  echo "       set it explicitly, e.g.  STARC_CC=/d/starc/starcc_v13/starcc.exe bash build.sh"
  exit 1
fi

export LC_ALL=C
mkdir -p "$ROOT/dist" "$ROOT/build"

echo "[0/5] project root"
echo "      $ROOT"
echo "      compiler = $CC"

# --- 1. normalize + sort raw rule text --------------------------------------
#   upstream seed (optional) + hand-maintained rules -> one sorted list
echo "[1/5] normalize + sort domains"
{
  cat "$ROOT"/data/src_*.txt 2>/dev/null || true
  cat "$ROOT/data/rules_raw.txt"
} \
  | grep -v '^#' \
  | grep -v '^[[:space:]]*$' \
  | tr 'A-Z' 'a-z' \
  | grep -E '^[a-z0-9]([a-z0-9._-]*[a-z0-9])?$' \
  | awk 'length($0) <= 47' \
  | sort -u > "$ROOT/data/rules_sorted.txt"
echo "      domains = $(wc -l < "$ROOT/data/rules_sorted.txt")"

# --- 2. path-patch .starc sources into ./build ------------------------------
#   The v1.2.1 sources embed an absolute workspace path. We rewrite it into a
#   scratch copy so the tracked sources stay clean and the build is portable.
echo "[2/5] patch source paths -> build/"
for f in "$ROOT"/src/core/*.starc "$ROOT"/src/compiler/*.starc; do
  sed "s#D:/starc/xin#${ROOT}#g" "$f" > "$ROOT/build/$(basename "$f")"
done

# --- 3. compile the four binaries -------------------------------------------
echo "[3/5] compile"
"$CC" "$ROOT/build/xin_pack.starc"  "$ROOT/src/core/u_xin.staro" "$ROOT/src/compiler/u_cos.staro"                  -o "$ROOT/dist/xin_pack.sc"  >/dev/null
"$CC" "$ROOT/build/xin_core.starc"  "$ROOT/src/core/u_xin.staro"                                                   -o "$ROOT/dist/xin_core.sc"  >/dev/null
"$CC" "$ROOT/build/xin_hosts.starc" "$ROOT/src/core/u_xin.staro" "$ROOT/src/core/u_hosts.staro"                    -o "$ROOT/dist/xin_hosts.sc" >/dev/null
if "$CC" "$ROOT/build/xin_serve.starc" "$ROOT/src/core/u_xin.staro" "$ROOT/src/core/u_http.staro" -o "$ROOT/dist/xin_serve.sc" >/dev/null 2>&1; then
  echo "      xin_serve.sc   ok"
else
  echo "      xin_serve.sc   SKIPPED (file locked: stop the running xin_serve first)"
fi

# --- 4. pack the binary rule container --------------------------------------
echo "[4/5] pack rules -> data/rules.qrb"
echo "      version = $(cat "$ROOT/data/VERSION" 2>/dev/null || echo unknown)"
echo "      mode    = $(cat "$ROOT/data/mode.txt" 2>/dev/null || echo soft)"
"$ROOT/dist/xin_pack.sc"

# --- 5. self-test + hosts fragment ------------------------------------------
echo "[5/5] self-test"
"$ROOT/dist/xin_core.sc"
"$ROOT/dist/xin_hosts.sc"

echo ""
echo "OK artifacts:"
ls -l "$ROOT/dist/xin_pack.sc" "$ROOT/dist/xin_core.sc" "$ROOT/dist/xin_hosts.sc" "$ROOT/data/rules.qrb" "$ROOT/data/hosts_block.txt" 2>/dev/null
