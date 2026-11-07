/* Sensor revision prototype. New board dimensions are provisional: measure first.
   Direct mounts in new shells; independent retrofit plates for existing V4 prints.
   mm. Retains V4 exterior/screen/power/battery/speaker datums. */
revision_part = "exploded"; // front, back, gps_retainer, am2302_holder, keyboard_tray, retrofit_gps, retrofit_ads, fit_coupon, assembled, exploded
sensor_revision = true;
coupon_mode = true;
open_battery_bay = true;
power_usb_at_bottom = true;
face_entry_case_screws = true;
heavy_tie_anchors = true;
heavy_heltec_posts = true;
heltec_post_drop = 5.2;
cyd_post_height_override = 3.3;
cyd_pilot_override = 1.8;

/* New hardware: nominal envelopes, NOT user measurements. */
receiver_size = [18,18,8];
receiver_xy = [59.5,94];
receiver_z = 12;
antenna_size = [18,18,6];
antenna_xy = [80,94];
antenna_face = 5.4; // 3mm air over ceramic behind 2.4mm plastic
antenna_carrier_t = 1.6;
ads_size = [18,28,10]; // long dimension follows Y
ads_xy = [71,92];
light_size = [14,14,7];
light_xy = [33,100];
light_z = 8;
light_aperture_d = 4;
keyboard_port_w = 12;
keyboard_port_h = 8;
keyboard_port_y = 100;
keyboard_port_z = 30;
keyboard_size = [88,54,5]; // M5Stack published outside dimensions
fit_gap = 0.5;
am_body = [14,40,23]; // X protrusion, Y length, Z width: VERIFY wired case
am_center_y = 51;
am_center_z = 35;
am_mount_ys = [25,77];
am_cable_d = 5;

function sensor_front_boxes() = [
 [receiver_xy[0],receiver_xy[1],receiver_z,receiver_size[0],receiver_size[1],receiver_size[2]],
 [antenna_xy[0],antenna_xy[1],antenna_face,antenna_size[0],antenna_size[1],antenna_size[2]+antenna_carrier_t],
 [light_xy[0],light_xy[1],light_z,light_size[0],light_size[1],light_size[2]]];
function sensor_rear_boxes() = [[ads_xy[0],ads_xy[1],case_depth-module_z-ads_size[2],ads_size[0],ads_size[1],ads_size[2]]];
include <enclosure_v4.scad>

