// =============================================================================
//  hands.scad — the two hands shared by creation_cuff.scad and creation_ring.scad
//
//  Both hands share one frame: wrist at the origin, fingers along +X, thumb on
//  +Y, back of the hand +Z, fingers curling toward -Z.  L is the distance from
//  the wrist to the tip of the extended index finger; every other dimension is
//  a fraction of L.  Pull in with:  use <hands.scad>
// =============================================================================

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------
module ellipsoid(r) { scale(r) sphere(1); }

// Cylinder along +X from x=0 to x=h
module xcyl(h, r1, r2 = undef) {
    rotate([0, 90, 0]) cylinder(h = h, r1 = r1, r2 = (r2 == undef ? r1 : r2));
}

// Organic finger: chain of hulled spheres.  L = phalanx lengths, r = radii at
// each joint (len(L)+1 entries), curl = flexion per joint in degrees.
module finger(L, r, curl, nail = false, ft = 1) { finger_(L, r * ft, curl, nail, 0); }
module finger_(L, r, curl, nail, i) {
    if (i < len(L)) rotate([0, curl[i], 0]) {
        hull() {
            scale([1, 1, 0.88]) sphere(r[i]);
            translate([L[i], 0, 0]) scale([1, 1, 0.88]) sphere(r[i + 1]);
        }
        // knuckle crease bulge on the back of the joint
        if (i > 0) translate([0, 0, r[i] * 0.15]) scale([0.9, 1.05, 0.9]) sphere(r[i]);
        if (nail && i == len(L) - 1)
            translate([L[i] - r[i + 1] * 0.5, 0, r[i + 1] * 0.55])
                scale([1.3, 0.85, 0.22]) sphere(r[i + 1]);
        translate([L[i], 0, 0]) finger_(L, r, curl, nail, i + 1);
    }
}

// Mechanical finger: tapered tubes with pinned knuckle joints and panel lines.
// gpos = where along each phalanx the panel groove sits (vary it between neighbouring
// fingers so overlapping tubes never share a groove and trap a sliver void)
module robot_finger(L, r, curl, gw, ft = 1, gpos = 0.5) { robot_finger_(L, r * ft, curl, gw, gpos, 0); }
module robot_finger_(L, r, curl, gw, gpos, i) {
    if (i < len(L)) rotate([0, curl[i], 0]) {
        // knuckle pin (axis = Y) with washers
        rotate([90, 0, 0]) cylinder(h = r[i] * 2.3, r = r[i] * 1.02, center = true);
        rotate([90, 0, 0]) cylinder(h = r[i] * 2.7, r = r[i] * 0.55, center = true);
        // phalanx tube with a panel groove
        difference() {
            hull() {
                xcyl(L[i], r[i], r[i + 1]);
                translate([L[i], 0, 0]) sphere(r[i + 1]);
            }
            translate([L[i] * gpos, 0, 0]) rotate([0, 90, 0]) difference() {
                cylinder(h = gw, r = r[i] * 1.5, center = true);
                cylinder(h = gw * 2, r = (r[i] + r[i + 1]) / 2 - gw * 0.6, center = true);
            }
        }
        // conical tip pad
        if (i == len(L) - 1) hull() {
            translate([L[i], 0, 0]) sphere(r[i + 1]);
            translate([L[i] + r[i + 1] * 0.9, 0, -r[i + 1] * 0.1]) sphere(r[i + 1] * 0.45);
        }
        translate([L[i], 0, 0]) robot_finger_(L, r, curl, gw, gpos, i + 1);
    }
}


// ---------------------------------------------------------------------------
//  Human hand.  Wrist at origin, fingers along +X, thumb on +Y, back of hand +Z.
//  L = wrist → index fingertip.  wrist_r = radius of the band it grows out of,
//  or [side, axial] half-sizes when the band is flat.
//  ft = multiplier on finger diameters, curl = multiplier on tucked-finger flexion.
// ---------------------------------------------------------------------------
module human_hand(L, wrist_r, ft = 1, curl = 1) {
    s  = L;
    pl = 0.47 * s;  pw = 0.40 * s;  pt = 0.13 * s;   // palm length / width / thickness
    ww = 0.26 * s;  wt = 0.17 * s;                   // wrist width / thickness

    // forearm blending into the band: wrist_r is a radius (round bulb) or
    // [side, axial] half-sizes (a flat band continuing as the forearm)
    flat = is_list(wrist_r);
    wr   = flat ? wrist_r : [wrist_r, wrist_r];
    hull() {
        // a flat band already reaches the wrist, so its stub is short; a bulb needs reach
        translate([flat ? -0.12 * s : -0.30 * s, 0, 0]) ellipsoid([wr[0], wr[0], wr[1]]);
        translate([-0.06 * s, 0, 0]) ellipsoid([0.10 * s, ww / 2, wt / 2]);
    }
    // bony wrist knob on the thumb side
    translate([-0.02 * s, ww * 0.45, wt * 0.15]) sphere(wt * 0.28);
    // palm
    hull() {
        ellipsoid([0.12 * s, ww / 2, wt / 2]);
        translate([pl, 0, 0]) ellipsoid([0.09 * s, pw / 2, pt / 2]);
    }
    // metacarpal dome on the back of the hand
    translate([pl * 0.60, 0, pt * 0.18]) ellipsoid([pl * 0.42, pw * 0.36, pt * 0.42]);
    // thenar (thumb) pad
    translate([0.24 * s, pw * 0.28, -pt * 0.25]) ellipsoid([0.15 * s, pw * 0.20, pt * 0.40]);

