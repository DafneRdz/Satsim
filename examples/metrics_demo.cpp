// Phase 6 demo: run identical traffic through Dijkstra and greedy
// geographic routing, export per-packet results to CSV, and print a
// summary comparison. The CSVs are the input to scripts/plot_results.py.
//
// Build & run (from repo root):
//   cmake -B build && cmake --build build
//   ./build/metrics_demo
//   python3 scripts/plot_results.py

#include "sim/Simulator.hpp"
#include "routing/DijkstraRouter.hpp"
#include "routing/GreedyGeographicRouter.hpp"
#include "metrics/CsvExport.hpp"

#include <filesystem>
#include <iostream>
#include <iomanip>

using namespace satsim;

namespace {
void printSummary(const std::string& label, const SimulationMetrics& metrics) {
    std::cout << label << "\n";
    std::cout << "  Packets generated: " << metrics.packetsGenerated << "\n";
    std::cout << "  Packets delivered: " << metrics.packetsDelivered << "\n";
    std::cout << "  Packets dropped:   " << metrics.packetsDropped << "\n";
    std::cout << "  Delivery rate:     " << std::fixed << std::setprecision(1)
               << metrics.deliveryRate() * 100.0 << "%\n";
    std::cout << "  Average latency:   " << std::setprecision(2)
               << metrics.averageLatencySeconds() * 1000.0 << " ms\n";
    std::cout << "  Max latency:       " << metrics.maxLatencySeconds * 1000.0 << " ms\n\n";
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

    // Identical simulation parameters and random seed for both routers -
    // this means both runs generate the exact same sequence of packets
    // (same creation times, same source/destination pairs), so any
    // difference in the results is attributable purely to the routing
    // strategy, not to different traffic.
    SimulationParams simParams;
    simParams.durationSeconds = 300.0;
    simParams.topologyUpdateIntervalSeconds = 20.0;
    simParams.meanPacketIntervalSeconds = 0.5;
    simParams.linkCapacityBitsPerSec = 50.0e6; // 50 Mbps: enough congestion to be interesting
    simParams.randomSeed = 42;

    std::filesystem::create_directories("results");

    DijkstraRouter dijkstra;
    Simulator dijkstraSim(constellation, groundStations, params.epoch, dijkstra, simParams);
    SimulationMetrics dijkstraMetrics = dijkstraSim.run();
    printSummary("Dijkstra (global shortest path)", dijkstraMetrics);
    metrics::exportPacketRecordsToCsv(dijkstraMetrics, "results/dijkstra_results.csv");

    GreedyGeographicRouter greedy;
    Simulator greedySim(constellation, groundStations, params.epoch, greedy, simParams);
    SimulationMetrics greedyMetrics = greedySim.run();
    printSummary("Greedy geographic (local knowledge only)", greedyMetrics);
    metrics::exportPacketRecordsToCsv(greedyMetrics, "results/greedy_results.csv");

    std::cout << "Wrote results/dijkstra_results.csv and results/greedy_results.csv\n";
    std::cout << "Run: python3 scripts/plot_results.py\n";

    return 0;
}
