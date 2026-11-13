"""Scan the original B1 telescope across a fixed H target; plot count data only."""
import argparse,csv,json,math,re,subprocess,sys,time
from pathlib import Path

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--exe',default=str(Path(__file__).resolve().parent/'exampleB1'))
 p.add_argument('--out',default='H_scan')
 p.add_argument('--block-test',action='store_true',help='Fixed lead block, lower module rotated 90 deg in-plane; no XY scan')
 p.add_argument('--upper-x',type=float,default=None,help='Upper module x offset cm')
 p.add_argument('--upper-y',type=float,default=None,help='Upper module y offset cm; block default 4.3')
 p.add_argument('--lower-x',type=float,default=None,help='Lower module x offset cm; block default -4.3')
 p.add_argument('--lower-y',type=float,default=None,help='Lower module y offset cm')
 p.add_argument('--block-width',type=float,default=5.,help='Block x width cm, default assumption 5')
 p.add_argument('--block-height',type=float,default=5.,help='Block y length cm, default assumption 5')
 p.add_argument('--lower-rotation',type=float,default=None,help='Lower module rotation about vertical z axis, degrees; block default 90')
 p.add_argument('--two-scintillator',action='store_true',help='Full-optics PMT count comparison; separate reference at every position')
 p.add_argument('--paired-pmt',action='store_true',help='With --two-scintillator: identical starting muons at H arm, full-optics PMT comparison')
 p.add_argument('--pilot',action='store_true',help='With --two-scintillator: test H arm, opening and outside before a full scan')
 p.add_argument('--full-scan',action='store_true',help='With --two-scintillator: explicitly request the full XY grid')
 p.add_argument('--resume',action='store_true',help='Resume an interrupted two-scintillator scan with exactly the same settings')
 p.add_argument('--gap',type=float,default=30.,help='Active scintillator face gap in cm; default preserves B1')
 p.add_argument('--source-pmin',type=float,default=None,help='Minimum momentum GeV/c: two-scintillator default 0.1; legacy modes 1')
 p.add_argument('--source-pmax',type=float,default=100.,help='Maximum momentum GeV/c')
 p.add_argument('--source-zenith',type=float,default=70.,help='Maximum zenith angle in degrees')
 p.add_argument('--source-half-size',type=float,default=None,help='Source square half width cm: count mode 12, legacy 10')
 p.add_argument('--source-z',type=float,default=None,help='Source height cm: count mode just above upper wrapper; legacy 60')
 p.add_argument('--audit-only',action='store_true',help='With count mode: source/geometric audit and macros only; NO Geant4 run')
 p.add_argument("--scattering",action="store_true",help="Image angular deflections using ideal tracking planes, not existing B1 readout")
 p.add_argument("--image-step",type=float,default=4.,help="Scattering image bin width in cm")
 p.add_argument("--min-bin",type=int,default=30,help="Minimum tracks per displayed scattering pixel")
 p.add_argument('--paired',action='store_true',help='Single-position transport comparison with identical initial primaries')
 p.add_argument('--transport-only',action='store_true',help='Disable optical physics; use primary-muon visits, not PMT counts')
 p.add_argument('--threads',type=int,default=1,help='Geant4 workers; merged counters, separate ROOT files per worker')
 p.add_argument('--events',type=int,default=None)
 p.add_argument('--reference-events',type=int,default=10000)
 p.add_argument('--span',type=float,default=None,help='telescope translation from -span to +span, cm')
 p.add_argument('--step',type=float,default=None,help='telescope translation step, cm')
 p.add_argument('--cell',type=float,default=None,help='H stroke width, cm; total width/height = 5 cells')
 p.add_argument('--thickness',type=float,default=None,help='H thickness along z, cm')
 p.add_argument('--material',default='G4_Pb')
 p.add_argument('--selection',choices=['pe_coincidence','voltage_coincidence','primary_both'],default=None)
 p.add_argument('--seed',type=int,default=None,help='Default: fresh seed; set explicitly to reproduce a scan')
 p.add_argument('--dry-run',action='store_true')
 a=p.parse_args()
 if a.block_test:a.two_scintillator=True
 for key,block_default in [('upper_x',0.),('upper_y',4.3),('lower_x',-4.3),('lower_y',0.)]:
  if getattr(a,key) is None:setattr(a,key,block_default if a.block_test else 0.)
 if not all(math.isfinite(v) for v in (a.upper_x,a.upper_y,a.lower_x,a.lower_y)):
  p.error('Module offsets must be finite')
 if a.lower_rotation is None:a.lower_rotation=90. if a.block_test else 0.
 if not all(math.isfinite(v) for v in (a.lower_rotation,a.block_width,a.block_height)) or abs(a.lower_rotation)>360 or min(a.block_width,a.block_height)<=0 or max(a.block_width,a.block_height)>=120:
  p.error('Invalid lower rotation or block dimensions')
 for key,normal,counting in [('events',1000,10000),('span',12.,32.),('step',3.,8.),('cell',3.,8.),('thickness',3.,5.)]:
  if getattr(a,key) is None:setattr(a,key,counting if a.two_scintillator else normal)
 if a.source_pmin is None:a.source_pmin=.1 if a.two_scintillator else 1.
 if a.source_half_size is None:a.source_half_size=12. if a.two_scintillator else 10.
 if a.source_z is None:a.source_z=a.gap/2+2. if a.two_scintillator else 60.
 if not all(math.isfinite(v) for v in (a.source_pmin,a.source_pmax,a.source_zenith,a.source_half_size,a.source_z)) or not .01<=a.source_pmin<a.source_pmax<=10000 or not 0<a.source_zenith<=85 or a.source_half_size<=0 or not a.gap/2+1.5<a.source_z<79:
  p.error('Invalid source spectrum or plane; source must be above upper wrapper')
 if a.audit_only:
  if not a.two_scintillator:p.error('--audit-only requires --two-scintillator')
  a.dry_run=True
 if a.two_scintillator:
  run_two_scintillator(a,p)
  return
 if a.pilot or a.resume or a.full_scan or a.paired_pmt:p.error('--pilot/--full-scan/--resume require --two-scintillator')
 if a.scattering and (not a.transport_only or a.paired):
  p.error("--scattering requires --transport-only and cannot be combined with --paired")
 if not math.isfinite(a.image_step) or a.image_step<=0 or a.min_bin<1:
  p.error("Invalid image bin width or minimum count")
 if 2*math.ceil((a.span+10)/a.image_step)>500:
  p.error("Scattering image has too many bins")
 if not math.isfinite(a.gap) or not .2<a.gap<=100:p.error('Gap must be above 0.2 and at most 100 cm')
 if a.scattering and a.thickness>=a.gap-1.21:
  p.error("Ideal tracking planes do not fit between target and scintillators")
 if a.selection is None:a.selection="primary_both" if a.transport_only else "pe_coincidence"
 if a.transport_only and a.selection!="primary_both":
  p.error("Transport-only mode supports primary_both only; PMT outputs are unavailable")
 if a.paired and (not a.transport_only or a.span!=0 or a.events!=a.reference_events):
  p.error("--paired requires --transport-only, --span 0 and equal events/reference-events")
 if a.seed is None:
  import secrets
  a.seed=secrets.randbelow(1000000000)+1
 if a.paired and a.seed>2000000000:p.error("Paired seed must be at most 2000000000")
 if not a.dry_run:
  if a.paired or a.scattering:import uproot
  from scipy.stats import fisher_exact, beta
 if not all(math.isfinite(v) for v in [a.span,a.step,a.cell,a.thickness]) or a.span<0 or a.step<=0 or a.cell<=0 or not 0<a.thickness<a.gap-.201 or a.span+10>=60 or 2.5*a.cell>=60 or min(a.events,a.reference_events,a.seed,a.threads)<1:p.error('Invalid dimensions or event counts')
 if not re.fullmatch(r'G4_[A-Za-z0-9_]+',a.material):p.error('Use a Geant4 NIST material name, e.g. G4_Pb')
 n=round(2*a.span/a.step)
 if abs(n*a.step-2*a.span)>1e-8:p.error('2*span must be divisible by step')
 exe=Path(a.exe).resolve()
 if not a.dry_run and not exe.is_file():p.error('Compile first: executable is missing')
 out=Path(a.out).resolve()
 if out.exists():p.error('Output folder already exists. Choose a new --out name, for example H_2.')
 out.mkdir(parents=True)
 a.scan_mode="moving_telescope_fixed_target"
 (out/'settings.json').write_text(json.dumps(vars(a),indent=2))
 coords=[-a.span+i*a.step for i in range(n+1)]
 jobs=[('reference',0.,0.,a.reference_events,False)]
 for iy,y in enumerate(coords):
  for ix in (range(len(coords)) if iy%2==0 else reversed(range(len(coords)))):jobs.append((f'p_{iy:03}_{ix:03}',coords[ix],y,a.events,True))
 print(f"Geometry: fixed H {5*a.cell:g} x {5*a.cell:g} x {a.thickness:g} cm; stroke {a.cell:g} cm. Original B1 detectors.",flush=True)
 mode_label="TRANSPORT ONLY: primary-muon visits; NOT PMT detection" if a.transport_only else "Full optics"
 print(f"Plan: {len(jobs)} runs, {a.reference_events+(n+1)**2*a.events} primaries, {a.threads} workers. {mode_label}.",flush=True)
 scan_started=time.monotonic()
 fields=['name','x_cm','y_cm','events','top_hits','bottom_hits','pe_coincidence','voltage_top_hits','voltage_bottom_hits','voltage_coincidence','primary_both','primary_both_target','coincidence_without_primary_both']
 with (out/'scan.csv').open('w',newline='') as f:
  writer=csv.DictWriter(f,fieldnames=fields);writer.writeheader()
  for i,(name,x,y,events,enabled) in enumerate(jobs):
   work=out/name;work.mkdir();macro=work/'run.mac'
   macro.write_text(f"""/control/verbose 2
/random/setSeeds {a.seed+2*i} {a.seed+2*i+1}
/run/numberOfThreads {a.threads}
{source_macro(a)}
/target/enabled {str(enabled).lower()}
/target/shape {'block' if a.block_test else 'H'}
/target/width {a.block_width} cm
/target/height {a.block_height} cm
/telescope/lowerRotation {a.lower_rotation} deg
/telescope/upperX {a.upper_x} cm
/telescope/upperY {a.upper_y} cm
/telescope/lowerX {a.lower_x} cm
/telescope/lowerY {a.lower_y} cm
/tracking/enabled {str(a.scattering).lower()}
/target/material {a.material}
/target/cell {a.cell} cm
/target/thickness {a.thickness} cm
/target/x 0 cm
/target/y 0 cm
/telescope/x {x} cm
/telescope/y {y} cm
/telescope/gap {a.gap} cm
/target/z 0 cm
/run/printProgress 100
/run/initialize
/run/beamOn {events}
""")
   print(f'{i+1}/{len(jobs)} {name}: telescope x={x:g}, y={y:g} cm',flush=True)
   if a.dry_run:continue
   with (work/'run.log').open('w') as log:
    command=[str(exe),str(macro)]
    if a.transport_only:command.append("--transport-only")
    if a.paired:command.extend(["--paired-primary-seed",str(a.seed)])
    child=subprocess.Popen(command,cwd=work,stdout=log,stderr=subprocess.STDOUT)
    started=time.monotonic()
    try:
     while True:
      try:child.wait(timeout=30);break
      except subprocess.TimeoutExpired:print(f'  Geant4 running: {time.monotonic()-started:.0f} s',flush=True)
    except KeyboardInterrupt:
     child.terminate()
     try:child.wait(timeout=5)
     except subprocess.TimeoutExpired:child.kill();child.wait()
     raise
   if child.returncode or not (work/'counts.csv').is_file():raise RuntimeError(f'Run failed; inspect {work}/run.log')
   verify_run_log((work/'run.log').read_text(),a,enabled,x,y)
   with (work/'counts.csv').open() as cf:counts=next(csv.DictReader(cf))
   if set(counts)!=set(fields)-{"name","x_cm","y_cm"}:
    raise RuntimeError("Counter schema mismatch: rebuild exampleB1")
   if any(v is None or v=="" for v in counts.values()):
    raise RuntimeError(f"Missing CSV values at {name}: rebuild exampleB1; inspect counts.csv")
   if int(counts["events"])!=events:raise RuntimeError(f"Incomplete run at {name}")
   if any(not 0<=int(v)<=events for k,v in counts.items() if k!="events"):
    raise RuntimeError(f"Invalid counters at {name}")
   writer.writerow(dict(name=name,x_cm=x,y_cm=y,**counts));f.flush()
   elapsed=time.monotonic()-started
   print(f"  Finished: {events} events in {elapsed:.1f} s; {counts[a.selection]} accepted.",flush=True)
   (work/"elapsed_seconds.txt").write_text(str(elapsed))
 if a.dry_run:print('Macros only; simulation was not run.');return
 print(f'Total elapsed: {(time.monotonic()-scan_started)/60:.1f} min',flush=True)
 if a.paired:
  paired_summary(out)
 elif a.scattering:
  scattering_summary(out)
 else:
  plot(out,a.selection)

