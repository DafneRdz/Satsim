#include "test_framework.hpp"
#include "network/Visibility.hpp"
#include "orbit/KeplerianElements.hpp"

#include <cmath>

using namespace satsim;

TEST_CASE(satellites_close_together_have_line_of_sight) {
    // Two satellites at the same altitude, 30 degrees apart in angular
    // position - close enough that the chord between them stays above
    // Earth's surface. (At low orbital altitudes, satellites separated by
    // large angles - e.g. 90 degrees - do NOT have line of sight, since
    // the chord between them dips below the horizon; this is expected,
    // physically correct behavior, not a bug.)
    double r = constants::kEarthRadius + 550.0;
    double angleRad = 30.0 * constants::kPi / 180.0;

    Vector3 satA(r, 0.0, 0.0);
    Vector3 satB(r * std::cos(angleRad), r * std::sin(angleRad), 0.0);

    CHECK_TRUE(visibility::hasLineOfSight(satA, satB, constants::kEarthRadius));
}

TEST_CASE(satellites_on_opposite_sides_of_earth_do_not_have_line_of_sight) {
    // Two satellites diametrically opposite each other - Earth is directly
    // between them, so the line connecting them passes through the planet.
    Vector3 satA(constants::kEarthRadius + 550.0, 0.0, 0.0);
    Vector3 satB(-(constants::kEarthRadius + 550.0), 0.0, 0.0);

    CHECK_TRUE(!visibility::hasLineOfSight(satA, satB, constants::kEarthRadius));
}

TEST_CASE(satellite_directly_overhead_has_ninety_degree_elevation) {
    Vector3 stationPos(constants::kEarthRadius, 0.0, 0.0); // on the surface at (R, 0, 0)
    Vector3 satPos(constants::kEarthRadius + 550.0, 0.0, 0.0); // directly above it

    double elevation = visibility::elevationAngleRad(stationPos, satPos);
    CHECK_NEAR(elevation, constants::kPi / 2.0, 1e-6);
}

TEST_CASE(satellite_on_the_horizon_has_near_zero_elevation) {
    // Place a satellite in the plane tangent to the station's position -
    // i.e. at the same "height" component as the station but offset
    // sideways - which sits near the local horizon.
    double stationX = constants::kEarthRadius;
    Vector3 stationPos(stationX, 0.0, 0.0);

    // A point far away in the y-direction at the same x - approximately on
    // the horizon plane for a station at (R,0,0), whose local horizontal
    // plane is the y-z plane through that point.
    Vector3 satPos(stationX, 5000.0, 0.0);

    double elevation = visibility::elevationAngleRad(stationPos, satPos);
    CHECK_TRUE(elevation < 5.0 * constants::kPi / 180.0); // within 5 degrees of horizon
}

TEST_CASE(satellite_below_horizon_is_not_visible_from_ground_station) {
    Vector3 stationPos(constants::kEarthRadius, 0.0, 0.0);
    // A satellite on the opposite side of Earth - well below this station's horizon.
    Vector3 satPos(-(constants::kEarthRadius + 550.0), 0.0, 0.0);

    bool visible = visibility::isVisibleFromGroundStation(
        stationPos, satPos, 10.0 * constants::kPi / 180.0, constants::kEarthRadius);

    CHECK_TRUE(!visible);
}

TEST_CASE(satellite_overhead_is_visible_from_ground_station) {
    Vector3 stationPos(constants::kEarthRadius, 0.0, 0.0);
    Vector3 satPos(constants::kEarthRadius + 550.0, 0.0, 0.0);

    bool visible = visibility::isVisibleFromGroundStation(
        stationPos, satPos, 10.0 * constants::kPi / 180.0, constants::kEarthRadius);

    CHECK_TRUE(visible);
}
