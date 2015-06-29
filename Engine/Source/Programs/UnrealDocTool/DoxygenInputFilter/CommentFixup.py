import sys
f = file(sys.argv[1], 'r')
line = f.readline()
while line:
    # Pad the string with an extra space at the end to catch end-of-line block comments.
    line = line + " "
    line = line.replace("/* ", "/** ")
    line = line.replace("// ", "/// ")
    sys.stdout.write(line)
    line = f.readline()
f.close() 