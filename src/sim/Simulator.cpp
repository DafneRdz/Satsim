#include "sim/Simulator.hpp"

#include <algorithm>

namespace satsim {

// A packet actively traveling across the network. It carries its full
// source-routed path (computed once, at generation time, from whichever
// network graph snapshot was current then) plus enough state to track its
// progress hop by hop. Holding a shared_ptr to the graph it was routed on
// keeps that snapshot alive for the packet's whole journey, even if the
// simulator has since rebuilt a newer one - the same way a real packet's
// route isn't retroactively changed just because the network state moved
// on after it was sent.
struct InFlightPacket {
    std::vector<size_t> path;
    size_t nextHopIndex = 0;
    double creationTime = 0.0;
    std::shared_ptr<NetworkGraph> graph;
    size_t recordIndex = 0; // index into metrics_.packetRecords for this packet
};

Simulator::Simulator(const WalkerConstellation& constellation,
                      std::vector<GroundStation> groundStations,
                      const JulianDate& startTime,
                      const Router& router,
                      const SimulationParams& params)
    : constellation_(constellation),
      groundStations_(std::move(groundStations)),
      startTime_(startTime),
      router_(router),
      params_(params),
      rng_(params.randomSeed),
      interArrivalDist_(1.0 / params.meanPacketIntervalSeconds),
      groundStationIndexDist_(0, groundStations_.size() - 1) {}

uint64_t Simulator::linkKey(size_t a, size_t b) {
    if (a > b) std::swap(a, b);
    return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
}

SimulationMetrics Simulator::run() {
    // Build the initial topology snapshot and kick off the two recurring
    // event chains: periodic topology rebuilds, and packet generation.
    currentGraph_ = std::make_shared<NetworkGraph>(constellation_, groundStations_, startTime_);
    metrics_.topologyUpdatesPerformed = 1;

    scheduleTopologyUpdate();
    scheduleNextPacketGeneration();

    while (!eventQueue_.empty()) {
        EventQueue::Entry entry = eventQueue_.popNext();
        if (entry.time > params_.durationSeconds) break;

        currentTime_ = entry.time;
        entry.action();
    }

    return metrics_;
}

void Simulator::scheduleTopologyUpdate() {
    double nextTime = currentTime_ + params_.topologyUpdateIntervalSeconds;
    if (nextTime > params_.durationSeconds) return;

    eventQueue_.schedule(nextTime, [this]() { handleTopologyUpdate(); });
}

void Simulator::handleTopologyUpdate() {
    JulianDate t = startTime_.addSeconds(currentTime_);
    currentGraph_ = std::make_shared<NetworkGraph>(constellation_, groundStations_, t);
    ++metrics_.topologyUpdatesPerformed;

    scheduleTopologyUpdate();
}

void Simulator::scheduleNextPacketGeneration() {
    double interArrival = interArrivalDist_(rng_);
    double nextTime = currentTime_ + interArrival;
    if (nextTime > params_.durationSeconds) return;

    eventQueue_.schedule(nextTime, [this]() { handlePacketGeneration(); });
}

void Simulator::handlePacketGeneration() {
    size_t numSatellites = constellation_.size();

    size_t stationA = groundStationIndexDist_(rng_);
    size_t stationB;
    do {
        stationB = groundStationIndexDist_(rng_);
    } while (stationB == stationA && groundStations_.size() > 1);

    routeAndSimulatePacket(numSatellites + stationA, numSatellites + stationB);

    scheduleNextPacketGeneration();
}

void Simulator::routeAndSimulatePacket(size_t sourceNode, size_t destinationNode) {
    ++metrics_.packetsGenerated;

    PacketRecord record;
    record.packetId = nextPacketId_++;
    record.sourceNode = sourceNode;
    record.destinationNode = destinationNode;
    record.creationTimeSeconds = currentTime_;
    record.delivered = false;

    RoutingResult result = router_.findPath(*currentGraph_, sourceNode, destinationNode);
    if (!result.success) {
        ++metrics_.packetsDropped;
        metrics_.packetRecords.push_back(record);
        return;
    }

    size_t recordIndex = metrics_.packetRecords.size();
    metrics_.packetRecords.push_back(record);

    auto packet = std::make_shared<InFlightPacket>();
    packet->path = result.path;
    packet->nextHopIndex = 0;
    packet->creationTime = currentTime_;
    packet->graph = currentGraph_;
    packet->recordIndex = recordIndex;

    scheduleHop(packet);
}

void Simulator::scheduleHop(std::shared_ptr<InFlightPacket> packet) {
    // The packet has traversed its entire path once nextHopIndex points
    // past the final node - i.e. it has arrived at its destination.
    if (packet->nextHopIndex + 1 >= packet->path.size()) {
        double latency = currentTime_ - packet->creationTime;
        ++metrics_.packetsDelivered;
        metrics_.totalLatencySeconds += latency;
        metrics_.maxLatencySeconds = std::max(metrics_.maxLatencySeconds, latency);

        PacketRecord& record = metrics_.packetRecords[packet->recordIndex];
        record.delivered = true;
        record.latencySeconds = latency;
        record.hopCount = static_cast<int>(packet->path.size()) - 1;
        return;
    }

    size_t fromNode = packet->path[packet->nextHopIndex];
    size_t toNode = packet->path[packet->nextHopIndex + 1];

    auto edgeIdx = packet->graph->findEdgeBetween(fromNode, toNode);
    // The edge is guaranteed to exist: this path came directly from a
    // routing result computed on this exact graph snapshot.
    const NetworkEdge& edge = packet->graph->edges()[*edgeIdx];

    double transmissionSeconds = params_.packetSizeBits / params_.linkCapacityBitsPerSec;

    // First-come-first-served queuing: if this link is still busy sending
    // an earlier packet, this packet has to wait for it to finish. This is
    // what allows heavy traffic to congest a link and inflate latency,
    // separately from the fixed propagation delay of the link itself.
    uint64_t key = linkKey(fromNode, toNode);
    auto it = linkBusyUntil_.find(key);
    double linkFreeAt = (it != linkBusyUntil_.end()) ? it->second : 0.0;

    double transmitStart = std::max(currentTime_, linkFreeAt);
    double transmitFinish = transmitStart + transmissionSeconds;
    linkBusyUntil_[key] = transmitFinish;

    double arrivalAtNextNode = transmitFinish + edge.delaySeconds;

    packet->nextHopIndex += 1;
    eventQueue_.schedule(arrivalAtNextNode, [this, packet]() { scheduleHop(packet); });
}

} // namespace satsim
