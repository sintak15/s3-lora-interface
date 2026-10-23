/* Rails matching the reinforced face's additional 0.5 mm/side XY clearance. */
coupon_mode = true;
patch_extra_clearance = 0.5;
include <enclosure_v4.scad>
translate([0.7,0,0]) antenna_retainers_print();