// Direct PCB corner supports; no assumed board hole pattern.
module direct_seat(xy,size,z,strap=true) {
 for(x=[xy[0]+0.8,xy[0]+size[0]-3.8],y=[xy[1]+0.8,xy[1]+size[1]-3.8])
  translate([x,y,skin-eps]) cube([3,3,z-skin+eps]);
 for(x=[xy[0]-fit_gap-1.2,xy[0]+size[0]+fit_gap])
  translate([x,xy[1]+2,skin-eps]) cube([1.2,size[1]-4,z+1.8-skin+eps]);
 translate([xy[0]+size[0]/2-2,xy[1]-fit_gap-1.2,skin-eps]) cube([4,1.2,z+1-skin+eps]);
 if(strap) for(x=[xy[0]-4.8,xy[0]+size[0]+1.6]) tie_anchor(x,xy[1]+size[1]/2-3);
}
module sensor_front_mounts() {
 // Strap passes through both side rails, below PCB, then over receiver.
 difference() {
  direct_seat(receiver_xy,receiver_size,receiver_z,false);
  translate([receiver_xy[0]-2,receiver_xy[1]+receiver_size[1]/2-1.7,receiver_z-2])
   cube([receiver_size[0]+4,3.4,1.6]);
 }
 // Antenna supported only at carrier edges; ceramic faces -Z.
 for(x=[antenna_xy[0],antenna_xy[0]+antenna_size[0]-1],y=[antenna_xy[1],antenna_xy[1]+antenna_size[1]-2])
  translate([x,y,skin-eps]) cube([1,2,antenna_face+antenna_size[2]-skin+eps]);
 for(y=[antenna_xy[1]-2.5,antenna_xy[1]+antenna_size[1]+2.5])
  difference() {
   cyl_at([antenna_xy[0]+antenna_size[0]/2,y],skin-eps,4.2,antenna_face+antenna_size[2]+antenna_carrier_t-skin+eps);
   cyl_at([antenna_xy[0]+antenna_size[0]/2,y],skin+0.8,1.6,20);
  }
 direct_seat(light_xy,light_size,light_z,true);
}
module sensor_rear_mounts() { direct_seat(ads_xy,ads_size,module_z,true); }
module new_front() {
 difference() {
  front();
  translate([light_xy[0]+light_size[0]/2,light_xy[1]+light_size[1]/2,-eps]) cylinder(d=light_aperture_d,h=skin+2*eps);
 }
}
module am_bosses() {
 for(y=am_mount_ys) translate([case_w-eps,y,am_center_z]) rotate([0,90,0])
  difference() { cylinder(d=7,h=6+eps); translate([0,0,2]) cylinder(d=1.6,h=4.2); }
}
module new_back() {
 difference() {
  union() { back(); am_bosses(); }
  side_slot("left",keyboard_port_y,keyboard_port_z,keyboard_port_w,keyboard_port_h);
  translate([case_w-wall-eps,am_center_y,am_center_z]) rotate([0,90,0]) cylinder(d=am_cable_d,h=wall+2*eps);
 }
}
// Antenna rear-side frame, open at feed. Fits new direct front posts only.
module gps_retainer() {
 difference() {
  union() {
   translate([-antenna_size[0]/2-1,-1.5,0]) cube([antenna_size[0]+2,antenna_size[1]+3,1.8]);
   for(y=[-2.5,antenna_size[1]+2.5]) hull() {
    translate([0,y,0]) cylinder(d=5.2,h=1.8);
    translate([0,y<0 ? 0 : antenna_size[1],0]) cylinder(d=5.2,h=1.8);
   }
  }
  translate([-antenna_size[0]/2+1,1,-eps]) cube([antenna_size[0]-2,antenna_size[1]-2,2]);
  for(y=[-2.5,antenna_size[1]+2.5]) translate([0,y,-eps]) cylinder(d=2.2,h=2);
 }
}
// Holder is local: X/Y lie flat on print bed; Z extends out from right case wall.
// Rotated onto side wall only in assembled preview. Contact bands stay at ends.
module am_holder() {
 w=am_body[2]+2*fit_gap+4; l=am_mount_ys[1]-am_mount_ys[0]+8;
 difference() {
  union() {
   cube([w,l,2.4]);
   for(y=[9,l-12]) for(x=[0,w-2]) translate([x,y,2.4]) cube([2,3,am_body[0]+2]);
   // Upper rain visor, not a waterproof seal; side/bottom vents remain open.
   translate([0,l-8,2.4]) cube([w,2,am_body[0]+6]);
  }
  for(y=[4,l-4]) translate([w/2,y,-eps]) cylinder(d=2.2,h=3);
  translate([4,15,-eps]) cube([w-8,l-32,3]); // airflow/thermal break
  for(x=[2,w-5],y=[11,l-13]) translate([x,y,-eps]) cube([3,3.4,3]); // end straps
 }
}
module am_holder_assembly() {
 w=am_body[2]+2*fit_gap+4;
 translate([case_w+6,am_mount_ys[0]-4,am_center_z+w/2]) rotate([0,90,0]) am_holder();
}
module keyboard_tray() {
 w=keyboard_size[0]+2*fit_gap; l=keyboard_size[1]+2*fit_gap;
 difference() {
  union() {
   rounded_block(w+4,l+4,2,3);
   // Low outside-edge rails avoid covering keys; connector side stays open.
   for(x=[0,w+2]) translate([x,0,2]) cube([2,l+4,3]);
   translate([0,0,2]) cube([w+4,2,3]);
  }
  for(x=[14,w-10],y=[12,l-8]) translate([x,y,-eps]) cube([3.4,8,3]);
 }
}
// V4 retrofit GPS plate: old 4 tower pilots, new receiver retained by ties.
module retrofit_gps() {
 w=gps_size[0]; l=gps_size[1];
 difference() {
  cube([w,l,2]);
  for(x=[2.1,w-2.1],y=[2.1,l-2.1]) translate([x,y,-eps]) cylinder(d=2.2,h=3);
  for(x=[3,w-6]) translate([x,l/2-5,-eps]) cube([3,10,3]);
 }
 for(x=[(w-receiver_size[0])/2-1.2,(w+receiver_size[0])/2])
  translate([x,5,2]) cube([1.2,receiver_size[1],2]);
}
// V4 MAX screw-post retrofit; keep original pilot centres.
module retrofit_ads() {
 w=gauge_size[0]; l=gauge_size[1];
 difference() {
  cube([w,l+3,2]);
  for(x=[2.5,w-2.5],y=[2.5,l-2.5]) translate([x,y,-eps]) cylinder(d=2.2,h=3);
  for(x=[3.5,w-6.5]) translate([x,8,-eps]) cube([3,10,3]);
 }
}
// Provisional 25mm outer frame for the existing V4 antenna bay.
// New carrier rests on a 1mm perimeter ledge, not on the ceramic face.
module retrofit_antenna() {
 difference() {
  cube([25,25,2]);
  translate([(25-antenna_size[0]-fit_gap)/2,(25-antenna_size[1]-fit_gap)/2,0.8])
   cube([antenna_size[0]+fit_gap,antenna_size[1]+fit_gap,2]);
  translate([(25-antenna_size[0]+2)/2,(25-antenna_size[1]+2)/2,-eps])
   cube([antenna_size[0]-2,antenna_size[1]-2,3]);
 }
}
module hardware_preview() {
 for(b=sensor_front_boxes()) color([0.12,0.65,0.42,0.65]) box_envelope(b);
 for(b=sensor_rear_boxes()) color([0.15,0.45,0.9,0.65]) box_envelope(b);
}
module revised_assembly(gap=0) {
 color([0.58,0.64,0.72]) new_front();
 translate([0,0,gap]) color([0.30,0.38,0.47]) new_back();
 translate([0,0,gap]) color([0.95,0.56,0.13]) am_holder_assembly();
 if($preview) { front_ghosts(); rear_ghosts(); hardware_preview(); }
}
module new_fit_coupon() {
 // Small wall sample for cable and optical apertures; clip widths check separately.
 difference() {
  cube([44,30,2.4]);
  translate([5,6,-eps]) cube([keyboard_port_w,keyboard_port_h,3]);
  translate([30,10,-eps]) cylinder(d=am_cable_d,h=3);
  translate([30,22,-eps]) cylinder(d=light_aperture_d,h=3);
 }
}
assert(receiver_xy[0]+receiver_size[0]+1 <= antenna_xy[0],"Receiver/antenna need lateral clearance");
assert(ads_xy[1]-1.7 >= battery_xy[1]+battery_size[1],"ADS rail hits battery bay");
assert(am_center_z-am_body[2]/2>front_depth+2 && am_center_z+am_body[2]/2<case_depth-2,"AM2302 exceeds rear wall height");
echo(provisional_receiver=receiver_size,provisional_antenna=antenna_size,provisional_ads=ads_size,provisional_am=am_body);
if(revision_part=="front") new_front();
else if(revision_part=="back") rear_frame() new_back();
else if(revision_part=="gps_retainer") translate([antenna_size[0]/2+3,4,0]) gps_retainer();
else if(revision_part=="am2302_holder") am_holder();
else if(revision_part=="keyboard_tray") keyboard_tray();
else if(revision_part=="retrofit_gps") retrofit_gps();
else if(revision_part=="retrofit_ads") retrofit_ads();
else if(revision_part=="retrofit_antenna") retrofit_antenna();
else if(revision_part=="fit_coupon") new_fit_coupon();
else if(revision_part=="assembled") revised_assembly();
else if(revision_part=="exploded") revised_assembly(32);
else if(revision_part=="interference") intersection() { new_front(); translate([0,0,0.05]) new_back(); }
else assert(false,"Unknown revision_part");
