#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include "G4RunManager.hh"

#include "G4Event.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"

#include <algorithm>
#include <cmath>

namespace B1
{
namespace
{
constexpr G4double kMinimumMomentumGeV = PrimaryGeneratorAction::SourcePMinGeV;
constexpr G4double kMaximumMomentumGeV = PrimaryGeneratorAction::SourcePMaxGeV;
constexpr G4double kMaximumZenithDeg   = PrimaryGeneratorAction::SourceZenithMaxDeg;
constexpr G4double kMuonMassGeV        = 0.1056583755;

// Gaisser parametrization parameters.
constexpr G4double kP1 =  0.102573;
constexpr G4double kP2 = -0.068287;
constexpr G4double kP3 =  0.958633;
constexpr G4double kP4 =  0.0407253;
constexpr G4double kP5 =  0.817285;

G4double EffectiveCosTheta(G4double theta)
{
  const G4double cosTheta = std::cos(theta);

  const G4double numerator =
      cosTheta * cosTheta
      + kP1 * kP1
      + kP2 * std::pow(cosTheta, kP3)
      + kP4 * std::pow(cosTheta, kP5);

  const G4double denominator =
      1.0 + kP1 * kP1 + kP2 + kP4;

  return std::sqrt(numerator / denominator);
}

// Modified Gaisser differential flux dI/dE.
G4double ModifiedGaisserFlux(G4double momentumGeV, G4double theta)
{
  if (momentumGeV <= 0.0) {
    return 0.0;
  }

  const G4double energyGeV =
      std::sqrt(momentumGeV * momentumGeV
                + kMuonMassGeV * kMuonMassGeV);

  const G4double cosThetaStar = EffectiveCosTheta(theta);
  if (cosThetaStar <= 0.0) {
    return 0.0;
  }

  const G4double lowEnergyCorrection =
      1.0
      + 3.64
      / (energyGeV * std::pow(cosThetaStar, 1.29));

  const G4double pionTerm =
      1.0
      / (1.0 + 1.1 * energyGeV * cosThetaStar / 115.0);

  const G4double kaonTerm =
      0.054
      / (1.0 + 1.1 * energyGeV * cosThetaStar / 850.0);

  return 0.14
       * std::pow(energyGeV * lowEnergyCorrection, -2.7)
       * (pionTerm + kaonTerm);
}

G4double SamplingWeight(G4double momentumGeV, G4double cosTheta)
{
  const G4double theta = std::acos(cosTheta);

  // Legacy B1 weighting restored at the user's request: p * flux * cos(theta).
  return momentumGeV
       * ModifiedGaisserFlux(momentumGeV, theta)
       * cosTheta;
}

G4double FindMaximumWeight()
{
  constexpr G4int nMomentumSteps = 1200;
  constexpr G4int nAngleSteps = 500;

  const G4double logPMin = std::log(kMinimumMomentumGeV);
  const G4double logPMax = std::log(kMaximumMomentumGeV);
  const G4double cosThetaMin =
      std::cos(kMaximumZenithDeg * deg);

  G4double maximum = 0.0;

  for (G4int ip = 0; ip <= nMomentumSteps; ++ip) {
    const G4double fractionP =
        static_cast<G4double>(ip) / nMomentumSteps;

    const G4double momentumGeV =
        std::exp(logPMin + fractionP * (logPMax - logPMin));

    for (G4int it = 0; it <= nAngleSteps; ++it) {
      const G4double fractionTheta =
          static_cast<G4double>(it) / nAngleSteps;

      const G4double cosTheta =
          cosThetaMin + fractionTheta * (1.0 - cosThetaMin);

      maximum = std::max(
          maximum,
          SamplingWeight(momentumGeV, cosTheta));
    }
  }

  return 1.01 * maximum;
}

void GenerateMomentumAndDirection(
    G4double& momentumGeV,
    G4ThreeVector& direction)
{
  static thread_local const G4double maximumWeight =
      FindMaximumWeight();

  const G4double logPMin = std::log(kMinimumMomentumGeV);
  const G4double logPMax = std::log(kMaximumMomentumGeV);
  const G4double cosThetaMin =
      std::cos(kMaximumZenithDeg * deg);

  while (true) {
    const G4double logMomentum =
        logPMin + G4UniformRand() * (logPMax - logPMin);

    const G4double trialMomentumGeV =
        std::exp(logMomentum);

    const G4double cosTheta =
        cosThetaMin + G4UniformRand() * (1.0 - cosThetaMin);

    const G4double weight =
        SamplingWeight(trialMomentumGeV, cosTheta);

    if (G4UniformRand() * maximumWeight > weight) {
      continue;
    }

    const G4double sinTheta =
        std::sqrt(std::max(0.0, 1.0 - cosTheta * cosTheta));

    const G4double phi =
        twopi * G4UniformRand();

    direction = G4ThreeVector(
        sinTheta * std::cos(phi),
        sinTheta * std::sin(phi),
        -cosTheta);

    momentumGeV = trialMomentumGeV;
    return;
  }
}
}  // namespace

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  fParticleGun = new G4ParticleGun(1);

  auto particleDefinition =
      G4ParticleTable::GetParticleTable()->FindParticle("mu-");

  fParticleGun->SetParticleDefinition(particleDefinition);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  // Uniform source plane above the telescope. Geant4 internal length units
  // are used directly; no external-generator unit conversion is involved.
  constexpr G4double sourceHalfSizeX = SourceHalfSizeCm * cm;
  constexpr G4double sourceHalfSizeY = SourceHalfSizeCm * cm;
  constexpr G4double sourceZ = SourceZCm * cm;

  const auto* detector=static_cast<const DetectorConstruction*>(G4RunManager::GetRunManager()->GetUserDetectorConstruction());
  const G4double sourceX =
      (2.0 * G4UniformRand() - 1.0) * sourceHalfSizeX;
  const G4double sourceY =
      detector->GetUpperY() + (2.0 * G4UniformRand() - 1.0) * sourceHalfSizeY;

  fParticleGun->SetParticlePosition(
      G4ThreeVector(sourceX, sourceY, sourceZ));

  // Approximate sea-level charge ratio mu+/mu- = 1.28.
  constexpr G4double positiveMuonFraction = 1.28 / (1.0 + 1.28);
  const G4bool isPositive =
      (G4UniformRand() < positiveMuonFraction);

  fCurrentMuonCharge = isPositive ? +1 : -1;

  const char* particleName = isPositive ? "mu+" : "mu-";
  auto particleDefinition =
      G4ParticleTable::GetParticleTable()->FindParticle(particleName);

  fParticleGun->SetParticleDefinition(particleDefinition);

  G4double momentumGeV = 0.0;
  G4ThreeVector direction;
  GenerateMomentumAndDirection(momentumGeV, direction);

  fParticleGun->SetParticleMomentumDirection(direction);

  const G4double momentum = momentumGeV * GeV;
  const G4double mass = particleDefinition->GetPDGMass();
  const G4double kineticEnergy =
      std::sqrt(momentum * momentum + mass * mass) - mass;

  fCurrentMuonEnergy = kineticEnergy;
  fCurrentMuonMomentum = momentum;
  fCurrentMuonTheta = std::acos(-direction.z());

  fParticleGun->SetParticleEnergy(kineticEnergy);
  fParticleGun->GeneratePrimaryVertex(anEvent);
}

}  // namespace B1
