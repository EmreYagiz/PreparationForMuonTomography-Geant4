#!/usr/bin/env bash
set -euo pipefail
# Only the USER starts this script. Reuse the completed legacy-weight 10 cm pair.
base=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
name=${1:-aralik_10_30_1}
previous=${2:-"$base/alan_10x10_tekrar_1"}
previous=$(cd -- "$previous" && pwd)
if [[ "$name" = /* ]]; then out=$name; else out="$PWD/$name"; fi
[[ -x "$base/exampleB1" ]] || { echo 'exampleB1 bulunamadi.' >&2; exit 1; }
[[ ! -e "$out" ]] || { echo "Klasor zaten var; yeni ad secin: $out" >&2; exit 1; }
for condition in reference block; do
    [[ -s "$previous/$condition.root" ]] || { echo "10 cm dosyasi yok: $previous/$condition.root" >&2; exit 1; }
done
for gap in 20 25 30; do
    for condition in reference block; do
        [[ -f "$base/sweep_${condition}_${gap}cm.mac" ]] || { echo 'Tarama makrosu bulunamadi.' >&2; exit 1; }
    done
done
mkdir -p -- "$out"
# Symbolic links: old ROOT contents are neither changed nor copied.
ln -s -- "$previous/reference.root" "$out/reference_10cm.root"
ln -s -- "$previous/block.root" "$out/block_10cm.root"
echo "10 cm: mevcut veriler kullaniliyor ($previous)"
echo 'Yeni plan: 20, 25, 30 cm; her birinde 100000 referans + 100000 bloklu olay.'
index=0
for gap in 20 25 30; do
    for condition in reference block; do
        index=$((index+1))
        macro="sweep_${condition}_${gap}cm.mac"
        cp -- "$base/$macro" "$out/$macro"
        echo "$index/6 BASLIYOR: $gap cm, $condition"
        (cd -- "$out"; "$base/exampleB1" "$macro" 2>&1 | tee "${condition}_${gap}cm.log")
    done
done
echo "ARALIK TARAMASI TAMAMLANDI: $out"
echo 'Analiz: root -l -b -q '\''aralik_analiz.C("'"$out"'")'\'
