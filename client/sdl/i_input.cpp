// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2009 by The Odamex Team.
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

#ifdef WIN32
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#ifndef _XBOX
#include <windows.h>
#endif // !_XBOX
#ifdef _MSC_VER
#ifndef snprintf
#define snprintf _snprintf
#endif // !snprintf
#endif // MSC_VER
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

EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)

static BOOL mousepaused = true; // SoM: This should start off true
static BOOL havefocus = false;
static BOOL nomouse = false;

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
EXTERN_CVAR (mouse_acceleration)
EXTERN_CVAR (mouse_threshold)

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
#endif

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

    // when menu is active or game is paused, release the mouse 
    if (menuactive || paused)
        return false;

    // only grab mouse when playing levels (but not demos)

    return (gamestate == GS_LEVEL) && !demoplayback;
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
    Uint8 state;

    state = SDL_GetAppState();

    // We should have input (keyboard) focus and be visible 
    // (not minimized)
    havefocus = (state & SDL_APPINPUTFOCUS) && (state & SDL_APPACTIVE);
}

//
// UpdateGrab (From chocolate-doom)
//
static void UpdateGrab(void)
{
    bool grab;

    grab = MouseShouldBeGrabbed();

    if (grab && !mousegrabbed)
    {
	   if(screen)
		  SDL_WarpMouse(screen->width/ 2, screen->height / 2);

        SetCursorState(false);
        SDL_WM_GrabInput(SDL_GRAB_ON);
        flushmouse = true;
    }
    else if (!grab && mousegrabbed)
    {
        SetCursorState(true);
        SDL_WM_GrabInput(SDL_GRAB_OFF);
    }

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
	
	// denis - disable key repeats as they mess with the mouse in XP
	SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

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
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

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
   SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

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
		SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

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
             if (!mousegrabbed && !vid_fullscreen)
             {
                char Command[64];
                
                snprintf(Command, sizeof(Command), "vid_setmode %d %d", ev.resize.w, ev.resize.h);
               
                AddCommandString(Command);

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
            else if(ev.button.button == SDL_BUTTON_WHEELUP)
               event.data1 = KEY_MWHEELUP;
            else if(ev.button.button == SDL_BUTTON_WHEELDOWN)
               event.data1 = KEY_MWHEELDOWN;

		D_PostEvent(&event);
		break;
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


