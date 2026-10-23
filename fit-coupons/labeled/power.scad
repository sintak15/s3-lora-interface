union() {
 import("../power.stl", convexity=10);
 translate([0,-9,0]) cube([34.0,7,0.8]);
 translate([1,-2.1,0]) cube([3,5.1,0.6]);
 translate([17.0,-5.5,0.7]) linear_extrude(0.7)
 text("POWER",size=3,font="Liberation Sans",halign="center",valign="center");
}
