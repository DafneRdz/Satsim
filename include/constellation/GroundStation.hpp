#pragma once

#include "orbit/Vector3.hpp"
#include "orbit/Time.hpp"
#include <string>

namespace satsim {

// A fixed point on Earth's surface - a ground station that satellites
// communicate with (uplink/downlink). Unlike a satellite, its position in
// the Earth-Centered Earth-Fixed (ECEF) frame never changes; only its
// position in the inertial (ECI) frame changes, as Earth rotates beneath it.
class GroundStation {
public:
    GroundStation(std::string name, double latitudeRad, double longitudeRad,
                  double altitudeKm = 0.0);

    const std::string& name() const { return name_; }
    double latitudeRad() const { return latitudeRad_; }
    double longitudeRad() const { return longitudeRad_; }
    double altitudeKm() const { return altitudeKm_; }

    // Position in the Earth-Centered Earth-Fixed (ECEF) frame - fixed,
    // does not depend on time.
    Vector3 positionEcefKm() const;

    // Position in the Earth-Centered Inertial (ECI) frame at a given time -
    // this rotates as Earth spins, so it depends on the current GMST.
    Vector3 positionEciKm(const JulianDate& time) const;

private:
    std::string name_;
    double latitudeRad_;
    double longitudeRad_;
    double altitudeKm_;
};

} // namespace satsim
