#!/usr/bin/python

import re, sys

#scalex = float(sys.argv[1])
#scaley = float(sys.argv[2])
#scalez = float(sys.argv[3])
#offsetx = int(sys.argv[4])
#offsety = int(sys.argv[5])
#offsetz = int(sys.argv[6])
#skpath = str(sys.argv[7])

#offsetx = float(raw_input("Enter offset x [nm]: "))
#offsety = float(raw_input("Enter offset y [nm]: "))
#offsetz = float(raw_input("Enter offset z [nm]: "))

skpath = raw_input("file to convert: ")

scalex = float(raw_input("Enter voxel scale x[nm]: "))
scaley = float(raw_input("Enter voxel scale y[nm]: "))
scalez = float(raw_input("Enter voxel scale z[nm]: "))

offsetz = offsety = offsetx = 0

skfile = open(skpath)
sktext = skfile.read()
skfile.close()

fltregu = re.compile("[xyz]\=\"([1234567890]*\.[1234567890]*)\"")
thingsregu = re.compile("\<things\>")

match = fltregu.search(sktext)
while match:
  newintval = int(float(sktext[match.span(1)[0]:match.span(1)[1]]) * 1000)
  sktext = sktext[:match.span(1)[0]] + str(newintval) + sktext[match.span(1)[1]:]
  match = fltregu.search(sktext)

match = thingsregu.search(sktext)
sktext = sktext[:match.span()[1]] + "\n\n  <parameters>\n    <scale x=\"" + str(scalex) + "\" y=\"" + str(scaley) + "\" z=\"" + str(scalez) + "\"/>\n    <offset x=\"" + str(offsetx) + "\" y=\"" + str(offsety) + "\" z=\"" + str(offsetz) + "\"/>\n  </parameters>\n" + sktext[match.span()[1]:]

nskpath = skpath + ".new"
nskfile = open(nskpath, 'a')
nskfile.write(sktext)
nskfile.close()
