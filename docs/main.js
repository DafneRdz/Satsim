// Global configuration state
const config = {
  planes: 6,
  satsPerPlane: 10,
  altitudeKm: 550,
  inclinationDeg: 53,
  algo: 'dijkstra',
  showISLs: true,
  simSpeed: 1.0,
  srcCity: 'nyc',
  dstCity: 'london'
};

const CITIES = {
  nyc: { name: 'New York', lat: 40.7128, lon: -74.0060 },
  london: { name: 'London', lat: 51.5074, lon: -0.1278 },
  tokyo: { name: 'Tokyo', lat: 35.6762, lon: 139.6503 },
  sydney: { name: 'Sydney', lat: -33.8688, lon: 151.2093 }
};

let scene, camera, renderer, controls;
let satGroup = null;
let islGroup = null;
let pathGroup = null;
let groundStationGroup = null;
let cloudMesh = null;
let satellites = [];
const earthRadius = 5;

initThreeJS();
setupUI();
rebuildConstellation();

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
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  container.appendChild(renderer.domElement);

  if (typeof THREE.OrbitControls !== 'undefined') {
    controls = new THREE.OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.05;
  }

  const ambientLight = new THREE.AmbientLight(0x333333, 1.2);
  scene.add(ambientLight);

  const sunLight = new THREE.DirectionalLight(0xffffff, 2.0);
  sunLight.position.set(15, 10, 15);
  scene.add(sunLight);

  const textureLoader = new THREE.TextureLoader();

  const earthGeo = new THREE.SphereGeometry(earthRadius, 64, 64);
  const earthMat = new THREE.MeshPhongMaterial({
    map: textureLoader.load('https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/planets/earth_atmos_2048.jpg'),
    specularMap: textureLoader.load('https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/planets/earth_specular_2048.jpg'),
    normalMap: textureLoader.load('https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/planets/earth_normal_2048.jpg'),
    normalScale: new THREE.Vector2(0.85, 0.85),
    specular: new THREE.Color(0x333333),
    shininess: 15
  });

  const earthMesh = new THREE.Mesh(earthGeo, earthMat);
  scene.add(earthMesh);

  const cloudGeo = new THREE.SphereGeometry(earthRadius + 0.04, 64, 64);
  const cloudMat = new THREE.MeshPhongMaterial({
    map: textureLoader.load('https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/planets/earth_clouds_2048.png'),
    transparent: true,
    opacity: 0.45,
    blending: THREE.AdditiveBlending
  });
  cloudMesh = new THREE.Mesh(cloudGeo, cloudMat);
  scene.add(cloudMesh);

  const atmosphereGeo = new THREE.SphereGeometry(earthRadius + 0.25, 64, 64);
  const atmosphereMat = new THREE.ShaderMaterial({
    vertexShader: `
      varying vec3 vNormal;
      void main() {
        vNormal = normalize(normalMatrix * normal);
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
      }
    `,
    fragmentShader: `
      varying vec3 vNormal;
      void main() {
        float intensity = pow(0.65 - dot(vNormal, vec3(0, 0, 1.0)), 2.0);
        gl_FragColor = vec4(0.2, 0.6, 1.0, 1.0) * intensity;
      }
    `,
    blending: THREE.AdditiveBlending,
    side: THREE.BackSide,
    transparent: true
  });

  const atmosphereMesh = new THREE.Mesh(atmosphereGeo, atmosphereMat);
  scene.add(atmosphereMesh);

  satGroup = new THREE.Group();
  islGroup = new THREE.Group();
  pathGroup = new THREE.Group();
  groundStationGroup = new THREE.Group();

  scene.add(satGroup);
  scene.add(islGroup);
  scene.add(pathGroup);
  scene.add(groundStationGroup);

  renderGroundStations();

  window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  let clock = new THREE.Clock();

  function animate() {
    requestAnimationFrame(animate);
    const elapsedTime = clock.getElapsedTime();
    
    earthMesh.rotation.y = elapsedTime * 0.02 * config.simSpeed;
    if (cloudMesh) cloudMesh.rotation.y = elapsedTime * 0.025 * config.simSpeed;
    
    if (controls) controls.update();
    updateSatellitePositions(elapsedTime);
    renderer.render(scene, camera);
  }
  animate();
}

function latLonToVector3(lat, lon, radius) {
  const phi = (90 - lat) * (Math.PI / 180);
  const theta = (lon + 180) * (Math.PI / 180);
  const x = -(radius * Math.sin(phi) * Math.cos(theta));
  const z = radius * Math.sin(phi) * Math.sin(theta);
  const y = radius * Math.cos(phi);
  return new THREE.Vector3(x, y, z);
}

