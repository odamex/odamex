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
#include "hashtable.h"
#include "cl_responderkeys.h"

extern NetDemo netdemo;

/* Most of these bindings are equivalent
 * to the original DOOM's keymappings.
 */
char DefBindings[] =
	"bind grave toggleconsole; "
	"bind tilde toggleconsole; "
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
	"bind m changeteams; ";


typedef OHashTable<int, std::string> BindingTable;
static BindingTable Bindings;
static BindingTable DoubleBindings;
static BindingTable NetDemoBindings;


struct KeyState
{
	KeyState() :
		double_click_time(0), double_clicked(false), key_down(false)
	{}

	int double_click_time;
	bool double_clicked;
	bool key_down;
};

typedef OHashTable<int, KeyState> KeyStateTable;
static KeyStateTable KeyStates;


BEGIN_COMMAND (unbindall)
{
	Bindings.clear();
	DoubleBindings.clear();
}
END_COMMAND (unbindall)


BEGIN_COMMAND (unbind)
{
	if (argc > 1)
	{
		int key = I_GetKeyFromName(StdStringToLower(argv[1]));
		if (key)
			Bindings.erase(key);
		else
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
	}
}
END_COMMAND (unbind)


BEGIN_COMMAND (bind)
{
	if (argc > 1)
	{
		std::string key_name = StdStringToLower(argv[1]);
		int key = I_GetKeyFromName(key_name);
		if (key)
		{
			if (argc == 2)
				Printf(PRINT_HIGH, "%s = %s\n", key_name.c_str(), C_QuoteString(Bindings[key]).c_str());
			else
				Bindings[key] = argv[2];
		}
		else
		{
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
		}
	}
	else
	{
		Printf(PRINT_HIGH, "Current key bindings:\n");
		for (BindingTable::const_iterator it = Bindings.begin(); it != Bindings.end(); ++it)
		{
			int key = it->first;
			const std::string& binding = it->second;
			if (!binding.empty())
				Printf(PRINT_HIGH, "%s %s\n", I_GetKeyName(key).c_str(), C_QuoteString(binding).c_str());
		}
	}
}
END_COMMAND (bind)


BEGIN_COMMAND (undoublebind)
{
	if (argc > 1)
	{
		int key = I_GetKeyFromName(StdStringToLower(argv[1]));
		if (key)
			DoubleBindings.erase(key);
		else
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
	}
}
END_COMMAND (undoublebind)