def verify_run_log(log,args,enabled,x,y):
 errors=("G4Exception","COMMAND NOT FOUND","Illegal application state",
         "Batch is interrupted","Batch command failed","Overlap is detected")
 if any(marker in log for marker in errors):
  raise RuntimeError("Geant4 command/geometry error: inspect run.log")
 gaps=re.findall(r'^B1_TELESCOPE_GAP_CM ([0-9.eE+-]+)$',log,re.MULTILINE)
 if len(gaps)!=1 or not math.isclose(float(gaps[0]),args.gap,abs_tol=1e-5):
  raise RuntimeError('Applied scintillator gap differs; rebuild exampleB1')
 if hasattr(args,'upper_x'):
  offsets=re.findall(r'^B1_MODULE_OFFSETS_CM (.+)$',log,re.MULTILINE)
  expected_offsets=[args.upper_x,args.upper_y,args.lower_x,args.lower_y]
  if len(offsets)!=1 or len(offsets[0].split())!=4 or any(not math.isclose(float(v),e,abs_tol=1e-5) for v,e in zip(offsets[0].split(),expected_offsets)):
   raise RuntimeError('Module offsets differ; rebuild B1')
 if hasattr(args,'lower_rotation'):
  rotation=re.findall(r'^B1_LOWER_ROTATION_DEG ([0-9.eE+-]+)$',log,re.MULTILINE)
  shape=re.findall(r'^B1_TARGET_SHAPE (.+)$',log,re.MULTILINE)
  expected_shape='block' if getattr(args,'block_test',False) else 'H'
  # GenericMessenger's unit conversion can quantize radians (90 deg -> 90.0002).
  # A millidegree tolerance accepts that numerical conversion, not a changed setup.
  if len(rotation)!=1 or not math.isclose(float(rotation[0]),args.lower_rotation,rel_tol=0.,abs_tol=1e-3):
   raise RuntimeError('Lower module rotation differs; rebuild B1')
  if len(shape)!=1 or len(shape[0].split())!=3 or shape[0].split()[0]!=expected_shape:
   raise RuntimeError('Target shape differs; rebuild B1')
  dims=[args.block_width,args.block_height]
  if any(not math.isclose(float(v),e,rel_tol=1e-5,abs_tol=1e-5) for v,e in zip(shape[0].split()[1:],dims)):
   raise RuntimeError('Block dimensions differ from request')
 if hasattr(args,'source_pmin'):
  records=re.findall(r'^B1_SOURCE_CONFIG (.+)$',log,re.MULTILINE)
  expected=[args.source_pmin,args.source_pmax,args.source_zenith,args.source_half_size,args.source_z]
  if len(records)!=1 or len(records[0].split())!=5 or any(not math.isclose(float(v),e,rel_tol=1e-5,abs_tol=1e-6) for v,e in zip(records[0].split(),expected)):
   raise RuntimeError('Applied source spectrum/plane differs; rebuild B1')
 expected_mode="transport-only" if args.transport_only else "full-optics"
 if re.findall(r"^B1_PHYSICS_MODE (.+)$",log,re.MULTILINE)!=[expected_mode]:
  raise RuntimeError("Physics mode differs from request; rebuild exampleB1")
 expected_seed=args.seed if (getattr(args,"paired",False) or getattr(args,"paired_pmt",False)) else 0
 if re.findall(r"^B1_PAIRED_PRIMARY_SEED ([0-9]+)$",log,re.MULTILINE)!=[str(expected_seed)]:
  raise RuntimeError("Source pairing mode/seed differs from request; rebuild exampleB1")
 entries=re.findall(r"^B1_GEOMETRY (.+)$",log,re.MULTILINE)
 if len(entries)!=1:raise RuntimeError("Missing applied geometry record; rebuild exampleB1")
 actual=entries[0].split()
 expected=[args.cell,args.thickness,x,y,0.,0.,0.]
 if len(actual)!=9 or actual[0]!=str(int(enabled)) or actual[1]!=args.material:
  raise RuntimeError("Applied target configuration differs from request")
 if any(not math.isclose(float(v),e,rel_tol=1e-5,abs_tol=1e-5) for v,e in zip(actual[2:],expected)):
  raise RuntimeError("Applied dimensions/positions differ from request")
 tracking=re.findall(r"^B1_TRACKING (.+)$",log,re.MULTILINE)
 if getattr(args,"scattering",False):
  expected_z=.5*args.thickness+.5
  if len(tracking)!=1 or len(tracking[0].split())!=3:
   raise RuntimeError("Missing ideal tracker configuration")
  flag,up,down=tracking[0].split()
  if flag!="1" or not math.isclose(float(up),expected_z,abs_tol=1e-5) or not math.isclose(float(down),-expected_z,abs_tol=1e-5):
   raise RuntimeError("Ideal tracker positions differ from request")
 elif tracking!=["0"]:
  raise RuntimeError("Unexpected tracking planes in normal mode")
 workers={int(w) for w in re.findall(r"G4WT([0-9]+)\s*>",log)}
 if len(workers)!=args.threads:
  raise RuntimeError(f"Expected {args.threads} workers, observed {len(workers)}")

