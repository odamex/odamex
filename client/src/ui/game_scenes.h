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
//   Game scenes are the phases of the game itself reached through natural
//   progression, like the level scene itself (gameplay), intermission,
//   finale and demo reel.
//
//-----------------------------------------------------------------------------

#pragma once

#include "ui/ui_scene.h"

// GS_LEVEL
class LevelScene : public IScene
{
  public:
	SceneType type() const override { return SCENE_LEVEL; }
	void onEnter() override;
	void onExit() override;
	void tick() override;
	void draw() override;
};

// GS_INTERMISSION
class IntermissionScene : public IScene
{
  public:
	SceneType type() const override { return SCENE_INTERMISSION; }
	void onEnter() override;
	void onExit() override;
	void tick() override;
	void draw() override;
};

// GS_FINALE
class FinaleScene : public IScene
{
  public:
	SceneType type() const override { return SCENE_FINALE; }
	void tick() override;
	void draw() override;
};

// GS_DEMOSCREEN
class DemoScene : public IScene
{
  public:
	SceneType type() const override { return SCENE_DEMOSCREEN; }
	void tick() override;
	void draw() override;
};

// GS_FULLCONSOLE / GS_CONNECTING.
// Draws nothing: the console and menu overlays compose the entire frame, and the
// per frame extras are skipped.
class ConnectScene : public IScene
{
  public:
	SceneType type() const override { return SCENE_CONNECT; }
	bool wantsFrameDecorations() const override { return false; }
};

// GS_STARTUP / GS_HIDECONSOLE and anything else: no scene content, but the
// per frame extras still draw.
class NullScene : public IScene
{
  public:
	SceneType type() const override { return SCENE_NULL; }
};

LevelScene& UI_LevelScene();
IntermissionScene& UI_IntermissionScene();
FinaleScene& UI_FinaleScene();
DemoScene& UI_DemoScene();
ConnectScene& UI_ConnectScene();
NullScene& UI_NullScene();
