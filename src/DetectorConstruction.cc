#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4ExtrudedSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4OpticalSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4TwoVector.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4RotationMatrix.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include <vector>
#include <cmath>
#include "G4GenericMessenger.hh"
#include "G4StateManager.hh"
#include "G4Exception.hh"
#include "G4Transform3D.hh"

namespace B1
{

DetectorConstruction::DetectorConstruction()
 : fBlockWidth(5*cm),fBlockHeight(5*cm),fBlockThickness(5*cm),fGap(30*cm),
   fOverlapWidth(10*cm),fUpperY(4.3*cm),fLowerX(-4.3*cm),fTimeWindow(15*ns)
{
  fMessenger=new G4GenericMessenger(this,"/setup/","Fixed crossed-scintillator coincidence setup");
  fMessenger->DeclareProperty("blockEnabled",fBlockEnabled).SetStates(G4State_PreInit);
  fMessenger->DeclarePropertyWithUnit("blockWidth","cm",fBlockWidth).SetStates(G4State_PreInit);
  fMessenger->DeclarePropertyWithUnit("blockHeight","cm",fBlockHeight).SetStates(G4State_PreInit);
  fMessenger->DeclarePropertyWithUnit("blockThickness","cm",fBlockThickness).SetStates(G4State_PreInit);
  fMessenger->DeclarePropertyWithUnit("blockZ","cm",fBlockZ).SetStates(G4State_PreInit);
  fMessenger->DeclarePropertyWithUnit("gap","cm",fGap).SetStates(G4State_PreInit);
  fMessenger->DeclarePropertyWithUnit("overlapWidth","cm",fOverlapWidth).SetStates(G4State_PreInit);
  fMessenger->DeclareProperty("peThreshold",fPEThreshold).SetStates(G4State_PreInit);
  fMessenger->DeclarePropertyWithUnit("timeWindow","ns",fTimeWindow).SetStates(G4State_PreInit);
}
DetectorConstruction::~DetectorConstruction() { delete fMessenger; }

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  if (!std::isfinite(fOverlapWidth) || fOverlapWidth<=0 || fOverlapWidth>10*cm ||
      !std::isfinite(fGap) || fGap<=0.2*cm || fGap>100*cm ||
      !std::isfinite(fBlockWidth) || fBlockWidth<=0 || fBlockWidth>=100*cm ||
      !std::isfinite(fBlockHeight) || fBlockHeight<=0 || fBlockHeight>=100*cm ||
      !std::isfinite(fBlockThickness) || fBlockThickness<=0 ||
      !std::isfinite(fBlockZ) ||
      (fBlockEnabled && std::abs(fBlockZ)+fBlockThickness/2>=fGap/2-0.1005*cm) ||
      fPEThreshold<1 || !std::isfinite(fTimeWindow) || fTimeWindow<=0) {
    G4Exception("DetectorConstruction","SetupBounds",FatalException,"Invalid block, gap or coincidence configuration.");
  }
  // Symmetric horizontal shift preserves the common-area center at the block.
  const G4double projectedShift=(10*cm-fOverlapWidth)/2;
  fUpperX=projectedShift;
  fLowerX=-4.3*cm-projectedShift;
  G4cout << "PROJECTED_OVERLAP x_cm=" << fOverlapWidth/cm << " y_cm=10 area_cm2="
         << 10*fOverlapWidth/cm << " upper_x_cm=" << fUpperX/cm << G4endl;
  G4cout << "FIXED_BLOCK_SETUP enabled=" << fBlockEnabled << " size_cm="
         << fBlockWidth/cm << "," << fBlockHeight/cm << "," << fBlockThickness/cm
         << " block_z_cm=" << fBlockZ/cm
         << " gap_cm=" << fGap/cm << " lower_rotation_deg=90 upper_y_cm="
         << fUpperY/cm << " lower_x_cm=" << fLowerX/cm << G4endl;
  auto nist = G4NistManager::Instance();

  auto worldMat  = nist->FindOrBuildMaterial("G4_AIR");
  auto scintMat  = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
  auto teflonMat = nist->FindOrBuildMaterial("G4_TEFLON");
  auto pmtMat    = nist->FindOrBuildMaterial("G4_Pyrex_Glass");

  const G4int nEntries = 8;

  G4double photonEnergy[nEntries] =
  {
    2.48 * eV,  // 500 nm
    2.58 * eV,  // 480 nm
    2.70 * eV,  // 460 nm
    2.82 * eV,  // 440 nm
    2.92 * eV,  // 425 nm
    3.02 * eV,  // 410 nm
    3.10 * eV,  // 400 nm
    3.26 * eV   // 380 nm
  };

  // Air optical properties
  // Needed for optical boundary processes at air/material interfaces.

