/*
 S3 LoRa Interface - V4, measured-component prototype (millimetres)
 See docs/ENCLOSURE_V4.md. User measurements and photo: 2026-10-17.
 Power board confirmed 70 x 30 x 24; SINGLE holder 76.4 x 22.6 x 20.9.
 CYD/Heltec/power/holder long edges follow Y; GPS/speaker long edges follow X.
 Port centres, PCB edge keepouts and mounting offsets still need fit checking.
 Front-facing (-Z): CYD glass, Heltec OLED and GPS ceramic patch.
 Rear-facing (+Z): speaker sound face, toward the rear grille.

 Assembly frame: X right, Y toward antenna end, Z from front face to rear.
 Every component and cutout uses that frame. Only print output flips the rear.
 Hardware ghosts are preview-only and never enter STL exports.
*/

/* [View] */
part = is_undef(coupon_mode) ? "exploded" : "components"; // [exploded, assembled, front, back, clamps, antenna_retainers, speaker_retainer, print_layout, fit_coupon, components]
show_components = true;
explosion = 32;

/* [Shell] */
case_w = 104;
case_l = 124;
case_depth = 50;
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
cyd_h = 9.5;
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
cyd_mount_pilot_d = is_undef(cyd_pilot_override) ? 2.5 : cyd_pilot_override;
cyd_usb_x = 25;
cyd_usb_z_from_glass = 8.5;
usb_open_w = 18;
usb_open_h = 11;

/* [Heltec - complete measured envelope INCLUDING soldered pins] */
heltec_x = 64;
heltec_y = 11;
heltec_w = 25.7;
heltec_l = 52;
heltec_h = 15.3; // full assembly height including pins; do not add pin height again
heltec_oled_face_z = 3.2;
heltec_oled_to_pcb = 5;
heltec_pcb_t = 1.6;
heltec_window_w = 19.2;
heltec_window_l = 27.9;
heltec_window_y = 17.4;
heltec_usb_z = 11.4;
clamp_pilot_d = 2.5;
clamp_seat_z = heltec_oled_face_z + heltec_h + 1.6; // bridge clears pin-inclusive envelope
clamp_post_top_z = clamp_seat_z - (is_undef(heltec_post_drop) ? 0 : heltec_post_drop);
clamp_t = 2.4;
clamp_pad_w = 0.8;
clamp_pad_brace = 1.2;
clamp_pad_gap = 0;

/* [Battery assembly - measured outside including holder] */
battery_size = (!is_undef(open_battery_bay) && open_battery_bay) ? [48,80,20.9] : [22.6,76.4,20.9];
battery_xy = (!is_undef(open_battery_bay) && open_battery_bay) ? [52,10] : [72,11];
battery_seat = 2;
tie_w = 3.4;
tie_h = 1.6;

/* [Assembled power board - do not split charger and boost] */
power_size = [30,70,24];
power_xy = [9,11]; // charger at upper end, USB faces LEFT as in photo
power_usb_y_from_top = 16; // estimate: measure connector centre on assembly
power_usb_local_z = 9.5; // estimate: connector centre from outside rear face
power_usb_w = 18;
power_usb_h = 12;
// Select the opposite end while retaining the same side-wall opening.
power_usb_y = power_xy[1] + ((!is_undef(power_usb_at_bottom) && power_usb_at_bottom)
    ? power_usb_y_from_top : power_size[1]-power_usb_y_from_top);

/* [MAX breakout - measured including connector height] */
gauge_size = [20.4,25.45,11.35];
gauge_xy = (!is_undef(open_battery_bay) && open_battery_bay) ? [70,94] : [45.3,45];
board_seat = 2;
module_rail_h = 2.8;

/* [GPS - separate patch on coax, behind plastic front] */
gps_xy = [58.5,90.5]; // landscape; left shift clears rear corner screw pillar
gps_size = [25.93,36.58,11.15];
gps_pcb_t = 1.6;
patch_size = [25,25,7.5];
patch_cover_air_gap = 3; // general GNSS guidance; verify with actual antenna/material
patch_face_z = skin + patch_cover_air_gap;
patch_board_t = 1.6; // check actual carrier PCB thickness
patch_back_relief = 2; // rear feed/solder allowance, additional to measured envelope
patch_support_z = patch_face_z + patch_size[2]
    - ((!is_undef(patch_height_includes_pcb) && patch_height_includes_pcb) ? patch_board_t : 0);
