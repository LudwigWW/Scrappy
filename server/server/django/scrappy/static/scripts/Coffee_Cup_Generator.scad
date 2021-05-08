cup_Bot_Outer_Diameter = 63.9; //[40:0.1:150]
cup_Top_Inner_Diameter = 83.9; //[40:0.1:150]
cup_Height = 149.3; //[50:0.1:200]
cup_Thickness = 1.3; //[0.1:0.1:5]

cup_Lip_Diameter = 3.6; //[0:0.1:8]

cup_Foot_Height = 11.5; //[0:0.1:80]

empty = false;

cup_Bot_Outer_Radius = cup_Bot_Outer_Diameter / 2;
cup_Bot_Inner_Radius = cup_Bot_Outer_Radius - cup_Thickness;
cup_Top_Inner_Radius = cup_Top_Inner_Diameter / 2;
cup_Top_Outer_Radius = cup_Top_Inner_Radius + cup_Thickness;

cup_Foot_Height_Percentage = cup_Foot_Height / cup_Height;
cup_Foot_Diameter = (cup_Foot_Height_Percentage * cup_Top_Outer_Radius) + ((1 - cup_Foot_Height_Percentage) * cup_Bot_Outer_Radius);

cup_Lip_Height_Percentage = (cup_Lip_Diameter/2) / (cup_Height);
cup_Lip_Distance = ((1 - cup_Lip_Height_Percentage) * cup_Top_Inner_Radius) + ((cup_Lip_Height_Percentage) * cup_Bot_Inner_Radius);

$fs = 1;
$fa = 4;
difference() {
    cylinder(r1=cup_Bot_Outer_Radius, r2=cup_Top_Outer_Radius, h=cup_Height);
    translate([0,0,-0.01])
    
    difference() {
        cylinder(r1=cup_Bot_Inner_Radius, r2=cup_Top_Inner_Radius, h=cup_Height + 0.02);
        if (empty) {
            
            translate([0,0,cup_Height * 2 + cup_Foot_Height + cup_Thickness/2])
            cube(size=[cup_Top_Inner_Diameter * 4, cup_Top_Inner_Diameter * 4, cup_Height * 4], center=true);
            
        }
    }
    if (!empty) {
        translate([0,0,cup_Height + 500 - cup_Lip_Diameter / 2])
        cube(size=[1000,1000,1000], center = true);
    }
}

rotate_extrude() {
    translate([0, cup_Foot_Height])
    square([cup_Foot_Diameter, cup_Thickness]);
    translate([cup_Lip_Distance + cup_Lip_Diameter/2, cup_Height - cup_Lip_Diameter / 2])
    //square([cup_Lip_Diameter,cup_Lip_Diameter]);
    circle(d = cup_Lip_Diameter);
}