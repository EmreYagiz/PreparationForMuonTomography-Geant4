"""Redraw existing count data without launching Geant4."""
import argparse,json
from pathlib import Path
from scan_H import plot
p=argparse.ArgumentParser(description=__doc__)
p.add_argument("directory")
p.add_argument("--selection",choices=["pe_coincidence","voltage_coincidence","primary_both"])
a=p.parse_args()
folder=Path(a.directory)
settings=json.loads((folder/"settings.json").read_text())
selection=a.selection or settings.get("selection","pe_coincidence")
plot(folder,selection)
