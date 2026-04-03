// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
//   Player setup menu, including team, gender, and color selection.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <algorithm>
#include <cctype>

#include "c_dispatch.h"
#include "c_bind.h"
#include "cl_main.h"
#include "cl_responderkeys.h"
#include "d_player.h"
#include "g_game.h"
#include "gstrings.h"
#include "i_video.h"
#include "m_menu.h"
#include "m_menuconf.h"
#include "m_playersetup.h"
#include "m_random.h"
#include "m_widgets.h"
#include "p_ctf.h"
#include "r_local.h"
#include "v_palette.h"
#include "v_text.h"
#include "w_wad.h"

team_t D_TeamByName(const char* team);
gender_t D_GenderByName(const char* gender);
colorpreset_t D_ColorPreset(const char* colorpreset);
extern int repeatCount;

EXTERN_CVAR(cl_name)
EXTERN_CVAR(cl_team)
EXTERN_CVAR(cl_colorpreset)
EXTERN_CVAR(cl_customcolor)
EXTERN_CVAR(cl_color)
EXTERN_CVAR(cl_gender)
EXTERN_CVAR(cl_autoaim)

namespace
{
	static constexpr const char* genders[] = { "male", "female", "cyborg", "other" };
	static constexpr const char* colorpresets[] = { "green", "indigo", "brown", "red",
													"blue", "orange", "gold", "jungle green",
													"purple", "white", "black", "custom" };

	static constexpr int PLAYERSETUP_X = 48;
	static constexpr int PLAYERSETUP_Y = 47;
	static constexpr int PlayerSetupTitleY = 10;
	static constexpr int PlayerSetupFallbackTitleX = 110;
	static constexpr int PlayerNameInputOffsetX = 60;
	static constexpr int PlayerNameInputOffsetY = -4;
	static constexpr int PlayerSetupCursorGap = 3;
	static constexpr int PlayerPreviewFireX = 200;
	static constexpr int PlayerPreviewFireYOffset = -14;
	static constexpr int PlayerPreviewBoxX = 236;
	static constexpr int PlayerPreviewBoxYOffset = 22;
	static constexpr int PlayerPreviewSpriteX = 236;
	static constexpr int PlayerPreviewSpriteYOffset = 46;
	static constexpr int fire_surface_width = 72;
	static constexpr int fire_surface_height = 77;

	enum psetup_t
	{
		playername,
		playerteam,
		playersex,
		playeraim,
		playercolorpreset,
		playerred,
		playergreen,
		playerblue,
		psetup_end
	};

	static IWindowSurface* fire_surface = nullptr;
	static byte FireRemap[256];
	static int playerSetupLastOn = playername;
	static state_t* PlayerState = nullptr;
	static int PlayerTics = 0;
	static bool editingName = false;
	static size_t nameCharIndex = 0;
	static char playerNameString[MAXPLAYERNAME + 1];
	static char playerNameOldString[MAXPLAYERNAME + 1];

	static int PlayerSetupItemCount()
	{
		return D_ColorPreset(cl_colorpreset.cstring()) == COLOR_CUSTOM ? psetup_end :
																		playercolorpreset + 1;
	}

	static void ClampPlayerSetupItem(int& currentItem)
	{
		currentItem = std::clamp(currentItem, 0, PlayerSetupItemCount() - 1);
		playerSetupLastOn = currentItem;
	}

	static void SendNewColor(int red, int green, int blue)
	{
		const int colorpreset = D_ColorPreset(cl_colorpreset.cstring());

		cl_color.ForceSet(fmt::format("{:02x} {:02x} {:02x}", red, green, blue).c_str());
		if (colorpreset == COLOR_CUSTOM)
		{
			cl_customcolor.ForceSet(fmt::format("{:02x} {:02x} {:02x}", red, green, blue).c_str());
		}

		if (!connected)
		{
			R_BuildPlayerTranslation(menuplayer_id, V_GetColorFromString(cl_color), colorpreset);

			if (consoleplayer().ingame())
			{
				R_CopyTranslationRGB(menuplayer_id, consoleplayer_id);
			}
		}
	}

