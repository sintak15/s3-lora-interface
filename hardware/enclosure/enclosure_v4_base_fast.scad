/* Lightweight base matching the measured CYD face. Print exterior down.
   Preserve all component seats, corner bosses, mating lip and port datums.
*/
coupon_mode = true;
cyd_post_height_override = 3.3;
include <enclosure_v4.scad>

base_floor_t = 1.6;
side_wall_t = 1.8;

module base_mount_footprints() {
    offset(delta=1) projection(cut=false) union() {
        rear_features();
        for(p=case_pts) cyl_at(p,0,case_boss_d,skin);
    }
}

module base_lightening_cuts() {
    // Floor remains solid externally, with original skin under all seats.
    translate([0,0,base_floor_t]) linear_extrude(skin-base_floor_t+eps)
        difference() {
            translate([wall+0.8,wall+0.8]) projection(cut=false)
                rounded_block(case_w-2*(wall+0.8),case_l-2*(wall+0.8),1,corner_r-wall-0.8);
            base_mount_footprints();
        }
    // Thin only straight inner wall sections. Retain full wall thickness at
    // the floor root, seam, corner pillars, top panel hardware and charger.
    difference() {
        union() {
            for(xx=[side_wall_t,case_w-wall-eps])
                translate([xx,corner_r+1,skin+1])
                    cube([wall-side_wall_t+eps,case_l-2*(corner_r+1),back_depth-4-(skin+1)]);
            translate([corner_r+1,side_wall_t,skin+1])
                cube([case_w-2*(corner_r+1),wall-side_wall_t+eps,back_depth-4-(skin+1)]);
        }
        for(p=case_pts) cyl_at(p,0,case_boss_d+2,back_depth);
        // Full-thickness rim around charging connector, across its full height.
        translate([0,power_usb_y-power_usb_w/2-3,0])
            cube([wall+1,power_usb_w+6,back_depth]);
    }
}

module fast_base() {
    difference() {
        back();
        rear_local() base_lightening_cuts();
    }
}

rear_frame() fast_base();
