// Phase 3 demo: build a Walker constellation, add ground stations, and
// snapshot the network graph at two different times to show the topology
// changing as satellites move - the core idea behind this whole project.
//
// Build & run (from repo root):
//   cmake -B build && cmake --build build
//   ./build/network_graph_demo

#include "network/NetworkGraph.hpp"

#include <iostream>
#include <iomanip>

using namespace satsim;

namespace {
void printGraphSummary(const NetworkGraph& graph, const std::string& label) {
    int satSatEdges = 0;
    int satGroundEdges = 0;

    for (const NetworkEdge& edge : graph.edges()) {
        const NetworkNode& a = graph.nodes()[edge.nodeA];
        const NetworkNode& b = graph.nodes()[edge.nodeB];
        bool bothSatellites = (a.type == NetworkNode::Type::Satellite &&
                                b.type == NetworkNode::Type::Satellite);
        if (bothSatellites) ++satSatEdges;
        else ++satGroundEdges;
    }

    std::cout << label << " (" << graph.time().toIsoString() << ")\n";
    std::cout << "  Nodes: " << graph.nodeCount() << "\n";
    std::cout << "  Inter-satellite links: " << satSatEdges << "\n";
    std::cout << "  Satellite-to-ground links: " << satGroundEdges << "\n\n";
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
    };

    // Snapshot 1: at epoch.
    NetworkGraph graphAtEpoch(constellation, groundStations, params.epoch);
    printGraphSummary(graphAtEpoch, "Snapshot at epoch");

    // Snapshot 2: 20 minutes later - satellites have moved noticeably,
    // so the topology should look different.
    JulianDate later = params.epoch.addSeconds(20 * 60.0);
    NetworkGraph graphLater(constellation, groundStations, later);
    printGraphSummary(graphLater, "Snapshot 20 minutes later");

    // Show a couple of specific links and their propagation delay.
    std::cout << "Sample links at epoch:\n";
    std::cout << std::fixed << std::setprecision(3);
    int shown = 0;
    for (const NetworkEdge& edge : graphAtEpoch.edges()) {
        const NetworkNode& a = graphAtEpoch.nodes()[edge.nodeA];
        const NetworkNode& b = graphAtEpoch.nodes()[edge.nodeB];
        std::cout << "  " << a.id << " <-> " << b.id
                  << "   distance=" << edge.distanceKm << " km"
                  << "   delay=" << edge.delaySeconds * 1000.0 << " ms\n";
        if (++shown >= 8) break;
    }

    return 0;
}
