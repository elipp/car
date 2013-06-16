bl_info = {
	"name": "Export Binary memcpy-obj (.bobj)",
	"author": "Kaido Kuukap & voimlemise karpsed =)",
	"version": (0, 0, 1),
	"blender": (2, 6, 7),
	"location": "File > Import-Export",
	"description": "Export (the currently selected!) object to memcpyable bobj.",
	"category": "Import-Export" 
	}
	

#install this script in blender by selecting the addon menu => install from file.
#alternatively, the script can be named __init__.py and put into a new folder in
#<blender_dir>/scripts/addons/, and it will be auto-detected.

	
import bpy
import time
from array import array
import struct

import os
import subprocess
	
from bpy_extras.io_utils import ExportHelper

		
def mesh_triangulate(me):
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()	
	
def do_export(context, filepath):
	#meshify currently selected objects, triangulate, compile, compress and dump to file.
	
	print("\nStarting .bobj export.\n")
	
	scene = context.scene
	
	if (len(context.selected_objects) <= 0):
		return "Error: no selected objects! Exiting.\n"
	
	APPLY_MODIFIERS = True
	
	bpy.ops.object.mode_set(mode="OBJECT")
	
	print("using (0.0, 0.0, 0.0) as object origin.")
	bpy.context.scene.cursor_location = (0.0, 0.0, 0.0)
	bpy.ops.object.origin_set(type="ORIGIN_CURSOR")

	ob = context.active_object
	dims = ob.bound_box.data.dimensions
	
	print("Object bounding box dimensions: (" + ", ".join("{0:.3f}".format(c) for c in dims) + ").")
	
	highest_z = 0
	lowest_z = 0
	
	for coord in ob.bound_box:
		curr_z = coord[2]
		if curr_z > highest_z:
			highest_z = curr_z
		elif curr_z < lowest_z:
			lowest_z = curr_z
			
	print("Lowest point: " + "{0:.4f}".format(lowest_z) + ", highest point: " + "{0:.4f}".format(highest_z))
	
	mesh = ob.to_mesh(scene, APPLY_MODIFIERS, "PREVIEW")
	print("Triangulating mesh...")
	mesh_triangulate(mesh)
	print("done.")
	mesh.calc_tessface()
	
	if(len(mesh.uv_textures) <= 0):
		#add option to generate at least some uv coordinates :P
		print("bobj-export: error: mesh doesnt seem to have a uv-layer.")
		return "bobj-export: error: mesh doesnt seem to have a uv-layer."
		
	# blender actually uses the z-coordinate to represent height, as opposed to y in opengl :P
	# also, since "into the screen" in a identity view transformation means _NEGATIVE-Z_ in opengl,
	# the sign of the swapped z-coordinate will have to be negated
		
	data = []
	for face in mesh.tessfaces:
		face_uv = mesh.tessface_uv_textures.active.data[face.index]
		face_uv = face_uv.uv1, face_uv.uv2, face_uv.uv3	
		# a triangulated mesh shouldn't have a (meaningful) uv4.
		i = 0
		for index in face.vertices:
			vert = mesh.vertices[index]
			coords_rearranged = [vert.co[0], vert.co[2], -vert.co[1]]
			for c in coords_rearranged:
				data.append(c)
			normal_rearranged = [vert.normal[0], vert.normal[2], -vert.normal[1]]
			for n in normal_rearranged:
				data.append(n)
			for uv in face_uv[i]:
				data.append(uv)
			i += 1
			
	
	print("Constructed a flat float list with a V3F N3F T2F arrangement (y-z swapped! *).\n")
	print("len(data) = " + str(len(data)) + " <=> " +
	"num_vertices = " + str(int(len(data)/8)) + " <=> " +
	"num_faces = " + str(int((len(data)/8)/3)) + ".")
	print("len(mesh.tessfaces): " + str(len(mesh.tessfaces)))
	print("len(tessface_uv_textures.active.data) : " + str(len(mesh.tessface_uv_textures.active.data)))
	print("\n(* Blender uses the z-coordinate to store height, whereas OpenGL uses y.)\n")
	tmpfilename = filepath + ".tmpuncompressed"
	
	out_fp = open(tmpfilename, "wb")
	out_fp.write("bobj".encode("UTF-8"))
	out_fp.write(struct.pack("i", int(len(data)/8)))
	data_arr = array('f', data)
	data_arr.tofile(out_fp)
	out_fp.close()
	
	uncompressed_size = int(os.path.getsize(tmpfilename)/1024)

	lzmacommand = ["lzma", "e", tmpfilename, filepath]
	print("Compressing with LZMA (lzma.exe)...")
	r = 1
	with open(os.devnull, "w") as fnull:
		r = subprocess.call(lzmacommand, stdout = fnull, stderr = fnull)
	if r != 0:
		print("lzma.exe returned error (code " + str(r) + ").\n")
		return "Error: lzma compression failed (lzma.exe)."
	print("Compression completed successfully.")
	print("(deleting temporary file " + tmpfilename + ".)")
	os.remove(tmpfilename)
	
	compressed_size = int(os.path.getsize(filepath)/1024)
	ratio = compressed_size / uncompressed_size
	print("Output file size: " + str(compressed_size) + " kB (uncompressed: " + str(uncompressed_size) + " kB -> ratio = " + "{0:.3f}".format(ratio) + ")")
	return "OK"
	

class Export_bobj(bpy.types.Operator, ExportHelper):
	bl_idname = "export_mesh.bobj"
	bl_label = "Export binary obj (.bobj)"
	bl_options = {"PRESET"}
	filename_ext = ".bobj"
	
	def execute(self, context):
		start_time = time.time()
		filepath = self.filepath
		props=self.properties
		filepath = bpy.path.ensure_ext(filepath, self.filename_ext)
		
		errmsg = do_export(context, filepath)
		if "Error" in errmsg:
			self.report({"ERROR"}, errmsg + "\nbobj export failed. No output files.\n")
			return {'CANCELLED'}	

		else:		
			print('Bobj export took {0:.3f} seconds'.format((time.time() - start_time)) )
			print("\nFinished baking output file " + filepath + " :D")
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
	
	
	
	