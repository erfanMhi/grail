# tools

Render helpers used to check the meshes against the reference photo.

```sh
cd cad/creation_cuff/tools
npm install                      # playwright + three (three.js r128 is served locally to the page)
npx playwright install chromium  # once
pip install pillow imageio-ffmpeg

python3 build.py ../export viewer.html            # the interactive viewer: sculpted ring first, then the templates
# (the sculpted ring loads from sculpt/build/creation_ring_sculpted_view.stl, a 320k-triangle copy of the export)
./render_stl.sh ../export/creation_ring_implicit.stl out/    # PNGs of one mesh from the standard angles
./make_video.sh viewer.html ring.mp4 ring yellow 15 30       # 15 s turntable, yellow gold
python3 compare.py photo.png out/photo.png compare.png       # side by side with a reference photo
```

Shots are `[model, view, name, [theta, phi, distFactor, tx, ty, tz], metal]`; metals are
`silver`, `white`, `yellow`, `rose`. `template.html` is the viewer page itself.
