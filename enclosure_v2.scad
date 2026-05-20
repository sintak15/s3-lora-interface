/* ==================================================================
   S3 LoRa Interface Enclosure - V2

   Goals:
   - Model the actual parts as internal envelopes.
   - Provide translucent component ghosts for fit checking.
   - Add trays/retainers for battery, TP4056, MT3608, MAX17048, GPS, and Heltec.
   - Tie the side USB-C relief to the TP4056 charger position.
   - Keep the same basic 94 x 100 mm internal case footprint.

   Notes:
   - All dimensions are millimeters.
   - Board sizes are practical starting points. Measure your exact modules
     with calipers and tune the values in the "Component Specs" section.
   ================================================================== */


/* [Render Options] */
part = "both"; // ["both", "front", "back", "spacer", "assembled", "components"]
show_component_ghosts = true;
show_component_labels = false;


/* [Global Fit / Print Specs] */
eps = 0.05;
wall = 1.6;
floor = 1.0;
fit_clearance = 0.4;
board_clearance = 0.6;
wire_clearance = 2.0;

round_fn = 28;
boss_fn = 22;
hole_fn = 20;


/* [Shell Specs] */
int_w = 94;
int_l = 100;
ext_w = int_w + wall * 2;
ext_l = int_l + wall * 2;
corner_r = 5;

front_depth = 7.8;
back_depth = 32.0;
lip_h = 1.6;
spacer_height = 2.0; // [0:0.5:20]
assembled_gap = 0.25;


/* [Fasteners] */
case_screw_d = 4.5;
case_pilot_d = 2.6;
case_head_d = 8.5;
case_head_recess_h = 2.0;
case_boss_d = 8.5;
case_boss_h = 4.5;

comp_pilot_d = 2.2;
s3_comp_pilot_d = 2.5;

front_case_boss_d = 11.5;
front_case_boss_h = 3.4;
front_case_boss_z = 0.8;
front_standoff_d = 7.2;
front_standoff_z = case_head_recess_h;
front_standoff_clearance = 0.2;
front_standoff_h = front_depth - front_standoff_z - front_standoff_clearance;


/* [Component Specs - Tune To Your Parts] */
cell_18650_d = 18.8;
cell_18650_l = 67.0;
cell_retainer_h = 5.0;
cell_clip_w = 4.0;

tp4056_w = 18.0;
tp4056_l = 28.0;
tp4056_h = 5.5;

mt3608_w = 17.0;
mt3608_l = 37.0;
mt3608_h = 8.0;

max17048_w = 15.0;
max17048_l = 15.0;
max17048_h = 4.0;

heltec_board_w = 25.5;
heltec_board_l = 50.2;
heltec_board_h = 8.0;
heltec_oled_w = 21.5;
heltec_oled_l = 37.0;

neo6m_w = 25.0;
neo6m_l = 35.0;
neo6m_h = 8.0;
gps_ant_w = 24.0;
gps_ant_l = 24.0;

s3_board_w = 50.0;
s3_board_l = 86.0;
s3_board_h = 8.0;
s3_glass_w = 50.4;
s3_glass_l = 69.6;
s3_glass_r = 1.0;


/* [Layout - Front-Mounted Boards] */
s3_x = 8.5;
s3_y = 2.5;
s3_boss_h = 3.0;
s3_screen_v_offset = 17.3;
s3_glass_x_offset = -0.2;
s3_glass_y_offset = 8.2;

heltec_x_inset = 6.0;
heltec_y = 45.0;
heltec_boss_h = 6.0;
heltec_boss_d = 5.0;
heltec_boss_x_offset = 7.0;
heltec_opening_x_offset = 0.0;
heltec_opening_y_offset = 0.0;

gps_x_inset = 6.0;
gps_y = 25.0;
gps_boss_h = 6.0;
gps_boss_d = 5.4;


/* [Layout - Internal Back-Shell Components] */
cell_x = 11.0;
cell_y = 70.0;
cell_z = 12.0;

tp4056_x = 8.0;
tp4056_y = 8.0;
tp4056_z = floor + 1.0;
tp4056_usb_wall = "left"; // ["bottom", "top", "left", "right"]

