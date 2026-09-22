#pragma once

#include "network/NetworkGraph.hpp"

#include <string>
#include <vector>

namespace satsim {

// The outcome of attempting to route between two nodes: either a full path
// with its accumulated cost, or a failure (e.g. no path exists, or a
// distributed strategy got stuck without global knowledge to recover).
struct RoutingResult {
    bool success = false;
    std::vector<size_t> path;      // node indices, source first, destination last
    double totalDelaySeconds = 0.0;
    double totalDistanceKm = 0.0;
    int hopCount = 0;
};

// Common interface for routing strategies. Different strategies make very
// different tradeoffs: a strategy with full knowledge of the topology (like
// Dijkstra) can always find the optimal path if one exists, while a
// strategy that only knows about its immediate neighbors (like greedy
// geographic routing) is more realistic for how a real distributed
// satellite network would actually operate, at the cost of sometimes
// failing to find a path that does exist.
class Router {
public:
    virtual ~Router() = default;

    virtual RoutingResult findPath(const NetworkGraph& graph,
                                    size_t sourceNode,
                                    size_t destinationNode) const = 0;

    virtual std::string name() const = 0;

protected:
    // Shared helper: given a graph and a path of node indices, computes the
    // accumulated delay/distance by looking up each hop's edge. Used by
    // every concrete router to build a consistent RoutingResult.
    static RoutingResult buildResultFromPath(const NetworkGraph& graph,
                                              const std::vector<size_t>& path) {
        RoutingResult result;
        if (path.empty()) return result;

        result.path = path;
        result.hopCount = static_cast<int>(path.size()) - 1;

        for (size_t i = 0; i + 1 < path.size(); ++i) {
            auto edgeIdx = graph.findEdgeBetween(path[i], path[i + 1]);
            if (!edgeIdx.has_value()) {
                // Path referenced a hop with no actual edge - treat as failure
                // rather than silently under-counting cost.
                return RoutingResult{};
            }
            const NetworkEdge& edge = graph.edges()[*edgeIdx];
            result.totalDelaySeconds += edge.delaySeconds;
            result.totalDistanceKm += edge.distanceKm;
        }

        result.success = true;
        return result;
    }
};

} // namespace satsim
