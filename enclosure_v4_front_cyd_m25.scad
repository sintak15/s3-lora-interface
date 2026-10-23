/* Current face: solid GPS screw rims and tighter CYD pilots for M2.5 screws.
   2.0 mm pilot diameter; CYD post height stays 3.3 mm above inside floor.
*/
cyd_pilot_override = is_undef(cyd_tight_pilot_d) ? 2.0 : cyd_tight_pilot_d;
include <enclosure_v4_front_gps_solid_rims.scad>