def build_scattering_map(tracks,edges,min_count):
 import numpy as np
 n=len(edges)-1
 counts=np.zeros((n,n),dtype=np.int64)
 median=np.full((n,n),np.nan);rms=median.copy();mean_square_se=median.copy()
 if len(tracks)==0:return counts,median,rms,mean_square_se
 x,y,angle=tracks.T
 ix=np.searchsorted(edges,x,side="right")-1
 iy=np.searchsorted(edges,y,side="right")-1
 ix[x==edges[-1]]=n-1;iy[y==edges[-1]]=n-1
 valid=np.isfinite(x)&np.isfinite(y)&np.isfinite(angle)&(angle>=0)&(angle<=180)&(ix>=0)&(ix<n)&(iy>=0)&(iy<n)
 flat=iy[valid]*n+ix[valid];values=angle[valid]
 order=np.argsort(flat);flat=flat[order];values=values[order]
 pixels,starts,sizes=np.unique(flat,return_index=True,return_counts=True)
 for pixel,start,size in zip(pixels,starts,sizes):
  index=divmod(int(pixel),n);counts[index]=size
  if size<min_count:continue
  q=values[start:start+size]
  median[index]=float(np.median(q));rms[index]=float(np.sqrt(np.mean(q*q)))
  if size>1:mean_square_se[index]=float(np.std(q*q,ddof=1)/np.sqrt(size))
 return counts,median,rms,mean_square_se

def scattering_summary(folder):
 import numpy as np,uproot
 import matplotlib
 matplotlib.use("Agg")
 import matplotlib.pyplot as plt
 settings=json.loads((folder/"settings.json").read_text())
 if not settings.get("scattering") or not settings.get("transport_only"):
  raise ValueError("Scattering imaging requires the ideal tracker transport mode")
 with (folder/"scan.csv").open() as f:rows=list(csv.DictReader(f))
 refs=[r for r in rows if r["name"]=="reference"]
 points=[r for r in rows if r["name"]!="reference"]
 expected=round(2*settings["span"]/settings["step"])+1
 if len(refs)!=1 or len(points)!=expected*expected:
  raise ValueError("Incomplete scattering scan")
 keys=["TrackerUpRecorded","TrackerDownRecorded","TrackerImpactX_cm",
       "TrackerImpactY_cm","TrackerDeflection_deg","PrimaryVisitedBoth"]
 def collect(row):
  parts=[];events=0;both_count=0
  for path in sorted((folder/row["name"]).glob("muon_telescope_data_t*.root")):
   with uproot.open(path) as root:
    if "MuonTelescope" not in root:continue
    tree=root["MuonTelescope"];a=tree.arrays(keys,library="np")
    events+=int(tree.num_entries)
   both=a["PrimaryVisitedBoth"]==1;both_count+=int(both.sum())
   mask=both&(a["TrackerUpRecorded"]==1)&(a["TrackerDownRecorded"]==1)
   q=np.column_stack([a["TrackerImpactX_cm"][mask],a["TrackerImpactY_cm"][mask],a["TrackerDeflection_deg"][mask]])
   q=q[np.all(np.isfinite(q),axis=1)]
   if len(q):parts.append(q)
  if events!=int(row["events"]) or both_count!=int(row["primary_both"]):
   raise ValueError("ROOT counts do not match scan counters: "+row["name"])
  tracks=np.concatenate(parts) if parts else np.empty((0,3))
  return tracks,events,both_count
 reference,_,ref_both=collect(refs[0])
 target_parts=[];target_events=0;target_both=0
 for row in points:
  q,events,both=collect(row);target_events+=events;target_both+=both
  if len(q):target_parts.append(q)
 tracks=np.concatenate(target_parts) if target_parts else np.empty((0,3))
 step=settings["image_step"];half=math.ceil((settings["span"]+10)/step)
 edges=np.arange(-half,half+1,dtype=float)*step
 counts,median,rms,se=build_scattering_map(tracks,edges,settings["min_bin"])
 centers=(edges[:-1]+edges[1:])/2
 with (folder/"scattering.csv").open("w",newline="") as f:
  w=csv.writer(f)
  w.writerow(["estimated_x_cm","estimated_y_cm","tracks","median_angle_deg",
              "rms_angle_deg","mean_angle_squared_se_deg2","displayed"])
  for iy,y in enumerate(centers):
   for ix,x in enumerate(centers):
    w.writerow([x,y,int(counts[iy,ix]),median[iy,ix],rms[iy,ix],se[iy,ix],int(np.isfinite(median[iy,ix]))])
 ref_median=float(np.median(reference[:,2])) if len(reference) else None
 ref_rms=float(np.sqrt(np.mean(reference[:,2]**2))) if len(reference) else None
 summary=dict(target_events=target_events,target_primary_both=target_both,
              target_tracks_used=len(tracks),reference_primary_both=ref_both,
              reference_tracks_used=len(reference),reference_median_angle_deg=ref_median,
              reference_rms_angle_deg=ref_rms,
              displayed_pixels=int(np.sum(np.isfinite(median))),image_step_cm=step,
              selection="Primary muon visits both B1 scintillators and both ideal tracking planes",
              position="Upstream track extrapolation to target middle plane",
              interpretation="IDEAL TRACKERS: not measured by the existing B1 PMTs. No target-membership mask or known H outline is used. Reference background is global; no significance or density reconstruction is claimed.")
 (folder/"scattering_summary.json").write_text(json.dumps(summary,indent=2))
 fig,axs=plt.subplots(1,3,figsize=(16,5),layout="constrained")
 extent=[edges[0],edges[-1],edges[0],edges[-1]]
 for ax,data,title,cmap in zip(axs,[counts,median,rms],
     ["Tracked muons per pixel","Median deflection (degrees)","RMS deflection (degrees)"],
     ["viridis","inferno","inferno"]):
  if np.any(np.isfinite(data)):
   vmax=float(np.nanmax(data));vmax=vmax if vmax>0 else 1.
   im=ax.imshow(np.ma.masked_invalid(data),origin="lower",extent=extent,interpolation="nearest",
                cmap=cmap,vmin=0,vmax=vmax)
   fig.colorbar(im,ax=ax)
  else:ax.text(.5,.5,"Insufficient tracks",ha="center",va="center",transform=ax.transAxes)
  ax.set(xlabel="Estimated impact x (cm)",ylabel="Estimated impact y (cm)",title=title)
 material=settings["material"];thickness=settings["thickness"];min_bin=settings["min_bin"]
 fig.suptitle(f"IDEAL TRACKER SCATTERING IMAGE | {material}, thickness {thickness:g} cm; optics OFF"+chr(10)+
              f"Existing B1 does not measure these directions. Pixels need {min_bin} tracks; white = insufficient data.")
 fig.savefig(folder/"scattering.png",dpi=170);plt.close(fig)
 print("Scattering tracks:",len(tracks),"/",target_both,"primary-both events")
 print("Reference median angle (degrees):",ref_median)
 print("Displayed pixels:",summary["displayed_pixels"])
 print("Image saved:",folder/"scattering.png")
 print(summary["interpretation"])

