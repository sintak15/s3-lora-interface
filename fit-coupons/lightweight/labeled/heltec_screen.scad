union() {
 import("../heltec_screen.stl", convexity=10);
 translate([0,-9,0]) cube([40.20000076293945,7,0.6]);
 translate([5,-2.1,0]) cube([3,5.1,0.6]);
 translate([20.100000381469727,-5.5,0.5]) linear_extrude(0.5)
 text("OLED FIT",size=3,font="Liberation Sans",halign="center",valign="center");
}
