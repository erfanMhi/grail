"""Contact sheet of viewer screenshots (stage area only), 2 columns."""
import sys
from PIL import Image, ImageDraw
shots = sys.argv[2:]
tiles = []
for path in shots:
    im = Image.open(path).convert("RGB")
    im = im.crop((0, 120, im.width - 340, im.height - 60))   # stage only, no header/panel/buttons
    tiles.append((path.split('/')[-1].replace('.png', '').replace('v_', ''), im.resize((im.width // 2, im.height // 2))))
w, h = tiles[0][1].size
cols = 2; rows = (len(tiles) + 1) // 2
out = Image.new("RGB", (cols * (w + 12) + 12, rows * (h + 34) + 12), (236, 238, 241))
d = ImageDraw.Draw(out)
for i, (name, im) in enumerate(tiles):
    x = 12 + (i % cols) * (w + 12); y = 12 + (i // cols) * (h + 34)
    d.text((x + 4, y + 2), name.replace('_', ' '), fill=(30, 33, 38))
    out.paste(im, (x, y + 20))
out.save(sys.argv[1]); print("wrote", sys.argv[1], out.size)