def compare_paired_arrays(reference,target,expected_events):
 import numpy as np
 from scipy.stats import binomtest,beta
 source=["MuonCharge","MuonEnergy_GeV","MuonMomentum_GeV",
         "PrimaryStartX_cm","PrimaryStartY_cm","PrimaryStartZ_cm",
         "PrimaryStartDirX","PrimaryStartDirY","PrimaryStartDirZ"]
 aligned=[]
 for data in [reference,target]:
  ids=np.asarray(data["EventID"])
  order=np.argsort(ids)
  if len(ids)!=expected_events or not np.array_equal(ids[order],np.arange(expected_events)):
   raise ValueError("Missing or duplicate EventID; pair comparison refused")
  aligned.append({k:np.asarray(v)[order] for k,v in data.items()})
 reference,target=aligned
 for key in source:
  if not np.all(np.isfinite(reference[key])) or not np.array_equal(reference[key],target[key]):
   raise ValueError(f"Initial primaries differ in {key}; pair comparison refused")
 for data in [reference,target]:
  if not np.all(np.isin(data["PrimaryVisitedBoth"],[0,1])):
   raise ValueError("Invalid primary-visit flags")
 r=reference["PrimaryVisitedBoth"]==1;t=target["PrimaryVisitedBoth"]==1
 lost=int(np.sum(r&~t));gained=int(np.sum(~r&t));discordant=lost+gained
 both=int(np.sum(r&t));neither=int(np.sum(~r&~t))
 def interval(k):
  tail=.0125
  return (0. if k==0 else float(beta.ppf(tail,k,expected_events-k+1)),
          1. if k==expected_events else float(beta.ppf(1-tail,k+1,expected_events-k)))
 ll,lu=interval(lost);gl,gu=interval(gained)
 summary=dict(events=expected_events,identical_initial_primaries=True,
              reference_both=int(r.sum()),target_both=int(t.sum()),
              both=both,lost=lost,gained=gained,neither=neither,
              net_deficit=lost-gained,net_deficit_rate=(lost-gained)/expected_events,
              net_deficit_ci95_low=ll-gu,net_deficit_ci95_high=lu-gl,
              exact_p_deficit=float(binomtest(lost,discordant,.5,alternative="greater").pvalue) if discordant else 1.,
              interpretation="Initial primaries paired; transport random streams are not paired. Individual lost/gained events are not a causal decomposition.",
              observable="SIMULATION TRUTH: primary visited both scintillators; not PMT detection")
 return summary,reference,target

def paired_summary(folder):
 import numpy as np,uproot
 import matplotlib
 matplotlib.use("Agg")
 import matplotlib.pyplot as plt
 settings=json.loads((folder/"settings.json").read_text())
 if not settings.get("paired",False):raise ValueError("Not a paired experiment")
 if settings["span"]!=0 or settings["events"]!=settings["reference_events"]:
  raise ValueError("Paired comparison requires one position with equal event counts")
 keys=["EventID","MuonCharge","MuonEnergy_GeV","MuonMomentum_GeV",
       "PrimaryStartX_cm","PrimaryStartY_cm","PrimaryStartZ_cm",
       "PrimaryStartDirX","PrimaryStartDirY","PrimaryStartDirZ","PrimaryVisitedBoth"]
 def load(name):
  parts=[]
  for path in sorted((folder/name).glob("muon_telescope_data_t*.root")):
   with uproot.open(path) as root:
    if "MuonTelescope" in root:parts.append(root["MuonTelescope"].arrays(keys,library="np"))
  if not parts:raise ValueError(f"Missing ROOT event data: {name}")
  return {k:np.concatenate([a[k] for a in parts]) for k in keys}
 summary,reference,target=compare_paired_arrays(load("reference"),load("p_000_000"),settings["events"])
 (folder/"paired_summary.json").write_text(json.dumps(summary,indent=2))
 with (folder/"paired_events.csv").open("w",newline="") as f:
  w=csv.writer(f);w.writerow(["event_id","reference_primary_both","target_primary_both","category"])
  for event,r,t in zip(reference["EventID"],reference["PrimaryVisitedBoth"],target["PrimaryVisitedBoth"]):
   category="both" if r and t else "lost" if r else "gained" if t else "neither"
   w.writerow([int(event),int(r),int(t),category])
 fig,axs=plt.subplots(1,2,figsize=(10,4),layout="constrained")
 axs[0].bar(["No H","H"],[summary["reference_both"],summary["target_both"]],color=["gray","tab:blue"])
 axs[0].set(ylabel="Primary visited both scintillators",title="Paired initial muons")
 axs[1].bar(["Lost","Gained"],[summary["lost"],summary["gained"]],color=["tab:red","tab:green"])
 axs[1].set(ylabel="Events",title="Transport fluctuations also contribute")
 n=summary["events"];net=summary["net_deficit"]
 pvalue=summary["exact_p_deficit"]
 ref_count=summary["reference_both"];target_count=summary["target_both"]
 fig.suptitle(f"SIMULATION TRUTH, optics OFF: {n} identical initial muons"+chr(10)+
              f"Net deficit {net}; exact paired p = {pvalue:.4g}")
 fig.savefig(folder/"paired.png",dpi=160);plt.close(fig)
 print(f"Identical initial primaries verified: {n} events.")
 print(f"Reference both: {ref_count}; target both: {target_count}")
 print("Lost:",summary["lost"],"Gained:",summary["gained"],"Net deficit:",net)
 print("95% conservative interval for net deficit rate:",summary["net_deficit_ci95_low"],summary["net_deficit_ci95_high"])
 print("Exact paired p (deficit):",summary["exact_p_deficit"])
 print("Source is paired; transport streams are independent. Lost/gained are not H-only causal counts.")
 print("Image saved:",folder/"paired.png")

def ratio_statistics(k,m,kr,nr,comparisons):
 from scipy.stats import beta,fisher_exact
 if m<=0 or nr<=0 or not 0<=k<=m or not 0<=kr<=nr or kr==0:
  raise ValueError("Invalid counts or zero reference")
 # Two 97.5% Clopper-Pearson intervals give at least 95% joint
 # coverage; their ratio bounds are conservative per-pixel intervals.
 tail=0.0125
 def interval(s,n):
  return (0. if s==0 else float(beta.ppf(tail,s,n-s+1)),
          1. if s==n else float(beta.ppf(1-tail,s+1,n-s)))
 lo,hi=interval(k,m);lr,ur=interval(kr,nr)
 ratio=(k/m)/(kr/nr)
 pvalue=float(fisher_exact([[k,m-k],[kr,nr-kr]],alternative="less").pvalue)
 return ratio,lo/ur,hi/lr if lr>0 else float("inf"),pvalue,min(1.,comparisons*pvalue)

def plot(folder,selection):
 import numpy as np
 import matplotlib
 matplotlib.use("Agg")
 import matplotlib.pyplot as plt
 from matplotlib.colors import TwoSlopeNorm
 settings=json.loads((folder/"settings.json").read_text())
 if settings.get('two_scintillator'):
  if selection not in ('pe_coincidence',None):raise ValueError('Two-scintillator result uses PE/time coincidence only')
  plot_two_scintillator(folder)
  return
 if settings.get("scattering",False):
  scattering_summary(folder)
  return
 if settings.get("paired",False):
  if selection!="primary_both":raise ValueError("Paired transport check requires primary_both")
  paired_summary(folder)
  return
 if settings.get("transport_only",False) and selection!="primary_both":
  raise ValueError("Transport-only results cannot be plotted as PMT detection")
 moving=settings.get("scan_mode")=="moving_telescope_fixed_target"
 coordinate="telescope" if moving else "target"
 axis="Telescope" if moving else "Target"
 with (folder/"scan.csv").open() as f:rows=list(csv.DictReader(f))
 refs=[r for r in rows if r["name"]=="reference"]
 points=[r for r in rows if r["name"]!="reference"]
 n=round(2*settings["span"]/settings["step"])+1
 if len(refs)!=1 or len(points)!=n*n:raise ValueError("Refusing an incomplete scan")
 if any(selection not in r for r in rows):raise ValueError("Selected counter absent from older results")
 nr=int(refs[0]["events"]);kr=int(refs[0][selection])
 if nr<=0 or not 0<=kr<=nr:raise ValueError("Invalid reference counts")
 if kr==0:
  print("Zero reference acceptance: counts retained; transmission undefined.")
  return
 xs=sorted({float(r["x_cm"]) for r in points});ys=sorted({float(r["y_cm"]) for r in points})
 if len(xs)!=n or len(ys)!=n or len({(r["x_cm"],r["y_cm"]) for r in points})!=n*n:
  raise ValueError("Grid incomplete or duplicated")
 counts=np.full((n,n),np.nan);T=counts.copy();evidence=counts.copy()
 significant=0
 with (folder/"transmission.csv").open("w",newline="") as f:
  w=csv.writer(f)
  w.writerow([f"{coordinate}_x_cm",f"{coordinate}_y_cm","accepted","generated",
              "transmission","ratio_ci95_low","ratio_ci95_high",
              "deficit_p","deficit_p_bonferroni","significant_deficit"])
  for r in points:
   x=float(r["x_cm"]);y=float(r["y_cm"]);k=int(r[selection]);m=int(r["events"])
   ratio,lo,hi,pvalue,padj=ratio_statistics(k,m,kr,nr,n*n)
   found=padj<0.05;significant+=int(found)
   index=(ys.index(y),xs.index(x))
   counts[index]=k;T[index]=ratio;evidence[index]=-math.log10(max(padj,1e-300))
   w.writerow([x,y,k,m,ratio,lo,hi,pvalue,padj,int(found)])
 step=settings["step"]
 extent=[xs[0]-step/2,xs[-1]+step/2,ys[0]-step/2,ys[-1]+step/2]
 fig,axs=plt.subplots(1,3,figsize=(16,5),layout="constrained")
 norm=TwoSlopeNorm(vmin=0,vcenter=1,vmax=max(2.,float(np.max(T))))
 for ax,data,title,cmap,norm_arg in zip(
     axs,[counts,T,evidence],
     ["Accepted counts","Transmission (reference = 1)",
      "Deficit evidence: -log10(adjusted p)"],
     ["viridis","RdBu_r","Purples"],[None,norm,None]):
  kwargs={"norm":norm_arg} if norm_arg is not None else {}
  if ax is axs[2]:kwargs.update(vmin=0,vmax=max(2.,float(np.max(evidence))))
  im=ax.imshow(data,origin="lower",extent=extent,interpolation="nearest",cmap=cmap,**kwargs)
  fig.colorbar(im,ax=ax)
  ax.set(xlabel=f"{axis} x (cm)",ylabel=f"{axis} y (cm)",title=title)
 label="SIMULATION TRUTH: primary visits both; not PMT detection" if selection=="primary_both" else selection
 physics_label="optics OFF" if settings.get("transport_only",False) else "optics ON"
 label=label+" | "+physics_label
 width=5*settings["cell"];thickness=settings["thickness"]
 fig.suptitle(f"B1 + H ({width:g} x {width:g} x {thickness:g} cm) | {label}"+chr(10)+
              f"Reference {kr}/{nr}; significant deficit pixels: {significant}/{n*n} (family-wise 5%)")
 fig.savefig(folder/"scan.png",dpi=170);plt.close(fig)
 print(f"Image saved: {folder}/scan.png")
 print(f"Significant deficit pixels: {significant}/{n*n}; one-sided Fisher tests, Bonferroni correction.")
 print("Ratio intervals: conservative per-pixel 95%, not simultaneous across pixels.")
 print("Shared reference correlates pixels. Count response is not a reconstructed density slice.")
 if significant==0:print("No pixel shows a statistically established deficit in this scan.")
