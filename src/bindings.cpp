#include <emscripten/bind.h>
#include <vector>
#include <string>

// Include your existing Satsim header files
#include "Satellite.h"
#include "Constellation.h"
#include "NetworkRouter.h"

using namespace emscripten;

// Expose C++ structures and classes to JavaScript
EMSCRIPTEN_BINDINGS(satsim_module) {
    
    // 1. Expose Satellite struct/class
    class_<Satellite>("Satellite")
        .constructor<int, double, double, double>() // id, lat, lon, alt
        .property("id", &Satellite::id)
        .property("x", &Satellite::x)
        .property("y", &Satellite::y)
        .property("z", &Satellite::z)
        .function("getPosition", &Satellite::getPosition);

    // 2. Expose Link/Edge struct
    value_object<Link>("Link")
        .field("sourceId", &Link::sourceId)
        .field("targetId", &Link::targetId)
        .field("distanceKm", &Link::distanceKm);

    // 3. Expose Constellation manager
    class_<Constellation>("Constellation")
        .constructor<int, int, double>() // numPlanes, satsPerPlane, altitude
        .function("updatePositions", &Constellation::updatePositions) // Step simulation
        .function("getSatellites", &Constellation::getSatellites)
        .function("computeTopology", &Constellation::computeTopology);

    // 4. Expose Routing solver
    class_<NetworkRouter>("NetworkRouter")
        .constructor<>()
        .function("computeShortestPath", &NetworkRouter::computeShortestPath);

    // Register std::vector bindings so JS can handle arrays returned by C++
    register_vector<Satellite>("VectorSatellite");
    register_vector<Link>("VectorLink");
    register_vector<int>("VectorInt");
}
