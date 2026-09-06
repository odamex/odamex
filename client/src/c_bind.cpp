// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2026 by The Odamex Team.
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


#include "odamex.h"

#include <stdlib.h>

#include "m_ostring.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "hu_stuff.h"
#include "cl_demo.h"
#include "d_player.h"
#include "i_input.h"
#include "hashtable.h"
#include "g_gametype.h"
#include "cl_responderkeys.h"

extern NetDemo netdemo;

/* Most of these bindings are equivalent
 * to the original DOOM's keymappings.
 */
constexpr std::array DefaultBindings = std::to_array<OBinding>(
{
	{.Key = "tilde",      .Bind = "toggleconsole" },
	{.Key = "grave",      .Bind = "toggleconsole" }, // <- This is new
	{.Key = "1",          .Bind = "impulse 1"     },
	{.Key = "2",          .Bind = "impulse 2"     },
	{.Key = "3",          .Bind = "impulse 3"     },
	{.Key = "4",          .Bind = "impulse 4"     },
	{.Key = "5",          .Bind = "impulse 5"     },
	{.Key = "6",          .Bind = "impulse 6"     },
	{.Key = "7",          .Bind = "impulse 7"     },
	{.Key = "8",          .Bind = "impulse 8"     },
	{.Key = "-",          .Bind = "sizedown"      },
	{.Key = "=",          .Bind = "sizeup"        },
	{.Key = "leftctrl",   .Bind = "+attack"       },
	{.Key = "leftalt",    .Bind = "+strafe"       },
	{.Key = "leftshift",  .Bind = "+speed"        },
	{.Key = "rightshift", .Bind = "+speed"        },
	{.Key = "capslock",   .Bind = "togglerun"     },
	{.Key = "space",      .Bind = "+use"          },
	{.Key = "e",          .Bind = "+use"          },
	{.Key = "uparrow",    .Bind = "+forward"      },
	{.Key = "downarrow",  .Bind = "+back"         },
	{.Key = "rightarrow", .Bind = "+right"        },
	{.Key = "leftarrow",  .Bind = "+left"         },
	{.Key = "w",          .Bind = "+forward"      },
	{.Key = "s",          .Bind = "+back"         },
	{.Key = "a",          .Bind = "+moveleft"     },
	{.Key = "d",          .Bind = "+moveright"    },
#ifdef GCONSOLE
	{.Key = "hat1right",  .Bind = "messagemode2"  },
	{.Key = "hat1left",   .Bind = "spynext"       },
	{.Key = "hat1up",     .Bind = "messagemode"   },
	{.Key = "hat1down",   .Bind = "impulse 3"     },
	{.Key = "joy1",       .Bind = "+use"          },
	{.Key = "joy2",       .Bind = "weapnext"      },
	{.Key = "joy3",       .Bind = "+jump"         },
	{.Key = "joy4",       .Bind = "weapprev"      },
	{.Key = "joy5",       .Bind = "togglemap"     },
	{.Key = "joy6",       .Bind = "+showscores"   },
	{.Key = "joy7",       .Bind = "+speed"        },
	{.Key = "joy8",       .Bind = "+attack"       },
	{.Key = "joy10",      .Bind = "toggleconsole" },
	{.Key = "joy12",      .Bind = "centerview"    },
#else
	{.Key = "mouse1",     .Bind = "+attack"       },
	{.Key = "mouse2",     .Bind = "+strafe"       },
	{.Key = "mouse3",     .Bind = "+forward"      },
	{.Key = "mouse4",     .Bind = "+jump"         }, // <- So is this <- change to jump
	{.Key = "mouse5",     .Bind = "+speed"        }, // <- new for +speed
	{.Key = "joy1",       .Bind = "+jump"         },
	{.Key = "joy2",       .Bind = "+use"          },
	{.Key = "joy5",       .Bind = "+showscores"   },
	{.Key = "joy8",       .Bind = "togglemap"     },
	{.Key = "joy9",       .Bind = "ready"         },
	{.Key = "joy10",      .Bind = "weapprev"      },
	{.Key = "joy11",      .Bind = "weapnext"      },
	{.Key = "joy20",      .Bind = "+use"          },
	{.Key = "joy21",      .Bind = "+attack"       },
	{.Key = "mwheelup",   .Bind = "weapprev"      },
	{.Key = "mwheeldown", .Bind = "weapnext"      },
#endif
	{.Key = "f1",         .Bind = "menu_help"     },
	{.Key = "f2",         .Bind = "menu_save"     },
	{.Key = "f3",         .Bind = "menu_load"     },
	{.Key = "f4",         .Bind = "menu_options"  }, // <- Since we don't have a separate sound menu anymore
	{.Key = "f5",         .Bind = "menu_display"  }, // <- More useful than just changing the detail level
	{.Key = "f6",         .Bind = "quicksave"     },
	{.Key = "f7",         .Bind = "menu_endgame"  },
	{.Key = "f8",         .Bind = "togglemessages"},
	{.Key = "f9",         .Bind = "quickload"     },
	{.Key = "f10",        .Bind = "menu_quit"     },
	{.Key = "tab",        .Bind = "togglemap"     },
	{.Key = "pause",      .Bind = "pause"         },
	{.Key = "sysrq",      .Bind = "screenshot"    }, // <- Also known as the Print Screen key
	{.Key = "print",      .Bind = "screenshot"    }, // <- AZERTY equivalent
	{.Key = "t",          .Bind = "messagemode"   },
	{.Key = "enter",      .Bind = "messagemode"   },
	{.Key = "y",          .Bind = "messagemode2"  },
	{.Key = "\\",         .Bind = "+showscores"   }, // <- Another new command
	{.Key = "f11",        .Bind = "bumpgamma"     },
	{.Key = "f12",        .Bind = "spynext"       },
	{.Key = "pgup",       .Bind = "vote_yes"      }, // <- New for voting
	{.Key = "pgdn",       .Bind = "vote_no"       }, // <- New for voting
	{.Key = "home",       .Bind = "ready"         },
	{.Key = "end",        .Bind = "spectate"      },
	{.Key = "m",          .Bind = "changeteams"   },
});

