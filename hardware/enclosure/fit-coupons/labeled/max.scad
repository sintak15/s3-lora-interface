union() {
 import("../max.stl", convexity=10);
 translate([0,-9,0]) cube([24.399999618530273,7,0.8]);
 translate([1,-2.1,0]) cube([3,5.1,0.6]);
 translate([12.199999809265137,-5.5,0.7]) linear_extrude(0.7)
 text("MAX",size=3,font="Liberation Sans",halign="center",valign="center");
}
