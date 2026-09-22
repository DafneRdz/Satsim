#include "metrics/CsvExport.hpp"

#include <fstream>
#include <iomanip>

namespace satsim {
namespace metrics {

bool exportPacketRecordsToCsv(const SimulationMetrics& metrics, const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    out << "packet_id,source_node,destination_node,creation_time_seconds,"
        << "delivered,latency_ms,hop_count\n";

    out << std::fixed << std::setprecision(6);

    for (const PacketRecord& record : metrics.packetRecords) {
        out << record.packetId << ","
            << record.sourceNode << ","
            << record.destinationNode << ","
            << record.creationTimeSeconds << ","
            << (record.delivered ? 1 : 0) << ","
            << record.latencySeconds * 1000.0 << ","
            << record.hopCount << "\n";
    }

    return true;
}

} // namespace metrics
} // namespace satsim