  G4double airRIndex[nEntries] =
  {
    1.0003, 1.0003, 1.0003, 1.0003,
    1.0003, 1.0003, 1.0003, 1.0003
  };

  auto airMPT = new G4MaterialPropertiesTable();
  airMPT->AddProperty("RINDEX", photonEnergy, airRIndex, nEntries);
  worldMat->SetMaterialPropertiesTable(airMPT);


  // Teflon material optical properties
  // This is the refractive index of the Teflon material itself.
  // The surface reflectivity is still defined separately below.

  G4double teflonRIndex[nEntries] =
  {
    1.35, 1.35, 1.35, 1.35,
    1.35, 1.35, 1.35, 1.35
  };

  auto teflonMPT = new G4MaterialPropertiesTable();
  teflonMPT->AddProperty("RINDEX", photonEnergy, teflonRIndex, nEntries);
  teflonMat->SetMaterialPropertiesTable(teflonMPT);

  // Scintillator optical properties

  G4double scintRIndex[nEntries]    = {1.58, 1.58, 1.58, 1.58, 1.58, 1.58, 1.58, 1.58};
  G4double scintAbsLength[nEntries] = {210.0 * cm, 210.0 * cm, 210.0 * cm, 210.0 * cm, 210.0 * cm, 210.0 * cm, 210.0 * cm, 210.0 * cm};
  G4double scintEmission[nEntries] = {0.05, 0.20, 0.45, 0.80, 1.00, 0.75, 0.45, 0.10};

  auto scintMPT = new G4MaterialPropertiesTable();
  scintMPT->AddProperty("RINDEX", photonEnergy, scintRIndex, nEntries);
  scintMPT->AddProperty("ABSLENGTH", photonEnergy, scintAbsLength, nEntries);
  scintMPT->AddProperty("SCINTILLATIONCOMPONENT1", photonEnergy, scintEmission, nEntries);

