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

#include "odamex.h"

#include "ui/ui_scene.h"

#include "ui/game_scenes.h"

static IScene* currentScene = nullptr;

static SceneType sceneType = SCENE_NONE;

IScene& UI_Scene()
{
	if (currentScene == nullptr)
	{
		currentScene = &UI_NullScene();
		sceneType = currentScene->type();
	}

	return *currentScene;
}

SceneType UI_SceneType()
{
	UI_Scene();
	return sceneType;
}

void Scene_ForceWipe()
{
	wipegamestate = GS_FORCEWIPE;
}

void Scene_Change(IScene& scene, gamestate_t newstate)
{
	IScene& current = UI_Scene();

	if (&current == &scene)
	{
		gamestate = newstate;
		return;
	}

	current.onExit();
	currentScene = &scene;
	sceneType = scene.type();
	gamestate = newstate;
	currentScene->onEnter();
}

void Scene_SetFromGamestate(gamestate_t gs)
{
	switch (gs)
	{
	case GS_LEVEL:
		Scene_Change(UI_LevelScene(), gs);
		break;

	case GS_INTERMISSION:
		Scene_Change(UI_IntermissionScene(), gs);
		break;

	case GS_FINALE:
		Scene_Change(UI_FinaleScene(), gs);
		break;

	case GS_DEMOSCREEN:
		Scene_Change(UI_DemoScene(), gs);
		break;

	case GS_FULLCONSOLE:
	case GS_CONNECTING:
	case GS_CONNECTED:
		Scene_Change(UI_ConnectScene(), gs);
		break;

	case GS_STARTUP:
	case GS_HIDECONSOLE:
	default:
		Scene_Change(UI_NullScene(), gs);
		break;
	}
}

void UI_InitScenes()
{
	currentScene = nullptr;
	Scene_SetFromGamestate(gamestate);
}
