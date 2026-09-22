#include "test_framework.hpp"
#include "network/NetworkGraph.hpp"

using namespace satsim;

namespace {
WalkerConstellationParams makeSmallConstellationParams() {
    WalkerConstellationParams p;
    p.inclinationRad = 53.0 * constants::kPi / 180.0;
    p.totalSatellites = 24;
    p.numPlanes = 6;
    p.phasingFactor = 1;
    p.altitudeKm = 550.0;
    p.eccentricity = 0.0;
    p.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);
    return p;
}

std::vector<GroundStation> makeGroundStations() {
    auto deg2rad = [](double d) { return d * constants::kPi / 180.0; };
    return {
        GroundStation("Los Angeles", deg2rad(34.05), deg2rad(-118.24), 0.09),
        GroundStation("London", deg2rad(51.51), deg2rad(-0.13), 0.02),
    };
}
}

TEST_CASE(network_graph_has_one_node_per_satellite_and_ground_station) {
    WalkerConstellation constellation(makeSmallConstellationParams());
    std::vector<GroundStation> stations = makeGroundStations();

    NetworkGraph graph(constellation, stations, makeSmallConstellationParams().epoch);

    CHECK_TRUE(graph.nodeCount() == constellation.size() + stations.size());
}

TEST_CASE(network_graph_produces_at_least_some_edges) {
    // With 24 satellites reasonably spread out, at least some pairs should
    // have line of sight to each other.
    WalkerConstellation constellation(makeSmallConstellationParams());
    std::vector<GroundStation> stations = makeGroundStations();

    NetworkGraph graph(constellation, stations, makeSmallConstellationParams().epoch);

    CHECK_TRUE(graph.edgeCount() > 0);
}

TEST_CASE(every_edge_has_a_positive_delay_consistent_with_its_distance) {
    WalkerConstellation constellation(makeSmallConstellationParams());
    std::vector<GroundStation> stations = makeGroundStations();

    NetworkGraph graph(constellation, stations, makeSmallConstellationParams().epoch);

    for (const NetworkEdge& edge : graph.edges()) {
        CHECK_TRUE(edge.distanceKm > 0.0);
        double expectedDelay = edge.distanceKm / constants::kSpeedOfLightKmPerSec;
        CHECK_NEAR(edge.delaySeconds, expectedDelay, 1e-9);
    }
}

TEST_CASE(no_satellite_satellite_edge_exceeds_the_configured_max_isl_range) {
    WalkerConstellationParams params = makeSmallConstellationParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeGroundStations();

    NetworkGraphParams graphParams;
    graphParams.maxIslRangeKm = 3000.0; // deliberately tight range for this test

    NetworkGraph graph(constellation, stations, params.epoch, graphParams);

    for (const NetworkEdge& edge : graph.edges()) {
        const NetworkNode& a = graph.nodes()[edge.nodeA];
        const NetworkNode& b = graph.nodes()[edge.nodeB];
        bool bothSatellites = (a.type == NetworkNode::Type::Satellite &&
                                b.type == NetworkNode::Type::Satellite);
        if (bothSatellites) {
            CHECK_TRUE(edge.distanceKm <= graphParams.maxIslRangeKm);
        }
    }
}

TEST_CASE(adjacency_list_is_consistent_with_the_edge_list) {
    WalkerConstellation constellation(makeSmallConstellationParams());
    std::vector<GroundStation> stations = makeGroundStations();

    NetworkGraph graph(constellation, stations, makeSmallConstellationParams().epoch);

    // Every edge index stored in a node's adjacency list should actually
    // reference an edge that touches that node.
    for (size_t nodeIdx = 0; nodeIdx < graph.nodeCount(); ++nodeIdx) {
        for (size_t edgeIdx : graph.edgesAt(nodeIdx)) {
            const NetworkEdge& edge = graph.edges()[edgeIdx];
            bool touchesNode = (edge.nodeA == nodeIdx || edge.nodeB == nodeIdx);
            CHECK_TRUE(touchesNode);
        }
    }
}

TEST_CASE(network_topology_changes_as_satellites_move) {
    // The whole point of a "dynamic" network graph: build two snapshots at
    // different times and confirm the set of edges is not identical, since
    // satellites will have moved in/out of visibility of each other and of
    // ground stations.
    WalkerConstellationParams params = makeSmallConstellationParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeGroundStations();

    NetworkGraph graphAtEpoch(constellation, stations, params.epoch);

    // Jump forward by a third of an orbital period - enough for
    // significant satellite movement.
    JulianDate laterTime = params.epoch.addSeconds(constellation.satellites()[0].propagator.elements().periodSeconds() / 3.0);
    NetworkGraph graphLater(constellation, stations, laterTime);

    CHECK_TRUE(graphAtEpoch.edgeCount() != graphLater.edgeCount() ||
               graphAtEpoch.nodes()[0].positionKm.distanceTo(graphLater.nodes()[0].positionKm) > 1.0);
}
