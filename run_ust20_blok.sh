#!/usr/bin/env bash
set -euo pipefail
base=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
name=${1:-Pb8_ust_20cm_1}
if [[ "$name" = /* ]]; then out=$name; else out="$PWD/$name"; fi
[[ -x "$base/exampleB1" ]] || { echo 'exampleB1 bulunamadi.' >&2; exit 1; }
[[ ! -e "$out" ]] || { echo "Klasor zaten var; yeni ad secin: $out" >&2; exit 1; }
for condition in reference block; do
    [[ -f "$base/ust20_${condition}.mac" ]] || { echo 'Makro bulunamadi.' >&2; exit 1; }
done
mkdir -p -- "$out"
for condition in reference block; do
    macro="ust20_${condition}.mac"
    cp -- "$base/$macro" "$out/$macro"
    echo "BASLIYOR: $condition; 20 cm aralik, 8x8x5 cm Pb, z=6.5 cm; 500000 olay"
    (cd -- "$out"; "$base/exampleB1" "$macro" 2>&1 | tee "${condition}.log")
done
echo "UST BLOK IKI KOSU TAMAMLANDI: $out"
