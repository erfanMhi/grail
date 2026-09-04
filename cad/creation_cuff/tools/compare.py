"""Side-by-side: reference photo (left) and the viewer's silver render (right)."""
import sys
from PIL import Image, ImageDraw
photo = Image.open(sys.argv[1]).convert("RGB")
shot = Image.open(sys.argv[2]).convert("RGB")
# crop the viewer screenshot to the 3D stage (left of the 340px side panel)
shot = shot.crop((0, 0, shot.width - 340, shot.height))
H = 800
photo = photo.resize((int(photo.width * H / photo.height), H))
shot = shot.resize((int(shot.width * H / shot.height), H))
out = Image.new("RGB", (photo.width + shot.width + 24, H + 40), (236, 238, 241))
out.paste(photo, (8, 32)); out.paste(shot, (photo.width + 16, 32))
d = ImageDraw.Draw(out)
d.text((12, 10), "reference photo", fill=(30, 33, 38))
d.text((photo.width + 20, 10), "creation_cuff.scad  preset=\"ring\"  (three.js silver render)", fill=(30, 33, 38))
out.save(sys.argv[3]); print("wrote", sys.argv[3], out.size)
