#pragma once

#include "sim/Simulator.hpp"

#include <string>

namespace satsim {
namespace metrics {

// Writes every packet's record (whether delivered or dropped, its latency,
// hop count, and endpoints) to a CSV file, one row per packet. This is the
// raw data behind scripts/plot_results.py - deliberately per-packet rather
// than pre-aggregated, so any analysis (percentiles, distributions,
// filtering by route) can be done afterward without re-running the
// simulation.
//
// Returns true on success, false if the file could not be opened for
// writing (e.g. the containing directory doesn't exist).
bool exportPacketRecordsToCsv(const SimulationMetrics& metrics, const std::string& filePath);

} // namespace metrics
} // namespace satsim
