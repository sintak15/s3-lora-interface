/* Small test section of the exported latest face. Print this before the
   whole face to confirm the antenna and receiver fit on the actual hardware.
   Regenerate the full STL first; the coupon never scales or moves features.
*/
translate([-57,-84,0]) intersection() {
    import("enclosure-v4-front-gps-repair.stl",convexity=10);
    translate([57,84,-0.01]) cube([47,40,50]);
}
