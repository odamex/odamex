// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------


#ifndef __V_VIDEO_H__
#define __V_VIDEO_H__

#include <string>

#include "doomtype.h"
#include "v_palette.h"
#include "doomdef.h"

// Needed because we are refering to patches.
#include "r_data.h"

class IWindowSurface;

extern int CleanXfac, CleanYfac;

//
// VIDEO
//
// [RH] Made screens more implementation-independant:
//
// V_LockScreen() must be called before drawing on a
// screen, and V_UnlockScreen() must be called when
// drawing is finished. As far as ZDoom is concerned,
// there are only two display depths: 8-bit indexed and
// 32-bit RGBA. The video driver is expected to be able
// to convert these to a format supported by the hardware.
// As such, the bits field is only used as an indicator
// of the output display depth and not as the screen's
// display depth (use is8bit for that).
class DCanvas : DObject
{
	DECLARE_CLASS (DCanvas, DObject)
public:
	enum EWrapperCode
	{
		EWrapper_Normal = 0,		// Just draws the patch as is
		EWrapper_Lucent = 1,		// Mixes the patch with the background
		EWrapper_Translated = 2,	// Color translates the patch and draws it
		EWrapper_TlatedLucent = 3,	// Translates the patch and mixes it with the background
		EWrapper_Colored = 4,		// Fills the patch area with a solid color
		EWrapper_ColoredLucent = 5	// Mixes a solid color in the patch area with the background
	};

	DCanvas(IWindowSurface* surface) :
		mSurface(surface)
	{ }

	virtual ~DCanvas ()
	{ }

	IWindowSurface* getSurface()
	{	return mSurface;	}

	const IWindowSurface* getSurface() const
	{	return mSurface;	}

	// Draw a linear block of pixels into the view buffer.
	void DrawBlock (int x, int y, int width, int height, const byte *src) const;

	// Reads a linear block of pixels from the view buffer.
	void GetBlock (int x, int y, int width, int height, byte *dest) const;

	// Reads a transposed block of pixels from the view buffer
	void GetTransposedBlock(int x, int y, int _width, int _height, byte* dest) const;

	// Darken a rectangle of th canvas
	void Dim (int x, int y, int width, int height, const char* color, float amount) const;
	void Dim (int x, int y, int width, int height) const;

	// Fill an area with a 64x64 flat texture
	void FlatFill(const Texture* texture, int left, int top, int right, int bottom) const;

	// Set an area to a specified color
	void Clear(int left, int top, int right, int bottom, argb_t color) const;

	// Text drawing functions
	// Output a line of text using the console font
	void PrintStr(int x, int y, const char *s, int default_color = -1, bool use_color_codes = true) const;

	// Output some text with wad heads-up font
	inline void DrawText (int normalcolor, int x, int y, const byte *string) const;
	inline void DrawTextLuc (int normalcolor, int x, int y, const byte *string) const;
	inline void DrawTextClean (int normalcolor, int x, int y, const byte *string) const;		// Does not adjust x and y
	inline void DrawTextCleanLuc (int normalcolor, int x, int y, const byte *string) const;		// ditto
	inline void DrawTextCleanMove (int normalcolor, int x, int y, const byte *string) const;	// This one does
	inline void DrawTextStretched (int normalcolor, int x, int y, const byte *string, int scalex, int scaley) const;
	inline void DrawTextStretchedLuc (int normalcolor, int x, int y, const byte *string, int scalex, int scaley) const;

	inline void DrawText (int normalcolor, int x, int y, const char *string) const;
	inline void DrawTextLuc (int normalcolor, int x, int y, const char *string) const;
	inline void DrawTextClean (int normalcolor, int x, int y, const char *string) const;
	inline void DrawTextCleanLuc (int normalcolor, int x, int y, const char *string) const;
	inline void DrawTextCleanMove (int normalcolor, int x, int y, const char *string) const;
	inline void DrawTextStretched (int normalcolor, int x, int y, const char *string, int scalex, int scaley) const;
	inline void DrawTextStretchedLuc (int normalcolor, int x, int y, const char *string, int scalex, int scaley) const;

	void DrawTextureFlipped(const Texture* texture, int x, int y) const;