COUNT_KEYS=('events','top_hits','bottom_hits','pe_coincidence',
            'voltage_top_hits','voltage_bottom_hits','voltage_coincidence',
            'primary_both','primary_both_target','coincidence_without_primary_both')

def source_macro(a):
 return f'''/source/pMinGeV {a.source_pmin}
/source/pMaxGeV {a.source_pmax}
/source/zenithMaxDeg {a.source_zenith}
/source/halfSize {a.source_half_size} cm
/source/z {a.source_z} cm'''

def surface_flux(p,cosine):
 """Same total-energy convention as the C++ source; not a local flux calibration."""
 import numpy as np
 energy=np.sqrt(p*p+.1056583755**2)
 cs=np.sqrt((cosine*cosine+.102573**2-.068287*cosine**.958633+.0407253*cosine**.817285)/(1+.102573**2-.068287+.0407253))
 return .14*(energy+3.64/cs**1.29)**-2.7*(1/(1+1.1*energy*cs/115)+.054/(1+1.1*energy*cs/850))

def source_audit(a):
 """Deterministic quadrature and geometry bounds. No transport or image generation."""
 import numpy as np
 from scipy.stats import norm
 nodes,weights=np.polynomial.legendre.leggauss(96)
 cmin=math.cos(math.radians(a.source_zenith))
 c=(nodes+1)*(1-cmin)/2+cmin;cw=weights*(1-cmin)/2
 def integral(p0,p1):
  if p1<=p0:return 0.
  l0,l1=math.log(p0),math.log(p1)
  p=np.exp((nodes+1)*(l1-l0)/2+l0)[:,None]
  w=weights[:,None]*(l1-l0)/2
  jac=p*p/np.sqrt(p*p+.1056583755**2)
  return float(2*math.pi*np.sum(w*cw[None,:]*jac*surface_flux(p,c[None,:])*c[None,:]))
 total=integral(a.source_pmin,a.source_pmax)
 below1=integral(a.source_pmin,min(1.,a.source_pmax))
 # The source is above the entire upper wrapper. Bounding rectangle includes
 # the original taper; all direct downward rays to the active top are covered.
 upper_top=a.gap/2+1.4
 margin=(a.source_z-upper_top)*math.tan(math.radians(a.source_zenith))
 required_half=9.3+margin
 kinetic_min=math.hypot(a.source_pmin,.1056583755)-.1056583755
 plans={}
 z=norm.ppf(1-.05/len(count_positions(a)))+norm.ppf(.8)
 for delta in [.01,.05,.10]:
  plans[str(delta)]=math.ceil(2*z*z/(delta*delta))
 return dict(momentum_min_GeV_c=a.source_pmin,kinetic_min_GeV=kinetic_min,
    momentum_max_GeV_c=a.source_pmax,zenith_max_deg=a.source_zenith,
    source_z_cm=a.source_z,source_half_size_cm=a.source_half_size,
    required_half_size_for_all_direct_upper_rays_cm=required_half,
    covers_upper_direct_rays=a.source_half_size>=required_half,
    old_30cm_gap_direct_both_central_planes_source_extents_cm=[5*120/31.4,9.3*120/31.4],
    modeled_horizontal_flux_cm2_s=total,
    modeled_generated_fraction_below_1GeV_c=below1/total,
    approximate_accepted_counts_per_condition_for_80percent_power=plans,
    power_note='Normal approximation for small count deficits, equal exposure, rare independent counts, Bonferroni over planned positions. Keys are fractional deficits. Counts are accepted coincidences, NOT starting muons.',
    limitations='Low-energy extrapolation is uncertain; altitude, roof/overburden and detector calibration are not included. Plane coverage concerns direct primary entry into the upper scintillator, not all scattered-in secondary backgrounds. Enlarge the source window to check convergence. Flux is a model integral, not measured exposure time.')

def energy_diagnostics(folder):
 """Simulation-only diagnostics, never used to select the measured count image."""
 import numpy as np,uproot
 settings=json.loads((folder/'settings.json').read_text())
 if settings.get('method_version',0)<3:return
 with (folder/'scan.csv').open() as f:rows=list(csv.DictReader(f))
 bins=[0.,.1,.2,.5,1.,10.,float('inf')]
 result=[]
 for row in rows:
  generated=np.zeros(len(bins)-1,dtype=int);accepted=generated.copy()
  total=0;coincidences=0;minimum=float('inf');maximum=0.
  for path in (folder/row['name']/row['condition']).glob('muon_telescope_data_t*.root'):
   with uproot.open(path) as f:
    if 'MuonTelescope' not in f:continue
    a=f['MuonTelescope'].arrays(['MuonEnergy_GeV','MuonMomentum_GeV','Coincidence'],library='np')
   energy=a['MuonEnergy_GeV'];momentum=a['MuonMomentum_GeV'];mask=a['Coincidence']==1
   total+=len(energy);coincidences+=int(mask.sum())
   generated+=np.histogram(energy,bins)[0];accepted+=np.histogram(energy[mask],bins)[0]
   if len(momentum):minimum=min(minimum,float(momentum.min()));maximum=max(maximum,float(momentum.max()))
  if total!=int(row['events']) or coincidences!=int(row['pe_coincidence']):
   raise ValueError('ROOT/CSV diagnostic counts differ: '+row['name']+'/'+row['condition'])
  if not settings['source_pmin']-1e-6<=minimum<=maximum<=settings['source_pmax']+1e-6:
   raise ValueError('Generated source momentum differs from settings')
  result.append(dict(position=row['name'],condition=row['condition'],events=total,coincidences=coincidences,
      generated_by_kinetic_energy=generated.tolist(),accepted_by_kinetic_energy=accepted.tolist()))
 output=dict(kinetic_energy_bins_GeV=['0-0.1','0.1-0.2','0.2-0.5','0.5-1','1-10','10+'],runs=result,
   note='Simulation diagnostics only. Existing two PMTs do not measure initial muon energy; no energy cut was applied to the count image.')
 (folder/'energy_diagnostics.json').write_text(json.dumps(output,indent=2))
 print('Source/accepted energy check:',folder/'energy_diagnostics.json')

def read_count_run(work,events):
 with (work/'counts.csv').open() as f:
  rows=list(csv.DictReader(f))
 if len(rows)!=1 or set(rows[0])!=set(COUNT_KEYS):
  raise ValueError(f'Invalid counts schema: {work}/counts.csv; rebuild B1')
 row=rows[0]
 if any(v is None or v=='' for v in row.values()):raise ValueError(f'Missing counters: {work}')
 row={k:int(v) for k,v in row.items()}
 if row['events']!=events or any(not 0<=v<=events for v in row.values()):
  raise ValueError(f'Incomplete or invalid counters: {work}')
 if row['pe_coincidence']>min(row['top_hits'],row['bottom_hits']):
  raise ValueError(f'Coincidence exceeds single detector counts: {work}')
 return row

