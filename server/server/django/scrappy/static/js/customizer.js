function cameraButton() {
    console.log($(this).attr('id'));
    var cameraDirection = $(this).attr('id');
    console.log(imgLocationVar);

    $.ajax({
        data: {
            'cameraDirection': cameraDirection,
            'getType' : 'customizer_camera_click'
        },  
        dataType: 'json',
        
        success: function (data) {
            $('#preview_'+data.customizerClassId).attr('src', imgLocationVar + data.preview_image + ".png");
        }
    });    
}

function imageDeleteButton() {
    console.log($(this).attr('id'));
    imageName = $(this).attr('name');

    if (imageName == 'scrap_image_holder') {
        $.ajax({
            data: {
                'imageVal' : null,
                'getType' : 'customizer_image',
                'delete' : true,
                'imageName' : imageName
            },  
            dataType: 'json',
            
            success: function (data) {
                if (data.successful) {
                    console.log("Hide it");
                    $('#scrap_image_holder_button_'+data.customizerClassId).hide();
                    $('#scrap_image_'+data.customizerClassId).hide();
                }
            }
        });  
    }
    else if (imageName == 'preview_image') {
        $.ajax({
            data: {
                'imageVal' : null,
                'getType' : 'scrap_detail_page_image',
                'delete' : true,
                'imageName' : imageName
            },  
            dataType: 'json',
            
            success: function (data) {
                if (data.successful) {
                    console.log("Hide it");
                    $('#preview_image_button_'+data.scrapObjectId).hide();
                    $('#preview_image_'+data.scrapObjectId).hide();
                }
            }
        });  
    }
    else if (imageName == 'storage_picture') {
        $.ajax({
            data: {
                'imageVal' : null,
                'getType' : 'scrap_detail_page_image',
                'delete' : true,
                'imageName' : imageName
            },  
            dataType: 'json',
            
            success: function (data) {
                if (data.successful) {
                    console.log("Hide it");
                    $('#storage_picture_button_'+data.scrapObjectId).hide();
                    $('#storage_picture_'+data.scrapObjectId).hide();
                }
            }
        });  
    }
}