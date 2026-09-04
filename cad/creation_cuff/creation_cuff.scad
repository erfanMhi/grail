// =============================================================================
//  Creation Cuff — parametric open bangle / ring with two reaching hands
//  A segmented robotic hand and an organic human hand reach for each other
//  across the opening of a hammered band ("Creation of Adam" pose).
//  bangle preset: round band, both ends swell into bulbs (the cuff photo).
//  ring preset:   wide flat hammered band, the human arm *is* the band and
//                 the robot hand grows out of a collared bulb (the ring photo).
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
// Radial thickness of the band at the back
custom_band_thickness = 5;
// Axial width of the band at the back (= thickness for a round band)
custom_band_width = 5;
// Diameter of the bulb the robot hand grows out of
custom_band_swell = 8;
// How the band meets the human hand: a matching bulb, or tapering into the forearm
custom_human_end = "bulb"; // ["bulb", "forearm"]
// Opening between the two band ends, in degrees (the hands fill it)
custom_gap_angle = 95;
// Distance left between the two index fingertips
custom_tip_gap = 3;

/* [Pose] */
// 0 = each hand continues the band's curve, 1 = each hand points straight across the opening.
// The hands are lengthened so the index tips still end up tip_gap apart.
wrist_bend = 0.55;
// Droop of the hands below the band plane (deg)
hand_pitch = 8;
// Twist of the hands about their own forearm axis (deg)
hand_roll = 0;
// Multiplier on the curl of the three tucked fingers
finger_curl = 1.25;
// Multiplier on finger diameters (thicken small pieces so they cast/print). Ring preset uses 1.5
custom_finger_thickness = 1.0;
// Degrees of band on each side over which it swells into the wrist
swell_span = 45;

/* [Surface] */
// Soft organic undulation of the band's outer surface, as a fraction of its radius (0 = perfectly even)
hammer = 0.06;
hammer_seed = 7;
// Number of hammer dimples planished into the outside of the band (0 = none; they cost render time)
custom_dimples = 0;
// Dimple size and depth as fractions of the band's radial half-thickness
dimple_size = 0.8;
dimple_depth = 0.12;
dimple_seed = 3;

/* [Quality] */
// Facets per circle. 24 previews fast, 48+ for the final export
$fn = 28;
// Hull segments used to sweep the band
band_segments = 72;

// ---------------------------------------------------------------------------
//  Derived sizing
// ---------------------------------------------------------------------------
inner_d  = preset == "bangle" ? 62  : preset == "ring" ? 17.3 : custom_inner_diameter;
band_t   = preset == "bangle" ? 5   : preset == "ring" ? 2.2  : custom_band_thickness;
band_w   = preset == "bangle" ? 5   : preset == "ring" ? 2.8  : custom_band_width;
swell_t  = preset == "bangle" ? 8   : preset == "ring" ? 3.3  : custom_band_swell;
gap_a    = preset == "bangle" ? 95  : preset == "ring" ? 108  : custom_gap_angle;
tip_gap  = preset == "bangle" ? 3   : preset == "ring" ? 0.8  : custom_tip_gap;
finger_thickness = preset == "bangle" ? 1.0 : preset == "ring" ? 1.15 : custom_finger_thickness;
human_end   = preset == "bangle" ? "bulb" : preset == "ring" ? "forearm" : custom_human_end;
dimples     = preset == "bangle" ? 0 : preset == "ring" ? 96 : custom_dimples;

Ri        = inner_d / 2;              // inner radius (constant all the way round)
th_start  = -90 + gap_a / 2;          // right-hand end of the band (x > 0)
th_end    = 270 - gap_a / 2;          // left-hand end of the band (x < 0)
arc       = th_end - th_start;

function smooth(u) = u * u * (3 - 2 * u);
// Band profile [radial half-thickness, axial half-width] at each end.
// side +1 = human end (th_start), -1 = robot end (th_end)
function end_prof(side) =
    side < 0 || human_end == "bulb" ? [swell_t / 2, swell_t / 2]
                                    : [band_t / 2 * 1.12, band_w / 2 * 1.18];   // forearm: fuller than the band
back_prof = [band_t / 2, band_w / 2];
// Band profile at polar angle th: back profile, morphing into each end profile over swell_span
function band_r(th) =
    let(du = th - th_start, dd = th_end - th,
        side = du < dd ? 1 : -1,
        u = min(1, min(du, dd) / swell_span),
        e = end_prof(side))
    e + (back_prof - e) * smooth(u);

// Centre of the band profile at angle th (keeps the inner face on the inner circle)
function band_c(th, r) = [(Ri + r) * cos(th), (Ri + r) * sin(th), 0];