mt3608_x = 32.0;
mt3608_y = 8.0;
mt3608_z = floor + 1.0;

max17048_x = 56.0;
max17048_y = 8.0;
max17048_z = floor + 1.0;

wire_channel_h = 2.0;
wire_channel_w = 6.0;


/* [Side Cutouts] */
charger_usb_enabled = true;
charger_usb_w = 14.0;
charger_usb_h = 7.0;
charger_usb_r = 2.0;
charger_usb_wall_depth = 5.6;
charger_usb_z_offset = 2.6;

switch_enabled = true;
switch_side = "top"; // ["bottom", "top", "left", "right"]
switch_side_pos = 50.0;
switch_z = 11.0;
switch_cutout_w = 13.7;
switch_cutout_h = 9.1;
switch_cutout_r = 0.5;
switch_wall_cut_depth = 6.0;

lora_ant_enabled = true;
lora_ant_d = 6.5;
lora_ant_x_offset = 0.0;
lora_ant_z = 17.0;


/* [Derived Coordinates] */
case_points = [
    [5, 5],
    [int_w - 5, 5],
    [5, int_l - 5],
    [int_w - 5, int_l - 5]
];

p_x = wall + s3_x;
p_y = wall + s3_y;
s3_points = [
    [p_x + 4.0, p_y + 4.0],
    [p_x + 46.0, p_y + 4.0],
    [p_x + 4.0, p_y + 82.0],
    [p_x + 46.0, p_y + 82.0]
];
s3_glass_x = p_x + s3_glass_x_offset;
s3_glass_y = p_y + s3_glass_y_offset;
s3_usb_x = p_x + 25.0;

h_x = wall + int_w - heltec_board_w - heltec_x_inset;
h_y = heltec_y;
heltec_screen_x = h_x + (heltec_board_w - heltec_oled_w) / 2 + heltec_opening_x_offset;
heltec_screen_y = h_y + (heltec_board_l - heltec_oled_l) + heltec_opening_y_offset;
heltec_outer_boss = [
    h_x + heltec_board_w / 2 + heltec_boss_x_offset,
    ext_l - wall - 2.8
];

g_x = wall + int_w - neo6m_w - gps_x_inset + neo6m_w / 2;
g_y = gps_y;
gps_outer_boss = [
    g_x,
    wall + 2.8
];


// ==================================================================
// BASIC HELPERS
// ==================================================================

module rounded_box(w, l, h, r) {
    rr = max(0.1, min(r, min(w, l) / 2 - 0.01));

    hull() {
        translate([rr, rr, 0])
            cylinder(r = rr, h = h, $fn = round_fn);
        translate([w - rr, rr, 0])
            cylinder(r = rr, h = h, $fn = round_fn);
        translate([rr, l - rr, 0])
            cylinder(r = rr, h = h, $fn = round_fn);
        translate([w - rr, l - rr, 0])
            cylinder(r = rr, h = h, $fn = round_fn);
    }
}


module rounded_rect_cut(x, y, z, w, l, h, r) {
    translate([x, y, z])
        rounded_box(w, l, h, r);
}


module label_2d(text_value, x, y, z) {
    if (show_component_labels) {
        color([0, 0, 0, 1])
            translate([x, y, z])
                linear_extrude(0.2)
                    text(text_value, size = 3.0, halign = "center", valign = "center");
    }
}


module internal_boss(d, h, pilot_d, pilot_depth) {
    difference() {
        cylinder(d = d, h = h, $fn = boss_fn);
        translate([0, 0, h - pilot_depth + eps])
            cylinder(d = pilot_d, h = pilot_depth + eps, $fn = hole_fn);
    }
}


module screw_boss_through(d, h, hole_d) {
    difference() {
        cylinder(d = d, h = h, $fn = boss_fn);
        translate([0, 0, -eps])
            cylinder(d = hole_d, h = h + eps * 2, $fn = hole_fn);
    }
}


