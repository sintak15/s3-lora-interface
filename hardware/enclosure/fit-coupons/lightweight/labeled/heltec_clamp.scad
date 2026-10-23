union() {
 import("../heltec_clamp.stl", convexity=10);
 translate([0,-9,0]) cube([41.0,7,0.6]);
 translate([1,-2.1,0]) cube([3,5.1,0.6]);
 translate([20.5,-5.5,0.5]) linear_extrude(0.5)
 text("CLAMP",size=3,font="Liberation Sans",halign="center",valign="center");
}
