include </home/hci-haptics-lab/.server/django/scrappy/media/parameters/Coffee_Cup_Params.scad>;

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
        if (!cup_Empty) {
            
            translate([0,0,cup_Height/2 - cup_Foot_Height/2 + cup_Foot_Height + cup_Thickness/2])
            cube(size=[cup_Top_Inner_Diameter, cup_Top_Inner_Diameter, cup_Height - cup_Foot_Height], center=true);
            
        }
        
        if (!cup_Empty_Bottom) {
            
            translate([0,0,cup_Foot_Height/2])
            cube(size=[cup_Top_Inner_Diameter, cup_Top_Inner_Diameter,cup_Foot_Height], center=true);
            
        }
    }
    if (cup_Empty) {
        translate([0,0,cup_Height])
        cube(size=[cup_Top_Inner_Diameter+cup_Thickness*2+cup_Lip_Diameter*2,cup_Top_Inner_Diameter+cup_Thickness*2+cup_Lip_Diameter*2,cup_Lip_Diameter], center = true);
    }
}

if (!cup_Empty) {
    translate([0,0,cup_Height-cup_Lip_Diameter/2])
    cylinder(d = cup_Top_Inner_Diameter + cup_Lip_Diameter, h = cup_Lip_Diameter/2);
    echo(cup_Top_Inner_Diameter);
}

rotate_extrude() {
    translate([0, cup_Foot_Height])
    square([cup_Foot_Diameter, cup_Thickness]);
    translate([cup_Lip_Distance + cup_Lip_Diameter/2, cup_Height - cup_Lip_Diameter / 2])
    //square([cup_Lip_Diameter,cup_Lip_Diameter]);
    circle(d = cup_Lip_Diameter);
}