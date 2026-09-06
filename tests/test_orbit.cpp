#include "test_framework.hpp"
#include "orbit/OrbitPropagator.hpp"
#include "orbit/KeplerianElements.hpp"
#include "orbit/Time.hpp"

using namespace satsim;

namespace {
// Helper: a circular LEO-like orbit for testing.
KeplerianElements makeCircularLeoOrbit() {
    KeplerianElements elems;
    elems.semiMajorAxisKm = constants::kEarthRadius + 550.0; // 550 km altitude, like Starlink
    elems.eccentricity = 0.0;
    elems.inclinationRad = 53.0 * constants::kPi / 180.0;
    elems.raanRad = 0.0;
    elems.argPerigeeRad = 0.0;
    elems.meanAnomalyRad = 0.0;
    elems.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);
    return elems;
}
}

TEST_CASE(orbital_period_matches_known_leo_value) {
    KeplerianElements elems = makeCircularLeoOrbit();
    // A 550 km altitude circular orbit has a period of roughly 95.4 minutes.
    double periodMinutes = elems.periodSeconds() / 60.0;
    CHECK_TRUE(periodMinutes > 94.0 && periodMinutes < 97.0);
}

TEST_CASE(propagated_position_stays_at_constant_radius_for_circular_orbit) {
    KeplerianElements elems = makeCircularLeoOrbit();
    OrbitPropagator propagator(elems);

    double expectedRadius = elems.semiMajorAxisKm; // circular orbit: r is constant

    for (int i = 0; i <= 10; ++i) {
        JulianDate t = elems.epoch.addSeconds(i * 500.0); // sample across the orbit
        StateVector state = propagator.stateAt(t);
        CHECK_NEAR(state.positionKm.norm(), expectedRadius, 1e-6);
    }
}

TEST_CASE(satellite_returns_to_start_after_one_full_period) {
    KeplerianElements elems = makeCircularLeoOrbit();
    OrbitPropagator propagator(elems);

    StateVector start = propagator.stateAt(elems.epoch);
    JulianDate oneOrbitLater = elems.epoch.addSeconds(elems.periodSeconds());
    StateVector after = propagator.stateAt(oneOrbitLater);

    CHECK_NEAR(after.positionKm.x, start.positionKm.x, 1e-3);
    CHECK_NEAR(after.positionKm.y, start.positionKm.y, 1e-3);
    CHECK_NEAR(after.positionKm.z, start.positionKm.z, 1e-3);
}

TEST_CASE(velocity_is_perpendicular_to_position_for_circular_orbit) {
    // For a circular orbit, velocity should always be perpendicular to the
    // radius vector (no radial component).
    KeplerianElements elems = makeCircularLeoOrbit();
    OrbitPropagator propagator(elems);

    JulianDate t = elems.epoch.addSeconds(1234.0);
    StateVector state = propagator.stateAt(t);

    double dotProduct = state.positionKm.dot(state.velocityKmPerSec);
    CHECK_NEAR(dotProduct, 0.0, 1e-4);
}

TEST_CASE(ground_track_latitude_stays_within_inclination_bounds) {
    // A satellite's ground track latitude should never exceed its orbital
    // inclination (for inclination <= 90 degrees).
    KeplerianElements elems = makeCircularLeoOrbit();
    OrbitPropagator propagator(elems);

    double maxLat = 0.0;
    for (int i = 0; i <= 100; ++i) {
        JulianDate t = elems.epoch.addSeconds(i * (elems.periodSeconds() / 100.0));
        double lat, lon;
        propagator.groundTrackAt(t, lat, lon);
        maxLat = std::max(maxLat, std::fabs(lat));
    }

    CHECK_TRUE(maxLat <= elems.inclinationRad + 1e-3);
}
