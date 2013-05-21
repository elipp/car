# jeeah

import itertools
import re
import os
import sys
from array import array
import struct

version = "0.1"

def bobj_compile(positions, normals, uvs, faces, invert_v = False):
	bobjlist = []
	if invert_v:
		print("Inverting texture coordinates' v-component (v = 1.0-v).")
		for face in faces:
			bobjlist.append([
					[float(x) for x in positions[int(face[0])]], 
					[float(x) for x in normals[int(face[1])]], 
					#[if (i%2)==0 float(x) else float(1.0)-float(x) for i, x in enumerate(uvs[int(face[2])])] ]) # invert v-coordinate :P
					[float(x) if p%2==0 else 1.0-float(x) for p, x in enumerate(uvs[int(face[2])]) ]
					])
	else:
		print("Not inverting v.")
		for face in faces:
			bobjlist.append([
					[float(x) for x in positions[int(face[0])]], 
					[float(x) for x in normals[int(face[1])]], 
					#[if (i%2)==0 float(x) else float(1.0)-float(x) for i, x in enumerate(uvs[int(face[2])])] ]) # invert v-coordinate :P
					[float(x) for x in uvs[int(face[2])] ]
					])

	return bobjlist

def clearfile(filename):
	try:
		with open(filename, 'w'): 	# this clears the file.
			pass
	except IOError as e:
		print("Error opening file " + filename + ": ", e)
	
def writebobj(filename, data):
	
	if os.path.isfile(filename):
		ans = raw_input("\nWARNING: the output file (" + filename + ") already exists. Is it OK to overwrite? [Y/n]")
		if ans == 'Y' or ans == 'y':
			clearfile(filename)
			output = open(filename, "wb")

		else:
			print("Exiting.")
			sys.exit()
	
	else:	
		try:
			output = open(filename, "wb")
		except IOError as e:
			print("Error opening file " + filename + ": ", e)
	#write header
	print("\nWriting to output file " + filename + ".\n")
	output.write("bobj".encode("UTF-8"))
	output.write(struct.pack('i', len(data)))
	# flatten input data list (should be in format [[f, f, f], [f, f, f], [f, f]] )
	flatdata = array('f', [item for sublist in data for another in sublist for item in another])
	flatdata.tofile(output)
	output.close()
	print("Done.")
	
positions = []
normals = []
uvs = []

filename = ""
if len(sys.argv) < 2:
	print("No input files.")

else: 
	filename = sys.argv[1]
	filename_base = os.path.splitext(filename)[0]

try:
	with open(filename, 'r') as f:
		f_contents = f.readlines()

except IOError as e:
	print("Error opening input file " + filename + ". Exiting.")
	exit(1)



geometry_id = ""

for line in f_contents:
	if "<geometry id=" in line:
		tmp = re.findall('<geometry id="([^"]*)"', line)
		geometry_id = tmp[0]
		break

s_stem = "<float_array id=\"" + geometry_id

for line in f_contents:
	if s_stem + "-positions-array\"" in line:
		contents = line[line.find('>') +1 : line.find('</f')].split()
		positions = zip(*(iter(contents),) * 3)

	if s_stem + "-normals-array\"" in line:
		contents = line[line.find('>') +1 : line.find('</f')].split()
		normals = zip(*(iter(contents),) * 3)

	if s_stem + "-map-0-array\"" in line:
		contents = line[line.find('>') +1 : line.find('</f')].split()
		uvs = zip(*(iter(contents),) * 2)

	if "<p>" in line:
		contents = line[line.find('<p>')+3 : line.find('</p>')].split()	# extract contents between xml tags <p>...</p>
		faces = zip(*(iter(contents),) * 3)	# split to groups of 3

print("dae_parse.py  " + version + ".\nInput file: " + filename + ".\n")
print("positions: \t" + str(len(positions)))
print("normals: \t"  + str(len(normals)))
print("uvs: \t\t" + str(len(uvs)))

bobjdata = bobj_compile(positions, normals, uvs, faces, True)

print("Output file has " + str(len(bobjdata)) + " vertices.")
writebobj(filename_base + ".bobj", bobjdata)




