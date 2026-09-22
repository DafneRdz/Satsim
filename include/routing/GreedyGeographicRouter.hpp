#pragma once

#include "routing/Router.hpp"

namespace satsim {

// Greedy geographic routing: at each hop, forward to whichever neighbor is
// physically closest to the destination, using only local information (the
// current node's position and its immediate neighbors' positions) rather
// than any knowledge of the full network topology. This is much closer to
// how a real distributed satellite network would actually operate, since
// no single satellite has an up-to-the-millisecond view of the entire
// constellation - but it comes with a real weakness: it can get stuck at a
// "local minimum," a node where every neighbor is farther from the
// destination than the current node is, even though a longer detour would
// reach the destination. When that happens, this router reports failure
// rather than looping forever - a realistic outcome that Dijkstra, with its
// global view, never suffers from.
class GreedyGeographicRouter : public Router {
public:
    explicit GreedyGeographicRouter(int maxHops = 60) : maxHops_(maxHops) {}

    RoutingResult findPath(const NetworkGraph& graph,
                            size_t sourceNode,
                            size_t destinationNode) const override;

    std::string name() const override { return "Greedy geographic (local knowledge only)"; }

private:
    int maxHops_;
};

} // namespace satsim
