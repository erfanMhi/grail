#!/bin/bash
# render_stl.sh <mesh.stl> <out_dir> [shots-json]
# Renders an STL in the silver three.js viewer and writes PNGs into out_dir.
# Default shots: photo angle, top, front, and two close-ups. Each shot is
# [model, view, name, [theta, phi, distFactor]] — theta/phi in radians about the
# model centre, distFactor times the bounding radius; omit the camera to use the view.
set -e
V=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
STL=$(readlink -f "$1"); OUT=$(readlink -f "$2"); mkdir -p "$OUT"
SHOTS=${3:-'[["ring","photo","photo",[-1.5708,0.85,3.7]],["ring","photo","top",[-1.5708,0.05,3.7]],["ring","photo","front",[-1.5708,1.45,3.7]],["ring","robot","robot"],["ring","human","human"]]'}
python3 $V/build.py "$(dirname "$STL")" "$OUT/view.html" ring "ring=$(basename "$STL")" > /dev/null
SHOTS="$SHOTS" node $V/shot.js "$OUT/view.html" "$OUT" 2>&1 | grep -v "^$" || true
ls "$OUT"/*.png
