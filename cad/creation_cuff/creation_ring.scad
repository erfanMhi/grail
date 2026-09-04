// =============================================================================
//  Creation Ring — rounded-square ring whose band is two arms
//
//  The two forearms *are* the band: flat and engraved on the inside, organic
//  on the outside.  They cross at the back-left corner.  A robotic left hand
//  runs along the bottom side and a human right hand runs up the right side;
//  their index fingers meet at the front-right corner.  Fingers droop below
//  the band the way Adam's do.
//
//  Units: millimetres.  The ring lies in the XY plane; Z is the ring axis.
//  The opening corner is at 315° (front-right, x > 0, y < 0).
//
//  Needs hands.scad next to this file.  Render with F6, export STL, or `make`.
// =============================================================================
use <hands.scad>

/* [Part] */
part = "assembly"; // ["assembly", "band", "human_hand", "robot_hand"]

/* [Sizing] */
// Inside size across the flats (US 7 ≈ 17.3, see README for the chart)
inner_d = 17.3;
// Axial width of the band (how tall the ring is on the finger)
band_w = 4.8;
// Radial thickness of the band at the wrists (thinnest point)
band_t = 1.5;
// Radial thickness at the belly of each forearm
arm_bulge = 2.6;
// Corner shape: 2 = circle, 4 = squircle, 6 = nearly square
squareness = 4.5;
// Space left between the two index fingertips
tip_gap = 0.9;
// Multiplier on finger diameters (thin bits must cast/print)
finger_thickness = 1.2;

/* [Pose] */
// Back of each hand tilted outward from the ring axis (deg)
hand_roll = 28;
// How far past the wrist corners the hands start, in degrees of path (bigger = shorter hands)
wrist_shift = 20;
// Droop of the hands below the band plane (deg)
hand_pitch = 0;
// Aim of each hand inward from its side, so the index tips converge at the corner (deg)
hand_yaw_in = 8;
// Multiplier on how tightly the three tucked fingers curl
finger_curl = 1.0;

/* [Detail] */
// Engraved on the inside of the left side (empty string for none)
inner_text = "925";
text_depth = 0.15;
// Ridge of tendon along the back of each forearm
tendons = true;
// Z offset of the two arms where they cross at the back corner
cross_offset = 0.9;

/* [Quality] */
$fn = 32;
band_segments = 96;

// ---------------------------------------------------------------------------
//  Superellipse path  |x/a|^n + |y/a|^n = 1, by polar angle
// ---------------------------------------------------------------------------
n_se = squareness;
a_in = inner_d / 2;
function se_r(a, phi) = a / pow(pow(abs(cos(phi)), n_se) + pow(abs(sin(phi)), n_se), 1 / n_se);
function se_pt(a, phi) = let(r = se_r(a, phi)) [r * cos(phi), r * sin(phi), 0];
function se_poly(a, k = 180) = [for (i = [0 : k - 1]) let(p = se_pt(a, 360 * i / k)) [p[0], p[1]]];
function smooth(u) = let(v = min(1, max(0, u))) v * v * (3 - 2 * v);

// ---------------------------------------------------------------------------
//  Arms.  Each arm runs from the crossing corner (135°) to a wrist corner.
//  u = 0 at the crossing, 1 at the wrist.
// ---------------------------------------------------------------------------
cross_phi = 135;
ext_w = wrist_shift + 8;   // degrees the arm runs past the wrist corner into the hand
ext_c = 10;        // degrees the arm runs past the crossing corner

function arm_t(u) = band_t + (arm_bulge - band_t) * pow(sin(180 * min(1, max(0, u))), 1.4);
function arm_w(u) = band_w * (1 - 0.18 * smooth((u - 0.7) / 0.35));