module board_tray(x, y, z, w, l, rail_h = 2.0, rail_w = 1.4) {
    translate([x - rail_w, y - rail_w, z])
        cube([w + rail_w * 2, rail_w, rail_h]);
    translate([x - rail_w, y + l, z])
        cube([w + rail_w * 2, rail_w, rail_h]);
    translate([x - rail_w, y, z])
        cube([rail_w, l, rail_h]);
    translate([x + w, y, z])
        cube([rail_w, l, rail_h]);
}


module board_snap_tab(x, y, z, w = 5.0, l = 1.2, h = 3.0) {
    translate([x - w / 2, y, z])
        cube([w, l, h]);
}


module ghost_box(x, y, z, w, l, h, c = [0.1, 0.5, 1.0, 0.28]) {
    if (show_component_ghosts) {
        color(c)
            translate([x, y, z])
                cube([w, l, h]);
    }
}


module ghost_cylinder_x(x, y, z, d, l, c = [0.2, 1.0, 0.25, 0.28]) {
    if (show_component_ghosts) {
        color(c)
            translate([x, y, z])
                rotate([0, 90, 0])
                    cylinder(d = d, h = l, $fn = 48);
    }
}


// ==================================================================
// COMPONENT GHOSTS
// ==================================================================

module component_ghosts() {
    ghost_box(
        p_x,
        p_y,
        back_depth + spacer_height + assembled_gap * 2,
        s3_board_w,
        s3_board_l,
        s3_board_h,
        [0.2, 0.4, 1.0, 0.20]
    );
    label_2d("S3 Display", p_x + s3_board_w / 2, p_y + s3_board_l / 2, back_depth + spacer_height + 9);

    ghost_box(
        h_x,
        h_y,
        back_depth + spacer_height + assembled_gap * 2,
        heltec_board_w,
        heltec_board_l,
        heltec_board_h,
        [1.0, 0.4, 0.1, 0.24]
    );
    label_2d("Heltec V3.2", h_x + heltec_board_w / 2, h_y + heltec_board_l / 2, back_depth + spacer_height + 9);

    ghost_box(
        g_x - neo6m_w / 2,
        g_y - neo6m_l / 2,
        back_depth + spacer_height + assembled_gap * 2,
        neo6m_w,
        neo6m_l,
        neo6m_h,
        [0.2, 0.9, 0.2, 0.22]
    );
    label_2d("GY-NEO6MV2", g_x, g_y, back_depth + spacer_height + 9);

    ghost_cylinder_x(cell_x, cell_y, cell_z, cell_18650_d, cell_18650_l);
    label_2d("18650", cell_x + cell_18650_l / 2, cell_y, cell_z + cell_18650_d / 2 + 2);

    ghost_box(tp4056_x, tp4056_y, tp4056_z, tp4056_w, tp4056_l, tp4056_h, [1.0, 0.15, 0.15, 0.28]);
    label_2d("TP4056", tp4056_x + tp4056_w / 2, tp4056_y + tp4056_l / 2, tp4056_z + tp4056_h + 1);

    ghost_box(mt3608_x, mt3608_y, mt3608_z, mt3608_w, mt3608_l, mt3608_h, [0.9, 0.7, 0.1, 0.28]);
    label_2d("MT3608", mt3608_x + mt3608_w / 2, mt3608_y + mt3608_l / 2, mt3608_z + mt3608_h + 1);

    ghost_box(max17048_x, max17048_y, max17048_z, max17048_w, max17048_l, max17048_h, [0.7, 0.2, 1.0, 0.28]);
    label_2d("MAX17048", max17048_x + max17048_w / 2, max17048_y + max17048_l / 2, max17048_z + max17048_h + 1);
}


// ==================================================================
// CUTOUTS
// ==================================================================

module face_usb_c_cutout() {
    r = min(2.0, 12.0 / 2 - eps, 7.0 / 2 - eps);

    translate([s3_usb_x, -1, front_depth])
        rotate([-90, 0, 0])
            hull() {
                translate([-12.0 / 2 + r, 0, 0])
                    cylinder(r = r, h = wall + 5, $fn = hole_fn);
                translate([12.0 / 2 - r, 0, 0])
                    cylinder(r = r, h = wall + 5, $fn = hole_fn);
                translate([-12.0 / 2 + r, -7.0 + r, 0])
                    cylinder(r = r, h = wall + 5, $fn = hole_fn);
                translate([12.0 / 2 - r, -7.0 + r, 0])
                    cylinder(r = r, h = wall + 5, $fn = hole_fn);
            }
}


