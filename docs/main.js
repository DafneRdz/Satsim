// Initialize WebAssembly module
if (typeof SatsimModule !== 'undefined') {
  SatsimModule().then((Module) => {
    document.getElementById('status').innerText = 'WASM Module Loaded Successfully';
    console.log('Satsim WASM Module initialized:', Module);
    initThreeJS();
  }).catch((err) => {
    console.error('Failed to load WASM module:', err);
    document.getElementById('status').innerText = 'WASM Load Error';
  });
} else {
  initThreeJS();
}

function initThreeJS() {
  const container = document.getElementById('canvas-container');
  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
  camera.position.z = 15;

  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(window.devicePixelRatio);
  container.appendChild(renderer.domElement);

  // Simple Globe Placeholder
  const geometry = new THREE.SphereGeometry(5, 64, 64);
  const material = new THREE.MeshBasicMaterial({ color: 0x1d4ed8, wireframe: true });
  const globe = new THREE.Mesh(geometry, material);
  scene.add(globe);

  window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  function animate() {
    requestAnimationFrame(animate);
    globe.rotation.y += 0.002;
    renderer.render(scene, camera);
  }
  animate();
}
