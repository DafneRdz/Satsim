#pragma once

#include "orbit/Time.hpp"

namespace satsim {

// Standard physical constants used throughout the orbital mechanics module.
namespace constants {
    constexpr double kEarthMu = 398600.4418;      // Earth's gravitational parameter, km^3/s^2
    constexpr double kEarthRadius = 6378.137;      // Earth's equatorial radius, km
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kTwoPi = 2.0 * kPi;
}

// The six classical (Keplerian) orbital elements that fully describe an
// unperturbed two-body orbit at a given epoch.
struct KeplerianElements {
    double semiMajorAxisKm;      // a: size of the orbit, km
    double eccentricity;         // e: 0 = circular, 0<e<1 = elliptical
    double inclinationRad;       // i: tilt relative to the equatorial plane
    double raanRad;              // Omega: right ascension of ascending node
    double argPerigeeRad;        // omega: orientation of the ellipse within the orbital plane
    double meanAnomalyRad;       // M: position of the satellite along the orbit at epoch
    JulianDate epoch;            // the time at which these elements are valid

    // Orbital period, in seconds, from Kepler's third law.
    double periodSeconds() const;

    // Mean motion (rad/s) - the average angular speed of the satellite.
    double meanMotionRadPerSec() const;
};

} // namespace satsim
