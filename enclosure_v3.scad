/*
 S3 LoRa Interface - V3 mechanical prototype (millimetres)
 See docs/ENCLOSURE.md for measurements, hardware and print instructions.
 Verified drawing dimensions: LCDWiki ES3C28P and Heltec V3.2.
 Small modules, connector centres and battery/holder MUST be checked on your parts.

 Assembly frame: X right, Y toward antenna end, Z from front face to rear.
 Every component and cutout uses that frame. Only print output flips the rear.
 Hardware ghosts are preview-only and never enter STL exports.
*/

/* [View] */
part = "exploded"; // [exploded, assembled, front, back, clamps, print_layout, fit_coupon, components]
show_components = true;
explosion = 32;

/* [Shell] */
case_w = 100;
case_l = 112;
case_depth = 42;
front_depth = 16;
wall = 2.4;
skin = 2.4;
corner_r = 7;
lip_h = 1.8;
lip_t = 1.2;
lip_clearance = 0.3;
board_clearance = 0.4;

/* [Case screws] */
case_fastener = "pilot"; // [pilot, heatset]
case_pilot_d = 2.6;
case_clearance_d = 3.4;
case_boss_d = 8;
insert_d = 4.2;
insert_length = 4.5;

/* [CYD - ES3C28P capacitive touch] */
cyd_x = 7;
cyd_y = 11;
cyd_w = 50;
cyd_l = 86;
cyd_h = 10.6;
cyd_glass_w = 50;
cyd_glass_l = 69.2;
cyd_glass_y = 8.22;
cyd_glass_gap = 0.3;
cyd_glass_to_pcb = 5.9;
cyd_pcb_t = 1.6;
cyd_hole_x = 4;
cyd_hole_y = 4;
cyd_hole_pitch_x = 42;
cyd_hole_pitch_y = 78;
cyd_mount_pilot_d = 2.5;
cyd_usb_x = 25;
cyd_usb_z_from_glass = 8.5;
usb_open_w = 18;
usb_open_h = 11;

/* [Heltec V3.2 - bare board without header stack] */
heltec_x = 64;
heltec_y = 11;
heltec_w = 25.5;
heltec_l = 50.2;
heltec_h = 10.2;
heltec_oled_face_z = 3.2;
heltec_oled_to_pcb = 5;
heltec_pcb_t = 1.6;
heltec_window_w = 19.2;
heltec_window_l = 27.9;
heltec_window_y = 17.4;
heltec_usb_z = 11.4;
clamp_pilot_d = 2.5;
clamp_seat_z = heltec_oled_face_z + heltec_h + 1.6;
clamp_t = 2.4;
clamp_pad_w = 0.8;
clamp_pad_gap = 0.2;

/* [Battery - measure protected cells and holders] */
battery_style = "bare"; // [bare, holder]
cell_d = 18.8;
cell_l = 67;
holder_l = 78;
holder_w = 22;
holder_h = 22;
battery_x = 12;
battery_y = battery_style == "bare" ? 60 : 63;
battery_seat = 1.4;
tie_w = 3.4;
tie_h = 1.6;

/* [Rear modules - outside envelopes including solder] */
charger_size = [28,18,5.5]; // long dimension along X; USB faces LEFT
charger_xy = [3.2,17];
charger_usb_local_z = 6.8; // measured from outside REAR face
boost_size = [17,37,8];
boost_xy = [37,13];
gauge_size = [15,15,4];
gauge_xy = [65,18];
board_seat = 1.4;
module_rail_h = 2.8;

/* [GPS - separate patch on coax, behind plastic front] */
gps_xy = [65,73];
gps_size = [25,35,8];
gps_board_z = 11.4;
gps_pcb_t = 1.6;
patch_size = [24,24,6];
patch_face_z = 3.4;
patch_xy = [65.5,78.5];

/* [Antenna and switch - verify actual panel hardware] */
sma_d = 6.5;
sma_x = 77;
sma_z = 30;
switch_enabled = true;
switch_x = 46;
switch_z = 29;
switch_open_w = 13.7;
switch_open_h = 9.1;
switch_body = [18,12,14];

/* [Optional speaker - disabled until measured] */
speaker_enabled = false;
speaker_xy = [21,92];
speaker_d = 28;
speaker_h = 6;

