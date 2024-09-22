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


#pragma once


#include "v_palette.h"
#include "m_vectors.h"

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
	void FlatFill (int left, int top, int right, int bottom, unsigned int flatlength, const byte *src) const;

	// Set an area to a specified color
	void Clear(int left, int top, int right, int bottom, argb_t color) const;

	// Draw a line with a specified color
	void Line(const v2int_t src, const v2int_t dst, argb_t color) const;

	// Draw an empty box with a specified border color
	void Box(const rectInt_t& bounds, const argb_t color) const;

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

	// Patch drawing functions
	void DrawPatchFlipped (const patch_t *patch, int x, int y) const;

	inline void DrawPatch (const patch_t *patch, int x, int y) const;
	inline void DrawPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const;
	inline void DrawPatchDirect (const patch_t *patch, int x, int y) const;
	inline void DrawPatchIndirect (const patch_t *patch, int x, int y) const;
	inline void DrawPatchClean (const patch_t *patch, int x, int y) const;
	inline void DrawPatchCleanNoMove (const patch_t *patch, int x, int y) const;

	void DrawPatchFullScreen(const patch_t* patch, bool clear) const;

	inline void DrawLucentPatch (const patch_t *patch, int x, int y) const;
	inline void DrawLucentPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const;
	inline void DrawLucentPatchDirect (const patch_t *patch, int x, int y) const;
	inline void DrawLucentPatchIndirect (const patch_t *patch, int x, int y) const;
	inline void DrawLucentPatchClean (const patch_t *patch, int x, int y) const;
	inline void DrawLucentPatchCleanNoMove (const patch_t *patch, int x, int y) const;

	inline void DrawTranslatedPatch (const patch_t *patch, int x, int y) const;
	inline void DrawTranslatedPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const;
	inline void DrawTranslatedPatchDirect (const patch_t *patch, int x, int y) const;
	inline void DrawTranslatedPatchIndirect (const patch_t *patch, int x, int y) const;
	inline void DrawTranslatedPatchClean (const patch_t *patch, int x, int y) const;
	inline void DrawTranslatedPatchCleanNoMove (const patch_t *patch, int x, int y) const;

	inline void DrawTranslatedLucentPatch (const patch_t *patch, int x, int y) const;
	inline void DrawTranslatedLucentPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const;
	inline void DrawTranslatedLucentPatchDirect (const patch_t *patch, int x, int y) const;
	inline void DrawTranslatedLucentPatchIndirect (const patch_t *patch, int x, int y) const;
	inline void DrawTranslatedLucentPatchClean (const patch_t *patch, int x, int y) const;
	inline void DrawTranslatedLucentPatchCleanNoMove (const patch_t *patch, int x, int y) const;

	inline void DrawColoredPatch (const patch_t *patch, int x, int y) const;
	inline void DrawColoredPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const;
	inline void DrawColoredPatchDirect (const patch_t *patch, int x, int y) const;
	inline void DrawColoredPatchIndirect (const patch_t *patch, int x, int y) const;
	inline void DrawColoredPatchClean (const patch_t *patch, int x, int y) const;
	inline void DrawColoredPatchCleanNoMove (const patch_t *patch, int x, int y) const;

	inline void DrawColoredLucentPatch (const patch_t *patch, int x, int y) const;
	inline void DrawColoredLucentPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const;
	inline void DrawColoredLucentPatchDirect (const patch_t *patch, int x, int y) const;
	inline void DrawColoredLucentPatchIndirect (const patch_t *patch, int x, int y) const;
	inline void DrawColoredLucentPatchClean (const patch_t *patch, int x, int y) const;
	inline void DrawColoredLucentPatchCleanNoMove (const patch_t *patch, int x, int y) const;

protected:
	void TextWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const;
	void TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const;
	void TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string, int scalex, int scaley) const;

	void DrawWrapper (EWrapperCode drawer, const patch_t *patch, int x, int y) const;
	void DrawSWrapper (EWrapperCode drawer, const patch_t *patch, int x, int y, int destwidth, int destheight) const;
	void DrawIWrapper (EWrapperCode drawer, const patch_t *patch, int x, int y) const;
	void DrawCWrapper (EWrapperCode drawer, const patch_t *patch, int x, int y) const;
	void DrawCNMWrapper (EWrapperCode drawer, const patch_t *patch, int x, int y) const;

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

inline void DCanvas::DrawPatch (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_Normal, patch, x, y);
}
inline void DCanvas::DrawPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_Normal, patch, x, y, dw, dh);
}
inline void DCanvas::DrawPatchDirect (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_Normal, patch, x, y);
}
inline void DCanvas::DrawPatchIndirect (const patch_t *patch, int x, int y) const
{
	DrawIWrapper (EWrapper_Normal, patch, x, y);
}
inline void DCanvas::DrawPatchClean (const patch_t *patch, int x, int y) const
{
	DrawCWrapper (EWrapper_Normal, patch, x, y);
}
inline void DCanvas::DrawPatchCleanNoMove (const patch_t *patch, int x, int y) const
{
	DrawCNMWrapper (EWrapper_Normal, patch, x, y);
}

