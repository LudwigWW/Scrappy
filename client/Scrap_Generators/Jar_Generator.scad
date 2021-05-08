jar_Width = 20;
jar_Body_Height = 40;
jar_Top_Round_Radius = 5;
jar_Bot_Round_Radius = 2;

jar_Neck_Diameter = 10;
jar_Neck_Height = 5;

jar_Cap_Diameter = 20;
jar_Cap_Height = 10;


jar();

module jar() {
    $fs = 1;
    $fa = 4;
    rotate_extrude()
    union() {
        difference() {
            translate([-jar_Width/2, 0])
            rounded_square(jar_Width, jar_Body_Height, jar_Top_Round_Radius, jar_Bot_Round_Radius);
            translate([-jar_Width, 0])
            square(size=[jar_Width,jar_Body_Height]);
        }
        
        if (jar_Neck_Height && jar_Neck_Diameter) neck();
        
        if (jar_Cap_Height && jar_Cap_Diameter) cap();
    }
}

module neck() {
    translate([0,jar_Body_Height])
    square([jar_Neck_Diameter/2, jar_Neck_Height]);
}

module cap() {
    translate([0,jar_Body_Height + jar_Neck_Height])
    square([jar_Cap_Diameter/2, jar_Cap_Height]);
}

module rounded_square( width, height, bot_corner, top_corner ) {
    $fs = 0.5;
    $fa = 5;
	hull() {
		translate( [top_corner, top_corner, 0] ) circle( top_corner );
		translate( [bot_corner, height - bot_corner, 0 ] ) circle( bot_corner );
		translate( [width - top_corner, top_corner, 0] ) circle( top_corner );
		translate( [width - bot_corner, height - bot_corner, 0] ) circle( bot_corner );
	}
}