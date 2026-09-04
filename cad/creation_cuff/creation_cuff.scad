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

use <hands.scad>

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
    place_hand(+1) human_hand(hand_len(+1), swell_t / 2, finger_thickness, finger_curl);
    place_hand(-1) robot_hand(hand_len(-1), swell_t / 2, finger_thickness, finger_curl);
}

if (part == "assembly")        assembly();
else if (part == "band")       band();
else if (part == "human_hand") human_hand(hand_len(+1), swell_t / 2, finger_thickness, finger_curl);
else if (part == "robot_hand") robot_hand(hand_len(-1), swell_t / 2, finger_thickness, finger_curl);
