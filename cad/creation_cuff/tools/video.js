// video.js <viewer.html> <out_dir> [model] [metal] [seconds] [fps]
// Captures a smooth camera path around the model as PNG frames.
const { chromium } = require('playwright');
const fs = require('fs');
(async () => {
  const [html, outDir, model = 'ring', metal = 'silver', seconds = '15', fps = '30'] = process.argv.slice(2);
  fs.mkdirSync(outDir, { recursive: true });
  const b = await chromium.launch({ ...(require('fs').existsSync('/opt/pw-browsers/chromium') ? { executablePath: '/opt/pw-browsers/chromium' } : {}), headless: false, args: ['--headless=new', '--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const pg = await b.newPage({ viewport: { width: 1280, height: 720 } });
  const local = html.replace(/\.html$/, '.local.html');
  fs.writeFileSync(local, fs.readFileSync(html, 'utf8')
    .replace(/https:\/\/cdnjs\.cloudflare\.com\/ajax\/libs\/three\.js\/r128\/three\.min\.js/, 'file://' + __dirname + '/node_modules/three/build/three.min.js')
    .replace(/<link rel="stylesheet" href="https:\/\/fonts\.googleapis\.com[^>]*>/, ''));
  await pg.goto('file://' + local, { waitUntil: 'load' });
  await pg.waitForTimeout(2500);
  await pg.evaluate(({ model, metal }) => {
    load(model); setMetal(metal);
    document.getElementById('spin').checked = false;
    // hide chrome: full-bleed stage
    document.querySelector('header').style.display = 'none';
    document.querySelector('aside').style.display = 'none';
    document.querySelector('.views').style.display = 'none';
    document.querySelector('.hint').style.display = 'none';
    document.querySelector('main').style.gridTemplateColumns = '1fr';
  }, { model, metal });
  await pg.waitForTimeout(800);
  // keyframes: [theta, phi, dist, tx, ty, tz] — a slow orbit that dips to the hands and rises to the top
  const K = [
    [-1.5708, 0.85, 3.7, 0, 0, 0],      // photo angle
    [-2.4,    0.95, 3.2, 0, 0, 0],      // swing round the robot side
    [-2.1,    1.05, 1.9, -0.45, -0.7, -0.1], // dip to the robot hand
    [-1.0,    1.05, 1.9,  0.45, -0.7, -0.1], // slide across to the human hand
    [-0.6,    0.7,  3.4, 0, 0, 0],      // pull back on the human side
    [ 0.9,    0.5,  3.6, 0, 0, 0],      // over the back
    [ 2.6,    0.25, 3.9, 0, 0, 0],      // near-top view
    [ 4.71,   0.85, 3.7, 0, 0, 0],      // back to the photo angle (one full turn)
  ];
  const total = Math.round(parseFloat(seconds) * parseInt(fps));
  const smooth = t => t * t * (3 - 2 * t);
  for (let i = 0; i < total; i++) {
    const u = i / (total - 1) * (K.length - 1);
    const k = Math.min(Math.floor(u), K.length - 2), t = smooth(u - k);
    const a = K[k], c = K[k + 1];
    const cam = a.map((v, j) => v + (c[j] - v) * t);
    await pg.evaluate(cam => { const bc = mesh.geometry.boundingSphere.center; ctl.theta = cam[0]; ctl.phi = cam[1]; ctl.dist = cam[2] * radius; ctl.target.set(bc.x + cam[3] * radius, bc.y + cam[4] * radius, bc.z + cam[5] * radius); }, cam);
    await pg.waitForTimeout(40);
    await pg.screenshot({ path: `${outDir}/f${String(i).padStart(4, '0')}.png` });
  }
  await b.close();
  console.log('frames', total);
})();
