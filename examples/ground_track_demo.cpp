// Phase 1 demo: define a LEO satellite via Keplerian elements, propagate it
// through one full orbit, and print its ground track (sub-satellite lat/lon).
//
// Build & run (from repo root):
//   cmake -B build && cmake --build build
//   ./build/ground_track_demo

#include "orbit/OrbitPropagator.hpp"
#include "orbit/KeplerianElements.hpp"
#include "orbit/Time.hpp"

#include <iostream>
#include <iomanip>

using namespace satsim;

int main() {
    // Define a satellite in a 550 km circular orbit at 53 degree inclination
    // - similar parameters to a Starlink shell.
    KeplerianElements elems;
    elems.semiMajorAxisKm = constants::kEarthRadius + 550.0;
    elems.eccentricity = 0.0;
    elems.inclinationRad = 53.0 * constants::kPi / 180.0;
    elems.raanRad = 0.0;
    elems.argPerigeeRad = 0.0;
    elems.meanAnomalyRad = 0.0;
    elems.epoch = JulianDate::fromCalendar(2026, 1, 1, 0, 0, 0.0);

    OrbitPropagator propagator(elems);

    std::cout << "Satellite orbital period: "
              << elems.periodSeconds() / 60.0 << " minutes\n\n";

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  t (min)   lat (deg)   lon (deg)   altitude (km)\n";
    std::cout << "  -------   ---------   ---------   -------------\n";

    int steps = 20;
    double periodSec = elems.periodSeconds();

    for (int i = 0; i <= steps; ++i) {
        double tSeconds = (periodSec / steps) * i;
        JulianDate t = elems.epoch.addSeconds(tSeconds);

        double lat, lon;
        propagator.groundTrackAt(t, lat, lon);

        StateVector state = propagator.stateAt(t);
        double altitude = state.positionKm.norm() - constants::kEarthRadius;

        std::cout << "  " << std::setw(7) << tSeconds / 60.0
                  << "   " << std::setw(9) << lat * 180.0 / constants::kPi
                  << "   " << std::setw(9) << lon * 180.0 / constants::kPi
                  << "   " << std::setw(13) << altitude
                  << "\n";
    }

    return 0;
}
