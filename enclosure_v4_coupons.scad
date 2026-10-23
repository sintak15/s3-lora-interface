// Suppress included assembly output; this file opens directly in OpenSCAD.
coupon_mode = true;
include <enclosure_v4.scad>
coupon = "cyd_mounts"; // [cyd_mounts,cyd_usb,heltec,heltec_screen,heltec_clamp,power,battery,max]
lightweight = false;

// Preserve production datums; remove the broad floor and unnecessary walls.
module cyd_mount_coupon() {
    x=cyd_x+cyd_hole_x; y=cyd_y+cyd_hole_y;
    translate([-x+3,-y+3,0]) difference() {
        union() {
            for(yy=[y,y+cyd_hole_pitch_y])
                translate([x-3,yy-(lightweight ? 1.5 : 3),0]) cube([cyd_hole_pitch_x+6,lightweight ? 3 : 6,lightweight ? 0.8 : 1.2]);
            for(xx=[x,x+cyd_hole_pitch_x])
                translate([xx-(lightweight ? 1.5 : 3),y-3,0]) cube([lightweight ? 3 : 6,cyd_hole_pitch_y+6,lightweight ? 0.8 : 1.2]);
            for(p=cyd_pts) cyl_at(p,0,6,cyd_pcb_z);
        }
        // Same blind pilots and support height as the production shell.
        for(p=cyd_pts) cyl_at(p,skin+0.5,cyd_mount_pilot_d,cyd_pcb_z);
    }
}

module heltec_coupon() {
    // One real clamp station, both edge seats and both screw posts.
    x=clamp_xs[0]-3; y=clamp_ys[0]-3;
    translate([-x,-y,0]) intersection() {
        union() {
            if(lightweight) {
                translate([x,y+1.5,0]) cube([clamp_xs[1]-clamp_xs[0]+6,3,skin]);
                for(xx=clamp_xs) cyl_at([xx,clamp_ys[0]],0,5.2,skin);
            } else translate([x,y,0]) cube([clamp_xs[1]-clamp_xs[0]+6,6,skin]);
            difference() {
                front_mounts();
                for(xx=clamp_xs) cyl_at([xx,clamp_ys[0]],clamp_seat_z-8,clamp_pilot_d,9);
            }
        }
        translate([x,y,0]) cube([clamp_xs[1]-clamp_xs[0]+6,6,clamp_seat_z]);
    }
}

module width_coupon(xy,size,seat=module_z,battery=false) {
    // First 10 mm only: actual end fences, corner seats and cradle ledge.
    // Tests width/edge seating, NOT full length or straps.
    origin=[xy[0]-2,xy[1]-2,0];
    translate(-origin) intersection() {
        union() {
            translate(origin) difference() {
                cube([size[0]+4,lightweight ? 8 : 12,skin]);
                // Retain the end fence strip and material under both corner seats.
                if(lightweight && size[0]>14)
                    translate([7,2,-eps]) cube([size[0]-10,7,skin+2*eps]);
            }
            if(battery) battery_cradle();
            else pcb_seat(xy,size,seat,false);
        }
        translate(origin) cube([size[0]+4,lightweight ? 8 : 12,10]);
    }
}

module heltec_screen_coupon() {
    x=clamp_xs[0]-3;
    y=heltec_y-1;
    wx=heltec_x+(heltec_w-heltec_window_w)/2;
    wy=heltec_y+heltec_window_y;
    // Intersect the actual front shell, including its exact rounded opening,
    // both support stations and pilots, with a lightweight connected skeleton.
    translate([-x,-y,0]) intersection() {
        front();
        union() {
            translate([heltec_x-1,y,0]) difference() {
                cube([heltec_w+2,heltec_l+2,clamp_seat_z]);
                translate([2,2,-eps]) cube([heltec_w-2,heltec_l-2,clamp_seat_z+2*eps]);
            }
            // Keep the whole window region in the mask so the production
            // shell alone defines the 1 mm rounded corners of the opening.
            translate([wx-3,wy-3,0])
                cube([heltec_window_w+6,heltec_window_l+6,clamp_seat_z]);
            for(yy=clamp_ys)
                translate([x,yy-3,0]) cube([clamp_xs[1]-clamp_xs[0]+6,6,clamp_seat_z]);
        }
    }
}

if(coupon=="cyd_mounts") cyd_mount_coupon();
else if(coupon=="cyd_usb") translate([-cyd_x+2,0,0]) intersection() {
    front();
    translate([cyd_x-2,0,0]) cube([cyd_w+4,cyd_y+10,front_depth]);
    if(lightweight) union() {
        translate([cyd_x-2,0,0]) cube([cyd_w+4,wall+1,front_depth]);
        for(xx=[cyd_x+cyd_hole_x,cyd_x+cyd_hole_x+cyd_hole_pitch_x])
            translate([xx-3,0,0]) cube([6,cyd_y+10,front_depth]);
    }
}
else if(coupon=="heltec") heltec_coupon();
else if(coupon=="heltec_screen") heltec_screen_coupon();
else if(coupon=="heltec_clamp")
    translate([clamp_xs[1]+3.4,3.4,clamp_seat_z+clamp_t]) rotate([0,180,0]) clamp_bar();
else if(coupon=="power") width_coupon(power_xy,power_size);
else if(coupon=="battery") width_coupon(battery_xy,battery_size,skin+battery_seat,true);
else if(coupon=="max") width_coupon(gauge_xy,gauge_size);
else assert(false,"Unknown coupon");
