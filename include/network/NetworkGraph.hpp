#pragma once

#include "constellation/WalkerConstellation.hpp"
#include "constellation/GroundStation.hpp"
#include "orbit/KeplerianElements.hpp" // for constants::kPi, constants::kEarthRadius
#include "orbit/Time.hpp"
#include "orbit/Vector3.hpp"

#include <optional>
#include <string>
#include <vector>

namespace satsim {

// Physical constant: speed of light, used to convert distance into
// propagation delay for each link.
namespace constants {
    constexpr double kSpeedOfLightKmPerSec = 299792.458;
}

// A node in the network graph: either a satellite or a ground station,
// identified by its index into a combined node list.
struct NetworkNode {
    enum class Type { Satellite, GroundStation };

    Type type;
    std::string id;
    Vector3 positionKm; // position at the graph's snapshot time, in ECI frame
};

// An edge (link) in the network graph: a pair of nodes that currently have
// visibility to each other, with the propagation delay that a signal
// between them would experience.
struct NetworkEdge {
    size_t nodeA;
    size_t nodeB;
    double distanceKm;
    double delaySeconds;
};

// Parameters controlling how the network graph decides whether a link
// exists between two nodes.
struct NetworkGraphParams {
    double minGroundElevationRad = 10.0 * constants::kPi / 180.0; // 10 degrees
    double maxIslRangeKm = 5000.0; // max realistic laser inter-satellite link range
};

// A snapshot of the satellite network's topology at a single instant in
// time. Rebuilding this at successive timesteps is what makes the network
// "dynamic" - links appear and disappear as satellites move in and out of
// view of each other and of ground stations.
class NetworkGraph {
public:
    NetworkGraph(const WalkerConstellation& constellation,
                 const std::vector<GroundStation>& groundStations,
                 const JulianDate& time,
                 const NetworkGraphParams& params = NetworkGraphParams{});

    const std::vector<NetworkNode>& nodes() const { return nodes_; }
    const std::vector<NetworkEdge>& edges() const { return edges_; }

    // The indices of edges incident to a given node - i.e. its neighbors
    // in the graph. Used by routing algorithms to traverse the topology.
    const std::vector<size_t>& edgesAt(size_t nodeIndex) const { return adjacency_[nodeIndex]; }

    size_t nodeCount() const { return nodes_.size(); }
    size_t edgeCount() const { return edges_.size(); }

    const JulianDate& time() const { return time_; }

    // Finds the edge directly connecting two nodes, if one exists. Used by
    // routing algorithms to look up the delay/distance of a specific hop
    // once they've decided which neighbor to forward to.
    std::optional<size_t> findEdgeBetween(size_t nodeA, size_t nodeB) const {
        for (size_t edgeIdx : adjacency_[nodeA]) {
            const NetworkEdge& edge = edges_[edgeIdx];
            if (edge.nodeA == nodeB || edge.nodeB == nodeB) return edgeIdx;
        }
        return std::nullopt;
    }

    // Given an edge and one of its endpoints, returns the index of the
    // other endpoint - a small convenience for walking the graph.
    static size_t otherEndOf(const NetworkEdge& edge, size_t knownEndpoint) {
        return (edge.nodeA == knownEndpoint) ? edge.nodeB : edge.nodeA;
    }

private:
    JulianDate time_;
    NetworkGraphParams params_;
    std::vector<NetworkNode> nodes_;
    std::vector<NetworkEdge> edges_;
    std::vector<std::vector<size_t>> adjacency_; // per-node list of edge indices

    void build(const WalkerConstellation& constellation,
               const std::vector<GroundStation>& groundStations);

    void addEdgeIfVisible(size_t nodeA, size_t nodeB);
};

} // namespace satsim