module side_rounded_cutout(w, h, depth, r) {
    rr = min(r, w / 2 - eps, h / 2 - eps);

    hull() {
        translate([-w / 2 + rr, 0, -h / 2 + rr])
            rotate([90, 0, 0])
                cylinder(r = rr, h = depth, center = true, $fn = hole_fn);
        translate([w / 2 - rr, 0, -h / 2 + rr])
            rotate([90, 0, 0])
                cylinder(r = rr, h = depth, center = true, $fn = hole_fn);
        translate([-w / 2 + rr, 0, h / 2 - rr])
            rotate([90, 0, 0])
                cylinder(r = rr, h = depth, center = true, $fn = hole_fn);
        translate([w / 2 - rr, 0, h / 2 - rr])
            rotate([90, 0, 0])
                cylinder(r = rr, h = depth, center = true, $fn = hole_fn);
    }
}


module wall_cutout_at(side, pos, z, w, h, depth, r) {
    if (side == "bottom") {
        translate([wall + pos, -1, z])
            side_rounded_cutout(w, h, depth, r);
    } else if (side == "top") {
        translate([wall + pos, ext_l + 1, z])
            side_rounded_cutout(w, h, depth, r);
    } else if (side == "left") {
        translate([-1, wall + pos, z])
            rotate([0, 0, 90])
                side_rounded_cutout(w, h, depth, r);
    } else if (side == "right") {
        translate([ext_w + 1, wall + pos, z])
            rotate([0, 0, 90])
                side_rounded_cutout(w, h, depth, r);
    }
}


module charger_usb_cutout() {
    if (charger_usb_enabled) {
        pos =
            tp4056_usb_wall == "bottom" || tp4056_usb_wall == "top"
            ? tp4056_x + tp4056_w / 2 - wall
            : tp4056_y + tp4056_l / 2 - wall;

        z = tp4056_z + charger_usb_h / 2 + charger_usb_z_offset;

        wall_cutout_at(
            tp4056_usb_wall,
            pos,
            z,
            charger_usb_w,
            charger_usb_h,
            charger_usb_wall_depth,
            charger_usb_r
        );
    }
}


module power_switch_cutout() {
    if (switch_enabled) {
        wall_cutout_at(
            switch_side,
            switch_side_pos,
            switch_z,
            switch_cutout_w,
            switch_cutout_h,
            switch_wall_cut_depth,
            switch_cutout_r
        );
    }
}


module lora_antenna_cutout() {
    if (lora_ant_enabled) {
        translate([h_x + heltec_board_w / 2 + lora_ant_x_offset, ext_l - wall - 1, lora_ant_z])
            rotate([-90, 0, 0])
                cylinder(d = lora_ant_d, h = wall + 5, $fn = hole_fn);
    }
}


// ==================================================================
// INTERNAL HOLDERS
// ==================================================================

module battery_cradle() {
    y0 = cell_y - cell_18650_d / 2 - 1.6;
    z0 = floor;

    // Two saddle rails under the 18650.
    for (x0 = [cell_x + 8, cell_x + cell_18650_l - 18]) {
        translate([x0, y0, z0])
            cube([10, 3.0, cell_retainer_h]);
        translate([x0, y0 + cell_18650_d + 0.2, z0])
            cube([10, 3.0, cell_retainer_h]);
    }

    // End stops leave wiring room at both ends.
    translate([cell_x - 1.2, cell_y - cell_18650_d / 2, z0])
        cube([1.2, cell_18650_d, cell_retainer_h]);
    translate([cell_x + cell_18650_l, cell_y - cell_18650_d / 2, z0])
        cube([1.2, cell_18650_d, cell_retainer_h]);
}


module electronics_trays() {
    board_tray(
        tp4056_x - board_clearance / 2,
        tp4056_y - board_clearance / 2,
        floor,
        tp4056_w + board_clearance,
        tp4056_l + board_clearance
    );
    board_snap_tab(tp4056_x + tp4056_w / 2, tp4056_y + tp4056_l + 1.2, floor, 8.0);

