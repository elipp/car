# 

import bpy

bpy.context.scene.cursor_location = (0.0, 0.0, 0.0)
bpy.ops.mesh.primitive_plane_add()

bpy.ops.object.mode_set(mode="OBJECT")
bpy.ops.transform.resize(value=(8,8,8))

bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.uv.unwrap()

bpy.ops.object.mode_set(mode="OBJECT")

bpy.ops.object.material_slot_add()


new_material = bpy.data.materials.new(name="hmap_texture")
bpy.context.object.active_material = new_material

material_name = new_material.name

bpy.ops.texture.new()
tex = bpy.data.textures[-1]
tex.name = "hmap_texture"
tex.type = "IMAGE"

tex_slot = bpy.context.object.material_slots[material_name].material.texture_slots.add()
tex_slot.texture = tex

#there we go, now just load a picture and set the color space and whatever