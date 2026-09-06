# Roadmap

## Phase 1 - Orbital mechanics core (complete)
- [x] `Vector3` math library
- [x] `JulianDate` time system with GMST
- [x] `KeplerianElements`
- [x] `OrbitPropagator` (two-body Kepler propagation, ground tracks)
- [x] Unit tests validating physical correctness

## Phase 2 - Constellation generation (complete)
- [x] `WalkerConstellation` generator (planes x satellites-per-plane x inclination x phasing)
- [x] `GroundStation` (fixed lat/lon/altitude points, ECEF + ECI positions)
- [ ] Config loading (JSON) so constellation parameters aren't hardcoded (deferred)

## Phase 3 - Dynamic network graph (complete)
- [x] Line-of-sight visibility check between two satellites (Earth-occlusion test)
- [x] Visibility check between a satellite and a ground station (elevation angle mask)
- [x] `NetworkGraph`: nodes = satellites + ground stations, edges = visible links
- [x] Edge weight = propagation delay (distance / speed of light)
- [x] Snapshot the graph at a given timestep; verified edges change as satellites move

## Phase 4 - Routing algorithms (complete)
- [x] `Router` interface
- [x] Dijkstra shortest-path routing (global-knowledge baseline)
- [x] Greedy geographic routing (local-knowledge only - more realistic)
- [x] Compare path length / latency between strategies on the same topology

## Phase 5 - Discrete-event simulation engine
- [ ] Event queue (priority queue keyed by simulated time)
- [ ] Link up/down events driven by the orbital propagator
- [ ] Packet/flow objects with source, destination, size
- [ ] Traffic generation (e.g. Poisson arrivals between random ground station pairs)
- [ ] Queuing/congestion at links with finite capacity

## Phase 6 - Metrics and analysis
- [ ] Export latency, hop count, packet loss to CSV
- [ ] Python/matplotlib script to plot Dijkstra vs. greedy routing performance
- [ ] README section summarizing results

## Phase 7 - Stretch goals
- [ ] Ingest real TLE data from Celestrak instead of synthetic Keplerian elements
- [ ] J2 perturbation (oblateness) for more accurate long-duration propagation
- [ ] 2D lat/lon visualization of ground tracks and links
- [ ] GitHub Actions CI (build and run tests on every push)
- [ ] Satellite failure injection and resilience testing
- [ ] Config loading (JSON) so constellation parameters aren't hardcoded