	static void ChangeTeam(int choice)
	{
		team_t team = D_TeamByName(cl_team.cstring());
		int index = static_cast<int>(team);

		if (choice)
		{
			index = (index + 1) % NUMTEAMS;
		}
		else
		{
			index = index > 0 ? index - 1 : NUMTEAMS - 1;
		}

		team = static_cast<team_t>(index);
		cl_team = GetTeamInfo(team)->ColorStringUpper.c_str();
	}

	static void ChangeGender(int choice)
	{
		static constexpr int MAX_GENDER = static_cast<int>(ARRAY_LENGTH(genders)) - 1;
		int gender = D_GenderByName(cl_gender.cstring());

		if (!choice)
		{
			gender = gender == 0 ? MAX_GENDER : gender - 1;
		}
		else
		{
			gender = gender == MAX_GENDER ? 0 : gender + 1;
		}

		cl_gender = genders[gender];
	}

	static void ChangeAutoAim(int choice)
	{
		static constexpr float ranges[] = { 0, 0.25F, 0.5F, 1, 2, 3, 5000 };
		float aim = cl_autoaim;

		if (!choice)
		{
			for (int i = 6; i >= 1; --i)
			{
				if (aim >= ranges[i])
				{
					aim = ranges[i - 1];
					break;
				}
			}
		}
		else
		{
			for (int i = 5; i >= 0; --i)
			{
				if (aim >= ranges[i])
				{
					aim = ranges[i + 1];
					break;
				}
			}
		}

		cl_autoaim.Set(aim);
	}

	static void ChangeColorPreset(int choice)
	{
		static constexpr int MAX_PRESET = static_cast<int>(ARRAY_LENGTH(colorpresets)) - 1;
		int colorpreset = D_ColorPreset(cl_colorpreset.cstring());
		const argb_t customcolor = V_GetColorFromString(cl_customcolor);

		colorpreset = choice ? (colorpreset == MAX_PRESET ? 0 : colorpreset + 1) :
							(colorpreset == 0 ? MAX_PRESET : colorpreset - 1);

		cl_colorpreset = colorpresets[colorpreset];

		switch (colorpreset)
		{
		case COLOR_GREEN:
			SendNewColor(64, 207, 0);
			break;
		case COLOR_INDIGO:
			SendNewColor(134, 134, 134);
			break;
		case COLOR_BROWN:
			SendNewColor(169, 87, 31);
			break;
		case COLOR_RED:
			SendNewColor(250, 62, 62);
			break;
		case COLOR_BLUE:
			SendNewColor(57, 57, 255);
			break;
		case COLOR_ORANGE:
			SendNewColor(255, 96, 0);
			break;
		case COLOR_GOLD:
			SendNewColor(255, 206, 43);
			break;
		case COLOR_JUNGLEGREEN:
			SendNewColor(32, 104, 0);
			break;
		case COLOR_PURPLE:
			SendNewColor(255, 10, 255);
			break;
		case COLOR_WHITE:
			SendNewColor(255, 255, 255);
			break;
		case COLOR_BLACK:
			SendNewColor(0, 0, 0);
			break;
		default:
			SendNewColor(customcolor.getr(), customcolor.getg(), customcolor.getb());
			break;
		}
	}

	static void SlidePlayerRed(int choice)
	{
		argb_t color = V_GetColorFromString(cl_color);
		const int accel = repeatCount < 10 ? 0 : 5;

		color.setr(choice ? std::min(255, int(color.getr()) + 1 + accel) :
							std::max(0, int(color.getr()) - 1 - accel));
		SendNewColor(color.getr(), color.getg(), color.getb());
	}

	static void SlidePlayerGreen(int choice)
	{
		argb_t color = V_GetColorFromString(cl_color);
		const int accel = repeatCount < 10 ? 0 : 5;

		color.setg(choice ? std::min(255, int(color.getg()) + 1 + accel) :
							std::max(0, int(color.getg()) - 1 - accel));
		SendNewColor(color.getr(), color.getg(), color.getb());
	}

	static void SlidePlayerBlue(int choice)
	{
		argb_t color = V_GetColorFromString(cl_color);
		const int accel = repeatCount < 10 ? 0 : 5;

		color.setb(choice ? std::min(255, int(color.getb()) + 1 + accel) :
							std::max(0, int(color.getb()) - 1 - accel));
		SendNewColor(color.getr(), color.getg(), color.getb());
	}

