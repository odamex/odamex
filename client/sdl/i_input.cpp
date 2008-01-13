// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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

#include <SDL.h>

#include "m_argv.h"
#include "i_input.h"
#include "v_video.h"
#include "d_main.h"
#include "c_console.h"
#include "c_cvars.h"
#include "i_system.h"
#include "c_dispatch.h"

static BOOL mousepaused = true; // SoM: This should start off true
static BOOL havefocus = false;
static BOOL noidle = false;

// Used by the console for making keys repeat
int KeyRepeatDelay;
int KeyRepeatRate;

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
CVAR (mouse_acceleration, "2", CVAR_ARCHIVE)
CVAR (mouse_threshold, "10", CVAR_ARCHIVE)

// joek - sort mouse grab issue
BOOL mousegrabbed = false;

// SoM: if true, the mouse events in the queue should be ignored until at least once event cycle 
// is complete.
static BOOL flushmouse = false;

extern constate_e ConsoleState;

// NES - Currently unused. Make some use of these if possible.
//CVAR (i_remapkeypad, "1", CVAR_ARCHIVE)
//CVAR (use_mouse, "1", CVAR_ARCHIVE)
//CVAR (use_joystick, "0", CVAR_ARCHIVE)
//CVAR (joy_speedmultiplier, "1", CVAR_ARCHIVE)
//CVAR (joy_xsensitivity, "1", CVAR_ARCHIVE)
//CVAR (joy_ysensitivity, "-1", CVAR_ARCHIVE)
//CVAR (joy_xthreshold, "0.15", CVAR_ARCHIVE)
//CVAR (joy_ythreshold, "0.15", CVAR_ARCHIVE)

#ifdef WIN32
#define _WIN32_WINNT 0x0400
#include <windows.h>
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
#endif

//
// I_InitInput
//
bool I_InitInput (void)
{
	atterm (I_ShutdownInput);

	noidle = Args.CheckParm ("-noidle");

	SDL_EnableUNICODE(true);
	
	// denis - disable key repeats as they mess with the mouse in XP
	SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

#ifdef WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
#endif

	return true;
}

//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput (void)
{
	I_PauseMouse();

#ifdef WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	UnhookWindowsHookEx(g_hKeyboardHook);
#endif
}

//
// SetCursorState
//
static void SetCursorState (int visible)
{
   SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

// denis - from chocolate doom
//
// AccelerateMouse
//
static int AccelerateMouse(int val)
{
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

//
// GrabMouse
//
static void GrabMouse (void)
{
   SDL_WM_GrabInput(SDL_GRAB_ON);
   mousegrabbed = true;
   flushmouse = true;
}

//
// UngrabMouse
//
static void UngrabMouse (void)
{
   SDL_WM_GrabInput(SDL_GRAB_OFF);
   mousegrabbed = false;
}

//
// I_PauseMouse
//

EXTERN_CVAR (fullscreen)

void I_PauseMouse (void)
{
   // denis - disable key repeats as they mess with the mouse in XP
   SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

   if (fullscreen)
    return;
   
   UngrabMouse();
   SetCursorState(true);
   
   mousepaused = true;
}

//
// I_ResumeMouse
//
void I_ResumeMouse (void)
{   
   if(havefocus)
   {
      GrabMouse();
      SetCursorState(false);

      // denis - disable key repeats as they mess with the mouse in XP
      SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
   }
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
   
   if (!(SDL_GetAppState()&SDL_APPINPUTFOCUS) && havefocus)
   {
      havefocus = false;
      UngrabMouse();
      SetCursorState(true);
   }
   else if((SDL_GetAppState()&SDL_APPINPUTFOCUS) && !havefocus)
   {
      havefocus = true;
      if(!mousepaused)
         I_ResumeMouse();
   }

   if(!SDL_PollEvent(NULL))
      return;
   
   while(SDL_PollEvent(&ev))
   {
      event.data1 = event.data2 = event.data3 = 0;
      switch(ev.type)
      {
         case SDL_QUIT:
            AddCommandString("quit");
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

#ifdef WIN32
            // SoM: Ignore the tab portion of alt-tab presses
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
            else if(ev.button.button == SDL_BUTTON_WHEELUP)
               event.data1 = KEY_MWHEELUP;
            else if(ev.button.button == SDL_BUTTON_WHEELDOWN)
               event.data1 = KEY_MWHEELDOWN;

			D_PostEvent(&event);
            break; 
         case SDL_MOUSEBUTTONUP:
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
            else if(ev.button.button == SDL_BUTTON_WHEELUP)
               event.data1 = KEY_MWHEELUP;
            else if(ev.button.button == SDL_BUTTON_WHEELDOWN)
               event.data1 = KEY_MWHEELDOWN;

			D_PostEvent(&event);
            break;
      };
   }

   if(sendmouseevent)
   {
      mouseevent.data1 = mbuttons;
      D_PostEvent(&mouseevent);
   }
   
   if(mousegrabbed)
   {
      SDL_WarpMouse(screen->width/ 2, screen->height / 2);
   }

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