/* [Hidden] */
$fn = $preview ? 36 : 64;
eps = 0.04;
back_depth = case_depth - front_depth;
cyd_face_z = skin + cyd_glass_gap;
cyd_pcb_z = cyd_face_z + cyd_glass_to_pcb;
heltec_pcb_z = heltec_oled_face_z + heltec_oled_to_pcb;
battery_w = battery_style == "bare" ? cell_d : holder_w;
battery_l = battery_style == "bare" ? cell_l : holder_l;
battery_h = battery_style == "bare" ? cell_d : holder_h;
battery_axis = skin + battery_seat + cell_d/2;
module_z = skin + board_seat;
case_pts = [[5,5],[case_w-5,5],[5,case_l-5],[case_w-5,case_l-5]];
clamp_ys = [heltec_y+9, heltec_y+47];
clamp_xs = [heltec_x-4, heltec_x+heltec_w+4.5];
cyd_pts = [for (x=[0,1],y=[0,1])
    [cyd_x+cyd_hole_x+x*cyd_hole_pitch_x,
     cyd_y+cyd_hole_y+y*cyd_hole_pitch_y]];

function separated(a,b,g=0) =
    a[0]+a[3]+g <= b[0] || b[0]+b[3]+g <= a[0] ||
    a[1]+a[4]+g <= b[1] || b[1]+b[4]+g <= a[1] ||
    a[2]+a[5]+g <= b[2] || b[2]+b[5]+g <= a[2];
function inside(a) = a[0]>=wall && a[1]>=wall && a[2]>=skin &&
    a[0]+a[3]<=case_w-wall && a[1]+a[4]<=case_l-wall &&
    a[2]+a[5]<=case_depth-skin;
front_boxes = [
    [cyd_x,cyd_y,cyd_face_z,cyd_w,cyd_l,cyd_h],
    [heltec_x,heltec_y,heltec_oled_face_z,heltec_w,heltec_l,heltec_h],
    [gps_xy[0],gps_xy[1],gps_board_z,gps_size[0],gps_size[1],gps_size[2]],
    [patch_xy[0],patch_xy[1],patch_face_z,patch_size[0],patch_size[1],patch_size[2]]];
rear_boxes = [
    [battery_x,battery_y-battery_w/2,case_depth-skin-battery_seat-battery_h,
        battery_l,battery_w,battery_h],
    for (m=[[charger_xy,charger_size],[boost_xy,boost_size],[gauge_xy,gauge_size]])
        [m[0][0],m[0][1],case_depth-module_z-m[1][2],m[1][0],m[1][1],m[1][2]]];
switch_box = [switch_x-switch_body[0]/2,case_l-wall-switch_body[1],switch_z-switch_body[2]/2,
    switch_body[0],switch_body[1],switch_body[2]];
// Conservative space for nut, connector and beginning of pigtail, not an RF model.
sma_box = [sma_x-6,case_l-wall-10,sma_z-6,12,10,12];
speaker_box = [speaker_xy[0]-speaker_d/2,speaker_xy[1]-speaker_d/2,
    case_depth-module_z-speaker_h,speaker_d,speaker_d,speaker_h];
all_boxes = concat(front_boxes,rear_boxes,[sma_box],
    switch_enabled ? [switch_box] : [],speaker_enabled ? [speaker_box] : []);
assert(case_depth > front_depth+skin+lip_h, "Rear shell too shallow");
assert(skin>=1.6 && wall>=2 && lip_clearance>0, "Check print wall/fit values");
assert(corner_r>wall+lip_clearance+lip_t, "Corner radius too small for alignment lip");
assert(case_fastener=="pilot" || case_fastener=="heatset", "Unknown case fastener");
assert(battery_style=="bare" || battery_style=="holder", "Unknown battery style");
assert(cyd_pcb_z-skin>=4.5, "CYD screw posts too shallow");
assert(heltec_pcb_z+heltec_pcb_t+clamp_pad_gap<clamp_seat_z, "Heltec clamp pads inverted");
assert(clamp_seat_z+clamp_t+1 <= rear_boxes[0][2], "Heltec clamps too close to battery: increase case_depth");
assert(clamp_xs[0]-2.6 >= cyd_x+cyd_w+0.2, "Heltec clamp post hits CYD");
assert(clamp_xs[1]+3.4 < case_w-wall, "Heltec clamps need more case width");
assert(back_depth>skin+battery_seat+battery_h+0.5, "Battery or holder too tall for rear half");
assert(battery_x>=wall+3 && battery_x+battery_l+3<=case_w-wall,
    "Battery/holder needs more width for end wires");