	static void BeginPlayerNameEdit()
	{
		editingName = true;
		M_StringCopy(playerNameOldString, playerNameString, MAXPLAYERNAME + 1);
		if (!strcmp(playerNameString, GStrings(EMPTYSTRING)))
		{
			playerNameString[0] = 0;
		}
		nameCharIndex = strlen(playerNameString);
	}

	static void CommitPlayerName()
	{
		AddCommandString(fmt::format("cl_name \"{}\"", playerNameString));
	}

	static void ActivatePlayerSetupItem(int currentItem, int choice)
	{
		switch (currentItem)
		{
		case playername:
			BeginPlayerNameEdit();
			break;
		case playerteam:
			ChangeTeam(choice);
			break;
		case playersex:
			ChangeGender(choice);
			break;
		case playeraim:
			ChangeAutoAim(choice);
			break;
		case playercolorpreset:
			ChangeColorPreset(choice);
			break;
		case playerred:
			SlidePlayerRed(choice);
			break;
		case playergreen:
			SlidePlayerGreen(choice);
			break;
		case playerblue:
			SlidePlayerBlue(choice);
			break;
		default:
			break;
		}
	}

	template<typename PIXEL_T>
	static forceinline PIXEL_T FirePixel(const byte c);

	template<>
	forceinline byte FirePixel<byte>(const byte c)
	{
		return FireRemap[c];
	}

	template<>
	forceinline argb_t FirePixel<argb_t>(const byte c)
	{
		return V_GammaCorrect(argb_t(c, 0, 0));
	}

	template<int xscale, typename PIXEL_T>
	static forceinline void RenderFire(int x, int y)
	{
		IWindowSurface* surface = I_GetPrimarySurface();
		const int surfacePitch = surface->getPitchInPixels();

		fire_surface->lock();

		for (int b = 0; b < fire_surface_height; ++b)
		{
			PIXEL_T* to = reinterpret_cast<PIXEL_T*>(surface->getBuffer()) + y * surfacePitch + x;
			const palindex_t* from =
				static_cast<palindex_t*>(fire_surface->getBuffer()) + b * fire_surface->getPitch();
			y += CleanYfac;

			for (int a = 0; a < fire_surface_width; ++a, to += xscale, ++from)
			{
				for (int c = CleanYfac; c; --c)
				{
					for (int i = 0; i < xscale; ++i)
					{
						*(to + surfacePitch * c + i) = FirePixel<PIXEL_T>(*from);
					}
				}
			}
		}

		fire_surface->unlock();
	}

	template<typename PIXEL_T>
	static forceinline void RenderFire(int x, int y)
	{
		IWindowSurface* surface = I_GetPrimarySurface();
		const int surfacePitch = surface->getPitchInPixels();

		fire_surface->lock();

		for (int b = 0; b < fire_surface_height; ++b)
		{
			PIXEL_T* to = reinterpret_cast<PIXEL_T*>(surface->getBuffer()) + y * surfacePitch + x;
			const palindex_t* from =
				static_cast<palindex_t*>(fire_surface->getBuffer()) + b * fire_surface->getPitch();
			y += CleanYfac;

			for (int a = 0; a < fire_surface_width; ++a, to += CleanXfac, ++from)
			{
				for (int c = CleanYfac; c; --c)
				{
					for (int i = 0; i < CleanXfac; ++i)
					{
						*(to + surfacePitch * c + i) = FirePixel<PIXEL_T>(*from);
					}
				}
			}
		}

		fire_surface->unlock();
	}

	struct playerpreviewlayout_t
	{
		int fireX = 0;
		int fireY = 0;
		int boxX = 0;
		int boxY = 0;
		int spriteX = 0;
		int spriteY = 0;
	};

	static playerpreviewlayout_t PlayerPreviewLayout()
	{
		const int previewRowY = PLAYERSETUP_Y + M_BigFontLineHeight() * 3;

		return playerpreviewlayout_t{
		    PlayerPreviewFireX,
		    previewRowY + PlayerPreviewFireYOffset,
		    PlayerPreviewBoxX,
		    previewRowY + PlayerPreviewBoxYOffset,
		    PlayerPreviewSpriteX,
		    previewRowY + PlayerPreviewSpriteYOffset,
		};
	}