	inline void DrawTexture(const Texture* texture, int x, int y) const;
	inline void DrawTextureStretched(const Texture* texture, int x, int y, int dw, int dh) const;
	inline void DrawTextureDirect(const Texture* texture, int x, int y) const;
	inline void DrawTextureIndirect (const Texture* texture, int x, int y) const;
	inline void DrawTextureClean (const Texture* texture, int x, int y) const;
	inline void DrawTextureCleanNoMove (const Texture* texture, int x, int y) const;

	void DrawTextureFullScreen(const Texture* texture) const;

	inline void DrawLucentTexture (const Texture* texture, int x, int y) const;
	inline void DrawLucentTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const;
	inline void DrawLucentTextureDirect (const Texture* texture, int x, int y) const;
	inline void DrawLucentTextureIndirect (const Texture* texture, int x, int y) const;
	inline void DrawLucentTextureClean (const Texture* texture, int x, int y) const;
	inline void DrawLucentTextureCleanNoMove (const Texture* texture, int x, int y) const;

	inline void DrawTranslatedTexture (const Texture* texture, int x, int y) const;
	inline void DrawTranslatedTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const;
	inline void DrawTranslatedTextureDirect (const Texture* texture, int x, int y) const;
	inline void DrawTranslatedTextureIndirect (const Texture* texture, int x, int y) const;
	inline void DrawTranslatedTextureClean (const Texture* texture, int x, int y) const;
	inline void DrawTranslatedTextureCleanNoMove (const Texture* texture, int x, int y) const;

	inline void DrawTranslatedLucentTexture (const Texture* texture, int x, int y) const;
	inline void DrawTranslatedLucentTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const;
	inline void DrawTranslatedLucentTextureDirect (const Texture* texture, int x, int y) const;
	inline void DrawTranslatedLucentTextureIndirect (const Texture* texture, int x, int y) const;
	inline void DrawTranslatedLucentTextureClean (const Texture* texture, int x, int y) const;
	inline void DrawTranslatedLucentTextureCleanNoMove (const Texture* texture, int x, int y) const;

	inline void DrawColoredTexture (const Texture* texture, int x, int y) const;
	inline void DrawColoredTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const;
	inline void DrawColoredTextureDirect (const Texture* texture, int x, int y) const;
	inline void DrawColoredTextureIndirect (const Texture* texture, int x, int y) const;
	inline void DrawColoredTextureClean (const Texture* texture, int x, int y) const;
	inline void DrawColoredTextureCleanNoMove (const Texture* texture, int x, int y) const;

	inline void DrawColoredLucentTexture (const Texture* texture, int x, int y) const;
	inline void DrawColoredLucentTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const;
	inline void DrawColoredLucentTextureDirect (const Texture* texture, int x, int y) const;
	inline void DrawColoredLucentTextureIndirect (const Texture* texture, int x, int y) const;
	inline void DrawColoredLucentTextureClean (const Texture* texture, int x, int y) const;
	inline void DrawColoredLucentTextureCleanNoMove (const Texture* texture, int x, int y) const;

protected:
	void TextWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const;
	void TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const;
	void TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string, int scalex, int scaley) const;

	void DrawWrapper (EWrapperCode drawer, const Texture* texture, int x, int y) const;
	void DrawSWrapper (EWrapperCode drawer, const Texture* texture, int x, int y, int destwidth, int destheight) const;
	void DrawIWrapper (EWrapperCode drawer, const Texture* texture, int x, int y) const;
	void DrawCWrapper (EWrapperCode drawer, const Texture* texture, int x, int y) const;
	void DrawCNMWrapper (EWrapperCode drawer, const Texture* texture, int x, int y) const;

	static void DrawPatchP (const byte *source, byte *dest, int count, int pitch);
	static void DrawLucentPatchP (const byte *source, byte *dest, int count, int pitch);
	static void DrawTranslatedPatchP (const byte *source, byte *dest, int count, int pitch);
	static void DrawTlatedLucentPatchP (const byte *source, byte *dest, int count, int pitch);
	static void DrawColoredPatchP (const byte *source, byte *dest, int count, int pitch);
	static void DrawColorLucentPatchP (const byte *source, byte *dest, int count, int pitch);

	static void DrawPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc);
	static void DrawLucentPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc);
	static void DrawTranslatedPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc);
	static void DrawTlatedLucentPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc);

	static void DrawPatchD (const byte *source, byte *dest, int count, int pitch);
	static void DrawLucentPatchD (const byte *source, byte *dest, int count, int pitch);
	static void DrawTranslatedPatchD (const byte *source, byte *dest, int count, int pitch);
	static void DrawTlatedLucentPatchD (const byte *source, byte *dest, int count, int pitch);
	static void DrawColoredPatchD (const byte *source, byte *dest, int count, int pitch);
	static void DrawColorLucentPatchD (const byte *source, byte *dest, int count, int pitch);

	static void DrawPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc);
	static void DrawLucentPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc);
	static void DrawTranslatedPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc);
	static void DrawTlatedLucentPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc);

	typedef void (*vdrawfunc) (const byte *source, byte *dest, int count, int pitch);
	typedef void (*vdrawsfunc) (const byte *source, byte *dest, int count, int pitch, int yinc);

	// Palettized versions of the column drawers
	static vdrawfunc Pfuncs[6];
	static vdrawsfunc Psfuncs[6];

	// Direct (true-color) versions of the column drawers
	static vdrawfunc Dfuncs[6];
	static vdrawsfunc Dsfuncs[6];

	// The current set of column drawers (set in V_SetResolution)
	static vdrawfunc *m_Drawfuncs;
	static vdrawsfunc *m_Drawsfuncs;

