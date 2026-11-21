#!/usr/bin/env bash
# ============================================================================
#  Fetch an upstream domain-list seed into data/src_<tag>.txt
# ----------------------------------------------------------------------------
#  These lists are NOT redistributed in this repository. They are only used as
#  raw input for ./build.sh, which merges them with data/rules_raw.txt and
#  compiles everything into the self-defined binary container (data/rules.qrb).
#
#  Xin does not depend on any upstream format or code at runtime: after the
#  build, the upstream text is irrelevant. That is the whole point of owning
#  the container format, the compiler and the matcher.
#
#  Usage:  bash scripts/fetch-rules.sh [url] [tag]
# ============================================================================
set -e

URL="${1:-https://anti-ad.net/domains.txt}"
TAG="${2:-antiad}"

ROOT_RAW="$(cd "$(dirname "$0")/.." && pwd)"
ROOT="$(cd "$ROOT_RAW" && pwd -W 2>/dev/null || echo "$ROOT_RAW")"
OUT="$ROOT/data/src_${TAG}.txt"

echo "fetch : $URL"
code=$(curl -sL -m 60 -o "$OUT" -w "%{http_code}" "$URL" || echo "000")
size=$(wc -c < "$OUT" 2>/dev/null || echo 0)
lines=$(wc -l < "$OUT" 2>/dev/null || echo 0)

echo "http  : $code"
echo "bytes : $size"
echo "lines : $lines"

if [ "$code" != "200" ] || [ "$size" -lt 1000 ]; then
  echo "FAILED: refusing to keep a short/invalid download"
  rm -f "$OUT"
  exit 1
fi

echo "saved : data/src_${TAG}.txt"
echo "next  : bash build.sh"