	static void SeedFireSurfaceBottomRows()
	{
		const int pitch = fire_surface->getPitch();
		palindex_t* from = static_cast<palindex_t*>(fire_surface->getBuffer()) +
		                   (fire_surface_height - 3) * pitch;
		for (int x = 0; x < fire_surface_width; ++x, ++from)
		{
			*from = *(from + (pitch << 1)) = M_Random();
		}
	}

	static void StepFireSurface()
	{
		const int pitch = fire_surface->getPitch();
		palindex_t* from = static_cast<palindex_t*>(fire_surface->getBuffer());

		for (int y = 0; y < fire_surface_height - 4; y += 2)
		{
			palindex_t* pixel = from;
			palindex_t* p = pixel + (pitch << 1);

			unsigned int top = *p + *(p + fire_surface_width - 1) + *(p + 1);
			unsigned int bottom = *(pixel + (pitch << 2));
			unsigned int c1 = (top + bottom) >> 2;
			if (c1 > 1)
			{
				--c1;
			}
			*pixel = c1;
			*(pixel + pitch) = (c1 + bottom) >> 1;
			++pixel;

			for (int x = 1; x < fire_surface_width - 1; ++x)
			{
				p = pixel + (pitch << 1);
				top = *p + *(p - 1) + *(p + 1);
				bottom = *(pixel + (pitch << 2));
				c1 = (top + bottom) >> 2;
				if (c1 > 1)
				{
					--c1;
				}

				*pixel = c1;
				*(pixel + pitch) = (c1 + bottom) >> 1;
				++pixel;
			}

			p = pixel + (pitch << 1);
			top = *p + *(p - 1) + *(p - fire_surface_width + 1);
			bottom = *(pixel + (pitch << 2));
			c1 = (top + bottom) >> 2;
			if (c1 > 1)
			{
				--c1;
			}
			*pixel = c1;
			*(pixel + pitch) = (c1 + bottom) >> 1;

			from += pitch << 1;
		}
	}

	static void DrawPlayerPreviewFire(const playerpreviewlayout_t& layout)
	{
		int drawX = (layout.fireX - 160) * CleanXfac + (I_GetSurfaceWidth() / 2);
		int drawY = (layout.fireY - 100) * CleanYfac + (I_GetSurfaceHeight() / 2);

		if (!fire_surface)
		{
			const argb_t color = V_GetDefaultPalette()->basecolors[34];
			screen->Clear(drawX, drawY, drawX + fire_surface_width * CleanXfac,
			              drawY + fire_surface_height * CleanYfac, color);
			return;
		}

		fire_surface->lock();
		SeedFireSurfaceBottomRows();
		StepFireSurface();
		fire_surface->unlock();

		--drawY;
		if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
		{
			     if (CleanXfac == 1) RenderFire<1, palindex_t>(drawX, drawY);
			else if (CleanXfac == 2) RenderFire<2, palindex_t>(drawX, drawY);
			else if (CleanXfac == 3) RenderFire<3, palindex_t>(drawX, drawY);
			else if (CleanXfac == 4) RenderFire<4, palindex_t>(drawX, drawY);
			else if (CleanXfac == 5) RenderFire<5, palindex_t>(drawX, drawY);
			else if (CleanXfac == 6) RenderFire<6, palindex_t>(drawX, drawY);
			else if (CleanXfac == 7) RenderFire<7, palindex_t>(drawX, drawY);
			else if (CleanXfac == 8) RenderFire<8, palindex_t>(drawX, drawY);
			else if (CleanXfac == 9) RenderFire<9, palindex_t>(drawX, drawY);
			else if (CleanXfac == 10) RenderFire<10, palindex_t>(drawX, drawY);
			else if (CleanXfac == 11) RenderFire<11, palindex_t>(drawX, drawY);
			else if (CleanXfac == 12) RenderFire<12, palindex_t>(drawX, drawY);
			else if (CleanXfac == 13) RenderFire<13, palindex_t>(drawX, drawY);
			else if (CleanXfac == 14) RenderFire<14, palindex_t>(drawX, drawY);
			else RenderFire<palindex_t>(drawX, drawY);
		}
		else
		{
			     if (CleanXfac == 1) RenderFire<1, argb_t>(drawX, drawY);
			else if (CleanXfac == 2) RenderFire<2, argb_t>(drawX, drawY);
			else if (CleanXfac == 3) RenderFire<3, argb_t>(drawX, drawY);
			else if (CleanXfac == 4) RenderFire<4, argb_t>(drawX, drawY);
			else if (CleanXfac == 5) RenderFire<5, argb_t>(drawX, drawY);
			else if (CleanXfac == 6) RenderFire<6, argb_t>(drawX, drawY);
			else if (CleanXfac == 7) RenderFire<7, argb_t>(drawX, drawY);
			else if (CleanXfac == 8) RenderFire<8, argb_t>(drawX, drawY);
			else if (CleanXfac == 9) RenderFire<9, argb_t>(drawX, drawY);
			else if (CleanXfac == 10) RenderFire<10, argb_t>(drawX, drawY);
			else if (CleanXfac == 11) RenderFire<11, argb_t>(drawX, drawY);
			else if (CleanXfac == 12) RenderFire<12, argb_t>(drawX, drawY);
			else if (CleanXfac == 13) RenderFire<13, argb_t>(drawX, drawY);
			else if (CleanXfac == 14) RenderFire<14, argb_t>(drawX, drawY);
			else RenderFire<argb_t>(drawX, drawY);
		}
	}

