// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	SDL input handler
//
//-----------------------------------------------------------------------------


// SoM 12-24-05: yeah... I'm programming on christmas eve.
// Removed all the DirectX crap.

#include <stdlib.h>
#include <list>
#include <sstream>

#ifdef WIN32
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#ifndef _XBOX
#include <windows.h>
#endif // !_XBOX
#endif // WIN32

#include <SDL.h>

#include "doomstat.h"
#include "m_argv.h"
#include "i_input.h"
#include "v_video.h"
#include "d_main.h"
#include "c_console.h"
#include "c_cvars.h"
#include "i_system.h"
#include "c_dispatch.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#define JOY_DEADZONE 6000

EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)

static BOOL mousepaused = true; // SoM: This should start off true
static BOOL havefocus = false;
static BOOL nomouse = false;

EXTERN_CVAR (use_joystick)
EXTERN_CVAR (joy_active)

typedef struct
{
	SDL_Event    Event;
	unsigned int RegTick;
	unsigned int LastTick;
} JoystickEvent_t;

static SDL_Joystick *openedjoy = NULL;
static std::list<JoystickEvent_t*> JoyEventList;

// denis - from chocolate doom
//
// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.
EXTERN_CVAR (mouse_acceleration)
EXTERN_CVAR (mouse_threshold)

EXTERN_CVAR (mouse_type)

// joek - sort mouse grab issue
static BOOL mousegrabbed = false;

// SoM: if true, the mouse events in the queue should be ignored until at least once event cycle
// is complete.
static BOOL flushmouse = false;

extern constate_e ConsoleState;

#if defined WIN32 && !defined _XBOX
// denis - in fullscreen, prevent exit on accidental windows key press
HHOOK g_hKeyboardHook;
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0 || nCode != HC_ACTION )  // do not process message
        return CallNextHookEx( g_hKeyboardHook, nCode, wParam, lParam);

	KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
	if((wParam == WM_KEYDOWN || wParam == WM_KEYUP)
		&& (mousegrabbed && ((p->vkCode == VK_LWIN) || (p->vkCode == VK_RWIN))))
		return 1;

	return CallNextHookEx( g_hKeyboardHook, nCode, wParam, lParam );
}

static int backup_mouse_settings[3];

// Backup global mouse settings
void BackupGDIMouseSettings()
{
    SystemParametersInfo (SPI_GETMOUSE, 0, &backup_mouse_settings, 0);
}

// [Russell - From Darkplaces/Nexuiz] - In gdi mode, disable accelerated mouse input
void FixGDIMouseInput()
{
    int mouse_settings[3];

    // Turn off accelerated mouse input incoming from windows
    mouse_settings[0] = 0;
    mouse_settings[1] = 0;
    mouse_settings[2] = 0;

    SystemParametersInfo (SPI_SETMOUSE, 0, (PVOID)mouse_settings, 0);
}

// Restore global mouse settings
void STACK_ARGS RestoreGDIMouseSettings()
{
    SystemParametersInfo (SPI_SETMOUSE, 0, (PVOID)backup_mouse_settings, 0);
}
#endif

void I_FlushInput()
{
	// eat all pending input from outside the game
	SDL_Event ev;
	while (SDL_PollEvent(&ev));
}

void I_EnableKeyRepeat()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY / 2, SDL_DEFAULT_REPEAT_INTERVAL);
}

void I_DisableKeyRepeat()
{
	SDL_EnableKeyRepeat(0, 0);	
}

void I_ResetKeyRepeat()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

static bool MouseShouldBeGrabbed()
{
	// if the window doesn't have focus, never grab it
	if (!havefocus)
		return false;

	// always grab the mouse when full screen (dont want to
	// see the mouse pointer)
	if (vid_fullscreen)
		return true;

	// Don't grab the mouse if mouse input is disabled
	if (nomouse)
		return false;

	// if we specify not to grab the mouse, never grab
	//if (!grabmouse)
	//	return false;

    // when menu is active, console is down or game is paused, release the mouse
    if (menuactive || ConsoleState == c_down || paused)
        return false;

    // only grab mouse when playing levels (but not demos)

    return (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && !demoplayback;
}

//
// SetCursorState
//
static void SetCursorState (int visible)
{
   if(nomouse)
      return;

   SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

// Update the value of havefocus when we get a focus event
//
// We try to make ourselves be well-behaved: the grab on the mouse
// is removed if we lose focus (such as a popup window appearing),
// and we dont move the mouse around if we aren't focused either.

static void UpdateFocus(void)
{
	static bool curfocus = false;

	SDL_PumpEvents();

	Uint8 state = SDL_GetAppState();

	// We should have input (keyboard) focus and be visible (not minimized)
	havefocus = (state & SDL_APPINPUTFOCUS) && (state & SDL_APPACTIVE);

	// [CG] Handle focus changes, this is all necessary to avoid repeat events.
	// [AM] This fixes the tab key sticking when alt-tabbing away from the
	//      program, but does not seem to solve the problem of tab being 'dead'
	//      for one keypress after switching back.
	if (curfocus != havefocus)
	{
		if (havefocus)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				// Do nothing
			}
		}

		curfocus = havefocus;
	}
}