/* Special bindings when it comes
 * to Odamex's demo playbacking.
 */
constexpr std::array DefaultNetDemoBindings = std::to_array<OBinding>(
{
	{.Key = "leftarrow",  .Bind = "netrew"      },
	{.Key = "rightarrow", .Bind = "netff"       },
	{.Key = "uparrow",    .Bind = "netprevmap"  },
	{.Key = "downarrow",  .Bind = "netnextmap"  },
	{.Key = "space",      .Bind = "netpause"    },
	{.Key = "pgup",       .Bind = "netprotoup"  },
	{.Key = "pgdn",       .Bind = "netprotodown"},
});

/* Special bindings for the automap
 */
constexpr std::array DefaultAutomapBindings = std::to_array<OBinding>(
{
	{.Key = "g",          .Bind = "am_grid"        },
	{.Key = "m",          .Bind = "am_setmark"     },
	{.Key = "c",          .Bind = "am_clearmarks"  },
	{.Key = "f",          .Bind = "am_togglefollow"},
	{.Key = "=",          .Bind = "+am_zoomin"     },
	{.Key = "kp+",        .Bind = "+am_zoomin"     },
	{.Key = "-",          .Bind = "+am_zoomout"    },
	{.Key = "kp-",        .Bind = "+am_zoomout"    },
	{.Key = "uparrow",    .Bind = "+am_panup"      },
	{.Key = "downarrow",  .Bind = "+am_pandown"    },
	{.Key = "leftarrow",  .Bind = "+am_panleft"    },
	{.Key = "rightarrow", .Bind = "+am_panright"   },
	{.Key = "0",          .Bind = "am_big"         },
});

OKeyBindings Bindings, DoubleBindings, AutomapBindings, NetDemoBindings;

struct KeyState
{
	int double_click_time = 0;
	bool double_clicked = false;
	bool key_down = false;
};

using KeyStateTable = OHashTable<int, KeyState>;
static KeyStateTable KeyStates;


void OKeyBindings::SetBindingType(IString cmd)
{
	command = std::move(cmd);
}

void OKeyBindings::UnbindKey(IStringView keyname)
{
	const auto keycode = I_GetKeyFromName(keyname);

	if (keycode)
		Binds.erase(keycode.value());
	else
		PrintFmt(PRINT_WARNING, "Unknown key {:s}\n", C_QuoteString(IStringToStdStringView(keyname)));
}