  scintMPT->AddConstProperty("SCINTILLATIONYIELD", 10400. / MeV);
  scintMPT->AddConstProperty("RESOLUTIONSCALE", 1.0);
  scintMPT->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.1 * ns);
  scintMPT->AddConstProperty("SCINTILLATIONYIELD1", 1.0);

  scintMat->SetMaterialPropertiesTable(scintMPT);

  // PMT material optical properties

  G4double pmtRIndex[nEntries] = {1.47, 1.47, 1.47, 1.47, 1.47, 1.47, 1.47, 1.47};

  auto pmtMPT = new G4MaterialPropertiesTable();
  pmtMPT->AddProperty("RINDEX", photonEnergy, pmtRIndex, nEntries);
  pmtMat->SetMaterialPropertiesTable(pmtMPT);

  // Teflon optical surface

  G4double teflonReflectivity[nEntries] = {0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95};

  G4double teflonEfficiency[nEntries]   = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  auto teflonSurface = new G4OpticalSurface("TeflonSurface");
  teflonSurface->SetType(dielectric_dielectric);
  teflonSurface->SetFinish(groundfrontpainted);
  teflonSurface->SetModel(unified);

  auto teflonSurfaceMPT = new G4MaterialPropertiesTable();
  teflonSurfaceMPT->AddProperty("REFLECTIVITY", photonEnergy, teflonReflectivity, nEntries);
  teflonSurfaceMPT->AddProperty("EFFICIENCY", photonEnergy, teflonEfficiency, nEntries);
  teflonSurface->SetMaterialPropertiesTable(teflonSurfaceMPT);

  // H10721-110 PMT detection and its wavelength-dependent QE are handled
  // manually in SteppingAction. Therefore no PMT optical detection skin
  // surface is attached here; this prevents applying QE twice.

  // Geometry dimensions

  const G4double totalLength    = 18.6 * cm;
  const G4double bodyLength     = 13.7 * cm;
  const G4double bottomWidth    = 10.0 * cm;
  const G4double topWidth       = 4.0  * cm;
  const G4double thickness      = 1.4  * cm;

  const G4double teflonThickness = 1.0 * mm;

  const G4double gapBetweenSurfaces = fGap;
  const G4double centerToCenter = gapBetweenSurfaces + thickness;
  const G4double upperZpos = +0.5 * centerToCenter;
  const G4double lowerZpos = -0.5 * centerToCenter;
  G4RotationMatrix lowerActiveRotation;
  lowerActiveRotation.rotateZ(90*deg);
  const G4Transform3D lowerTransform(lowerActiveRotation,G4ThreeVector(fLowerX,0.,lowerZpos));

  const G4double pmtRadius    = 4.0 * mm;
  const G4double pmtThickness = 2.0 * mm;

  // World

  auto solidWorld = new G4Box("World", 60.0 * cm, 60.0 * cm, 80.0 * cm);

  auto logicWorld = new G4LogicalVolume(solidWorld, worldMat, "World");

  auto physWorld = new G4PVPlacement(nullptr,
                                     G4ThreeVector(),
                                     logicWorld,
                                     "World",
                                     nullptr,
                                     false,
                                     0,
                                     true);

  auto worldVis = new G4VisAttributes();
  worldVis->SetVisibility(false);
  logicWorld->SetVisAttributes(worldVis);

  // --------------------
  // Scintillator shape
  // --------------------

  const G4double yBottom   = -0.5 * totalLength;
  const G4double yShoulder = yBottom + bodyLength;
  const G4double yTop      = +0.5 * totalLength;

  std::vector<G4TwoVector> scintShape;

  scintShape.push_back(G4TwoVector(-0.5 * bottomWidth, yBottom));
  scintShape.push_back(G4TwoVector( 0.5 * bottomWidth, yBottom));
  scintShape.push_back(G4TwoVector( 0.5 * bottomWidth, yShoulder));
  scintShape.push_back(G4TwoVector( 0.5 * topWidth,    yTop));
  scintShape.push_back(G4TwoVector(-0.5 * topWidth,    yTop));
  scintShape.push_back(G4TwoVector(-0.5 * bottomWidth, yShoulder));

  auto solidScint = new G4ExtrudedSolid("ScintillatorSolid",
                                        scintShape,
                                        0.5 * thickness,
                                        G4TwoVector(0., 0.),
                                        1.0,
                                        G4TwoVector(0., 0.),
                                        1.0);

  fScintillator1Volume = new G4LogicalVolume(solidScint, scintMat, "Scintillator1");
  fScintillator2Volume = new G4LogicalVolume(solidScint, scintMat, "Scintillator2");

  auto scintVis = new G4VisAttributes(G4Colour(0.0, 0.8, 0.2, 0.65));
  scintVis->SetForceSolid(true);
  fScintillator1Volume->SetVisAttributes(scintVis);
  fScintillator2Volume->SetVisAttributes(scintVis);

  new G4PVPlacement(nullptr,
                    G4ThreeVector(fUpperX, fUpperY, upperZpos),
                    fScintillator1Volume,
                    "Scintillator1",
                    logicWorld,
                    false,
                    0,
                    true);

  new G4PVPlacement(lowerTransform,
                    fScintillator2Volume,
                    "Scintillator2",
                    logicWorld,
                    false,
                    1,
                    true);

  // --------------------
  // Teflon shell
  // --------------------

  const G4double t = teflonThickness;

  std::vector<G4TwoVector> teflonOuterShape;

  teflonOuterShape.push_back(G4TwoVector(-0.5 * bottomWidth - t, yBottom - t));
  teflonOuterShape.push_back(G4TwoVector( 0.5 * bottomWidth + t, yBottom - t));
  teflonOuterShape.push_back(G4TwoVector( 0.5 * bottomWidth + t, yShoulder));
  teflonOuterShape.push_back(G4TwoVector( 0.5 * topWidth + t,    yTop + t));
  teflonOuterShape.push_back(G4TwoVector(-0.5 * topWidth - t,    yTop + t));
  teflonOuterShape.push_back(G4TwoVector(-0.5 * bottomWidth - t, yShoulder));

  auto solidTeflonOuter = new G4ExtrudedSolid("TeflonOuterSolid",
                                             teflonOuterShape,
                                             0.5 * (thickness + 2.0 * t),
                                             G4TwoVector(0., 0.),
                                             1.0,
                                             G4TwoVector(0., 0.),
                                             1.0);

  // A tiny numerical clearance avoids coincident surfaces without creating
  // a physically significant air layer between scintillator and Teflon.
  const G4double geometryTolerance = 1.0 * um;

  // Enlarge the subtraction cut very slightly in x, y and z. This avoids
  // coincident scintillator/Teflon surfaces while keeping the clearance far
  // below any physically relevant optical length scale.
  std::vector<G4TwoVector> teflonInnerShape;
  teflonInnerShape.push_back(
      G4TwoVector(-0.5 * bottomWidth - geometryTolerance,
                  yBottom - geometryTolerance));
  teflonInnerShape.push_back(
      G4TwoVector( 0.5 * bottomWidth + geometryTolerance,
                  yBottom - geometryTolerance));
  teflonInnerShape.push_back(
      G4TwoVector( 0.5 * bottomWidth + geometryTolerance,
                  yShoulder));
  teflonInnerShape.push_back(
      G4TwoVector( 0.5 * topWidth + geometryTolerance,
                  yTop + geometryTolerance));
  teflonInnerShape.push_back(
      G4TwoVector(-0.5 * topWidth - geometryTolerance,
                  yTop + geometryTolerance));
  teflonInnerShape.push_back(
      G4TwoVector(-0.5 * bottomWidth - geometryTolerance,
                  yShoulder));

  auto solidTeflonInner = new G4ExtrudedSolid("TeflonInnerCut",
                                             teflonInnerShape,
                                             0.5 * thickness + geometryTolerance,
                                             G4TwoVector(0., 0.),
                                             1.0,
                                             G4TwoVector(0., 0.),
                                             1.0);

  auto solidTeflonShell = new G4SubtractionSolid("TeflonShellSolid",
                                                solidTeflonOuter,
                                                solidTeflonInner,
                                                nullptr,
                                                G4ThreeVector());

  // Open a circular hole in the Teflon cap where the PMT active window is coupled.
  // Without this cut, the Teflon wrapper overlaps the PMT entrance and blocks the optical path.
  auto pmtHoleRotation = new G4RotationMatrix();
  pmtHoleRotation->rotateX(90.0 * deg);

  auto solidPMTHole = new G4Tubs("PMTHole",
                                 0.0,
                                 pmtRadius + geometryTolerance,
                                 2.0 * teflonThickness,
                                 0.0,
                                 360.0 * deg);

  auto solidTeflonShellWithPMTHole =
      new G4SubtractionSolid("TeflonShellWithPMTHole",
                             solidTeflonShell,
                             solidPMTHole,
                             pmtHoleRotation,
                             G4ThreeVector(0., yTop + 0.5 * teflonThickness, 0.));

  auto logicTeflon1 = new G4LogicalVolume(solidTeflonShellWithPMTHole, teflonMat, "TeflonShell1");
  auto logicTeflon2 = new G4LogicalVolume(solidTeflonShellWithPMTHole, teflonMat, "TeflonShell2");

  auto teflonVis = new G4VisAttributes(G4Colour::White());
  teflonVis->SetForceSolid(true);
  logicTeflon1->SetVisAttributes(teflonVis);
  logicTeflon2->SetVisAttributes(teflonVis);

  new G4LogicalSkinSurface("TeflonSkin1", logicTeflon1, teflonSurface);
  new G4LogicalSkinSurface("TeflonSkin2", logicTeflon2, teflonSurface);

  new G4PVPlacement(nullptr,
                    G4ThreeVector(fUpperX, fUpperY, upperZpos),
                    logicTeflon1,
                    "TeflonShell1",
                    logicWorld,
                    false,
                    0,
                    true);

  new G4PVPlacement(lowerTransform,
                    logicTeflon2,
                    "TeflonShell2",
                    logicWorld,
                    false,
                    1,
                    true);

  // --------------------
  // PMT active windows
  // --------------------

  auto solidPMT = new G4Tubs("PMTWindowSolid",
                             0.0,
                             pmtRadius,
                             0.5 * pmtThickness,
                             0.0,
                             360.0 * deg);

  fPMT1Volume = new G4LogicalVolume(solidPMT, pmtMat, "PMT1");
  fPMT2Volume = new G4LogicalVolume(solidPMT, pmtMat, "PMT2");

  auto pmtVis = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));
  pmtVis->SetForceSolid(true);
  fPMT1Volume->SetVisAttributes(pmtVis);
  fPMT2Volume->SetVisAttributes(pmtVis);

  auto pmtRotation = new G4RotationMatrix();
  pmtRotation->rotateX(90.0 * deg);

  // The PMT entrance face is placed directly against the scintillator end.
  const G4double pmtY = yTop + 0.5 * pmtThickness;

  new G4PVPlacement(pmtRotation,
                    G4ThreeVector(fUpperX, fUpperY+pmtY, upperZpos),
                    fPMT1Volume,
                    "PMT1",
                    logicWorld,
                    false,
                    0,
                    true);

  new G4PVPlacement(lowerTransform*G4Transform3D(pmtRotation->inverse(),G4ThreeVector(0.,pmtY,0.)),
                    fPMT2Volume,
                    "PMT2",
                    logicWorld,
                    false,
                    1,
                    true);

  fBlockVolume=nullptr;
  if (fBlockEnabled) {
    auto blockSolid=new G4Box("LeadBlockSolid",fBlockWidth/2,fBlockHeight/2,fBlockThickness/2);
    fBlockVolume=new G4LogicalVolume(blockSolid,nist->FindOrBuildMaterial("G4_Pb"),"LeadBlock");
    new G4PVPlacement(nullptr,G4ThreeVector(0.,0.,fBlockZ),fBlockVolume,"LeadBlock",logicWorld,false,0,true);
  }
  return physWorld;
}

} // namespace B1
