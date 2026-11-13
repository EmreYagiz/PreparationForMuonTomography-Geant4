#include "EventAction.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4ParticleGun.hh"
#include "G4PrimaryVertex.hh"
#include "G4Threading.hh"
#include "G4SystemOfUnits.hh"
#include <cmath>
namespace B1 {
void EventAction::BeginOfEventAction(const G4Event*) {
  fRecord=EventRecord{};fPE1={};fPE2={};fPhotons1={};fPhotons2={};
  fRecord.PrimaryBlockEntryKinetic_MeV=-1.;fRecord.PrimaryBlockExitKinetic_MeV=-1.;
}
void EventAction::PrimaryVisit(G4bool top,G4bool bottom,G4bool block) {
  fRecord.PrimaryVisitedTop|=top;fRecord.PrimaryVisitedBottom|=bottom;fRecord.PrimaryEnteredBlock|=block;
}
void EventAction::PrimaryDeposit(G4int region,G4double energy,G4double length) {
  if(region==0){fRecord.PrimaryEdepTop_MeV+=energy/MeV;fRecord.PrimaryPathTop_cm+=length/cm;}
  if(region==1){fRecord.PrimaryEdepBottom_MeV+=energy/MeV;fRecord.PrimaryPathBottom_cm+=length/cm;}
  if(region==2){fRecord.PrimaryEdepBlock_MeV+=energy/MeV;fRecord.PrimaryPathBlock_cm+=length/cm;}
}
void EventAction::BlockEntry(G4double energy) {
  if(!fRecord.PrimaryBlockEntries)fRecord.PrimaryBlockEntryKinetic_MeV=energy/MeV;
  ++fRecord.PrimaryBlockEntries;fRecord.PrimaryEnteredBlock=1;
}
void EventAction::BlockExit(G4double energy) {
  if(!fRecord.PrimaryBlockExits)fRecord.PrimaryBlockExitKinetic_MeV=energy/MeV;
  ++fRecord.PrimaryBlockExits;
}
void EventAction::EndOfEventAction(const G4Event* event) {
  auto* manager=G4RunManager::GetRunManager();
  const auto* d=static_cast<const DetectorConstruction*>(manager->GetUserDetectorConstruction());
  const auto* source=static_cast<const PrimaryGeneratorAction*>(manager->GetUserPrimaryGeneratorAction());
  const auto* gun=source->GetParticleGun();
  auto& e=fRecord;
  e.RunID=manager->GetCurrentRun()->GetRunID();e.EventID=event->GetEventID();e.WorkerID=G4Threading::G4GetThreadId();
  e.BlockEnabled=d->GetBlockEnabled();e.MuonCharge=source->GetCurrentMuonCharge();
  e.MuonEnergy_GeV=source->GetCurrentMuonEnergy()/GeV;e.MuonMomentum_GeV=source->GetCurrentMuonMomentum()/GeV;e.MuonTheta_deg=source->GetCurrentMuonTheta()/deg;
  const auto position=event->GetPrimaryVertex()->GetPosition();const auto direction=gun->GetParticleMomentumDirection();
  e.MuonPhi_deg=std::atan2(direction.y(),direction.x())/deg;
  e.PrimaryStartX_cm=position.x()/cm;e.PrimaryStartY_cm=position.y()/cm;e.PrimaryStartZ_cm=position.z()/cm;
  e.PrimaryStartDirX=direction.x();e.PrimaryStartDirY=direction.y();e.PrimaryStartDirZ=direction.z();
  e.EdepTop_MeV/=MeV;e.EdepBottom_MeV/=MeV;e.EdepBlock_MeV/=MeV;
  e.PhotonsPMT1=fPhotons1.count;e.PhotonsPMT2=fPhotons2.count;e.PEPMT1=fPE1.count;e.PEPMT2=fPE2.count;
  e.PEThreshold=d->GetPEThreshold();e.TimeWindow_ns=d->GetTimeWindow()/ns;
  e.PMT1Hit=fPE1.count>=e.PEThreshold;e.PMT2Hit=fPE2.count>=e.PEThreshold;
  e.FirstPETimePMT1_ns=fPE1.first/ns;e.FirstPETimePMT2_ns=fPE2.first/ns;
  e.LastPETimePMT1_ns=fPE1.last/ns;e.LastPETimePMT2_ns=fPE2.last/ns;
  e.MeanPETimePMT1_ns=fPE1.Mean()/ns;e.MeanPETimePMT2_ns=fPE2.Mean()/ns;
  e.RMSPETimePMT1_ns=fPE1.RMS()/ns;e.RMSPETimePMT2_ns=fPE2.RMS()/ns;
  e.DeltaT_ns=(fPE1.count&&fPE2.count)?(fPE2.first-fPE1.first)/ns:-1.;
  e.TimeCoincidence=(fPE1.count&&fPE2.count&&std::abs(e.DeltaT_ns)<=e.TimeWindow_ns);
  e.Coincidence=IsCoincidence(fPE1.count,fPE2.count,fPE1.first,fPE2.first,e.PEThreshold,d->GetTimeWindow());
  e.FirstPhotonTransitPMT1_ns=fPhotons1.first/ns;e.FirstPhotonTransitPMT2_ns=fPhotons2.first/ns;
  e.MeanPhotonTransitPMT1_ns=fPhotons1.Mean()/ns;e.MeanPhotonTransitPMT2_ns=fPhotons2.Mean()/ns;
  e.PrimaryVisitedBoth=e.PrimaryVisitedTop&&e.PrimaryVisitedBottom;
  fRunAction->RecordEvent(e);
}
}
