union() {
 import("../heltec.stl", convexity=10);
 translate([0,-9,0]) cube([40.20000076293945,7,0.8]);
 translate([1,-2.1,0]) cube([3,5.1,0.6]);
 translate([20.100000381469727,-5.5,0.7]) linear_extrude(0.7)
 text("HELTEC",size=3,font="Liberation Sans",halign="center",valign="center");
}