def count_positions(a):
 if getattr(a,'block_test',False):return [('blok',0.,0.)]
 if getattr(a,'paired_pmt',False):return [('kol',2*a.cell,1.5*a.cell)]
 if a.pilot:
  # These are detector translations, not event-level position measurements.
  return [('kol',2*a.cell,1.5*a.cell),('bosluk',0.,1.5*a.cell),('dis',0.,3.5*a.cell)]
 n=round(2*a.span/a.step)
 coords=[-a.span+i*a.step for i in range(n+1)]
 return [(f'p_{iy:03}_{ix:03}',coords[ix],y)
         for iy,y in enumerate(coords)
         for ix in (range(n+1) if iy%2==0 else reversed(range(n+1)))]

def run_two_scintillator(a,p):
 if sum([a.pilot,a.full_scan,a.paired_pmt,a.block_test])!=1:p.error('Choose exactly one: --pilot, --full-scan, --paired-pmt or --block-test')
 if a.paired_pmt and a.events!=a.reference_events:p.error('Paired PMT requires equal events/reference-events')
 if a.transport_only or a.scattering or a.paired or a.selection not in (None,'pe_coincidence'):
  p.error('Two-scintillator mode uses full optics and PE/time coincidence; truth/tracker modes cannot be combined')
 a.selection='pe_coincidence'
 if not all(math.isfinite(v) for v in (a.gap,a.cell,a.thickness,a.span,a.step)):
  p.error('Dimensions must be finite')
 if not .2<a.gap<=100 or not 0<a.thickness<a.gap-.201 or a.cell<=0 or 2.5*a.cell>=60:
  p.error('Target must fit in world and inside wrappers; gap must be at most 100 cm')
 if a.span<0 or a.step<=0 or min(a.events,a.reference_events,a.threads)<1:
  p.error('Invalid grid, events or workers')
 if not a.pilot and abs(round(2*a.span/a.step)*a.step-2*a.span)>1e-8:
  p.error('2*span must be divisible by step')
 if not re.fullmatch(r'G4_[A-Za-z0-9_]+',a.material):p.error('Invalid NIST material name')
 positions=count_positions(a)
 if any(max(abs(x),abs(y))+max(10,a.source_half_size)>=60 for _,x,y in positions):p.error('Source window would leave world')
 for _,x,y in positions:
  if max(abs(x+a.upper_x)+11,abs(y+a.upper_y)+11,abs(x+a.lower_x)+11,abs(y+a.lower_y)+11,abs(x+a.upper_x)+a.source_half_size,abs(y+a.upper_y)+a.source_half_size)>=60:
   p.error('Module offsets or source window leave world')
 if len(positions)>10000:p.error('Too many positions')
 out=Path(a.out).resolve();exe=Path(a.exe).resolve()
 if not a.dry_run:
  import matplotlib,numpy,scipy.stats,uproot
  if not exe.is_file():p.error('Executable missing; compile B1 first')
 if a.resume:
  if a.dry_run:p.error('Cannot combine --resume and --dry-run')
  try:old=json.loads((out/'settings.json').read_text())
  except (OSError,ValueError):p.error('Resume requires an existing settings.json')
  if a.seed is None:a.seed=old.get('seed')
 else:
  if out.exists():p.error('Output folder exists. Use a new name or --resume')
  if a.seed is None:
   import secrets
   a.seed=secrets.randbelow(1000000000)+1
 if not isinstance(a.seed,int) or not 1<=a.seed<=2000000000-4*len(positions):p.error('Seed outside supported range')
 a.scan_mode='fixed_block_fixed_telescope' if a.block_test else 'moving_telescope_fixed_target'
 config=vars(a).copy();config.update(exe=str(exe),out=str(out),resume=False,
    method_version=4,
    observable='Two PMTs: >=1 PE each and |first PE time difference| <=15 ns',
    reference_mode=('identical starting primaries; independent transport/optical streams' if a.paired_pmt else 'independent reference at every telescope position'),
    source_note='Configurable modified-Gaisser surface source, with total energy in flux and p/E Jacobian. Low-energy extrapolation and local overburden are model uncertainties. Not a calibrated experimental flux.',
    interpretation=('Fixed-position two-PMT block comparison; no scan or density reconstruction' if a.block_test else 'Count response only; no measured tracks, no density reconstruction, no guaranteed H image'))
 if a.resume:
  if old!=config:p.error('Resume settings differ. Repeat original arguments exactly, or use a new output folder')
 else:
  out.mkdir(parents=True);(out/'settings.json').write_text(json.dumps(config,indent=2))
 audit=source_audit(a)
 (out/'source_audit.json').write_text(json.dumps(audit,indent=2))
 print(f"Source: p={a.source_pmin:g}..{a.source_pmax:g} GeV/c, plane z={a.source_z:g} cm, square {2*a.source_half_size:g} cm",flush=True)
 print(f"Straight-line coverage of upper scintillator for all configured angles: {audit['covers_upper_direct_rays']}",flush=True)
 print('Low-energy spectrum is a model assumption; no automatic claim of H visibility.',flush=True)
 total=len(positions)*(a.events+a.reference_events)
 target_label=(f'block {a.block_width:g} x {a.block_height:g}' if a.block_test else f'H {5*a.cell:g} x {5*a.cell:g}')
 print(f'TWO SCINTILLATORS | full optics | {target_label} x {a.thickness:g} cm {a.material}; gap {a.gap:g} cm; lower rotation {a.lower_rotation:g} deg',flush=True)
 print(f'{len(positions)} positions, {2*len(positions)} runs, {total} starting muons, {a.threads} workers.',flush=True)
 print('Measured observable: two-PMT PE/time coincidence. '+config['reference_mode'],flush=True)
 print('No ideal trackers, true hit positions or primary-both filters enter the image.',flush=True)
 fields=['name','condition','x_cm','y_cm',*COUNT_KEYS]
 started_all=time.monotonic()
 with (out/'scan.csv').open('w',newline='') as f:
  writer=csv.DictWriter(f,fieldnames=fields);writer.writeheader()
  for i,(name,x,y) in enumerate(positions):
   for j,(condition,enabled,events) in enumerate([('referans',False,a.reference_events),('hedef',True,a.events)]):
    work=out/name/condition;work.mkdir(parents=True,exist_ok=True)
    macro=work/'run.mac';seed=a.seed+4*i+2*j
    content=f'''/control/verbose 2
/random/setSeeds {seed} {seed+1}
/run/numberOfThreads {a.threads}
{source_macro(a)}
/tracking/enabled false
/target/enabled {str(enabled).lower()}
/target/shape {'block' if a.block_test else 'H'}
/target/width {a.block_width} cm
/target/height {a.block_height} cm
/telescope/lowerRotation {a.lower_rotation} deg
/telescope/upperX {a.upper_x} cm
/telescope/upperY {a.upper_y} cm
/telescope/lowerX {a.lower_x} cm
/telescope/lowerY {a.lower_y} cm
/target/material {a.material}
/target/cell {a.cell} cm
/target/thickness {a.thickness} cm
/target/x 0 cm
/target/y 0 cm
/target/z 0 cm
/telescope/x {x} cm
/telescope/y {y} cm
/telescope/gap {a.gap} cm
/run/printProgress 1000
/run/initialize
/run/beamOn {events}
'''
    if macro.exists() and macro.read_text()!=content:raise ValueError('Existing macro differs: '+str(macro))
    macro.write_text(content)
    command=[str(exe),str(macro)]
    if a.paired_pmt:command.extend(['--paired-primary-seed',str(a.seed)])
    (work/'command.json').write_text(json.dumps(command,indent=2))
    print(f'{2*i+j+1}/{2*len(positions)} {name}/{condition}: x={x:g}, y={y:g} cm',flush=True)
    if a.dry_run:continue
    done=work/'completed.json'
    if done.exists():
     if json.loads(done.read_text())!={'seed':seed,'events':events}:raise ValueError('Completion marker differs')
     verify_run_log((work/'run.log').read_text(),a,enabled,x,y)
     counts=read_count_run(work,events)
     print('  Already completed; retained.',flush=True)
    else:
     # A failed run may have left a valid-looking CSV. Never accept it after a new failure.
     (work/'counts.csv').unlink(missing_ok=True)
     started=time.monotonic()
     with (work/'run.log').open('w') as log:
      child=subprocess.Popen(command,cwd=work,stdout=log,stderr=subprocess.STDOUT)
      try:
       while True:
        try:child.wait(timeout=30);break
        except subprocess.TimeoutExpired:
         print(f'  Running: {time.monotonic()-started:.0f} s | log: {work}/run.log',flush=True)
      except KeyboardInterrupt:
       child.terminate()
       try:child.wait(timeout=5)
       except subprocess.TimeoutExpired:child.kill();child.wait()
       print('Stopped. Resume later with the same command plus --resume.',flush=True)
       raise
     if child.returncode:raise RuntimeError(f'Geant4 failed: {work}/run.log')
     verify_run_log((work/'run.log').read_text(),a,enabled,x,y)
     counts=read_count_run(work,events)
     done.write_text(json.dumps({'seed':seed,'events':events}))
     print(f"  Finished: {counts['pe_coincidence']} coincidences / {events}; {time.monotonic()-started:.1f} s",flush=True)
    writer.writerow(dict(name=name,condition=condition,x_cm=x,y_cm=y,**counts));f.flush()
 if a.dry_run:
  print('Macros prepared only. Geant4 was NOT launched.');return
 plot_two_scintillator(out)
 energy_diagnostics(out)
 print(f'ALL RUNS COMPLETED. Elapsed {(time.monotonic()-started_all)/60:.1f} min',flush=True)

