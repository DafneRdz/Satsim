#include "test_framework.hpp"
#include "routing/DijkstraRouter.hpp"
#include "routing/GreedyGeographicRouter.hpp"

using namespace satsim;

namespace {
WalkerConstellationParams makeRoutingTestParams() {
    WalkerConstellationParams p;
    p.inclinationRad = 53.0 * constants::kPi / 180.0;
    p.totalSatellites = 60;
    p.numPlanes = 12;
    p.phasingFactor = 1;
    p.altitudeKm = 550.0;
    p.eccentricity = 0.0;
    p.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);
    return p;
}

std::vector<GroundStation> makeRoutingTestGroundStations() {
    auto deg2rad = [](double d) { return d * constants::kPi / 180.0; };
    return {
        GroundStation("Los Angeles", deg2rad(34.05), deg2rad(-118.24), 0.09),
        GroundStation("London", deg2rad(51.51), deg2rad(-0.13), 0.02),
        GroundStation("Singapore", deg2rad(1.35), deg2rad(103.82), 0.01),
    };
}
}

TEST_CASE(dijkstra_finds_a_path_between_two_ground_stations) {
    WalkerConstellationParams params = makeRoutingTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeRoutingTestGroundStations();
    NetworkGraph graph(constellation, stations, params.epoch);

    size_t numSats = constellation.size();
    size_t laNode = numSats + 0;     // ground stations are appended after satellites
    size_t londonNode = numSats + 1;

    DijkstraRouter router;
    RoutingResult result = router.findPath(graph, laNode, londonNode);

    CHECK_TRUE(result.success);
    CHECK_TRUE(result.path.front() == laNode);
    CHECK_TRUE(result.path.back() == londonNode);
    CHECK_TRUE(result.hopCount > 0);
    CHECK_TRUE(result.totalDelaySeconds > 0.0);
}

TEST_CASE(dijkstra_path_to_itself_is_trivial) {
    WalkerConstellationParams params = makeRoutingTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeRoutingTestGroundStations();
    NetworkGraph graph(constellation, stations, params.epoch);

    DijkstraRouter router;
    RoutingResult result = router.findPath(graph, 0, 0);

    CHECK_TRUE(result.success);
    CHECK_TRUE(result.hopCount == 0);
    CHECK_NEAR(result.totalDelaySeconds, 0.0, 1e-12);
}

TEST_CASE(dijkstra_reports_no_path_between_disconnected_nodes) {
    // Build a graph with an artificially tiny ISL range so satellites can't
    // reach each other at all, guaranteeing ground stations are isolated.
    WalkerConstellationParams params = makeRoutingTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeRoutingTestGroundStations();

    NetworkGraphParams graphParams;
    graphParams.maxIslRangeKm = 1.0; // effectively disables all inter-satellite links

    NetworkGraph graph(constellation, stations, params.epoch, graphParams);

    size_t numSats = constellation.size();
    size_t laNode = numSats + 0;
    size_t londonNode = numSats + 1;

    DijkstraRouter router;
    RoutingResult result = router.findPath(graph, laNode, londonNode);

    // With no working ISLs, a ground station can only reach satellites
    // directly overhead, and those satellites can't relay anywhere -
    // so there should be no path all the way to another ground station.
    CHECK_TRUE(!result.success);
}

TEST_CASE(greedy_router_either_succeeds_with_a_valid_path_or_fails_cleanly) {
    WalkerConstellationParams params = makeRoutingTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeRoutingTestGroundStations();
    NetworkGraph graph(constellation, stations, params.epoch);

    size_t numSats = constellation.size();
    size_t laNode = numSats + 0;
    size_t londonNode = numSats + 1;

    GreedyGeographicRouter router;
    RoutingResult result = router.findPath(graph, laNode, londonNode);

    if (result.success) {
        CHECK_TRUE(result.path.front() == laNode);
        CHECK_TRUE(result.path.back() == londonNode);
        CHECK_TRUE(result.totalDelaySeconds > 0.0);
    } else {
        // A clean failure means an empty path and no bogus cost - not a crash.
        CHECK_TRUE(result.path.empty());
        CHECK_NEAR(result.totalDelaySeconds, 0.0, 1e-12);
    }
}

TEST_CASE(dijkstra_is_never_slower_than_greedy_when_both_succeed) {
    // Dijkstra has global knowledge and always finds the objectively best
    // path, so whenever greedy also manages to find a path, Dijkstra's
    // total delay should be less than or equal to it. If greedy fails, we
    // can't conclude anything about whether a path exists at all (it might
    // be a genuine coverage gap, or just a local-minimum failure) - so we
    // only assert the comparison in the case where it's guaranteed to hold.
    WalkerConstellationParams params = makeRoutingTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeRoutingTestGroundStations();
    NetworkGraph graph(constellation, stations, params.epoch);

    size_t numSats = constellation.size();
    size_t laNode = numSats + 0;
    size_t singaporeNode = numSats + 2;

    DijkstraRouter dijkstra;
    GreedyGeographicRouter greedy;

    RoutingResult dijkstraResult = dijkstra.findPath(graph, laNode, singaporeNode);
    RoutingResult greedyResult = greedy.findPath(graph, laNode, singaporeNode);

    if (greedyResult.success) {
        // If greedy found any path at all, a path definitely exists, so
        // Dijkstra (with full knowledge) must find one too, and it must be
        // at least as good.
        CHECK_TRUE(dijkstraResult.success);
        if (dijkstraResult.success) {
            CHECK_TRUE(dijkstraResult.totalDelaySeconds <= greedyResult.totalDelaySeconds + 1e-9);
        }
    }
    // If greedy failed, no assertion: it may be a genuine coverage gap
    // (no path exists at all, so Dijkstra fails too - as seen with sparse
    // constellations) or a local-minimum failure specific to greedy
    // routing. Both are valid, expected outcomes.
}