void OKeyBindings::UnbindAll()
{
	this->Binds.clear();
}

void OKeyBindings::BindAKey(size_t argc, char** argv, const char* msg)
{
	if (argc > 1)
	{
		IString key_name = argv[1];
		auto key = I_GetKeyFromName(key_name);
		if (!key)
		{
			PrintFmt(PRINT_HIGH, "Unknown key {:s}\n", C_QuoteString(argv[1]));
		}
		else
		{
			if (argc == 2)
				PrintFmt(PRINT_HIGH, "{:s} = {:s}\n", key_name, C_QuoteString(IStringToStdStringView(Binds[key.value()])));
			else
				Binds[key.value()] = argv[2];
		}
	}
	else
	{
		PrintFmt(PRINT_HIGH, "{:s}\n", msg);
		for (const auto& [key, binding] : Binds)
		{
			if (!binding.empty())
				PrintFmt(PRINT_HIGH, "{:s} = {:s}\n", I_GetKeyName(key), C_QuoteString(IStringToStdStringView(binding)));
		}
	}
}

void OKeyBindings::DoBind(IStringView key, const char* bind)
{
	auto keynum = I_GetKeyFromName(key);
	if (keynum)
	{
		this->Binds[keynum.value()] = bind;
	}
}

void OKeyBindings::SetBinds(std::span<const OBinding> binds)
{
	for (const auto& [key, bind] : binds)
	{
		DoBind(key, bind);
	}
}


//
// C_DoNetDemoKey
//
// [SL] 2012-03-29 - Handles the hard-coded key bindings used during
// NetDemo playback.  Returns false if the key pressed is not
// bound to any netdemo command.
//
bool C_DoNetDemoKey(const event_t& ev)
{
	if (not netdemo.isInPlayback())
		return false;

	const IString *binding = nullptr;

	if (ev.type != ev_keydown && ev.type != ev_keyup)
		return false;

	binding = &NetDemoBindings.Binds[ev.data1];

	// hardcode the pause key to also control netpause
	if (iequals(Bindings.Binds[ev.data1], "pause"))
		binding = &NetDemoBindings.Binds[I_GetKeyFromName("space").value()];

	// nothing bound to this key specific to netdemos?
	if (binding->empty())
		return false;

	if (ev.type == ev_keydown)
		AddCommandString(IStringToStdStringView(*binding), ev.data1);

	return true;
}


//
// C_DoSpectatorKey
//
// [SL] 2012-09-14 - Handles the hard-coded key bindings used while spectating
// or during NetDemo playback.  Returns false if the key pressed is not
// bound to any spectating command such as spynext.
//
bool C_DoSpectatorKey (const event_t& ev)
{
	if (G_IsLivesGame())
	{
		if (!consoleplayer().spectator && consoleplayer().lives > 0 &&
		    !netdemo.isInPlayback())
		return false;
	}
	else
	{
		if (!consoleplayer().spectator && !netdemo.isInPlayback())
		return false;
	}

	if (ev.type == ev_keydown && Key_IsSpyPrevKey(ev.data1))
	{
		AddCommandString("spyprev", ev.data1);
		return true;
	}
	if (ev.type == ev_keydown && Key_IsSpyNextKey(ev.data1))
	{
		AddCommandString("spynext", ev.data1);
		return true;
	}

	return false;
}


