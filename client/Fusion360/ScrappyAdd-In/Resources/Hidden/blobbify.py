import bpy
import sys
from mathutils import Vector

target_polygons = int(sys.argv[len(sys.argv)-4])
target_path = sys.argv[len(sys.argv)-3]
file_path_to_simplify = sys.argv[len(sys.argv)-2]
file_name_to_simplify = sys.argv[len(sys.argv)-1]

file_name_to_simplify = file_name_to_simplify.replace("_", " ")

print(file_path_to_simplify)
print(file_name_to_simplify)

bpy.ops.import_mesh.stl(filepath=file_path_to_simplify)

object_to_simplify = bpy.data.objects[file_name_to_simplify]

sel = bpy.context.selected_objects

for i, obj in enumerate(bpy.context.selected_objects):

    # Get center of existing objects bounding box
    local_bbox_center = 0.125 * sum((Vector(b) for b in object_to_simplify.bound_box), Vector())
    
    # Deselect original object
    object_to_simplify.select_set(False)
    
    # obj.display_type = 'BOUNDS'
    # Make new cube, set center to previous BB center, set dimensions to [twice] the original + margin for very thin objects
    bpy.ops.mesh.primitive_cube_add(size=1, enter_editmode=False, location=local_bbox_center, rotation=obj.rotation_euler)
    addedV = Vector((30.0, 30.0, 30.0))
    bpy.context.active_object.dimensions = sel[i].dimensions + addedV
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=True)

    # Rename in case the original was called "Cube" as well and select only the original
    bpy.context.object.name = "bb_Cube_Scrappy_BLOB"
    bpy.data.objects["bb_Cube_Scrappy_BLOB"].select_set(False)
    object_to_simplify.select_set(True)

    # Delete original
    bpy.ops.object.delete()

# Save this blob
bpy.data.objects["bb_Cube_Scrappy_BLOB"].select_set(True)

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