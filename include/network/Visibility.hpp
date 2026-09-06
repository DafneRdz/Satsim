#pragma once

#include "orbit/Vector3.hpp"

namespace satsim {

// Functions for determining whether two points can communicate with each
// other - i.e. whether a link between them is geometrically possible.
// This is what turns a static list of satellites/ground stations into a
// time-varying network topology: a link exists only while these checks
// pass, and stops existing the moment they don't.
namespace visibility {

// Whether two points (typically two satellites) have an unobstructed line
// of sight to each other, i.e. the straight line between them does not pass
// through Earth. Both positions must be in the same frame (ECI or ECEF -
// it doesn't matter which, as long as both use the same one).
bool hasLineOfSight(const Vector3& posA, const Vector3& posB, double earthRadiusKm);

// The elevation angle (radians) of a satellite as seen from a ground
// station: 0 means the satellite is exactly on the horizon, pi/2 (90 deg)
// means directly overhead, and negative means below the horizon (not
// visible at all). Both positions must be in the same frame.
double elevationAngleRad(const Vector3& groundStationPosKm, const Vector3& satellitePosKm);

// Whether a satellite is visible from a ground station, requiring both a
// clear line of sight (not blocked by Earth) and a minimum elevation angle
// above the horizon. Real ground stations typically require 5-10 degrees
// of minimum elevation, since very low elevation paths pass through more
// atmosphere and are prone to signal degradation and terrain obstruction.
bool isVisibleFromGroundStation(const Vector3& groundStationPosKm, const Vector3& satellitePosKm,
                                 double minElevationRad, double earthRadiusKm);

} // namespace visibility
} // namespace satsim
