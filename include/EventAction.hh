#ifndef B1EventAction_h
#define B1EventAction_h 1
#include "G4UserEventAction.hh"
#include "RunAction.hh"
#include <algorithm>
#include <cmath>
namespace B1 {
inline G4bool IsCoincidence(G4int n1,G4int n2,G4double first1,G4double first2,
                            G4int threshold,G4double window) {
  return n1>=threshold && n2>=threshold && first1>=0. && first2>=0.
         && std::abs(first2-first1)<=window;
}
struct ArrivalStats {
  G4int count=0;
  G4double first=-1,last=-1,sum=0,sum2=0;
  void Add(G4double t) { if(count==0||t<first)first=t;if(count==0||t>last)last=t;++count;sum+=t;sum2+=t*t; }
  G4double Mean() const {return count?sum/count:-1.;}
  G4double RMS() const {return count?std::sqrt(std::max(0.,sum2/count-Mean()*Mean())):-1.;}
};
class EventAction : public G4UserEventAction {
 public:
  explicit EventAction(RunAction* run):fRunAction(run){}
  void BeginOfEventAction(const G4Event*) override;
  void EndOfEventAction(const G4Event*) override;
  void AddEdep1(G4double e){fRecord.EdepTop_MeV+=e;}
  void AddEdep2(G4double e){fRecord.EdepBottom_MeV+=e;}
  void AddBlockEdep(G4double e){fRecord.EdepBlock_MeV+=e;}
  void AddPhotonPMT1(G4double t){fPhotons1.Add(t);}
  void AddPhotonPMT2(G4double t){fPhotons2.Add(t);}
  void AddPEPMT1(G4double t){fPE1.Add(t);}
  void AddPEPMT2(G4double t){fPE2.Add(t);}
  void AddScintPhotonPMT1(){++fRecord.ScintPhotonsPMT1;}
  void AddScintPhotonPMT2(){++fRecord.ScintPhotonsPMT2;}
  void AddCerenkovPhotonPMT1(){++fRecord.CerenkovPhotonsPMT1;}
  void AddCerenkovPhotonPMT2(){++fRecord.CerenkovPhotonsPMT2;}
  void AddOtherOpticalPhotonPMT1(){++fRecord.OtherOpticalPhotonsPMT1;}
  void AddOtherOpticalPhotonPMT2(){++fRecord.OtherOpticalPhotonsPMT2;}
  void PrimaryVisit(G4bool top,G4bool bottom,G4bool block);
  void PrimaryDeposit(G4int region,G4double energy,G4double length);
  void BlockEntry(G4double energy);
  void BlockExit(G4double energy);
  void StopInBlock(){fRecord.PrimaryStoppedInBlock=1;}
 private:
  RunAction* fRunAction;
  EventRecord fRecord;
  ArrivalStats fPE1,fPE2,fPhotons1,fPhotons2;
};
}
#endif
