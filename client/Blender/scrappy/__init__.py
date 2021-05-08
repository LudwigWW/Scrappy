bl_info = {
    "name": "Scrappy",
    "author": "Ludwig Wilhelm Wall",
    "version": (1, 0, 0),
    "blender": (2, 80, 0),
    "location": "View3D > UI",
    "description": "reducing infill by including scrap material in objects",
    "category": "Research Prototype",
}

import bpy
import gpu
import bgl
from gpu_extras.batch import batch_for_shader
import os
import bpy.utils.previews


directory   = os.path.join(bpy.utils.user_resource('SCRIPTS'), "scrappy", "scrappy_library\\")
list_raw = []

print("######################")

print(directory)
print("######################")

from os import listdir
from os.path import isfile, join
onlyfiles = [f for f in listdir(directory) if isfile(join(directory, f))]

for f in onlyfiles:
    if f[-4:] == ".jpg":
        list_raw.append(f)

# global variable to store icons in
custom_icons = None

if bpy.app.version < (2,80,0):
    Region = "TOOLS"
else:
    Region = "UI"

def makeMonkey(context):
    #body
    bpy.ops.mesh.primitive_cube_add()
    bpy.ops.transform.resize(value=(1.2, 0.8, 1.5))

    #head
    bpy.ops.mesh.primitive_monkey_add()
    bpy.ops.transform.translate(value=(0, -0.3, 2))

    #arms
    bpy.ops.mesh.primitive_cylinder_add()
    bpy.ops.transform.translate(value=(2, 0, 0))
    bpy.ops.transform.resize(value=(0.5, 0.5, 1.5))
    bpy.ops.transform.rotate(value=0.523599, orient_axis='Y')
    bpy.ops.mesh.primitive_cylinder_add()
    bpy.ops.transform.translate(value=(-2, 0, 0))
    bpy.ops.transform.resize(value=(0.5, 0.5, 1.5))
    bpy.ops.transform.rotate(value=-0.523599, orient_axis='Y')

    #legs
    bpy.ops.mesh.primitive_cylinder_add()
    bpy.ops.transform.translate(value=(0.7, 0, -3.5))
    bpy.ops.transform.resize(value=(0.5, 0.5, 2))
    bpy.ops.mesh.primitive_cylinder_add()
    bpy.ops.transform.translate(value=(-0.7, 0, -3.5))
    bpy.ops.transform.resize(value=(0.5, 0.5, 2))
    #bpy.ops.object.duplicate_move(TRANSFORM_OT_translate={"value":(-1.4, 0, 0)})

class ScrappyPanel(bpy.types.Panel):
    """Creates a Panel in the 3D view Tools panel"""
    bl_idname = "TEST_PT_Panel" 
    bl_label = "Scrappy Library Icon test"
    bl_category = "Scrappy Library"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_context = "objectmode"

    def draw(self, context):
        global custom_icons

        for z in list_raw:
            self.layout.template_icon(icon_value=custom_icons[z[:-4]].icon_id,scale=10)

class ObjectMoveX(bpy.types.Operator):
    """My Object Moving Script"""      # Use this as a tooltip for menu items and buttons.
    bl_idname = "object.move_x"        # Unique identifier for buttons and menu items to reference.
    bl_label = "Move X by One"         # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}  # Enable undo for the operator.

    def execute(self, context):        # execute() is called when running the operator.

        # The original script
        scene = context.scene
        for obj in scene.objects:
            obj.location.x += 1.0

        return {'FINISHED'}            # Lets Blender know the operator finished successfully.

class CreateMonkey(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "myops.add_monkey"
    bl_label = "Add Monkey Operator"

    def execute(self, context):
        makeMonkey(context)
        return {'FINISHED'}

class MonkeyPanel(bpy.types.Panel):
    """Creates a Panel in the Tool Shelf"""
    bl_label = "Scrappy"
    bl_idname = "OBJECT_PT_monkey"
    bl_space_type = 'VIEW_3D'
    bl_region_type = Region
    bl_category = "Scrappy Controls"

    def draw(self, context):
        layout = self.layout

        obj = context.object

        row = layout.row()
        row.label(text="Hello world!", icon='WORLD_DATA')

        row = layout.row()
        row.operator("myops.add_monkey")

        row = layout.row()
        row.operator("object.move_x")
        
        filepath = "C:/Users/Ludwig/Desktop/OldPrint4.PNG"
        
        bpy.data.images.load(filepath, check_existing=True)
        # Load a new image into the main database

        # bpy.data.images.load("/home/zeffii/Desktop/some_image.png")
        # bpy.data.images['some_image.png']

        # if an image exists by that name already, you could set the name
        # bpy.data.images['OldPrint4.PNG.001'].name = 'OldPrint4.PNG'

        # check_exist=True will reuse an existing image with that name
        # bpy.data.images.load("/home/zeffii/Desktop/some_image.png", check_existing=True)

        IMAGE_NAME = "OldPrint4.PNG"
        image = bpy.data.images[IMAGE_NAME]

        shader = gpu.shader.from_builtin('2D_IMAGE')
        batch = batch_for_shader(
            shader, 'TRI_FAN',
            {
                "pos": ((100, 100), (200, 100), (200, 200), (100, 200)),
                "texCoord": ((0, 0), (1, 0), (1, 1), (0, 1)),
            },
        )

        if image.gl_load():
            raise Exception()

        def draw():
            bgl.glActiveTexture(bgl.GL_TEXTURE0)
            bgl.glBindTexture(bgl.GL_TEXTURE_2D, image.bindcode)

            shader.bind()
            shader.uniform_int("image", 0)
            batch.draw(shader)


        bpy.types.SpaceView3D.draw_handler_add(draw, (), 'WINDOW', 'POST_PIXEL')
        print(bpy.data.images)
        # tex = bpy.data.images['OldPrint4.PNG']
        # print(tex)
        # col = layout.box().column()
        # col.template_preview(tex)

        row = layout.row()
        row.label(text="Active object is: " + obj.name)
        row = layout.row()
        row.prop(obj, "name")


def register():
    bpy.utils.register_class(MonkeyPanel)
    bpy.utils.register_class(CreateMonkey)
    bpy.utils.register_class(ObjectMoveX)
    print("Hello World")
    
    global custom_icons
    custom_icons = bpy.utils.previews.new()

    for z in list_raw:
        custom_icons.load(z[:-4], os.path.join(directory, z), 'IMAGE')


    bpy.utils.register_class(ScrappyPanel)

def unregister():
    bpy.utils.unregister_class(MonkeyPanel)
    bpy.utils.unregister_class(CreateMonkey)
    bpy.utils.unregister_class(ObjectMoveX)
    print("Goodbye World")
    
    global custom_icons
    bpy.utils.previews.remove(custom_icons)

    bpy.utils.unregister_class(ScrappyPanel)

# def register():
    # print("Hello World")
    # bpy.utils.register_class(ObjectMoveX)
    
# def unregister():
    # bpy.utils.unregister_class(ObjectMoveX)
    # print("Goodbye World")

# This allows you to run the script directly from Blender's Text editor
# to test the add-on without having to install it.
if __name__ == "__main__":
    register()