patch_ledge_lowering = is_undef(patch_ledge_drop) ? 0 : patch_ledge_drop;
patch_ledge_z = patch_support_z-patch_ledge_lowering;
patch_effective_face_z = (!is_undef(antenna_flat_stack) && antenna_flat_stack)
    ? patch_ledge_z : patch_face_z-patch_ledge_lowering;
patch_retainer_z = patch_ledge_lowering>0 ? patch_support_z : patch_support_z + patch_board_t + 0.1;
patch_retainer_t = 1.8;
patch_fit_extra = is_undef(patch_extra_clearance) ? 0 : patch_extra_clearance;
patch_shift_x = is_undef(patch_lateral_shift) ? 0 : patch_lateral_shift;
patch_post_xs = [64.5-patch_fit_extra+patch_shift_x,94.5+patch_fit_extra+patch_shift_x];
patch_post_ys = [99,108];
gps_board_z = 18;
patch_xy = [67+patch_shift_x,90.965];

/* [Antenna and switch - verify actual panel hardware] */
sma_d = 6.5;
sma_x = 16;
sma_z = 28;
switch_enabled = true;
switch_x = 43;
switch_z = 27;
switch_open_w = 13.7;
switch_open_h = 9.1;
switch_body = [18,12,14];

/* [Measured oval speaker - sound face toward rear grille] */
speaker_enabled = true;
speaker_xy = [10,86]; // lower-left corner of outside housing
speaker_size = [40.8,28.2,10.3];
speaker_face_gap = 0.8; // empty sound gap; no foam or adhesive
speaker_corner_r = 12; // approximate oval housing; verify against actual frame
speaker_rim_t = 2; // initial rim thickness; floating retainer adjusts by screw travel
speaker_retainer_z = 9;
speaker_retainer_t = 2;
speaker_post_h = 6;

/* [Hidden] */
$fn = $preview ? 36 : 64;
eps = 0.04;
back_depth = case_depth - front_depth;
cyd_face_z = skin + cyd_glass_gap;
cyd_pcb_z = is_undef(cyd_post_height_override)
    ? cyd_face_z + cyd_glass_to_pcb : skin + cyd_post_height_override;
// A measured support correction moves the connector with the PCB without
// inventing a new glass-to-PCB measurement for the component preview.
cyd_mount_shift_z = cyd_pcb_z - (cyd_face_z + cyd_glass_to_pcb);
cyd_usb_center_z = cyd_face_z + cyd_usb_z_from_glass + cyd_mount_shift_z;
heltec_pcb_z = heltec_oled_face_z + heltec_oled_to_pcb;
battery_tie_ys = [battery_xy[1]+18,battery_xy[1]+65];
power_tie_ys = (!is_undef(power_usb_at_bottom) && power_usb_at_bottom)
    ? [power_xy[1]+3,power_xy[1]+power_size[1]-9]
    : [power_xy[1]+9,power_xy[1]+power_size[1]-3];
module_z = skin + board_seat;
speaker_post_xs = [speaker_xy[0]-3,speaker_xy[0]+speaker_size[0]+3];
speaker_post_y = speaker_xy[1]+speaker_size[1]/2;
case_screw_len = (!is_undef(face_entry_case_screws) && face_entry_case_screws)
    ? floor((front_depth+9)/5)*5 : floor((case_depth-front_depth+9)/5)*5;
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
    [gps_xy[0],gps_xy[1],gps_board_z,gps_size[1],gps_size[0],gps_size[2]],
    [patch_xy[0],patch_xy[1],patch_effective_face_z,patch_size[0],patch_size[1],patch_size[2]
        + ((!is_undef(patch_height_includes_pcb) && patch_height_includes_pcb) ? 0 : patch_board_t)
        +patch_back_relief]];
rear_boxes = [
    [battery_xy[0],battery_xy[1],
        (!is_undef(open_battery_bay) && open_battery_bay)
            ? case_depth-1.6-1-battery_size[2] : case_depth-skin-battery_seat-battery_size[2],
        battery_size[0],battery_size[1],battery_size[2]],
    for (m=[[power_xy,power_size],[gauge_xy,gauge_size]])
        [m[0][0],m[0][1],case_depth-module_z-m[1][2],m[1][0],m[1][1],m[1][2]]];
