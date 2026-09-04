// =============================================================================
//  Creation Cuff — parametric open bangle / ring with two reaching hands
//  A segmented robotic hand and an organic human hand reach for each other
//  across the opening of a hammered round band ("Creation of Adam" pose).
//
//  Units: millimetres.  Y- is the front (the opening), Z+ is the back of the
//  hands (they lie in the band plane and curl their fingers downward).
//
//  Usage
//    - Open in OpenSCAD, pick a preset in the Customizer, F6, export STL.
//    - Or from a shell:  make        (see Makefile next to this file)
//    - `part` lets you export the band and each hand as separate castings.
// =============================================================================

/* [Preset] */
// bangle = wrist cuff, ring = finger ring, custom = use the "Custom sizing" values
preset = "bangle"; // ["bangle", "ring", "custom"]
// What to render / export
part = "assembly"; // ["assembly", "band", "human_hand", "robot_hand"]

/* [Custom sizing (preset = "custom")] */
// Inside diameter measured at the thin back of the band
custom_inner_diameter = 62;
// Diameter of the round band profile at the back
custom_band_thickness = 5;
// Diameter of the band profile where it swells into the wrists
custom_band_swell = 8;
// Opening between the two band ends, in degrees (the hands fill it)
custom_gap_angle = 95;
// Distance left between the two index fingertips
custom_tip_gap = 3;

/* [Pose] */
// Pull the meeting point inward from the band circle (mm). 0 = tips meet on the circle
meet_inset = 0;
// Droop of the hands below the band plane (deg)
hand_pitch = 8;
// Twist of the hands about their own forearm axis (deg)
hand_roll = 0;
// Multiplier on the curl of the three tucked fingers
finger_curl = 1.0;
// Multiplier on finger diameters (thicken small pieces so they cast/print). Ring preset uses 1.6
custom_finger_thickness = 1.0;
// Degrees of band on each side over which it swells into the wrist
swell_span = 45;

/* [Surface] */
// Random hammered relief on the outside of the band, as a fraction of the band radius (0 = smooth)
hammer = 0.05;
hammer_seed = 7;

/* [Quality] */
// Facets per circle. 24 previews fast, 48+ for the final export
$fn = 28;
// Hull segments used to sweep the band
band_segments = 72;

// ---------------------------------------------------------------------------
//  Derived sizing
// ---------------------------------------------------------------------------
inner_d  = preset == "bangle" ? 62  : preset == "ring" ? 17.3 : custom_inner_diameter;
band_t   = preset == "bangle" ? 5   : preset == "ring" ? 1.9  : custom_band_thickness;
swell_t  = preset == "bangle" ? 8   : preset == "ring" ? 2.8  : custom_band_swell;
gap_a    = preset == "bangle" ? 95  : preset == "ring" ? 100  : custom_gap_angle;
tip_gap  = preset == "bangle" ? 3   : preset == "ring" ? 0.8  : custom_tip_gap;
finger_thickness = preset == "bangle" ? 1.0 : preset == "ring" ? 1.6 : custom_finger_thickness;

Ri        = inner_d / 2;              // inner radius (constant all the way round)
th_start  = -90 + gap_a / 2;          // right-hand end of the band (x > 0)
th_end    = 270 - gap_a / 2;          // left-hand end of the band (x < 0)
arc       = th_end - th_start;

function smooth(u) = u * u * (3 - 2 * u);
// Band profile radius at polar angle th: swells from band_t/2 to swell_t/2 near each end
function band_r(th) =
    let(u = min(1, min(th - th_start, th_end - th) / swell_span))
    swell_t / 2 + (band_t / 2 - swell_t / 2) * smooth(u);

// Centre of the band profile at angle th (keeps the inner face on the inner circle)
function band_c(th, r) = [(Ri + r) * cos(th), (Ri + r) * sin(th), 0];

// Where the two index fingertips meet
meet_pt   = [0, -(Ri + swell_t / 2) + meet_inset, 0];

function end_pt(side)  = band_c(side > 0 ? th_start : th_end, swell_t / 2);
function hand_vec(side) = meet_pt - end_pt(side);
function hand_len(side) = norm(hand_vec(side)) - tip_gap / 2;
function hand_yaw(side) = let(v = hand_vec(side)) atan2(v[1], v[0]);

echo(str("inner Ø ", inner_d, " mm, band ", band_t, "→", swell_t, " mm, gap ", gap_a, "°"));
hammer_mm = hammer * band_t / 2;
min_feature = 2 * 0.032 * finger_thickness * hand_len(1);   // pinky-tip diameter, the thinnest thing in the model
echo(str("hand length (wrist → index tip) ", hand_len(1), " mm"));
echo(str("thinnest feature (pinky tip) ", min_feature, " mm"));
if (min_feature < 0.8)
    echo("WARNING: thinnest feature is under 0.8 mm - hard to cast/print. Widen custom_gap_angle or scale up.");

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
module finger(L, r, curl, nail = false) { finger_(L, r * finger_thickness, curl, nail, 0); }
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
module robot_finger(L, r, curl, gw) { robot_finger_(L, r * finger_thickness, curl, gw, 0); }
module robot_finger_(L, r, curl, gw, i) {
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
            translate([L[i] * 0.5, 0, 0]) rotate([0, 90, 0]) difference() {
                cylinder(h = gw, r = r[i] * 1.5, center = true);
                cylinder(h = gw * 2, r = (r[i] + r[i + 1]) / 2 - gw * 0.6, center = true);
            }
        }
        // conical tip pad
        if (i == len(L) - 1) hull() {
            translate([L[i], 0, 0]) sphere(r[i + 1]);
            translate([L[i] + r[i + 1] * 0.9, 0, -r[i + 1] * 0.1]) sphere(r[i + 1] * 0.45);
        }
        translate([L[i], 0, 0]) robot_finger_(L, r, curl, gw, i + 1);
    }
}

