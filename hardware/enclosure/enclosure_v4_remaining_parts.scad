/* Remaining printed parts for the lowered-CYD face and open-battery base.
   All parts are already in bed orientation. No battery inserts are required.
*/
coupon_mode = true;
cyd_post_height_override = 3.3;
include <enclosure_v4.scad>

retainer_part = "plate"; // [plate,heltec_clamps,gps_rails,speaker_frame]

if(retainer_part=="heltec_clamps") translate([0,3.4,0]) clamps_print();
else if(retainer_part=="gps_rails") translate([0.7,0,0]) antenna_retainers_print();
else if(retainer_part=="speaker_frame") speaker_retainer_print();
else if(retainer_part=="plate") {
    translate([8,11.4,0]) clamps_print();
    translate([72.7,8,0]) antenna_retainers_print();
    translate([8,34,0]) speaker_retainer_print();
} else assert(false,"Unknown retainer_part");
