#include "constellation/WalkerConstellation.hpp"

#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace satsim {

WalkerConstellation::WalkerConstellation(const WalkerConstellationParams& params)
    : params_(params) {
    if (params_.numPlanes <= 0) {
        throw std::invalid_argument("numPlanes must be positive");
    }
    if (params_.totalSatellites % params_.numPlanes != 0) {
        throw std::invalid_argument(
            "totalSatellites must be evenly divisible by numPlanes for a Walker-Delta pattern");
    }
    if (params_.phasingFactor < 0 || params_.phasingFactor >= params_.numPlanes) {
        throw std::invalid_argument("phasingFactor must be in [0, numPlanes)");
    }

    generate();
}

void WalkerConstellation::generate() {
    int satsPerPlane = params_.totalSatellites / params_.numPlanes;
    double semiMajorAxisKm = constants::kEarthRadius + params_.altitudeKm;

    // Planes are evenly spaced around 360 degrees of RAAN.
    double raanSpacing = constants::kTwoPi / params_.numPlanes;

    // Satellites within a plane are evenly spaced around 360 degrees of
    // mean anomaly.
    double meanAnomalySpacing = constants::kTwoPi / satsPerPlane;

    // The phasing factor F offsets each plane's starting mean anomaly by
    // F * 360/T degrees relative to the previous plane - this is the
    // standard Walker-Delta phasing rule.
    double phasingStep = constants::kTwoPi * params_.phasingFactor / params_.totalSatellites;

    satellites_.reserve(params_.totalSatellites);

    for (int plane = 0; plane < params_.numPlanes; ++plane) {
        double raan = plane * raanSpacing;
        double planePhaseOffset = plane * phasingStep;

        for (int slot = 0; slot < satsPerPlane; ++slot) {
            KeplerianElements elements;
            elements.semiMajorAxisKm = semiMajorAxisKm;
            elements.eccentricity = params_.eccentricity;
            elements.inclinationRad = params_.inclinationRad;
            elements.raanRad = raan;
            elements.argPerigeeRad = 0.0; // circular orbits: argument of perigee is undefined/irrelevant
            elements.meanAnomalyRad = slot * meanAnomalySpacing + planePhaseOffset;
            elements.epoch = params_.epoch;

            std::ostringstream idStream;
            idStream << "SAT-P" << std::setfill('0') << std::setw(2) << plane
                      << "-S" << std::setfill('0') << std::setw(2) << slot;

            satellites_.emplace_back(idStream.str(), plane, slot, elements);
        }
    }
}

} // namespace satsim