inline void DCanvas::DrawLucentPatch (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_Lucent, patch, x, y);
}
inline void DCanvas::DrawLucentPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_Lucent, patch, x, y, dw, dh);
}
inline void DCanvas::DrawLucentPatchDirect (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_Lucent, patch, x, y);
}
inline void DCanvas::DrawLucentPatchIndirect (const patch_t *patch, int x, int y) const
{
	DrawIWrapper (EWrapper_Lucent, patch, x, y);
}
inline void DCanvas::DrawLucentPatchClean (const patch_t *patch, int x, int y) const
{
	DrawCWrapper (EWrapper_Lucent, patch, x, y);
}
inline void DCanvas::DrawLucentPatchCleanNoMove (const patch_t *patch, int x, int y) const
{
	DrawCNMWrapper (EWrapper_Lucent, patch, x, y);
}

inline void DCanvas::DrawTranslatedPatch (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_Translated, patch, x, y);
}
inline void DCanvas::DrawTranslatedPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_Translated, patch, x, y, dw, dh);
}
inline void DCanvas::DrawTranslatedPatchDirect (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_Translated, patch, x, y);
}
inline void DCanvas::DrawTranslatedPatchIndirect (const patch_t *patch, int x, int y) const
{
	DrawIWrapper (EWrapper_Translated, patch, x, y);
}
inline void DCanvas::DrawTranslatedPatchClean (const patch_t *patch, int x, int y) const
{
	DrawCWrapper (EWrapper_Translated, patch, x, y);
}
inline void DCanvas::DrawTranslatedPatchCleanNoMove (const patch_t *patch, int x, int y) const
{
	DrawCNMWrapper (EWrapper_Translated, patch, x, y);
}

inline void DCanvas::DrawTranslatedLucentPatch (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_TlatedLucent, patch, x, y);
}
inline void DCanvas::DrawTranslatedLucentPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_TlatedLucent, patch, x, y, dw, dh);
}
inline void DCanvas::DrawTranslatedLucentPatchDirect (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_TlatedLucent, patch, x, y);
}
inline void DCanvas::DrawTranslatedLucentPatchIndirect (const patch_t *patch, int x, int y) const
{
	DrawIWrapper (EWrapper_TlatedLucent, patch, x, y);
}
inline void DCanvas::DrawTranslatedLucentPatchClean (const patch_t *patch, int x, int y) const
{
	DrawCWrapper (EWrapper_TlatedLucent, patch, x, y);
}
inline void DCanvas::DrawTranslatedLucentPatchCleanNoMove (const patch_t *patch, int x, int y) const
{
	DrawCNMWrapper (EWrapper_TlatedLucent, patch, x, y);
}

inline void DCanvas::DrawColoredPatch (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_Colored, patch, x, y);
}
inline void DCanvas::DrawColoredPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_Colored, patch, x, y, dw, dh);
}
inline void DCanvas::DrawColoredPatchDirect (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_Colored, patch, x, y);
}
inline void DCanvas::DrawColoredPatchIndirect (const patch_t *patch, int x, int y) const
{
	DrawIWrapper (EWrapper_Colored, patch, x, y);
}
inline void DCanvas::DrawColoredPatchClean (const patch_t *patch, int x, int y) const
{
	DrawCWrapper (EWrapper_Colored, patch, x, y);
}
inline void DCanvas::DrawColoredPatchCleanNoMove (const patch_t *patch, int x, int y) const
{
	DrawCNMWrapper (EWrapper_Colored, patch, x, y);
}

inline void DCanvas::DrawColoredLucentPatch (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_ColoredLucent, patch, x, y);
}
inline void DCanvas::DrawColoredLucentPatchStretched (const patch_t *patch, int x, int y, int dw, int dh) const
{
	DrawSWrapper (EWrapper_ColoredLucent, patch, x, y, dw, dh);
}
inline void DCanvas::DrawColoredLucentPatchDirect (const patch_t *patch, int x, int y) const
{
	DrawWrapper (EWrapper_ColoredLucent, patch, x, y);
}
inline void DCanvas::DrawColoredLucentPatchIndirect (const patch_t *patch, int x, int y) const
{
	DrawIWrapper (EWrapper_ColoredLucent, patch, x, y);
}
inline void DCanvas::DrawColoredLucentPatchClean (const patch_t *patch, int x, int y) const
{
	DrawCWrapper (EWrapper_ColoredLucent, patch, x, y);
}
inline void DCanvas::DrawColoredLucentPatchCleanNoMove (const patch_t *patch, int x, int y) const
{
	DrawCNMWrapper (EWrapper_ColoredLucent, patch, x, y);
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