function renderGroundStations() {
  if (!groundStationGroup) return;
  groundStationGroup.clear();

  const gsGeo = new THREE.SphereGeometry(0.12, 16, 16);
  const gsMat = new THREE.MeshBasicMaterial({ color: 0xef4444 });

  Object.keys(CITIES).forEach((key) => {
    const city = CITIES[key];
    const pos = latLonToVector3(city.lat, city.lon, earthRadius + 0.02);
    const mesh = new THREE.Mesh(gsGeo, gsMat);
    mesh.position.copy(pos);
    groundStationGroup.add(mesh);
  });
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

  const srcSelect = document.getElementById('src-city');
  const dstSelect = document.getElementById('dst-city');

  if (srcSelect) srcSelect.addEventListener('change', (e) => { config.srcCity = e.target.value; });
  if (dstSelect) dstSelect.addEventListener('change', (e) => { config.dstCity = e.target.value; });
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

  const satGeo = new THREE.SphereGeometry(0.18, 12, 12);
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
    const lineMat = new THREE.LineBasicMaterial({ color: 0x0284c7, transparent: true, opacity: 0.35 });
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

  updateRoutingPath();
}

function updateRoutingPath() {
  if (!pathGroup || !satellites.length) return;
  pathGroup.clear();

  const srcCity = CITIES[config.srcCity];
  const dstCity = CITIES[config.dstCity];
  if (!srcCity || !dstCity) return;

  const srcPos = latLonToVector3(srcCity.lat, srcCity.lon, earthRadius);
  const dstPos = latLonToVector3(dstCity.lat, dstCity.lon, earthRadius);

  let closestSrcSat = null;
  let closestDstSat = null;
  let minSrcDist = Infinity;
  let minDstDist = Infinity;

  satellites.forEach((sat) => {
    const dSrc = sat.pos.distanceTo(srcPos);
    const dDst = sat.pos.distanceTo(dstPos);

    if (dSrc < minSrcDist) { minSrcDist = dSrc; closestSrcSat = sat; }
    if (dDst < minDstDist) { minDstDist = dDst; closestDstSat = sat; }
  });

  if (!closestSrcSat || !closestDstSat) return;

  const pathPositions = [
    srcPos.x, srcPos.y, srcPos.z,
    closestSrcSat.pos.x, closestSrcSat.pos.y, closestSrcSat.pos.z
  ];

  // Build intermediate multi-hop ISL trajectory
  let curr = closestSrcSat;
  let hops = 1;

  while (curr !== closestDstSat && hops < 12) {
    let nextSat = null;
    let minD = Infinity;

    satellites.forEach((candidate) => {
      if (candidate === curr) return;
      const distToCurr = candidate.pos.distanceTo(curr.pos);
      if (distToCurr < 4.5) { // ISL connectivity threshold
        const distToDst = candidate.pos.distanceTo(closestDstSat.pos);
        if (distToDst < minD) {
          minD = distToDst;
          nextSat = candidate;
        }
      }
    });

    if (!nextSat || nextSat === curr) break;

    pathPositions.push(
      curr.pos.x, curr.pos.y, curr.pos.z,
      nextSat.pos.x, nextSat.pos.y, nextSat.pos.z
    );

    curr = nextSat;
    hops++;
  }

  pathPositions.push(
    curr.pos.x, curr.pos.y, curr.pos.z,
    dstPos.x, dstPos.y, dstPos.z
  );

  const pathMat = new THREE.LineBasicMaterial({ color: 0xef4444, linewidth: 3 });
  const pathGeo = new THREE.BufferGeometry();
  pathGeo.setAttribute('position', new THREE.Float32BufferAttribute(pathPositions, 3));
  const pathMesh = new THREE.LineSegments(pathGeo, pathMat);
  pathGroup.add(pathMesh);

  // Update UI metrics
  const hopsEl = document.getElementById('metric-hops');
  const latencyEl = document.getElementById('metric-latency');

  if (hopsEl) hopsEl.innerText = `${hops} hops`;
  if (latencyEl) latencyEl.innerText = `${(hops * 4.2 + 12.5).toFixed(1)} ms`;
}

function updateMetrics() {
  const totalSats = satellites.length;
  const totalISLs = config.showISLs ? totalSats * 2 : 0;
  
  const totalEl = document.getElementById('metric-total-sats');
  const islsEl = document.getElementById('metric-active-isls');
  
  if (totalEl) totalEl.innerText = totalSats;
  if (islsEl) islsEl.innerText = totalISLs;
}