BEGIN_COMMAND (doublebind)
{
	if (argc > 1)
	{
		std::string key_name = StdStringToLower(argv[1]);
		int key = I_GetKeyFromName(key_name);
		if (key)
		{
			if (argc == 2)
				Printf(PRINT_HIGH, "%s = %s\n", key_name.c_str(), C_QuoteString(DoubleBindings[key]).c_str());
			else
				DoubleBindings[key] = argv[2];
		}
		else
		{
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
		}
	}
	else
	{
		Printf(PRINT_HIGH, "Current key doublebindings:\n");
		for (BindingTable::const_iterator it = DoubleBindings.begin(); it != DoubleBindings.end(); ++it)
		{
			int key = it->first;
			const std::string& binding = it->second;
			if (!binding.empty())
				Printf(PRINT_HIGH, "%s %s\n", I_GetKeyName(key).c_str(), C_QuoteString(binding).c_str());
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

	if (ev->type == ev_keydown && keypress.IsSpyPrevKey(ev->data1))
	{
		AddCommandString("spyprev", ev->data1);
		return true;
	}
	if (ev->type == ev_keydown && keypress.IsSpyNextKey(ev->data1))
	{
		AddCommandString("spynext", ev->data1);
		return true;
	}

	return false;
}


BOOL C_DoKey(event_t *ev)
{
	if (ev->type != ev_keydown && ev->type != ev_keyup)
		return false;

	const std::string* binding = NULL;
	int key = ev->data1;

	KeyState& key_state = KeyStates[key];
	if (ev->type == ev_keydown && key_state.double_click_time > level.time)
	{
		// Key pressed for a double click
		binding = &DoubleBindings[key];
		key_state.double_clicked = true;
	}
	else
	{
		if (ev->type == ev_keydown)
		{
			// Key pressed for a normal press
			binding = &Bindings[key];
			key_state.double_click_time = level.time + 20;
		}
		else if (key_state.double_clicked)
		{
			// Key released from a double click
			binding = &DoubleBindings[key];
			key_state.double_click_time = 0;
			key_state.double_clicked = false;
		} else {
			// Key released from a normal press
			binding = &Bindings[key];
		}
	}

	if (binding->empty())
		binding = &Bindings[key];

	if (!binding->empty() && (HU_ChatMode() == CHAT_INACTIVE || key < 256))
	{
		if (ev->type == ev_keydown)
		{
			key_state.key_down = true;
			AddCommandString(*binding, key);
		}
		else if (ev->type == ev_keyup)
		{
			key_state.key_down = false;

			size_t achar = binding->find_first_of('+');
			if (achar == std::string::npos)
				return false;

			if (achar == 0 || (*binding)[achar - 1] <= ' ')
			{
				std::string action_release(*binding);
				action_release[achar] = '-';
				AddCommandString(action_release, key);
			}
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
	for (KeyStateTable::iterator it = KeyStates.begin(); it != KeyStates.end(); ++it)
	{
		int key = it->first;
		KeyState& key_state = it->second;
		if (key_state.key_down)
		{
			key_state.key_down = false;
			std::string *binding = &Bindings[key];
			if (!binding->empty())
			{
				size_t achar = binding->find_first_of('+');
				if (achar != std::string::npos && (achar == 0 || (*binding)[achar - 1] <= ' '))
				{
					std::string action_release(*binding);
					action_release[achar] = '-';
					AddCommandString(action_release, key);
				}
			}
		}
	}

	HU_ReleaseKeyStates();
}


void C_ArchiveBindings (FILE *f)
{
	fprintf(f, "unbindall\n");

	for (BindingTable::const_iterator it = Bindings.begin(); it != Bindings.end(); ++it)
	{
		int key = it->first;
		const std::string& binding = it->second;
		if (!binding.empty())
			fprintf(f, "bind %s %s\n", C_QuoteString(I_GetKeyName(key)).c_str(), C_QuoteString(binding).c_str());
	}

	for (BindingTable::const_iterator it = DoubleBindings.begin(); it != DoubleBindings.end(); ++it)
	{
		int key = it->first;
		const std::string& binding = it->second;
		if (!binding.empty())
			fprintf(f, "doublebind %s %s\n", C_QuoteString(I_GetKeyName(key)).c_str(), C_QuoteString(binding).c_str());
	}
}


int C_GetKeysForCommand (const char* cmd, int* first, int* second)
{
	int c = 0;
	*first = *second = 0;

	for (BindingTable::const_iterator it = Bindings.begin(); it != Bindings.end(); ++it)
	{
		int key = it->first;
		const std::string& binding = it->second;
		if (!binding.empty() && stricmp(cmd, binding.c_str()) == 0)
		{
			c++;
			if (c == 1)
			{
				*first = key;
			}
			else
			{
				*second = key;
				break;
			}
		}
	}
	return c;
}


std::string C_NameKeys(int first, int second)
{
	if (!first && !second)
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


void C_UnbindACommand(const char* str)
{
	for (BindingTable::iterator it = Bindings.begin(); it != Bindings.end(); ++it)
	{
		const std::string& binding = it->second;
		if (!binding.empty() && stricmp(str, binding.c_str()) == 0)
		{
			Bindings.erase(it);
			it = Bindings.begin();		// restart iteration since the container was modified during iteration
		}
	}
}


void C_ChangeBinding (const char *str, int newone)
{
	// Check which bindings that are already set. If both binding slots are taken,
	// erase all bindings and reassign the new one and the secondary binding to the key instead.
	int first = 0;
	int second = 0;

	C_GetKeysForCommand(str, &first, &second);

	if (newone == first || newone == second)
	{
		return;
	}
	else if (first > 0 && second > 0)
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