// ---------------------------------------------------------------------------
//  Band
// ---------------------------------------------------------------------------
module band_ball(i, rnd) {
    th = th_start + arc * i / band_segments;
    r  = band_r(th) + rnd[i];
    translate(band_c(th, r)) sphere(r);
}

module band() {
    rnd = rands(-hammer_mm, hammer_mm, band_segments + 1, hammer_seed);
    for (i = [0 : band_segments - 1]) hull() {
        band_ball(i, rnd);
        band_ball(i + 1, rnd);
    }
}

// ---------------------------------------------------------------------------
//  Human hand.  Wrist at origin, fingers along +X, thumb on +Y, back of hand +Z.
//  L = wrist → index fingertip.  wrist_r = radius of the band it grows out of.
// ---------------------------------------------------------------------------
module human_hand(L, wrist_r) {
    s  = L;
    pl = 0.47 * s;  pw = 0.40 * s;  pt = 0.13 * s;   // palm length / width / thickness
    ww = 0.26 * s;  wt = 0.17 * s;                   // wrist width / thickness

    // forearm blending into the band bulb
    hull() {
        translate([-0.30 * s, 0, 0]) sphere(wrist_r);
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

    fy = [0.36, 0.12, -0.12, -0.36] * pw;
    c  = finger_curl;
    // index — extended, pointing at the other hand
    translate([pl, fy[0], 0]) rotate([0, 0, 3])
        finger([0.24, 0.16, 0.13] * s, [0.056, 0.051, 0.046, 0.040] * s, [-3, 5, 7], nail = true);
    // middle, ring, pinky — tucked
    translate([pl, fy[1], 0.005 * s])
        finger([0.26, 0.17, 0.13] * s, [0.056, 0.051, 0.046, 0.040] * s, [40, 55, 45] * c);
    translate([pl - 0.01 * s, fy[2], 0]) rotate([0, 0, -3])
        finger([0.24, 0.16, 0.12] * s, [0.052, 0.048, 0.043, 0.037] * s, [48, 62, 48] * c);
    translate([pl - 0.04 * s, fy[3], -0.01 * s]) rotate([0, 0, -8])
        finger([0.19, 0.13, 0.10] * s, [0.046, 0.042, 0.037, 0.032] * s, [55, 68, 50] * c);
    // thumb — out to the side, tucking under toward the index
    translate([0.24 * s, pw * 0.44, -pt * 0.30]) rotate([0, 35, 38]) rotate([-65, 0, 0])
        finger([0.20, 0.15] * s, [0.072, 0.060, 0.048] * s, [20, 38], nail = true);
}

// ---------------------------------------------------------------------------
//  Robot hand.  Same frame as human_hand.
// ---------------------------------------------------------------------------
module robot_hand(L, wrist_r) {
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
    c  = finger_curl;
    // index — extended
    translate([pl, fy[0], 0]) rotate([0, 0, 3])
        robot_finger([0.24, 0.16, 0.13] * s, [0.058, 0.052, 0.046, 0.038] * s, [-2, 4, 6], gw);
    // tucked fingers
    translate([pl, fy[1], 0])
        robot_finger([0.26, 0.17, 0.13] * s, [0.058, 0.052, 0.046, 0.038] * s, [40, 58, 45] * c, gw);
    translate([pl - 0.01 * s, fy[2], 0]) rotate([0, 0, -3])
        robot_finger([0.24, 0.16, 0.12] * s, [0.054, 0.049, 0.043, 0.036] * s, [48, 64, 48] * c, gw);
    translate([pl - 0.04 * s, fy[3], -0.005 * s]) rotate([0, 0, -8])
        robot_finger([0.19, 0.13, 0.10] * s, [0.048, 0.043, 0.038, 0.032] * s, [55, 70, 50] * c, gw);
    // thumb on a side pivot
    translate([0.20 * s, pw * 0.46, -pt * 0.15]) {
        rotate([0, 0, 50]) rotate([90, 0, 0]) cylinder(h = 0.12 * s, r = 0.07 * s, center = true);
        rotate([0, 20, 50]) rotate([-45, 0, 0])
            robot_finger([0.21, 0.15] * s, [0.072, 0.062, 0.050] * s, [15, 32], gw);
    }
}

// ---------------------------------------------------------------------------
//  Assembly
// ---------------------------------------------------------------------------
// side = +1 places on the right band end (human), -1 on the left (robot).
// Both hands are posed thumb-forward (toward -Y, the viewer).
module place_hand(side) {
    translate(end_pt(side))
        rotate([0, 0, hand_yaw(side)])
        rotate([side * hand_roll, hand_pitch, 0]) {
            if (side < 0) mirror([0, 1, 0]) children();
            else children();
        }
}

module assembly() {
    band();
    place_hand(+1) human_hand(hand_len(+1), swell_t / 2);
    place_hand(-1) robot_hand(hand_len(-1), swell_t / 2);
}

if (part == "assembly")        assembly();
else if (part == "band")       band();
else if (part == "human_hand") human_hand(hand_len(+1), swell_t / 2);
else if (part == "robot_hand") robot_hand(hand_len(-1), swell_t / 2);