switch_box = [switch_x-switch_body[0]/2,case_l-wall-switch_body[1],switch_z-switch_body[2]/2,
    switch_body[0],switch_body[1],switch_body[2]];
// Conservative space for nut, connector and beginning of pigtail, not an RF model.
sma_box = [sma_x-6,case_l-wall-10,sma_z-6,12,10,12];
speaker_box = [speaker_xy[0],speaker_xy[1],case_depth-skin-speaker_face_gap-speaker_size[2],
    speaker_size[0],speaker_size[1],speaker_size[2]];
all_boxes = concat(front_boxes,rear_boxes,[sma_box],
    switch_enabled ? [switch_box] : [],speaker_enabled ? [speaker_box] : []);
assert(case_depth > front_depth+skin+lip_h, "Rear shell too shallow");
assert(skin>=1.6 && wall>=2 && lip_clearance>0, "Check print wall/fit values");
assert(corner_r>wall+lip_clearance+lip_t, "Corner radius too small for alignment lip");
assert(case_fastener=="pilot" || case_fastener=="heatset", "Unknown case fastener");
assert(cyd_pcb_z-skin>=2.5, "CYD screw posts too shallow for blind pilots");
assert(heltec_pcb_z+heltec_pcb_t+clamp_pad_gap<clamp_seat_z, "Heltec clamp pads inverted");
assert(clamp_seat_z+clamp_t+1 <= rear_boxes[0][2], "Heltec clamps too close to battery: increase case_depth");
assert(clamp_xs[0]-2.6 >= cyd_x+cyd_w+0.2, "Heltec clamp post hits CYD");
assert(clamp_xs[1]+3.4 < case_w-wall, "Heltec clamps need more case width");
assert(back_depth>skin+battery_seat+battery_size[2]+0.5, "Battery assembly too tall for rear half");
if(is_undef(open_battery_bay) || !open_battery_bay)
    assert(battery_xy[0]>=wall+3 && battery_xy[0]+battery_size[0]+3<=case_w-wall,
        "Battery assembly needs side clearance");
assert(gps_board_z>=patch_support_z+patch_board_t+patch_back_relief+1, "GPS PCB too close to antenna rear feed");
assert(patch_cover_air_gap>=3, "Keep a 3 mm air gap in front of GPS patch");
assert(patch_xy[0]-(cyd_x+cyd_w)>=10, "GPS patch too close to CYD edge");
assert(power_xy[0]-4.8-wall>=1.5, "Power strap needs space to turn beside wall");
assert(gauge_xy[0]-4.8-(power_xy[0]+power_size[0])>=1.49, "MAX strap too close to power board");
if(is_undef(open_battery_bay) || !open_battery_bay) {
    assert(battery_xy[0]-(gauge_xy[0]+gauge_size[0]+4.8)>=1.49, "MAX strap too close to holder");
    assert(case_w-wall-(battery_xy[0]+battery_size[0]+4.8)>=1.5, "Battery strap too close to wall");
}
for (i=[0:len(all_boxes)-1]) {
    assert(inside(all_boxes[i]),str("Component envelope outside shell: ",i));
    for(j=[0:len(all_boxes)-1]) if(j>i)
        assert(separated(all_boxes[i],all_boxes[j],0.3),str("Component envelopes overlap: ",i," / ",j));
}
echo(case_mm=[case_w,case_l,case_depth], front_mm=front_depth, rear_mm=back_depth);
echo(battery_to_heltec_clamp_gap_mm=rear_boxes[0][2]-(clamp_seat_z+clamp_t));
echo(case_screw_under_head_mm=case_screw_len,case_fastener=case_fastener,
    engagement_mm=case_screw_len-((!is_undef(face_entry_case_screws) && face_entry_case_screws)
        ? front_depth : (case_depth-1-front_depth)));

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
    side_slot("bottom",cyd_x+cyd_usb_x,cyd_usb_center_z,usb_open_w,usb_open_h);
    side_slot("bottom",heltec_x+heltec_w/2,heltec_usb_z,usb_open_w,usb_open_h);
    side_slot("left",power_usb_y,
        case_depth-power_usb_local_z,power_usb_w,power_usb_h);
    if(switch_enabled) side_slot("top",switch_x,switch_z,switch_open_w,switch_open_h);
    translate([sma_x,case_l+eps,sma_z]) rotate([90,0,0]) cylinder(d=sma_d,h=wall+2*eps);
}

