# Regenerate meshes and previews for the Creation Cuff.
# Requires OpenSCAD (2021.01+) on PATH; previews need xvfb-run on headless boxes.

SCAD    := creation_cuff.scad
RING    := creation_ring.scad
LIB     := hands.scad
OUT     := export
FN      ?= 36
OPENSCAD ?= openscad
CLEAN    := python3 clean_stl.py
XVFB    ?= $(shell command -v xvfb-run >/dev/null 2>&1 && echo "xvfb-run -a")

STLS := $(OUT)/creation_cuff_bangle.stl \
        $(OUT)/creation_cuff_ring.stl \
        $(OUT)/creation_cuff_bangle_band.stl \
        $(OUT)/creation_cuff_bangle_human_hand.stl \
        $(OUT)/creation_cuff_bangle_robot_hand.stl \
        $(OUT)/creation_ring.stl \
        $(OUT)/creation_ring_band.stl \
        $(OUT)/creation_ring_human_hand.stl \
        $(OUT)/creation_ring_robot_hand.stl

PNGS := $(OUT)/creation_cuff_bangle.png $(OUT)/creation_cuff_ring.png \
        $(OUT)/creation_cuff_top.png $(OUT)/creation_cuff_front.png \
        $(OUT)/creation_ring.png $(OUT)/creation_ring_top.png $(OUT)/creation_ring_side.png

all: stl png
stl: $(STLS)
png: $(PNGS)

$(OUT)/creation_ring.stl: $(RING) $(LIB)
	$(OPENSCAD) -o $@ -D '$$fn=$(FN)' $<
	$(CLEAN) $@
$(OUT)/creation_ring_%.stl: $(RING) $(LIB)
	$(OPENSCAD) -o $@ -D '$$fn=$(FN)' -D 'part="$*"' $<
	$(CLEAN) $@

$(OUT)/creation_cuff_bangle.stl: $(SCAD) $(LIB)
	$(OPENSCAD) -o $@ -D '$$fn=$(FN)' -D 'preset="bangle"' $<
	$(CLEAN) $@
$(OUT)/creation_cuff_ring.stl: $(SCAD) $(LIB)
	$(OPENSCAD) -o $@ -D '$$fn=$(FN)' -D 'preset="ring"' $<
	$(CLEAN) $@
$(OUT)/creation_cuff_bangle_%.stl: $(SCAD) $(LIB)
	$(OPENSCAD) -o $@ -D '$$fn=$(FN)' -D 'preset="bangle"' -D 'part="$*"' $<
	$(CLEAN) $@

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

$(OUT)/creation_ring.png: $(RING) $(LIB)
	$(XVFB) $(OPENSCAD) -o $@ --imgsize=1600,1000 --projection=p --colorscheme=Tomorrow \
	  --camera=0,0,0,58,0,40,72 $<
$(OUT)/creation_ring_top.png: $(RING) $(LIB)
	$(XVFB) $(OPENSCAD) -o $@ --imgsize=1200,1200 --projection=o --colorscheme=Tomorrow \
	  --camera=0,0,0,0,0,0,60 $<
$(OUT)/creation_ring_side.png: $(RING) $(LIB)
	$(XVFB) $(OPENSCAD) -o $@ --imgsize=1600,800 --projection=o --colorscheme=Tomorrow \
	  --camera=0,0,0,90,0,45,60 $<

clean:
	rm -f $(STLS) $(PNGS)

.PHONY: all stl png clean
