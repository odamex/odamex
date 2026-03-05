// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//   Heretic statusbar/HUD baseline for OdaHeretic milestone work.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "i_video.h"
#include "m_random.h"
#include "r_local.h"
#include "w_wad.h"
#include "st_lib.h"
#include "st_stuff.h"

EXTERN_CVAR(st_scale)

namespace
{
constexpr int HTIC_BASE_WIDTH = 320;
constexpr int HTIC_BASE_HEIGHT = 42;

lumpHandle_t hticBigNum[10];
lumpHandle_t hticSmallNum[10];
lumpHandle_t hticNegNum;
lumpHandle_t hticLameNum;
lumpHandle_t hticKeys[3];

lumpHandle_t hticBarBack;
lumpHandle_t hticBarMain;
lumpHandle_t hticChain;
lumpHandle_t hticLifeGem;
lumpHandle_t hticLeftFace;
lumpHandle_t hticRightFace;
lumpHandle_t hticGodEyesLeft;
lumpHandle_t hticGodEyesRight;
lumpHandle_t hticTopLeftCap;
lumpHandle_t hticTopRightCap;

bool hticAssetsLoaded = false;
bool hticHasCoreStatusbar = false;

int hticHealth = 100;
int hticArmor = 0;
int hticReadyAmmo = ST_DONT_DRAW_NUM;
int hticKeyboxes[3] = {-1, -1, -1};
int hticChainHealth = 100;
int hticChainWiggle = 0;

lumpHandle_t ST_HticTryCachePatch(const char* name)
{
	const int lump = W_CheckNumForName(name, ns_global);
	if (lump < 0)
		return lumpHandle_t();

	return W_CachePatchHandle(lump, PU_STATIC);
}

void ST_HticClearAssets()
{
	for (int i = 0; i < 10; i++)
	{
		hticBigNum[i].clear();
		hticSmallNum[i].clear();
	}

	for (int i = 0; i < 3; i++)
	{
		hticKeys[i].clear();
	}

	hticNegNum.clear();
	hticLameNum.clear();
	hticBarBack.clear();
	hticBarMain.clear();
	hticChain.clear();
	hticLifeGem.clear();
	hticLeftFace.clear();
	hticRightFace.clear();
	hticGodEyesLeft.clear();
	hticGodEyesRight.clear();
	hticTopLeftCap.clear();
	hticTopRightCap.clear();

	hticAssetsLoaded = false;
	hticHasCoreStatusbar = false;
}

void ST_HticEnsureSurfaces()
{
	const int currentBpp = I_GetVideoBitDepth() == 32 ? 32 : 8;

	if (stbar_surface != nullptr)
	{
		if (stbar_surface->getWidth() != HTIC_BASE_WIDTH ||
		    stbar_surface->getHeight() != HTIC_BASE_HEIGHT ||
		    stbar_surface->getBitsPerPixel() != currentBpp)
		{
			I_FreeSurface(stbar_surface);
			stbar_surface = nullptr;
		}
	}

	if (stnum_surface != nullptr)
	{
		I_FreeSurface(stnum_surface);
		stnum_surface = nullptr;
	}

	if (stbar_surface == nullptr)
	{
		stbar_surface = I_AllocateSurface(HTIC_BASE_WIDTH, HTIC_BASE_HEIGHT, currentBpp);
	}
}

void ST_HticSetLayoutHidden()
{
	IWindowSurface* surface = R_GetRenderingSurface();
	if (!surface)
	{
		ST_X = 0;
		ST_Y = 0;
		ST_WIDTH = 0;
		ST_HEIGHT = 0;
		return;
	}

	ST_X = 0;
	ST_Y = surface->getHeight();
	ST_WIDTH = 0;
	ST_HEIGHT = 0;
}

void ST_HticSetLayoutVisible()
{
	IWindowSurface* surface = R_GetRenderingSurface();
	if (!surface)
	{
		ST_HticSetLayoutHidden();
		return;
	}

	const int surfaceWidth = surface->getWidth();
	const int surfaceHeight = surface->getHeight();

	if (st_scale)
	{
		ST_HEIGHT = std::max(1, HTIC_BASE_HEIGHT * surfaceHeight / 200);
		ST_WIDTH = std::max(1, HTIC_BASE_WIDTH * surfaceHeight / 200);
	}
	else
	{
		ST_HEIGHT = HTIC_BASE_HEIGHT;
		ST_WIDTH = HTIC_BASE_WIDTH;
	}

	ST_WIDTH = std::min(ST_WIDTH, surfaceWidth);
	ST_HEIGHT = std::min(ST_HEIGHT, surfaceHeight);
	ST_X = (surfaceWidth - ST_WIDTH) / 2;
	ST_Y = surfaceHeight - ST_HEIGHT;
}

int ST_HticGetReadyAmmo(const player_t& plyr)
{
	if (weaponinfo[plyr.readyweapon].ammotype == am_noammo)
		return ST_DONT_DRAW_NUM;

	return plyr.ammo[weaponinfo[plyr.readyweapon].ammotype];
}

void ST_HticDrawNumber(const DCanvas* canvas, int value, int rightX, int y, int maxDigits,
                       lumpHandle_t* digits)
{
	if (value == ST_DONT_DRAW_NUM)
	{
		if (!hticLameNum.empty())
			canvas->DrawPatch(W_ResolvePatchHandle(hticLameNum), rightX, y);
		return;
	}

	int number = value;
	bool negative = number < 0;
	if (negative)
		number = -number;

	const patch_t* p0 = W_ResolvePatchHandle(digits[0]);
	const int digitWidth = p0->width();
	int drawX = rightX;

	if (number == 0)
	{
		drawX -= digitWidth;
		canvas->DrawPatch(W_ResolvePatchHandle(digits[0]), drawX, y);
	}
	else
	{
		for (int numDigits = 0; numDigits < maxDigits && number > 0; numDigits++)
		{
			const int d = number % 10;
			drawX -= digitWidth;
			canvas->DrawPatch(W_ResolvePatchHandle(digits[d]), drawX, y);
			number /= 10;
		}
	}

	if (negative && !hticNegNum.empty())
	{
		canvas->DrawPatch(W_ResolvePatchHandle(hticNegNum), drawX - 8, y);
	}
}

void ST_HticDrawTextFallback()
{
	const player_t& plyr = displayplayer();
	const int ammo = ST_HticGetReadyAmmo(plyr);
	const int lineY = ST_Y + 4 * CleanYfac;

	screen->DrawText(CR_GOLD, ST_X + 8 * CleanXfac, lineY,
	                 fmt::format("HEALTH {:3d}", std::max(0, plyr.health)).c_str());
	if (ammo >= 0)
	{
		screen->DrawText(CR_GOLD, ST_X + (ST_WIDTH / 2) - (40 * CleanXfac), lineY,
		                 fmt::format("AMMO {:3d}", ammo).c_str());
	}
	else
	{
		screen->DrawText(CR_GOLD, ST_X + (ST_WIDTH / 2) - (40 * CleanXfac), lineY, "AMMO ---");
	}

	screen->DrawText(CR_GOLD, ST_X + ST_WIDTH - (120 * CleanXfac), lineY,
	                 fmt::format("ARMOR {:3d}", std::max(0, plyr.armorpoints)).c_str());
}

void ST_HticUpdateData()
{
	const player_t& plyr = displayplayer();

	hticHealth = std::max(0, plyr.health);
	hticArmor = std::max(0, plyr.armorpoints);
	hticReadyAmmo = ST_HticGetReadyAmmo(plyr);

	for (int i = 0; i < 3; i++)
	{
		hticKeyboxes[i] = plyr.cards[i] ? i : -1;
	}

	if (hticHealth != hticChainHealth)
	{
		const int minValue = std::min(hticHealth, hticChainHealth);
		const int maxValue = std::max(hticHealth, hticChainHealth);
		int diff = (maxValue - minValue) >> 2;

		if (diff < 1)
			diff = 1;
		else if (diff > 8)
			diff = 8;

		if (hticHealth > hticChainHealth)
			hticChainHealth += diff;
		else
			hticChainHealth -= diff;
	}

	if (level.time & 1)
	{
		hticChainWiggle = M_Random() & 1;
	}
}

void ST_HticShadeChainMouths(IWindowSurface* surface, int left, int right, int top, int height)
{
	if (!surface)
		return;

	const int bpp = surface->getBitsPerPixel();
	const int pitch = surface->getPitch();
	const PixelFormat* pf = surface->getPixelFormat();
	uint8_t* base = surface->getBuffer();
	const argb_t* pal = V_GetDefaultPalette()->basecolors;

	// Ported from odaraven's ST_HticShadeChain geometry:
	// right-side sample starts at right+15 and moves inward by 2 each column.
	int diff = right + 15 - left;

	for (int i = 0; i < 16; i++)
	{
		const int lx = left + i;
		const int rx = left + diff;
		diff -= 2;

		if (lx < 0 || rx < 0 || lx >= surface->getWidth() || rx >= surface->getWidth())
			continue;

		for (int y = top; y < top + height; y++)
		{
			if (y < 0 || y >= surface->getHeight())
				continue;

			const argb_t* darkener = Col2RGB8[18 + i * 2];

			if (bpp == 8)
			{
				uint8_t* l = base + y * pitch + lx;
				uint8_t* r = base + y * pitch + rx;
				const argb_t lbg = darkener[*l] | 0x1f07c1f;
				const argb_t rbg = darkener[*r] | 0x1f07c1f;
				*l = RGB32k[0][0][lbg & (lbg >> 15)];
				*r = RGB32k[0][0][rbg & (rbg >> 15)];
			}
			else if (bpp == 32)
			{
				uint32_t* l = reinterpret_cast<uint32_t*>(base + y * pitch + lx * 4);
				uint32_t* r = reinterpret_cast<uint32_t*>(base + y * pitch + rx * 4);

				auto shade32 = [&](uint32_t& px)
				{
					const uint8_t a = pf->a(px);
					const palindex_t idx = V_BestColor(pal, pf->r(px), pf->g(px), pf->b(px));
					const argb_t bg = darkener[idx] | 0x1f07c1f;
					const palindex_t shadedIdx = RGB32k[0][0][bg & (bg >> 15)];
					const argb_t shaded = pal[shadedIdx];
					px = pf->convert(a, shaded.getr(), shaded.getg(), shaded.getb());
				};

				shade32(*l);
				shade32(*r);
			}
		}
	}
}

void ST_HticDrawBackgroundAndWidgets()
{
	const player_t& plyr = displayplayer();
	DCanvas* canvas = stbar_surface->getDefaultCanvas();

	if (!hticBarBack.empty())
		canvas->DrawPatch(W_ResolvePatchHandle(hticBarBack), 0, 0);

	canvas->DrawPatch(W_ResolvePatchHandle(hticBarMain), 34, 2);

	const int clampedChainHealth = clamp(hticChainHealth, 0, 100);
	const int chainPos = (clampedChainHealth << 8) / 100;
	const int chainY = 32 + ((plyr.health != hticChainHealth) ? hticChainWiggle : 0);

	if (!hticChain.empty())
		canvas->DrawPatch(W_ResolvePatchHandle(hticChain), 2 + (chainPos % 17), chainY);
	if (!hticLifeGem.empty())
		canvas->DrawPatch(W_ResolvePatchHandle(hticLifeGem), 17 + chainPos, chainY);
	if (!hticLeftFace.empty())
		canvas->DrawPatch(W_ResolvePatchHandle(hticLeftFace), 0, 32);
	if (!hticRightFace.empty())
		canvas->DrawPatch(W_ResolvePatchHandle(hticRightFace), 276, 32);

	if (((plyr.cheats & CF_GODMODE) || plyr.powers[pw_invulnerability]) && !hticGodEyesLeft.empty() &&
	    !hticGodEyesRight.empty())
	{
		canvas->DrawPatch(W_ResolvePatchHandle(hticGodEyesLeft), 16, 9);
		canvas->DrawPatch(W_ResolvePatchHandle(hticGodEyesRight), 287, 9);
	}

	ST_HticShadeChainMouths(stbar_surface, 19, 277, 32, 10);

	ST_HticDrawNumber(canvas, hticHealth, 87, 12, 3, hticBigNum);
	ST_HticDrawNumber(canvas, hticReadyAmmo, 135, 4, 3, hticBigNum);
	ST_HticDrawNumber(canvas, hticArmor, 254, 12, 3, hticBigNum);

	for (int i = 0; i < 3; i++)
	{
		if (hticKeyboxes[i] == -1 || hticKeys[i].empty())
			continue;

		const int y = 6 + i * 8;
		canvas->DrawPatch(W_ResolvePatchHandle(hticKeys[i]), 153, y);
	}
}

void ST_HticDrawTopCaps(IWindowSurface* surface)
{
	if (hticTopLeftCap.empty() || hticTopRightCap.empty())
		return;

	const patch_t* left = W_ResolvePatchHandle(hticTopLeftCap);
	const patch_t* right = W_ResolvePatchHandle(hticTopRightCap);
	const int leftW = std::max(1, left->width() * ST_WIDTH / HTIC_BASE_WIDTH);
	const int leftH = std::max(1, left->height() * ST_HEIGHT / HTIC_BASE_HEIGHT);
	const int rightW = std::max(1, right->width() * ST_WIDTH / HTIC_BASE_WIDTH);
	const int rightH = std::max(1, right->height() * ST_HEIGHT / HTIC_BASE_HEIGHT);

	// Legacy Heretic places these at x=0 and x=290 in a 320-wide layout.
	const int leftX = ST_X;
	const int idealRightX = ST_X + (290 * ST_WIDTH) / HTIC_BASE_WIDTH;
	const int maxRightX = ST_X + ST_WIDTH - rightW;
	const int rightX = std::clamp(idealRightX, ST_X, maxRightX);
	const int leftY = ST_Y - leftH + 1;
	const int rightY = ST_Y - rightH + 1;

	const DCanvas* canvas = surface->getDefaultCanvas();
	canvas->DrawPatchStretched(left, leftX, leftY, leftW, leftH);
	canvas->DrawPatchStretched(right, rightX, rightY, rightW, rightH);
}
} // namespace

