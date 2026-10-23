/* V4 lightweight front, face down on bed. Millimetres.
   Original outside, mating rim, screen openings and PCB datums are retained.
   GPS receiver uses TWO narrow straps instead of overhanging keeper tabs.
   The tiny tie-anchor tunnels require short bridges, not slicer supports.
*/
coupon_mode = true;
include <enclosure_v4.scad>

floor_t = 1.6;
assert(floor_t >= 1.6 && floor_t < skin);

module extra_gps_anchors() {
    if(is_undef(gps_screw_mounts) || !gps_screw_mounts) gps_frame() {
        tie_anchor(-4.8,gps_size[1]-7);
        tie_anchor(gps_size[0]+1.6,gps_size[1]-7);
    }
}

module protected_footprints() {
    // Keep full original skin under every mount, anchor and screw pillar,
    // with a 1 mm border. All raised features keep their original heights.
    offset(delta=1) projection(cut=false) union() {
        front_mounts();
        extra_gps_anchors();
        for(p=case_pts) cyl_at(p,0,case_boss_d,skin);
    }
}

module fast_front() {
    difference() {
        union() {
            front();
            extra_gps_anchors();
        }
        // Interior pockets only: the exterior face remains flat on the bed.
        translate([0,0,floor_t]) linear_extrude(skin-floor_t+eps)
            difference() {
                // Keep a solid border at the wall root; also prevents tiny
                // underside ledges from different curve tessellations.
                translate([wall+0.8,wall+0.8]) projection(cut=false)
                    rounded_block(case_w-2*(wall+0.8),case_l-2*(wall+0.8),1,corner_r-wall-0.8);
                protected_footprints();
            }
        // Remove just the two inward keeper ledges; vertical guides remain.
        gps_frame() for(x=[-1.6,gps_size[0]-0.8])
            translate([x-eps,gps_size[1]-3.5-eps,gps_board_z+gps_pcb_t+0.3-eps])
                cube([2.4+2*eps,3+2*eps,0.7+2*eps]);
        // Keep the CYD USB notch open to the seam even after lowering its
        // centre, avoiding an 18 mm bridge above the connector opening.
        translate([cyd_x+cyd_usb_x-usb_open_w/2,-eps,cyd_usb_center_z-usb_open_h/2])
            cube([usb_open_w,wall+2*eps,front_depth+usb_open_h]);
    }
}

fast_front();
