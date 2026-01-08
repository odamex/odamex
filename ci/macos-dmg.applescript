on run argv
	if (count of argv) < 1 then
		error "Missing volume name"
	end if
	set dmgName to item 1 of argv

	tell application "Finder"
		set targetDisk to missing value
		repeat with i from 1 to 10
			try
				set targetDisk to disk dmgName
				exit repeat
			end try
			delay 1
		end repeat
		if targetDisk is missing value then
			error "Could not find disk " & dmgName
		end if
		tell targetDisk
			open
			set options to icon view options of container window
			tell options
				set icon size to 104
				set arrangement to not arranged
			end tell
			set background picture of options to file ".background:background.png"
			tell container window
				set the bounds to {0, 0, 500, 350}
				set current view to icon view
				set toolbar visible to false
				set statusbar visible to false
			end tell

			try
				set icon of folder "Odamex" to icon of file ".background:odamex.icns"
			end try

			make new alias file to POSIX file "/Applications" at disk dmgName with properties {name:"Applications"}

			set position of item "Odamex" to {75, 150}
			set position of item "Applications" to {425, 150}
			close
		end tell
	end tell
end run
