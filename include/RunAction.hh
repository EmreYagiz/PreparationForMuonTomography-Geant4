#ifndef B1RunAction_h
#define B1RunAction_h 1
#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"
#include <chrono>
namespace B1 {
// One entry for EVERY generated event, including zero-hit events.
// Primary* fields are simulation diagnostics, not measured PMT observables.
struct EventRecord {
  G4int RunID=0;
  G4int EventID=0;
  G4int WorkerID=0;
  G4int BlockEnabled=0;
  G4int MuonCharge=0;
  G4int Coincidence=0;
  G4int PMT1Hit=0;
  G4int PMT2Hit=0;
  G4int PEThreshold=0;
  G4int TimeCoincidence=0;
  G4int PhotonsPMT1=0;
  G4int PhotonsPMT2=0;
  G4int PEPMT1=0;
  G4int PEPMT2=0;
  G4int ScintPhotonsPMT1=0;
  G4int ScintPhotonsPMT2=0;
  G4int CerenkovPhotonsPMT1=0;
  G4int CerenkovPhotonsPMT2=0;
  G4int OtherOpticalPhotonsPMT1=0;
  G4int OtherOpticalPhotonsPMT2=0;
  G4int PrimaryVisitedTop=0;
  G4int PrimaryVisitedBottom=0;
  G4int PrimaryVisitedBoth=0;
  G4int PrimaryEnteredBlock=0;
  G4int PrimaryStoppedInBlock=0;
  G4int PrimaryBlockEntries=0;
  G4int PrimaryBlockExits=0;
  G4double MuonEnergy_GeV=0.;
  G4double MuonMomentum_GeV=0.;
  G4double MuonTheta_deg=0.;
  G4double MuonPhi_deg=0.;
  G4double PrimaryStartX_cm=0.;
  G4double PrimaryStartY_cm=0.;
  G4double PrimaryStartZ_cm=0.;
  G4double PrimaryStartDirX=0.;
  G4double PrimaryStartDirY=0.;
  G4double PrimaryStartDirZ=0.;
  G4double EdepTop_MeV=0.;
  G4double EdepBottom_MeV=0.;
  G4double EdepBlock_MeV=0.;
  G4double FirstPETimePMT1_ns=0.;
  G4double FirstPETimePMT2_ns=0.;
  G4double LastPETimePMT1_ns=0.;
  G4double LastPETimePMT2_ns=0.;
  G4double MeanPETimePMT1_ns=0.;
  G4double MeanPETimePMT2_ns=0.;
  G4double RMSPETimePMT1_ns=0.;
  G4double RMSPETimePMT2_ns=0.;
  G4double DeltaT_ns=0.;
  G4double TimeWindow_ns=0.;
  G4double FirstPhotonTransitPMT1_ns=0.;
  G4double FirstPhotonTransitPMT2_ns=0.;
  G4double MeanPhotonTransitPMT1_ns=0.;
  G4double MeanPhotonTransitPMT2_ns=0.;
  G4double PrimaryEdepTop_MeV=0.;
  G4double PrimaryEdepBottom_MeV=0.;
  G4double PrimaryEdepBlock_MeV=0.;
  G4double PrimaryPathTop_cm=0.;
  G4double PrimaryPathBottom_cm=0.;
  G4double PrimaryPathBlock_cm=0.;
  G4double PrimaryBlockEntryKinetic_MeV=0.;
  G4double PrimaryBlockExitKinetic_MeV=0.;
};
class RunAction : public G4UserRunAction {
 public:
  RunAction();
  void BeginOfRunAction(const G4Run*) override;
  void EndOfRunAction(const G4Run*) override;
  void RecordEvent(const EventRecord&);
 private:
  G4Accumulable<G4int> fEvents=0,fTop=0,fBottom=0,fCoincidences=0;
  std::chrono::steady_clock::time_point fStarted;
  G4String fInitialRandomState;
};
}
#endif
