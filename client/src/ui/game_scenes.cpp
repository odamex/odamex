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

#include "odamex.h"

#include "ui/game_scenes.h"

#include "d_main.h"
#include "g_game.h"

#include "ui/ui_stack.h"
#include "ui/overlay_hud.h"
#include "ui/overlay_automap.h"

// The HUD and automap overlays belong to the scenes that use them.
void LevelScene::onEnter()
{
	g_UIStack.push(&UI_HudOverlay());
	g_UIStack.push(&UI_AutomapOverlay());
}

void LevelScene::onExit()
{
	g_UIStack.remove(&UI_AutomapOverlay());
	g_UIStack.remove(&UI_HudOverlay());
}

// Requires HUD overlay to run ST_Ticker()
void IntermissionScene::onEnter()
{
	g_UIStack.push(&UI_HudOverlay());
}

void IntermissionScene::onExit()
{
	g_UIStack.remove(&UI_HudOverlay());
}

void LevelScene::tick()
{
	G_LevelTick();
}

void LevelScene::draw()
{
	D_DrawLevelScene();
}

void IntermissionScene::tick()
{
	G_IntermissionTick();
}

void IntermissionScene::draw()
{
	D_DrawIntermissionScene();
}

void FinaleScene::tick()
{
	G_FinaleTick();
}

void FinaleScene::draw()
{
	D_DrawFinaleScene();
}

void DemoScene::tick()
{
	G_DemoScreenTick();
}

void DemoScene::draw()
{
	D_DrawDemoScene();
}

LevelScene& UI_LevelScene()
{
	static LevelScene scene;
	return scene;
}

IntermissionScene& UI_IntermissionScene()
{
	static IntermissionScene scene;
	return scene;
}

FinaleScene& UI_FinaleScene()
{
	static FinaleScene scene;
	return scene;
}

DemoScene& UI_DemoScene()
{
	static DemoScene scene;
	return scene;
}

ConnectScene& UI_ConnectScene()
{
	static ConnectScene scene;
	return scene;
}

NullScene& UI_NullScene()
{
	static NullScene scene;
	return scene;
}
