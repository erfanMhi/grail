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
finger_curl = 1.0;
// Multiplier on finger diameters (thicken small pieces so they cast/print). Ring preset uses 1.5
custom_finger_thickness = 1.0;
// Degrees of band on each side over which it swells into the wrist
swell_span = 45;

/* [Surface] */
// Random hammered relief on the outside of the band, as a fraction of the band radius (0 = smooth)
hammer = 0.05;
hammer_seed = 7;
// Facets per circle used for the band alone; low values (12-16) give planished, hammered facets. 0 = use $fn
custom_band_facets = 0;

/* [Quality] */
// Facets per circle. 24 previews fast, 48+ for the final export
$fn = 28;
// Hull segments used to sweep the band
band_segments = 72;

// ---------------------------------------------------------------------------
//  Derived sizing
// ---------------------------------------------------------------------------
inner_d  = preset == "bangle" ? 62  : preset == "ring" ? 17.3 : custom_inner_diameter;
band_t   = preset == "bangle" ? 5   : preset == "ring" ? 2.0  : custom_band_thickness;
band_w   = preset == "bangle" ? 5   : preset == "ring" ? 4.2  : custom_band_width;
swell_t  = preset == "bangle" ? 8   : preset == "ring" ? 3.4  : custom_band_swell;
gap_a    = preset == "bangle" ? 95  : preset == "ring" ? 110  : custom_gap_angle;
tip_gap  = preset == "bangle" ? 3   : preset == "ring" ? 0.8  : custom_tip_gap;
finger_thickness = preset == "bangle" ? 1.0 : preset == "ring" ? 1.5 : custom_finger_thickness;
human_end   = preset == "bangle" ? "bulb" : preset == "ring" ? "forearm" : custom_human_end;
band_facets = preset == "bangle" ? 0 : preset == "ring" ? 14 : custom_band_facets;

Ri        = inner_d / 2;              // inner radius (constant all the way round)
th_start  = -90 + gap_a / 2;          // right-hand end of the band (x > 0)
th_end    = 270 - gap_a / 2;          // left-hand end of the band (x < 0)
arc       = th_end - th_start;

function smooth(u) = u * u * (3 - 2 * u);
// Band profile [radial half-thickness, axial half-width] at each end.
// side +1 = human end (th_start), -1 = robot end (th_end)
function end_prof(side) =
    side < 0 || human_end == "bulb" ? [swell_t / 2, swell_t / 2]
                                    : [max(band_t / 2, band_w * 0.30), band_w * 0.40];
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
hammer_mm = hammer * band_t / 2;
min_feature = 2 * 0.032 * finger_thickness * hand_len(1);   // pinky-tip diameter, the thinnest thing in the model
echo(str("hand length (wrist → index tip) ", hand_len(1), " mm, heading ", hand_yaw(1), "°, tips reach y = ", tip_pt(1)[1], " mm"));
echo(str("thinnest feature (pinky tip) ", min_feature, " mm"));
if (min_feature < 0.8)
    echo("WARNING: thinnest feature is under 0.8 mm - hard to cast/print. Widen custom_gap_angle or scale up.");

use <hands.scad>

// ---------------------------------------------------------------------------
//  Band
// ---------------------------------------------------------------------------
module band_ball(i, rnd) {
    th = th_start + arc * i / band_segments;
    r  = band_r(th) + [rnd[i], 0];               // hammering jitters the outside only
    translate(band_c(th, r[0])) scale([r[0], r[0], r[1]])
        sphere(1, $fn = band_facets > 0 ? band_facets : $fn);
}

module band() {
    rnd = rands(-hammer_mm, hammer_mm, band_segments + 1, hammer_seed);
    for (i = [0 : band_segments - 1]) hull() {
        band_ball(i, rnd);
        band_ball(i + 1, rnd);
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