	static void DrawPlayerPreviewSprite(const playerpreviewlayout_t& layout, const palette_t* palette,
	                                    int colorpreset)
	{
		const int32_t spritenum = states[mobjinfo[MT_PLAYER].spawnstate].sprite;
		const spriteframe_t* sprframe =
		    &sprites[spritenum].spriteframes[PlayerState->frame & FF_FRAMEMASK];
		const argb_t playerColor = CL_GetPlayerColor(consoleplayer());
		const translationref_t oldColorMap = V_ColorMap;

		R_BuildPlayerTranslation(menuplayer_id, playerColor, colorpreset);
		V_ColorMap = translationref_t(translationtables, menuplayer_id);

		screen->DrawPatchCleanWithPalette(W_CachePatch("M_PBOX"), layout.boxX, layout.boxY, palette);
		screen->DrawTranslatedPatchClean(W_CachePatch(sprframe->lump[0]), layout.spriteX,
		                                 layout.spriteY);

		V_ColorMap = oldColorMap;
	}
} // namespace

void M_PlayerSetupInit()
{
	for (int i = 0; i < 256; ++i)
	{
		FireRemap[i] = V_BestColor(V_GetDefaultPalette()->basecolors, i, 0, 0);
	}
}

void M_PlayerSetupShutdown()
{
	I_FreeSurface(fire_surface);
	fire_surface = nullptr;
	PlayerState = nullptr;
	PlayerTics = 0;
	editingName = false;
	nameCharIndex = 0;
}

void M_PlayerSetupOpen(int& currentItem)
{
	currentItem = playerSetupLastOn;
	M_StringCopy(playerNameString, cl_name.cstring(), MAXPLAYERNAME + 1);
	editingName = false;
	nameCharIndex = strlen(playerNameString);
	PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	PlayerTics = PlayerState->tics;

	if (fire_surface == nullptr)
	{
		fire_surface = I_AllocateSurface(fire_surface_width, fire_surface_height, 8);
	}

	const argb_t playerColor = CL_GetPlayerColor(consoleplayer());
	const int colorpreset = D_ColorPreset(cl_colorpreset.cstring());
	R_BuildPlayerTranslation(menuplayer_id, playerColor, colorpreset);
	ClampPlayerSetupItem(currentItem);
}

void M_PlayerSetupTicker()
{
	if (--PlayerTics > 0)
	{
		return;
	}

	if (PlayerState->tics == -1 || PlayerState->nextstate == S_NULL)
	{
		PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	}
	else
	{
		PlayerState = &states[PlayerState->nextstate];
	}

	PlayerTics = PlayerState->tics;
}

