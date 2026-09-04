// Screenshot the viewer page in each model/view for a silver-render comparison.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch({ ...(require('fs').existsSync('/opt/pw-browsers/chromium') ? { executablePath: '/opt/pw-browsers/chromium' } : {}), headless: false, args: ['--headless=new', '--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const pg = await b.newPage({ viewport: { width: 1400, height: 900 } });
  pg.on('pageerror', e => console.log('PAGEERROR', e.message));
  pg.on('console', m => { if (m.type() === 'error') console.log('CONSOLE', m.text()); });
  // offline copy: local three.js, no web fonts
  const fs = require('fs');
  const local = process.argv[2].replace(/\.html$/, '.local.html');
  fs.writeFileSync(local, fs.readFileSync(process.argv[2], 'utf8')
    .replace(/https:\/\/cdnjs\.cloudflare\.com\/ajax\/libs\/three\.js\/r128\/three\.min\.js/, 'file://' + __dirname + '/node_modules/three/build/three.min.js')
    .replace(/<link rel="stylesheet" href="https:\/\/fonts\.googleapis\.com[^>]*>/, ''));
  await pg.goto('file://' + local, { waitUntil: 'load' });
  await pg.waitForTimeout(2500);
  const shots = process.env.SHOTS ? JSON.parse(process.env.SHOTS)
    : [['ring','photo','shot_ring'], ['cring','photo','shot_cring'], ['bangle','photo','shot_bangle']];
  for (const [model, view, name, cam, metal] of shots) {
    await pg.evaluate(m => setMetal(m || 'silver'), metal || null);
    await pg.click(`.seg button[data-model="${model}"]`);
    await pg.click(`.views .btn[data-view="${view}"]`);
    await pg.evaluate(() => { document.getElementById('spin').checked = false; });
    if (cam) await pg.evaluate(c => {
      const bc = mesh.geometry.boundingSphere.center;
      ctl.theta = c[0]; ctl.phi = c[1]; ctl.dist = c[2] * radius;
      ctl.target.set(bc.x + (c[3] || 0) * radius, bc.y + (c[4] || 0) * radius, bc.z + (c[5] || 0) * radius);
    }, cam);
    await pg.waitForTimeout(1200);
    await pg.screenshot({ path: process.argv[3] + '/' + name + '.png' });
  }
  await b.close();
})();
