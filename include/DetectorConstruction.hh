#ifndef B1DetectorConstruction_h
#define B1DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"

class G4GenericMessenger;
class G4VPhysicalVolume;
class G4LogicalVolume;

namespace B1
{

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;

    G4LogicalVolume* GetScintillator1Volume() const { return fScintillator1Volume; }
    G4LogicalVolume* GetScintillator2Volume() const { return fScintillator2Volume; }

    G4LogicalVolume* GetPMT1Volume() const { return fPMT1Volume; }
    G4LogicalVolume* GetPMT2Volume() const { return fPMT2Volume; }

    G4LogicalVolume* GetBlockVolume() const { return fBlockVolume; }
    G4bool GetBlockEnabled() const { return fBlockEnabled; }
    G4double GetBlockWidth() const { return fBlockWidth; }
    G4double GetBlockHeight() const { return fBlockHeight; }
    G4double GetBlockThickness() const { return fBlockThickness; }
    G4double GetBlockZ() const { return fBlockZ; }
    G4double GetGap() const { return fGap; }
    G4double GetUpperX() const { return fUpperX; }
    G4double GetOverlapWidth() const { return fOverlapWidth; }
    G4double GetUpperY() const { return fUpperY; }
    G4double GetLowerX() const { return fLowerX; }
    G4int GetPEThreshold() const { return fPEThreshold; }
    G4double GetTimeWindow() const { return fTimeWindow; }

  private:
    G4GenericMessenger* fMessenger = nullptr;
    G4bool fBlockEnabled = true;
    G4double fBlockWidth, fBlockHeight, fBlockThickness, fGap;
    G4double fBlockZ = 0.;
    G4double fUpperX = 0.;
    G4double fOverlapWidth;
    G4double fUpperY, fLowerX, fTimeWindow;
    G4int fPEThreshold = 1;
    G4LogicalVolume* fBlockVolume = nullptr;
    G4LogicalVolume* fScintillator1Volume = nullptr;
    G4LogicalVolume* fScintillator2Volume = nullptr;

    G4LogicalVolume* fPMT1Volume = nullptr;
    G4LogicalVolume* fPMT2Volume = nullptr;
};

}  // namespace B1

#endif