// Raised anchors accept small zip ties without holes through the outer skin.
// The tunnel is along X; a 3.4 mm wide tie lies in the Y/Z plane.
module tie_anchor(x,y,z=skin,h=5) {
    if(!is_undef(heavy_tie_anchors) && heavy_tie_anchors) {
        // Keep the original tunnel centre and 3.4 x 1.6 mm opening. Larger
        // Y sidewalls and roof carry the tie load; a tapered root spreads it
        // into the full-thickness floor retained by the lightweight models.
        translate([x,y,z-eps]) difference() {
            union() {
                translate([0,-1,0]) rounded_block(3.2,8,h+1+eps,0.7);
                hull() {
                    translate([0,-1.5,0]) rounded_block(3.2,9,0.4,0.8);
                    translate([0,-1,0.8]) rounded_block(3.2,8,0.4,0.7);
                }
            }
            translate([-eps,(6-tie_w)/2,0.8])
                cube([3.2+2*eps,tie_w,tie_h]);
        }
    } else translate([x,y,z-eps]) difference() {
        rounded_block(3.2,6,h+eps,0.7);
        translate([-eps,(6-tie_w)/2,0.8]) cube([3.2+2*eps,tie_w,tie_h]);
    }
}
module pcb_seat(xy,size,z=module_z,anchors=true,screw_mounts=false) {
    // Four short ledges leave the centre and underside solder joints free.
    for(x=[xy[0]+1,xy[0]+size[0]-4],y=[xy[1]+1,xy[1]+size[1]-4])
        if(screw_mounts) cyl_at([x+1.5,y+1.5],skin-eps,4.2,z-skin+eps);
        else translate([x,y,skin-eps]) cube([3,3,z-skin+eps]);
    // Corner fences restrain XY without covering connector or solder edges.
    for(x=[xy[0]-board_clearance-1.2,xy[0]+size[0]+board_clearance],
        y=[xy[1]+2,xy[1]+size[1]-6])
        translate([x,y,skin-eps]) cube([1.2,4,module_rail_h+eps]);
    for(y=[xy[1]-board_clearance-1.2,xy[1]+size[1]+board_clearance])
        translate([xy[0]+size[0]/2-2,y,skin-eps]) cube([4,1.2,module_rail_h+eps]);
    if(anchors) {
        tie_anchor(xy[0]-4.8,xy[1]+size[1]/2-3);
        tie_anchor(xy[0]+size[0]+1.6,xy[1]+size[1]/2-3);
    }
}

