// Phase 2 demo: generate a full Walker-Delta satellite constellation and
// a couple of ground stations, and print a snapshot of where everything is.
//
// Build & run (from repo root):
//   cmake -B build && cmake --build build
//   ./build/constellation_demo

#include "constellation/WalkerConstellation.hpp"
#include "constellation/GroundStation.hpp"

#include <iostream>
#include <iomanip>

using namespace satsim;

int main() {
    // A small Starlink-like shell: 60 satellites across 12 planes at
    // 550 km altitude, 53 degree inclination.
    WalkerConstellationParams params;
    params.inclinationRad = 53.0 * constants::kPi / 180.0;
    params.totalSatellites = 60;
    params.numPlanes = 12;
    params.phasingFactor = 1;
    params.altitudeKm = 550.0;
    params.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);

    WalkerConstellation constellation(params);

    std::cout << "Generated constellation: " << constellation.size()
              << " satellites across " << params.numPlanes << " planes\n\n";

    // A couple of real ground stations (lat/lon in degrees, converted to radians).
    auto deg2rad = [](double d) { return d * constants::kPi / 180.0; };
    std::vector<GroundStation> groundStations = {
        {"Los Angeles", deg2rad(34.05), deg2rad(-118.24), 0.09},
        {"London",      deg2rad(51.51), deg2rad(-0.13),   0.02},
        {"Singapore",   deg2rad(1.35),  deg2rad(103.82),  0.01},
    };

    std::cout << "Ground stations:\n";
    for (const auto& gs : groundStations) {
        Vector3 pos = gs.positionEcefKm();
        std::cout << "  " << std::left << std::setw(14) << gs.name()
                  << " ECEF position: " << pos << " km\n";
    }

    std::cout << "\nSample of satellite positions at epoch (first satellite per plane):\n";
    std::cout << std::fixed << std::setprecision(1);
    for (const Satellite& sat : constellation.satellites()) {
        if (sat.slotIndex != 0) continue; // just show one per plane to keep it readable
        StateVector state = sat.propagator.stateAt(params.epoch);
        std::cout << "  " << sat.id
                  << "  plane=" << sat.planeIndex
                  << "  pos=" << state.positionKm << " km\n";
    }

    return 0;
}
