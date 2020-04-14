// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	C_BIND
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "doomdef.h"
#include "m_ostring.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "g_level.h"
#include "hu_stuff.h"
#include "cl_demo.h"
#include "d_player.h"
#include "i_input.h"

extern NetDemo netdemo;

/* Most of these bindings are equivalent
 * to the original DOOM's keymappings.
 */
char DefBindings[] =
	"bind grave toggleconsole; "			// <- This is new
	"bind 1 \"impulse 1\"; "
	"bind 2 \"impulse 2\"; "
	"bind 3 \"impulse 3\"; "
	"bind 4 \"impulse 4\"; "
	"bind 5 \"impulse 5\"; "
	"bind 6 \"impulse 6\"; "
	"bind 7 \"impulse 7\"; "
	"bind 8 \"impulse 8\"; "
	"bind - sizedown; "
	"bind = sizeup; "
	"bind leftctrl +attack; "
	"bind leftalt +strafe; "	
	"bind leftshift +speed; "
	"bind rightshift +speed; "
	"bind space +use; "
	"bind e +use; "
	"bind rightarrow +right; "
	"bind leftarrow +left; "
	"bind w +forward; "
	"bind s +back; "
	"bind a +moveleft; "
	"bind d +moveright; "
#ifdef _XBOX // Alternative defaults for Xbox
	"bind hat1right messagemode2; "
	"bind hat1left spynext; "
	"bind hat1up messagemode; "
	"bind hat1down \"impulse 3\"; "
	"bind joy1 +use; "
	"bind joy2 weapnext; "
	"bind joy3 +jump; "
	"bind joy4 weapprev; "
	"bind joy5 togglemap; "
	"bind joy6 +showscores; "
	"bind joy7 +speed; "
	"bind joy8 +attack; "
	"bind joy10 toggleconsole; "
	"bind joy12 centerview; "
#else
	"bind mouse1 +attack; "
	"bind mouse2 +strafe; "
	"bind mouse3 +forward; "
	"bind mouse4 +jump; "				// <- So is this <- change to jump
	"bind mouse5 +speed; "				// <- new for +speed
	"bind joy1 +attack; "
	"bind joy2 +strafe; "
	"bind joy3 +speed; "
	"bind joy4 +use; "
	"bind mwheelup  weapprev; "
	"bind mwheeldown weapnext; "
#endif
	"bind f1 menu_help; "
	"bind f2 menu_save; "
	"bind f3 menu_load; "
	"bind f4 menu_options; "			// <- Since we don't have a separate sound menu anymore
	"bind f5 menu_display; "			// <- More useful than just changing the detail level
	"bind f6 quicksave; "
	"bind f7 menu_endgame; "
	"bind f8 togglemessages; "
	"bind f9 quickload; "
	"bind f10 menu_quit; "
	"bind tab togglemap; "
	"bind pause pause; "
	"bind sysrq screenshot; "			// <- Also known as the Print Screen key
	"bind t messagemode; "
	"bind enter messagemode; "
	"bind y messagemode2; "
	"bind \\\\ +showscores; "				// <- Another new command
	"bind f11 bumpgamma; "
	"bind f12 spynext; "
	"bind pgup vote_yes; "				// <- New for voting
	"bind pgdn vote_no; "				// <- New for voting
	"bind home ready; "
	"bind end spectate; "
	"bind m changeteams ";



static std::string Bindings[NUM_KEYS];
static std::string DoubleBindings[NUM_KEYS];
static std::string NetDemoBindings[NUM_KEYS];
static int DClickTime[NUM_KEYS];
static byte DClicked[(NUM_KEYS+7)/8];
static bool KeysDown[NUM_KEYS];


BEGIN_COMMAND (unbindall)
{
	for (int key = 0; key < NUM_KEYS; key++)
		Bindings[key] = "";

	for (key = 0; key < NUM_KEYS; key++)
		DoubleBindings[key] = "";
}
END_COMMAND (unbindall)


BEGIN_COMMAND (unbind)
{
	if (argc > 1)
	{
		std::string strArg = StdStringToLower(argv[1]);
		int key = I_GetKeyFromName(strArg);
		if (key)
			Bindings[key] = "";
		else
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
	}
}
END_COMMAND (unbind)


BEGIN_COMMAND (bind)
{
	if (argc > 1) {

		std::string strArg = StdStringToLower(argv[1]);
		int key = I_GetKeyFromName(strArg);
		if (!key)
		{
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
			return;
		}
		if (argc == 2)
		{
			Printf(PRINT_HIGH, "%s = %s\n", strArg.c_str(), C_QuoteString(Bindings[key]).c_str());
		}
		else
		{
			Bindings[key] = argv[2];
		}
	}
	else
	{
		Printf(PRINT_HIGH, "Current key bindings:\n");

		for (int key = 0; key < NUM_KEYS; key++)
		{
			if (Bindings[key].length())
				Printf(PRINT_HIGH, "%s %s\n", I_GetKeyName(key), C_QuoteString(Bindings[key]).c_str());
		}
	}
}
END_COMMAND (bind)


