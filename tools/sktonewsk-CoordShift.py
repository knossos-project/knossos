#!/usr/bin/python

import re, sys, os, glob

fltregu = re.compile("([xyz])\=\"([1234567890]*)\"")
regOffset = re.compile("offset x\=\"1\" y\=\"1\" z\=\"1\"")
def matchF(match):
	return match.group(1) + '="' + str(int(match.group(2)) + 1) + '"'
	
files = glob.glob('*.nml');
for skpath in files:
	skfile = open(skpath)
	sktext = skfile.read()
	skfile.close()

	sktext = fltregu.sub(matchF, sktext)
	sktext = regOffset.sub('offset x="0" y="0" z="0"', sktext)
	
	nskpath = skpath
	nskfile = open(nskpath + ".new", 'w')
	nskfile.write(sktext)
	nskfile.close()
