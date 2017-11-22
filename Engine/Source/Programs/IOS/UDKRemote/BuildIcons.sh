#!/bin/bash

declare -a sizes=(
	29
	40
	58
	76
	80
	80-1
	87
	120
	152
	167
	180
	1024
        )

for i in "${sizes[@]}"
do
	sips --resampleWidth $i SourceIcon.png --out UDKRemote/Images.xcassets/AppIcon.appiconset/Icon$i.png
done

