#include "orbit/KeplerianElements.hpp"

#include <cmath>

namespace satsim {

double KeplerianElements::periodSeconds() const {
    // Kepler's third law: T = 2*pi * sqrt(a^3 / mu)
    return constants::kTwoPi * std::sqrt(
        (semiMajorAxisKm * semiMajorAxisKm * semiMajorAxisKm) / constants::kEarthMu);
}

double KeplerianElements::meanMotionRadPerSec() const {
    return constants::kTwoPi / periodSeconds();
}

} // namespace satsim