    fy = [0.36, 0.12, -0.12, -0.36] * pw;
    c  = curl;
    // index — extended, pointing at the other hand
    translate([pl, fy[0], 0]) rotate([0, 0, 3])
        finger([0.24, 0.16, 0.13] * s, [0.056, 0.051, 0.046, 0.040] * s, [-3, 5, 7], nail = true, ft = ft);
    // middle, ring, pinky — tucked
    translate([pl, fy[1], 0.005 * s])
        finger([0.26, 0.17, 0.13] * s, [0.056, 0.051, 0.046, 0.040] * s, [40, 55, 45] * c, ft = ft);
    translate([pl - 0.01 * s, fy[2], 0]) rotate([0, 0, -3])
        finger([0.24, 0.16, 0.12] * s, [0.052, 0.048, 0.043, 0.037] * s, [48, 62, 48] * c, ft = ft);
    translate([pl - 0.04 * s, fy[3], -0.01 * s]) rotate([0, 0, -8])
        finger([0.19, 0.13, 0.10] * s, [0.046, 0.042, 0.037, 0.032] * s, [55, 68, 50] * c, ft = ft);
    // thumb — out to the side, tucking under toward the index
    translate([0.24 * s, pw * 0.44, -pt * 0.30]) rotate([0, 35, 38]) rotate([-65, 0, 0])
        finger([0.20, 0.15] * s, [0.072, 0.060, 0.048] * s, [20, 38], nail = true, ft = ft);
}

// ---------------------------------------------------------------------------
//  Robot hand.  Same frame as human_hand.
// ---------------------------------------------------------------------------
module robot_hand(L, wrist_r, ft = 1, curl = 1) {
    s  = L;
    pl = 0.46 * s;  pw = 0.44 * s;  pt = 0.17 * s;
    ww = 0.32 * s;  wt = 0.24 * s;
    gw = 0.018 * s;                                  // panel-groove width
    cr = 0.035 * s;                                  // palm corner radius

    // forearm stub into the band
    hull() {
        translate([-0.36 * s, 0, 0]) sphere(wrist_r);
        translate([-0.22 * s, 0, 0]) xcyl(0.01 * s, wrist_r * 0.98);
    }
    // wrist collar: stacked rings / bellows over a solid core
    translate([-0.26 * s, 0, 0]) xcyl(0.27 * s, wrist_r * 0.90);
    for (i = [0 : 4]) translate([-0.23 * s + i * 0.042 * s, 0, 0])
        xcyl(0.046 * s, i % 2 == 0 ? wrist_r * 1.10 : wrist_r * 0.96);
    // ball joint at the wrist
    sphere(wrist_r * 0.92);

    // palm: rounded, tapered box with panel lines
    difference() {
        hull() for (x = [0, pl], z = [-1, 1]) {
            w = (x == 0 ? ww : pw) / 2 - cr;
            t = (x == 0 ? wt : pt) / 2 - cr;
            for (y = [-w, w]) translate([x, y, z * t]) sphere(cr);
        }
        // transverse groove
        translate([pl * 0.42, 0, pt / 2]) cube([gw, pw * 1.2, gw * 1.6], center = true);
        // longitudinal grooves between the finger rays
        for (y = [-0.24, 0, 0.24] * pw)
            translate([pl * 0.72, y, pt / 2]) cube([pl * 0.56, gw, gw * 1.6], center = true);
        // underside groove
        translate([pl * 0.55, 0, -pt / 2]) cube([gw, pw * 1.2, gw * 1.6], center = true);
    }
    // raised back plate
    translate([pl * 0.50, 0, pt / 2 - gw]) hull()
        for (y = [-1, 1], x = [-1, 1])
            translate([x * pl * 0.22, y * pw * 0.30, 0]) cylinder(h = gw * 2, r = cr);

    fy = [0.36, 0.12, -0.12, -0.36] * pw;
    c  = curl;
    // index — extended
    translate([pl, fy[0], 0]) rotate([0, 0, 3])
        robot_finger([0.24, 0.16, 0.13] * s, [0.058, 0.052, 0.046, 0.038] * s, [-2, 4, 6], gw, ft = ft);
    // tucked fingers
    translate([pl, fy[1], 0])
        robot_finger([0.26, 0.17, 0.13] * s, [0.058, 0.052, 0.046, 0.038] * s, [40, 58, 45] * c, gw, ft = ft, gpos = 0.40);
    translate([pl - 0.01 * s, fy[2], 0]) rotate([0, 0, -3])
        robot_finger([0.24, 0.16, 0.12] * s, [0.054, 0.049, 0.043, 0.036] * s, [48, 64, 48] * c, gw, ft = ft, gpos = 0.60);
    translate([pl - 0.04 * s, fy[3], -0.005 * s]) rotate([0, 0, -8])
        robot_finger([0.19, 0.13, 0.10] * s, [0.048, 0.043, 0.038, 0.032] * s, [55, 70, 50] * c, gw, ft = ft);
    // thumb on a side pivot
    translate([0.20 * s, pw * 0.46, -pt * 0.15]) {
        rotate([0, 0, 50]) rotate([90, 0, 0]) cylinder(h = 0.12 * s, r = 0.07 * s, center = true);
        rotate([0, 20, 50]) rotate([-45, 0, 0])
            robot_finger([0.21, 0.15] * s, [0.072, 0.062, 0.050] * s, [15, 32], gw, ft = ft);
    }
}

