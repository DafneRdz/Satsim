#include "network/NetworkGraph.hpp"
#include "network/Visibility.hpp"

namespace satsim {

NetworkGraph::NetworkGraph(const WalkerConstellation& constellation,
                            const std::vector<GroundStation>& groundStations,
                            const JulianDate& time,
                            const NetworkGraphParams& params)
    : time_(time), params_(params) {
    build(constellation, groundStations);
}

void NetworkGraph::build(const WalkerConstellation& constellation,
                          const std::vector<GroundStation>& groundStations) {
    // 1. Create one node per satellite, at its position at this snapshot's time.
    for (const Satellite& sat : constellation.satellites()) {
        StateVector state = sat.propagator.stateAt(time_);
        nodes_.push_back(NetworkNode{NetworkNode::Type::Satellite, sat.id, state.positionKm});
    }

    // 2. Create one node per ground station, at its ECI position at this time
    //    (ground stations are fixed on Earth's surface, but their ECI
    //    coordinates rotate with the planet).
    for (const GroundStation& gs : groundStations) {
        Vector3 pos = gs.positionEciKm(time_);
        nodes_.push_back(NetworkNode{NetworkNode::Type::GroundStation, gs.name(), pos});
    }

    adjacency_.resize(nodes_.size());

    size_t numSatellites = constellation.satellites().size();

    // 3. Satellite-satellite links (inter-satellite links / ISLs): visible
    //    to each other and within a realistic maximum ISL range.
    for (size_t i = 0; i < numSatellites; ++i) {
        for (size_t j = i + 1; j < numSatellites; ++j) {
            addEdgeIfVisible(i, j);
        }
    }

    // 4. Satellite-ground station links: visible above the minimum
    //    elevation angle.
    for (size_t i = 0; i < numSatellites; ++i) {
        for (size_t j = numSatellites; j < nodes_.size(); ++j) {
            addEdgeIfVisible(i, j);
        }
    }
}

void NetworkGraph::addEdgeIfVisible(size_t nodeA, size_t nodeB) {
    const NetworkNode& a = nodes_[nodeA];
    const NetworkNode& b = nodes_[nodeB];

    bool isSatToSat = (a.type == NetworkNode::Type::Satellite &&
                        b.type == NetworkNode::Type::Satellite);

    bool visible;
    if (isSatToSat) {
        visible = visibility::hasLineOfSight(a.positionKm, b.positionKm, constants::kEarthRadius);

        double distance = a.positionKm.distanceTo(b.positionKm);
        if (distance > params_.maxIslRangeKm) visible = false;
    } else {
        // One of the two is a ground station - figure out which side is which.
        const Vector3& groundPos = (a.type == NetworkNode::Type::GroundStation)
            ? a.positionKm : b.positionKm;
        const Vector3& satPos = (a.type == NetworkNode::Type::GroundStation)
            ? b.positionKm : a.positionKm;

        visible = visibility::isVisibleFromGroundStation(
            groundPos, satPos, params_.minGroundElevationRad, constants::kEarthRadius);
    }

    if (!visible) return;

    double distanceKm = a.positionKm.distanceTo(b.positionKm);
    double delaySeconds = distanceKm / constants::kSpeedOfLightKmPerSec;

    size_t edgeIndex = edges_.size();
    edges_.push_back(NetworkEdge{nodeA, nodeB, distanceKm, delaySeconds});

    adjacency_[nodeA].push_back(edgeIndex);
    adjacency_[nodeB].push_back(edgeIndex);
}

} // namespace satsim
