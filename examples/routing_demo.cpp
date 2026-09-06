// Phase 4 demo: build a constellation + network graph, then route between
// every pair of ground stations using both Dijkstra (global knowledge) and
// greedy geographic routing (local knowledge only), comparing the results.
//
// Build & run (from repo root):
//   cmake -B build && cmake --build build
//   ./build/routing_demo

#include "routing/DijkstraRouter.hpp"
#include "routing/GreedyGeographicRouter.hpp"

#include <iostream>
#include <iomanip>

using namespace satsim;

namespace {
void printResult(const std::string& routerName, const RoutingResult& result) {
    std::cout << "  " << std::left << std::setw(38) << routerName;
    if (result.success) {
        std::cout << "hops=" << std::setw(3) << result.hopCount
                   << " delay=" << std::fixed << std::setprecision(2)
                   << result.totalDelaySeconds * 1000.0 << " ms\n";
    } else {
        std::cout << "FAILED to find a path\n";
    }
}
}

int main() {
    WalkerConstellationParams params;
    params.inclinationRad = 53.0 * constants::kPi / 180.0;
    params.totalSatellites = 60;
    params.numPlanes = 12;
    params.phasingFactor = 1;
    params.altitudeKm = 550.0;
    params.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);

    WalkerConstellation constellation(params);

    auto deg2rad = [](double d) { return d * constants::kPi / 180.0; };
    std::vector<GroundStation> groundStations = {
        {"Los Angeles", deg2rad(34.05), deg2rad(-118.24), 0.09},
        {"London",      deg2rad(51.51), deg2rad(-0.13),   0.02},
        {"Singapore",   deg2rad(1.35),  deg2rad(103.82),  0.01},
        {"Sao Paulo",   deg2rad(-23.55), deg2rad(-46.63), 0.76},
    };

    NetworkGraph graph(constellation, groundStations, params.epoch);
    size_t numSats = constellation.size();

    DijkstraRouter dijkstra;
    GreedyGeographicRouter greedy;

    std::cout << "=== Part 1: Routing comparison at a single instant ===\n";
    std::cout << graph.time().toIsoString()
              << " (" << graph.nodeCount() << " nodes, " << graph.edgeCount() << " edges)\n\n";

    for (size_t i = 0; i < groundStations.size(); ++i) {
        for (size_t j = i + 1; j < groundStations.size(); ++j) {
            size_t nodeA = numSats + i;
            size_t nodeB = numSats + j;

            std::cout << groundStations[i].name() << " -> " << groundStations[j].name() << "\n";
            printResult(dijkstra.name(), dijkstra.findPath(graph, nodeA, nodeB));
            printResult(greedy.name(), greedy.findPath(graph, nodeA, nodeB));
            std::cout << "\n";
        }
    }

    std::cout << "Note: with only " << constellation.size() << " satellites (real Starlink shells\n"
              << "use thousands), coverage is sparse - some ground stations may have zero\n"
              << "satellites overhead at a given instant, so no path exists no matter how good\n"
              << "the routing algorithm is. This is a real, expected limitation of small\n"
              << "constellations, not a routing bug (both routers correctly report failure\n"
              << "together when a route is genuinely disconnected).\n\n";

    std::cout << "=== Part 2: Watching one route's latency change over time ===\n";
    std::cout << "Los Angeles -> London, sampled every 10 minutes across roughly one orbit:\n\n";

    size_t laNode = numSats + 0;
    size_t londonNode = numSats + 1;

    for (int i = 0; i <= 9; ++i) {
        JulianDate t = params.epoch.addSeconds(i * 600.0); // every 10 minutes
        NetworkGraph snapshot(constellation, groundStations, t);

        RoutingResult result = dijkstra.findPath(snapshot, laNode, londonNode);

        std::cout << "  t+" << std::setw(3) << (i * 10) << " min: ";
        if (result.success) {
            std::cout << "hops=" << result.hopCount
                       << "  delay=" << std::fixed << std::setprecision(2)
                       << result.totalDelaySeconds * 1000.0 << " ms\n";
        } else {
            std::cout << "no path available\n";
        }
    }

    return 0;
}