module front_mounts() {
    for(p=cyd_pts) cyl_at(p,skin-eps,6,cyd_pcb_z-skin+eps);
    // Narrow Heltec supports touch only the outer PCB margin, not pin rows.
    for(x=[heltec_x,heltec_x+heltec_w-clamp_pad_w],y=clamp_ys)
        translate([x,y-1.4,skin-eps]) cube([clamp_pad_w,2.8,heltec_pcb_z-skin+eps]);
    for(x=clamp_xs,y=clamp_ys) {
        if(!is_undef(heavy_heltec_posts) && heavy_heltec_posts) intersection() {
            union() {
                cyl_at([x,y],skin-eps,7.2,clamp_post_top_z-skin+eps);
                translate([x,y,skin-eps]) cylinder(d1=9,d2=7.2,h=2.5);
            }
            // Flatten only the CYD-facing side, preserving its PCB/glass
            // clearance while enlarging the other sides of the screw post.
            translate([cyd_x+cyd_w+0.7,0,0]) cube([case_w,case_l,case_depth]);
        } else cyl_at([x,y],skin-eps,5.2,clamp_post_top_z-skin+eps);
    }
    gps_frame() gps_mounts();
    // Bearing ledges touch only the backing PCB margins, never the ceramic.
    for(x=[patch_xy[0]-1.5-(is_undef(patch_ledge_extra) ? patch_fit_extra : patch_ledge_extra),
           patch_xy[0]+patch_size[0]-1.2+(is_undef(patch_ledge_extra) ? patch_fit_extra : patch_ledge_extra)],y=[97,107])
        translate([x,y,skin-eps]) cube([2.7,3,patch_ledge_z-skin+eps]);
    for(x=patch_post_xs,y=patch_post_ys)
        cyl_at([x,y],skin-eps,4.2,patch_support_z-skin+eps);
}
module gps_frame() {
    translate([gps_xy[0],gps_xy[1]+gps_size[0],0]) rotate([0,0,-90]) children();
}
module gps_mounts() {
    // User confirmed the four existing support centres match PCB holes.
    for(x=[0.6,gps_size[0]-3.6],y=[0.6,gps_size[1]-3.6]) {
        if(!is_undef(gps_robust_mounts) && gps_robust_mounts) gps_robust_post(x+1.5,y+1.5);
        else if(!is_undef(gps_screw_mounts) && gps_screw_mounts) difference() {
            union() {
                translate([x,y,skin-eps]) cube([3,3,gps_board_z-skin+eps]);
                // Broaden the screw bearing face only above antenna/feed
                // relief; lower columns retain their original footprints.
                cap_z=max(patch_support_z+patch_board_t+patch_back_relief+0.4,gps_board_z-1.2);
                translate([x+1.5,y+1.5,cap_z])
                    cylinder(d1=3,d2=4.2,h=gps_board_z-cap_z);
            }
            translate([x+1.5,y+1.5,gps_board_z-6]) cylinder(d=1.6,h=6+eps);
        } else translate([x,y,skin-eps]) cube([3,3,gps_board_z-skin+eps]);
    }
    if(is_undef(gps_screw_mounts) || !gps_screw_mounts) {
    // Insert upper PCB corners under these keepers, then strap the lower end.
    // Requires a clear 0.8 mm PCB edge and measured PCB thickness.
    for(x=[-1.6,gps_size[0]+0.4])
        translate([x,gps_size[1]-3.5,skin-eps]) cube([1.2,3,gps_board_z+gps_pcb_t+1-skin+eps]);
    for(x=[-1.6,gps_size[0]-0.8])
        translate([x,gps_size[1]-3.5,gps_board_z+gps_pcb_t+0.3]) cube([2.4,3,0.7]);
    tie_anchor(-4.8,-1);
    tie_anchor(gps_size[0]+1.6,-1);
    }
}

module gps_robust_post(x,y) {
    // Local GPS coordinates converted through gps_frame as usual. Trim a
    // true antenna envelope from the columns, then close its roof at 45 deg.
    difference() {
        intersection() {
            union() {
                translate([x,y,skin-eps]) cylinder(d=6,h=gps_board_z-skin+eps);
                translate([x,y,skin-eps]) cylinder(d1=8,d2=6,h=2.5);
                // Moving the antenna toward the right wall would otherwise
                // thin the right receiver columns. Add an outboard spine,
                // stopping short of the rear mating lip's swept envelope.
                if(patch_shift_x>0 && y>gps_size[1]/2) {
                    sy=patch_xy[0]+patch_size[0]+0.3-gps_xy[0];
                    ey=case_w-wall-lip_clearance-lip_t-0.3-gps_xy[0];
                    translate([x-2.8,sy,skin-eps])
                        cube([5.6,ey-sy,gps_board_z-skin+eps]);
                }
            }
            // Do not add material into the adjacent CYD glass area.
            translate([-case_l,cyd_x+cyd_w+0.7-gps_xy[0],0]) cube([case_l*2,case_w,case_depth]);
        }
        // Antenna space is easiest to specify in assembly coordinates.
        // Inverse of gps_frame: (X,Y)->(gpsY+gpsW-Y, X-gpsX).
        cx=gps_xy[1]+gps_size[0]-(patch_xy[1]+12.5);
        cy=patch_xy[0]+12.5-gps_xy[0];
        fill_rims=!is_undef(gps_solid_screw_rims) && gps_solid_screw_rims;
        taper_rims=!is_undef(gps_taper_screw_rims) && gps_taper_screw_rims;
        relief_z=taper_rims ? patch_ledge_z+patch_size[2]+0.3
            : fill_rims ? patch_ledge_z+patch_size[2]+patch_back_relief+0.3
            : patch_support_z+patch_board_t+patch_back_relief;
        loft_h=gps_board_z-relief_z;
        translate([cx-13.5,cy-13.5,skin-eps]) cube([27,27,relief_z-skin+eps]);
        if(!fill_rims) translate([cx,cy,relief_z]) linear_extrude(loft_h,
            scale=(27-2*(loft_h+(patch_shift_x>0 ? 0.8 : 0)))/27)
            square([27,27],center=true);
        // Restore a 45-degree underside beneath the filled rims. The clear
        // antenna box continues through its full 7.5 mm height plus 0.3 mm.
        if(taper_rims) translate([cx,cy,relief_z]) linear_extrude(6,scale=15/27)
            square([27,27],center=true);
        pilot_depth=taper_rims ? 2.8 : 3.2;
        translate([x,y,gps_board_z-pilot_depth]) cylinder(d=1.6,h=pilot_depth+eps);
        if(patch_shift_x>0)
            translate([gps_xy[1]+gps_size[0]-(case_l-5),case_w-5-gps_xy[0],0])
                cylinder(d=case_boss_d+1,h=case_depth);
    }
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
        for(x=clamp_xs,y=clamp_ys) cyl_at([x,y],clamp_post_top_z-8,clamp_pilot_d,9);
        for(x=patch_post_xs,y=patch_post_ys) cyl_at([x,y],skin+1,1.6,patch_support_z);
        for(p=case_pts) {
            if(!is_undef(face_entry_case_screws) && face_entry_case_screws)
                cyl_at(p,-eps,case_clearance_d,front_depth+2*eps);
            else {
                cyl_at(p,front_depth-11,case_fastener=="heatset" ? case_clearance_d : case_pilot_d,12);
                if(case_fastener=="heatset") cyl_at(p,front_depth-insert_length,insert_d,insert_length+eps);
            }
        }
        ports();
    }
}