//
// UpdateGrab (From chocolate-doom)
//
static void UpdateGrab(void)
{
	bool grab = MouseShouldBeGrabbed();

	if (grab && !mousegrabbed)
	{
		SetCursorState(false);
		SDL_WM_GrabInput(SDL_GRAB_ON);
		
		// Warp the mouse back to the middle of the screen
		if(screen)
			SDL_WarpMouse(screen->width/ 2, screen->height / 2);
	
		I_FlushInput();	
	}
	else if (!grab && mousegrabbed)
	{
		SetCursorState(true);
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	}

	#if defined WIN32 && !defined _XBOX
    if (Args.CheckParm ("-gdi"))
	{
        if (grab)
            FixGDIMouseInput();
        else
            RestoreGDIMouseSettings();
    }
    #endif

	mousegrabbed = grab;
}

// denis - from chocolate doom
//
// AccelerateMouse
//
static int AccelerateMouse(int val)
{
    if (!mouse_acceleration)
        return val;

    if (val < 0)
        return -AccelerateMouse(-val);

    if (val > mouse_threshold)
    {
        return (int)((val - mouse_threshold) * mouse_acceleration + mouse_threshold);
    }
    else
    {
        return val;
    }
}

// Add any joystick event to a list if it will require manual polling
// to detect release. This includes hat events (mostly due to d-pads not
// triggering the centered event when released) and analog axis bound
// as a key/button -- HyperEye
//
// RegisterJoystickEvent
//
static int RegisterJoystickEvent(SDL_Event *ev, int value)
{
	JoystickEvent_t *evc = NULL;
	event_t          event;

	if(!ev)
		return -1;

	if(ev->type == SDL_JOYHATMOTION)
	{
		if(!JoyEventList.empty())
		{
			std::list<JoystickEvent_t*>::iterator i;

			for(i = JoyEventList.begin(); i != JoyEventList.end(); ++i)
			{
				if(((*i)->Event.type == ev->type) && ((*i)->Event.jhat.which == ev->jhat.which) 
							&& ((*i)->Event.jhat.hat == ev->jhat.hat) && ((*i)->Event.jhat.value == value))
					return 0;
			}
		}

		evc = new JoystickEvent_t;

		memcpy(&evc->Event, ev, sizeof(SDL_Event));
		evc->Event.jhat.value = value;
		evc->LastTick = evc->RegTick = SDL_GetTicks();

		event.data1 = event.data2 = event.data3 = 0;

		event.type = ev_keydown;
		if(value == SDL_HAT_UP)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT1;
		else if(value == SDL_HAT_RIGHT)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT2;
		else if(value == SDL_HAT_DOWN)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT3;
		else if(value == SDL_HAT_LEFT)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT4;

		event.data2 = event.data1;
	}

	if(evc)
	{
		JoyEventList.push_back(evc);
		D_PostEvent(&event);
		return 1;
	}

	return 0;
}

void UpdateJoystickEvents()
{
	std::list<JoystickEvent_t*>::iterator i;
	event_t    event;

	if(JoyEventList.empty())
		return;

	i = JoyEventList.begin();
	while(i != JoyEventList.end())
	{
		if((*i)->Event.type == SDL_JOYHATMOTION)
		{
			// Hat position released
			if(!(SDL_JoystickGetHat(openedjoy, (*i)->Event.jhat.hat) & (*i)->Event.jhat.value))
				event.type = ev_keyup;
			// Hat button still held - Repeat at key repeat interval
			else if((SDL_GetTicks() - (*i)->RegTick >= SDL_DEFAULT_REPEAT_DELAY) && 
		            (SDL_GetTicks() - (*i)->LastTick >= SDL_DEFAULT_REPEAT_INTERVAL*2))
			{
				(*i)->LastTick = SDL_GetTicks();
				event.type = ev_keydown;
			}
			else
			{
				++i;
				continue;
			}

			event.data1 = event.data2 = event.data3 = 0;

			if((*i)->Event.jhat.value == SDL_HAT_UP)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT1;
			else if((*i)->Event.jhat.value == SDL_HAT_RIGHT)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT2;
			else if((*i)->Event.jhat.value == SDL_HAT_DOWN)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT3;
			else if((*i)->Event.jhat.value == SDL_HAT_LEFT)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT4;

			D_PostEvent(&event);

			if(event.type == ev_keyup)
			{
				// Delete the released event
				delete *i;
				i = JoyEventList.erase(i);
				continue;
			}
		}
		++i;
	}
}

