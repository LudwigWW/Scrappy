box_Bot_Width = 150; //[0:0.1:250]
box_Bot_Length = 200; //[0:0.1:250]
box_Front_Height = 50; //[0:0.1:250]

box_Top_Width = 0; //[0:0.1:250]
box_Top_Length = 0; //[0:0.1:250]
box_Back_Height = 0; //[0:0.1:250]

box_Rounded_Corner_Diameter = 0; //[0:0.1:50]

box_Rounded_Corner_Diam = 0.00002 + box_Rounded_Corner_Diameter;


if (box_Top_Width == 0) {
    box_Top_WidthX = box_Bot_Width;
}

if (box_Top_Length == 0) {
    box_Top_LengthX = box_Bot_Length;
}

if (box_Back_Height == 0) {
    box_Back_HeightX = box_Front_Height;
}

hull() {
    
    if (box_Top_Width == 0 && box_Top_Length == 0 && box_Back_Height == 0) {
        // BOT
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        
        //TOP
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
    }
    else if (box_Top_Width == 0 && box_Top_Length == 0) {
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, -box_Back_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, -box_Back_Height/2], box_Rounded_Corner_Diam);
        
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, box_Back_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, box_Back_Height/2], box_Rounded_Corner_Diam);
    }
    else if (box_Top_Width == 0 && box_Back_Height == 0) {
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        
        edge_Ball([-box_Bot_Width/2, -box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
    }
    else if (box_Top_Length == 0 && box_Back_Height == 0) {
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        
        edge_Ball([-box_Top_Width/2, -box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Top_Width/2, -box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Top_Width/2, box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Top_Width/2, box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
    }
    else if (box_Top_Width == 0) {
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, -box_Back_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, -box_Back_Height/2], box_Rounded_Corner_Diam);
        
        edge_Ball([-box_Bot_Width/2, -box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Top_Length/2, box_Back_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Top_Length/2, box_Back_Height/2], box_Rounded_Corner_Diam);
    }
    else if (box_Top_Length == 0) {
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, -box_Back_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, -box_Back_Height/2], box_Rounded_Corner_Diam);
        
        edge_Ball([-box_Top_Width/2, -box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Top_Width/2, -box_Bot_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Top_Width/2, box_Bot_Length/2, box_Back_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Top_Width/2, box_Bot_Length/2, box_Back_Height/2], box_Rounded_Corner_Diam);
    }
    else if (box_Back_Height == 0) {
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        
        edge_Ball([-box_Top_Width/2, -box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Top_Width/2, -box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Top_Width/2, box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Top_Width/2, box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
    }
    else {
        edge_Ball([-box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, -box_Bot_Length/2, -box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Bot_Width/2, box_Bot_Length/2, -box_Back_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Bot_Width/2, box_Bot_Length/2, -box_Back_Height/2], box_Rounded_Corner_Diam);
        
        edge_Ball([-box_Top_Width/2, -box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Top_Width/2, -box_Top_Length/2, box_Front_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([box_Top_Width/2, box_Top_Length/2, box_Back_Height/2], box_Rounded_Corner_Diam);
        edge_Ball([-box_Top_Width/2, box_Top_Length/2, box_Back_Height/2], box_Rounded_Corner_Diam);
    }
}

module edge_Ball(pos, diam) {
    $fs = 1;
    $fa = 4;
    translate(pos)
    sphere(d=diam);
}