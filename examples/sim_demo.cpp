// Phase 5 demo: run the discrete-event simulation under two link-capacity
// scenarios to show the queuing model actually working - heavy traffic
// over a constrained link should show up as higher average latency.
//
// Build & run (from repo root):
//   cmake -B build && cmake --build build
//   ./build/sim_demo

#include "sim/Simulator.hpp"
#include "routing/DijkstraRouter.hpp"

#include <iostream>
#include <iomanip>

using namespace satsim;

namespace {
void printMetrics(const std::string& label, const SimulationMetrics& metrics) {
    std::cout << label << "\n";
    std::cout << "  Packets generated:     " << metrics.packetsGenerated << "\n";
    std::cout << "  Packets delivered:     " << metrics.packetsDelivered << "\n";
    std::cout << "  Packets dropped:       " << metrics.packetsDropped
               << " (no route existed at generation time)\n";
    std::cout << "  Delivery rate:         " << std::fixed << std::setprecision(1)
               << metrics.deliveryRate() * 100.0 << "%\n";
    std::cout << "  Average latency:       " << std::setprecision(2)
               << metrics.averageLatencySeconds() * 1000.0 << " ms\n";
    std::cout << "  Max latency:           "
               << metrics.maxLatencySeconds * 1000.0 << " ms\n";
    std::cout << "  Topology rebuilds:     " << metrics.topologyUpdatesPerformed << "\n\n";
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
    };

    DijkstraRouter router;

    // Scenario A: high link capacity, heavy traffic. The network can
    // absorb the load easily, so latency should be dominated almost
    // entirely by propagation delay.
    SimulationParams uncongestedParams;
    uncongestedParams.durationSeconds = 120.0;
    uncongestedParams.topologyUpdateIntervalSeconds = 20.0;
    uncongestedParams.meanPacketIntervalSeconds = 0.02; // heavy traffic
    uncongestedParams.linkCapacityBitsPerSec = 10.0e9;  // 10 Gbps

    Simulator uncongestedSim(constellation, groundStations, params.epoch, router, uncongestedParams);
    printMetrics("Scenario A: high-capacity links (10 Gbps)", uncongestedSim.run());

    // Scenario B: identical traffic pattern, but link capacity throttled
    // down to something a real, congested downlink might see. Packets now
    // have to queue behind each other on shared links.
    SimulationParams congestedParams = uncongestedParams;
    congestedParams.linkCapacityBitsPerSec = 2.0e6; // 2 Mbps

    Simulator congestedSim(constellation, groundStations, params.epoch, router, congestedParams);
    printMetrics("Scenario B: congested links (2 Mbps), same traffic", congestedSim.run());

    return 0;
}
