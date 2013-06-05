bl_info = {
	"name": "Export Binary memcpy-obj (.bobj)",
	"author": "kaido kuukap & voimlemise karpsed",
	"version": (0, 0, 1),
	"blender": (2, 6, 7),
	"location": "File > Import-Export",
	"description": "Export mesh data to memcpyable bobj.",
	"category": "Import-Export" 
	}
	

#install this script in blender by selecting the addon menu => install from file.
#alternatively, the script can be named __init__.py and put into a new folder in
#<blender_dir>/scripts/addons/, and it will be auto-detected.

	
import bpy
import time
from array import array
import struct

from bpy_extras.io_utils import ExportHelper


def deselect_all(objects):
	for ob in objects:
		ob.select = False
		
def mesh_triangulate(me):
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()	
	
def clearfile(filename):
	try:
		with open(filename, 'w'): 	# this clears the file.
			pass
	except IOError as e:
		print("clearfile: error opening file " + filename + ": ", e)
		
def do_export(context, filepath):
	#meshify, triangulate, compile and dump to file.
	
	scene = context.scene
	
	if (len(context.selected_objects) <= 0):
		print("No selected objects. Exiting.\n")
		return False
	
	APPLY_MODIFIERS = True
	
	mesh = context.selected_objects[0].to_mesh(scene, APPLY_MODIFIERS, "PREVIEW")
	mesh_triangulate(mesh)
	mesh.calc_tessface()
	
	if(len(mesh.uv_textures) <= 0):
		print("Error: mesh doesnt have uv-coordinates. Exiting.")
		return False
		
	data = []
	for face in mesh.tessfaces:
		face_uv = mesh.tessface_uv_textures.active.data[face.index]
		face_uv = face_uv.uv1, face_uv.uv2, face_uv.uv3	# a triangulated mesh shouldn't have uv4 in the first place :P
		i = 0
		for index in face.vertices:
			vert = mesh.vertices[index]
			for c in vert.co:
				data.append(c)
			for n in vert.normal:
				data.append(n)
			for uv in face_uv[i]:
				data.append(uv)
			i += 1
			
	
	print("Constructed vertex data structure with a V3FN3FT2F arrangment in a flat python list.\nOutput file statistics:")
	print("len(data) = " + str(len(data)) + " <=> " +
	"num_vertices = " + str(int(len(data)/8)) + " <=> " +
	"num_faces = " + str(int((len(data)/8)/3)) + ".")
	print(data[0])
	print("len(mesh.tessfaces): " + str(len(mesh.tessfaces)))
	print("len(tessface_uv_textures.active.data) : " + str(len(mesh.tessface_uv_textures.active.data)))
	
	out_fp = open(filepath, "wb")
	out_fp.write("bobj".encode("UTF-8"))
	out_fp.write(struct.pack("i", int(len(data)/8)))
	data_arr = array('f', data)
	data_arr.tofile(out_fp)
	out_fp.close()
	import os
	print("Output file size: " + str(os.path.getsize(filepath)))
	return True
	

class Export_bobj(bpy.types.Operator, ExportHelper):
	bl_idname = "export_mesh.bobj"
	bl_label = "Export binary obj (.bobj)"
	bl_options = {"PRESET"}
	filename_ext = ".bobj"
	
	def execute(self, context):
		start_time = time.time()
		print("\nStarting .bobj export.	")
		filepath = self.filepath
		props=self.properties
		filepath = bpy.path.ensure_ext(filepath, self.filename_ext)
		
		exported = do_export(context, filepath)
		if exported:
			print('finished export in %s seconds' %((time.time() - start_time)) )
			print(filepath)
			return {'FINISHED'}
		else:
			return {'FINISHED'}		
		
	
### REGISTER ###
def menu_func(self, context):
    self.layout.operator(Export_bobj.bl_idname, text="Memcpyable .bobj file");

def register():
    bpy.utils.register_module(__name__);
    bpy.types.INFO_MT_file_export.append(menu_func);
    
def unregister():
    bpy.utils.unregister_module(__name__);
    bpy.types.INFO_MT_file_export.remove(menu_func);

if __name__ == "__main__":
    register()
	
	
	
	