void ST_HticInit()
{
	ST_HticClearAssets();

	for (int i = 0; i < 10; i++)
	{
		hticBigNum[i] = ST_HticTryCachePatch(fmt::format("IN{}", i).c_str());
		hticSmallNum[i] = ST_HticTryCachePatch(fmt::format("SMALLIN{}", i).c_str());
	}

	hticNegNum = ST_HticTryCachePatch("NEGNUM");
	hticLameNum = ST_HticTryCachePatch("LAME");

	hticKeys[0] = ST_HticTryCachePatch("YKEYICON");
	hticKeys[1] = ST_HticTryCachePatch("GKEYICON");
	hticKeys[2] = ST_HticTryCachePatch("BKEYICON");

	hticBarBack = ST_HticTryCachePatch("BARBACK");
	hticBarMain = multiplayer ? ST_HticTryCachePatch("STATBAR") : ST_HticTryCachePatch("LIFEBAR");
	if (hticBarMain.empty())
		hticBarMain = ST_HticTryCachePatch("STATBAR");

	hticChain = ST_HticTryCachePatch("CHAIN");
	hticLifeGem = ST_HticTryCachePatch("LIFEGEM2");
	hticLeftFace = ST_HticTryCachePatch("LTFACE");
	hticRightFace = ST_HticTryCachePatch("RTFACE");
	hticGodEyesLeft = ST_HticTryCachePatch("GOD1");
	hticGodEyesRight = ST_HticTryCachePatch("GOD2");
	hticTopLeftCap = ST_HticTryCachePatch("LTFCTOP");
	hticTopRightCap = ST_HticTryCachePatch("RTFCTOP");

	hticAssetsLoaded = true;
	hticHasCoreStatusbar = !hticBarMain.empty() && !hticBigNum[0].empty();

	hticHealth = 100;
	hticChainHealth = 100;
	hticChainWiggle = 0;

	ST_HticEnsureSurfaces();
}

