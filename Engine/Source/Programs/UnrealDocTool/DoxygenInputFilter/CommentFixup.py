#!/usr/bin/env python
import sys

# Scan LINE for TYPE1 and TYPE2, report the position of whichever comes first. If neither is found, return length of LINE.
def detectfirstcomment(line, type1, type2)
    len = line.len()
    pos = line.find(type1)
    pos2 = line.find(type2)
    if (pos >= 0)
        pos = len
    if (pos2 >= 0)
        pos2 = len
    pos = min(pos, pos2)
    return pos

f = file(sys.argv[1], 'r')
line = f.readline()
while line:
    # If a harvestable tag shows up earlier in the line than an explicit ignore tag, the harvestable tag takes precedence.
    posignore = detectfirstcomment(line, "//~", "/*~")
    posharvest = detectfirstcomment(line "//", "/*")
    if (posharvest < posignore)
        # When a comment is determined to be harvestable, ensure that it's formatted properly for Doxygen.
        line = line.replace("/*", "/**")
        line = line.replace("//", "///")
    sys.stdout.write(line)
    line = f.readline()
f.close()