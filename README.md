# satsim - LEO Satellite Constellation Network Simulator

A C++ simulator for low-Earth-orbit (LEO) satellite constellations, modeling
both the orbital mechanics (where satellites actually are over time) and
the network layer (how packets route across a topology that's constantly
changing as satellites move in and out of range of each other and the
ground) - similar in spirit to how Starlink or Iridium operate.

## Why this project

Satellite mega-constellations are a genuinely hard networking problem: unlike
a data center, the topology isn't static - links appear and disappear as
satellites orbit, so routing has to adapt continuously. This project builds
that up from first principles: real orbital mechanics, a time-varying network
graph, and multiple routing strategies compared against each other.

## Status

Phase 4 complete: Routing algorithms.
See [docs/roadmap.md](docs/roadmap.md) for the full build plan (discrete-event
simulation -> metrics -> stretch goals).

## What's implemented so far

**Phase 1 - Orbital mechanics core**
- `Vector3` - 3D vector math (dot/cross product, normalization, angles)
- `JulianDate` - astronomical time system with GMST for ECI/ECEF conversion
- `KeplerianElements` - the six classical orbital elements
- `OrbitPropagator` - two-body Keplerian propagator:
  - Solves Kepler's equation via Newton-Raphson
  - Converts orbital elements into Cartesian position/velocity (ECI frame)
  - Computes ground tracks (sub-satellite latitude/longitude over time)

**Phase 2 - Constellation generation**
- `WalkerConstellation` - generates a full Walker-Delta pattern (the same
  design approach real systems like Starlink, Iridium, and GPS use):
  evenly-spaced orbital planes, evenly-spaced satellites per plane, and a
  phasing factor that offsets planes relative to each other for uniform
  global coverage
- `GroundStation` - fixed points on Earth's surface, with position in both
  the rotating ECEF frame and the inertial ECI frame

**Phase 3 - Dynamic network graph**
- `visibility::hasLineOfSight` - determines whether two points can see each
  other, or whether Earth blocks the line between them
- `visibility::isVisibleFromGroundStation` - elevation-angle-based check for
  satellite-to-ground visibility, with a configurable minimum elevation
- `NetworkGraph` - builds a full snapshot of the network topology at a given
  instant: nodes for every satellite and ground station, edges for every
  visible link, with propagation delay computed from distance and the speed
  of light

**Phase 4 - Routing algorithms**
- `Router` - a common interface so different routing strategies can be
  swapped and compared on the same topology
- `DijkstraRouter` - global-knowledge shortest-path routing, minimizing total
  propagation delay; always finds the objectively best path if one exists
- `GreedyGeographicRouter` - distributed routing using only local knowledge
  (a node's own position and its immediate neighbors' positions), which is
  much closer to how a real satellite network operates; can fail at a
  "local minimum" where every neighbor is farther from the destination than
  the current node, a realistic limitation Dijkstra never has

Verified against known physical behavior: a 550 km circular orbit at 53
degree inclination (Starlink-like parameters) produces a ~95.6 minute
period, a ground track that oscillates within +/-53 degrees latitude,
velocity that stays perpendicular to position, a generated 60-satellite/
12-plane constellation has satellites and planes evenly spaced exactly as
the Walker pattern requires, the network graph correctly identifies which
satellites can see each other and the ground, Dijkstra always finds a path
at least as good as greedy routing whenever greedy succeeds, and both
routers correctly agree when a route is genuinely impossible due to a
coverage gap - all confirmed by the test suite (38 tests, ~2,000
assertions).

### A note on orbital plane crossings

Running the network graph demo, you may notice two satellites in different
planes occasionally sitting at (almost) the exact same position. This isn't
a bug: any two orbital planes around a sphere intersect at exactly two
antipodal points (since both are great circles through Earth's center), and
depending on the constellation's phasing factor, two satellites can cross
that intersection point at the same moment. Real constellation designs have
to choose their phasing carefully to manage this - it's a genuine design
consideration, not a simulation artifact.

### A note on coverage gaps

With only 60 satellites (real Starlink shells use thousands), the routing
demo will sometimes show both routers failing to find a path between two
ground stations. This is also not a bug: at that exact instant, there may
genuinely be no satellite overhead near one of the stations, so no path
exists no matter how good the routing algorithm is. This is a real,
expected limitation of small constellations, and a good illustration of
why real systems need either many more satellites or additional
coordination between shells.

## Building

Requires CMake 3.16+ and a C++17 compiler.

```bash
cmake -B build
cmake --build build

./build/ground_track_demo    # propagate a satellite and print its ground track
./build/constellation_demo   # generate a 60-satellite Walker constellation + ground stations
./build/network_graph_demo   # build the dynamic network graph and show it changing over time
./build/routing_demo         # compare Dijkstra vs greedy geographic routing
./build/satsim_tests         # run the unit test suite
```

## Example output

```
=== Part 1: Routing comparison at a single instant ===
2026-01-01T00:00:00Z (64 nodes, 196 edges)

Los Angeles -> London
  Dijkstra (global shortest path)          hops=5   delay=36.59 ms
  Greedy geographic (local knowledge only) hops=5   delay=36.59 ms

Los Angeles -> Singapore
  Dijkstra (global shortest path)          FAILED to find a path
  Greedy geographic (local knowledge only) FAILED to find a path

=== Part 2: Watching one route's latency change over time ===
Los Angeles -> London, sampled every 10 minutes across roughly one orbit:

  t+0   min: hops=5  delay=36.59 ms
  t+10  min: hops=5  delay=40.92 ms
  t+20  min: hops=5  delay=36.66 ms
  ...
```

## Project layout

```
include/orbit/           - Vector3, Time, KeplerianElements, OrbitPropagator
include/constellation/   - WalkerConstellation, GroundStation
include/network/         - Visibility, NetworkGraph
include/routing/         - Router, DijkstraRouter, GreedyGeographicRouter
src/                     - Implementation, mirroring the include/ layout
tests/           - Unit tests (dependency-free custom test framework)
examples/        - Demo programs
docs/roadmap.md  - Full multi-phase build plan
```

## Design notes

The propagator currently models unperturbed two-body dynamics - accurate
enough to validate the geometry and network layers, with room to add J2
oblateness perturbation later without changing the public interface (see
roadmap). Kepler's equation is solved with Newton-Raphson rather than a
closed form since no closed-form solution exists for elliptical orbits.

The network graph uses a spherical Earth model for occlusion and elevation
checks - a reasonable approximation at these altitudes, with the same
extension path (WGS84 oblate spheroid) noted in the roadmap if more
precision is ever needed.

Routing strategies share a common `Router` interface specifically so new
strategies can be added and benchmarked against the existing ones without
touching any calling code - this is the same seam Phase 5's discrete-event
simulation will plug into.

## License

MIT - see [LICENSE](LICENSE).
