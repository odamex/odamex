on run argv
	if (count of argv) < 1 then
		error "Missing mount path"
	end if
	set mountPath to item 1 of argv

	tell application "Finder"
		set targetDisk to disk of (POSIX file mountPath as alias)
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

			make new alias file to POSIX file "/Applications" at targetDisk with properties {name:"Applications"}

			set position of item "Odamex" to {111, 179}
			set position of item "Applications" to {384, 179}
			close
		end tell
	end tell
end run
