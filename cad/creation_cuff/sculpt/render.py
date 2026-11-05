"""Photoreal product renders of the sculpted Creation Ring with Blender Cycles.

    /opt/bpyenv/bin/python render.py --mesh ring.ply --out shot.png \
        [--view photo|front|top|back|left|right|robot|human|under] \
        [--res 1200x1200] [--samples 96] [--metal silver|white|yellow|rose] \
        [--antique 1.0] [--frames N --turntable out_dir]

Meshes are in millimetres; the scene is built in metres so lights and the
camera behave like a real tabletop product setup: a white sweep, a big
overhead softbox, two strip lights and a low fill, a long lens.
"""
import argparse
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

METALS = {
    # base colour (linear), roughness
    "silver": ((0.962, 0.949, 0.922), 0.20),
    "white": ((0.913, 0.901, 0.858), 0.19),
    "yellow": ((1.000, 0.700, 0.270), 0.20),
    "rose": ((0.930, 0.660, 0.540), 0.20),
}

# camera presets: (azimuth deg, elevation deg, lens mm, look-at offset mm, fill)
# azimuth 0 = looking from -Y toward +Y (the opening faces the camera)
VIEWS = {
    "photo": (0.0, 27.0, 135.0, (0.0, 0.0, 0.0), 0.74),
    "front": (0.0, 4.0, 135.0, (0.0, 0.0, 0.0), 0.92),
    "low": (0.0, 14.0, 135.0, (0.0, 0.0, 0.0), 0.92),
    "top": (0.0, 89.0, 135.0, (0.0, 0.0, 0.0), 0.92),
    "back": (180.0, 25.0, 135.0, (0.0, 0.0, 0.0), 0.92),
    "left": (-90.0, 20.0, 135.0, (0.0, 0.0, 0.0), 0.92),
    "right": (90.0, 20.0, 135.0, (0.0, 0.0, 0.0), 0.92),
    "robot": (-28.0, 24.0, 135.0, (-4.6, -9.5, -0.6), 0.42),
    "human": (28.0, 24.0, 135.0, (4.4, -9.5, -0.6), 0.42),
    "hands": (0.0, 20.0, 135.0, (0.0, -9.0, -0.8), 0.62),
    "under": (0.0, -35.0, 135.0, (0.0, -8.0, -1.0), 0.70),
}


