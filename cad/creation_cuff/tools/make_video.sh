#!/bin/bash
# make_video.sh <viewer.html> <out.mp4> [model] [metal] [seconds] [fps]
set -e
V=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
FR=$(mktemp -d)
node $V/video.js "$1" "$FR" "${3:-ring}" "${4:-silver}" "${5:-15}" "${6:-30}"
FF=$(python3 -c "import imageio_ffmpeg; print(imageio_ffmpeg.get_ffmpeg_exe())")
$FF -y -loglevel error -framerate "${6:-30}" -i "$FR/f%04d.png" -c:v libx264 -pix_fmt yuv420p -crf 18 -movflags +faststart "$2"
rm -rf "$FR"; ls -la "$2"