void M_PlayerSetupDrawer(int currentItem)
{
	const OFont* smallFont = OFonts.small();
	const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
	const int colorpreset = D_ColorPreset(cl_colorpreset.cstring());
	const playerpreviewlayout_t preview = PlayerPreviewLayout();
	const EColorRange titleColor = M_MenuTextColor("title");
	const EColorRange itemColor = M_MenuTextColor("item");
	const EColorRange valueColor = M_MenuTextColor("value");

	if (W_CheckNumForName("M_PSTTL") >= 0)
	{
		const patch_t* patch = W_CachePatch("M_PSTTL");
		screen->DrawPatchCleanWithPalette(
		    patch, 160 - patch->width() / 2, PlayerSetupTitleY, palette);
	}
	else
	{
		screen->DrawTextCleanMove(
		    smallFont, titleColor, PlayerSetupFallbackTitleX, PlayerSetupTitleY,
		    M_LocalizedMenuString("MNU_PLAYERSETUP"));
	}

	screen->DrawTextCleanMove(smallFont, itemColor, PLAYERSETUP_X, PLAYERSETUP_Y, "Name");
	M_DrawInputBox(playerNameString, PLAYERSETUP_X + PlayerNameInputOffsetX,
	               PLAYERSETUP_Y + PlayerNameInputOffsetY, MAXPLAYERNAME + 1);

	if (editingName)
	{
		screen->DrawTextCleanMove(smallFont, itemColor,
		                          PLAYERSETUP_X + V_StringWidth(smallFont, playerNameString) +
		                              PlayerNameInputOffsetX,
		                          PLAYERSETUP_Y, "_");
	}

	DrawPlayerPreviewFire(preview);
	DrawPlayerPreviewSprite(preview, palette, colorpreset);

	{
		const team_t team = D_TeamByName(cl_team.cstring());
		const int x = V_StringWidth(smallFont, "Preferred Team") + 8 + PLAYERSETUP_X;
		screen->DrawTextCleanMove(smallFont, itemColor, PLAYERSETUP_X,
		                          PLAYERSETUP_Y + M_BigFontLineHeight(), "Preferred Team");
		screen->DrawTextCleanMove(
		    smallFont, valueColor, x, PLAYERSETUP_Y + M_BigFontLineHeight(),
		    team == TEAM_NONE ? "NONE" : GetTeamInfo(team)->ColorStringUpper.c_str());
	}

	{
		const gender_t gender = D_GenderByName(cl_gender.cstring());
		const int x = V_StringWidth(smallFont, "Gender") + 8 + PLAYERSETUP_X;
		screen->DrawTextCleanMove(smallFont, itemColor, PLAYERSETUP_X,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 2, "Gender");
		screen->DrawTextCleanMove(smallFont, valueColor, x,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 2, genders[gender]);
	}

	{
		const int x = V_StringWidth(smallFont, "Autoaim") + 8 + PLAYERSETUP_X;
		const float aim = cl_autoaim;
		const char* aimLabel = aim == 0     ? "Never" :
		                       aim <= 0.25F ? "Very Low" :
		                       aim <= 0.5F  ? "Low" :
		                       aim <= 1     ? "Medium" :
		                       aim <= 2     ? "High" :
		                       aim <= 3     ? "Very High" :
		                                       "Always";

		screen->DrawTextCleanMove(smallFont, itemColor, PLAYERSETUP_X,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 3, "Autoaim");
		screen->DrawTextCleanMove(smallFont, valueColor, x,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 3, aimLabel);
	}

	{
		const int x = V_StringWidth(smallFont, "Color") + 8 + PLAYERSETUP_X;
		screen->DrawTextCleanMove(smallFont, itemColor, PLAYERSETUP_X,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 4, "Color");
		screen->DrawTextCleanMove(smallFont, valueColor, x,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 4,
		                          colorpresets[colorpreset]);
	}

	if (colorpreset == COLOR_CUSTOM)
	{
		screen->DrawTextCleanMove(smallFont, itemColor, PLAYERSETUP_X,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 5, "Red");
		screen->DrawTextCleanMove(smallFont, itemColor, PLAYERSETUP_X,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 6, "Green");
		screen->DrawTextCleanMove(smallFont, itemColor, PLAYERSETUP_X,
		                          PLAYERSETUP_Y + M_BigFontLineHeight() * 7, "Blue");

		const int x = V_StringWidth(smallFont, "Green") + 8 + PLAYERSETUP_X;
		const argb_t playercolor = V_GetColorFromString(cl_color);
		M_DrawColoredSlider(x, PLAYERSETUP_Y + M_BigFontLineHeight() * 5, 0.0F, 255.0F,
		                    playercolor.getr(), argb_t(playercolor.getr(), 0, 0));
		M_DrawColoredSlider(x, PLAYERSETUP_Y + M_BigFontLineHeight() * 6, 0.0F, 255.0F,
		                    playercolor.getg(), argb_t(0, playercolor.getg(), 0));
		M_DrawColoredSlider(x, PLAYERSETUP_Y + M_BigFontLineHeight() * 7, 0.0F, 255.0F,
		                    playercolor.getb(), argb_t(0, 0, playercolor.getb()));
	}

	ClampPlayerSetupItem(currentItem);

	if (!editingName)
	{
		const patch_t* cursor = M_MenuCursorPatch();
		screen->DrawPatchCleanWithPaletteFlipped(
		    cursor, PLAYERSETUP_X - PlayerSetupCursorGap,
		    PLAYERSETUP_Y + currentItem * M_BigFontLineHeight() + M_MenuCursorOffsetY(), palette);
	}

}