// This turns on automatic event polling for joysticks so that the state
// of each button and axis doesn't need to be manually queried each tick. -- Hyper_Eye
//
// EnableJoystickPolling
//
static int EnableJoystickPolling()
{
	return SDL_JoystickEventState(SDL_ENABLE);
}

static int DisableJoystickPolling()
{
	return SDL_JoystickEventState(SDL_IGNORE);
}

CVAR_FUNC_IMPL (use_joystick)
{
	if(var <= 0.0)
	{
		// Don't let console users disable joystick support because
		// they won't have any way to reenable through the menu.
#ifdef GCONSOLE
		use_joystick = 1.0;
#else
		I_CloseJoystick();
		DisableJoystickPolling();
#endif
	}
	else
	{
		I_OpenJoystick();
		EnableJoystickPolling();
	}
}


CVAR_FUNC_IMPL (joy_active)
{
	if( (var < 0.0) || ((int)var > I_GetJoystickCount()) )
		var = 0.0;

	I_CloseJoystick();

	I_OpenJoystick();
}

//
// I_GetJoystickCount
//
int I_GetJoystickCount()
{
	return SDL_NumJoysticks();
}

//
// I_GetJoystickNameFromIndex
//
std::string I_GetJoystickNameFromIndex (int index)
{
	const char  *joyname = NULL;
	std::string  ret;

	joyname = SDL_JoystickName(index);

	if(!joyname)
		return "";
	
	ret = joyname;

	return ret;
}

//
// I_OpenJoystick
//
bool I_OpenJoystick()
{
	int numjoy;

	numjoy = I_GetJoystickCount();

	if(!numjoy || !use_joystick)
		return false;

	if((int)joy_active > numjoy)
		joy_active.Set(0.0);

	if(!SDL_JoystickOpened(joy_active))
		openedjoy = SDL_JoystickOpen(joy_active);

	if(!SDL_JoystickOpened(joy_active))
		return false;

	return true;
}

//
// I_CloseJoystick
//
void I_CloseJoystick()
{
	extern int joyforward, joystrafe, joyturn, joylook;
	int        ndx;

#ifndef _XBOX // This is to avoid a bug in SDLx
	if(!I_GetJoystickCount() || !openedjoy)
		return;

	ndx = SDL_JoystickIndex(openedjoy);

	if(SDL_JoystickOpened(ndx))
		SDL_JoystickClose(openedjoy);

	openedjoy = NULL;
#endif

	// Reset joy position values. Wouldn't want to get stuck in a turn or something. -- Hyper_Eye
	joyforward = joystrafe = joyturn = joylook = 0;
}

//
// I_InitInput
//
bool I_InitInput (void)
{
	if(Args.CheckParm("-nomouse"))
	{
		nomouse = true;
	}

	atterm (I_ShutdownInput);

	SDL_EnableUNICODE(true);

	I_DisableKeyRepeat();

	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	Printf(PRINT_HIGH, "I_InitInput: Initializing SDL's joystick subsystem.\n");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	if((int)use_joystick && I_GetJoystickCount())
	{
		I_OpenJoystick();
		EnableJoystickPolling();
	}

#ifdef WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	// [Russell] - Disabled because it screws with the mouse
	//g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
#endif
	//CreateCursors();
	UpdateFocus();
	UpdateGrab();

	return true;
}

//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput (void)
{
	//SDL_SetCursor(cursors[1]);
	SDL_ShowCursor(1);
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	I_ResetKeyRepeat();

#ifdef WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	// [Russell] - Disabled because it screws with the mouse
	//UnhookWindowsHookEx(g_hKeyboardHook);
#endif
}

//
// I_PauseMouse
//
void I_PauseMouse (void)
{
   // denis - disable key repeats as they mess with the mouse in XP
   //SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

   UpdateGrab();

   mousepaused = true;
}

