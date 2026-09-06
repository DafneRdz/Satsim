#include "network/Visibility.hpp"
#include "orbit/KeplerianElements.hpp" // for constants::kPi

#include <algorithm>
#include <cmath>

namespace satsim {
namespace visibility {

bool hasLineOfSight(const Vector3& posA, const Vector3& posB, double earthRadiusKm) {
    // Find the point on the line segment [posA, posB] that is closest to
    // Earth's center (the origin). If that closest point is still farther
    // from the origin than Earth's radius, the segment never dips inside
    // Earth, so the two points can see each other.
    Vector3 segment = posB - posA;
    double segmentLengthSquared = segment.normSquared();

    double t = 0.0;
    if (segmentLengthSquared > 1e-9) {
        // t is the parameter (0 = posA, 1 = posB) of the closest point to
        // the origin on the infinite line through posA/posB, clamped to
        // the actual segment.
        t = -posA.dot(segment) / segmentLengthSquared;
        t = std::clamp(t, 0.0, 1.0);
    }

    Vector3 closestPoint = posA + segment * t;
    return closestPoint.norm() >= earthRadiusKm;
}

double elevationAngleRad(const Vector3& groundStationPosKm, const Vector3& satellitePosKm) {
    // The local "up" direction at the ground station, approximating Earth
    // as a sphere (the zenith direction is just the radial direction).
    Vector3 up = groundStationPosKm.normalized();

    Vector3 toSatellite = satellitePosKm - groundStationPosKm;
    double range = toSatellite.norm();
    if (range < 1e-9) return constants::kPi / 2.0; // degenerate: same position

    // The elevation angle is the angle between the line-of-sight vector and
    // the local horizontal plane, which equals asin of the component of the
    // (normalized) line-of-sight vector along the local up direction.
    double sinElevation = toSatellite.dot(up) / range;
    sinElevation = std::clamp(sinElevation, -1.0, 1.0);

    return std::asin(sinElevation);
}

bool isVisibleFromGroundStation(const Vector3& groundStationPosKm, const Vector3& satellitePosKm,
                                 double minElevationRad, double earthRadiusKm) {
    double elevation = elevationAngleRad(groundStationPosKm, satellitePosKm);
    if (elevation < minElevationRad) return false;

    // Elevation angle alone is sufficient for a spherical Earth model (a
    // satellite above the local horizon by definition has clear line of
    // sight to a point on the surface), but we also run the explicit
    // line-of-sight check for consistency with the satellite-satellite case
    // and as a safety net if the model changes later.
    return hasLineOfSight(groundStationPosKm, satellitePosKm, earthRadiusKm);
}

} // namespace visibility
} // namespace satsim
