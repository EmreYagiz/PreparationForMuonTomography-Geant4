# Preparation for Muon Tomography Using Geant4

## Overview

This project focuses on the development and simulation of a cosmic-ray muon detection system as a preliminary study for muon tomography.

The simulation is implemented using **Geant4** and is based on a modified version of the Geant4 Basic Example B1. The main objective is to investigate how detector geometry, detector separation, and the presence of high-density materials affect muon detection and coincidence rates.

These studies aim to provide a foundation for future muon tomography experiments.

## Detector Configuration

The simulated detection system consists of two plastic scintillator detectors coupled to photomultiplier tubes (PMTs).

The simulation includes:

- Plastic scintillator detectors based on BC-408
- Optical photon generation and transport
- PMT response and photoelectron detection
- Cosmic-ray muon generation
- Energy deposition and coincidence detection

## Simulation Studies

### 1. Detector Geometry and Separation

Different detector geometries and separation distances are investigated to understand their effects on muon detection efficiency and coincidence rates.

The simulations examine how changes in detector positioning and geometry influence the number of muons detected simultaneously by both scintillators.

### 2. Lead Block Measurements

Lead blocks are introduced between the two scintillator detectors to investigate how high-density materials affect the detected muon coincidence rate.

Different geometrical configurations and detector-to-block distances are studied to evaluate the response of the detection system.

### 3. Coincidence Analysis

Coincidence events are identified when both scintillator detectors register signals associated with the same simulated muon event.

The analysis focuses on:

- Single-detector detection rates
- Two-detector coincidence rates
- Effects of detector separation
- Effects of lead block placement and geometry
- Comparison of different simulation configurations

## Software and Tools

- **Geant4:** Particle transport and detector simulation
- **C++:** Simulation development and implementation
- **ROOT:** Data analysis and visualization
- **CMake:** Build configuration

## Project Structure

- `src/` — Geant4 source files
- `include/` — Header files
- `*.mac` — Simulation configuration macros
- `*.sh` — Scripts for automated simulation runs
- `*.C` — ROOT analysis macros

## Future Work

The next stage of this research involves extending the detector simulations toward muon tomography applications, including the investigation of muon interactions with different materials and the development of methods for identifying material distributions through detector measurements.

## Research Context

This simulation study is part of an undergraduate research project at **Middle East Technical University (METU)** focusing on cosmic-ray muon detection and the development of a muon tomography system.
