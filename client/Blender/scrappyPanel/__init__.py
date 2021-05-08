bl_info = {
    "name": "ScrappyPanel",
    "author": "Ludwig Wilhelm Wall",
    "version": (1, 0, 0),
    "blender": (2, 80, 0),
    "location": "View3D",
    "description": "reducing infill by including scrap material in objects",
    "category": "Research Prototype",
}

import bpy
import gpu
import bgl
from gpu_extras.batch import batch_for_shader



class HelloWorldPanel(bpy.types.Panel):
    """Creates a Panel in the Object properties window"""
    bl_label = "Scrappy Panel"
    bl_idname = "OBJECT_PT_hello"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"
    

    # def draw(self, context):
        # layout = self.layout

        # row = layout.row()
        # NOTE: works only at 1:1 panel zoom!
        # split = row.split(factor=(110 / (context.region.width - 45)))
        
        # split.template_preview(bpy.data.textures['.hidden'], show_buttons=False)
    def draw(self, context):
    
        filepath = "C:/Users/Ludwig/Desktop/OldPrint3.PNG"
        bpy.data.images.load(filepath, check_existing=True)
        print(bpy.data.images)
        x1 = 0
        x2 = 200
        y1 = 0
        y2 = 200
        
        IMAGE_NAME = "OldPrint3.PNG"
        image = bpy.data.images[IMAGE_NAME]
        
        shader = gpu.shader.from_builtin('2D_IMAGE')
        batch = batch_for_shader(
            shader, 'TRI_FAN',
            {
                "pos": ((x1, y1), (x2, y1), (x2, y2), (x1, y2)),
                "texCoord": ((0, 0), (1, 0), (1, 1), (0, 1)),
            },
        )
        

        # image = bpy.data.images['logo']

        if image.gl_load():
            print("OMGOMGOMGOMGOMGOMGOMGOMGOMOGMOMG")
            return # an exception happened
        print(image.bindcode)
        def draw():
            bgl.glActiveTexture(bgl.GL_TEXTURE0)
            bgl.glBindTexture(bgl.GL_TEXTURE_2D, image.bindcode)
            shader.bind()
            shader.uniform_int("image", 0)
            batch.draw(shader)

        # image.gl_free()
        layout = self.layout
        obj = context.object
        row = layout.row()
        row.prop(obj, "name")

    # handler = bpy.types.SpaceProperties.draw_handler_add(draw,(),'WINDOW','POST_PIXEL')
        bpy.types.SpaceProperties.draw_handler_add(draw, (), 'WINDOW', 'POST_PIXEL')
        # bpy.types.SpaceView3D.draw_handler_add(draw, (), 'WINDOW', 'POST_PIXEL')

    # then remove the handler with ...
    # bpy.types.SpaceProperties.draw_handler_remove(handler, 'WINDOW')
    
    
def register():
    bpy.utils.register_class(HelloWorldPanel)


def unregister():
    bpy.utils.unregister_class(HelloWorldPanel)