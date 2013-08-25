#!/usr/bin/python

# download glext.h from http://www.opengl.org/registry/oldspecs/glext.h, and get prototype/definition of symbol

import urllib2
import sys

if len(sys.argv) < 2:
	print("\n" + sys.argv[0] + ": error: no arguments specified!")
	sys.exit()

print("\nDownloading glext.h from http://www.opengl.org/registry/oldspecs/glext.h...")
response = urllib2.urlopen('http://www.opengl.org/registry/oldspecs/glext.h')
glext_h = response.read().splitlines()
print("done.")

print("Using embedded glext.h.\n")
symbol_to_search = sys.argv[1]

print("\nSearching for symbol " + symbol_to_search + "...")

matches = []

for num, line in enumerate(glext_h.splitlines()):
	if symbol_to_search in line:
		matches.append("line " + str(num) + ":\t" + line)
		
print("Found " + str(len(matches)) + " matches.\n")

for match in matches:
	print(match)
