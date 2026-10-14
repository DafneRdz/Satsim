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

let scene, camera, renderer, controls;
let satGroup = null;
let islGroup = null;
let satellites = [];
const earthRadius = 5;

// 1. Initialize 3D Three.js canvas first
initThreeJS();

// 2. Setup UI Control Bindings
setupUI();

// 3. Build satellites and start rendering
rebuildConstellation();

// 4. Try loading WASM module
if (typeof SatsimModule === 'function') {
  SatsimModule({
    locateFile: (path) => `wasm/${path}`
  }).then((Module) => {
    const badge = document.getElementById('status-badge');
    if (badge) {
      badge.innerText = 'WASM Engine Active';
      badge.classList.add('active');
    }
  }).catch((err) => {
    console.warn('WASM fallback mode:', err);
    const badge = document.getElementById('status-badge');
    if (badge) badge.innerText = 'JS Physics Active';
  });
}

function initThreeJS() {
  const container = document.getElementById('canvas-container');
  if (!container) return;

  scene = new THREE.Scene();
  camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
  camera.position.set(0, 10, 20);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(window.devicePixelRatio);
  container.appendChild(renderer.domElement);

  if (typeof THREE.OrbitControls !== 'undefined') {
    controls = new THREE.OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.05;
  }

  // Earth Globe
  const geometry = new THREE.SphereGeometry(earthRadius, 48, 48);
  const material = new THREE.MeshBasicMaterial({ color: 0x1d4ed8, wireframe: true, transparent: true, opacity: 0.5 });
  const globe = new THREE.Mesh(geometry, material);
  scene.add(globe);

  satGroup = new THREE.Group();
  islGroup = new THREE.Group();
  scene.add(satGroup);
  scene.add(islGroup);

  window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  let clock = new THREE.Clock();

  function animate() {
    requestAnimationFrame(animate);
    const elapsedTime = clock.getElapsedTime();
    globe.rotation.y = elapsedTime * 0.03 * config.simSpeed;
    
    if (controls) controls.update();
    updateSatellitePositions(elapsedTime);
    renderer.render(scene, camera);
  }
  animate();
}

function setupUI() {
  bindSlider('planes-slider', 'planes-val', (v) => { config.planes = parseInt(v); rebuildConstellation(); });
  bindSlider('sats-slider', 'sats-val', (v) => { config.satsPerPlane = parseInt(v); rebuildConstellation(); });
  bindSlider('altitude-slider', 'altitude-val', (v) => { config.altitudeKm = parseInt(v); rebuildConstellation(); });
  bindSlider('inclination-slider', 'inclination-val', (v) => { config.inclinationDeg = parseInt(v); rebuildConstellation(); });
  bindSlider('speed-slider', 'speed-val', (v) => { config.simSpeed = parseFloat(v); }, (v) => `${parseFloat(v).toFixed(1)}x`);

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
  if (!scene || !satGroup) return;

  satGroup.clear();
  if (islGroup) islGroup.clear();
  satellites = [];

  const numPlanes = config.planes;
  const numSatsPerPlane = config.satsPerPlane;
  const totalSats = numPlanes * numSatsPerPlane;
  const incRad = (config.inclinationDeg * Math.PI) / 180;
  const orbitRadius = earthRadius + (config.altitudeKm / 6371) * earthRadius * 1.5;

  const satGeo = new THREE.SphereGeometry(0.2, 12, 12);
  const satMat = new THREE.MeshBasicMaterial({ color: 0x38bdf8 });

  for (let p = 0; p < numPlanes; p++) {
    const raan = (2 * Math.PI * p) / numPlanes;
    for (let s = 0; s < numSatsPerPlane; s++) {
      const meanAnomaly = (2 * Math.PI * s) / numSatsPerPlane + (p * Math.PI) / totalSats;
      const mesh = new THREE.Mesh(satGeo, satMat);
      satGroup.add(mesh);

      satellites.push({
        plane: p,
        index: s,
        raan: raan,
        inc: incRad,
        anomaly: meanAnomaly,
        radius: orbitRadius,
        pos: new THREE.Vector3(),
        mesh: mesh
      });
    }
  }

  if (islGroup) islGroup.visible = config.showISLs;
  updateMetrics();
}

function updateSatellitePositions(time) {
  if (!satellites.length) return;

  satellites.forEach((sat) => {
    const currentAnomaly = sat.anomaly + time * 0.15 * config.simSpeed;
    
    const xOrb = sat.radius * Math.cos(currentAnomaly);
    const yOrb = sat.radius * Math.sin(currentAnomaly);

    const x = xOrb * Math.cos(sat.raan) - yOrb * Math.cos(sat.inc) * Math.sin(sat.raan);
    const y = xOrb * Math.sin(sat.raan) + yOrb * Math.cos(sat.inc) * Math.cos(sat.raan);
    const z = yOrb * Math.sin(sat.inc);

    sat.pos.set(x, z, y);
    sat.mesh.position.copy(sat.pos);
  });

  if (config.showISLs && islGroup) {
    islGroup.clear();
    const lineMat = new THREE.LineBasicMaterial({ color: 0x0284c7, transparent: true, opacity: 0.4 });
    const linePositions = [];

    const numPlanes = config.planes;
    const satsPerPlane = config.satsPerPlane;

    for (let p = 0; p < numPlanes; p++) {
      for (let s = 0; s < satsPerPlane; s++) {
        const currentIdx = p * satsPerPlane + s;
        const nextInPlaneIdx = p * satsPerPlane + ((s + 1) % satsPerPlane);
        const nextPlaneIdx = ((p + 1) % numPlanes) * satsPerPlane + s;

        if (satellites[currentIdx] && satellites[nextInPlaneIdx]) {
          const p1 = satellites[currentIdx].pos;
          const p2 = satellites[nextInPlaneIdx].pos;
          linePositions.push(p1.x, p1.y, p1.z, p2.x, p2.y, p2.z);
        }

        if (satellites[currentIdx] && satellites[nextPlaneIdx]) {
          const p1 = satellites[currentIdx].pos;
          const p3 = satellites[nextPlaneIdx].pos;
          linePositions.push(p1.x, p1.y, p1.z, p3.x, p3.y, p3.z);
        }
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
