lid_Diameter = 104.75;
lid_Height = 5.3;

lid_Rim_Width = 1.6;
lid_Rim_Depth = 4.75;

ridge_Distance_From_Edge = 1.2;
ridge_Width = 4.75;
ridge_Height = 6.1;
ridge_Height_Difference = ridge_Height - lid_Height;

indent_depth = 0.8;

$fs = 1;
$fa = 4;
lid_3D();

module lid_3D() {
    
    rotate_extrude() 
    lid_2D();
}

// 2D
module lid_2D() {
    difference() {
        union() {
            square([lid_Diameter/2, lid_Height]);
            
            ridge();
        }
        square([lid_Diameter/2 - lid_Rim_Width, lid_Rim_Depth]);
        indent();
    }
}

module ridge(){
    translate([0, 0])
    square([lid_Diameter/2 - ridge_Distance_From_Edge, ridge_Height]);
};

module indent() {
    translate([0, ridge_Height - indent_depth])
    square([lid_Diameter/2 - ridge_Distance_From_Edge - ridge_Width, ridge_Height]);
};