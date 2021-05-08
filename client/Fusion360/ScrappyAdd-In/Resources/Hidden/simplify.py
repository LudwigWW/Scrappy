import bpy
import sys
# import argparse

# parser=argparse.ArgumentParser()

# parser.add_argument('--ftspath', help='Do the bar option')
# parser.add_argument('--ftsname', help='Foo the program')
# parser.add_argument('--python', help='Calls this script')

# args=parser.parse_args()

# print('AAAAAAAAAAAAAAAAAAAH')
# print(args)
# print(sys)

target_polygons = int(sys.argv[len(sys.argv)-4])
target_path = sys.argv[len(sys.argv)-3]
file_path_to_simplify = sys.argv[len(sys.argv)-2]
file_name_to_simplify = sys.argv[len(sys.argv)-1]

print(file_path_to_simplify)
print(file_name_to_simplify)

file_name_to_simplify = file_name_to_simplify.replace("_", " ")

bpy.ops.import_mesh.stl(filepath=file_path_to_simplify)

# file_loc = "D:/Dropbox/Eigenes/UWaterloo Courses/PhD/Scrappy/Fusion360/ScrappyAdd-In/Resources/Library/473mlCan.stl"
# imported_object = bpy.ops.import_scene.obj(filepath=file_loc)
# obj_object = bpy.context.selected_objects[0] ####<--Fix
# print('Imported name: ', obj_object.name)

# all = [item.name for item in bpy.data.objects]
# for name in all:
    # print(name)

# for item in bpy.data.objects:
    # if item.name == file_name_to_simplify:
        # file_to_simplify = item
        # break

object_to_simplify = bpy.data.objects[file_name_to_simplify]
print(object_to_simplify)
object_to_simplify_data = object_to_simplify.data
print(len(object_to_simplify_data.polygons))

bpy.ops.object.mode_set(mode='OBJECT')

if(len(object_to_simplify_data.polygons) > target_polygons):
    modifier = object_to_simplify.modifiers.new(name="decimator", type='DECIMATE')
    modifier.ratio = 1 / (len(object_to_simplify_data.polygons) / target_polygons)
    bpy.ops.object.modifier_apply(modifier="decimator")

bpy.ops.object.mode_set(mode='EDIT')

bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.fill_holes(0)
bpy.ops.mesh.quads_convert_to_tris()
bpy.ops.mesh.remove_doubles(use_unselected=True)
bpy.ops.mesh.dissolve_degenerate()
bpy.ops.mesh.delete_loose()
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.normals_make_consistent()


bpy.ops.export_mesh.stl(
    filepath=str(target_path),
    use_selection=True,
    use_scene_unit=True,
    ascii=True)



# bpy.context.scene.objects.active = object_to_simplify #sets the obj accessible to bpy.ops
# decimator = bpy.ops.object.modifier_add(type='DECIMATE')
# print(decimator)