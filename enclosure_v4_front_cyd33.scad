/* Measured CYD revision: posts are 3.3 mm above the 2.4 mm inside floor.
   PCB seat Z = 5.7 mm; USB centre moves down 2.9 mm with the PCB.
   Heltec heights and XY positions remain those of the lightweight V4 front.
   Use M3 x 4 mm CYD screws with the nominal 1.6 mm PCB, not the old 6 mm.
*/
cyd_post_height_override = 3.3;
include <enclosure_v4_front_fast.scad>