// One ellipsoid of the sweep: arm from phi0 (crossing) to phi1 (wrist), z drift zs
module arm_ball(phi0, phi1, zs, i, k) {
    u   = i / k;
    phi = phi0 + (phi1 - phi0) * u;
    t   = arm_t(u);
    w   = arm_w(u);
    z   = zs * cross_offset * smooth((0.35 - u) / 0.35);
    translate(se_pt(a_in + t / 2, phi) + [0, 0, z]) scale([t / 2, t / 2, w / 2]) sphere(1);
}
module arm(phi0, phi1, zs) {
    k = band_segments / 2;
    for (i = [0 : k - 1]) hull() { arm_ball(phi0, phi1, zs, i, k); arm_ball(phi0, phi1, zs, i + 1, k); }
    if (tendons) {
        // a ridge along the outer face, fading out at both ends
        for (i = [3 : k - 5]) hull() for (j = [i, i + 1]) {
            u = j / k; phi = phi0 + (phi1 - phi0) * u; t = arm_t(u);
            r = 0.30 * t * sin(180 * u);
            z = zs * cross_offset * smooth((0.35 - u) / 0.35) + band_w * 0.18 * zs;
            translate(se_pt(a_in + t - r * 0.7, phi) + [0, 0, z]) sphere(max(r, 0.05));
        }
    }
}

module bore() { linear_extrude(height = band_w * 3, center = true) polygon(se_poly(a_in)); }

module engraving() {
    if (len(inner_text) > 0)
        translate([-a_in - text_depth, 0, 0]) rotate([90, 0, 90])
            linear_extrude(height = text_depth + 0.05)
                text(inner_text, size = band_w * 0.42, font = "Liberation Sans:style=Bold",
                     halign = "center", valign = "center");
}

module band() {
    difference() {
        union() {
            // robot's arm: left side, crossing corner → bottom-left wrist corner
            arm(cross_phi - ext_c, 225 + ext_w, +1);
            // human's arm: top side, crossing corner → top-right wrist corner
            arm(cross_phi + ext_c, 45 - ext_w, -1);
        }
        bore();
        engraving();
    }
}

// ---------------------------------------------------------------------------
//  Hands
// ---------------------------------------------------------------------------
a_c      = a_in + band_t / 2;                  // centreline at the wrists
corner   = se_pt(a_c, 315);                    // where the index fingers meet
wrist_l  = se_pt(a_c, 225 + wrist_shift);      // robot (left) hand, bottom side, points +X
wrist_r  = se_pt(a_c, 45 - wrist_shift);       // human (right) hand, right side, points -Y
hand_L   = norm(corner - wrist_l) - tip_gap;
wrist_rad = [band_t * 0.55, band_w * 0.42];        // [side, axial]: the arm flows straight into each wrist

// approximate tip positions for the console readout
tip_l = wrist_l + hand_L * [cos(hand_yaw_in), sin(hand_yaw_in), 0];
tip_r = wrist_r + hand_L * [cos(-90 - hand_yaw_in), sin(-90 - hand_yaw_in), 0];
min_feature = 2 * 0.032 * finger_thickness * hand_L;
echo(str("inner across flats ", inner_d, " mm, band ", band_t, "→", arm_bulge, " mm × ", band_w, " mm wide"));
echo(str("hand length (wrist → index tip) ", hand_L, " mm, tips ≈ ", norm(tip_l - tip_r), " mm apart"));
echo(str("thinnest feature (pinky tip) ", min_feature, " mm"));
if (min_feature < 0.8) echo("WARNING: thinnest feature under 0.8 mm - raise finger_thickness or the ring size.");

module place_robot() {
    translate(wrist_l) rotate([0, 0, hand_yaw_in]) rotate([hand_roll, hand_pitch, 0])
        mirror([0, 1, 0]) children();
}
module place_human() {
    translate(wrist_r) rotate([0, 0, -90 - hand_yaw_in]) rotate([-hand_roll, hand_pitch, 0])
        children();
}

module assembly() {
    band();
    place_robot() robot_hand(hand_L, wrist_rad, finger_thickness, finger_curl);
    place_human() human_hand(hand_L, wrist_rad, finger_thickness, finger_curl);
}

if (part == "assembly")        assembly();
else if (part == "band")       band();
else if (part == "human_hand") human_hand(hand_L, wrist_rad, finger_thickness, finger_curl);
else if (part == "robot_hand") robot_hand(hand_L, wrist_rad, finger_thickness, finger_curl);
