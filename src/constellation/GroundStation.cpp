#include "constellation/GroundStation.hpp"
#include "orbit/KeplerianElements.hpp" // for constants::kEarthRadius

#include <cmath>

namespace satsim {

GroundStation::GroundStation(std::string name, double latitudeRad, double longitudeRad,
                              double altitudeKm)
    : name_(std::move(name)),
      latitudeRad_(latitudeRad),
      longitudeRad_(longitudeRad),
      altitudeKm_(altitudeKm) {}

Vector3 GroundStation::positionEcefKm() const {
    // Simple spherical Earth model (good enough for this project - a more
    // precise oblate-spheroid/WGS84 model could replace this later without
    // changing the public interface).
    double r = constants::kEarthRadius + altitudeKm_;
    double cosLat = std::cos(latitudeRad_);

    return Vector3(
        r * cosLat * std::cos(longitudeRad_),
        r * cosLat * std::sin(longitudeRad_),
        r * std::sin(latitudeRad_)
    );
}

Vector3 GroundStation::positionEciKm(const JulianDate& time) const {
    // Rotate the fixed ECEF position by GMST to get its position in the
    // inertial frame at this instant - this is what lets us directly
    // compare distances/angles with satellite positions (which are
    // naturally computed in ECI).
    Vector3 ecef = positionEcefKm();
    double theta = time.gmstRadians();

    double cosTheta = std::cos(theta);
    double sinTheta = std::sin(theta);

    return Vector3(
        ecef.x * cosTheta - ecef.y * sinTheta,
        ecef.x * sinTheta + ecef.y * cosTheta,
        ecef.z
    );
}

} // namespace satsim