private:
	IWindowSurface*			mSurface;

	int getCleanX(int x) const;
	int getCleanY(int y) const;
};

inline void DCanvas::DrawText (int normalcolor, int x, int y, const byte *string) const
{
	TextWrapper (EWrapper_Translated, normalcolor, x, y, string);
}
inline void DCanvas::DrawTextLuc (int normalcolor, int x, int y, const byte *string) const
{
	TextWrapper (EWrapper_TlatedLucent, normalcolor, x, y, string);
}
inline void DCanvas::DrawTextClean (int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper (EWrapper_Translated, normalcolor, x, y, string);
}
inline void DCanvas::DrawTextCleanLuc (int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper (EWrapper_TlatedLucent, normalcolor, x, y, string);
}
inline void DCanvas::DrawTextCleanMove (int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper (EWrapper_Translated, normalcolor, getCleanX(x), getCleanY(y), string);
}
inline void DCanvas::DrawTextStretched (int normalcolor, int x, int y, const byte *string, int scalex, int scaley) const
{
	TextSWrapper (EWrapper_Translated, normalcolor, x, y, string, scalex, scaley);
}

inline void DCanvas::DrawTextStretchedLuc (int normalcolor, int x, int y, const byte *string, int scalex, int scaley) const
{
	TextSWrapper (EWrapper_TlatedLucent, normalcolor, x, y, string, scalex, scaley);
}

inline void DCanvas::DrawText (int normalcolor, int x, int y, const char *string) const
{
	TextWrapper (EWrapper_Translated, normalcolor, x, y, (const byte *)string);
}
inline void DCanvas::DrawTextLuc (int normalcolor, int x, int y, const char *string) const
{
	TextWrapper (EWrapper_TlatedLucent, normalcolor, x, y, (const byte *)string);
}
inline void DCanvas::DrawTextClean (int normalcolor, int x, int y, const char *string) const
{
	TextSWrapper (EWrapper_Translated, normalcolor, x, y, (const byte *)string);
}
inline void DCanvas::DrawTextCleanLuc (int normalcolor, int x, int y, const char *string) const
{
	TextSWrapper (EWrapper_TlatedLucent, normalcolor, x, y, (const byte *)string);
}
inline void DCanvas::DrawTextCleanMove (int normalcolor, int x, int y, const char *string) const
{
	TextSWrapper (EWrapper_Translated, normalcolor, getCleanX(x), getCleanY(y), (const byte*)string);
}
inline void DCanvas::DrawTextStretched (int normalcolor, int x, int y, const char *string, int scalex, int scaley) const
{
	TextSWrapper (EWrapper_Translated, normalcolor, x, y, (const byte *)string, scalex, scaley);
}
inline void DCanvas::DrawTextStretchedLuc (int normalcolor, int x, int y, const char *string, int scalex, int scaley) const
{
	TextSWrapper (EWrapper_TlatedLucent, normalcolor, x, y, (const byte *)string, scalex, scaley);
}