BEGIN_COMMAND (undoublebind)
{
	if (argc > 1)
	{
		std::string strArg = StdStringToLower(argv[1]);
		int key = I_GetKeyFromName(strArg);
		if (key)
			DoubleBindings[key] = "";
		else
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
	}
}
END_COMMAND (undoublebind)


BEGIN_COMMAND (doublebind)
{
	if (argc > 1)
	{
		std::string strArg = StdStringToLower(argv[1]);
		int key = I_GetKeyFromName(strArg);
		if (!key)
		{
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
			return;
		}
		if (argc == 2)
		{
			Printf(PRINT_HIGH, "%s = %s\n", strArg.c_str(), C_QuoteString(DoubleBindings[key]).c_str());
		}
		else
		{
			DoubleBindings[key] = argv[2];
		}
	}
	else
	{
		Printf(PRINT_HIGH, "Current key doublebindings:\n");

		for (key = 0; key < NUM_KEYS; key++)
		{
			if (DoubleBindings[key].length())
				Printf(PRINT_HIGH, "%s %s\n", I_GetKeyName(key), C_QuoteString(DoubleBindings[key]).c_str());
		}
	}
}
END_COMMAND (doublebind)


BEGIN_COMMAND (binddefaults)
{
	AddCommandString (DefBindings);
}
END_COMMAND (binddefaults)


//
// C_DoNetDemoKey
//
// [SL] 2012-03-29 - Handles the hard-coded key bindings used during
// NetDemo playback.  Returns false if the key pressed is not
// bound to any netdemo command.
//
bool C_DoNetDemoKey (event_t *ev)
{
	if (!netdemo.isPlaying() && !netdemo.isPaused())
		return false;

	static bool initialized = false;
	std::string *binding;

	if (!initialized)
	{
		NetDemoBindings[I_GetKeyFromName("leftarrow")] = "netrew";
		NetDemoBindings[I_GetKeyFromName("rightarrow")] = "netff";
		NetDemoBindings[I_GetKeyFromName("uparrow")] = "netprevmap";
		NetDemoBindings[I_GetKeyFromName("downarrow")] = "netnextmap";
		NetDemoBindings[I_GetKeyFromName("space")] = "netpause";

		initialized = true;
	}

	if (ev->type != ev_keydown && ev->type != ev_keyup)
		return false;

	binding = &NetDemoBindings[ev->data1];

	// hardcode the pause key to also control netpause
	if (iequals(Bindings[ev->data1], "pause"))
		binding = &NetDemoBindings[I_GetKeyFromName("space")];

	// nothing bound to this key specific to netdemos?
	if (binding->empty())
		return false;

	if (ev->type == ev_keydown)
		AddCommandString(*binding, ev->data1);

	return true;
}

//
// C_DoSpectatorKey
//
// [SL] 2012-09-14 - Handles the hard-coded key bindings used while spectating
// or during NetDemo playback.  Returns false if the key pressed is not
// bound to any spectating command such as spynext.
//
bool C_DoSpectatorKey (event_t *ev)
{
	if (!consoleplayer().spectator && !netdemo.isPlaying() && !netdemo.isPaused())
		return false;

	if (ev->type == ev_keydown && ev->data1 == KEY_MWHEELUP)
	{
		AddCommandString("spyprev", ev->data1);
		return true;
	}
	if (ev->type == ev_keydown && ev->data1 == KEY_MWHEELDOWN)
	{
		AddCommandString("spynext", ev->data1);
		return true;
	}

	return false;
}