void M_PlayerSetupResponder(int keyCode, int typedChar, bool numlock, int& currentItem)
{
	const OFont* smallFont = OFonts.small();

	if (editingName)
	{
		if (keyCode == OKEY_BACKSPACE)
		{
			if (nameCharIndex > 0)
			{
				--nameCharIndex;
				playerNameString[nameCharIndex] = 0;
			}
		}
		else if (Key_IsCancelKey(keyCode))
		{
			editingName = false;
			M_StringCopy(playerNameString, playerNameOldString, MAXPLAYERNAME + 1);
		}
		else if (Key_IsAcceptKey(keyCode))
		{
			editingName = false;
			if (playerNameString[0] != '\0')
			{
				CommitPlayerName();
			}
		}
		else if (Key_IsPrintableChar(typedChar) && nameCharIndex < MAXPLAYERNAME &&
		         V_StringWidth(smallFont, playerNameString) < (MAXPLAYERNAME - 1) * 8)
		{
			playerNameString[nameCharIndex++] = static_cast<char>(typedChar);
			playerNameString[nameCharIndex] = 0;
		}

		return;
	}

	if (Key_IsDownKey(keyCode, numlock))
	{
		currentItem = (currentItem + 1) % PlayerSetupItemCount();
		playerSetupLastOn = currentItem;
		M_PlayMenuSound("navigate");
		return;
	}

	if (Key_IsUpKey(keyCode, numlock))
	{
		currentItem = currentItem > 0 ? currentItem - 1 : PlayerSetupItemCount() - 1;
		playerSetupLastOn = currentItem;
		M_PlayMenuSound("navigate");
		return;
	}

	if (Key_IsLeftKey(keyCode, numlock))
	{
		if (currentItem != playername)
		{
			ActivatePlayerSetupItem(currentItem, 0);
			ClampPlayerSetupItem(currentItem);
			M_PlayMenuSound("changeValue");
		}
		return;
	}

	if (Key_IsRightKey(keyCode, numlock))
	{
		if (currentItem != playername)
		{
			ActivatePlayerSetupItem(currentItem, 1);
			ClampPlayerSetupItem(currentItem);
			M_PlayMenuSound("changeValue");
		}
		return;
	}

	if (Key_IsAcceptKey(keyCode))
	{
		const bool editingName = currentItem == playername;
		ActivatePlayerSetupItem(currentItem, editingName ? currentItem : 1);
		ClampPlayerSetupItem(currentItem);
		M_PlayMenuSound(editingName ? "select" : "changeValue");
		return;
	}

	if (Key_IsCancelKey(keyCode))
	{
		playerSetupLastOn = currentItem;
		M_PopMenuStack();
		return;
	}

	if (typedChar && keyCode < OKEY_JOY1)
	{
		const char alpha = static_cast<char>(tolower(typedChar));
		static constexpr char alphaKeys[psetup_end] = { 'n', 't', 'e', 'a', 'c', 'r', 'g', 'b' };

		for (int i = currentItem + 1; i < PlayerSetupItemCount(); ++i)
		{
			if (alphaKeys[i] == alpha)
			{
				currentItem = i;
				playerSetupLastOn = currentItem;
				M_PlayMenuSound("navigate");
				return;
			}
		}

		for (int i = 0; i <= currentItem && i < PlayerSetupItemCount(); ++i)
		{
			if (alphaKeys[i] == alpha)
			{
				currentItem = i;
				playerSetupLastOn = currentItem;
				M_PlayMenuSound("navigate");
				return;
			}
		}
	}
}
