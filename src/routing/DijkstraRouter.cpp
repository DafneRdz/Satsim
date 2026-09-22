#include "routing/DijkstraRouter.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

namespace satsim {

RoutingResult DijkstraRouter::findPath(const NetworkGraph& graph,
                                        size_t sourceNode,
                                        size_t destinationNode) const {
    const double kInfinity = std::numeric_limits<double>::infinity();

    std::vector<double> distance(graph.nodeCount(), kInfinity);
    std::vector<size_t> previous(graph.nodeCount(), graph.nodeCount()); // sentinel = "no predecessor"
    std::vector<bool> visited(graph.nodeCount(), false);

    // Min-heap of (distance, node), using a vector + greater-than comparator
    // so the smallest distance is popped first.
    using QueueEntry = std::pair<double, size_t>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;

    distance[sourceNode] = 0.0;
    queue.push({0.0, sourceNode});

    while (!queue.empty()) {
        auto [dist, node] = queue.top();
        queue.pop();

        if (visited[node]) continue;
        visited[node] = true;

        if (node == destinationNode) break; // shortest path to destination is finalized

        for (size_t edgeIdx : graph.edgesAt(node)) {
            const NetworkEdge& edge = graph.edges()[edgeIdx];
            size_t neighbor = NetworkGraph::otherEndOf(edge, node);

            double candidateDistance = dist + edge.delaySeconds;
            if (candidateDistance < distance[neighbor]) {
                distance[neighbor] = candidateDistance;
                previous[neighbor] = node;
                queue.push({candidateDistance, neighbor});
            }
        }
    }

    if (distance[destinationNode] == kInfinity) {
        return RoutingResult{}; // no path exists at all
    }

    // Reconstruct the path by walking backward from destination to source.
    std::vector<size_t> path;
    for (size_t at = destinationNode; ; at = previous[at]) {
        path.push_back(at);
        if (at == sourceNode) break;
    }
    std::reverse(path.begin(), path.end());

    return buildResultFromPath(graph, path);
}

} // namespace satsim