function end_pt(side)  = band_c(side > 0 ? th_start : th_end, end_prof(side)[0]);
// Radius of the stub each hand grows out of
function wrist_r(side) = side < 0 || human_end == "bulb" ? swell_t / 2 : end_prof(side);
// Hand heading: the band's tangent at its end, bent toward the chord by wrist_bend.
// Right hand (side +1) heads -X across the gap, left hand +X; both are symmetric in X.
function tangent_yaw(side) = side > 0 ? (th_start - 90) : (th_end + 90);
function chord_yaw(side)   = side > 0 ? 180 : 0;
function hand_yaw(side)    = let(t = tangent_yaw(side), c = chord_yaw(side),
                                 d = ((c - t + 540) % 360) - 180)   // shortest turn from t to c
                             t + wrist_bend * d;
function hand_dir(side)    = [cos(hand_yaw(side)), sin(hand_yaw(side)), 0];
// Length so that the two index tips (which meet on the x = 0 plane) are tip_gap apart
function hand_len(side)    = let(e = end_pt(side), d = hand_dir(side))
                             (side * tip_gap / 2 - e[0]) / d[0];
function tip_pt(side)      = end_pt(side) + hand_len(side) * hand_dir(side);

echo(str("inner Ø ", inner_d, " mm, band ", band_t, " × ", band_w, " mm, bulb Ø ", swell_t, " mm, gap ", gap_a, "°"));
// low-frequency, seed-shifted waves: smooth like a planished surface, never faceted
function wave(th) = let(k = hammer_seed * 37)
    0.5 * sin(3 * th + k) + 0.3 * sin(7 * th + 2.3 * k) + 0.2 * sin(13 * th + 5.1 * k);
min_feature = 2 * 0.032 * finger_thickness * hand_len(1);   // pinky-tip diameter, the thinnest thing in the model
echo(str("hand length (wrist → index tip) ", hand_len(1), " mm, heading ", hand_yaw(1), "°, tips reach y = ", tip_pt(1)[1], " mm"));
echo(str("thinnest feature (pinky tip) ", min_feature, " mm"));
if (min_feature < 0.6)
    echo("WARNING: thinnest feature is under 0.6 mm - below what most casters accept. Raise custom_finger_thickness.");

use <hands.scad>

// ---------------------------------------------------------------------------
//  Band
// ---------------------------------------------------------------------------
// Profile at th including the organic undulation (radial fully, axial half as much)
function band_prof(th) = let(r = band_r(th), h = hammer * wave(th))
    [r[0] * (1 + h), r[1] * (1 + 0.5 * h)];

module band_ball(i) {
    th = th_start + arc * i / band_segments;
    r  = band_prof(th);
    translate(band_c(th, r[0])) scale([r[0], r[0], r[1]]) sphere(1);
}

// Hammer marks: shallow spheres subtracted from the outer face, each one seated on
// the surface normal of the oval profile so the rows near the edges bite as deep as
// the middle row.  Kept clear of the hands.
module dimple_cutters() {
    rows = 3;
    per  = ceil(dimples / rows);
    rr   = rands(0, 1, rows * per * 3, dimple_seed);
    a0   = th_start + swell_span * 0.35;
    a1   = th_end - swell_span * 0.35;
    for (row = [0 : rows - 1], i = [0 : per - 1]) {
        k   = row * per + i;
        th  = a0 + (a1 - a0) * (i + 0.5 + 0.5 * (row % 2) + 0.5 * (rr[3 * k] - 0.5)) / per;
        r   = band_prof(th);
        phi = (row - (rows - 1) / 2) * 48 + 20 * (rr[3 * k + 1] - 0.5);   // angle round the profile
        d   = r[0] * dimple_size * (0.75 + 0.5 * rr[3 * k + 2]);
        // surface point and outward normal of the ellipse (radial, axial)
        sp  = [Ri + r[0] + r[0] * cos(phi), r[1] * sin(phi)];
        nn  = [cos(phi) / r[0], sin(phi) / r[1]] / norm([cos(phi) / r[0], sin(phi) / r[1]]);
        c   = sp + nn * (d - r[0] * dimple_depth);
        translate([c[0] * cos(th), c[0] * sin(th), c[1]]) sphere(d, $fn = 24);
    }
}

module band() {
    difference() {
        for (i = [0 : band_segments - 1]) hull() { band_ball(i); band_ball(i + 1); }
        if (dimples > 0) dimple_cutters();
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
    place_hand(+1) human_hand(hand_len(+1), wrist_r(+1), finger_thickness, finger_curl);
    place_hand(-1) robot_hand(hand_len(-1), wrist_r(-1), finger_thickness, finger_curl);
}

if (part == "assembly")        assembly();
else if (part == "band")       band();
else if (part == "human_hand") human_hand(hand_len(+1), wrist_r(+1), finger_thickness, finger_curl);
else if (part == "robot_hand") robot_hand(hand_len(-1), wrist_r(-1), finger_thickness, finger_curl);
