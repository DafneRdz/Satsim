// Global configuration state
const config = {
  planes: 6,
  satsPerPlane: 10,
  altitudeKm: 550,
  inclinationDeg: 53,
  algo: 'dijkstra',
  showISLs: true,
  simSpeed: 1.0
};

let wasmModule = null;
let satGroup = null;
let islGroup = null;
let satellites = [];
let earthRadius = 5; // Three.js units representing ~6371 km Earth radius

// Boot WASM module safely
if (typeof SatsimModule === 'function') {
  SatsimModule({
    locateFile: (path) => `wasm/${path}`
  }).then((Module) => {
    wasmModule = Module;
    const badge = document.getElementById('status-badge');
    if (badge) {
      badge.innerText = 'WASM Engine Active';
      badge.classList.add('active');
    }
    console.log('Satsim WASM Module initialized:', Module);
    rebuildConstellation();
  }).catch((err) => {
    console.error('WASM initialization error:', err);
    const badge = document.getElementById('status-badge');
    if (badge) badge.innerText = 'JS Math Engine (Fallback)';
    rebuildConstellation();
  });
} else {
  rebuildConstellation();
}

setupUI();
const { scene, camera, renderer } = initThreeJS();

function setupUI() {
  bindSlider('planes-slider', 'planes-val', (v) => { config.planes = parseInt(v); rebuildConstellation(); });
  bindSlider('sats-slider', 'sats-val', (v) => { config.satsPerPlane = parseInt(v); rebuildConstellation(); });
  bindSlider('altitude-slider', 'altitude-val', (v) => { config.altitudeKm = parseInt(v); rebuildConstellation(); });
  bindSlider('inclination-slider', 'inclination-val', (v) => { config.inclinationDeg = parseInt(v); rebuildConstellation(); });
  bindSlider('speed-slider', 'speed-val', (v) => { config.simSpeed = parseFloat(v); }, (v) => `${parseFloat(v).toFixed(1)}x`);

  const algoSelect = document.getElementById('algo-select');
  if (algoSelect) {
    algoSelect.addEventListener('change', (e) => {
      config.algo = e.target.value;
    });
  }

  const toggleISLs = document.getElementById('toggle-isls');
  if (toggleISLs) {
    toggleISLs.addEventListener('change', (e) => {
      config.showISLs = e.target.checked;
      if (islGroup) islGroup.visible = config.showISLs;
      updateMetrics();
    });
  }
}

function bindSlider(id, valId, onChange, formatFn = (v) => v) {
  const slider = document.getElementById(id);
  const valEl = document.getElementById(valId);
  if (slider && valEl) {
    slider.addEventListener('input', (e) => {
      valEl.innerText = formatFn(e.target.value);
      onChange(e.target.value);
    });
  }
}

function rebuildConstellation() {
  satellites = [];
  const numPlanes = config.planes;
  const numSatsPerPlane = config.satsPerPlane;
  const totalSats = numPlanes * numSatsPerPlane;
  const incRad = (config.inclinationDeg * Math.PI) / 180;
  const orbitRadius = earthRadius + (config.altitudeKm / 6371) * earthRadius;

  for (let p = 0; p < numPlanes; p++) {
    const raan = (2 * Math.PI * p) / numPlanes;
    for (let s = 0; s < numSatsPerPlane; s++) {
      const meanAnomaly = (2 * Math.PI * s) / numSatsPerPlane + (p * Math.PI) / totalSats;
      satellites.push({
        plane: p,
        index: s,
        raan: raan,
        inc: incRad,
        anomaly: meanAnomaly,
        radius: orbitRadius,
        pos: new THREE.Vector3()
      });
    }
  }

  if (satGroup) scene.remove(satGroup);
  if (islGroup) scene.remove(islGroup);

  satGroup = new THREE.Group();
  islGroup = new THREE.Group();

  const satGeo = new THREE.SphereGeometry(0.12, 12, 12);
  const satMat = new THREE.MeshBasicMaterial({ color: 0x38bdf8 });

  satellites.forEach((sat, i) => {
    const mesh = new THREE.Mesh(satGeo, satMat);
    sat.mesh = mesh;
    satGroup.add(mesh);
  });

  scene.add(satGroup);
  scene.add(islGroup);
  islGroup.visible = config.showISLs;

  updateSatellitePositions(0);
  updateMetrics();
}

