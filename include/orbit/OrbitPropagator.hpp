#pragma once

#include "orbit/KeplerianElements.hpp"
#include "orbit/Vector3.hpp"
#include "orbit/Time.hpp"

namespace satsim {

// The Cartesian state of a satellite at some instant: where it is and how
// fast it's moving, expressed in the Earth-Centered Inertial (ECI) frame.
struct StateVector {
    Vector3 positionKm;
    Vector3 velocityKmPerSec;
};

// Propagates a satellite's orbit forward/backward in time using unperturbed
// two-body (Keplerian) dynamics. This ignores perturbations like J2 oblateness,
// atmospheric drag, and solar radiation pressure - a reasonable first model,
// with room to extend later (see docs/roadmap.md).
class OrbitPropagator {
public:
    explicit OrbitPropagator(const KeplerianElements& elements);

    // Computes the satellite's ECI position/velocity at the given time.
    StateVector stateAt(const JulianDate& time) const;

    // Computes the satellite's ground track: sub-satellite latitude/longitude
    // (radians) at the given time, accounting for Earth's rotation.
    void groundTrackAt(const JulianDate& time, double& latRad, double& lonRad) const;

    const KeplerianElements& elements() const { return elements_; }

private:
    KeplerianElements elements_;

    // Solves Kepler's equation M = E - e*sin(E) for the eccentric anomaly E,
    // given the mean anomaly M, via Newton-Raphson iteration.
    static double solveEccentricAnomaly(double meanAnomalyRad, double eccentricity);
};

} // namespace satsim
