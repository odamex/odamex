-- ----------------------------------------------------------------------------
--  makemacdmg.applescript
-- ----------------------------------------------------------------------------
--
--  This library is free software; you can redistribute it and/or
--  modify it under the terms of the GNU Lesser General Public
--  License as published by the Free Software Foundation; either
--  version 2.1 of the License, or (at your option) any later version.
--
--  This library is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--  Lesser General Public License for more details.
--
--  You should have received a copy of the GNU Lesser General Public
--  License along with this library; if not, write to the Free Software
--  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
--  02110-1301  USA
--
-- ----------------------------------------------------------------------------
--  Copyright (C) 2010 Braden "Blzut3" Obrzut <admin@maniacsvault.net>
-- ----------------------------------------------------------------------------

-- Automatically executed from makemacdmg.sh
-- Not to be run alone.

on run
	tell application "Finder"
		tell disk "image"
			open
			set options to icon view options of container window
			tell options
					set icon size to 96
					set arrangement to not arranged
			end tell
			set background picture of options to file ".background:background.png"
			tell container window
				set the bounds to {0, 0, 500, 350}
				set current view to icon view
				set toolbar visible to false
				set statusbar visible to false
			end tell

			make new alias file to POSIX file "/Applications" at "image" with properties {name:"Applications"}

			set position of item "Doomseeker.app" to {75, 150}
			set position of item "Applications" to {425, 150}
			close
		end tell
	end tell
end run