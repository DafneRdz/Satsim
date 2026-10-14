// Configuration state driven by UI
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

// Boot WASM module safely
if (typeof SatsimModule === 'function') {
  SatsimModule({
    locateFile: (path) => `wasm/${path}`
  }).then((Module) => {
    wasmModule = Module;
    const badge = document.getElementById('status-badge');
    if (badge) {
      badge.innerText = 'WASM Core Active';
      badge.classList.add('active');
    }
    console.log('Satsim WASM Module initialized:', Module);
  }).catch((err) => {
    console.error('WASM initialization error:', err);
    const badge = document.getElementById('status-badge');
    if (badge) badge.innerText = 'WASM Fallback (JS Engine)';
  });
}

// Attach UI Event Listeners
setupUI();

// Start Three.js Canvas
initThreeJS();

function setupUI() {
  bindSlider('planes-slider', 'planes-val', (v) => { config.planes = parseInt(v); updateMetrics(); });
  bindSlider('sats-slider', 'sats-val', (v) => { config.satsPerPlane = parseInt(v); updateMetrics(); });
  bindSlider('altitude-slider', 'altitude-val', (v) => { config.altitudeKm = parseInt(v); });
  bindSlider('inclination-slider', 'inclination-val', (v) => { config.inclinationDeg = parseInt(v); });
  bindSlider('speed-slider', 'speed-val', (v) => { config.simSpeed = parseFloat(v); }, (v) => `${parseFloat(v).toFixed(1)}x`);

  const algoSelect = document.getElementById('algo-select');
  if (algoSelect) {
    algoSelect.addEventListener('change', (e) => {
      config.algo = e.target.value;
      console.log('Routing algorithm switched to:', config.algo);
    });
  }

  const toggleISLs = document.getElementById('toggle-isls');
  if (toggleISLs) {
    toggleISLs.addEventListener('change', (e) => {
      config.showISLs = e.target.checked;
    });
  }

  updateMetrics();
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

function updateMetrics() {
  const totalSats = config.planes * config.satsPerPlane;
  const totalISLs = totalSats * 2; // Est. 4 links per sat (bidirectional)
  
  const totalEl = document.getElementById('metric-total-sats');
  const islsEl = document.getElementById('metric-active-isls');
  
  if (totalEl) totalEl.innerText = totalSats;
  if (islsEl) islsEl.innerText = config.showISLs ? totalISLs : 0;
}

function initThreeJS() {
  const container = document.getElementById('canvas-container');
  if (!container) return;

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
  camera.position.z = 16;

  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(window.devicePixelRatio);
  container.appendChild(renderer.domElement);

  // Create 3D Globe with atmosphere glow
  const geometry = new THREE.SphereGeometry(5, 48, 48);
  const material = new THREE.MeshBasicMaterial({ color: 0x1e3a8a, wireframe: true });
  const globe = new THREE.Mesh(geometry, material);
  scene.add(globe);

  window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  function animate() {
    requestAnimationFrame(animate);
    globe.rotation.y += 0.002 * config.simSpeed;
    renderer.render(scene, camera);
  }
  animate();
}