def count_comparisons(rows,settings):
 groups={}
 for row in rows:
  name=row['name'];condition=row['condition']
  if condition not in ('referans','hedef') or condition in groups.setdefault(name,{}):
   raise ValueError('Duplicate or invalid count condition')
  groups[name][condition]=row
 from types import SimpleNamespace
 expected=count_positions(SimpleNamespace(**settings))
 if set(groups)!={p[0] for p in expected}:raise ValueError('Incomplete count scan')
 results=[]
 for name,x,y in expected:
  pair=groups[name]
  if set(pair)!={'referans','hedef'}:raise ValueError('Incomplete reference/target pair')
  for condition,events in [('referans',settings['reference_events']),('hedef',settings['events'])]:
   row=pair[condition]
   if int(row['events'])!=events or not math.isclose(float(row['x_cm']),x) or not math.isclose(float(row['y_cm']),y):
    raise ValueError('Count settings/coordinates differ')
  r=pair['referans'];t=pair['hedef']
  kr=int(r['pe_coincidence']);nr=int(r['events']);k=int(t['pe_coincidence']);n=int(t['events'])
  if not 0<=kr<=nr or not 0<=k<=n:raise ValueError('Invalid count range')
  if kr:
   ratio,lo,hi,pv,padj=ratio_statistics(k,n,kr,nr,len(expected))
  else:ratio=lo=hi=pv=padj=float('nan')
  results.append(dict(name=name,telescope_x_cm=x,telescope_y_cm=y,
      reference_accepted=kr,reference_generated=nr,target_accepted=k,target_generated=n,
      transmission=ratio,ratio_ci95_low=lo,ratio_ci95_high=hi,
      deficit_p=pv,deficit_p_bonferroni=padj,
      significant_deficit=int(math.isfinite(padj) and padj<.05)))
 return results

def plot_two_scintillator(folder):
 if json.loads((folder/'settings.json').read_text()).get('block_test'):
  plot_fixed_block(folder)
  return
 if json.loads((folder/'settings.json').read_text()).get('paired_pmt'):
  paired_pmt_summary(folder)
  return
 import numpy as np
 import matplotlib
 matplotlib.use('Agg')
 import matplotlib.pyplot as plt
 from matplotlib.colors import TwoSlopeNorm
 settings=json.loads((folder/'settings.json').read_text())
 if not settings.get('two_scintillator') or settings.get('transport_only') or settings.get('scattering'):
  raise ValueError('Not a two-PMT full-optics count scan')
 with (folder/'scan.csv').open() as f:results=count_comparisons(list(csv.DictReader(f)),settings)
 with (folder/'transmission.csv').open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(results[0]));w.writeheader();w.writerows(results)
 sig=sum(r['significant_deficit'] for r in results)
 summary={'positions':len(results),'significant_deficit_positions':sig,
          'selection':settings['observable'],'optics':'ON','ideal_trackers':False,
          'gap_cm':settings['gap'],'material':settings['material'],'thickness_cm':settings['thickness'],
          'interpretation':'Independent per-position references. Conservative per-position 95% ratio intervals; Bonferroni-corrected one-sided Fisher deficit tests. A significant deficit at one point does not establish an H image. Source sampling is not exposure time.'}
 summary['precision_diagnostics']=[dict(position=r['name'],
    approximate_relative_ratio_standard_error=(math.sqrt(1/r['reference_accepted']-1/r['reference_generated']+1/r['target_accepted']-1/r['target_generated']) if min(r['reference_accepted'],r['target_accepted'])>0 else None)) for r in results]
 (folder/'counting_summary.json').write_text(json.dumps(summary,indent=2))
 fig,axs=plt.subplots(1,2,figsize=(12,5),layout='constrained')
 if settings['pilot']:
  index=np.arange(len(results));labels=[r['name'] for r in results]
  nr=np.array([r['reference_generated'] for r in results]);nt=np.array([r['target_generated'] for r in results])
  kr=np.array([r['reference_accepted'] for r in results]);kt=np.array([r['target_accepted'] for r in results])
  axs[0].bar(index-.18,10000*kr/nr,.36,label='No H')
  axs[0].bar(index+.18,10000*kt/nt,.36,label='H present')
  axs[0].set(xticks=index,xticklabels=labels,ylabel='Coincidences per 10000 starting muons',title='Two-PMT counts');axs[0].legend()
  for i,r in enumerate(results):
   value=r['transmission'];lo=r['ratio_ci95_low'];hi=r['ratio_ci95_high']
   if all(math.isfinite(q) for q in (value,lo,hi)):
    axs[1].errorbar(i,value,yerr=[[value-lo],[hi-value]],fmt='o',capsize=5,color='tab:blue')
   else:axs[1].text(i,1,'Undefined\nzero reference',ha='center')
  axs[1].axhline(1,color='gray',linestyle='--')
  axs[1].set(xticks=index,xticklabels=labels,ylabel='Transmission',title='Conservative per-position 95% intervals')
 else:
  xs=sorted({r['telescope_x_cm'] for r in results});ys=sorted({r['telescope_y_cm'] for r in results})
  T=np.full((len(ys),len(xs)),np.nan);evidence=T.copy()
  for r in results:
   ix=xs.index(r['telescope_x_cm']);iy=ys.index(r['telescope_y_cm'])
   T[iy,ix]=r['transmission'];q=r['deficit_p_bonferroni']
   if math.isfinite(q):evidence[iy,ix]=-math.log10(max(q,1e-300))
  half=settings['step']/2;extent=[xs[0]-half,xs[-1]+half,ys[0]-half,ys[-1]+half]
  finite=T[np.isfinite(T)];vmax=max(2.,float(finite.max())) if finite.size else 2.
  for ax,data,title,cmap,norm in zip(axs,[T,evidence],['Two-PMT transmission','Deficit evidence: -log10(adjusted p)'],['RdBu_r','Purples'],[TwoSlopeNorm(vmin=0,vcenter=1,vmax=vmax),None]):
   kwargs={'norm':norm} if norm else {'vmin':0,'vmax':max(2.,float(np.nanmax(evidence))) if np.any(np.isfinite(evidence)) else 2.}
   im=ax.imshow(np.ma.masked_invalid(data),origin='lower',extent=extent,cmap=cmap,interpolation='nearest',**kwargs)
   fig.colorbar(im,ax=ax);ax.set(xlabel='Telescope x (cm)',ylabel='Telescope y (cm)',title=title)
 fig.suptitle(f"TWO SCINTILLATORS | full optics | {settings['material']} {settings['thickness']:g} cm; gap {settings['gap']:g} cm\nSignificant deficit positions: {sig}/{len(results)}; no track information used")
 fig.savefig(folder/'scan.png',dpi=170);plt.close(fig)
 for r in results:
  print(f"{r['name']}: ref={r['reference_accepted']}/{r['reference_generated']}, H={r['target_accepted']}/{r['target_generated']}, T={r['transmission']:.4g}, adjusted p={r['deficit_p_bonferroni']:.4g}")
 print('Image:',folder/'scan.png')
 print('Numbers:',folder/'transmission.csv')
 if sig==0:print('No established count deficit. This does not prove zero physical effect; inspect the uncertainty intervals before planning more runs.')
 for r in summary['precision_diagnostics']:
  if r['approximate_relative_ratio_standard_error'] is not None:
   print(f"{r['position']}: approximate relative ratio uncertainty (1 sigma) = {100*r['approximate_relative_ratio_standard_error']:.1f}%")
 print(summary['interpretation'])

def compare_paired_pmt(reference,target,events):
 import numpy as np
 for data in (reference,target):
  if not np.all(data['OpticalPhysicsEnabled']==1):raise ValueError('PMT comparison requires optical physics in every event')
  if not np.all(np.isin(data['Coincidence'],[0,1])):raise ValueError('Invalid coincidence flags')
  if not np.all(data['PEThreshold']==1) or not np.all(data['TimeWindow_ns']==15):
   raise ValueError('PMT threshold/window differs from configured observable')
 # Reuse source identity and exact paired binary-outcome statistics. Only the
 # measured coincidence is supplied as the binary outcome, never geometry truth.
 r=dict(reference);t=dict(target)
 r['PrimaryVisitedBoth']=r['Coincidence'];t['PrimaryVisitedBoth']=t['Coincidence']
 summary,r,t=compare_paired_arrays(r,t,events)
 summary['reference_detected']=summary.pop('reference_both')
 summary['target_detected']=summary.pop('target_both')
 summary['observable']='Two PMTs: >=1 PE each and |first PE time difference| <=15 ns; full optics'
 summary['interpretation']='Identical starting muons verified event by event. Transport and optical streams are independent. Lost/gained are paired detection outcomes, not a causal list of muons stopped by H. Simulation pairing does not improve real detector sensitivity.'
 return summary,r,t

