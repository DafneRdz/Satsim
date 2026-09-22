#include "test_framework.hpp"
#include "constellation/WalkerConstellation.hpp"

using namespace satsim;

namespace {
WalkerConstellationParams makeStarlinkLikeParams() {
    WalkerConstellationParams p;
    p.inclinationRad = 53.0 * constants::kPi / 180.0;
    p.totalSatellites = 60;   // 12 planes x 5 sats/plane, small enough to test fast
    p.numPlanes = 12;
    p.phasingFactor = 1;
    p.altitudeKm = 550.0;
    p.eccentricity = 0.0;
    p.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);
    return p;
}
}

TEST_CASE(constellation_generates_correct_total_satellite_count) {
    WalkerConstellation constellation(makeStarlinkLikeParams());
    CHECK_TRUE(constellation.size() == 60);
}

TEST_CASE(constellation_rejects_uneven_plane_division) {
    WalkerConstellationParams p = makeStarlinkLikeParams();
    p.totalSatellites = 61; // not divisible by 12 planes

    bool threw = false;
    try {
        WalkerConstellation constellation(p);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK_TRUE(threw);
}

TEST_CASE(all_satellites_share_the_same_altitude_and_inclination) {
    WalkerConstellationParams p = makeStarlinkLikeParams();
    WalkerConstellation constellation(p);

    double expectedSemiMajorAxis = constants::kEarthRadius + p.altitudeKm;

    for (const Satellite& sat : constellation.satellites()) {
        CHECK_NEAR(sat.propagator.elements().semiMajorAxisKm, expectedSemiMajorAxis, 1e-9);
        CHECK_NEAR(sat.propagator.elements().inclinationRad, p.inclinationRad, 1e-9);
    }
}

TEST_CASE(planes_are_evenly_spaced_in_raan) {
    WalkerConstellationParams p = makeStarlinkLikeParams();
    WalkerConstellation constellation(p);

    double expectedSpacing = constants::kTwoPi / p.numPlanes;

    // Check the first satellite of plane 0 vs plane 1.
    double raan0 = constellation.satellites()[0].propagator.elements().raanRad;
    int satsPerPlane = p.totalSatellites / p.numPlanes;
    double raan1 = constellation.satellites()[satsPerPlane].propagator.elements().raanRad;

    CHECK_NEAR(raan1 - raan0, expectedSpacing, 1e-9);
}

TEST_CASE(satellites_within_a_plane_are_evenly_spaced_in_mean_anomaly) {
    WalkerConstellationParams p = makeStarlinkLikeParams();
    WalkerConstellation constellation(p);

    int satsPerPlane = p.totalSatellites / p.numPlanes;
    double expectedSpacing = constants::kTwoPi / satsPerPlane;

    // First two satellites are both in plane 0 (slots 0 and 1).
    double m0 = constellation.satellites()[0].propagator.elements().meanAnomalyRad;
    double m1 = constellation.satellites()[1].propagator.elements().meanAnomalyRad;

    CHECK_NEAR(m1 - m0, expectedSpacing, 1e-9);
}

TEST_CASE(all_satellites_have_unique_ids) {
    WalkerConstellation constellation(makeStarlinkLikeParams());

    for (size_t i = 0; i < constellation.satellites().size(); ++i) {
        for (size_t j = i + 1; j < constellation.satellites().size(); ++j) {
            CHECK_TRUE(constellation.satellites()[i].id != constellation.satellites()[j].id);
        }
    }
}
