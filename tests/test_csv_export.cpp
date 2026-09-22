#include "test_framework.hpp"
#include "metrics/CsvExport.hpp"
#include "routing/DijkstraRouter.hpp"

#include <fstream>
#include <sstream>

using namespace satsim;

namespace {
WalkerConstellationParams makeCsvTestParams() {
    WalkerConstellationParams p;
    p.inclinationRad = 53.0 * constants::kPi / 180.0;
    p.totalSatellites = 24;
    p.numPlanes = 6;
    p.phasingFactor = 1;
    p.altitudeKm = 550.0;
    p.eccentricity = 0.0;
    p.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);
    return p;
}

std::vector<GroundStation> makeCsvTestGroundStations() {
    auto deg2rad = [](double d) { return d * constants::kPi / 180.0; };
    return {
        GroundStation("Los Angeles", deg2rad(34.05), deg2rad(-118.24), 0.09),
        GroundStation("London", deg2rad(51.51), deg2rad(-0.13), 0.02),
    };
}
}

TEST_CASE(csv_export_writes_a_header_and_one_row_per_packet) {
    WalkerConstellationParams params = makeCsvTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeCsvTestGroundStations();

    DijkstraRouter router;
    SimulationParams simParams;
    simParams.durationSeconds = 60.0;
    simParams.meanPacketIntervalSeconds = 2.0;

    Simulator sim(constellation, stations, params.epoch, router, simParams);
    SimulationMetrics metrics = sim.run();

    std::string path = "/tmp/satsim_test_export.csv";
    bool success = metrics::exportPacketRecordsToCsv(metrics, path);
    CHECK_TRUE(success);

    std::ifstream in(path);
    CHECK_TRUE(in.is_open());

    std::string header;
    std::getline(in, header);
    CHECK_TRUE(header.find("packet_id") != std::string::npos);
    CHECK_TRUE(header.find("latency_ms") != std::string::npos);
    CHECK_TRUE(header.find("hop_count") != std::string::npos);

    int rowCount = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) ++rowCount;
    }

    CHECK_TRUE(rowCount == static_cast<int>(metrics.packetRecords.size()));
    CHECK_TRUE(rowCount == metrics.packetsGenerated);
}

TEST_CASE(csv_export_fails_gracefully_for_an_invalid_path) {
    SimulationMetrics metrics; // empty, doesn't matter for this test
    bool success = metrics::exportPacketRecordsToCsv(metrics, "/nonexistent_directory/out.csv");
    CHECK_TRUE(!success);
}

TEST_CASE(delivered_packets_have_positive_latency_and_dropped_packets_have_zero) {
    WalkerConstellationParams params = makeCsvTestParams();
    WalkerConstellation constellation(params);
    std::vector<GroundStation> stations = makeCsvTestGroundStations();

    DijkstraRouter router;
    SimulationParams simParams;
    simParams.durationSeconds = 120.0;
    simParams.meanPacketIntervalSeconds = 1.0;

    Simulator sim(constellation, stations, params.epoch, router, simParams);
    SimulationMetrics metrics = sim.run();

    for (const PacketRecord& record : metrics.packetRecords) {
        if (record.delivered) {
            CHECK_TRUE(record.latencySeconds > 0.0);
            CHECK_TRUE(record.hopCount > 0);
        } else {
            CHECK_NEAR(record.latencySeconds, 0.0, 1e-12);
            CHECK_TRUE(record.hopCount == 0);
        }
    }
}
