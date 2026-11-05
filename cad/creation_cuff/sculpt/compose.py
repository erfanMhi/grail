"""Labelled sheets for the README:

    compose.py metals OUT sterling.png white.png yellow.png rose.png
    compose.py compare OUT reference.jpg render.png [reference2 render2]
"""
import sys

from PIL import Image, ImageDraw, ImageFont


def font(size):
    for path in ("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                 "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"):
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            pass
    return ImageFont.load_default()


def label(im, text, size=28):
    d = ImageDraw.Draw(im)
    f = font(size)
    d.text((22, 18), text, fill=(60, 64, 70), font=f)


def grid(images, labels, cols, cell):
    rows = (len(images) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell, rows * cell), (236, 237, 239))
    for i, (im, lab) in enumerate(zip(images, labels)):
        im = im.convert("RGB")
        w, h = im.size
        s = cell / max(w, h)
        im = im.resize((int(w * s), int(h * s)), Image.LANCZOS)
        tile = Image.new("RGB", (cell, cell), (236, 237, 239))
        tile.paste(im, ((cell - im.width) // 2, (cell - im.height) // 2))
        label(tile, lab, max(18, cell // 34))
        sheet.paste(tile, ((i % cols) * cell, (i // cols) * cell))
    return sheet


def main():
    mode, out = sys.argv[1], sys.argv[2]
    files = sys.argv[3:]
    if mode == "metals":
        labs = ["Sterling silver", "18k white gold", "18k yellow gold", "18k rose gold"]
        grid([Image.open(f) for f in files], labs, 2, 900).save(out)
    elif mode == "compare":
        ims = [Image.open(f) for f in files]
        labs = []
        for i in range(len(ims)):
            labs.append("Reference photo" if i % 2 == 0 else "Sculpted model, same angle")
        grid(ims, labs, 2, 1000).save(out)


main()
