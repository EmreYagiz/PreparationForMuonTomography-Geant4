#!/usr/bin/env bash
set -euo pipefail
# Run explicitly by the user; only the two NEW 8 cm measurements.
base=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-mesafe_8cm_100k_1}
if [[ "$out" = /* ]]; then dest=$out; else dest="$PWD/$out"; fi
[[ -x "$base/exampleB1" ]] || { echo "exampleB1 bulunamadi." >&2; exit 1; }
for condition in reference block; do
    [[ -f "$base/${condition}_8cm.mac" ]] || { echo "8 cm makrosu bulunamadi." >&2; exit 1; }
done
if [[ -e "$dest" ]]; then
    echo "Cikti klasoru zaten var; yeni ad secin: $dest" >&2
    exit 1
fi
mkdir -p -- "$dest"
for condition in reference block; do
    macro="${condition}_8cm.mac"
    cp -- "$base/$macro" "$dest/$macro"
    echo "BASLIYOR: 8 cm $condition (100000 olay)"
    (cd -- "$dest"; "$base/exampleB1" "$macro" 2>&1 | tee "${condition}_8cm.log")
done
echo "8 CM IKI KOSU TAMAMLANDI: $dest"
