#!/bin/bash
# test_part.sh <all|band|human_hand|robot_hand> [resolution]  -> STL + silver PNGs
# Output: implicit/review/<part>/{mesh.stl, *.png}
set -e
cd "$(dirname "$0")"
PART=${1:-all}; RES=${2:-110}
node build_model.mjs $PART
MODEL=$([ $PART = all ] && echo creation_ring.implicit.js || echo test_$PART.implicit.js)
OUT=review/$PART; mkdir -p $OUT
# multi-threaded mesher (cadgen gen) -> sibling GLB -> binary STL
( cd ~/.claude/skills/implicit-cad && python3 scripts/gen "$OLDPWD/$MODEL" --write --resolution $RES --threads ${THREADS:-4} --force 2>&1 | grep -iE "error|fail|generated|wrote" ) || true
python3 glb2stl.py "${MODEL%.implicit.js}.glb" $OUT/mesh.stl
if [ $PART = all ]; then
  SHOTS='[["ring","photo","photo",[-1.5708,0.85,3.7]],["ring","photo","top",[-1.5708,0.05,3.7]],["ring","photo","front",[-1.5708,1.45,3.7]],["ring","robot","robot"],["ring","human","human"]]'
else
  # hand frames: +X fingers, +Y thumb, +Z back. back view from above-front, side, underside, front-on
  SHOTS='[["ring","photo","back_above",[-1.2,0.75,2.6]],["ring","photo","thumb_side",[1.57,1.2,2.6]],["ring","photo","underside",[-1.2,2.4,2.6]],["ring","photo","fingertips",[0.0,1.3,2.6]],["ring","photo","top",[-1.5708,0.05,2.6]]]'
fi
/tmp/claude-0/-home-user-grail/c3d02766-5458-503f-85a4-f790cd9e4775/scratchpad/viewer/render_stl.sh $OUT/mesh.stl $OUT "$SHOTS"
