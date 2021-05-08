include </home/hci-haptics-lab/.server/django/scrappy/media/parameters/Can_Generator_params.scad>;

can_Height = 38; //[10:0.1:200]
can_Diameter = 85.8; //[20:0.1:200]
can_Thickness = 0.6; //[0.1:0.1:2]

can_Top_Rim_Thickness = 1.2; //[0:0.1:4]
can_Top_Rim_Height = 3.1; //[0:0.1:10]

can_Bot_Rim_Thickness = 0; //[0:0.1:4]
can_Bot_Rim_Height = 0; //[0:0.1:10]

can_Bot_Indent_Depth = 2.2; //[0:0.1:20]
can_Bot_Indent_Diameter = 74.5; //[10:0.1:200]

empty = true;


$fs = 1;
$fa = 4;

difference() {
    union() {
        cylinder(d=can_Diameter, h=can_Height);
        
        translate([0,0,can_Height - can_Top_Rim_Height])
        cylinder(d=((can_Top_Rim_Thickness - can_Thickness)*2 + can_Diameter), h=can_Top_Rim_Height);
        
        cylinder(d=((can_Bot_Rim_Thickness - can_Thickness)*2 + can_Diameter), h=can_Bot_Rim_Height);
    }
    if(empty) {
        translate([0,0,can_Thickness + can_Bot_Indent_Depth])
        cylinder(d=can_Diameter - 2*can_Thickness, h=can_Height);
    }
    translate([0,0,-0.01])
    cylinder(d=can_Bot_Indent_Diameter, h=can_Bot_Indent_Depth+0.01);
}
