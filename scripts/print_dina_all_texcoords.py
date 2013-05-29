#!/usr/bin/python

# print dina_all.png texcoords to file

import sys

glyph_spacing_horiz_pixels = 7.0
glyph_spacing_vert_pixels = 13.0

glyph_width_pixels = 6.0
glyph_height_pixels = 12.0

texture_width_pixels = 128.0
texture_height_pixels = 128.0

glyphs_per_line = 14
number_of_lines = 7

U_incr_special = glyph_width_pixels/texture_width_pixels
V_incr_special = glyph_height_pixels/texture_width_pixels
U_incr = glyph_spacing_horiz_pixels/texture_width_pixels
V_incr = glyph_spacing_vert_pixels/texture_height_pixels

# counter clockwise order, ie.
# 1-----4
# |		|
# |		|
# 2-----3
#

def get_texcoords(v_inverted = False):
	invert_if = None
	if (v_inverted == True):
		invert_if = lambda x: 1.0 - x
	else:
		invert_if = lambda x: x
	
	texcoords = []
	for LINE_NUM in range(0, number_of_lines, 1):
		for GLYPH_NUM in range(0, glyphs_per_line, 1):
			texcoords.append( [
			(GLYPH_NUM*U_incr, invert_if(LINE_NUM*V_incr)), 
			(GLYPH_NUM*U_incr, invert_if(LINE_NUM*V_incr + V_incr_special)), 
			(GLYPH_NUM*U_incr + U_incr_special, invert_if(LINE_NUM*V_incr + V_incr_special)), 
			(GLYPH_NUM*U_incr + U_incr_special, invert_if(LINE_NUM*V_incr))
			] )
	#return [coord for glyph in texcoords for tuple in glyph for coord in tuple]
	return texcoords
			


def print_glyph(glyph, has_comma = True):
	sys.stdout.write("{")
	for coord_pair in glyph[:-1]:
			sys.stdout.write("{ " + str(coord_pair[0]) + ", " + str(coord_pair[1]) + " }, ")
	sys.stdout.write("{ " + str(glyph[-1][0]) + ", " + str(glyph[-1][1]) + " }}")
	if (has_comma == True):
		sys.stdout.write(",\n")
		
			
def main():
	
	V_INVERTED = True
	
	texcoords = get_texcoords(True)
	print("#ifndef PRECALCULATED_TEXCOORDS_H")
	print("#define PRECALCULATED_TEXCOORDS_H")
	print("/*\n This file was generated via scripts/print_dina_all_texcoords.py.")
	print(" using counter-clockwise vertex ordering.\n")
	print(" params:")
	print(" V_INVERTED = " + str(V_INVERTED) + ",")
	print(" char_spacing_horiz = " + str(glyph_spacing_horiz_pixels) + " pixels,")
	print(" char_spacing_vert = " + str(glyph_spacing_vert_pixels) + " pixels,")
	print(" Actual glyph size = " + str(glyph_width_pixels) + "x" + str(glyph_height_pixels) + ".\n*/")
	print("#define CURSOR_GLYPH_INDEX (14*6 + 11) // the cursor glyph is located at line 7, index 11")
	print("struct uv { float u; float v; };\n")
	print("static const struct uv glyph_texcoords[][4] = {")
	for glyph in texcoords[:-1]:
		print_glyph(glyph, True)
	print_glyph(texcoords[-1], False)
	print("\n};")
	print("\n#endif")
	return 0
			
if __name__=="__main__":
	main()