inline void DCanvas::DrawTexture (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_Normal, texture, x, y);
}
inline void DCanvas::DrawTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_Normal, texture, x, y, dw, dh);
}
inline void DCanvas::DrawTextureDirect (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_Normal, texture, x, y);
}
inline void DCanvas::DrawTextureIndirect (const Texture* texture, int x, int y) const
{
	DrawIWrapper (EWrapper_Normal, texture, x, y);
}
inline void DCanvas::DrawTextureClean (const Texture* texture, int x, int y) const
{
	DrawCWrapper (EWrapper_Normal, texture, x, y);
}
inline void DCanvas::DrawTextureCleanNoMove (const Texture* texture, int x, int y) const
{
	DrawCNMWrapper (EWrapper_Normal, texture, x, y);
}

inline void DCanvas::DrawLucentTexture (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_Lucent, texture, x, y);
}
inline void DCanvas::DrawLucentTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_Lucent, texture, x, y, dw, dh);
}
inline void DCanvas::DrawLucentTextureDirect (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_Lucent, texture, x, y);
}
inline void DCanvas::DrawLucentTextureIndirect (const Texture* texture, int x, int y) const
{
	DrawIWrapper (EWrapper_Lucent, texture, x, y);
}
inline void DCanvas::DrawLucentTextureClean (const Texture* texture, int x, int y) const
{
	DrawCWrapper (EWrapper_Lucent, texture, x, y);
}
inline void DCanvas::DrawLucentTextureCleanNoMove (const Texture* texture, int x, int y) const
{
	DrawCNMWrapper (EWrapper_Lucent, texture, x, y);
}

inline void DCanvas::DrawTranslatedTexture (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_Translated, texture, x, y);
}
inline void DCanvas::DrawTranslatedTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_Translated, texture, x, y, dw, dh);
}
inline void DCanvas::DrawTranslatedTextureDirect (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_Translated, texture, x, y);
}
inline void DCanvas::DrawTranslatedTextureIndirect (const Texture* texture, int x, int y) const
{
	DrawIWrapper (EWrapper_Translated, texture, x, y);
}
inline void DCanvas::DrawTranslatedTextureClean (const Texture* texture, int x, int y) const
{
	DrawCWrapper (EWrapper_Translated, texture, x, y);
}
inline void DCanvas::DrawTranslatedTextureCleanNoMove (const Texture* texture, int x, int y) const
{
	DrawCNMWrapper (EWrapper_Translated, texture, x, y);
}

inline void DCanvas::DrawTranslatedLucentTexture (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_TlatedLucent, texture, x, y);
}
inline void DCanvas::DrawTranslatedLucentTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_TlatedLucent, texture, x, y, dw, dh);
}
inline void DCanvas::DrawTranslatedLucentTextureDirect (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_TlatedLucent, texture, x, y);
}
inline void DCanvas::DrawTranslatedLucentTextureIndirect (const Texture* texture, int x, int y) const
{
	DrawIWrapper (EWrapper_TlatedLucent, texture, x, y);
}
inline void DCanvas::DrawTranslatedLucentTextureClean (const Texture* texture, int x, int y) const
{
	DrawCWrapper (EWrapper_TlatedLucent, texture, x, y);
}
inline void DCanvas::DrawTranslatedLucentTextureCleanNoMove (const Texture* texture, int x, int y) const
{
	DrawCNMWrapper (EWrapper_TlatedLucent, texture, x, y);
}

inline void DCanvas::DrawColoredTexture (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_Colored, texture, x, y);
}
inline void DCanvas::DrawColoredTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_Colored, texture, x, y, dw, dh);
}
inline void DCanvas::DrawColoredTextureDirect (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_Colored, texture, x, y);
}
inline void DCanvas::DrawColoredTextureIndirect (const Texture* texture, int x, int y) const
{
	DrawIWrapper (EWrapper_Colored, texture, x, y);
}
inline void DCanvas::DrawColoredTextureClean (const Texture* texture, int x, int y) const
{
	DrawCWrapper (EWrapper_Colored, texture, x, y);
}
inline void DCanvas::DrawColoredTextureCleanNoMove (const Texture* texture, int x, int y) const
{
	DrawCNMWrapper (EWrapper_Colored, texture, x, y);
}