    board_tray(
        mt3608_x - board_clearance / 2,
        mt3608_y - board_clearance / 2,
        floor,
        mt3608_w + board_clearance,
        mt3608_l + board_clearance
    );
    board_snap_tab(mt3608_x + mt3608_w / 2, mt3608_y + mt3608_l + 1.2, floor, 8.0);

    board_tray(
        max17048_x - board_clearance / 2,
        max17048_y - board_clearance / 2,
        floor,
        max17048_w + board_clearance,
        max17048_l + board_clearance
    );

    // Low wire channel from power boards toward the battery.
    translate([tp4056_x + tp4056_w, tp4056_y + tp4056_l / 2 - wire_channel_w / 2, floor])
        cube([cell_x + 10 - (tp4056_x + tp4056_w), wire_channel_w, wire_channel_h]);
}


module back_internal_features() {
    battery_cradle();
    electronics_trays();
}


// ==================================================================
// FRONT SHELL
// ==================================================================

module front_case_boss() {
    screw_boss_through(front_case_boss_d, front_case_boss_h, case_screw_d + 0.8);
}


module front_case_standoff() {
    screw_boss_through(front_standoff_d, front_standoff_h, case_screw_d);
}


module front_shell() {
    difference() {
        union() {
            difference() {
                rounded_box(ext_w, ext_l, front_depth, corner_r);

                translate([wall, wall, floor])
                    rounded_box(int_w, int_l, front_depth + eps, corner_r - 1);

                translate([wall / 2, wall / 2, front_depth - lip_h])
                    rounded_box(int_w + wall, int_l + wall, lip_h + 1, corner_r - 0.5);
            }

            for (p = case_points) {
                translate([wall + p[0], wall + p[1], front_case_boss_z])
                    front_case_boss();

                translate([wall + p[0], wall + p[1], front_standoff_z])
                    front_case_standoff();
            }

            for (p = s3_points) {
                translate([p[0], p[1], floor])
                    internal_boss(4.8, s3_boss_h, s3_comp_pilot_d, 4.0);
            }

            translate([heltec_outer_boss[0], heltec_outer_boss[1], floor])
                internal_boss(heltec_boss_d, heltec_boss_h, comp_pilot_d, 5.5);

            translate([gps_outer_boss[0], gps_outer_boss[1], floor])
                internal_boss(gps_boss_d, gps_boss_h, comp_pilot_d, 5.2);

            // GPS antenna ledge.
            translate([g_x, g_y, floor + 0.85])
                difference() {
                    cube([gps_ant_w + 3.5, gps_ant_l + 3.5, 1.7], center = true);
                    cube([gps_ant_w + 1.6, gps_ant_l + 1.6, 2.0], center = true);
                }

            // Small Heltec side rails.
            for (ix = [-1.4, heltec_board_w + 0.4]) {
                for (iy = [10, 35.2]) {
                    translate([h_x + ix, h_y + iy, floor])
                        cube([1.0, 4.0, 2.6]);
                }
            }
        }

        // CYD glass opening.
        rounded_rect_cut(
            s3_glass_x,
            s3_glass_y,
            -1,
            s3_glass_w,
            s3_glass_l,
            front_depth + 4,
            s3_glass_r
        );

        // Heltec OLED opening.
        translate([heltec_screen_x, heltec_screen_y, -1])
            cube([heltec_oled_w, heltec_oled_l, front_depth + 4]);

        // GPS antenna opening.
        rounded_rect_cut(
            g_x - gps_ant_w / 2,
            g_y - gps_ant_l / 2,
            -1,
            gps_ant_w,
            gps_ant_l,
            front_depth + 4,
            0.8
        );

        face_usb_c_cutout();

        for (p = case_points) {
            translate([wall + p[0], wall + p[1], -1])
                cylinder(d = case_screw_d, h = front_depth + 5, $fn = hole_fn);

            translate([wall + p[0], wall + p[1], -0.1])
                cylinder(d = case_head_d, h = case_head_recess_h, $fn = boss_fn);
        }
    }
}


// ==================================================================
// BACK SHELL
// ==================================================================