def paired_pmt_summary(folder):
 import numpy as np,uproot
 import matplotlib
 matplotlib.use('Agg')
 import matplotlib.pyplot as plt
 settings=json.loads((folder/'settings.json').read_text())
 if not settings.get('paired_pmt') or not settings.get('two_scintillator') or settings.get('transport_only') or settings.get('scattering'):
  raise ValueError('Not a paired full-optics two-PMT comparison')
 if settings['events']!=settings['reference_events']:raise ValueError('Unequal paired sample sizes')
 keys=['EventID','MuonCharge','MuonEnergy_GeV','MuonMomentum_GeV',
       'PrimaryStartX_cm','PrimaryStartY_cm','PrimaryStartZ_cm',
       'PrimaryStartDirX','PrimaryStartDirY','PrimaryStartDirZ',
       'Coincidence','OpticalPhysicsEnabled','PEThreshold','TimeWindow_ns']
 from types import SimpleNamespace
 args=SimpleNamespace(**settings);x=2*settings['cell'];y=1.5*settings['cell']
 def load(condition,enabled):
  work=folder/'kol'/condition
  verify_run_log((work/'run.log').read_text(),args,enabled,x,y)
  counts=read_count_run(work,settings['events'])
  parts=[]
  for path in sorted(work.glob('muon_telescope_data_t*.root')):
   with uproot.open(path) as f:
    if 'MuonTelescope' in f:parts.append(f['MuonTelescope'].arrays(keys,library='np'))
  if not parts:raise ValueError('Missing ROOT events: '+str(work))
  data={k:np.concatenate([p[k] for p in parts]) for k in keys}
  if len(data['EventID'])!=counts['events'] or int(np.count_nonzero(data['Coincidence']==1))!=counts['pe_coincidence']:
   raise ValueError('ROOT and CSV detection counts differ: '+condition)
  return data
 summary,r,t=compare_paired_pmt(load('referans',False),load('hedef',True),settings['events'])
 summary.update(telescope_x_cm=x,telescope_y_cm=y,source_seed=settings['seed'],
                confidence_interval='Conservative two-sided 95% interval for net deficit per starting event; exact p is one-sided McNemar/binomial. They are not the same interval/test construction.')
 (folder/'paired_summary.json').write_text(json.dumps(summary,indent=2))
 with (folder/'paired_events.csv').open('w',newline='') as f:
  w=csv.writer(f);w.writerow(['event_id','reference_PMT_coincidence','target_PMT_coincidence','category'])
  for event,rc,tc in zip(r['EventID'],r['Coincidence'],t['Coincidence']):
   w.writerow([int(event),int(rc),int(tc),'both' if rc and tc else 'lost' if rc else 'gained' if tc else 'neither'])
 fig,axs=plt.subplots(1,3,figsize=(14,4),layout='constrained')
 axs[0].bar(['No H','H'],[summary['reference_detected'],summary['target_detected']],color=['gray','tab:blue'])
 axs[0].set(ylabel='Two-PMT coincidences',title='Identical starting muons')
 axs[1].bar(['Lost','Gained'],[summary['lost'],summary['gained']],color=['tab:red','tab:green'])
 axs[1].set(ylabel='Paired events',title='Transport/optical randomness also contributes')
 n=summary['events'];v=summary['net_deficit'];lo=n*summary['net_deficit_ci95_low'];hi=n*summary['net_deficit_ci95_high']
 axs[2].errorbar([0],[v],yerr=[[v-lo],[hi-v]],fmt='o',capsize=6)
 axs[2].axhline(0,color='gray',linestyle='--');axs[2].set(xticks=[0],xticklabels=['Reference - H'],ylabel='Net detection deficit',title='Conservative 95% interval')
 fig.suptitle(f"Two scintillators, full optics | {n} verified starting pairs | exact one-sided p={summary['exact_p_deficit']:.4g}")
 fig.savefig(folder/'paired.png',dpi=170);plt.close(fig)
 print(f"IDENTICAL INITIAL MUONS VERIFIED: {n}")
 print(f"PMT counts: reference={summary['reference_detected']}, H={summary['target_detected']}")
 print(f"Lost={summary['lost']}, gained={summary['gained']}, net deficit={v}; 95% interval [{lo:.2f}, {hi:.2f}]")
 print('Exact paired deficit p:',summary['exact_p_deficit'])
 print('Image:',folder/'paired.png')
 print(summary['interpretation'])

def plot_fixed_block(folder):
 import numpy as np
 import matplotlib
 matplotlib.use('Agg')
 import matplotlib.pyplot as plt
 from scipy.stats import beta
 settings=json.loads((folder/'settings.json').read_text())
 if not settings.get('block_test') or settings.get('transport_only') or settings.get('scattering'):
  raise ValueError('Fixed block requires full-optics two-PMT counts')
 with (folder/'scan.csv').open() as f:rows=list(csv.DictReader(f))
 results=count_comparisons(rows,settings)
 if len(results)!=1:raise ValueError('Fixed block comparison must have exactly one position')
 result=results[0]
 with (folder/'transmission.csv').open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(result));w.writeheader();w.writerow(result)
 summary=dict(result,target_shape='block',material=settings['material'],
    width_cm=settings['block_width'],height_cm=settings['block_height'],thickness_cm=settings['thickness'],
    gap_cm=settings['gap'],lower_rotation_deg=settings['lower_rotation'],
    module_offsets_cm={k:settings.get(k,0.) for k in ['upper_x','upper_y','lower_x','lower_y']},
    observable='Two PMTs: at least one PE each, first-PE time difference <=15 ns',
    interpretation='Single fixed position; independent no-block/block runs. No scan or track-based selection. A block smaller than the vertical overlap does not intercept every accepted muon. Ratio intervals are conservative 95%; exact deficit p is one-sided Fisher.')
 (folder/'block_summary.json').write_text(json.dumps(summary,indent=2))
 k=np.array([result['reference_accepted'],result['target_accepted']]);n=np.array([result['reference_generated'],result['target_generated']])
 rate=k/n;lo=[];hi=[]
 for ki,ni in zip(k,n):
  lo.append(0 if ki==0 else beta.ppf(.025,ki,ni-ki+1))
  hi.append(1 if ki==ni else beta.ppf(.975,ki+1,ni-ki))
 fig,axs=plt.subplots(1,2,figsize=(10,4.5),layout='constrained')
 axs[0].bar(['No block','Lead block'],10000*rate,color=['tab:blue','tab:orange'])
 axs[0].errorbar([0,1],10000*rate,yerr=[10000*(rate-np.array(lo)),10000*(np.array(hi)-rate)],fmt='none',ecolor='black',capsize=5)
 axs[0].set(ylabel='PMT coincidences per 10000 starting muons',title='Independent 95% binomial intervals')
 for i in range(2):axs[0].text(i,10000*hi[i],f'{k[i]}/{n[i]}',ha='center',va='bottom')
 value=result['transmission'];low=result['ratio_ci95_low'];high=result['ratio_ci95_high']
 if all(math.isfinite(v) for v in (value,low,high)):
  axs[1].errorbar([0],[value],yerr=[[value-low],[high-value]],fmt='o',capsize=6)
 else:axs[1].text(.5,.5,'Undefined ratio: zero reference',transform=axs[1].transAxes,ha='center')
 axs[1].axhline(1,color='gray',linestyle='--');axs[1].set(xticks=[0],xticklabels=['Block / no block'],ylabel='Normalized count ratio',title='Conservative 95% ratio interval')
 fig.suptitle(f"Fixed {settings['block_width']:g} x {settings['block_height']:g} x {settings['thickness']:g} cm {settings['material']} block | lower module {settings['lower_rotation']:g} deg\nFull optics; gap {settings['gap']:g} cm; no XY scan")
 fig.savefig(folder/'scan.png',dpi=170);plt.close(fig)
 print('No-block PMT coincidences:',int(k[0]),'/',int(n[0]))
 print('Block PMT coincidences:',int(k[1]),'/',int(n[1]))
 print('Transmission:',value,'95% interval:',low,high,'one-sided deficit p:',result['deficit_p'])
 print('Image:',folder/'scan.png')
 print('Summary:',folder/'block_summary.json')

if __name__=="__main__":main()