assert(!speaker_enabled || speaker_d/2+speaker_xy[0]+2 < gps_xy[0], "Speaker overlaps GPS bay");
for (i=[0:len(all_boxes)-1]) {
    assert(inside(all_boxes[i]),str("Component envelope outside shell: ",i));
    for(j=[0:len(all_boxes)-1]) if(j>i)
        assert(separated(all_boxes[i],all_boxes[j],0.3),str("Component envelopes overlap: ",i," / ",j));
}
echo(case_mm=[case_w,case_l,case_depth], front_mm=front_depth, rear_mm=back_depth);
echo(battery_to_cyd_gap_mm=rear_boxes[0][2]-(cyd_face_z+cyd_h));
echo(case_screw_target_under_head_mm=case_depth-front_depth+(case_fastener=="heatset" ? 4 : 9),
    case_fastener=case_fastener);

module rounded_block(w,l,h,r=2) {
    assert(min(w,l,h)>0,"Non-positive solid dimension");
    linear_extrude(h) hull()
        for(x=[r,w-r],y=[r,l-r]) translate([x,y]) circle(r=r);
}
module ring(w,l,h,t,r) {
    difference() {
        rounded_block(w,l,h,r);
        translate([t,t,-eps]) rounded_block(w-2*t,l-2*t,h+2*eps,max(0.5,r-t));
    }
}
module cyl_at(p,z,d,h) { translate([p[0],p[1],z]) cylinder(d=d,h=h); }
module box_envelope(b) { translate([b[0],b[1],b[2]]) cube([b[3],b[4],b[5]]); }
// Involution: same mapping is used for rear construction, preview and print.
module rear_frame() { translate([case_w,0,case_depth]) rotate([0,180,0]) children(); }
// A local rear module keeps assembly X/Y unchanged, reversing only its Z.
module rear_local() { translate([0,0,case_depth]) mirror([0,0,1]) children(); }

module side_slot(side,pos,z,w,h) {
    if(side=="bottom") translate([pos-w/2,-eps,z-h/2]) cube([w,wall+2*eps,h]);
    if(side=="left") translate([-eps,pos-w/2,z-h/2]) cube([wall+2*eps,w,h]);
    if(side=="top") translate([pos-w/2,case_l-wall-eps,z-h/2]) cube([w,wall+2*eps,h]);
}
module ports() {
    side_slot("bottom",cyd_x+cyd_usb_x,cyd_face_z+cyd_usb_z_from_glass,usb_open_w,usb_open_h);
    side_slot("bottom",heltec_x+heltec_w/2,heltec_usb_z,usb_open_w,usb_open_h);
    side_slot("left",charger_xy[1]+charger_size[1]/2,case_depth-charger_usb_local_z,14,8);
    if(switch_enabled) side_slot("top",switch_x,switch_z,switch_open_w,switch_open_h);
    translate([sma_x,case_l+eps,sma_z]) rotate([90,0,0]) cylinder(d=sma_d,h=wall+2*eps);
}

// Raised anchors accept small zip ties without holes through the outer skin.
// The tunnel is along X; a 3.4 mm wide tie lies in the Y/Z plane.
module tie_anchor(x,y,z=skin,h=5) {
    translate([x,y,z-eps]) difference() {
        rounded_block(3.2,6,h+eps,0.7);
        translate([-eps,(6-tie_w)/2,0.8]) cube([3.2+2*eps,tie_w,tie_h]);
    }
}
module pcb_seat(xy,size,z=module_z) {
    // Four short ledges leave the centre and underside solder joints free.
    for(x=[xy[0]+1,xy[0]+size[0]-4],y=[xy[1]+1,xy[1]+size[1]-4])
        translate([x,y,skin-eps]) cube([3,3,z-skin+eps]);
    // Corner fences restrain XY without covering connector or solder edges.
    for(x=[xy[0]-board_clearance-1.2,xy[0]+size[0]+board_clearance],
        y=[xy[1]+2,xy[1]+size[1]-6])
        translate([x,y,skin-eps]) cube([1.2,4,module_rail_h+eps]);
    for(y=[xy[1]-board_clearance-1.2,xy[1]+size[1]+board_clearance])
        translate([xy[0]+size[0]/2-2,y,skin-eps]) cube([4,1.2,module_rail_h+eps]);
    tie_anchor(xy[0]-4.8,xy[1]+size[1]/2-3);
    tie_anchor(xy[0]+size[0]+1.6,xy[1]+size[1]/2-3);
}

