/* One actual lowered CYD post and retained full-thickness floor, for
   checking the actual M2.5 screw against this printer's hole tolerance. */
coupon_mode = true;
cyd_post_height_override = 3.3;
cyd_pilot_override = is_undef(cyd_tight_pilot_d) ? 2.0 : cyd_tight_pilot_d;
include <enclosure_v4.scad>
difference() {
    union() {
        rounded_block(12,12,skin,1.5);
        cyl_at([6,6],skin-eps,6,cyd_pcb_z-skin+eps);
    }
    cyl_at([6,6],skin+0.5,cyd_mount_pilot_d,cyd_pcb_z);
}
