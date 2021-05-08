can_Height = 157.8; //[10:0.1:200]
can_Diameter = 67.4; //[20:0.1:200]
can_Thickness = 0.8; //[0.1:0.1:2]

can_foot_Thinning_Height = 5.0; //[0:0.1:4]
can_foot_Thinning_Diameter = 52.2; //[0:0.1:10]

can_foot_Rim_Height = 2.2; //[0:0.1:4]
can_foot_Rim_Thickness = 3.6; //[0:0.1:6]

can_top_Thinning_Combined_Height = 16; //[0:0.1:30]
can_top_Thinning_Diameter = 52.8; //[0:0.1:100]
can_top_Rim_Diameter = 53.8; //[0:0.1:100]
can_top_Rim_Thickness = 1.4; //[0:0.1:4]
can_top_Rim_Height = 2.4; //[0:0.1:6]

empty = true;

padding = 1;


$fs = 1;
$fa = 4;

union() {
    difference() {
        cylinder(d=can_foot_Thinning_Diameter, h=can_foot_Rim_Height);
        translate([0,0,-padding])
        cylinder(d=can_foot_Thinning_Diameter-2*can_foot_Rim_Thickness, h=can_foot_Rim_Height+2*padding);
    }
    
    translate([0,0,can_foot_Rim_Height])
    cylinder(d1=can_foot_Thinning_Diameter, d2=can_Diameter, h=can_foot_Thinning_Height);
    
    translate([0,0,can_foot_Rim_Height + can_foot_Thinning_Height])
    cylinder(d=can_Diameter, h=can_Height-can_foot_Rim_Height-can_foot_Thinning_Height-can_top_Thinning_Combined_Height);
    
    translate([0,0,can_Height-can_top_Thinning_Combined_Height])
    cylinder(d1=can_Diameter, d2=can_top_Thinning_Diameter, h=can_top_Thinning_Combined_Height-can_top_Rim_Height);
    
    difference() {
        translate([0,0,can_Height-can_top_Rim_Height])
        cylinder(d=can_top_Rim_Diameter, h=can_top_Rim_Height);
        translate([0,0,can_Height-can_top_Rim_Height-padding])
        cylinder(d=can_top_Rim_Diameter-2*can_top_Rim_Thickness, h=can_top_Rim_Height+2*padding);
    }
}