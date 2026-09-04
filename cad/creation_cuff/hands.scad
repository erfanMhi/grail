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

// Mechanical finger: tapered tubes with a ball at every knuckle, two raised
// ring bands on each segment and a pair of small rivets on its back.  Every
// element is a body of revolution, so the finger stays smooth and curvy.
module robot_finger(L, r, curl, gw, ft = 1) { robot_finger_(L, r * ft, curl, gw, 0); }
module robot_finger_(L, r, curl, gw, i) {
    if (i < len(L)) rotate([0, curl[i], 0]) {
        sphere(r[i] * 1.04);                                     // ball knuckle
        hull() {                                                 // segment tube
            xcyl(L[i], r[i] * 0.94, r[i + 1] * 0.94);
            translate([L[i], 0, 0]) sphere(r[i + 1] * 0.94);
        }
        for (f = [0.32, 0.72]) {                                 // raised ring bands
            rf = r[i] + (r[i + 1] - r[i]) * f;
            translate([L[i] * f - gw * 0.9, 0, 0]) xcyl(gw * 1.8, rf * 1.03);
        }
        for (x = [-1, 1]) {                                      // rivets on the back
            xf = 0.52 + x * 0.20;
            rf = r[i] + (r[i + 1] - r[i]) * xf;
            translate([L[i] * xf, 0, rf * 0.90]) sphere(gw * 1.1);
        }
        if (i == len(L) - 1) hull() {                            // rounded tip pad
            translate([L[i], 0, 0]) sphere(r[i + 1] * 0.94);
            translate([L[i] + r[i + 1] * 0.8, 0, -r[i + 1] * 0.1]) sphere(r[i + 1] * 0.5);
        }
        translate([L[i], 0, 0]) robot_finger_(L, r, curl, gw, i + 1);
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
    pl = 0.47 * s;  pw = 0.42 * s;  pt = 0.18 * s;   // palm length / width / thickness
    ww = 0.26 * s;  wt = 0.17 * s;                   // wrist width / thickness

    // forearm blending into the band: wrist_r is a radius (round bulb) or
    // [side, axial] half-sizes (a flat band continuing as the forearm)
    flat = is_list(wrist_r);
    wr   = flat ? wrist_r : [wrist_r, wrist_r];
    hull() {
        // a flat band already reaches the wrist, so its stub is short; a bulb needs reach
        translate([flat ? -0.12 * s : -0.30 * s, 0, 0]) ellipsoid(flat ? [wr[0] * 0.9, wr[0] * 0.9, wr[1] * 0.9] : [wr[0], wr[0], wr[1]]);
        translate([-0.06 * s, 0, 0]) ellipsoid([0.10 * s, ww / 2, wt / 2]);
    }
    // palm
    hull() {
        ellipsoid([0.12 * s, ww / 2, wt / 2]);
        translate([pl, 0, 0]) ellipsoid([0.09 * s, pw / 2, pt / 2]);
    }
    // metacarpal dome on the back of the hand
    translate([pl * 0.60, 0, pt * 0.18]) ellipsoid([pl * 0.42, pw * 0.36, pt * 0.42]);
    // thenar (thumb) pad
    translate([0.24 * s, pw * 0.28, -pt * 0.25]) ellipsoid([0.15 * s, pw * 0.20, pt * 0.40]);
    // extensor tendons fanning from the wrist to each knuckle, seated just under
    // the back surface so they read as fine ridges everywhere
    function back_z(x, y) = max(
        pt * 0.18 + pt * 0.42 * sqrt(max(0, 1 - pow((x - pl * 0.6) / (pl * 0.42), 2) - pow(y / (pw * 0.36), 2))),
        pt / 2 * sqrt(max(0, 1 - pow(y / (pw / 2), 2))) * (x > 0.08 * s ? 1 : 0));
    for (y = [0.36, 0.12, -0.12, -0.36] * pw)
        for (seg = [[0.18, 0.55, 0.30, 0.80], [0.30, 0.80, 0.44, 1.0]]) hull() {
            x0 = seg[0] * s; y0 = y * seg[1]; x1 = seg[2] * s; y1 = y * seg[3];
            translate([x0, y0, back_z(x0, y0) - 0.008 * s]) sphere(0.015 * s);
            translate([x1, y1, back_z(x1, y1) - 0.008 * s]) sphere(0.016 * s);
        }

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
module robot_hand(L, wrist_r, ft_in = 1, curl = 1) {
    s  = L;
    ft = ft_in * 1.25;                               // mechanical fingers are stouter than flesh
    pl = 0.46 * s;  pw = 0.44 * s;  pt = 0.19 * s;
    ww = 0.32 * s;  wt = 0.24 * s;
    gw = 0.018 * s;                                  // seam width

    // forearm stub into the band
    hull() {
        translate([-0.36 * s, 0, 0]) sphere(wrist_r);
        translate([-0.22 * s, 0, 0]) xcyl(0.01 * s, wrist_r * 0.98);
    }
    // wrist collar: one wide cuff with raised rims and four rivets
    translate([-0.26 * s, 0, 0]) xcyl(0.27 * s, wrist_r * 0.90);
    translate([-0.25 * s, 0, 0]) xcyl(0.21 * s, wrist_r * 1.02);
    for (x = [-0.25, -0.07]) translate([x * s, 0, 0]) xcyl(0.03 * s, wrist_r * 1.10);
    for (a = [45 : 90 : 315]) rotate([a, 0, 0])
        translate([-0.145 * s, 0, wrist_r * 1.0]) sphere(0.022 * s);
    // ball joint at the wrist
    sphere(wrist_r * 0.92);

    // palm: a smooth curved shell (wrist ellipsoid hulled to the knuckle ellipsoid,
    // with a domed back) and shallow seams that follow the curvature
    dz = pt * 0.20;  dx = pl * 0.46;  dy = pw * 0.42;  dr = pt * 0.48;   // back dome
    function dome_z(x, y) = dz + dr * sqrt(max(0, 1 - pow((x - pl * 0.56) / dx, 2) - pow(y / dy, 2)));
    difference() {
        union() {
            hull() {
                ellipsoid([0.12 * s, ww / 2, wt / 2]);
                translate([pl, 0, 0]) ellipsoid([0.10 * s, pw / 2, pt / 2]);
            }
            translate([pl * 0.56, 0, dz]) ellipsoid([dx, dy, dr]);
        }
        // transverse seam across the back, traced over the dome
        for (k = [-8 : 7]) hull() for (j = [k, k + 1]) {
            y = j / 8 * dy * 0.92;  x = pl * 0.40;
            translate([x, y, dome_z(x, y)]) sphere(gw * 0.9, $fn = 12);
        }
        // three seams running toward the knuckles
        for (y = [-0.24, 0, 0.24] * pw, k = [0 : 5]) hull() for (j = [k, k + 1]) {
            x = pl * (0.48 + 0.08 * j);
            translate([x, y, dome_z(x, y)]) sphere(gw * 0.9, $fn = 12);
        }
    }

    fy = [0.36, 0.12, -0.12, -0.36] * pw;
    c  = curl;
    // index — extended
    translate([pl, fy[0], 0]) rotate([0, 0, 3])
        robot_finger([0.24, 0.16, 0.13] * s, [0.058, 0.052, 0.046, 0.038] * s, [-2, 4, 6], gw, ft = ft);
    // tucked fingers
    translate([pl, fy[1], 0])
        robot_finger([0.26, 0.17, 0.13] * s, [0.058, 0.052, 0.046, 0.038] * s, [40, 58, 45] * c, gw, ft = ft);
    translate([pl - 0.01 * s, fy[2], 0]) rotate([0, 0, -3])
        robot_finger([0.24, 0.16, 0.12] * s, [0.054, 0.049, 0.043, 0.036] * s, [48, 64, 48] * c, gw, ft = ft);
    translate([pl - 0.04 * s, fy[3], -0.005 * s]) rotate([0, 0, -8])
        robot_finger([0.19, 0.13, 0.10] * s, [0.048, 0.043, 0.038, 0.032] * s, [55, 70, 50] * c, gw, ft = ft);
    // thumb on a side pivot
    translate([0.20 * s, pw * 0.46, -pt * 0.15]) {
        rotate([0, 0, 50]) rotate([90, 0, 0]) cylinder(h = 0.12 * s, r = 0.07 * s, center = true);
        rotate([0, 20, 50]) rotate([-45, 0, 0])
            robot_finger([0.21, 0.15] * s, [0.072, 0.062, 0.050] * s, [15, 32], gw, ft = ft);
    }
}

