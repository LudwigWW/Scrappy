cup_Bot_Outer_Diameter = 131; //[20:0.1:200]
cup_Top_Inner_Diameter = 143.8; //[20:0.1:200]
cup_Height = 81; //[20:0.1:2500]
cup_Thickness = 0.8; //[0.1:0.1:3]

cup_Rim_Height = 2.2; //[0:0.1:20]
cup_Rim_Width = 4.9; //[0:0.1:20]

cup_Ridge_Height_From_Top = 5.4; //[0:0.1:40]
cup_Ridge_Inner_Diameter = 147.3; //[0:0.1:200]

cup_Bottom_Hole_Diameter = 64.6; //[0:0.1:200]
cup_Bottom_Hole_Depth = 1.3; //[0:0.1:20]

empty = true;


cup_Bot_Outer_Radius = cup_Bot_Outer_Diameter / 2;
cup_Bot_Inner_Radius = cup_Bot_Outer_Radius - cup_Thickness;
cup_Top_Inner_Radius = cup_Top_Inner_Diameter / 2;
cup_Top_Outer_Radius = cup_Top_Inner_Radius + cup_Thickness;

$fs = 1;
$fa = 4;
if(true) {
    difference() {
        union() {
            cylinder(r1=cup_Bot_Outer_Radius, r2=cup_Top_Outer_Radius, h=cup_Height - cup_Ridge_Height_From_Top);
            
            translate([0,0,cup_Height - cup_Ridge_Height_From_Top])
            cylinder(r=(cup_Ridge_Inner_Diameter/2) + cup_Thickness, h=cup_Ridge_Height_From_Top);
            
            translate([0,0,cup_Height - cup_Rim_Height])
            cylinder(r=(cup_Ridge_Inner_Diameter/2) + cup_Rim_Width, h=cup_Rim_Height);
        }
        if(empty) {
            difference() {
                translate([0,0,cup_Thickness])
                cylinder(r1=cup_Bot_Inner_Radius, r2=cup_Top_Inner_Radius, h=cup_Height - cup_Ridge_Height_From_Top + 0.01);
                cylinder(d=cup_Bottom_Hole_Diameter + cup_Thickness, h=cup_Bottom_Hole_Depth + cup_Thickness);
            }
            
            translate([0,0,cup_Height - cup_Ridge_Height_From_Top + cup_Thickness])
            cylinder(d=cup_Ridge_Inner_Diameter, h=cup_Ridge_Height_From_Top);
        }
        
        translate([0,0,-0.01])
        cylinder(d=cup_Bottom_Hole_Diameter, h=cup_Bottom_Hole_Depth+0.01);
    }
}