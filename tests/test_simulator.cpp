#include "test_framework.hpp"
#include "sim/Simulator.hpp"
#include "routing/DijkstraRouter.hpp"

using namespace satsim;

namespace {
WalkerConstellationParams makeSimTestParams() {
    WalkerConstellationParams p;
    p.inclinationRad = 53.0 * constants::kPi / 180.0;
    p.totalSatellites = 60;
    p.numPlanes = 12;
    p.phasingFactor = 1;
    p.altitudeKm = 550.0;
    p.eccentricity = 0.0;
    p.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);
    return p;
}

std::vector<GroundStation> makeSimTestGroundStations() {
    auto deg2rad = [](double d) { return d * constants::kPi / 180.0; };
    return {
        GroundStation("Los Angeles", deg2rad(34.05), deg2rad(-118.24), 0.09),
        GroundStation("London", deg2rad(51.51), deg2rad(-0.13), 0.02),
    };
}
}

TEST_CASE(simulation_accounts_for_every_generated_packet) {
    WalkerConstellationParams params = makeSimTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeSimTestGroundStations();

    DijkstraRouter router;
    SimulationParams simParams;
    simParams.durationSeconds = 300.0;
    simParams.meanPacketIntervalSeconds = 2.0;

    Simulator sim(constellation, stations, params.epoch, router, simParams);
    SimulationMetrics metrics = sim.run();

    CHECK_TRUE(metrics.packetsGenerated > 0);
    CHECK_TRUE(metrics.packetsDelivered + metrics.packetsDropped == metrics.packetsGenerated);
}

TEST_CASE(delivery_rate_and_average_latency_are_well_formed) {
    WalkerConstellationParams params = makeSimTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeSimTestGroundStations();

    DijkstraRouter router;
    SimulationParams simParams;
    simParams.durationSeconds = 300.0;
    simParams.meanPacketIntervalSeconds = 2.0;

    Simulator sim(constellation, stations, params.epoch, router, simParams);
    SimulationMetrics metrics = sim.run();

    CHECK_TRUE(metrics.deliveryRate() >= 0.0 && metrics.deliveryRate() <= 1.0);
    if (metrics.packetsDelivered > 0) {
        CHECK_TRUE(metrics.averageLatencySeconds() > 0.0);
        CHECK_TRUE(metrics.averageLatencySeconds() <= metrics.maxLatencySeconds + 1e-9);
    }
}

TEST_CASE(topology_is_rebuilt_the_expected_number_of_times) {
    WalkerConstellationParams params = makeSimTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeSimTestGroundStations();

    DijkstraRouter router;
    SimulationParams simParams;
    simParams.durationSeconds = 100.0;
    simParams.topologyUpdateIntervalSeconds = 25.0;
    simParams.meanPacketIntervalSeconds = 1000.0; // effectively no traffic, isolate topology updates

    Simulator sim(constellation, stations, params.epoch, router, simParams);
    SimulationMetrics metrics = sim.run();

    // One initial build at t=0, then one every 25s up to (but not past) 100s:
    // t=25, 50, 75, 100 -> 4 more, for 5 total.
    CHECK_TRUE(metrics.topologyUpdatesPerformed == 5);
}

TEST_CASE(congestion_from_limited_link_capacity_increases_average_latency) {
    // The core claim of the queuing model: with heavy traffic sharing a
    // link of limited capacity, packets should experience meaningfully
    // higher average latency than the same traffic pattern over an
    // effectively uncongested (very high capacity) link.
    WalkerConstellationParams params = makeSimTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeSimTestGroundStations();
    DijkstraRouter router;

    SimulationParams uncongestedParams;
    uncongestedParams.durationSeconds = 120.0;
    uncongestedParams.meanPacketIntervalSeconds = 0.02; // heavy traffic: ~6000 packets over 120s
    uncongestedParams.packetSizeBits = 1500.0 * 8.0;
    uncongestedParams.linkCapacityBitsPerSec = 10.0e9; // 10 Gbps: effectively uncongested at this traffic rate

    SimulationParams congestedParams = uncongestedParams;
    congestedParams.linkCapacityBitsPerSec = 2.0e6; // 2 Mbps: heavily congested at this traffic rate

    Simulator uncongestedSim(constellation, stations, params.epoch, router, uncongestedParams);
    SimulationMetrics uncongestedMetrics = uncongestedSim.run();

    Simulator congestedSim(constellation, stations, params.epoch, router, congestedParams);
    SimulationMetrics congestedMetrics = congestedSim.run();

    CHECK_TRUE(uncongestedMetrics.packetsDelivered > 0);
    CHECK_TRUE(congestedMetrics.packetsDelivered > 0);
    CHECK_TRUE(congestedMetrics.averageLatencySeconds() > uncongestedMetrics.averageLatencySeconds());
}

TEST_CASE(simulation_is_deterministic_given_the_same_seed) {
    // Reproducibility matters for a simulator: the same parameters and
    // random seed should always produce identical results.
    WalkerConstellationParams params = makeSimTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeSimTestGroundStations();
    DijkstraRouter router;

    SimulationParams simParams;
    simParams.durationSeconds = 100.0;
    simParams.meanPacketIntervalSeconds = 1.0;
    simParams.randomSeed = 7;

    Simulator sim1(constellation, stations, params.epoch, router, simParams);
    SimulationMetrics metrics1 = sim1.run();

    Simulator sim2(constellation, stations, params.epoch, router, simParams);
    SimulationMetrics metrics2 = sim2.run();

    CHECK_TRUE(metrics1.packetsGenerated == metrics2.packetsGenerated);
    CHECK_TRUE(metrics1.packetsDelivered == metrics2.packetsDelivered);
    CHECK_NEAR(metrics1.totalLatencySeconds, metrics2.totalLatencySeconds, 1e-9);
}