def args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
    p = argparse.ArgumentParser()
    p.add_argument("--mesh", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--view", default="photo")
    p.add_argument("--az", type=float)
    p.add_argument("--el", type=float)
    p.add_argument("--fill", type=float)
    p.add_argument("--target", type=str, help="x,y,z mm look-at")
    p.add_argument("--res", default="1200x1200")
    p.add_argument("--samples", type=int, default=96)
    p.add_argument("--metal", default="silver")
    p.add_argument("--antique", type=float, default=1.0)
    p.add_argument("--floor", type=int, default=1)
    p.add_argument("--frames", type=int, default=0)
    p.add_argument("--orbit", default="", help="comma list az:el:fill:tx:ty:tz keyframes for video")
    p.add_argument("--blend", default="", help="save the .blend for inspection")
    p.add_argument("--cams", default="", help="';'-separated az:el[:radius_mm[:tx:ty:tz]] -> OUT_<i>.png")
    p.add_argument("--lens", type=float, default=0.0)
    p.add_argument("--clay", type=int, default=0, help="matte clay material for judging form")
    p.add_argument("--env", default="grad", help="grad (studio gradient) or hdri")
    return p.parse_args(argv)


def build_material(metal, antique):
    col, rough = METALS[metal]
    m = bpy.data.materials.new("metal")
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Metallic"].default_value = 1.0
    bsdf.inputs["Roughness"].default_value = rough
    # oxidised recesses: an "antique" patina where ambient occlusion is high,
    # the way a silversmith blackens then polishes back the high points
    ao = nt.nodes.new("ShaderNodeAmbientOcclusion")
    ao.samples = 16
    ao.only_local = True
    ao.inputs["Distance"].default_value = 0.0006
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.45
    ramp.color_ramp.elements[0].color = (0.0, 0.0, 0.0, 1)
    ramp.color_ramp.elements[1].position = 1.0
    ramp.color_ramp.elements[1].color = (1.0, 1.0, 1.0, 1)
    nt.links.new(ao.outputs["AO"], ramp.inputs["Fac"])
    mixc = nt.nodes.new("ShaderNodeMix")
    mixc.data_type = "RGBA"
    dark = tuple(c * 0.16 for c in col)
    mixc.inputs[6].default_value = (*dark, 1)       # A: patina
    mixc.inputs[7].default_value = (*col, 1)        # B: polished
    fac = nt.nodes.new("ShaderNodeMath")
    fac.operation = "POWER"
    nt.links.new(ramp.outputs["Color"], fac.inputs[0])
    fac.inputs[1].default_value = 1.6
    strength = nt.nodes.new("ShaderNodeMix")         # blend toward polished by (1-antique)
    strength.data_type = "FLOAT"
    strength.inputs["Factor"].default_value = 1.0 - antique
    nt.links.new(fac.outputs[0], strength.inputs[2])
    strength.inputs[3].default_value = 1.0
    nt.links.new(strength.outputs[0], mixc.inputs["Factor"])
    nt.links.new(mixc.outputs[2], bsdf.inputs["Base Color"])
    rmix = nt.nodes.new("ShaderNodeMapRange")
    rmix.inputs["From Min"].default_value = 0.0
    rmix.inputs["From Max"].default_value = 1.0
    rmix.inputs["To Min"].default_value = min(0.55, rough + 0.30 * antique)
    rmix.inputs["To Max"].default_value = rough
    nt.links.new(strength.outputs[0], rmix.inputs["Value"])
    nt.links.new(rmix.outputs["Result"], bsdf.inputs["Roughness"])
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    return m


def area(name, loc, size, power, look=(0, 0, 0), shape="RECTANGLE", size_y=None, color=(1, 1, 1)):
    ld = bpy.data.lights.new(name, "AREA")
    ld.shape = shape
    ld.size = size
    if size_y:
        ld.size_y = size_y
    ld.energy = power
    ld.color = color
    ob = bpy.data.objects.new(name, ld)
    bpy.context.scene.collection.objects.link(ob)
    ob.location = Vector(loc)
    d = Vector(look) - ob.location
    ob.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    return ob


def main():
    a = args()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    cy = scene.cycles
    cy.device = "CPU"
    cy.samples = a.samples
    cy.use_adaptive_sampling = True
    cy.adaptive_threshold = 0.015
    cy.use_denoising = True
    cy.denoiser = "OPENIMAGEDENOISE"
    cy.max_bounces = 12
    cy.glossy_bounces = 10
    cy.diffuse_bounces = 3
    cy.transmission_bounces = 0
    cy.transparent_max_bounces = 2
    cy.caustics_reflective = False
    cy.caustics_refractive = False
    cy.sample_clamp_indirect = 4.0
    w, h = (int(x) for x in a.res.split("x"))
    scene.render.resolution_x, scene.render.resolution_y = w, h
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = 0.0

    # --- the ring (mm -> m)
    bpy.ops.wm.ply_import(filepath=a.mesh)
    ring = bpy.context.selected_objects[0]
    ring.scale = (0.001, 0.001, 0.001)
    bpy.context.view_layer.objects.active = ring
    bpy.ops.object.shade_smooth()
    if a.clay:
        cm = bpy.data.materials.new("clay")
        cm.use_nodes = True
        cb = cm.node_tree.nodes["Principled BSDF"]
        cb.inputs["Base Color"].default_value = (0.62, 0.60, 0.57, 1)
        cb.inputs["Roughness"].default_value = 0.55
        cb.inputs["Specular IOR Level"].default_value = 0.35
        ring.data.materials.append(cm)
    else:
        ring.data.materials.append(build_material(a.metal, a.antique))
    bpy.context.view_layer.update()
    bb = [ring.matrix_world @ Vector(c) for c in ring.bound_box]
    lo = Vector((min(v.x for v in bb), min(v.y for v in bb), min(v.z for v in bb)))
    hi = Vector((max(v.x for v in bb), max(v.y for v in bb), max(v.z for v in bb)))
    center = (lo + hi) / 2
    radius = (hi - lo).length / 2

    # --- floor sweep
    if a.floor:
        bpy.ops.mesh.primitive_plane_add(size=1.2, location=(0, 0, lo.z - 0.00002))
        floor = bpy.context.active_object
        fm = bpy.data.materials.new("floor")
        fm.use_nodes = True
        fnt = fm.node_tree
        fb = fnt.nodes["Principled BSDF"]
        fb.inputs["Base Color"].default_value = (0.86, 0.86, 0.865, 1)
        fb.inputs["Roughness"].default_value = 0.65
        fb.inputs["Specular IOR Level"].default_value = 0.25
        # the metal sees a darker sweep than the camera does (like a studio
        # with the far floor in shadow): keeps contrast in the reflections
        dark = fnt.nodes.new("ShaderNodeBsdfDiffuse")
        dark.inputs["Color"].default_value = (0.16, 0.16, 0.165, 1)
        lpf = fnt.nodes.new("ShaderNodeLightPath")
        mixf = fnt.nodes.new("ShaderNodeMixShader")
        fnt.links.new(lpf.outputs["Is Glossy Ray"], mixf.inputs["Fac"])
        fnt.links.new(fb.outputs["BSDF"], mixf.inputs[1])
        fnt.links.new(dark.outputs["BSDF"], mixf.inputs[2])
        fnt.links.new(mixf.outputs["Shader"], fnt.nodes["Material Output"].inputs["Surface"])
        floor.data.materials.append(fm)

    # --- world: soft studio wrap for reflections
    world = bpy.data.worlds.new("studio")
    scene.world = world
    world.use_nodes = True
    wn = world.node_tree
    bg = wn.nodes["Background"]
    hdri = os.path.join(os.path.dirname(bpy.__file__), "5.2", "datafiles", "studiolights", "world", "studio.exr")
    if a.env == "hdri" and os.path.exists(hdri):
        env = wn.nodes.new("ShaderNodeTexEnvironment")
        env.image = bpy.data.images.load(hdri)
        wn.links.new(env.outputs["Color"], bg.inputs["Color"])
        bg.inputs["Strength"].default_value = 0.55
    else:
        # a photo studio as the metal sees it: bright overhead, a dark band
        # round the horizon, a pale floor, with a few vertical flags
        tc = wn.nodes.new("ShaderNodeTexCoord")
        sep = wn.nodes.new("ShaderNodeSeparateXYZ")
        wn.links.new(tc.outputs["Generated"], sep.inputs[0])
        ramp = wn.nodes.new("ShaderNodeValToRGB")
        cr = ramp.color_ramp
        cr.interpolation = "EASE"
        cr.elements[0].position = 0.0
        cr.elements[0].color = (0.30, 0.30, 0.30, 1)
        cr.elements[1].position = 1.0
        cr.elements[1].color = (2.2, 2.2, 2.2, 1)
        for pos, v in ((0.42, 0.22), (0.49, 0.015), (0.56, 0.02), (0.66, 0.10), (0.80, 0.55), (0.90, 1.4)):
            e = cr.elements.new(pos)
            e.color = (v, v, v, 1)
        mr = wn.nodes.new("ShaderNodeMapRange")
        mr.inputs["From Min"].default_value = -1.0
        mr.inputs["From Max"].default_value = 1.0
        wn.links.new(sep.outputs["Z"], mr.inputs["Value"])
        wn.links.new(mr.outputs["Result"], ramp.inputs["Fac"])
        # flags: azimuthal stripes that give the polished surfaces structure
        wave = wn.nodes.new("ShaderNodeTexWave")
        wave.wave_type = "BANDS"
        wave.bands_direction = "X"
        wave.inputs["Scale"].default_value = 1.3
        wave.inputs["Distortion"].default_value = 2.0
        wn.links.new(tc.outputs["Generated"], wave.inputs["Vector"])
        mulw = wn.nodes.new("ShaderNodeMix")
        mulw.data_type = "RGBA"
        mulw.blend_type = "MULTIPLY"
        mulw.inputs["Factor"].default_value = 0.6
        wn.links.new(ramp.outputs["Color"], mulw.inputs[6])
        wn.links.new(wave.outputs["Color"], mulw.inputs[7])
        wn.links.new(mulw.outputs[2], bg.inputs["Color"])
        bg.inputs["Strength"].default_value = 0.8
    # camera rays see a clean light-grey sweep, not the HDRI
    lp = wn.nodes.new("ShaderNodeLightPath")
    mixw = wn.nodes.new("ShaderNodeMixShader")
    bg2 = wn.nodes.new("ShaderNodeBackground")
    bg2.inputs["Color"].default_value = (0.90, 0.90, 0.905, 1)
    bg2.inputs["Strength"].default_value = 1.0
    wn.links.new(lp.outputs["Is Camera Ray"], mixw.inputs["Fac"])
    wn.links.new(bg.outputs["Background"], mixw.inputs[1])
    wn.links.new(bg2.outputs["Background"], mixw.inputs[2])
    wn.links.new(mixw.outputs["Shader"], wn.nodes["World Output"].inputs["Surface"])

    # --- softboxes (metres)
    c = center
    area("key", (c.x - 0.05, c.y - 0.06, c.z + 0.22), 0.26, 0.9, look=c, size_y=0.20)
    area("stripL", (c.x - 0.16, c.y + 0.02, c.z + 0.07), 0.04, 0.30, look=c, size_y=0.30)
    area("stripR", (c.x + 0.16, c.y + 0.03, c.z + 0.08), 0.04, 0.30, look=c, size_y=0.30)
    area("fill", (c.x, c.y - 0.25, c.z + 0.03), 0.30, 0.25, look=c, size_y=0.08)
    area("back", (c.x, c.y + 0.20, c.z + 0.12), 0.22, 0.25, look=c, size_y=0.10)

    cam_d = bpy.data.cameras.new("cam")
    cam = bpy.data.objects.new("cam", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam_d.sensor_width = 36

    def place(az, el, fill, target_mm, lens):
        tgt = Vector(target_mm) * 0.001 if target_mm is not None else c
        cam_d.lens = lens
        fov = 2 * math.atan(cam_d.sensor_width / 2 / lens)
        aspect = scene.render.resolution_y / scene.render.resolution_x
        if aspect < 1:   # landscape: the vertical field is the tighter one
            fov = 2 * math.atan(math.tan(fov / 2) * aspect)
        rad = radius * fill
        dist = rad / math.tan(fov / 2) * 1.05
        azr, elr = math.radians(az), math.radians(el)
        d = Vector((math.sin(azr) * math.cos(elr), -math.cos(azr) * math.cos(elr), math.sin(elr)))
        cam.location = tgt + d * dist
        up = Vector((0, 0, 1)) if abs(el) < 85 else Vector((0, 1, 0))
        look = (tgt - cam.location).normalized()
        right = look.cross(up).normalized()
        up2 = right.cross(look)
        R = Matrix((right, up2, -look)).transposed()
        cam.rotation_euler = R.to_euler()
        cam_d.clip_start = dist * 0.2
        cam_d.clip_end = dist * 5

    az, el, lens, tgt, fill = VIEWS[a.view]
    if a.az is not None:
        az = a.az
    if a.el is not None:
        el = a.el
    if a.fill is not None:
        fill = a.fill
    if a.target:
        tgt = tuple(float(v) for v in a.target.split(","))
    tgt_mm = tgt if any(abs(t) > 1e-9 for t in tgt) else None

    if a.frames > 0:
        # keyframed orbit: list of az:el:fill:tx:ty:tz
        keys = [tuple(float(x) for x in k.split(":")) for k in a.orbit.split(",")]
        os.makedirs(a.out, exist_ok=True)
        n = a.frames
        for f in range(n):
            # Catmull-Rom through the keys: the camera keeps moving through
            # each key instead of stopping, with an ease at the very ends
            g = f / (n - 1)
            g = g * g * (3 - 2 * g) * 0.15 + g * 0.85
            t = g * (len(keys) - 1)
            i = min(int(t), len(keys) - 2)
            u = t - i
            K = lambda m: keys[max(0, min(len(keys) - 1, m))]
            k = []
            for j in range(6):
                p0, p1, p2, p3 = K(i - 1)[j], K(i)[j], K(i + 1)[j], K(i + 2)[j]
                k.append(0.5 * (2 * p1 + (-p0 + p2) * u + (2 * p0 - 5 * p1 + 4 * p2 - p3) * u * u + (-p0 + 3 * p1 - 3 * p2 + p3) * u ** 3))
            k[2] = k[2] * 0.001 / radius if k[2] > 2.0 else k[2]   # fill given in mm when > 2
            fp = os.path.join(a.out, f"f{f:04d}.png")
            if os.path.exists(fp):
                continue      # resumable: frames already on disk are kept
            place(k[0], k[1], k[2], (k[3], k[4], k[5]), lens)
            scene.render.filepath = fp
            bpy.ops.render.render(write_still=True)
        return
    if a.lens > 0:
        lens = a.lens
    if a.cams:
        for i, cs in enumerate(a.cams.split(";")):
            v = [float(x) for x in cs.split(":")]
            caz, cel = v[0], v[1]
            if len(v) >= 3:
                rmm = v[2]
                cfill = rmm * 0.001 / radius
            else:
                cfill = fill
            ct = (v[3], v[4], v[5]) if len(v) >= 6 else tgt_mm
            place(caz, cel, cfill, ct, lens)
            scene.render.filepath = a.out.replace(".png", f"_{i}.png")
            bpy.ops.render.render(write_still=True)
        return
    place(az, el, fill, tgt_mm, lens)
    if a.blend:   # the whole studio, camera included, to open in Blender
        bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(a.blend))
    scene.render.filepath = a.out
    bpy.ops.render.render(write_still=True)


main()
