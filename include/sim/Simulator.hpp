#pragma once

#include "sim/EventQueue.hpp"
#include "network/NetworkGraph.hpp"
#include "routing/Router.hpp"

#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

namespace satsim {

// Configuration for a simulation run.
struct SimulationParams {
    double durationSeconds = 600.0;            // total simulated time to run
    double topologyUpdateIntervalSeconds = 30.0; // how often the network graph is rebuilt from orbital positions
    double meanPacketIntervalSeconds = 1.0;    // average time between packet generations (Poisson process)
    double packetSizeBits = 1500.0 * 8.0;      // ~1500 byte packet, a typical MTU
    double linkCapacityBitsPerSec = 1.0e9;     // assumed capacity of every link (1 Gbps default)
    unsigned randomSeed = 42;                  // fixed seed for reproducible runs
};

// A record of a single packet's journey through the simulation - the raw
// material for the CSV export and analysis in Phase 6. Aggregate metrics
// (averages, totals) can always be recomputed from a full set of these
// records, but not the other way around, so this is the more fundamental
// data to keep.
struct PacketRecord {
    int packetId = 0;
    size_t sourceNode = 0;
    size_t destinationNode = 0;
    double creationTimeSeconds = 0.0;
    bool delivered = false;
    double latencySeconds = 0.0; // 0 for dropped packets
    int hopCount = 0;            // 0 for dropped packets
};

// Aggregate results from a simulation run.
struct SimulationMetrics {
    int packetsGenerated = 0;
    int packetsDropped = 0;      // no route existed at the moment the packet was generated
    int packetsDelivered = 0;
    int topologyUpdatesPerformed = 0;

    double totalLatencySeconds = 0.0; // sum over delivered packets only
    double maxLatencySeconds = 0.0;

    // The full per-packet history, in generation order - used for CSV
    // export and detailed offline analysis (see scripts/plot_results.py).
    std::vector<PacketRecord> packetRecords;

    double averageLatencySeconds() const {
        return packetsDelivered > 0 ? totalLatencySeconds / packetsDelivered : 0.0;
    }

    double deliveryRate() const {
        return packetsGenerated > 0
            ? static_cast<double>(packetsDelivered) / packetsGenerated
            : 0.0;
    }
};

// Forward-declared here and fully defined in the .cpp - callers never need
// to know its layout, only that packets are tracked via shared_ptr as they
// hop across the network.
struct InFlightPacket;

// A discrete-event simulation of a satellite network under traffic: ground
// stations generate packets at random, each packet is routed using the
// current network topology, and the simulator advances packets hop by hop,
// modeling both propagation delay (from the network graph) and queuing
// delay (when multiple packets compete for the same link's limited
// capacity). The network topology itself is periodically rebuilt to
// reflect satellite motion, so both the topology AND the traffic evolve
// together over the simulated duration.
class Simulator {
public:
    Simulator(const WalkerConstellation& constellation,
              std::vector<GroundStation> groundStations,
              const JulianDate& startTime,
              const Router& router,
              const SimulationParams& params);

    // Runs the simulation to completion and returns aggregate metrics.
    SimulationMetrics run();

private:
    const WalkerConstellation& constellation_;
    std::vector<GroundStation> groundStations_;
    JulianDate startTime_;
    const Router& router_;
    SimulationParams params_;

    EventQueue eventQueue_;
    double currentTime_ = 0.0; // seconds since startTime_

    std::shared_ptr<NetworkGraph> currentGraph_;

    std::mt19937 rng_;
    std::exponential_distribution<double> interArrivalDist_;
    std::uniform_int_distribution<size_t> groundStationIndexDist_;

    SimulationMetrics metrics_;
    int nextPacketId_ = 0;

    // Per-link "when does this link become free" tracker, modeling a
    // simple first-come-first-served transmission queue on every link.
    // Keyed by an order-independent combination of the two endpoint node
    // indices, since the underlying edge index can change whenever the
    // topology is rebuilt.
    std::unordered_map<uint64_t, double> linkBusyUntil_;

    void scheduleTopologyUpdate();
    void handleTopologyUpdate();

    void scheduleNextPacketGeneration();
    void handlePacketGeneration();

    void routeAndSimulatePacket(size_t sourceNode, size_t destinationNode);
    void scheduleHop(std::shared_ptr<InFlightPacket> packet);

    static uint64_t linkKey(size_t a, size_t b);
};

} // namespace satsim
