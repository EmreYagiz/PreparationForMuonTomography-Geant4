#!/usr/bin/env bash
set -euo pipefail
# Starts simulations ONLY when the user explicitly runs this script.
base=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-mesafe_100k_1}
if [[ "$out" = /* ]]; then dest=$out; else dest="$PWD/$out"; fi
if [[ -e "$dest" ]]; then
    echo "Klasor zaten var; yeni bir cikti adi secin: $dest" >&2
    exit 1
fi
[[ -x "$base/exampleB1" ]] || { echo "exampleB1 bulunamadi; once derleyin." >&2; exit 1; }
mkdir -p -- "$dest"
for gap in 10 15 20; do
    for condition in reference block; do
        macro="${condition}_${gap}cm.mac"
        cp -- "$base/$macro" "$dest/$macro"
        echo "BASLIYOR: gap=$gap cm, $condition (100000 olay)"
        (cd -- "$dest"; "$base/exampleB1" "$macro" 2>&1 | tee "${condition}_${gap}cm.log")
    done
done
echo "ALTI KOSU TAMAMLANDI: $dest"
echo "Analiz: root -l -b -q '$base/mesafe_analiz.C(\"$dest\")'"
