#include "test_framework.hpp"
#include "constellation/GroundStation.hpp"
#include "orbit/KeplerianElements.hpp"

using namespace satsim;

TEST_CASE(ground_station_ecef_position_at_equator_prime_meridian) {
    // A station at (0 lat, 0 lon, 0 altitude) should sit exactly on the
    // x-axis at Earth's radius.
    GroundStation station("TestStation", 0.0, 0.0, 0.0);
    Vector3 pos = station.positionEcefKm();

    CHECK_NEAR(pos.x, constants::kEarthRadius, 1e-6);
    CHECK_NEAR(pos.y, 0.0, 1e-6);
    CHECK_NEAR(pos.z, 0.0, 1e-6);
}

TEST_CASE(ground_station_ecef_position_at_north_pole) {
    GroundStation station("NorthPole", constants::kPi / 2.0, 0.0, 0.0);
    Vector3 pos = station.positionEcefKm();

    CHECK_NEAR(pos.x, 0.0, 1e-6);
    CHECK_NEAR(pos.y, 0.0, 1e-6);
    CHECK_NEAR(pos.z, constants::kEarthRadius, 1e-6);
}

TEST_CASE(ground_station_ecef_norm_equals_earth_radius_plus_altitude) {
    GroundStation station("SomeCity", 0.65, -1.2, 0.1); // arbitrary lat/lon, 100m altitude
    Vector3 pos = station.positionEcefKm();

    CHECK_NEAR(pos.norm(), constants::kEarthRadius + 0.1, 1e-6);
}

TEST_CASE(ground_station_eci_position_has_same_radius_as_ecef) {
    // Rotating into the ECI frame shouldn't change the station's distance
    // from Earth's center, only its x/y orientation.
    GroundStation station("SomeCity", 0.4, 2.1, 0.0);
    JulianDate t = JulianDate::fromCalendar(2026, 6, 15, 12, 30, 0.0);

    Vector3 eci = station.positionEciKm(t);
    CHECK_NEAR(eci.norm(), constants::kEarthRadius, 1e-6);
}
