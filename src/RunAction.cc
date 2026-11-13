#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Threading.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"
#include "Randomize.hh"
#include <fstream>
#include <sstream>

namespace B1 {
RunAction::RunAction() {
  auto* m=G4AccumulableManager::Instance();
  m->Register(fEvents);m->Register(fTop);m->Register(fBottom);m->Register(fCoincidences);
  auto* a=G4AnalysisManager::Instance();
  a->SetDefaultFileType("root");
  a->SetNtupleMerging(true);
  a->SetFileName("coincidence_data");
  a->CreateNtuple("MuonTelescope","Every source event: PMT coincidences and diagnostic energy deposits");
  a->CreateNtupleIColumn("RunID");
  a->CreateNtupleIColumn("EventID");
  a->CreateNtupleIColumn("WorkerID");
  a->CreateNtupleIColumn("BlockEnabled");
  a->CreateNtupleIColumn("MuonCharge");
  a->CreateNtupleIColumn("Coincidence");
  a->CreateNtupleIColumn("PMT1Hit");
  a->CreateNtupleIColumn("PMT2Hit");
  a->CreateNtupleIColumn("PEThreshold");
  a->CreateNtupleIColumn("TimeCoincidence");
  a->CreateNtupleIColumn("PhotonsPMT1");
  a->CreateNtupleIColumn("PhotonsPMT2");
  a->CreateNtupleIColumn("PEPMT1");
  a->CreateNtupleIColumn("PEPMT2");
  a->CreateNtupleIColumn("ScintPhotonsPMT1");
  a->CreateNtupleIColumn("ScintPhotonsPMT2");
  a->CreateNtupleIColumn("CerenkovPhotonsPMT1");
  a->CreateNtupleIColumn("CerenkovPhotonsPMT2");
  a->CreateNtupleIColumn("OtherOpticalPhotonsPMT1");
  a->CreateNtupleIColumn("OtherOpticalPhotonsPMT2");
  a->CreateNtupleIColumn("PrimaryVisitedTop");
  a->CreateNtupleIColumn("PrimaryVisitedBottom");
  a->CreateNtupleIColumn("PrimaryVisitedBoth");
  a->CreateNtupleIColumn("PrimaryEnteredBlock");
  a->CreateNtupleIColumn("PrimaryStoppedInBlock");
  a->CreateNtupleIColumn("PrimaryBlockEntries");
  a->CreateNtupleIColumn("PrimaryBlockExits");
  a->CreateNtupleDColumn("MuonEnergy_GeV");
  a->CreateNtupleDColumn("MuonMomentum_GeV");
  a->CreateNtupleDColumn("MuonTheta_deg");
  a->CreateNtupleDColumn("MuonPhi_deg");
  a->CreateNtupleDColumn("PrimaryStartX_cm");
  a->CreateNtupleDColumn("PrimaryStartY_cm");
  a->CreateNtupleDColumn("PrimaryStartZ_cm");
  a->CreateNtupleDColumn("PrimaryStartDirX");
  a->CreateNtupleDColumn("PrimaryStartDirY");
  a->CreateNtupleDColumn("PrimaryStartDirZ");
  a->CreateNtupleDColumn("EdepTop_MeV");
  a->CreateNtupleDColumn("EdepBottom_MeV");
  a->CreateNtupleDColumn("EdepBlock_MeV");
  a->CreateNtupleDColumn("FirstPETimePMT1_ns");
  a->CreateNtupleDColumn("FirstPETimePMT2_ns");
  a->CreateNtupleDColumn("LastPETimePMT1_ns");
  a->CreateNtupleDColumn("LastPETimePMT2_ns");
  a->CreateNtupleDColumn("MeanPETimePMT1_ns");
  a->CreateNtupleDColumn("MeanPETimePMT2_ns");
  a->CreateNtupleDColumn("RMSPETimePMT1_ns");
  a->CreateNtupleDColumn("RMSPETimePMT2_ns");
  a->CreateNtupleDColumn("DeltaT_ns");
  a->CreateNtupleDColumn("TimeWindow_ns");
  a->CreateNtupleDColumn("FirstPhotonTransitPMT1_ns");
  a->CreateNtupleDColumn("FirstPhotonTransitPMT2_ns");
  a->CreateNtupleDColumn("MeanPhotonTransitPMT1_ns");
  a->CreateNtupleDColumn("MeanPhotonTransitPMT2_ns");
  a->CreateNtupleDColumn("PrimaryEdepTop_MeV");
  a->CreateNtupleDColumn("PrimaryEdepBottom_MeV");
  a->CreateNtupleDColumn("PrimaryEdepBlock_MeV");
  a->CreateNtupleDColumn("PrimaryPathTop_cm");
  a->CreateNtupleDColumn("PrimaryPathBottom_cm");
  a->CreateNtupleDColumn("PrimaryPathBlock_cm");
  a->CreateNtupleDColumn("PrimaryBlockEntryKinetic_MeV");
  a->CreateNtupleDColumn("PrimaryBlockExitKinetic_MeV");
  a->FinishNtuple();
  a->CreateNtuple("RunInfo","One row per worker and run; sum Events across workers");
  for (const auto* name:{"SchemaVersion","RunID","WorkerID","Events","TopHits","BottomHits","Coincidences","BlockEnabled","PEThreshold"}) a->CreateNtupleIColumn(name);
  for (const auto* name:{"BlockWidth_cm","BlockHeight_cm","BlockThickness_cm","Gap_cm","UpperCenterX_cm","UpperCenterY_cm","UpperCenterZ_cm","LowerCenterX_cm","LowerCenterY_cm","LowerCenterZ_cm","LowerRotation_deg","TimeWindow_ns","SourcePMin_GeV_c","SourcePMax_GeV_c","SourceZenithMax_deg","SourceHalfSize_cm","SourceZ_cm","SourceCenterX_cm","SourceCenterY_cm","OpticalMaxTime_ns","OpticalMaxSteps","WallTime_s"}) a->CreateNtupleDColumn(name);
  a->CreateNtupleSColumn("BlockMaterial");
  a->CreateNtupleSColumn("SourceModel");
  a->CreateNtupleSColumn("InitialRandomEngineState");
  a->CreateNtupleDColumn("OverlapWidth_cm");
  a->CreateNtupleDColumn("OverlapHeight_cm");
  a->CreateNtupleDColumn("OverlapArea_cm2");
  a->CreateNtupleDColumn("BlockCenterX_cm");
  a->CreateNtupleDColumn("BlockCenterY_cm");
  a->CreateNtupleDColumn("BlockCenterZ_cm");
  a->FinishNtuple();
}
void RunAction::BeginOfRunAction(const G4Run*) {
  G4AccumulableManager::Instance()->Reset();
  G4RunManager::GetRunManager()->SetRandomNumberStore(false);
  fStarted=std::chrono::steady_clock::now();
  std::ostringstream state;G4Random::getTheEngine()->put(state);fInitialRandomState=state.str();
  auto* a=G4AnalysisManager::Instance();
  if (IsMaster()) {
    auto name=a->GetFileName();
    if (name.size()<5 || name.substr(name.size()-5)!=".root") name+=".root";
    if (std::ifstream(name).good()) G4Exception("RunAction","ExistingROOT",FatalException,"ROOT output already exists. Select a new /analysis/setFileName or a new directory.");
  }
  if (!a->OpenFile()) G4Exception("RunAction","OpenROOT",FatalException,"Cannot open ROOT output.");
}
void RunAction::RecordEvent(const EventRecord& e) {
  fEvents+=1;if (e.PMT1Hit) fTop+=1;if (e.PMT2Hit) fBottom+=1;if(e.Coincidence) fCoincidences+=1;
  auto* a=G4AnalysisManager::Instance();
  a->FillNtupleIColumn(0,0,e.RunID);
  a->FillNtupleIColumn(0,1,e.EventID);
  a->FillNtupleIColumn(0,2,e.WorkerID);
  a->FillNtupleIColumn(0,3,e.BlockEnabled);
  a->FillNtupleIColumn(0,4,e.MuonCharge);
  a->FillNtupleIColumn(0,5,e.Coincidence);
  a->FillNtupleIColumn(0,6,e.PMT1Hit);
  a->FillNtupleIColumn(0,7,e.PMT2Hit);
  a->FillNtupleIColumn(0,8,e.PEThreshold);
  a->FillNtupleIColumn(0,9,e.TimeCoincidence);
  a->FillNtupleIColumn(0,10,e.PhotonsPMT1);
  a->FillNtupleIColumn(0,11,e.PhotonsPMT2);
  a->FillNtupleIColumn(0,12,e.PEPMT1);
  a->FillNtupleIColumn(0,13,e.PEPMT2);
  a->FillNtupleIColumn(0,14,e.ScintPhotonsPMT1);
  a->FillNtupleIColumn(0,15,e.ScintPhotonsPMT2);
  a->FillNtupleIColumn(0,16,e.CerenkovPhotonsPMT1);
  a->FillNtupleIColumn(0,17,e.CerenkovPhotonsPMT2);
  a->FillNtupleIColumn(0,18,e.OtherOpticalPhotonsPMT1);
  a->FillNtupleIColumn(0,19,e.OtherOpticalPhotonsPMT2);
  a->FillNtupleIColumn(0,20,e.PrimaryVisitedTop);
  a->FillNtupleIColumn(0,21,e.PrimaryVisitedBottom);
  a->FillNtupleIColumn(0,22,e.PrimaryVisitedBoth);
  a->FillNtupleIColumn(0,23,e.PrimaryEnteredBlock);
  a->FillNtupleIColumn(0,24,e.PrimaryStoppedInBlock);
  a->FillNtupleIColumn(0,25,e.PrimaryBlockEntries);
  a->FillNtupleIColumn(0,26,e.PrimaryBlockExits);
  a->FillNtupleDColumn(0,27,e.MuonEnergy_GeV);
  a->FillNtupleDColumn(0,28,e.MuonMomentum_GeV);
  a->FillNtupleDColumn(0,29,e.MuonTheta_deg);
  a->FillNtupleDColumn(0,30,e.MuonPhi_deg);
  a->FillNtupleDColumn(0,31,e.PrimaryStartX_cm);
  a->FillNtupleDColumn(0,32,e.PrimaryStartY_cm);
  a->FillNtupleDColumn(0,33,e.PrimaryStartZ_cm);
  a->FillNtupleDColumn(0,34,e.PrimaryStartDirX);
  a->FillNtupleDColumn(0,35,e.PrimaryStartDirY);
  a->FillNtupleDColumn(0,36,e.PrimaryStartDirZ);
  a->FillNtupleDColumn(0,37,e.EdepTop_MeV);
  a->FillNtupleDColumn(0,38,e.EdepBottom_MeV);
  a->FillNtupleDColumn(0,39,e.EdepBlock_MeV);
  a->FillNtupleDColumn(0,40,e.FirstPETimePMT1_ns);
  a->FillNtupleDColumn(0,41,e.FirstPETimePMT2_ns);
  a->FillNtupleDColumn(0,42,e.LastPETimePMT1_ns);
  a->FillNtupleDColumn(0,43,e.LastPETimePMT2_ns);
  a->FillNtupleDColumn(0,44,e.MeanPETimePMT1_ns);
  a->FillNtupleDColumn(0,45,e.MeanPETimePMT2_ns);
  a->FillNtupleDColumn(0,46,e.RMSPETimePMT1_ns);
  a->FillNtupleDColumn(0,47,e.RMSPETimePMT2_ns);
  a->FillNtupleDColumn(0,48,e.DeltaT_ns);
  a->FillNtupleDColumn(0,49,e.TimeWindow_ns);
  a->FillNtupleDColumn(0,50,e.FirstPhotonTransitPMT1_ns);
  a->FillNtupleDColumn(0,51,e.FirstPhotonTransitPMT2_ns);
  a->FillNtupleDColumn(0,52,e.MeanPhotonTransitPMT1_ns);
  a->FillNtupleDColumn(0,53,e.MeanPhotonTransitPMT2_ns);
  a->FillNtupleDColumn(0,54,e.PrimaryEdepTop_MeV);
  a->FillNtupleDColumn(0,55,e.PrimaryEdepBottom_MeV);
  a->FillNtupleDColumn(0,56,e.PrimaryEdepBlock_MeV);
  a->FillNtupleDColumn(0,57,e.PrimaryPathTop_cm);
  a->FillNtupleDColumn(0,58,e.PrimaryPathBottom_cm);
  a->FillNtupleDColumn(0,59,e.PrimaryPathBlock_cm);
  a->FillNtupleDColumn(0,60,e.PrimaryBlockEntryKinetic_MeV);
  a->FillNtupleDColumn(0,61,e.PrimaryBlockExitKinetic_MeV);
  a->AddNtupleRow(0);
}
void RunAction::EndOfRunAction(const G4Run* run) {
  auto* a=G4AnalysisManager::Instance();
  const auto* d=static_cast<const DetectorConstruction*>(G4RunManager::GetRunManager()->GetUserDetectorConstruction());
  // In ROOT ntuple-merging mode the master must NOT fill an ntuple.
  if (!G4Threading::IsMultithreadedApplication() || G4Threading::IsWorkerThread()) {
    const G4int counts[]={2,run->GetRunID(),G4Threading::G4GetThreadId(),fEvents.GetValue(),fTop.GetValue(),fBottom.GetValue(),fCoincidences.GetValue(),d->GetBlockEnabled()?1:0,d->GetPEThreshold()};
    for(G4int i=0;i<9;++i)a->FillNtupleIColumn(1,i,counts[i]);
    const auto elapsed=std::chrono::duration<G4double>(std::chrono::steady_clock::now()-fStarted).count();
    const G4double values[]={d->GetBlockWidth()/cm,d->GetBlockHeight()/cm,d->GetBlockThickness()/cm,d->GetGap()/cm,
      d->GetUpperX()/cm,d->GetUpperY()/cm,0.5*(d->GetGap()/cm+1.4),d->GetLowerX()/cm,0.,-0.5*(d->GetGap()/cm+1.4),90.,d->GetTimeWindow()/ns,
      PrimaryGeneratorAction::SourcePMinGeV,PrimaryGeneratorAction::SourcePMaxGeV,PrimaryGeneratorAction::SourceZenithMaxDeg,
      PrimaryGeneratorAction::SourceHalfSizeCm,PrimaryGeneratorAction::SourceZCm,0.,d->GetUpperY()/cm,100.,1000.,elapsed};
    for(G4int i=0;i<22;++i)a->FillNtupleDColumn(1,9+i,values[i]);
    a->FillNtupleSColumn(1,31,"G4_Pb");
    a->FillNtupleSColumn(1,32,"modified_gaisser_legacy_logp_p_times_flux; finite source plane; not calibrated exposure time");
    a->FillNtupleSColumn(1,33,fInitialRandomState);
    a->FillNtupleDColumn(1,34,d->GetOverlapWidth()/cm);
    a->FillNtupleDColumn(1,35,10.);
    a->FillNtupleDColumn(1,36,10*d->GetOverlapWidth()/cm);
    a->FillNtupleDColumn(1,37,0.);
    a->FillNtupleDColumn(1,38,0.);
    a->FillNtupleDColumn(1,39,d->GetBlockZ()/cm);
    a->AddNtupleRow(1);
  }
  G4AccumulableManager::Instance()->Merge();
  if(IsMaster())G4cout << "COMPLETED RunID=" << run->GetRunID() << " events=" << fEvents.GetValue()
    << " top=" << fTop.GetValue() << " bottom=" << fBottom.GetValue() << " coincidence=" << fCoincidences.GetValue()
    << " ROOT=" << a->GetFileName() << G4endl;
  a->Write();a->CloseFile();
}
}
