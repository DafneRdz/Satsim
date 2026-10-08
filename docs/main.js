// Initialize Three.js scene first
initThreeJS();

// Boot WASM module safely
if (typeof SatsimModule === 'function') {
  SatsimModule({
    locateFile: (path) => `wasm/${path}`
  }).then((Module) => {
    const statusEl = document.getElementById('status');
    if (statusEl) statusEl.innerText = 'WASM Module Loaded Successfully';
    console.log('Satsim WASM Module initialized:', Module);
  }).catch((err) => {
    console.error('WASM initialization error:', err);
    const statusEl = document.getElementById('status');
    if (statusEl) statusEl.innerText = 'WASM Load Error (Check Console)';
  });
} else {
  console.warn('SatsimModule function not found on window object.');
}

function initThreeJS() {
  const container = document.getElementById('canvas-container');
  if (!container) return;

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
  camera.position.z = 15;

  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(window.devicePixelRatio);
  container.appendChild(renderer.domElement);

  // Create 3D Wireframe Globe
  const geometry = new THREE.SphereGeometry(5, 32, 32);
  const material = new THREE.MeshBasicMaterial({ color: 0x38bdf8, wireframe: true });
  const globe = new THREE.Mesh(geometry, material);
  scene.add(globe);

  window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  function animate() {
    requestAnimationFrame(animate);
    globe.rotation.y += 0.003;
    renderer.render(scene, camera);
  }
  animate();
}
