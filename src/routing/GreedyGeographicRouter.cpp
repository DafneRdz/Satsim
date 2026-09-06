#include "routing/GreedyGeographicRouter.hpp"

#include <unordered_set>

namespace satsim {

RoutingResult GreedyGeographicRouter::findPath(const NetworkGraph& graph,
                                                size_t sourceNode,
                                                size_t destinationNode) const {
    if (sourceNode == destinationNode) {
        return buildResultFromPath(graph, {sourceNode});
    }

    const Vector3& destinationPos = graph.nodes()[destinationNode].positionKm;

    std::vector<size_t> path;
    std::unordered_set<size_t> visited;

    size_t current = sourceNode;
    path.push_back(current);
    visited.insert(current);

    for (int hop = 0; hop < maxHops_; ++hop) {
        if (current == destinationNode) {
            return buildResultFromPath(graph, path);
        }

        double currentDistanceToDest = graph.nodes()[current].positionKm.distanceTo(destinationPos);

        // Look at every immediate neighbor - this is the "local knowledge
        // only" constraint: we never consult the full graph, only the
        // current node's own edge list.
        bool foundBetterNeighbor = false;
        size_t bestNeighbor = current;
        double bestDistance = currentDistanceToDest;

        for (size_t edgeIdx : graph.edgesAt(current)) {
            const NetworkEdge& edge = graph.edges()[edgeIdx];
            size_t neighbor = NetworkGraph::otherEndOf(edge, current);

            if (visited.count(neighbor) > 0) continue; // don't backtrack into a loop

            double neighborDistanceToDest = graph.nodes()[neighbor].positionKm.distanceTo(destinationPos);
            if (neighborDistanceToDest < bestDistance) {
                bestDistance = neighborDistanceToDest;
                bestNeighbor = neighbor;
                foundBetterNeighbor = true;
            }
        }

        if (!foundBetterNeighbor) {
            // Local minimum: every reachable, unvisited neighbor is farther
            // from the destination than we already are. A real distributed
            // protocol would need a fallback (e.g. "perimeter routing"
            // around the obstacle) to escape this - out of scope for now,
            // so we report failure rather than pretend to succeed.
            return RoutingResult{};
        }

        current = bestNeighbor;
        path.push_back(current);
        visited.insert(current);
    }

    return RoutingResult{}; // exceeded maxHops without reaching the destination
}

} // namespace satsim
