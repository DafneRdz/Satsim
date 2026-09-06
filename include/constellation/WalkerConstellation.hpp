#pragma once

#include "orbit/KeplerianElements.hpp"
#include "orbit/OrbitPropagator.hpp"
#include "orbit/Time.hpp"

#include <string>
#include <vector>

namespace satsim {

// A single satellite within a constellation: an ID, its orbital elements,
// and a propagator ready to compute its position at any time.
struct Satellite {
    std::string id;            // e.g. "SAT-P2-S05" (plane 2, slot 5)
    int planeIndex;
    int slotIndex;
    OrbitPropagator propagator;

    Satellite(std::string id_, int plane, int slot, KeplerianElements elements)
        : id(std::move(id_)), planeIndex(plane), slotIndex(slot),
          propagator(elements) {}
};

// Parameters describing a Walker-Delta constellation pattern, notated
// as i:T/P/F in the literature:
//   i = inclination
//   T = total number of satellites
//   P = number of orbital planes (T must be divisible by P)
//   F = phasing factor (0 to P-1), controls relative offset of satellites
//       between adjacent planes
struct WalkerConstellationParams {
    double inclinationRad;
    int totalSatellites;
    int numPlanes;
    int phasingFactor = 0;
    double altitudeKm;         // circular orbit altitude
    double eccentricity = 0.0; // Walker constellations are typically circular
    JulianDate epoch;
};

// Generates a Walker-Delta constellation: a standard pattern for evenly
// distributing satellites across multiple orbital planes, used by real
// systems like Starlink, Iridium, and GPS. Satellites within a plane are
// evenly spaced; planes are evenly spaced in RAAN; the phasing factor
// offsets each plane's satellites relative to the one before it so the
// whole constellation has uniform global coverage rather than satellites
// clustering together as they cross the equator.
class WalkerConstellation {
public:
    explicit WalkerConstellation(const WalkerConstellationParams& params);

    const std::vector<Satellite>& satellites() const { return satellites_; }
    size_t size() const { return satellites_.size(); }

    const WalkerConstellationParams& params() const { return params_; }

private:
    WalkerConstellationParams params_;
    std::vector<Satellite> satellites_;

    void generate();
};

} // namespace satsim