module under_tie_channels(xy,size,ys) {
    for(y=ys) translate([xy[0]-1,y-tie_w/2,skin+0.5])
        cube([size[0]+2,tie_w,tie_h]);
}
module assembly_seat(xy,size,ys,z=module_z,anchor_gap=0) {
    difference() {
        pcb_seat(xy,size,z,false);
        under_tie_channels(xy,size,ys);
    }
    for(y=ys) {
        tie_anchor(xy[0]-4.8-anchor_gap,y-3);
        tie_anchor(xy[0]+size[0]+1.6+anchor_gap,y-3);
    }
}
module battery_cradle() {
    assembly_seat(battery_xy,battery_size,battery_tie_ys,skin+battery_seat);
    // Wide rectangular assembly: support its perimeter, leaving wire lanes.
    difference() {
        translate([battery_xy[0],battery_xy[1],skin-eps])
            ring(battery_size[0],battery_size[1],battery_seat+eps,3,3.5);
        under_tie_channels(battery_xy,battery_size,battery_tie_ys);
    }
}
module speaker_seat() {
    // Printed ledge supports housing rim; retainer loads it from the rear.
    difference() {
        translate([speaker_xy[0],speaker_xy[1],skin-eps])
            ring(speaker_size[0],speaker_size[1],speaker_face_gap+eps,2,speaker_corner_r);
        translate([speaker_xy[0]+speaker_size[0]/2-10,speaker_xy[1]-1,skin-2*eps])
            cube([4,4,speaker_face_gap+3*eps]);
    }
    for(x=[speaker_xy[0]-1.6,speaker_xy[0]+speaker_size[0]+0.4])
        translate([x,speaker_xy[1]+speaker_size[1]/2-4,skin-eps]) cube([1.2,8,4+eps]);
    for(y=[speaker_xy[1]-1.6,speaker_xy[1]+speaker_size[1]+0.4])
        translate([speaker_xy[0]+speaker_size[0]/2-4,y,skin-eps]) cube([8,1.2,4+eps]);
    for(x=speaker_post_xs) cyl_at([x,speaker_post_y],skin-eps,5.2,speaker_post_h-skin+eps);
}
module rear_features() {
    if(is_undef(open_battery_bay) || !open_battery_bay) battery_cradle();
    // Keep mirrored strap anchors clear of the lower corner fences instead
    // of sharing an exactly coincident vertical face with those fences.
    assembly_seat(power_xy,power_size,power_tie_ys,
        anchor_gap=(!is_undef(power_usb_at_bottom) && power_usb_at_bottom) ? 0.2 : 0);
    if(!is_undef(max_screw_mounts) && max_screw_mounts)
        pcb_seat(gauge_xy,gauge_size,anchors=false,screw_mounts=true);
    else pcb_seat(gauge_xy,gauge_size);
    if(speaker_enabled) speaker_seat();
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
            if(!is_undef(face_entry_case_screws) && face_entry_case_screws) {
                // Blind pilots open at the seam, for screws through the face.
                // Twelve millimetres leaves a closed, continuous rear skin.
                cyl_at(p,front_depth-eps,case_pilot_d,12+eps);
            } else {
                cyl_at(p,front_depth-eps,case_clearance_d,back_depth+2*eps);
                cyl_at(p,case_depth-1,6.6,1+eps);
            }
        }
        // Clamp end caps enter the lip zone; give them clearance at the seam.
        for(x=clamp_xs,y=clamp_ys)
            cyl_at([x,y],front_depth-lip_h-eps,8.4,clamp_seat_z+clamp_t-front_depth+lip_h+1);
        ports();
        // MAX blind pilots extend into retained solid skin, never through
        // the exterior. Centres match the user's confirmed support pattern.
        if(!is_undef(max_screw_mounts) && max_screw_mounts) rear_local()
            for(x=[gauge_xy[0]+2.5,gauge_xy[0]+gauge_size[0]-2.5],
                y=[gauge_xy[1]+2.5,gauge_xy[1]+gauge_size[1]-2.5])
                cyl_at([x,y],module_z-3.2,1.6,3.2+eps);
        if(speaker_enabled) rear_local() for(x=speaker_post_xs)
            cyl_at([x,speaker_post_y],1,1.6,speaker_post_h+1);
        if(speaker_enabled) for(x=[-14,-10,-6,-2,2,6,10,14])
            translate([speaker_xy[0]+speaker_size[0]/2+x-0.7,speaker_xy[1]+5,case_depth-skin-eps])
                rounded_block(1.4,speaker_size[1]-10,skin+2*eps,0.6);
    }
}
module clamp_bar() {
    span=clamp_xs[1]-clamp_xs[0];
    pad_bottom=heltec_pcb_z+heltec_pcb_t+clamp_pad_gap;
    difference() {
        union() {
            translate([clamp_xs[0]-3,-2,clamp_seat_z]) rounded_block(span+6,4,clamp_t,1.5);
            for(x=clamp_xs) cyl_at([x,0],clamp_seat_z,6.8,clamp_t);
            for(x=[heltec_x-clamp_pad_brace,heltec_x+heltec_w-clamp_pad_w])
                translate([x,-1.4,pad_bottom]) cube([clamp_pad_w+clamp_pad_brace,2.8,clamp_seat_z-pad_bottom+eps]);
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
module countersink_m2(p,z,t) {
    cyl_at(p,z-eps,2.3,t+2*eps);
    translate([p[0],p[1],z+t-0.85]) cylinder(d1=2.3,d2=4,h=0.85+eps);
}
module antenna_rail(side=0) {
    ledge_extra=is_undef(patch_ledge_extra) ? patch_fit_extra : patch_ledge_extra;
    xx=side==0 ? patch_xy[0]-1.5-ledge_extra : patch_xy[0]+patch_size[0]-1.2+ledge_extra;
    px=patch_post_xs[side];
    difference() {
        union() {
            translate([xx,95.5,patch_retainer_z]) rounded_block(2.7,16,patch_retainer_t,0.7);
            for(y=patch_post_ys) cyl_at([px,y],patch_retainer_z,5.4,patch_retainer_t);
            if(!is_undef(patch_ledge_extra)) for(y=patch_post_ys)
                translate([min(px,xx),y-1.3,patch_retainer_z])
                    cube([max(px,xx+2.7)-min(px,xx),2.6,patch_retainer_t]);
        }
        for(y=patch_post_ys) countersink_m2([px,y],patch_retainer_z,patch_retainer_t);
        // Coax relief under the left rail; its upper web remains connected.
        if(side==0) translate([xx-eps,101.5,patch_retainer_z-eps]) cube([2.7+2*eps,4,1.4]);
    }
}
module antenna_retainers_print() {
    for(side=[0,1]) translate([patch_post_xs[side]+3,side*24,patch_retainer_z+patch_retainer_t])
        rotate([0,180,0]) translate([0,-95.5,0]) antenna_rail(side);
}
// Rear-local coordinates, sound face at low Z and magnet projecting up.
module speaker_retainer() {
    pad_z=skin+speaker_face_gap+speaker_rim_t;
    difference() {
        union() {
            translate([speaker_xy[0]-2.5,speaker_xy[1]-2.5,speaker_retainer_z])
                ring(speaker_size[0]+5,speaker_size[1]+5,speaker_retainer_t,4.5,speaker_corner_r+2.5);
            for(x=speaker_post_xs) cyl_at([x,speaker_post_y],speaker_retainer_z,6,speaker_retainer_t);
            for(x=[speaker_xy[0]+speaker_size[0]/2-5,speaker_xy[0]+speaker_size[0]/2+3],
                y=[speaker_xy[1]+0.8,speaker_xy[1]+speaker_size[1]-1.6])
                translate([x,y,pad_z]) cube([2,0.8,speaker_retainer_z-pad_z+eps]);
        }
        for(x=speaker_post_xs) countersink_m2([x,speaker_post_y],speaker_retainer_z,speaker_retainer_t);
    }
}
module speaker_retainer_print() {
    translate([speaker_xy[0]+speaker_size[0]+6,-speaker_xy[1]+2.5,speaker_retainer_z+speaker_retainer_t])
        rotate([0,180,0]) speaker_retainer();
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
    color([0.16,0.45,0.85,0.5]) for(i=[1:len(rear_boxes)-1]) box_envelope(rear_boxes[i]);
    color([0.2,0.6,0.85,0.6]) box_envelope(rear_boxes[0]);
    if(switch_enabled) color([0.2,0.2,0.2,0.5])
        translate([switch_x-switch_body[0]/2,case_l-wall-switch_body[1],switch_z-switch_body[2]/2]) cube(switch_body);
    if(speaker_enabled) color([0.2,0.2,0.2,0.5]) box_envelope(speaker_box);
}
module ghosts() { front_ghosts(); rear_ghosts(); }
module assembly(gap=0) {
    color([0.60,0.66,0.72]) front();
    for(y=clamp_ys) color([0.95,0.57,0.15]) translate([0,y,gap/3]) clamp_bar();
    for(side=[0,1]) color([0.95,0.57,0.15]) translate([0,0,gap/4]) antenna_rail(side);
    translate([0,0,gap]) color([0.32,0.39,0.46]) back();
    if(speaker_enabled) translate([0,0,gap]) color([0.95,0.57,0.15]) rear_local() speaker_retainer();
    if($preview && show_components) {
        // Move only rear-mounted hardware with its half in exploded views.
        front_ghosts();
        translate([0,0,gap]) rear_ghosts();
    }
}
// Shallow trays retain the production floor, seats and full-height mounts.
// Only the perimeter wall / corner pillars above 7 mm are removed.
// These check each half independently, not closed-case depth or upper ports.
module fit_tray(rear=false) {
    intersection() {
        if(rear) rear_frame() back(); else front();
        union() {
            cube([case_w,case_l,7]);
            translate([wall+1,wall+1,0])
                cube([case_w-2*(wall+1),case_l-2*(wall+1),case_depth]);
        }
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

if(part=="fit_front") fit_tray();
else if(part=="fit_back") fit_tray(true);
else if(part=="front") color([0.60,0.66,0.72]) front();
else if(part=="back") color([0.60,0.66,0.72]) rear_frame() back();
else if(part=="clamps") clamps_print();
else if(part=="antenna_retainers") antenna_retainers_print();
else if(part=="speaker_retainer") speaker_retainer_print();
else if(part=="fit_coupon") fit_coupon();
else if(part=="components") { if($preview) ghosts(); }
else if(part=="assembled") assembly();
else if(part=="exploded") assembly(explosion);
else if(part=="interference") intersection() {
    union() { front(); for(y=clamp_ys) translate([0,y,0]) clamp_bar(); for(side=[0,1]) antenna_rail(side); }
    // Lift 0.05 mm to avoid reporting zero-thickness seam contact as a mesh.
    translate([0,0,0.05]) union() { back(); if(speaker_enabled) rear_local() speaker_retainer(); }
}
else if(part=="print_layout") {
    front();
    translate([case_w+10,0,0]) rear_frame() back();
    translate([0,case_l+10,0]) clamps_print();
    translate([60,case_l+10,0]) antenna_retainers_print();
    translate([case_w+10,case_l+10,0]) speaker_retainer_print();
} else assert(false,str("Unknown part: ",part));