module front_mounts() {
    for(p=cyd_pts) cyl_at(p,skin-eps,6,cyd_pcb_z-skin+eps);
    // Narrow Heltec supports touch only the outer PCB margin, not pin rows.
    for(x=[heltec_x,heltec_x+heltec_w-clamp_pad_w],y=clamp_ys)
        translate([x,y-1.4,skin-eps]) cube([clamp_pad_w,2.8,heltec_pcb_z-skin+eps]);
    for(x=clamp_xs,y=clamp_ys) cyl_at([x,y],skin-eps,5.2,clamp_seat_z-skin+eps);
    // Open GPS support, with no invented mounting-hole pattern.
    for(x=[gps_xy[0]+0.6,gps_xy[0]+gps_size[0]-3.6],
        y=[gps_xy[1]+0.6,gps_xy[1]+gps_size[1]-3.6])
        translate([x,y,skin-eps]) cube([3,3,gps_board_z-skin+eps]);
    // Patch stays behind an unbroken plastic face; foam sets its Z position.
    for(x=[patch_xy[0]-1.4,patch_xy[0]+patch_size[0]+0.4])
        translate([x,patch_xy[1],skin-eps]) cube([1,patch_size[1],6+eps]);
    // Insert upper PCB corners under these keepers, then strap the lower end.
    // Requires a clear 0.8 mm PCB edge and measured PCB thickness.
    for(x=[gps_xy[0]-1.6,gps_xy[0]+gps_size[0]+0.4])
        translate([x,gps_xy[1]+gps_size[1]-3.5,skin-eps]) cube([1.2,3,gps_board_z+gps_pcb_t+1-skin+eps]);
    for(x=[gps_xy[0]-1.6,gps_xy[0]+gps_size[0]-0.8])
        translate([x,gps_xy[1]+gps_size[1]-3.5,gps_board_z+gps_pcb_t+0.3]) cube([2.4,3,0.7]);
    tie_anchor(gps_xy[0]-4.8,gps_xy[1]-1);
    tie_anchor(gps_xy[0]+gps_size[0]+1.6,gps_xy[1]-1);
}
module front() {
    difference() {
        union() {
            difference() {
                rounded_block(case_w,case_l,front_depth,corner_r);
                translate([wall,wall,skin]) rounded_block(case_w-2*wall,case_l-2*wall,front_depth,corner_r-wall);
            }
            for(p=case_pts) cyl_at(p,skin-eps,case_boss_d,front_depth-skin+eps);
            front_mounts();
        }
        translate([cyd_x-board_clearance,cyd_y+cyd_glass_y-board_clearance,-eps])
            rounded_block(cyd_glass_w+2*board_clearance,cyd_glass_l+2*board_clearance,skin+2*eps,1);
        translate([heltec_x+(heltec_w-heltec_window_w)/2,heltec_y+heltec_window_y,-eps])
            rounded_block(heltec_window_w,heltec_window_l,skin+2*eps,1);
        for(p=cyd_pts) cyl_at(p,skin+0.5,cyd_mount_pilot_d,cyd_pcb_z);
        for(x=clamp_xs,y=clamp_ys) cyl_at([x,y],clamp_seat_z-8,clamp_pilot_d,9);
        for(p=case_pts) {
            cyl_at(p,front_depth-11,case_fastener=="heatset" ? case_clearance_d : case_pilot_d,12);
            if(case_fastener=="heatset") cyl_at(p,front_depth-insert_length,insert_d,insert_length+eps);
        }
        ports();
    }
}

