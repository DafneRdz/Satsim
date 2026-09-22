#pragma once

#include "routing/Router.hpp"

namespace satsim {

// Shortest-path routing using Dijkstra's algorithm, minimizing total
// propagation delay. This assumes full, instantaneous knowledge of the
// entire network topology - unrealistic for a real distributed system, but
// a useful baseline: it always finds the objectively best path if one
// exists, so every other routing strategy can be measured against it.
class DijkstraRouter : public Router {
public:
    RoutingResult findPath(const NetworkGraph& graph,
                            size_t sourceNode,
                            size_t destinationNode) const override;

    std::string name() const override { return "Dijkstra (global shortest path)"; }
};

} // namespace satsim