void ST_HticStart()
{
	ST_ForceRefresh();
	ST_HticUpdateData();
	hticChainHealth = hticHealth;
}

void ST_HticTicker()
{
	if (!hticAssetsLoaded)
		return;

	ST_HticUpdateData();
}

void ST_HticDrawer()
{
	if (!R_StatusBarVisible())
	{
		ST_HticSetLayoutHidden();
		return;
	}

	ST_HticSetLayoutVisible();

	if (!hticHasCoreStatusbar)
	{
		ST_HticDrawTextFallback();
		return;
	}

	ST_HticEnsureSurfaces();
	if (!stbar_surface)
	{
		ST_HticDrawTextFallback();
		return;
	}

	if (R_GetRenderingSurface()->getWidth() > ST_WIDTH)
	{
		R_DrawBorder(0, ST_Y, ST_X, R_GetRenderingSurface()->getHeight());
		R_DrawBorder(R_GetRenderingSurface()->getWidth() - ST_X, ST_Y,
		             R_GetRenderingSurface()->getWidth(), R_GetRenderingSurface()->getHeight());
	}

	stbar_surface->lock();
	ST_HticDrawBackgroundAndWidgets();
	stbar_surface->unlock();

	IWindowSurface* surface = R_GetRenderingSurface();
	surface->blitcrop(stbar_surface, 0, 0, HTIC_BASE_WIDTH, HTIC_BASE_HEIGHT, ST_X, ST_Y, ST_WIDTH,
	                 ST_HEIGHT);
	ST_HticDrawTopCaps(surface);
}

void ST_HticShutdown()
{
	ST_HticClearAssets();

	if (stbar_surface)
	{
		I_FreeSurface(stbar_surface);
		stbar_surface = nullptr;
	}

	if (stnum_surface)
	{
		I_FreeSurface(stnum_surface);
		stnum_surface = nullptr;
	}
}
