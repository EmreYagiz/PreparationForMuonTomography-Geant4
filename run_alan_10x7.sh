#!/usr/bin/env bash
set -euo pipefail
base=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
name=${1:-alan_10x7_1}
if [[ "$name" = /* ]]; then out=$name; else out="$PWD/$name"; fi
[[ -x "$base/exampleB1" ]] || { echo 'exampleB1 bulunamadi.' >&2; exit 1; }
[[ ! -e "$out" ]] || { echo "Klasor zaten var; yeni ad secin: $out" >&2; exit 1; }
for condition in reference block; do
    [[ -f "$base/${condition}_10x7.mac" ]] || { echo 'Makro bulunamadi.' >&2; exit 1; }
done
mkdir -p -- "$out"
for condition in reference block; do
    macro="${condition}_10x7.mac"
    cp -- "$base/$macro" "$out/$macro"
    echo "BASLIYOR: 10x7 cm ortak alan, $condition, 100000 olay"
    (cd -- "$out"; "$base/exampleB1" "$macro" 2>&1 | tee "${condition}.log")
done
echo "10x7 CM IKI KOSU TAMAMLANDI: $out"
