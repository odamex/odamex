// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
//   Scene manager.
//
//   Scenes are different stages of the game loop
//   reached through natural progression, like the game itself,
//   intermission and the finale cast/endpic/bunny.
//
//   Exactly one scene is active at a time.
//
//   Scenes draw and tic themselves.
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomdef.h"

#include "ui/ui_layer.h"

enum SceneType
{
	SCENE_NONE = 0,
	SCENE_NULL,         // startup / hide-console: no scene content, frame extras still drawn
	SCENE_CONNECT,      // full console / connecting / connected: overlays compose the frame
	SCENE_LEVEL,
	SCENE_INTERMISSION,
	SCENE_FINALE,
	SCENE_DEMOSCREEN,
};

class IScene : public ILayer
{
  public:
	virtual SceneType type() const = 0;

	// Should D_Display draw the per-frame extras (pause pic, icon, screen wipe)
	// after this scene?
	virtual bool wantsFrameDecorations() const { return true; }
};

// The active scene.
IScene& UI_Scene();

// The type of the active scene.
SceneType UI_SceneType();

// Request a screen wipe on the next display, independent of any scene change.
void Scene_ForceWipe();

// Swap the active scene, running onExit/onEnter, and update gamestate.
void Scene_Change(IScene& scene, gamestate_t newstate);

// Drop-in replacement for the old gamestate assignments.
// Sets a scene based on the gamestate state.
void Scene_SetFromGamestate(gamestate_t gs);

void UI_InitScenes();