function updateSatellitePositions(time) {
  if (!satellites.length) return;

  const numSats = satellites.length;

  satellites.forEach((sat, idx) => {
    const currentAnomaly = sat.anomaly + time * 0.2 * config.simSpeed;
    
    // Orbital mechanics transformation: Keplerian elements -> Cartesian XYZ
    const xOrb = sat.radius * Math.cos(currentAnomaly);
    const yOrb = sat.radius * Math.sin(currentAnomaly);

    const x = xOrb * Math.cos(sat.raan) - yOrb * Math.cos(sat.inc) * Math.sin(sat.raan);
    const y = xOrb * Math.sin(sat.raan) + yOrb * Math.cos(sat.inc) * Math.cos(sat.raan);
    const z = yOrb * Math.sin(sat.inc);

    sat.pos.set(x, z, y);
    if (sat.mesh) sat.mesh.position.copy(sat.pos);
  });

  // Render Inter-Satellite Links (ISLs) between intra-plane and inter-plane neighbors
  if (config.showISLs && islGroup) {
    islGroup.clear();
    const lineMat = new THREE.LineBasicMaterial({ color: 0x0284c7, transparent: true, opacity: 0.35 });
    const linePositions = [];

    const numPlanes = config.planes;
    const satsPerPlane = config.satsPerPlane;

    for (let p = 0; p < numPlanes; p++) {
      for (let s = 0; s < satsPerPlane; s++) {
        const currentIdx = p * satsPerPlane + s;
        const nextInPlaneIdx = p * satsPerPlane + ((s + 1) % satsPerPlane);
        const nextPlaneIdx = ((p + 1) % numPlanes) * satsPerPlane + s;

        // Intra-plane link
        const p1 = satellites[currentIdx].pos;
        const p2 = satellites[nextInPlaneIdx].pos;
        linePositions.push(p1.x, p1.y, p1.z, p2.x, p2.y, p2.z);

        // Inter-plane link
        const p3 = satellites[nextPlaneIdx].pos;
        linePositions.push(p1.x, p1.y, p1.z, p3.x, p3.y, p3.z);
      }
    }

    const lineGeo = new THREE.BufferGeometry();
    lineGeo.setAttribute('position', new THREE.Float32BufferAttribute(linePositions, 3));
    const linesMesh = new THREE.LineSegments(lineGeo, lineMat);
    islGroup.add(linesMesh);
  }
}

function updateMetrics() {
  const totalSats = satellites.length;
  const totalISLs = config.showISLs ? totalSats * 2 : 0;
  
  const totalEl = document.getElementById('metric-total-sats');
  const islsEl = document.getElementById('metric-active-isls');
  
  if (totalEl) totalEl.innerText = totalSats;
  if (islsEl) islsEl.innerText = totalISLs;
}

function initThreeJS() {
  const container = document.getElementById('canvas-container');
  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
  camera.position.set(0, 8, 18);

  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(window.devicePixelRatio);
  container.appendChild(renderer.domElement);

  // 3D Globe
  const geometry = new THREE.SphereGeometry(earthRadius, 48, 48);
  const material = new THREE.MeshBasicMaterial({ color: 0x1d4ed8, wireframe: true, transparent: true, opacity: 0.6 });
  const globe = new THREE.Mesh(geometry, material);
  scene.add(globe);

  window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  let clock = new THREE.Clock();

  function animate() {
    requestAnimationFrame(animate);
    const elapsedTime = clock.getElapsedTime();
    globe.rotation.y = elapsedTime * 0.05 * config.simSpeed;
    
    updateSatellitePositions(elapsedTime);
    renderer.render(scene, camera);
  }
  animate();

  return { scene, camera, renderer };
}