inline void DCanvas::DrawColoredLucentTexture (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_ColoredLucent, texture, x, y);
}
inline void DCanvas::DrawColoredLucentTextureStretched (const Texture* texture, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_ColoredLucent, texture, x, y, dw, dh);
}
inline void DCanvas::DrawColoredLucentTextureDirect (const Texture* texture, int x, int y) const
{
	DrawWrapper (EWrapper_ColoredLucent, texture, x, y);
}
inline void DCanvas::DrawColoredLucentTextureIndirect (const Texture* texture, int x, int y) const
{
	DrawIWrapper (EWrapper_ColoredLucent, texture, x, y);
}
inline void DCanvas::DrawColoredLucentTextureClean (const Texture* texture, int x, int y) const
{
	DrawCWrapper (EWrapper_ColoredLucent, texture, x, y);
}
inline void DCanvas::DrawColoredLucentTextureCleanNoMove (const Texture* texture, int x, int y) const
{
	DrawCNMWrapper (EWrapper_ColoredLucent, texture, x, y);
}

// This is the screen updated by I_FinishUpdate.
extern	DCanvas *screen;

// Translucency tables
extern argb_t Col2RGB8[65][256];
extern palindex_t RGB32k[32][32][32];

void V_Init();
void STACK_ARGS V_Close();

void V_ForceVideoModeAdjustment();
void V_AdjustVideoMode();

// The color to fill with for #4 and #5 above
extern int V_ColorFill;

// The color map for #1 and #2 above
extern translationref_t V_ColorMap;

// The palette lookup table to be used with for direct modes
extern shaderef_t V_Palette;

void V_MarkRect (int x, int y, int width, int height);

// Returns the closest color to the one desired. String
// should be of the form "rr gg bb" or the name of a color
// as defined in the X11R6RGB lump.
argb_t V_GetColorFromString(const std::string& str);

void V_SetResolution(uint16_t width, uint16_t height);

template<>
forceinline palindex_t rt_blend2(const palindex_t bg, const int bga, const palindex_t fg, const int fga)
{
	// Crazy 8bpp alpha-blending using lookup tables and bit twiddling magic
	argb_t bgARGB = Col2RGB8[bga >> 2][bg];
	argb_t fgARGB = Col2RGB8[fga >> 2][fg];

	argb_t mix = (fgARGB + bgARGB) | 0x1f07c1f;
	return RGB32k[0][0][mix & (mix >> 15)];
}

template<>
forceinline argb_t rt_blend2(const argb_t bg, const int bga, const argb_t fg, const int fga)
{
	return alphablend2a(bg, bga, fg, fga);
}

bool V_UsePillarBox();
bool V_UseLetterBox();
bool V_UseWidescreen();

// Alpha blend between two RGB colors with only dest alpha value
// 0 <=   toa <= 255
forceinline argb_t alphablend1a(const argb_t from, const argb_t to, const int toa)
{
	const int fr = from.getr();
	const int fg = from.getg();
	const int fb = from.getb();

	const int dr = to.getr() - fr;
	const int dg = to.getg() - fg;
	const int db = to.getb() - fb;

	return argb_t(
		fr + ((dr * toa) >> 8),
		fg + ((dg * toa) >> 8),
		fb + ((db * toa) >> 8));
}

// Alpha blend between two RGB colors with two alpha values
// 0 <= froma <= 255
// 0 <=   toa <= 255
forceinline argb_t alphablend2a(const argb_t from, const int froma, const argb_t to, const int toa)
{
	return argb_t(
		(from.getr() * froma + to.getr() * toa) >> 8,
		(from.getg() * froma + to.getg() * toa) >> 8,
		(from.getb() * froma + to.getb() * toa) >> 8);
}

void V_DrawFPSWidget();
void V_DrawFPSTicker();

#endif // __V_VIDEO_H__