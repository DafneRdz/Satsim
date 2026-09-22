#include "orbit/OrbitPropagator.hpp"

#include <cmath>
#include <stdexcept>

namespace satsim {

using constants::kEarthMu;
using constants::kTwoPi;

OrbitPropagator::OrbitPropagator(const KeplerianElements& elements) : elements_(elements) {
    if (elements_.eccentricity < 0.0 || elements_.eccentricity >= 1.0) {
        throw std::invalid_argument(
            "OrbitPropagator currently supports only circular/elliptical orbits (0 <= e < 1)");
    }
    if (elements_.semiMajorAxisKm <= 0.0) {
        throw std::invalid_argument("Semi-major axis must be positive");
    }
}

double OrbitPropagator::solveEccentricAnomaly(double meanAnomalyRad, double eccentricity) {
    // Normalize M into [0, 2*pi) for numerical stability.
    double M = std::fmod(meanAnomalyRad, kTwoPi);
    if (M < 0.0) M += kTwoPi;

    // Initial guess: for low eccentricity, E ~= M is already close.
    double E = (eccentricity < 0.8) ? M : constants::kPi;

    constexpr int kMaxIterations = 50;
    constexpr double kTolerance = 1e-12;

    for (int i = 0; i < kMaxIterations; ++i) {
        double f = E - eccentricity * std::sin(E) - M;
        double fPrime = 1.0 - eccentricity * std::cos(E);
        double deltaE = f / fPrime;
        E -= deltaE;
        if (std::fabs(deltaE) < kTolerance) break;
    }
    return E;
}

StateVector OrbitPropagator::stateAt(const JulianDate& time) const {
    // 1. Propagate the mean anomaly forward from epoch using mean motion.
    double dtSeconds = time.secondsSince(elements_.epoch);
    double n = elements_.meanMotionRadPerSec();
    double M = elements_.meanAnomalyRad + n * dtSeconds;

    // 2. Solve Kepler's equation for eccentric anomaly.
    double E = solveEccentricAnomaly(M, elements_.eccentricity);

    // 3. Convert eccentric anomaly to true anomaly (nu).
    double e = elements_.eccentricity;
    double cosE = std::cos(E);
    double sinE = std::sin(E);
    double sqrtOneMinusE2 = std::sqrt(1.0 - e * e);

    double trueAnomaly = std::atan2(sqrtOneMinusE2 * sinE, cosE - e);

    // 4. Compute distance from focus (Earth) and position in the perifocal
    //    (orbital-plane) frame, where x points toward perigee.
    double a = elements_.semiMajorAxisKm;
    double r = a * (1.0 - e * cosE);

    double xPerifocal = r * std::cos(trueAnomaly);
    double yPerifocal = r * std::sin(trueAnomaly);

    // Velocity in the perifocal frame (from the vis-viva relations).
    double p = a * (1.0 - e * e); // semi-latus rectum
    double h = std::sqrt(kEarthMu * p); // specific angular momentum

    double vxPerifocal = -(kEarthMu / h) * std::sin(trueAnomaly);
    double vyPerifocal = (kEarthMu / h) * (e + std::cos(trueAnomaly));

    // 5. Rotate from the perifocal frame into the Earth-Centered Inertial
    //    (ECI) frame using the classical 3-1-3 rotation (RAAN, inclination,
    //    argument of perigee).
    double raan = elements_.raanRad;
    double incl = elements_.inclinationRad;
    double argP = elements_.argPerigeeRad;

    double cosRaan = std::cos(raan), sinRaan = std::sin(raan);
    double cosIncl = std::cos(incl), sinIncl = std::sin(incl);
    double cosArgP = std::cos(argP), sinArgP = std::sin(argP);

    // Combined rotation matrix R = R3(-raan) * R1(-incl) * R3(-argP)
    double r11 = cosRaan * cosArgP - sinRaan * sinArgP * cosIncl;
    double r12 = -cosRaan * sinArgP - sinRaan * cosArgP * cosIncl;
    double r21 = sinRaan * cosArgP + cosRaan * sinArgP * cosIncl;
    double r22 = -sinRaan * sinArgP + cosRaan * cosArgP * cosIncl;
    double r31 = sinArgP * sinIncl;
    double r32 = cosArgP * sinIncl;

    StateVector state;
    state.positionKm = Vector3(
        r11 * xPerifocal + r12 * yPerifocal,
        r21 * xPerifocal + r22 * yPerifocal,
        r31 * xPerifocal + r32 * yPerifocal
    );
    state.velocityKmPerSec = Vector3(
        r11 * vxPerifocal + r12 * vyPerifocal,
        r21 * vxPerifocal + r22 * vyPerifocal,
        r31 * vxPerifocal + r32 * vyPerifocal
    );

    return state;
}

void OrbitPropagator::groundTrackAt(const JulianDate& time, double& latRad, double& lonRad) const {
    StateVector state = stateAt(time);
    const Vector3& pos = state.positionKm;

    double r = pos.norm();
    latRad = std::asin(pos.z / r);

    // Longitude in the inertial frame, then subtract GMST to account for
    // Earth's rotation and get the sub-satellite longitude on the ground.
    double lonInertial = std::atan2(pos.y, pos.x);
    lonRad = lonInertial - time.gmstRadians();

    // Normalize to [-pi, pi]
    while (lonRad > constants::kPi) lonRad -= kTwoPi;
    while (lonRad < -constants::kPi) lonRad += kTwoPi;
}

} // namespace satsim
