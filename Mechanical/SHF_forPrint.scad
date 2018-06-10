/* 
SHF-inspired shaft clamp for 3D print
While not accurate to SHF dimensions, this design is meant to provide a working prototype
similar in dimensions, for quick and cheap 3D printed clamps, using a fastening bolt + hex nut.
Print with 40% 3D honeycomb infill, no supports. PETG / ABS / Nylon preferred.
Part of the Tannerino project: https://hackaday.io/project/133879-tannerino
Designed by Uri Shani, usdogi_at_gmail.com June 10th, 2018. Published under CC0-1.0: https://creativecommons.org/publicdomain/zero/1.0/
Disclaimer:
This design is not intended for high loads, and is assumed to be used as placement aid only.
Author assumes no liability, and may not be held responsible for misuse of the design.
*/ 

//Change this section
basicDiameter = 8; // held shaft diameter
postHeight = 20; // held shaft post height
screwHole = 4; //fastening screws diameter
chamferedScrew = true; //are fastening screws countersunk?
clampingBolt = 3; //clamping bolt diameter. creates slot + countersink
hexNutWidthAcrossFlats = 6; //for countersunk hex nut
hexNutThickness = 2.5; //for countersunk hex nut

//Do not change unless you know what you are doing!
rad1 = (basicDiameter + 0.25) / 2;
rad2 = postHeight / 2;
keyHeight = rad1 * 10;
earLength = rad2 * 4;
earHeight = postHeight / 2; 
wingtipA = rad2 + 0.1;
screwRad = (screwHole / 2) + 0.5;
clampingBoltDrillSlotRad = (clampingBolt / 2) + 0.2;
hexNutCirRad = ((hexNutWidthAcrossFlats * (2 / sqrt(3))) + 0.2) / 2;
hexNutDepth = (postHeight*0.75)/2 - (hexNutThickness * 0.75);
boltCounterSink = (postHeight*0.75)/2;

use <MCAD/triangles.scad>

module regular_polygon(order, rad){
        angles=[ for (i = [0:order-1]) i*(360/order) ];
        coords=[ for (th=angles) [rad*cos(th), rad*sin(th)] ];
        polygon(coords);
 }

difference() {
    union() {
        //main shaft post
        cylinder(h = postHeight, r = rad2);
        //clamping bolt extension
        translate([-(postHeight-(postHeight*0.25))/2, -postHeight*0.6, 0]) 
          cube([postHeight*0.75, postHeight/2, postHeight]);
        //clamping ears and chamfers
        difference() {
            difference() {
                translate([0, 0, earHeight / 2]) 
                  cube([earLength, rad2 * 2, earHeight], true);
                union() {
                    for(i = [1:4]) {
                        j = (i % 2) ? 1:-1;
                        k = (i % 2) ? 1:0;
                        m = (i % 2) ? i:(i-1);
                        //echo(i, j, k, m);
                        mirror([k, 0, 0]) rotate([0, 0, m * 90])
                        translate([-j * wingtipA, -j * earLength, 0])
                          triangle(o_len = j * earLength, a_len = j * wingtipA, depth = earHeight);
                     }
                  }
             }
            union () {
                //clamping screws section
                for (n = [0:1]) {
                    mirror([n, 0, 0]) {
                        translate([rad2*1.5, 0, 0]) {
                             majorRad = chamferedScrew ? (screwRad*2):screwRad;
                           translate([0, 0, earHeight-majorRad]) 
                             cylinder(h = majorRad, r1 = 0, r2 = majorRad, center = false);
                           cylinder(r = screwRad, h = earHeight);
                        }
                    }
                }
            }
        }
    }
    union() {
        //keyhole for main shaft and triangular slot for tightening
        cylinder(r = rad1, h = postHeight, $fn = 20);
        translate(v = [0, -((keyHeight) - rad1), 0]) {
            union() {
                triangle(o_len = keyHeight, a_len = rad1 / 2, depth = postHeight);
                mirror([1, 0, 0]) {
                    triangle(o_len = keyHeight, a_len = rad1 / 2, depth = postHeight);
                }
              }
            }
        }
    
    //clamping bolt section    
    translate([0, -rad2 + hexNutCirRad/2, earHeight]) rotate([0, 90, 0]) {
      union() {
          //clamping bolt shaft slot
          cube([clampingBoltDrillSlotRad,clampingBoltDrillSlotRad,earLength],center = true);
          for(s = [0:1])
              mirror([0, s, 0])
                translate ([0,clampingBoltDrillSlotRad/3,0]) 
                  cylinder(d = clampingBoltDrillSlotRad, h = earLength, center = true, $fn = 20);
      }
      for (t = [0:1])
        mirror([0, 0, t]) {
            //hex nut countersink
            translate([0, 0, hexNutDepth])
            linear_extrude(h = earLength, center = false)
            regular_polygon(order = 6, rad = hexNutCirRad);
            //clamping bolt head countersink
            translate([0, 0, boltCounterSink])
              cylinder(r = hexNutCirRad + 0.5, h = earLength, center = false);
        }
    }
}