#!/usr/bin/env python
import sys
f = file(sys.argv[1], 'r')
line = f.readline()
while line:
    line = line + " "
    line = line.replace("/* ", "/** ")
    line = line.replace("// ", "/// ")
    sys.stdout.write(line)
    line = f.readline()
f.close() 