BOOL C_DoKey (event_t *ev)
{
	if (ev->type != ev_keydown && ev->type != ev_keyup)
		return false;

	std::string *binding = NULL;

	int dclickspot = ev->data1 >> 3;
	byte dclickmask = 1 << (ev->data1 & 7);

	if (DClickTime[ev->data1] > level.time && ev->type == ev_keydown)
	{
		// Key pressed for a double click
		binding = &DoubleBindings[ev->data1];
		DClicked[dclickspot] |= dclickmask;
	}
	else
	{
		if (ev->type == ev_keydown) {
			// Key pressed for a normal press
			binding = &Bindings[ev->data1];
			DClickTime[ev->data1] = level.time + 20;
		} else if (DClicked[dclickspot] & dclickmask) {
			// Key released from a double click
			binding = &DoubleBindings[ev->data1];
			DClicked[dclickspot] &= ~dclickmask;
			DClickTime[ev->data1] = 0;
		} else {
			// Key released from a normal press
			binding = &Bindings[ev->data1];
		}
	}

	if (!binding->length())
		binding = &Bindings[ev->data1];

	if (binding->length() && (HU_ChatMode() == CHAT_INACTIVE || ev->data1 < 256))
	{
		if (ev->type == ev_keydown)
		{
			AddCommandString(*binding, ev->data1);
			KeysDown[ev->data1] = true;
		}
		else
		{
			size_t achar = binding->find_first_of('+');

			if (achar == std::string::npos)
				return false;

			if (achar == 0 || (*binding)[achar - 1] <= ' ')
			{
				(*binding)[achar] = '-';
				AddCommandString(*binding, ev->data1);
				(*binding)[achar] = '+';
			}

			KeysDown[ev->data1] = false;
		}
		return true;
	}
	return false;
}

//
// C_ReleaseKeys
//
// Calls the key-release action for all bound keys that are currently
// being held down.
//
void C_ReleaseKeys()
{
	for (int i = 0; i < NUM_KEYS; i++)
	{
		if (!KeysDown[i])
			continue;

		KeysDown[i] = false;
		std::string *binding = &Bindings[i];
		if (binding->empty())
			continue;

		size_t achar = binding->find_first_of('+');

		if (achar != std::string::npos &&
			(achar == 0 || (*binding)[achar - 1] <= ' '))
		{
			(*binding)[achar] = '-';
			AddCommandString(*binding, i);
			(*binding)[achar] = '+';
		}
	}

	HU_ReleaseKeyStates();
}

void C_ArchiveBindings (FILE *f)
{
	fprintf(f, "unbindall\n");
	for (int key = 0; key < NUM_KEYS; key++)
	{
		if (Bindings[key].length())
			fprintf(f, "bind %s %s\n", C_QuoteString(I_GetKeyName(key)).c_str(), C_QuoteString(Bindings[key]).c_str());
	}
	for (int key = 0; key < NUM_KEYS; key++)
	{
		if (DoubleBindings[key].length())
			fprintf(f, "doublebind %s %s\n", C_QuoteString(I_GetKeyName(key)).c_str(), C_QuoteString(DoubleBindings[key]).c_str());
	}
}

int C_GetKeysForCommand (const char *cmd, int *first, int *second)
{
	int c = 0;
	*first = *second = 0;

	for (int key = 0; key < NUM_KEYS && c < 2; key++)
	{
		if (Bindings[key].length() && !stricmp(cmd, Bindings[key].c_str()))
		{
			if (c++ == 0)
				*first = key;
			else
				*second = key;
		}
	}
	return c;
}

std::string C_NameKeys (int first, int second)
{
	if(!first && !second)
		return "???";

	std::string out;

	if (first)
	{
		out += I_GetKeyName(first);
		if (second)
			out += " or ";
	}

	if (second)
	{
		out += I_GetKeyName(second);
	}

	return out;
}

void C_UnbindACommand (const char *str)
{
	for (int key = 0; key < NUM_KEYS; key++) 
	{
		if (Bindings[key].length() && !stricmp(str, Bindings[key].c_str()))
			Bindings[key] = "";
	}
}

void C_ChangeBinding (const char *str, int newone)
{
	// Check which bindings that are already set. If both binding slots are taken,
	// erase all bindings and reassign the new one and the secondary binding to the key instead.
	int first = -1;
	int second = -1;

	C_GetKeysForCommand(str, &first, &second);

	if (newone == first || newone == second)
	{
		return;
	}
	else if (first > -1 && second > -1)
	{
		C_UnbindACommand(str);
		Bindings[newone] = str;
		Bindings[second] = str;
	}
	else
	{
		Bindings[newone] = str;
	}
}

const char *C_GetBinding (int key)
{
	return Bindings[key].c_str();
}

/*
C_GetKeyStringsFromCommand
Finds binds from a command and returns it into a std::string .
- If TRUE, second arg returns up to 2 keys. ("x OR y")
*/
std::string C_GetKeyStringsFromCommand(const char *cmd, bool bTwoEntries)
{
	int first = -1;
	int second = -1;

	C_GetKeysForCommand(cmd, &first, &second);

	if (!first && !second)
		return "<??\?>";

	if (bTwoEntries)
		return C_NameKeys(first, second);
	else
	{
		if (!first && second)
			return I_GetKeyName(second);
		else
			return I_GetKeyName(first);
	}
	return "<??\?>";
}


VERSION_CONTROL (c_bind_cpp, "$Id$")