//
// I_ResumeMouse
//
void I_ResumeMouse (void)
{
	UpdateGrab();

	if(havefocus)
		// denis - disable key repeats as they mess with the mouse in XP
		//SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

   mousepaused = false;
}

//
// I_GetEvent
//
void I_GetEvent (void)
{
   event_t event;
   event_t mouseevent = {ev_mouse, 0, 0, 0};
   static int mbuttons = 0;
   int sendmouseevent = 0;

   SDL_Event ev;

	if (!havefocus)
		I_PauseMouse();
	else
	{
		I_ResumeMouse();
	}

   while(SDL_PollEvent(&ev))
   {
      event.data1 = event.data2 = event.data3 = 0;
      switch(ev.type)
      {
         case SDL_QUIT:
            AddCommandString("quit");
            break;
         // Resizable window mode resolutions
         case SDL_VIDEORESIZE:
         {
             if (!vid_fullscreen)
             {            	
                std::stringstream Command;
                
                mousegrabbed = false;

                Command << "vid_setmode " << ev.resize.w << " " << ev.resize.h 
                    << std::endl;

                AddCommandString(Command.str());

                vid_defwidth.Set((float)ev.resize.w);
				vid_defheight.Set((float)ev.resize.h);
             }
         }
         break;

		case SDL_ACTIVEEVENT:
			// need to update our focus state
			UpdateFocus();
		break;

         case SDL_KEYDOWN:
            event.type = ev_keydown;
            event.data1 = ev.key.keysym.sym;

            if(event.data1 >= SDLK_KP0 && event.data1 <= SDLK_KP9)
               event.data2 = event.data3 = '0' + (event.data1 - SDLK_KP0);
            else if(event.data1 == SDLK_KP_PERIOD)
               event.data2 = event.data3 = '.';
            else if(event.data1 == SDLK_KP_DIVIDE)
               event.data2 = event.data3 = '/';
            else if(event.data1 == SDLK_KP_ENTER)
               event.data2 = event.data3 = '\r';
            else if ( (ev.key.keysym.unicode & 0xFF80) == 0 )
               event.data2 = event.data3 = ev.key.keysym.unicode;
            else
               event.data2 = event.data3 = 0;

#ifdef _XBOX
			// Fix for ENTER key on Xbox
            if(event.data1 == SDLK_RETURN)
               event.data2 = event.data3 = '\r';
#endif

#ifdef WIN32
            //HeX9109: Alt+F4 for cheats! Thanks Spleen
            if(event.data1 == SDLK_F4 && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
                AddCommandString("quit");
            // SoM: Ignore the tab portion of alt-tab presses
            // [AM] Windows 7 seems to preempt this check.
            if(event.data1 == SDLK_TAB && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
               event.data1 = event.data2 = event.data3 = 0;
            else
#endif
         D_PostEvent(&event);
         break;

         case SDL_KEYUP:
            event.type = ev_keyup;
            event.data1 = ev.key.keysym.sym;
            if ( (ev.key.keysym.unicode & 0xFF80) == 0 )
               event.data2 = event.data3 = ev.key.keysym.unicode;
            else
               event.data2 = event.data3 = 0;
         D_PostEvent(&event);
         break;

         case SDL_MOUSEMOTION:
            if(flushmouse)
            {
               flushmouse = false;
               break;
            }
            if (!havefocus)
				break;
			// denis - ignore artificially inserted events (see SDL_WarpMouse below)
			if(ev.motion.x == screen->width/2 &&
			   ev.motion.y == screen->height/2)
			{
				break;
			}
           	mouseevent.data2 += AccelerateMouse(ev.motion.xrel);
           	mouseevent.data3 -= AccelerateMouse(ev.motion.yrel);
            sendmouseevent = 1;
         break;

         case SDL_MOUSEBUTTONDOWN:
            if(nomouse || !havefocus)
				break;
            event.type = ev_keydown;
            if(ev.button.button == SDL_BUTTON_LEFT)
            {
               event.data1 = KEY_MOUSE1;
               mbuttons |= 1;
            }
            else if(ev.button.button == SDL_BUTTON_RIGHT)
            {
               event.data1 = KEY_MOUSE2;
               mbuttons |= 2;
            }
            else if(ev.button.button == SDL_BUTTON_MIDDLE)
            {
               event.data1 = KEY_MOUSE3;
               mbuttons |= 4;
            }
			// [Xyltol 07/21/2011] - Add support for MOUSE4 and MOUSE5 (back thumb and front thumb on most mice)
			else if(ev.button.button == SDL_BUTTON_X1){//back thumb
				event.data1 = KEY_MOUSE4;
				mbuttons |= 8;
			}else if(ev.button.button == SDL_BUTTON_X2){//front thumb
				event.data1 = KEY_MOUSE5;
				mbuttons |= 16;
			}
            else if(ev.button.button == SDL_BUTTON_WHEELUP)
               event.data1 = KEY_MWHEELUP;
            else if(ev.button.button == SDL_BUTTON_WHEELDOWN)
               event.data1 = KEY_MWHEELDOWN;

		D_PostEvent(&event);
		break;

	case SDL_MOUSEBUTTONUP:
            if(nomouse || !havefocus)
				break;
            event.type = ev_keyup;
			
            if(ev.button.button == SDL_BUTTON_LEFT)
            {
               event.data1 = KEY_MOUSE1;
               mbuttons &= ~1;
            }
            else if(ev.button.button == SDL_BUTTON_RIGHT)
            {
               event.data1 = KEY_MOUSE2;
               mbuttons &= ~2;
            }
            else if(ev.button.button == SDL_BUTTON_MIDDLE)
            {
               event.data1 = KEY_MOUSE3;
               mbuttons &= ~4;
            }
			// [Xyltol 07/21/2011] - Add support for MOUSE4 and MOUSE5 (back thumb and front thumb on most mice)
			else if(ev.button.button == SDL_BUTTON_X1){//back thumb
				event.data1 = KEY_MOUSE4;
				mbuttons &= ~8;
			}else if(ev.button.button == SDL_BUTTON_X2){//front thumb
				event.data1 = KEY_MOUSE5;
				mbuttons &= ~16;
			}
            else if(ev.button.button == SDL_BUTTON_WHEELUP)
               event.data1 = KEY_MWHEELUP;
            else if(ev.button.button == SDL_BUTTON_WHEELDOWN)
               event.data1 = KEY_MWHEELDOWN;

		D_PostEvent(&event);
		break;
	case SDL_JOYBUTTONDOWN:
		if(ev.jbutton.which == joy_active)
		{
			event.type = ev_keydown;
			event.data1 = ev.jbutton.button + KEY_JOY1;
			event.data2 = event.data1;

			D_PostEvent(&event);
			break;
		}
	case SDL_JOYBUTTONUP:
		if(ev.jbutton.which == joy_active)
		{
			event.type = ev_keyup;
			event.data1 = ev.jbutton.button + KEY_JOY1;
			event.data2 = event.data1;

			D_PostEvent(&event);
			break;
		}
	case SDL_JOYAXISMOTION:
		if(ev.jaxis.which == joy_active)
		{
			event.type = ev_joystick;
			event.data1 = 0;
			event.data2 = ev.jaxis.axis;
			if( (ev.jaxis.value < JOY_DEADZONE) && (ev.jaxis.value > -JOY_DEADZONE) )
				event.data3 = 0;
			else
				event.data3 = ev.jaxis.value;

			D_PostEvent(&event);
			break;
		}
	case SDL_JOYHATMOTION:
		if(ev.jhat.which == joy_active)
		{
			// Each of these need to be tested because more than one can be pressed and a
			// unique event is needed for each
			if(ev.jhat.value & SDL_HAT_UP)
				RegisterJoystickEvent(&ev, SDL_HAT_UP);
			if(ev.jhat.value & SDL_HAT_RIGHT)
				RegisterJoystickEvent(&ev, SDL_HAT_RIGHT);
			if(ev.jhat.value & SDL_HAT_DOWN)
				RegisterJoystickEvent(&ev, SDL_HAT_DOWN);
			if(ev.jhat.value & SDL_HAT_LEFT)
				RegisterJoystickEvent(&ev, SDL_HAT_LEFT);

			break;
		}
      };
   }

   if(!nomouse)
   {
       if(sendmouseevent)
       {
          mouseevent.data1 = mbuttons;
          D_PostEvent(&mouseevent);
       }

       if(mousegrabbed && screen)
       {
          SDL_WarpMouse(screen->width/ 2, screen->height / 2);
       }
   }

   if(use_joystick)
       UpdateJoystickEvents();
}

//
// I_StartTic
//
void I_StartTic (void)
{
	I_GetEvent ();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

VERSION_CONTROL (i_input_cpp, "$Id$")