module battery_cradle() {
    if(battery_style=="bare") {
        for(x=[battery_x+8,battery_x+cell_l-18]) difference() {
            translate([x,battery_y-cell_d/2-2,skin-eps])
                cube([10,cell_d+4,battery_axis-skin-2+eps]);
            translate([x-eps,battery_y,battery_axis]) rotate([0,90,0])
                cylinder(d=cell_d+0.6,h=10+2*eps);
            translate([x+(10-tie_w)/2,battery_y-cell_d/2-3,skin+0.5])
                cube([tie_w,cell_d+6,tie_h]);
        }
        for(x=[battery_x-2.2,battery_x+cell_l+0.6])
            translate([x,battery_y-5,skin-eps]) cube([1.6,10,6+eps]);
    } else {
        pcb_seat([battery_x,battery_y-holder_w/2],[holder_l,holder_w,holder_h],skin+battery_seat);
    }
}
module rear_features() {
    battery_cradle();
    // Charger at LEFT boundary: omit its outside anchor, preserve USB clearance.
    difference() {
        pcb_seat(charger_xy,charger_size);
        translate([-6,charger_xy[1]+4,skin-2*eps]) cube([charger_xy[0]+6,charger_size[1]-8,12+2*eps]);
    }
    // A second charger tie anchor sits at its accessible, inside end.
    tie_anchor(charger_xy[0]+charger_size[0]/2-1.6,charger_xy[1]-8);
    tie_anchor(charger_xy[0]+charger_size[0]/2-1.6,charger_xy[1]+charger_size[1]+2);
    pcb_seat(boost_xy,boost_size);
    pcb_seat(gauge_xy,gauge_size);
    if(speaker_enabled) translate([speaker_xy[0],speaker_xy[1],skin-eps]) difference() {
        cylinder(d=speaker_d+3,h=3+eps);
        translate([0,0,-eps]) cylinder(d=speaker_d+0.6,h=3+3*eps);
    }
}
module back() {
    difference() {
        union() {
            rear_local() {
                difference() {
                    rounded_block(case_w,case_l,back_depth,corner_r);
                    translate([wall,wall,skin]) rounded_block(case_w-2*wall,case_l-2*wall,back_depth,corner_r-wall);
                }
                for(p=case_pts) cyl_at(p,skin-eps,case_boss_d,back_depth-skin+eps);
                rear_features();
            }
            // A tapered root joins the inset lip to the rear wall. Without
            // this bridge a clearance-offset ring would be four loose parts.
            difference() {
                translate([wall-0.6,wall-0.6,front_depth])
                    rounded_block(case_w-2*(wall-0.6),case_l-2*(wall-0.6),1.6,corner_r-wall+0.6);
                hull() {
                    translate([wall+lip_clearance+lip_t,wall+lip_clearance+lip_t,front_depth-eps])
                        rounded_block(case_w-2*(wall+lip_clearance+lip_t),case_l-2*(wall+lip_clearance+lip_t),eps,corner_r-wall-lip_clearance-lip_t);
                    translate([wall,wall,front_depth+1.6])
                        rounded_block(case_w-2*wall,case_l-2*wall,eps,corner_r-wall);
                }
            }
            // Male lip enters the front cavity, interrupted around pillars.
            difference() {
                translate([wall+lip_clearance,wall+lip_clearance,front_depth-lip_h])
                    ring(case_w-2*(wall+lip_clearance),case_l-2*(wall+lip_clearance),lip_h+eps,lip_t,corner_r-wall-lip_clearance);
                for(p=case_pts) cyl_at(p,front_depth-lip_h-eps,case_boss_d+1,lip_h+3*eps);
            }
        }
        for(p=case_pts) {
            cyl_at(p,front_depth-eps,case_clearance_d,back_depth+2*eps);
            cyl_at(p,case_depth-1,6.6,1+eps);
        }
        // Clamp end caps enter the lip zone; give them clearance at the seam.
        for(x=clamp_xs,y=clamp_ys)
            cyl_at([x,y],front_depth-lip_h-eps,8.4,clamp_seat_z+clamp_t-front_depth+lip_h+1);
        ports();
        if(speaker_enabled) for(x=[-8,-4,0,4,8])
            translate([speaker_xy[0]+x-0.7,speaker_xy[1]-7,case_depth-skin-eps])
                rounded_block(1.4,14,skin+2*eps,0.6);
    }
}
module clamp_bar() {
    span=clamp_xs[1]-clamp_xs[0];
    pad_bottom=heltec_pcb_z+heltec_pcb_t+clamp_pad_gap;
    difference() {
        union() {
            translate([clamp_xs[0]-3,-2,clamp_seat_z]) rounded_block(span+6,4,clamp_t,1.5);
            for(x=clamp_xs) cyl_at([x,0],clamp_seat_z,6.8,clamp_t);
            for(x=[heltec_x,heltec_x+heltec_w-clamp_pad_w])
                translate([x,-1.4,pad_bottom]) cube([clamp_pad_w,2.8,clamp_seat_z-pad_bottom+eps]);
        }
        for(x=clamp_xs) {
            cyl_at([x,0],clamp_seat_z-eps,3.2,clamp_t+2*eps);
            // M3 countersunk heads must sit flush beneath the battery zone.
            translate([x,0,clamp_seat_z+clamp_t-1.5]) cylinder(d1=3.2,d2=6.2,h=1.5+eps);
        }
    }
}
module clamps_print() {
    // Flat bridge face on the bed; little edge pads point up.
    for(y=[0,10]) translate([clamp_xs[1]+3.4,y,clamp_seat_z+clamp_t]) rotate([0,180,0]) clamp_bar();
}
module front_ghosts() {
    // Rectangles are conservative total stack envelopes, NOT detailed PCBs.
    for(i=[0:len(front_boxes)-1]) color(i==3 ? [0.9,0.8,0.5,0.65] : [0.05,0.55,0.42,0.45]) box_envelope(front_boxes[i]);
    color([0.025,0.04,0.06]) translate([cyd_x,cyd_y+cyd_glass_y,cyd_face_z-0.06])
        cube([cyd_glass_w,cyd_glass_l,0.06]);
    color([0.10,0.26,0.37]) translate([cyd_x+3.4,cyd_y+11.12,cyd_face_z-0.09]) cube([43.2,57.6,0.03]);
    color([0.02,0.03,0.04]) translate([heltec_x+(heltec_w-18.56)/2,heltec_y+17.7,heltec_oled_face_z-0.06]) cube([18.56,27.28,0.06]);
}
module rear_ghosts() {
    color([0.16,0.45,0.85,0.5]) for(i=[1:3]) box_envelope(rear_boxes[i]);
    color([0.2,0.6,0.85,0.6]) if(battery_style=="bare") rear_local()
        translate([battery_x,battery_y,battery_axis]) rotate([0,90,0]) cylinder(d=cell_d,h=cell_l);
    else box_envelope(rear_boxes[0]);
    if(switch_enabled) color([0.2,0.2,0.2,0.5])
        translate([switch_x-switch_body[0]/2,case_l-wall-switch_body[1],switch_z-switch_body[2]/2]) cube(switch_body);
    if(speaker_enabled) color([0.2,0.2,0.2,0.5]) rear_local()
        translate([speaker_xy[0],speaker_xy[1],module_z]) cylinder(d=speaker_d,h=speaker_h);
}
module ghosts() { front_ghosts(); rear_ghosts(); }
module assembly(gap=0) {
    color([0.60,0.66,0.72]) front();
    for(y=clamp_ys) color([0.95,0.57,0.15]) translate([0,y,gap/3]) clamp_bar();
    translate([0,0,gap]) color([0.32,0.39,0.46]) back();
    if($preview && show_components) {
        // Move only rear-mounted hardware with its half in exploded views.
        front_ghosts();
        translate([0,0,gap]) rear_ghosts();
    }
}
module fit_coupon() {
    // Real CYD post pitch and real shell USB slot, trimmed to a narrow strip.
    intersection() {
        front();
        translate([cyd_x-2,0,0]) cube([cyd_w+4,cyd_y+10,front_depth]);
    }
    // Separate gauge: left = case pilot, middle = clearance, right = insert.
    translate([0,32,0]) difference() {
        union() {
            rounded_block(40,14,skin,2);
            for(x=[6,20,34]) cyl_at([x,7],skin-eps,8,6+eps);
        }
        for(i=[0:2]) cyl_at([6+14*i,7],-eps,[case_pilot_d,case_clearance_d,insert_d][i],skin+6+2*eps);
    }
}

if(part=="front") front();
else if(part=="back") rear_frame() back();
else if(part=="clamps") clamps_print();
else if(part=="fit_coupon") fit_coupon();
else if(part=="components") { if($preview) ghosts(); }
else if(part=="assembled") assembly();
else if(part=="exploded") assembly(explosion);
else if(part=="interference") intersection() {
    union() { front(); for(y=clamp_ys) translate([0,y,0]) clamp_bar(); }
    // Lift 0.05 mm to avoid reporting zero-thickness seam contact as a mesh.
    translate([0,0,0.05]) back();
}
else if(part=="print_layout") {
    front();
    translate([case_w+10,0,0]) rear_frame() back();
    translate([0,case_l+10,0]) clamps_print();
} else assert(false,str("Unknown part: ",part));
