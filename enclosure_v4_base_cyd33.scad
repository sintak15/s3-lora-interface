/* Matching rear base for enclosure_v4_front_cyd33.scad.
   Exterior rear face on bed, component seats upward. Millimetres.
   Existing V4 battery, power, MAX and speaker seats and mating lip retained.
*/
coupon_mode = true;
cyd_post_height_override = 3.3;
include <enclosure_v4.scad>

rear_frame() back();
