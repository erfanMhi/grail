# Regenerate meshes and previews for the Creation Cuff.
# Requires OpenSCAD (2021.01+) on PATH; previews need xvfb-run on headless boxes.

SCAD    := creation_cuff.scad
OUT     := export
FN      ?= 36
OPENSCAD ?= openscad
XVFB    ?= $(shell command -v xvfb-run >/dev/null 2>&1 && echo "xvfb-run -a")

STLS := $(OUT)/creation_cuff_bangle.stl \
        $(OUT)/creation_cuff_ring.stl \
        $(OUT)/creation_cuff_bangle_band.stl \
        $(OUT)/creation_cuff_bangle_human_hand.stl \
        $(OUT)/creation_cuff_bangle_robot_hand.stl

PNGS := $(OUT)/creation_cuff_bangle.png $(OUT)/creation_cuff_ring.png \
        $(OUT)/creation_cuff_top.png $(OUT)/creation_cuff_front.png

all: stl png
stl: $(STLS)
png: $(PNGS)

$(OUT)/creation_cuff_bangle.stl: $(SCAD)
	$(OPENSCAD) -o $@ -D '$$fn=$(FN)' -D 'preset="bangle"' $<
$(OUT)/creation_cuff_ring.stl: $(SCAD)
	$(OPENSCAD) -o $@ -D '$$fn=$(FN)' -D 'preset="ring"' $<
$(OUT)/creation_cuff_bangle_%.stl: $(SCAD)
	$(OPENSCAD) -o $@ -D '$$fn=$(FN)' -D 'preset="bangle"' -D 'part="$*"' $<

# photo-like view: from the front, slightly above
$(OUT)/creation_cuff_bangle.png: $(SCAD)
	$(XVFB) $(OPENSCAD) -o $@ --imgsize=1600,1000 --projection=p --colorscheme=Tomorrow \
	  --camera=0,-6,-3,62,0,-8,210 -D 'preset="bangle"' $<
$(OUT)/creation_cuff_ring.png: $(SCAD)
	$(XVFB) $(OPENSCAD) -o $@ --imgsize=1600,1000 --projection=p --colorscheme=Tomorrow \
	  --camera=0,-2,-1,62,0,-8,60 -D 'preset="ring"' $<
$(OUT)/creation_cuff_top.png: $(SCAD)
	$(XVFB) $(OPENSCAD) -o $@ --imgsize=1200,1200 --projection=o --colorscheme=Tomorrow \
	  --camera=0,0,0,0,0,0,200 -D 'preset="bangle"' $<
$(OUT)/creation_cuff_front.png: $(SCAD)
	$(XVFB) $(OPENSCAD) -o $@ --imgsize=1600,800 --projection=o --colorscheme=Tomorrow \
	  --camera=0,0,0,90,0,0,200 -D 'preset="bangle"' $<

clean:
	rm -f $(STLS) $(PNGS)

.PHONY: all stl png clean
