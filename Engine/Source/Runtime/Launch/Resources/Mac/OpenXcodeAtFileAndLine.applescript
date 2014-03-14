on OpenXcodeAtFileAndLine(filepath, linenumber)
	set theOffset to offset of "/" in filepath
	tell application "Xcode"
		activate
		if theOffset is 1 then
			open filepath
		end if
		tell application "System Events"
			tell process "Xcode"
				if theOffset is not 1 then
					keystroke "o" using {command down, shift down}
					repeat until window "Open Quickly" exists
					end repeat
					click text field 1 of window "Open Quickly"
					set value of text field 1 of window "Open Quickly" to filepath
					keystroke return
				end if
				keystroke "l" using command down
				repeat until window "Open Quickly" exists
				end repeat
				click text field 1 of window "Open Quickly"
				set value of text field 1 of window "Open Quickly" to linenumber
				keystroke return
				keystroke return
			end tell
		end tell
	end tell
end OpenXcodeAtFileAndLine