bool C_DoKey(const event_t& ev, OKeyBindings* binds, OKeyBindings* doublebinds)
{
	if (ev.type != ev_keydown && ev.type != ev_keyup)
		return false;

	const IString* binding = NULL;
	int key = ev.data1;

	KeyState& key_state = KeyStates[key];
	if (doublebinds != NULL && ev.type == ev_keydown && key_state.double_click_time > level.time)
	{
		// Key pressed for a double click
		binding = &doublebinds->Binds[key];
		key_state.double_clicked = true;
	}
	else
	{
		if (ev.type == ev_keydown)
		{
			// Key pressed for a normal press
			binding = &binds->Binds[key];
			key_state.double_click_time = level.time + 20;
		}
		else if (doublebinds != NULL && key_state.double_clicked)
		{
			// Key released from a double click
			binding = &doublebinds->Binds[key];
			key_state.double_click_time = 0;
			key_state.double_clicked = false;
		} else {
			// Key released from a normal press
			binding = &binds->Binds[key];
		}
	}

	if (binding->empty())
		binding = &binds->Binds[key];

	if (!binding->empty() && (HU_ChatMode() == CHAT_INACTIVE || key < 256))
	{
		if (ev.type == ev_keydown)
		{
			AddCommandString(IStringToStdStringView(*binding), key);
			key_state.key_down = true;
		}
		else if (ev.type == ev_keyup)
		{
			key_state.key_down = false;

			size_t achar = binding->find_first_of('+');
			if (achar == std::string::npos)
				return false;

			if (achar == 0 || (*binding)[achar - 1] <= ' ')
			{
				IString action_release(*binding);
				action_release[achar] = '-';
				AddCommandString(IStringToStdStringView(action_release), key);
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
	for (auto& [key, key_state] : KeyStates)
	{
		if (key_state.key_down)
		{
			key_state.key_down = false;
			IString *binding = &Bindings.Binds[key];
			if (!binding->empty())
			{
				size_t achar = binding->find_first_of('+');
				if (achar != IString::npos && (achar == 0 || (*binding)[achar - 1] <= ' '))
				{
					IString action_release(*binding);
					action_release[achar] = '-';
					AddCommandString(IStringToStdStringView(action_release), key);
				}
			}
		}
	}

	HU_ReleaseKeyStates();
}

void OKeyBindings::ArchiveBindings(FILE* f)
{
	for (const auto& [key, binding] : Binds)
	{
		if (!binding.empty())
			fmt::print(f, "{} {} {}\n", command, C_QuoteString(I_GetKeyName(key)), C_QuoteString(IStringToStdStringView(binding)));
	}
}


int OKeyBindings::GetKeysForCommand(const char* cmd, int* first, int* second)
{
	int c = 0;
	*first = *second = 0;

	for (const auto& [key, binding] : Binds)
	{
		if (!binding.empty() && binding == cmd)
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


namespace
{

//
// C_KeyMatchesDevice
//
// Keyboard and mouse binds are interchangeable here - a player on either one
// is not looking for a gamepad prompt.
//
bool C_KeyMatchesDevice(int key, keydevice_t device)
{
	return (I_GetKeyDevice(key) == KEYDEV_JOYSTICK) == (device == KEYDEV_JOYSTICK);
}

} // namespace

//
// OKeyBindings::GetKeysForCommandByLastDevice
//
// Returns every key bound to cmd, with the keys belonging to the device the
// player used last placed first.
//
std::vector<int> OKeyBindings::GetKeysForCommandByLastDevice(const char* cmd)
{
	const keydevice_t device = I_GetLastInputDevice();

	std::vector<int> keys;
	std::vector<int> other_keys;

	for (const auto& [key, binding] : Binds)
	{
		if (binding.empty() || binding.c_str() != cmd)
			continue;

		if (C_KeyMatchesDevice(key, device))
			keys.push_back(key);
		else
			other_keys.push_back(key);
	}

	keys.insert(keys.end(), other_keys.begin(), other_keys.end());
	return keys;
}


std::string OKeyBindings::GetNameKeys(int first, int second)
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


void OKeyBindings::UnbindACommand(const char* str)
{
	for (BindingTable::iterator it = Binds.begin(); it != Binds.end(); ++it)
	{
		const IString& binding = it->second;
		if (!binding.empty() && stricmp(str, binding.c_str()) == 0)
		{
			Binds.erase(it);
			it = Binds.begin();		// restart iteration since the container was modified during iteration
		}
	}
}


void OKeyBindings::ChangeBinding (const char *str, int newone)
{
	// Check which bindings that are already set. If both binding slots are taken,
	// erase all bindings and reassign the new one and the secondary binding to the key instead.
	int first = 0;
	int second = 0;

	GetKeysForCommand(str, &first, &second);

	if (newone == first || newone == second)
	{
		return;
	}
	else if (first > 0 && second > 0)
	{
		UnbindACommand(str);
		Binds[newone] = str;
		Binds[second] = str;
	}
	else
	{
		Binds[newone] = str;
	}
}


const IString &OKeyBindings::GetBind (int key)
{
	return Binds[key];
}

/*
C_GetKeyStringsFromCommand
Finds binds from a command and returns it into a std::string.
Prefers the bind on the input device the player used last.
- If TRUE, second arg returns up to 2 keys. ("x OR y")
*/
std::string OKeyBindings::GetKeynameFromCommand(const char* cmd, bool bTwoEntries)
{
	const std::vector<int> keys = GetKeysForCommandByLastDevice(cmd);

	if (keys.empty())
		return "<??\?>";

	if (bTwoEntries)
		return GetNameKeys(keys[0], keys.size() > 1 ? keys[1] : 0);

	return I_GetKeyName(keys[0]);
}


void C_BindingsInit()
{
	Bindings.SetBindingType("bind");
	DoubleBindings.SetBindingType("doublebind");
	AutomapBindings.SetBindingType("ambind");
	NetDemoBindings.SetBindingType("netdemobind");
}

// Bind default bindings
void C_BindDefaults()
{
	Bindings.SetBinds(DefaultBindings);
	AutomapBindings.SetBinds(DefaultAutomapBindings);
	NetDemoBindings.SetBinds(DefaultNetDemoBindings);
}


// Bind command
BEGIN_COMMAND(bind)
{
	Bindings.BindAKey(argc, argv, "Current key bindings: ");
}
END_COMMAND(bind)

BEGIN_COMMAND(unbind)
{
	if (argc < 2) {
		PrintFmt(PRINT_WARNING, "Unbinds a key. \"all\" unbinds every key.\n");
		PrintFmt(PRINT_WARNING, "Usage: unbind <key>\n");
		return;
	}

	if (iequals(argv[1], "all"))
		Bindings.UnbindAll();
	else
		Bindings.UnbindKey(argv[1]);
}
END_COMMAND(unbind)


// Doublebind command
BEGIN_COMMAND(doublebind)
{
	DoubleBindings.BindAKey(argc, argv, "Current doublebindings: ");
}
END_COMMAND(doublebind)

BEGIN_COMMAND(undoublebind)
{
	if (argc < 2)
	{
		PrintFmt(PRINT_WARNING, "Unbinds a doublekey. \"all\" unbinds every doublebind key.\n");
		PrintFmt(PRINT_WARNING, "Usage: undoublebind <key>\n");
		return;
	}

	if (iequals(argv[1], "all"))
		DoubleBindings.UnbindAll();
	else
		DoubleBindings.UnbindKey(argv[1]);
}
END_COMMAND(undoublebind)

// Automapbind command
BEGIN_COMMAND(ambind)
{
	AutomapBindings.BindAKey(argc, argv, "Current automap bindings: ");
}
END_COMMAND(ambind)

BEGIN_COMMAND(unambind)
{
	if (argc < 2)
	{
		PrintFmt(PRINT_WARNING, "Unbinds an automap key. \"all\" unbinds every automap key.\n");
		PrintFmt(PRINT_WARNING, "Usage: unambind <key>\n");
		return;
	}

	if (argc > 1)
	{
		if (iequals(argv[1], "all"))
			AutomapBindings.UnbindAll();
		else
			AutomapBindings.UnbindKey(argv[1]);
	}
}
END_COMMAND(unambind)

// NetDemoBind command
BEGIN_COMMAND(netdemobind)
{
	AutomapBindings.BindAKey(argc, argv, "Current netdemomap bindings: ");
}
END_COMMAND(netdemobind)

BEGIN_COMMAND(unnetdemobind)
{
	if (argc < 2)
	{
		PrintFmt(PRINT_WARNING,
		         "Unbinds a netdemo key. \"all\" unbinds every existing netdemo key.\n");
		PrintFmt(PRINT_WARNING, "Usage: unnetdemobind <key>\n");
		return;
	}

	if (argc > 1)
	{
		if (iequals(argv[1], "all"))
			NetDemoBindings.UnbindAll();
		else
			NetDemoBindings.UnbindKey(argv[1]);
	}
}
END_COMMAND(unnetdemobind)

// Other commands
BEGIN_COMMAND(binddefaults)
{
	C_BindDefaults();
}
END_COMMAND(binddefaults)

BEGIN_COMMAND(unbindall)
{
		Bindings.UnbindAll();
		DoubleBindings.UnbindAll();
}
END_COMMAND(unbindall)



VERSION_CONTROL (c_bind_cpp, "$Id$")
