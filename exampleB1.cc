#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "G4OpticalPhysics.hh"
#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "QBBC.hh"
#include <string>
int main(int argc,char** argv) {
  if(argc!=2||std::string(argv[1])=="--help") {
    G4cout << "Usage: ./exampleB1 reference.mac OR ./exampleB1 run_block.mac" << G4endl;
    G4cout << "Batch only; ROOT output; no viewer, scan or automatic analysis." << G4endl;
    return argc==2?0:2;
  }
  auto* run=G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);
  auto* detector=new B1::DetectorConstruction;
  run->SetUserInitialization(detector);
  auto* physics=new QBBC;physics->RegisterPhysics(new G4OpticalPhysics);
  run->SetUserInitialization(physics);
  run->SetUserInitialization(new B1::ActionInitialization(detector));
  const auto status=G4UImanager::GetUIpointer()->ApplyCommand(G4String("/control/execute ")+argv[1]);
  delete run;
  return status==0?0:1;
}