module back_case_boss() {
    boss_h = back_depth - 0.1 - floor;

    difference() {
        cylinder(d = case_boss_d, h = boss_h, $fn = boss_fn);

        translate([0, 0, 1.2])
            cylinder(d = case_pilot_d, h = boss_h + 2, $fn = hole_fn);

        translate([0, 0, boss_h * 0.45])
            cylinder(d = case_pilot_d + 1.3, h = boss_h, $fn = hole_fn);
    }
}


module back_shell() {
    difference() {
        union() {
            difference() {
                rounded_box(ext_w, ext_l, back_depth, corner_r);

                translate([wall, wall, floor])
                    rounded_box(int_w, int_l, back_depth + eps, corner_r - 1);
            }

            // Male interlock lip.
            translate([wall / 2 + 0.3, wall / 2 + 0.3, back_depth])
                difference() {
                    rounded_box(int_w + wall - 0.6, int_l + wall - 0.6, lip_h, corner_r - 0.5);

                    translate([wall / 2 - 0.3, wall / 2 - 0.3, -1])
                        rounded_box(int_w, int_l, lip_h + 2, corner_r - 1);
                }

            for (p = case_points) {
                translate([wall + p[0], wall + p[1], floor])
                    back_case_boss();
            }

            back_internal_features();
        }

        for (p = case_points) {
            translate([wall + p[0], wall + p[1], back_depth - 13.5])
                cylinder(d = case_pilot_d, h = 16, $fn = hole_fn);
        }

        charger_usb_cutout();
        power_switch_cutout();
        lora_antenna_cutout();
    }
}


// ==================================================================
// SPACER
// ==================================================================

module extension_spacer() {
    if (spacer_height > 0) {
        difference() {
            union() {
                difference() {
                    rounded_box(ext_w, ext_l, spacer_height, corner_r);

                    translate([wall, wall, -1])
                        rounded_box(int_w, int_l, spacer_height + 2, corner_r - 1);

                    translate([wall / 2, wall / 2, -1])
                        rounded_box(int_w + wall, int_l + wall, lip_h + 1, corner_r - 0.5);
                }

                for (p = case_points) {
                    hull() {
                        translate([wall + p[0], wall + p[1], 0])
                            cylinder(d = case_boss_d, h = spacer_height, $fn = boss_fn);

                        cx = (p[0] == 5) ? wall + corner_r - 1 : ext_w - wall - corner_r + 1;
                        cy = (p[1] == 5) ? wall + corner_r - 1 : ext_l - wall - corner_r + 1;
                        translate([cx, cy, 0])
                            cylinder(r = corner_r - 1, h = spacer_height, $fn = boss_fn);
                    }
                }

                translate([wall / 2 + 0.3, wall / 2 + 0.3, spacer_height])
                    difference() {
                        rounded_box(int_w + wall - 0.6, int_l + wall - 0.6, lip_h, corner_r - 0.5);

                        translate([wall / 2 - 0.3, wall / 2 - 0.3, -1])
                            rounded_box(int_w, int_l, lip_h + 2, corner_r - 1);
                    }
            }

            for (p = case_points) {
                translate([wall + p[0], wall + p[1], -1])
                    cylinder(d = case_screw_d, h = spacer_height + lip_h + 2, $fn = hole_fn);
            }
        }
    }
}


// ==================================================================
// ASSEMBLY
// ==================================================================

module assembled_view() {
    back_shell();

    translate([0, 0, back_depth + assembled_gap])
        extension_spacer();

    translate([ext_w, 0, back_depth + spacer_height + front_depth + assembled_gap * 2])
        rotate([180, 0, 180])
            front_shell();

    component_ghosts();
}


// ==================================================================
// RENDER
// ==================================================================

if (part == "front") {
    front_shell();

} else if (part == "back") {
    back_shell();

} else if (part == "spacer") {
    extension_spacer();

} else if (part == "assembled") {
    assembled_view();

} else if (part == "components") {
    component_ghosts();

} else {
    front_shell();

    translate([ext_w + 14, 0, 0])
        back_shell();

    translate([-ext_w - 14, 0, 0])
        extension_spacer();

    translate([0, -34, 0])
        component_ghosts();
}
