// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//   Nuklear GUI implemented against a 32-bit sufrace.
//   Based on x11_rawfb/nuklear_rawfb.h by Patrick Rudolph, whose copyright
//   is below.
//
// Copyright (c) 2016-2017 Patrick Rudolph <siro@das-labor.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//-----------------------------------------------------------------------------

#include "i_gui.h"

#include <assert.h>
#include <math.h>
#include <time.h>

#include "doomkeys.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_video.h"

struct GUIContext
{
	nk_context ctx;
	struct nk_rect scissors;
	IWindowSurface* surface;
	IWindowSurface* fontSurface;
	nk_font_atlas atlas;

	~GUIContext()
	{
		nk_free(&this->ctx);
		delete this->fontSurface;
	}

	static GUIContext* init(IWindowSurface* surface);
	void update(IWindowSurface* surface);
};

static void CTXSetpixel(const GUIContext* rawfb, const short x0, const short y0,
                        const nk_color col)
{
	if (!(y0 < rawfb->scissors.h && y0 >= rawfb->scissors.y && x0 >= rawfb->scissors.x &&
	      x0 < rawfb->scissors.w))
		return;

	argb_t* pixel = (argb_t*)rawfb->surface->getBuffer(x0, y0);
	pixel->setr(col.r);
	pixel->setg(col.g);
	pixel->setb(col.b);
	pixel->seta(col.a);
}

static void LineHorizontal(const GUIContext* rawfb, const short x0, const short y,
                           const short x1, const nk_color col)
{
	/* This function is called the most. Try to optimize it a bit...
	 * It does not check for scissors or image borders.
	 * The caller has to make sure it does no exceed bounds. */
	argb_t* pixels = (argb_t*)rawfb->surface->getBuffer(x0, y);
	for (short i = x0; i <= x1; i++, pixels++)
	{
		pixels->setr(col.r);
		pixels->setg(col.g);
		pixels->setb(col.b);
		pixels->seta(col.a);
	}
}

static void ImgSetpixel(const IWindowSurface* img, const int x0, const int y0,
                        const nk_color col)
{
	argb_t* pixel = (argb_t*)img->getBuffer(x0, y0);
	pixel->setr(col.r);
	pixel->setg(col.g);
	pixel->setb(col.b);
	pixel->seta(col.a);
}

static nk_color ImgGetpixel(const IWindowSurface* img, const int x0, const int y0)
{
	nk_color col = {0, 0, 0, 0};

	argb_t* pixel = (argb_t*)img->getBuffer(x0, y0);
	col.r = pixel->getr();
	col.g = pixel->getg();
	col.b = pixel->getb();
	col.a = pixel->geta();

	return col;
}

static void ImgBlendpixel(const IWindowSurface* img, const int x0, const int y0,
                          nk_color col)
{
	nk_color col2;
	unsigned char inv_a;
	if (col.a == 0)
		return;

	inv_a = 0xff - col.a;
	col2 = ImgGetpixel(img, x0, y0);
	col.r = (col.r * col.a + col2.r * inv_a) >> 8;
	col.g = (col.g * col.a + col2.g * inv_a) >> 8;
	col.b = (col.b * col.a + col2.b * inv_a) >> 8;
	ImgSetpixel(img, x0, y0, col);
}

static void Scissor(GUIContext* rawfb, const float x, const float y, const float w,
                    const float h)
{
	rawfb->scissors.x = MIN(MAX(x, 0.f), (float)rawfb->surface->getWidth());
	rawfb->scissors.y = MIN(MAX(y, 0.f), (float)rawfb->surface->getHeight());
	rawfb->scissors.w = MIN(MAX(w + x, 0.f), (float)rawfb->surface->getWidth());
	rawfb->scissors.h = MIN(MAX(h + y, 0.f), (float)rawfb->surface->getHeight());
}

static void StrokeLine(const GUIContext* rawfb, short x0, short y0, short x1, short y1,
                       const unsigned int line_thickness, const nk_color col)
{
	short tmp;
	int dy, dx, stepx, stepy;

	dy = y1 - y0;
	dx = x1 - x0;

	/* fast path */
	if (dy == 0)
	{
		if (dx == 0 || y0 >= rawfb->scissors.h || y0 < rawfb->scissors.y)
			return;

		if (dx < 0)
		{
			/* swap x0 and x1 */
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		x1 = MIN((short)rawfb->scissors.w, x1);
		x0 = MIN((short)rawfb->scissors.w, x0);
		x1 = MAX((short)rawfb->scissors.x, x1);
		x0 = MAX((short)rawfb->scissors.x, x0);
		LineHorizontal(rawfb, x0, y0, x1, col);
		return;
	}
	if (dy < 0)
	{
		dy = -dy;
		stepy = -1;
	}
	else
		stepy = 1;

	if (dx < 0)
	{
		dx = -dx;
		stepx = -1;
	}
	else
		stepx = 1;

	dy <<= 1;
	dx <<= 1;

	CTXSetpixel(rawfb, x0, y0, col);
	if (dx > dy)
	{
		int fraction = dy - (dx >> 1);
		while (x0 != x1)
		{
			if (fraction >= 0)
			{
				y0 += stepy;
				fraction -= dx;
			}
			x0 += stepx;
			fraction += dy;
			CTXSetpixel(rawfb, x0, y0, col);
		}
	}
	else
	{
		int fraction = dx - (dy >> 1);
		while (y0 != y1)
		{
			if (fraction >= 0)
			{
				x0 += stepx;
				fraction -= dy;
			}
			y0 += stepy;
			fraction += dx;
			CTXSetpixel(rawfb, x0, y0, col);
		}
	}
}

static void FillPolygon(const GUIContext* rawfb, const struct nk_vec2i* pnts, int count,
                        const nk_color col)
{
	int i = 0;
#define MAX_POINTS 64
	int left = 10000, top = 10000, bottom = 0, right = 0;
	int nodes, nodeX[MAX_POINTS], pixelX, pixelY, j, swap;

	if (count == 0)
		return;
	if (count > MAX_POINTS)
		count = MAX_POINTS;

	/* Get polygon dimensions */
	for (i = 0; i < count; i++)
	{
		if (left > pnts[i].x)
			left = pnts[i].x;
		if (right < pnts[i].x)
			right = pnts[i].x;
		if (top > pnts[i].y)
			top = pnts[i].y;
		if (bottom < pnts[i].y)
			bottom = pnts[i].y;
	}
	bottom++;
	right++;

	/* Polygon scanline algorithm released under public-domain by Darel Rex Finley, 2007
	 */
	/*  Loop through the rows of the image. */
	for (pixelY = top; pixelY < bottom; pixelY++)
	{
		nodes = 0; /*  Build a list of nodes. */
		j = count - 1;
		for (i = 0; i < count; i++)
		{
			if (((pnts[i].y < pixelY) && (pnts[j].y >= pixelY)) ||
			    ((pnts[j].y < pixelY) && (pnts[i].y >= pixelY)))
			{
				nodeX[nodes++] =
				    (int)((float)pnts[i].x + ((float)pixelY - (float)pnts[i].y) /
				                                 ((float)pnts[j].y - (float)pnts[i].y) *
				                                 ((float)pnts[j].x - (float)pnts[i].x));
			}
			j = i;
		}

		/*  Sort the nodes, via a simple “Bubble” sort. */
		i = 0;
		while (i < nodes - 1)
		{
			if (nodeX[i] > nodeX[i + 1])
			{
				swap = nodeX[i];
				nodeX[i] = nodeX[i + 1];
				nodeX[i + 1] = swap;
				if (i)
					i--;
			}
			else
				i++;
		}
		/*  Fill the pixels between node pairs. */
		for (i = 0; i < nodes; i += 2)
		{
			if (nodeX[i + 0] >= right)
				break;
			if (nodeX[i + 1] > left)
			{
				if (nodeX[i + 0] < left)
					nodeX[i + 0] = left;
				if (nodeX[i + 1] > right)
					nodeX[i + 1] = right;
				for (pixelX = nodeX[i]; pixelX < nodeX[i + 1]; pixelX++)
					CTXSetpixel(rawfb, pixelX, pixelY, col);
			}
		}
	}
#undef MAX_POINTS
}

static void StrokeArc(const GUIContext* rawfb, short x0, short y0, short w, short h,
                      const short s, const short line_thickness, const nk_color col)
{
	/* Bresenham's ellipses - modified to draw one quarter */
	const int a2 = (w * w) / 4;
	const int b2 = (h * h) / 4;
	const int fa2 = 4 * a2, fb2 = 4 * b2;
	int x, y, sigma;

	if (s != 0 && s != 90 && s != 180 && s != 270)
		return;
	if (w < 1 || h < 1)
		return;

	/* Convert upper left to center */
	h = (h + 1) / 2;
	w = (w + 1) / 2;
	x0 += w;
	y0 += h;

	/* First half */
	for (x = 0, y = h, sigma = 2 * b2 + a2 * (1 - 2 * h); b2 * x <= a2 * y; x++)
	{
		if (s == 180)
			CTXSetpixel(rawfb, x0 + x, y0 + y, col);
		else if (s == 270)
			CTXSetpixel(rawfb, x0 - x, y0 + y, col);
		else if (s == 0)
			CTXSetpixel(rawfb, x0 + x, y0 - y, col);
		else if (s == 90)
			CTXSetpixel(rawfb, x0 - x, y0 - y, col);
		if (sigma >= 0)
		{
			sigma += fa2 * (1 - y);
			y--;
		}
		sigma += b2 * ((4 * x) + 6);
	}

	/* Second half */
	for (x = w, y = 0, sigma = 2 * a2 + b2 * (1 - 2 * w); a2 * y <= b2 * x; y++)
	{
		if (s == 180)
			CTXSetpixel(rawfb, x0 + x, y0 + y, col);
		else if (s == 270)
			CTXSetpixel(rawfb, x0 - x, y0 + y, col);
		else if (s == 0)
			CTXSetpixel(rawfb, x0 + x, y0 - y, col);
		else if (s == 90)
			CTXSetpixel(rawfb, x0 - x, y0 - y, col);
		if (sigma >= 0)
		{
			sigma += fb2 * (1 - x);
			x--;
		}
		sigma += a2 * ((4 * y) + 6);
	}
}

static void FillArc(const GUIContext* rawfb, short x0, short y0, short w, short h,
                    const short s, const nk_color col)
{
	/* Bresenham's ellipses - modified to fill one quarter */
	const int a2 = (w * w) / 4;
	const int b2 = (h * h) / 4;
	const int fa2 = 4 * a2, fb2 = 4 * b2;
	int x, y, sigma;
	struct nk_vec2i pnts[3];
	if (w < 1 || h < 1)
		return;
	if (s != 0 && s != 90 && s != 180 && s != 270)
		return;

	/* Convert upper left to center */
	h = (h + 1) / 2;
	w = (w + 1) / 2;
	x0 += w;
	y0 += h;

	pnts[0].x = x0;
	pnts[0].y = y0;
	pnts[2].x = x0;
	pnts[2].y = y0;

	/* First half */
	for (x = 0, y = h, sigma = 2 * b2 + a2 * (1 - 2 * h); b2 * x <= a2 * y; x++)
	{
		if (s == 180)
		{
			pnts[1].x = x0 + x;
			pnts[1].y = y0 + y;
		}
		else if (s == 270)
		{
			pnts[1].x = x0 - x;
			pnts[1].y = y0 + y;
		}
		else if (s == 0)
		{
			pnts[1].x = x0 + x;
			pnts[1].y = y0 - y;
		}
		else if (s == 90)
		{
			pnts[1].x = x0 - x;
			pnts[1].y = y0 - y;
		}
		FillPolygon(rawfb, pnts, 3, col);
		pnts[2] = pnts[1];
		if (sigma >= 0)
		{
			sigma += fa2 * (1 - y);
			y--;
		}
		sigma += b2 * ((4 * x) + 6);
	}

	/* Second half */
	for (x = w, y = 0, sigma = 2 * a2 + b2 * (1 - 2 * w); a2 * y <= b2 * x; y++)
	{
		if (s == 180)
		{
			pnts[1].x = x0 + x;
			pnts[1].y = y0 + y;
		}
		else if (s == 270)
		{
			pnts[1].x = x0 - x;
			pnts[1].y = y0 + y;
		}
		else if (s == 0)
		{
			pnts[1].x = x0 + x;
			pnts[1].y = y0 - y;
		}
		else if (s == 90)
		{
			pnts[1].x = x0 - x;
			pnts[1].y = y0 - y;
		}
		FillPolygon(rawfb, pnts, 3, col);
		pnts[2] = pnts[1];
		if (sigma >= 0)
		{
			sigma += fb2 * (1 - x);
			x--;
		}
		sigma += a2 * ((4 * y) + 6);
	}
}

static void StrokeRect(const GUIContext* rawfb, const short x, const short y,
                       const short w, const short h, const short r,
                       const short line_thickness, const nk_color col)
{
	if (r == 0)
	{
		StrokeLine(rawfb, x, y, x + w, y, line_thickness, col);
		StrokeLine(rawfb, x, y + h, x + w, y + h, line_thickness, col);
		StrokeLine(rawfb, x, y, x, y + h, line_thickness, col);
		StrokeLine(rawfb, x + w, y, x + w, y + h, line_thickness, col);
	}
	else
	{
		const short xc = x + r;
		const short yc = y + r;
		const short wc = (short)(w - 2 * r);
		const short hc = (short)(h - 2 * r);

		StrokeLine(rawfb, xc, y, xc + wc, y, line_thickness, col);
		StrokeLine(rawfb, x + w, yc, x + w, yc + hc, line_thickness, col);
		StrokeLine(rawfb, xc, y + h, xc + wc, y + h, line_thickness, col);
		StrokeLine(rawfb, x, yc, x, yc + hc, line_thickness, col);

		StrokeArc(rawfb, xc + wc - r, y, (unsigned)r * 2, (unsigned)r * 2, 0,
		          line_thickness, col);
		StrokeArc(rawfb, x, y, (unsigned)r * 2, (unsigned)r * 2, 90, line_thickness, col);
		StrokeArc(rawfb, x, yc + hc - r, (unsigned)r * 2, (unsigned)r * 2, 270,
		          line_thickness, col);
		StrokeArc(rawfb, xc + wc - r, yc + hc - r, (unsigned)r * 2, (unsigned)r * 2, 180,
		          line_thickness, col);
	}
}

static void FillRect(const GUIContext* rawfb, const short x, const short y, const short w,
                     const short h, const short r, const nk_color col)
{
	int i;
	if (r == 0)
	{
		for (i = 0; i < h; i++)
			StrokeLine(rawfb, x, y + i, x + w, y + i, 1, col);
	}
	else
	{
		const short xc = x + r;
		const short yc = y + r;
		const short wc = (short)(w - 2 * r);
		const short hc = (short)(h - 2 * r);

		struct nk_vec2i pnts[12];
		pnts[0].x = x;
		pnts[0].y = yc;
		pnts[1].x = xc;
		pnts[1].y = yc;
		pnts[2].x = xc;
		pnts[2].y = y;

		pnts[3].x = xc + wc;
		pnts[3].y = y;
		pnts[4].x = xc + wc;
		pnts[4].y = yc;
		pnts[5].x = x + w;
		pnts[5].y = yc;

		pnts[6].x = x + w;
		pnts[6].y = yc + hc;
		pnts[7].x = xc + wc;
		pnts[7].y = yc + hc;
		pnts[8].x = xc + wc;
		pnts[8].y = y + h;

		pnts[9].x = xc;
		pnts[9].y = y + h;
		pnts[10].x = xc;
		pnts[10].y = yc + hc;
		pnts[11].x = x;
		pnts[11].y = yc + hc;

		FillPolygon(rawfb, pnts, 12, col);

		FillArc(rawfb, xc + wc - r, y, (unsigned)r * 2, (unsigned)r * 2, 0, col);
		FillArc(rawfb, x, y, (unsigned)r * 2, (unsigned)r * 2, 90, col);
		FillArc(rawfb, x, yc + hc - r, (unsigned)r * 2, (unsigned)r * 2, 270, col);
		FillArc(rawfb, xc + wc - r, yc + hc - r, (unsigned)r * 2, (unsigned)r * 2, 180,
		        col);
	}
}

void DrawRectMultiColor(const GUIContext* rawfb, const short x, const short y,
                        const short w, const short h, nk_color tl, nk_color tr,
                        nk_color br, nk_color bl)
{
	int i, j;
	nk_color* edge_buf;
	nk_color* edge_t;
	nk_color* edge_b;
	nk_color* edge_l;
	nk_color* edge_r;
	nk_color pixel;

	edge_buf = (nk_color*)malloc(((2 * w) + (2 * h)) * sizeof(nk_color));
	if (edge_buf == NULL)
		return;

	edge_t = edge_buf;
	edge_b = edge_buf + w;
	edge_l = edge_buf + (w * 2);
	edge_r = edge_buf + (w * 2) + h;

	/* Top and bottom edge gradients */
	for (i = 0; i < w; i++)
	{
		edge_t[i].r = (((((float)tr.r - tl.r) / (w - 1)) * i) + 0.5) + tl.r;
		edge_t[i].g = (((((float)tr.g - tl.g) / (w - 1)) * i) + 0.5) + tl.g;
		edge_t[i].b = (((((float)tr.b - tl.b) / (w - 1)) * i) + 0.5) + tl.b;
		edge_t[i].a = (((((float)tr.a - tl.a) / (w - 1)) * i) + 0.5) + tl.a;

		edge_b[i].r = (((((float)br.r - bl.r) / (w - 1)) * i) + 0.5) + bl.r;
		edge_b[i].g = (((((float)br.g - bl.g) / (w - 1)) * i) + 0.5) + bl.g;
		edge_b[i].b = (((((float)br.b - bl.b) / (w - 1)) * i) + 0.5) + bl.b;
		edge_b[i].a = (((((float)br.a - bl.a) / (w - 1)) * i) + 0.5) + bl.a;
	}

	/* Left and right edge gradients */
	for (i = 0; i < h; i++)
	{
		edge_l[i].r = (((((float)bl.r - tl.r) / (h - 1)) * i) + 0.5) + tl.r;
		edge_l[i].g = (((((float)bl.g - tl.g) / (h - 1)) * i) + 0.5) + tl.g;
		edge_l[i].b = (((((float)bl.b - tl.b) / (h - 1)) * i) + 0.5) + tl.b;
		edge_l[i].a = (((((float)bl.a - tl.a) / (h - 1)) * i) + 0.5) + tl.a;

		edge_r[i].r = (((((float)br.r - tr.r) / (h - 1)) * i) + 0.5) + tr.r;
		edge_r[i].g = (((((float)br.g - tr.g) / (h - 1)) * i) + 0.5) + tr.g;
		edge_r[i].b = (((((float)br.b - tr.b) / (h - 1)) * i) + 0.5) + tr.b;
		edge_r[i].a = (((((float)br.a - tr.a) / (h - 1)) * i) + 0.5) + tr.a;
	}

	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			if (i == 0)
			{
				ImgBlendpixel(rawfb->surface, x + j, y + i, edge_t[j]);
			}
			else if (i == h - 1)
			{
				ImgBlendpixel(rawfb->surface, x + j, y + i, edge_b[j]);
			}
			else
			{
				if (j == 0)
				{
					ImgBlendpixel(rawfb->surface, x + j, y + i, edge_l[i]);
				}
				else if (j == w - 1)
				{
					ImgBlendpixel(rawfb->surface, x + j, y + i, edge_r[i]);
				}
				else
				{
					pixel.r =
					    (((((float)edge_r[i].r - edge_l[i].r) / (w - 1)) * j) + 0.5) +
					    edge_l[i].r;
					pixel.g =
					    (((((float)edge_r[i].g - edge_l[i].g) / (w - 1)) * j) + 0.5) +
					    edge_l[i].g;
					pixel.b =
					    (((((float)edge_r[i].b - edge_l[i].b) / (w - 1)) * j) + 0.5) +
					    edge_l[i].b;
					pixel.a =
					    (((((float)edge_r[i].a - edge_l[i].a) / (w - 1)) * j) + 0.5) +
					    edge_l[i].a;
					ImgBlendpixel(rawfb->surface, x + j, y + i, pixel);
				}
			}
		}
	}

	free(edge_buf);
}

static void FillTriangle(const GUIContext* rawfb, const short x0, const short y0,
                         const short x1, const short y1, const short x2, const short y2,
                         const nk_color col)
{
	struct nk_vec2i pnts[3];
	pnts[0].x = x0;
	pnts[0].y = y0;
	pnts[1].x = x1;
	pnts[1].y = y1;
	pnts[2].x = x2;
	pnts[2].y = y2;
	FillPolygon(rawfb, pnts, 3, col);
}

static void StrokeTriangle(const GUIContext* rawfb, const short x0, const short y0,
                           const short x1, const short y1, const short x2, const short y2,
                           const unsigned short line_thickness, const nk_color col)
{
	StrokeLine(rawfb, x0, y0, x1, y1, line_thickness, col);
	StrokeLine(rawfb, x1, y1, x2, y2, line_thickness, col);
	StrokeLine(rawfb, x2, y2, x0, y0, line_thickness, col);
}

static void StrokePolygon(const GUIContext* rawfb, const struct nk_vec2i* pnts,
                          const int count, const unsigned short line_thickness,
                          const nk_color col)
{
	int i;
	for (i = 1; i < count; ++i)
		StrokeLine(rawfb, pnts[i - 1].x, pnts[i - 1].y, pnts[i].x, pnts[i].y,
		           line_thickness, col);
	StrokeLine(rawfb, pnts[count - 1].x, pnts[count - 1].y, pnts[0].x, pnts[0].y,
	           line_thickness, col);
}

static void StrokePolyline(const GUIContext* rawfb, const struct nk_vec2i* pnts,
                           const int count, const unsigned short line_thickness,
                           const nk_color col)
{
	int i;
	for (i = 0; i < count - 1; ++i)
		StrokeLine(rawfb, pnts[i].x, pnts[i].y, pnts[i + 1].x, pnts[i + 1].y,
		           line_thickness, col);
}

static void FillCircle(const GUIContext* rawfb, short x0, short y0, short w, short h,
                       const nk_color col)
{
	/* Bresenham's ellipses */
	const int a2 = (w * w) / 4;
	const int b2 = (h * h) / 4;
	const int fa2 = 4 * a2, fb2 = 4 * b2;
	int x, y, sigma;

	/* Convert upper left to center */
	h = (h + 1) / 2;
	w = (w + 1) / 2;
	x0 += w;
	y0 += h;

	/* First half */
	for (x = 0, y = h, sigma = 2 * b2 + a2 * (1 - 2 * h); b2 * x <= a2 * y; x++)
	{
		StrokeLine(rawfb, x0 - x, y0 + y, x0 + x, y0 + y, 1, col);
		StrokeLine(rawfb, x0 - x, y0 - y, x0 + x, y0 - y, 1, col);
		if (sigma >= 0)
		{
			sigma += fa2 * (1 - y);
			y--;
		}
		sigma += b2 * ((4 * x) + 6);
	}
	/* Second half */
	for (x = w, y = 0, sigma = 2 * a2 + b2 * (1 - 2 * w); a2 * y <= b2 * x; y++)
	{
		StrokeLine(rawfb, x0 - x, y0 + y, x0 + x, y0 + y, 1, col);
		StrokeLine(rawfb, x0 - x, y0 - y, x0 + x, y0 - y, 1, col);
		if (sigma >= 0)
		{
			sigma += fb2 * (1 - x);
			x--;
		}
		sigma += a2 * ((4 * y) + 6);
	}
}

static void StrokeCircle(const GUIContext* rawfb, short x0, short y0, short w, short h,
                         const short line_thickness, const nk_color col)
{
	/* Bresenham's ellipses */
	const int a2 = (w * w) / 4;
	const int b2 = (h * h) / 4;
	const int fa2 = 4 * a2, fb2 = 4 * b2;
	int x, y, sigma;

	/* Convert upper left to center */
	h = (h + 1) / 2;
	w = (w + 1) / 2;
	x0 += w;
	y0 += h;

	/* First half */
	for (x = 0, y = h, sigma = 2 * b2 + a2 * (1 - 2 * h); b2 * x <= a2 * y; x++)
	{
		CTXSetpixel(rawfb, x0 + x, y0 + y, col);
		CTXSetpixel(rawfb, x0 - x, y0 + y, col);
		CTXSetpixel(rawfb, x0 + x, y0 - y, col);
		CTXSetpixel(rawfb, x0 - x, y0 - y, col);
		if (sigma >= 0)
		{
			sigma += fa2 * (1 - y);
			y--;
		}
		sigma += b2 * ((4 * x) + 6);
	}
	/* Second half */
	for (x = w, y = 0, sigma = 2 * a2 + b2 * (1 - 2 * w); a2 * y <= b2 * x; y++)
	{
		CTXSetpixel(rawfb, x0 + x, y0 + y, col);
		CTXSetpixel(rawfb, x0 - x, y0 + y, col);
		CTXSetpixel(rawfb, x0 + x, y0 - y, col);
		CTXSetpixel(rawfb, x0 - x, y0 - y, col);
		if (sigma >= 0)
		{
			sigma += fb2 * (1 - x);
			x--;
		}
		sigma += a2 * ((4 * y) + 6);
	}
}

static void StrokeCurve(const GUIContext* rawfb, const struct nk_vec2i p1,
                        const struct nk_vec2i p2, const struct nk_vec2i p3,
                        const struct nk_vec2i p4, const unsigned int num_segments,
                        const unsigned short line_thickness, const nk_color col)
{
	unsigned int i_step, segments;
	float t_step;
	struct nk_vec2i last = p1;

	segments = MAX(num_segments, 1u);
	t_step = 1.0f / (float)segments;
	for (i_step = 1; i_step <= segments; ++i_step)
	{
		float t = t_step * (float)i_step;
		float u = 1.0f - t;
		float w1 = u * u * u;
		float w2 = 3 * u * u * t;
		float w3 = 3 * u * t * t;
		float w4 = t * t * t;
		float x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
		float y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
		StrokeLine(rawfb, last.x, last.y, (short)x, (short)y, line_thickness, col);
		last.x = (short)x;
		last.y = (short)y;
	}
}

static void Clear(const GUIContext* rawfb, const nk_color col)
{
	FillRect(rawfb, 0, 0, rawfb->surface->getWidth(), rawfb->surface->getHeight(), 0,
	         col);
}

//
// Initialize a GUIContext from scratch.
//
// This function allocates with `new` - it is up to the caller to free it.
//
GUIContext* GUIContext::init(IWindowSurface* surface)
{
	// First bake our font atlas.
	nk_font_atlas atlas;
	int fwidth, fheight;

	nk_font_atlas_init_default(&atlas);
	nk_font_atlas_begin(&atlas);
	const struct nk_color* tex = (struct nk_color*)nk_font_atlas_bake(
	    &atlas, &fwidth, &fheight, NK_FONT_ATLAS_RGBA32);
	if (!tex)
		return NULL;

	// Blit our atlas into a dedicated surface.
	IWindowSurface* fontSurface =
	    new IWindowSurface(fwidth, fheight, I_Get32bppPixelFormat());

	// [AM] A surface can sometimes have extra scratch space in the far
	//      right margin, so we need to blit this a row at a time.
	for (size_t y = 0; y < fheight; y++)
	{
		size_t srcoff = y * fwidth;
		argb_t* buffer = (argb_t*)fontSurface->getBuffer(0, y);
		for (size_t i = 0; i < fwidth; i++)
		{
			struct nk_color px = tex[srcoff + i];
			buffer[i].seta(px.a);
			buffer[i].setr(px.r);
			buffer[i].setg(px.g);
			buffer[i].setb(px.b);
		}
	}
	nk_font_atlas_end(&atlas, nk_handle_ptr(fontSurface), NULL);

	// Now that we have our font and an atlas, we can initialize Nuklear properly.
	GUIContext* rawfb = new GUIContext;
	memset(rawfb, 0, sizeof(GUIContext));
	rawfb->surface = surface;
	rawfb->fontSurface = fontSurface;
	rawfb->atlas = atlas;

	if (0 == nk_init_default(&rawfb->ctx, NULL))
	{
		delete fontSurface;
		delete rawfb;
		return NULL;
	}

	if (rawfb->atlas.default_font)
		nk_style_set_font(&rawfb->ctx, &rawfb->atlas.default_font->handle);

	nk_style_load_all_cursors(&rawfb->ctx, rawfb->atlas.cursors);
	Scissor(rawfb, 0, 0, rawfb->surface->getWidth(), rawfb->surface->getHeight());

	return rawfb;
}

//
// Updates an existing GUI with a new surface.
//
void GUIContext::update(IWindowSurface* surface)
{
	this->surface = surface;
}

static void StretchImage(const IWindowSurface* dst, const IWindowSurface* src,
                         const struct nk_rect* dst_rect, const struct nk_rect* src_rect,
                         const struct nk_rect* dst_scissors, const nk_color* fg)
{
	short i, j;
	nk_color col;
	float xinc = src_rect->w / dst_rect->w;
	float yinc = src_rect->h / dst_rect->h;
	float xoff = src_rect->x, yoff = src_rect->y;

	/* Simple nearest filtering rescaling */
	/* TODO: use bilinear filter */
	for (j = 0; j < (short)dst_rect->h; j++)
	{
		for (i = 0; i < (short)dst_rect->w; i++)
		{
			if (dst_scissors)
			{
				if (i + (int)(dst_rect->x + 0.5f) < dst_scissors->x ||
				    i + (int)(dst_rect->x + 0.5f) >= dst_scissors->w)
					continue;
				if (j + (int)(dst_rect->y + 0.5f) < dst_scissors->y ||
				    j + (int)(dst_rect->y + 0.5f) >= dst_scissors->h)
					continue;
			}
			col = ImgGetpixel(src, (int)xoff, (int)yoff);
			if (col.r || col.g || col.b)
			{
				col.r = fg->r;
				col.g = fg->g;
				col.b = fg->b;
			}
			ImgBlendpixel(dst, i + (int)(dst_rect->x + 0.5f),
			              j + (int)(dst_rect->y + 0.5f), col);
			xoff += xinc;
		}
		xoff = src_rect->x;
		yoff += yinc;
	}
}

static void FontQueryFontGlyph(nk_handle handle, const float height,
                               nk_user_font_glyph* glyph, const nk_rune codepoint,
                               const nk_rune next_codepoint)
{
	float scale;
	const nk_font_glyph* g;
	nk_font* font;
	assert(glyph);
	NK_UNUSED(next_codepoint);

	font = (nk_font*)handle.ptr;
	assert(font);
	assert(font->glyphs);
	if (!font || !glyph)
		return;

	scale = height / font->info.height;
	g = nk_font_find_glyph(font, codepoint);
	glyph->width = (g->x1 - g->x0) * scale;
	glyph->height = (g->y1 - g->y0) * scale;
	glyph->offset = nk_vec2(g->x0 * scale, g->y0 * scale);
	glyph->xadvance = (g->xadvance * scale);
	glyph->uv[0] = nk_vec2(g->u0, g->v0);
	glyph->uv[1] = nk_vec2(g->u1, g->v1);
}

static void DrawText(const GUIContext* rawfb, const nk_user_font* font,
                     const struct nk_rect rect, const char* text, const int len,
                     const float font_height, const nk_color fg)
{
	float x = 0;
	int text_len = 0;
	nk_rune unicode = 0;
	nk_rune next = 0;
	int glyph_len = 0;
	int next_glyph_len = 0;
	nk_user_font_glyph g;
	if (!len || !text)
		return;

	x = 0;
	glyph_len = nk_utf_decode(text, &unicode, len);
	if (!glyph_len)
		return;

	/* draw every glyph image */
	while (text_len < len && glyph_len)
	{
		struct nk_rect src_rect;
		struct nk_rect dst_rect;
		float char_width = 0;
		if (unicode == NK_UTF_INVALID)
			break;

		/* query currently drawn glyph information */
		next_glyph_len =
		    nk_utf_decode(text + text_len + glyph_len, &next, (int)len - text_len);
		FontQueryFontGlyph(font->userdata, font_height, &g, unicode,
		                   (next == NK_UTF_INVALID) ? '\0' : next);

		/* calculate and draw glyph drawing rectangle and image */
		char_width = g.xadvance;
		src_rect.x = g.uv[0].x * rawfb->fontSurface->getWidth();
		src_rect.y = g.uv[0].y * rawfb->fontSurface->getHeight();
		src_rect.w = g.uv[1].x * rawfb->fontSurface->getWidth() -
		             g.uv[0].x * rawfb->fontSurface->getWidth();
		src_rect.h = g.uv[1].y * rawfb->fontSurface->getHeight() -
		             g.uv[0].y * rawfb->fontSurface->getHeight();

		dst_rect.x = x + g.offset.x + rect.x;
		dst_rect.y = g.offset.y + rect.y;
		dst_rect.w = ceil(g.width);
		dst_rect.h = ceil(g.height);

		/* Use software rescaling to blit glyph from font_text to framebuffer */
		StretchImage(rawfb->surface, rawfb->fontSurface, &dst_rect, &src_rect,
		             &rawfb->scissors, &fg);

		/* offset next glyph */
		text_len += glyph_len;
		x += char_width;
		glyph_len = next_glyph_len;
		unicode = next;
	}
}

static void DrawImage(const GUIContext* rawfb, const int x, const int y, const int w,
                      const int h, const struct nk_image* img, const nk_color* col)
{
	struct nk_rect src_rect;
	struct nk_rect dst_rect;

	src_rect.x = img->region[0];
	src_rect.y = img->region[1];
	src_rect.w = img->region[2];
	src_rect.h = img->region[3];

	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;
	StretchImage(rawfb->surface, rawfb->fontSurface, &dst_rect, &src_rect,
	             &rawfb->scissors, col);
}

static void DestroyContext(GUIContext* rawfb)
{
	if (rawfb)
	{
		delete rawfb->fontSurface;
		nk_free(&rawfb->ctx);

		memset(rawfb, 0, sizeof(GUIContext));
		delete rawfb;
	}
}

static void Render(const GUIContext* rawfb, const nk_color clear,
                   const unsigned char enable_clear)
{
	const nk_command* cmd;
	if (enable_clear)
		Clear(rawfb, clear);

	nk_foreach(cmd, (nk_context*)&rawfb->ctx)
	{
		switch (cmd->type)
		{
		case NK_COMMAND_NOP:
			break;
		case NK_COMMAND_SCISSOR: {
			const nk_command_scissor* s = (const nk_command_scissor*)cmd;
			Scissor((GUIContext*)rawfb, s->x, s->y, s->w, s->h);
		}
		break;
		case NK_COMMAND_LINE: {
			const nk_command_line* l = (const nk_command_line*)cmd;
			StrokeLine(rawfb, l->begin.x, l->begin.y, l->end.x, l->end.y,
			           l->line_thickness, l->color);
		}
		break;
		case NK_COMMAND_RECT: {
			const nk_command_rect* r = (const nk_command_rect*)cmd;
			StrokeRect(rawfb, r->x, r->y, r->w, r->h, (unsigned short)r->rounding,
			           r->line_thickness, r->color);
		}
		break;
		case NK_COMMAND_RECT_FILLED: {
			const nk_command_rect_filled* r = (const nk_command_rect_filled*)cmd;
			FillRect(rawfb, r->x, r->y, r->w, r->h, (unsigned short)r->rounding,
			         r->color);
		}
		break;
		case NK_COMMAND_CIRCLE: {
			const nk_command_circle* c = (const nk_command_circle*)cmd;
			StrokeCircle(rawfb, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
		}
		break;
		case NK_COMMAND_CIRCLE_FILLED: {
			const nk_command_circle_filled* c = (const nk_command_circle_filled*)cmd;
			FillCircle(rawfb, c->x, c->y, c->w, c->h, c->color);
		}
		break;
		case NK_COMMAND_TRIANGLE: {
			const nk_command_triangle* t = (const nk_command_triangle*)cmd;
			StrokeTriangle(rawfb, t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y,
			               t->line_thickness, t->color);
		}
		break;
		case NK_COMMAND_TRIANGLE_FILLED: {
			const nk_command_triangle_filled* t = (const nk_command_triangle_filled*)cmd;
			FillTriangle(rawfb, t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y, t->color);
		}
		break;
		case NK_COMMAND_POLYGON: {
			const nk_command_polygon* p = (const nk_command_polygon*)cmd;
			StrokePolygon(rawfb, p->points, p->point_count, p->line_thickness, p->color);
		}
		break;
		case NK_COMMAND_POLYGON_FILLED: {
			const nk_command_polygon_filled* p = (const nk_command_polygon_filled*)cmd;
			FillPolygon(rawfb, p->points, p->point_count, p->color);
		}
		break;
		case NK_COMMAND_POLYLINE: {
			const nk_command_polyline* p = (const nk_command_polyline*)cmd;
			StrokePolyline(rawfb, p->points, p->point_count, p->line_thickness, p->color);
		}
		break;
		case NK_COMMAND_TEXT: {
			const nk_command_text* t = (const nk_command_text*)cmd;
			DrawText(rawfb, t->font, nk_rect(t->x, t->y, t->w, t->h), t->string,
			         t->length, t->height, t->foreground);
		}
		break;
		case NK_COMMAND_CURVE: {
			const nk_command_curve* q = (const nk_command_curve*)cmd;
			StrokeCurve(rawfb, q->begin, q->ctrl[0], q->ctrl[1], q->end, 22,
			            q->line_thickness, q->color);
		}
		break;
		case NK_COMMAND_RECT_MULTI_COLOR: {
			const nk_command_rect_multi_color* q =
			    (const nk_command_rect_multi_color*)cmd;
			DrawRectMultiColor(rawfb, q->x, q->y, q->w, q->h, q->left, q->top, q->right,
			                   q->bottom);
		}
		break;
		case NK_COMMAND_IMAGE: {
			const nk_command_image* q = (const nk_command_image*)cmd;
			DrawImage(rawfb, q->x, q->y, q->w, q->h, &q->img, &q->col);
		}
		break;
		case NK_COMMAND_ARC: {
			assert(0 && "NK_COMMAND_ARC not implemented\n");
		}
		break;
		case NK_COMMAND_ARC_FILLED: {
			assert(0 && "NK_COMMAND_ARC_FILLED not implemented\n");
		}
		break;
		default:
			break;
		}
	}
	nk_clear((nk_context*)&rawfb->ctx);
}

static GUIContext* ctx;

namespace gui
{

//
// Initialize or update the GUI context.
//
void Init(IWindowSurface* surface)
{
	if (::ctx)
		::ctx->update(surface);
	else
		::ctx = GUIContext::init(surface);
}

//
// Destroy the GUI context.
//
void Quit()
{
	if (!::ctx)
		return;

	delete ::ctx;
	::ctx = NULL;
}

//
// Get a pointer to the underlying Nuklear context for interface creation.
//
nk_context* GetContext()
{
	if (::ctx == NULL)
	{
		I_FatalError("Tried to get GUI context before GUI init.");
		return NULL;
	}

	return &::ctx->ctx;
}

//
// Draw any queued draw commands.
//
void Draw()
{
	if (!::ctx)
		return;

	nk_color color = {0x00, 0x7F, 0x7F, 0x00};
	Render(::ctx, color, 1);
}

//
// Start parsing events.
//
// Call this before you start responding to events.
//
void BeginEvents()
{
	if (!::ctx)
		return;

	nk_input_begin(&::ctx->ctx);
}

//
// Finish parsing events.
//
// Call this before you start responding to events.
//
void EndEvents()
{
	if (!::ctx)
		return;

	nk_input_end(&::ctx->ctx);
}

//
// Handle events.
//
bool Responder(event_t* evt)
{
	if (!::ctx)
		return false;

	static int lastx, lasty;
	nk_context* ctx = &::ctx->ctx;

	if (evt->type == ev_keydown || evt->type == ev_keyup)
	{
		int down = evt->type == ev_keydown;

		if (evt->data1 >= ' ' && evt->data1 <= '~')
			nk_input_char(ctx, evt->data1);
		else if (evt->data1 == KEY_LSHIFT || evt->data1 == KEY_RSHIFT)
			nk_input_key(ctx, NK_KEY_SHIFT, down);
		else if (evt->data1 == KEY_LCTRL || evt->data1 == KEY_RCTRL)
			nk_input_key(ctx, NK_KEY_CTRL, down);
		else if (evt->data1 == KEY_DEL)
			nk_input_key(ctx, NK_KEY_DEL, down);
		else if (evt->data1 == KEY_ENTER || evt->data1 == KEYP_ENTER)
			nk_input_key(ctx, NK_KEY_ENTER, down);
		else if (evt->data1 == KEY_TAB)
			nk_input_key(ctx, NK_KEY_TAB, down);
		else if (evt->data1 == KEY_BACKSPACE)
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
		else if (evt->data1 == KEY_UPARROW)
			nk_input_key(ctx, NK_KEY_UP, down);
		else if (evt->data1 == KEY_DOWNARROW)
			nk_input_key(ctx, NK_KEY_DOWN, down);
		else if (evt->data1 == KEY_LEFTARROW)
			nk_input_key(ctx, NK_KEY_LEFT, down);
		else if (evt->data1 == KEY_RIGHTARROW)
			nk_input_key(ctx, NK_KEY_RIGHT, down);
		else if (evt->data1 == KEY_MOUSE1)
			nk_input_button(ctx, NK_BUTTON_LEFT, lastx, lasty, down);
		else if (evt->data1 == KEY_MOUSE2)
			nk_input_button(ctx, NK_BUTTON_RIGHT, lastx, lasty, down);
		else if (evt->data1 == KEY_MOUSE3)
			nk_input_button(ctx, NK_BUTTON_MIDDLE, lastx, lasty, down);
		else
			return false;

		return true;
	}
	else if (evt->type == ev_amouse)
	{
		nk_input_motion(ctx, evt->data2, evt->data3);
		lastx = evt->data2;
		lasty = evt->data3;
		return true;
	}

	return false;
}

#include <limits.h>

void Demo()
{
	nk_context* ctx = &::ctx->ctx;

	/* window flags */
	static int show_menu = nk_true;
	static int titlebar = nk_true;
	static int border = nk_true;
	static int resize = nk_true;
	static int movable = nk_true;
	static int no_scrollbar = nk_false;
	static int scale_left = nk_false;
	static nk_flags window_flags = 0;
	static int minimizable = nk_true;

	/* popups */
	static enum nk_style_header_align header_align = NK_HEADER_RIGHT;
	static int show_app_about = nk_false;

	/* window flags */
	window_flags = 0;
	ctx->style.window.header.align = header_align;
	if (border)
		window_flags |= NK_WINDOW_BORDER;
	if (resize)
		window_flags |= NK_WINDOW_SCALABLE;
	if (movable)
		window_flags |= NK_WINDOW_MOVABLE;
	if (no_scrollbar)
		window_flags |= NK_WINDOW_NO_SCROLLBAR;
	if (scale_left)
		window_flags |= NK_WINDOW_SCALE_LEFT;
	if (minimizable)
		window_flags |= NK_WINDOW_MINIMIZABLE;

	if (nk_begin(ctx, "Overview", nk_rect(10, 10, 400, 600), window_flags))
	{
		if (show_menu)
		{
			/* menubar */
			enum menu_states
			{
				MENU_DEFAULT,
				MENU_WINDOWS
			};
			static nk_size mprog = 60;
			static int mslider = 10;
			static int mcheck = nk_true;
			nk_menubar_begin(ctx);

			/* menu #1 */
			nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
			nk_layout_row_push(ctx, 45);
			if (nk_menu_begin_label(ctx, "MENU", NK_TEXT_LEFT, nk_vec2(120, 200)))
			{
				static size_t prog = 40;
				static int slider = 10;
				static int check = nk_true;
				nk_layout_row_dynamic(ctx, 25, 1);
				if (nk_menu_item_label(ctx, "Hide", NK_TEXT_LEFT))
					show_menu = nk_false;
				if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT))
					show_app_about = nk_true;
				nk_progress(ctx, &prog, 100, NK_MODIFIABLE);
				nk_slider_int(ctx, 0, &slider, 16, 1);
				nk_checkbox_label(ctx, "check", &check);
				nk_menu_end(ctx);
			}
			/* menu #2 */
			nk_layout_row_push(ctx, 60);
			if (nk_menu_begin_label(ctx, "ADVANCED", NK_TEXT_LEFT, nk_vec2(200, 600)))
			{
				enum menu_state
				{
					MENU_NONE,
					MENU_FILE,
					MENU_EDIT,
					MENU_VIEW,
					MENU_CHART
				};
				static enum menu_state menu_state = MENU_NONE;
				enum nk_collapse_states state;

				state = (menu_state == MENU_FILE) ? NK_MAXIMIZED : NK_MINIMIZED;
				if (nk_tree_state_push(ctx, NK_TREE_TAB, "FILE", &state))
				{
					menu_state = MENU_FILE;
					nk_menu_item_label(ctx, "New", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Open", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Close", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT);
					nk_tree_pop(ctx);
				}
				else
					menu_state = (menu_state == MENU_FILE) ? MENU_NONE : menu_state;

				state = (menu_state == MENU_EDIT) ? NK_MAXIMIZED : NK_MINIMIZED;
				if (nk_tree_state_push(ctx, NK_TREE_TAB, "EDIT", &state))
				{
					menu_state = MENU_EDIT;
					nk_menu_item_label(ctx, "Copy", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Delete", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Cut", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Paste", NK_TEXT_LEFT);
					nk_tree_pop(ctx);
				}
				else
					menu_state = (menu_state == MENU_EDIT) ? MENU_NONE : menu_state;

				state = (menu_state == MENU_VIEW) ? NK_MAXIMIZED : NK_MINIMIZED;
				if (nk_tree_state_push(ctx, NK_TREE_TAB, "VIEW", &state))
				{
					menu_state = MENU_VIEW;
					nk_menu_item_label(ctx, "About", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Options", NK_TEXT_LEFT);
					nk_menu_item_label(ctx, "Customize", NK_TEXT_LEFT);
					nk_tree_pop(ctx);
				}
				else
					menu_state = (menu_state == MENU_VIEW) ? MENU_NONE : menu_state;

				state = (menu_state == MENU_CHART) ? NK_MAXIMIZED : NK_MINIMIZED;
				if (nk_tree_state_push(ctx, NK_TREE_TAB, "CHART", &state))
				{
					size_t i = 0;
					const float values[] = {26.0f, 13.0f, 30.0f, 15.0f, 25.0f, 10.0f,
					                        20.0f, 40.0f, 12.0f, 8.0f,  22.0f, 28.0f};
					menu_state = MENU_CHART;
					nk_layout_row_dynamic(ctx, 150, 1);
					nk_chart_begin(ctx, NK_CHART_COLUMN, NK_LEN(values), 0, 50);
					for (i = 0; i < NK_LEN(values); ++i)
						nk_chart_push(ctx, values[i]);
					nk_chart_end(ctx);
					nk_tree_pop(ctx);
				}
				else
					menu_state = (menu_state == MENU_CHART) ? MENU_NONE : menu_state;
				nk_menu_end(ctx);
			}
			/* menu widgets */
			nk_layout_row_push(ctx, 70);
			nk_progress(ctx, &mprog, 100, NK_MODIFIABLE);
			nk_slider_int(ctx, 0, &mslider, 16, 1);
			nk_checkbox_label(ctx, "check", &mcheck);
			nk_menubar_end(ctx);
		}

		if (show_app_about)
		{
			/* about popup */
			static struct nk_rect s = {20, 100, 300, 190};
			if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About", NK_WINDOW_CLOSABLE, s))
			{
				nk_layout_row_dynamic(ctx, 20, 1);
				nk_label(ctx, "Nuklear", NK_TEXT_LEFT);
				nk_label(ctx, "By Micha Mettke", NK_TEXT_LEFT);
				nk_label(ctx, "nuklear is licensed under the public domain License.",
				         NK_TEXT_LEFT);
				nk_popup_end(ctx);
			}
			else
				show_app_about = nk_false;
		}

		/* window flags */
		if (nk_tree_push(ctx, NK_TREE_TAB, "Window", NK_MINIMIZED))
		{
			nk_layout_row_dynamic(ctx, 30, 2);
			nk_checkbox_label(ctx, "Titlebar", &titlebar);
			nk_checkbox_label(ctx, "Menu", &show_menu);
			nk_checkbox_label(ctx, "Border", &border);
			nk_checkbox_label(ctx, "Resizable", &resize);
			nk_checkbox_label(ctx, "Movable", &movable);
			nk_checkbox_label(ctx, "No Scrollbar", &no_scrollbar);
			nk_checkbox_label(ctx, "Minimizable", &minimizable);
			nk_checkbox_label(ctx, "Scale Left", &scale_left);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Widgets", NK_MINIMIZED))
		{
			enum options
			{
				A,
				B,
				C
			};
			static int checkbox;
			static int option;
			if (nk_tree_push(ctx, NK_TREE_NODE, "Text", NK_MINIMIZED))
			{
				/* Text Widgets */
				nk_layout_row_dynamic(ctx, 20, 1);
				nk_label(ctx, "Label aligned left", NK_TEXT_LEFT);
				nk_label(ctx, "Label aligned centered", NK_TEXT_CENTERED);
				nk_label(ctx, "Label aligned right", NK_TEXT_RIGHT);
				nk_label_colored(ctx, "Blue text", NK_TEXT_LEFT, nk_rgb(0, 0, 255));
				nk_label_colored(ctx, "Yellow text", NK_TEXT_LEFT, nk_rgb(255, 255, 0));
				nk_text(ctx, "Text without /0", 15, NK_TEXT_RIGHT);

				nk_layout_row_static(ctx, 100, 200, 1);
				nk_label_wrap(ctx,
				              "This is a very long line to hopefully get this text to be "
				              "wrapped into multiple lines to show line wrapping");
				nk_layout_row_dynamic(ctx, 100, 1);
				nk_label_wrap(ctx, "This is another long text to show dynamic window "
				                   "changes on multiline text");
				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Button", NK_MINIMIZED))
			{
				/* Buttons Widgets */
				nk_layout_row_static(ctx, 30, 100, 3);
				if (nk_button_label(ctx, "Button"))
					fprintf(stdout, "Button pressed!\n");
				nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
				if (nk_button_label(ctx, "Repeater"))
					fprintf(stdout, "Repeater is being pressed!\n");
				nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);
				nk_button_color(ctx, nk_rgb(0, 0, 255));

				nk_layout_row_static(ctx, 25, 25, 8);
				nk_button_symbol(ctx, NK_SYMBOL_CIRCLE_SOLID);
				nk_button_symbol(ctx, NK_SYMBOL_CIRCLE_OUTLINE);
				nk_button_symbol(ctx, NK_SYMBOL_RECT_SOLID);
				nk_button_symbol(ctx, NK_SYMBOL_RECT_OUTLINE);
				nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_UP);
				nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_DOWN);
				nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_LEFT);
				nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT);

				nk_layout_row_static(ctx, 30, 100, 2);
				nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "prev",
				                       NK_TEXT_RIGHT);
				nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "next",
				                       NK_TEXT_LEFT);
				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Basic", NK_MINIMIZED))
			{
				/* Basic widgets */
				static int int_slider = 5;
				static float float_slider = 2.5f;
				static size_t prog_value = 40;
				static float property_float = 2;
				static int property_int = 10;
				static int property_neg = 10;

				static float range_float_min = 0;
				static float range_float_max = 100;
				static float range_float_value = 50;
				static int range_int_min = 0;
				static int range_int_value = 2048;
				static int range_int_max = 4096;
				static const float ratio[] = {120, 150};

				nk_layout_row_static(ctx, 30, 100, 1);
				nk_checkbox_label(ctx, "Checkbox", &checkbox);

				nk_layout_row_static(ctx, 30, 80, 3);
				option = nk_option_label(ctx, "optionA", option == A) ? A : option;
				option = nk_option_label(ctx, "optionB", option == B) ? B : option;
				option = nk_option_label(ctx, "optionC", option == C) ? C : option;

				nk_layout_row(ctx, NK_STATIC, 30, 2, ratio);
				nk_labelf(ctx, NK_TEXT_LEFT, "Slider int");
				nk_slider_int(ctx, 0, &int_slider, 10, 1);

				nk_label(ctx, "Slider float", NK_TEXT_LEFT);
				nk_slider_float(ctx, 0, &float_slider, 5.0, 0.5f);
				nk_labelf(ctx, NK_TEXT_LEFT, "Progressbar: %zu", prog_value);
				nk_progress(ctx, &prog_value, 100, NK_MODIFIABLE);

				nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
				nk_label(ctx, "Property float:", NK_TEXT_LEFT);
				nk_property_float(ctx, "Float:", 0, &property_float, 64.0f, 0.1f, 0.2f);
				nk_label(ctx, "Property int:", NK_TEXT_LEFT);
				nk_property_int(ctx, "Int:", 0, &property_int, 100, 1, 1);
				nk_label(ctx, "Property neg:", NK_TEXT_LEFT);
				nk_property_int(ctx, "Neg:", -10, &property_neg, 10, 1, 1);

				nk_layout_row_dynamic(ctx, 25, 1);
				nk_label(ctx, "Range:", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 25, 3);
				nk_property_float(ctx, "#min:", 0, &range_float_min, range_float_max,
				                  1.0f, 0.2f);
				nk_property_float(ctx, "#float:", range_float_min, &range_float_value,
				                  range_float_max, 1.0f, 0.2f);
				nk_property_float(ctx, "#max:", range_float_min, &range_float_max, 100,
				                  1.0f, 0.2f);

				nk_property_int(ctx, "#min:", INT_MIN, &range_int_min, range_int_max, 1,
				                10);
				nk_property_int(ctx, "#neg:", range_int_min, &range_int_value,
				                range_int_max, 1, 10);
				nk_property_int(ctx, "#max:", range_int_min, &range_int_max, INT_MAX, 1,
				                10);

				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Inactive", NK_MINIMIZED))
			{
				static int inactive = 1;
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_checkbox_label(ctx, "Inactive", &inactive);

				nk_layout_row_static(ctx, 30, 80, 1);
				if (inactive)
				{
					struct nk_style_button button;
					button = ctx->style.button;
					ctx->style.button.normal = nk_style_item_color(nk_rgb(40, 40, 40));
					ctx->style.button.hover = nk_style_item_color(nk_rgb(40, 40, 40));
					ctx->style.button.active = nk_style_item_color(nk_rgb(40, 40, 40));
					ctx->style.button.border_color = nk_rgb(60, 60, 60);
					ctx->style.button.text_background = nk_rgb(60, 60, 60);
					ctx->style.button.text_normal = nk_rgb(60, 60, 60);
					ctx->style.button.text_hover = nk_rgb(60, 60, 60);
					ctx->style.button.text_active = nk_rgb(60, 60, 60);
					nk_button_label(ctx, "button");
					ctx->style.button = button;
				}
				else if (nk_button_label(ctx, "button"))
					fprintf(stdout, "button pressed\n");
				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Selectable", NK_MINIMIZED))
			{
				if (nk_tree_push(ctx, NK_TREE_NODE, "List", NK_MINIMIZED))
				{
					static int selected[4] = {nk_false, nk_false, nk_true, nk_false};
					nk_layout_row_static(ctx, 18, 100, 1);
					nk_selectable_label(ctx, "Selectable", NK_TEXT_LEFT, &selected[0]);
					nk_selectable_label(ctx, "Selectable", NK_TEXT_LEFT, &selected[1]);
					nk_label(ctx, "Not Selectable", NK_TEXT_LEFT);
					nk_selectable_label(ctx, "Selectable", NK_TEXT_LEFT, &selected[2]);
					nk_selectable_label(ctx, "Selectable", NK_TEXT_LEFT, &selected[3]);
					nk_tree_pop(ctx);
				}
				if (nk_tree_push(ctx, NK_TREE_NODE, "Grid", NK_MINIMIZED))
				{
					int i;
					static int selected[16] = {1, 0, 0, 0, 0, 1, 0, 0,
					                           0, 0, 1, 0, 0, 0, 0, 1};
					nk_layout_row_static(ctx, 50, 50, 4);
					for (i = 0; i < 16; ++i)
					{
						if (nk_selectable_label(ctx, "Z", NK_TEXT_CENTERED, &selected[i]))
						{
							int x = (i % 4), y = i / 4;
							if (x > 0)
								selected[i - 1] ^= 1;
							if (x < 3)
								selected[i + 1] ^= 1;
							if (y > 0)
								selected[i - 4] ^= 1;
							if (y < 3)
								selected[i + 4] ^= 1;
						}
					}
					nk_tree_pop(ctx);
				}
				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Combo", NK_MINIMIZED))
			{
				/* Combobox Widgets
				 * In this library comboboxes are not limited to being a popup
				 * list of selectable text. Instead it is a abstract concept of
				 * having something that is *selected* or displayed, a popup window
				 * which opens if something needs to be modified and the content
				 * of the popup which causes the *selected* or displayed value to
				 * change or if wanted close the combobox.
				 *
				 * While strange at first handling comboboxes in a abstract way
				 * solves the problem of overloaded window content. For example
				 * changing a color value requires 4 value modifier (slider, property,...)
				 * for RGBA then you need a label and ways to display the current color.
				 * If you want to go fancy you even add rgb and hsv ratio boxes.
				 * While fine for one color if you have a lot of them it because
				 * tedious to look at and quite wasteful in space. You could add
				 * a popup which modifies the color but this does not solve the
				 * fact that it still requires a lot of cluttered space to do.
				 *
				 * In these kind of instance abstract comboboxes are quite handy. All
				 * value modifiers are hidden inside the combobox popup and only
				 * the color is shown if not open. This combines the clarity of the
				 * popup with the ease of use of just using the space for modifiers.
				 *
				 * Other instances are for example time and especially date picker,
				 * which only show the currently activated time/data and hide the
				 * selection logic inside the combobox popup.
				 */
				static float chart_selection = 8.0f;
				static int current_weapon = 0;
				static int check_values[5];
				static float position[3];
				static struct nk_color combo_color = {130, 50, 50, 255};
				static struct nk_colorf combo_color2 = {0.509f, 0.705f, 0.2f, 1.0f};
				static size_t prog_a = 20, prog_b = 40, prog_c = 10, prog_d = 90;
				static const char* weapons[] = {"Fist", "Pistol", "Shotgun", "Plasma",
				                                "BFG"};

				char buffer[64];
				size_t sum = 0;

				/* default combobox */
				nk_layout_row_static(ctx, 25, 200, 1);
				current_weapon = nk_combo(ctx, weapons, NK_LEN(weapons), current_weapon,
				                          25, nk_vec2(200, 200));

				/* slider color combobox */
				if (nk_combo_begin_color(ctx, combo_color, nk_vec2(200, 200)))
				{
					float ratios[] = {0.15f, 0.85f};
					nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratios);
					nk_label(ctx, "R:", NK_TEXT_LEFT);
					combo_color.r = (nk_byte)nk_slide_int(ctx, 0, combo_color.r, 255, 5);
					nk_label(ctx, "G:", NK_TEXT_LEFT);
					combo_color.g = (nk_byte)nk_slide_int(ctx, 0, combo_color.g, 255, 5);
					nk_label(ctx, "B:", NK_TEXT_LEFT);
					combo_color.b = (nk_byte)nk_slide_int(ctx, 0, combo_color.b, 255, 5);
					nk_label(ctx, "A:", NK_TEXT_LEFT);
					combo_color.a = (nk_byte)nk_slide_int(ctx, 0, combo_color.a, 255, 5);
					nk_combo_end(ctx);
				}
				/* complex color combobox */
				if (nk_combo_begin_color(ctx, nk_rgb_cf(combo_color2), nk_vec2(200, 400)))
				{
					enum color_mode
					{
						COL_RGB,
						COL_HSV
					};
					static int col_mode = COL_RGB;
#ifndef DEMO_DO_NOT_USE_COLOR_PICKER
					nk_layout_row_dynamic(ctx, 120, 1);
					combo_color2 = nk_color_picker(ctx, combo_color2, NK_RGBA);
#endif

					nk_layout_row_dynamic(ctx, 25, 2);
					col_mode = nk_option_label(ctx, "RGB", col_mode == COL_RGB)
					               ? COL_RGB
					               : col_mode;
					col_mode = nk_option_label(ctx, "HSV", col_mode == COL_HSV)
					               ? COL_HSV
					               : col_mode;

					nk_layout_row_dynamic(ctx, 25, 1);
					if (col_mode == COL_RGB)
					{
						combo_color2.r = nk_propertyf(ctx, "#R:", 0, combo_color2.r, 1.0f,
						                              0.01f, 0.005f);
						combo_color2.g = nk_propertyf(ctx, "#G:", 0, combo_color2.g, 1.0f,
						                              0.01f, 0.005f);
						combo_color2.b = nk_propertyf(ctx, "#B:", 0, combo_color2.b, 1.0f,
						                              0.01f, 0.005f);
						combo_color2.a = nk_propertyf(ctx, "#A:", 0, combo_color2.a, 1.0f,
						                              0.01f, 0.005f);
					}
					else
					{
						float hsva[4];
						nk_colorf_hsva_fv(hsva, combo_color2);
						hsva[0] =
						    nk_propertyf(ctx, "#H:", 0, hsva[0], 1.0f, 0.01f, 0.05f);
						hsva[1] =
						    nk_propertyf(ctx, "#S:", 0, hsva[1], 1.0f, 0.01f, 0.05f);
						hsva[2] =
						    nk_propertyf(ctx, "#V:", 0, hsva[2], 1.0f, 0.01f, 0.05f);
						hsva[3] =
						    nk_propertyf(ctx, "#A:", 0, hsva[3], 1.0f, 0.01f, 0.05f);
						combo_color2 = nk_hsva_colorfv(hsva);
					}
					nk_combo_end(ctx);
				}
				/* progressbar combobox */
				sum = prog_a + prog_b + prog_c + prog_d;
				sprintf(buffer, "%lu", sum);
				if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 200)))
				{
					nk_layout_row_dynamic(ctx, 30, 1);
					nk_progress(ctx, &prog_a, 100, NK_MODIFIABLE);
					nk_progress(ctx, &prog_b, 100, NK_MODIFIABLE);
					nk_progress(ctx, &prog_c, 100, NK_MODIFIABLE);
					nk_progress(ctx, &prog_d, 100, NK_MODIFIABLE);
					nk_combo_end(ctx);
				}

				/* checkbox combobox */
				sum = (size_t)(check_values[0] + check_values[1] + check_values[2] +
				               check_values[3] + check_values[4]);
				sprintf(buffer, "%lu", sum);
				if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 200)))
				{
					nk_layout_row_dynamic(ctx, 30, 1);
					nk_checkbox_label(ctx, weapons[0], &check_values[0]);
					nk_checkbox_label(ctx, weapons[1], &check_values[1]);
					nk_checkbox_label(ctx, weapons[2], &check_values[2]);
					nk_checkbox_label(ctx, weapons[3], &check_values[3]);
					nk_combo_end(ctx);
				}

				/* complex text combobox */
				sprintf(buffer, "%.2f, %.2f, %.2f", position[0], position[1],
				        position[2]);
				if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 200)))
				{
					nk_layout_row_dynamic(ctx, 25, 1);
					nk_property_float(ctx, "#X:", -1024.0f, &position[0], 1024.0f, 1,
					                  0.5f);
					nk_property_float(ctx, "#Y:", -1024.0f, &position[1], 1024.0f, 1,
					                  0.5f);
					nk_property_float(ctx, "#Z:", -1024.0f, &position[2], 1024.0f, 1,
					                  0.5f);
					nk_combo_end(ctx);
				}

				/* chart combobox */
				sprintf(buffer, "%.1f", chart_selection);
				if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 250)))
				{
					size_t i = 0;
					static const float values[] = {26.0f, 13.0f, 30.0f, 15.0f, 25.0f,
					                               10.0f, 20.0f, 40.0f, 12.0f, 8.0f,
					                               22.0f, 28.0f, 5.0f};
					nk_layout_row_dynamic(ctx, 150, 1);
					nk_chart_begin(ctx, NK_CHART_COLUMN, NK_LEN(values), 0, 50);
					for (i = 0; i < NK_LEN(values); ++i)
					{
						nk_flags res = nk_chart_push(ctx, values[i]);
						if (res & NK_CHART_CLICKED)
						{
							chart_selection = values[i];
							nk_combo_close(ctx);
						}
					}
					nk_chart_end(ctx);
					nk_combo_end(ctx);
				}

				{
					static int time_selected = 0;
					static int date_selected = 0;
					static struct tm sel_time;
					static struct tm sel_date;
					if (!time_selected || !date_selected)
					{
						/* keep time and date updated if nothing is selected */
						time_t cur_time = time(0);
						struct tm* n = localtime(&cur_time);
						if (!time_selected)
							memcpy(&sel_time, n, sizeof(struct tm));
						if (!date_selected)
							memcpy(&sel_date, n, sizeof(struct tm));
					}

					/* time combobox */
					sprintf(buffer, "%02d:%02d:%02d", sel_time.tm_hour, sel_time.tm_min,
					        sel_time.tm_sec);
					if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 250)))
					{
						time_selected = 1;
						nk_layout_row_dynamic(ctx, 25, 1);
						sel_time.tm_sec =
						    nk_propertyi(ctx, "#S:", 0, sel_time.tm_sec, 60, 1, 1);
						sel_time.tm_min =
						    nk_propertyi(ctx, "#M:", 0, sel_time.tm_min, 60, 1, 1);
						sel_time.tm_hour =
						    nk_propertyi(ctx, "#H:", 0, sel_time.tm_hour, 23, 1, 1);
						nk_combo_end(ctx);
					}

					/* date combobox */
					sprintf(buffer, "%02d-%02d-%02d", sel_date.tm_mday,
					        sel_date.tm_mon + 1, sel_date.tm_year + 1900);
					if (nk_combo_begin_label(ctx, buffer, nk_vec2(350, 400)))
					{
						int i = 0;
						const char* month[] = {"January", "February", "March",
						                       "April",   "May",      "June",
						                       "July",    "August",   "September",
						                       "October", "November", "December"};
						const char* week_days[] = {"SUN", "MON", "TUE", "WED",
						                           "THU", "FRI", "SAT"};
						const int month_days[] = {31, 28, 31, 30, 31, 30,
						                          31, 31, 30, 31, 30, 31};
						int year = sel_date.tm_year + 1900;
						int leap_year = (!(year % 4) && ((year % 100))) || !(year % 400);
						int days = (sel_date.tm_mon == 1)
						               ? month_days[sel_date.tm_mon] + leap_year
						               : month_days[sel_date.tm_mon];

						/* header with month and year */
						date_selected = 1;
						nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 3);
						nk_layout_row_push(ctx, 0.05f);
						if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_LEFT))
						{
							if (sel_date.tm_mon == 0)
							{
								sel_date.tm_mon = 11;
								sel_date.tm_year = NK_MAX(0, sel_date.tm_year - 1);
							}
							else
								sel_date.tm_mon--;
						}
						nk_layout_row_push(ctx, 0.9f);
						sprintf(buffer, "%s %d", month[sel_date.tm_mon], year);
						nk_label(ctx, buffer, NK_TEXT_CENTERED);
						nk_layout_row_push(ctx, 0.05f);
						if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT))
						{
							if (sel_date.tm_mon == 11)
							{
								sel_date.tm_mon = 0;
								sel_date.tm_year++;
							}
							else
								sel_date.tm_mon++;
						}
						nk_layout_row_end(ctx);

						/* good old week day formula (double because precision) */
						{
							int year_n = (sel_date.tm_mon < 2) ? year - 1 : year;
							int y = year_n % 100;
							int c = year_n / 100;
							int y4 = (int)((float)y / 4);
							int c4 = (int)((float)c / 4);
							int m =
							    (int)(2.6 * (double)(((sel_date.tm_mon + 10) % 12) + 1) -
							          0.2);
							int week_day = (((1 + m + y + y4 + c4 - 2 * c) % 7) + 7) % 7;

							/* weekdays  */
							nk_layout_row_dynamic(ctx, 35, 7);
							for (i = 0; i < (int)NK_LEN(week_days); ++i)
								nk_label(ctx, week_days[i], NK_TEXT_CENTERED);

							/* days  */
							if (week_day > 0)
								nk_spacing(ctx, week_day);
							for (i = 1; i <= days; ++i)
							{
								sprintf(buffer, "%d", i);
								if (nk_button_label(ctx, buffer))
								{
									sel_date.tm_mday = i;
									nk_combo_close(ctx);
								}
							}
						}
						nk_combo_end(ctx);
					}
				}

				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Input", NK_MINIMIZED))
			{
				static const float ratio[] = {120, 150};
				static char field_buffer[64];
				static char text[9][64];
				static int text_len[9];
				static char box_buffer[512];
				static int field_len;
				static int box_len;
				nk_flags active;

				nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
				nk_label(ctx, "Default:", NK_TEXT_LEFT);

				nk_edit_string(ctx, NK_EDIT_SIMPLE, text[0], &text_len[0], 64,
				               nk_filter_default);
				nk_label(ctx, "Int:", NK_TEXT_LEFT);
				nk_edit_string(ctx, NK_EDIT_SIMPLE, text[1], &text_len[1], 64,
				               nk_filter_decimal);
				nk_label(ctx, "Float:", NK_TEXT_LEFT);
				nk_edit_string(ctx, NK_EDIT_SIMPLE, text[2], &text_len[2], 64,
				               nk_filter_float);
				nk_label(ctx, "Hex:", NK_TEXT_LEFT);
				nk_edit_string(ctx, NK_EDIT_SIMPLE, text[4], &text_len[4], 64,
				               nk_filter_hex);
				nk_label(ctx, "Octal:", NK_TEXT_LEFT);
				nk_edit_string(ctx, NK_EDIT_SIMPLE, text[5], &text_len[5], 64,
				               nk_filter_oct);
				nk_label(ctx, "Binary:", NK_TEXT_LEFT);
				nk_edit_string(ctx, NK_EDIT_SIMPLE, text[6], &text_len[6], 64,
				               nk_filter_binary);

				nk_label(ctx, "Password:", NK_TEXT_LEFT);
				{
					int i = 0;
					int old_len = text_len[8];
					char buffer[64];
					for (i = 0; i < text_len[8]; ++i)
						buffer[i] = '*';
					nk_edit_string(ctx, NK_EDIT_FIELD, buffer, &text_len[8], 64,
					               nk_filter_default);
					if (old_len < text_len[8])
						memcpy(&text[8][old_len], &buffer[old_len],
						       (nk_size)(text_len[8] - old_len));
				}

				nk_label(ctx, "Field:", NK_TEXT_LEFT);
				nk_edit_string(ctx, NK_EDIT_FIELD, field_buffer, &field_len, 64,
				               nk_filter_default);

				nk_label(ctx, "Box:", NK_TEXT_LEFT);
				nk_layout_row_static(ctx, 180, 278, 1);
				nk_edit_string(ctx, NK_EDIT_BOX, box_buffer, &box_len, 512,
				               nk_filter_default);

				nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
				active = nk_edit_string(ctx, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER, text[7],
				                        &text_len[7], 64, nk_filter_ascii);
				if (nk_button_label(ctx, "Submit") || (active & NK_EDIT_COMMITED))
				{
					text[7][text_len[7]] = '\n';
					text_len[7]++;
					memcpy(&box_buffer[box_len], &text[7], (nk_size)text_len[7]);
					box_len += text_len[7];
					text_len[7] = 0;
				}
				nk_tree_pop(ctx);
			}
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Chart", NK_MINIMIZED))
		{
			/* Chart Widgets
			 * This library has two different rather simple charts. The line and the
			 * column chart. Both provide a simple way of visualizing values and
			 * have a retained mode and immediate mode API version. For the retain
			 * mode version `nk_plot` and `nk_plot_function` you either provide
			 * an array or a callback to call to handle drawing the graph.
			 * For the immediate mode version you start by calling `nk_chart_begin`
			 * and need to provide min and max values for scaling on the Y-axis.
			 * and then call `nk_chart_push` to push values into the chart.
			 * Finally `nk_chart_end` needs to be called to end the process. */
			float id = 0;
			static int col_index = -1;
			static int line_index = -1;
			float step = (2 * 3.141592654f) / 32;

			int i;
			int index = -1;
			struct nk_rect bounds;

			/* line chart */
			id = 0;
			index = -1;
			nk_layout_row_dynamic(ctx, 100, 1);
			bounds = nk_widget_bounds(ctx);
			if (nk_chart_begin(ctx, NK_CHART_LINES, 32, -1.0f, 1.0f))
			{
				for (i = 0; i < 32; ++i)
				{
					nk_flags res = nk_chart_push(ctx, (float)cos(id));
					if (res & NK_CHART_HOVERING)
						index = (int)i;
					if (res & NK_CHART_CLICKED)
						line_index = (int)i;
					id += step;
				}
				nk_chart_end(ctx);
			}

			if (index != -1)
				nk_tooltipf(ctx, "Value: %.2f", (float)cos((float)index * step));
			if (line_index != -1)
			{
				nk_layout_row_dynamic(ctx, 20, 1);
				nk_labelf(ctx, NK_TEXT_LEFT, "Selected value: %.2f",
				          (float)cos((float)index * step));
			}

			/* column chart */
			nk_layout_row_dynamic(ctx, 100, 1);
			bounds = nk_widget_bounds(ctx);
			if (nk_chart_begin(ctx, NK_CHART_COLUMN, 32, 0.0f, 1.0f))
			{
				for (i = 0; i < 32; ++i)
				{
					nk_flags res = nk_chart_push(ctx, (float)fabs(sin(id)));
					if (res & NK_CHART_HOVERING)
						index = (int)i;
					if (res & NK_CHART_CLICKED)
						col_index = (int)i;
					id += step;
				}
				nk_chart_end(ctx);
			}
			if (index != -1)
				nk_tooltipf(ctx, "Value: %.2f", (float)fabs(sin(step * (float)index)));
			if (col_index != -1)
			{
				nk_layout_row_dynamic(ctx, 20, 1);
				nk_labelf(ctx, NK_TEXT_LEFT, "Selected value: %.2f",
				          (float)fabs(sin(step * (float)col_index)));
			}

			/* mixed chart */
			nk_layout_row_dynamic(ctx, 100, 1);
			bounds = nk_widget_bounds(ctx);
			if (nk_chart_begin(ctx, NK_CHART_COLUMN, 32, 0.0f, 1.0f))
			{
				nk_chart_add_slot(ctx, NK_CHART_LINES, 32, -1.0f, 1.0f);
				nk_chart_add_slot(ctx, NK_CHART_LINES, 32, -1.0f, 1.0f);
				for (id = 0, i = 0; i < 32; ++i)
				{
					nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
					nk_chart_push_slot(ctx, (float)cos(id), 1);
					nk_chart_push_slot(ctx, (float)sin(id), 2);
					id += step;
				}
			}
			nk_chart_end(ctx);

			/* mixed colored chart */
			nk_layout_row_dynamic(ctx, 100, 1);
			bounds = nk_widget_bounds(ctx);
			if (nk_chart_begin_colored(ctx, NK_CHART_LINES, nk_rgb(255, 0, 0),
			                           nk_rgb(150, 0, 0), 32, 0.0f, 1.0f))
			{
				nk_chart_add_slot_colored(ctx, NK_CHART_LINES, nk_rgb(0, 0, 255),
				                          nk_rgb(0, 0, 150), 32, -1.0f, 1.0f);
				nk_chart_add_slot_colored(ctx, NK_CHART_LINES, nk_rgb(0, 255, 0),
				                          nk_rgb(0, 150, 0), 32, -1.0f, 1.0f);
				for (id = 0, i = 0; i < 32; ++i)
				{
					nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
					nk_chart_push_slot(ctx, (float)cos(id), 1);
					nk_chart_push_slot(ctx, (float)sin(id), 2);
					id += step;
				}
			}
			nk_chart_end(ctx);
			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Popup", NK_MINIMIZED))
		{
			static struct nk_color color = {255, 0, 0, 255};
			static int select[4];
			static int popup_active;
			const struct nk_input* in = &ctx->input;
			struct nk_rect bounds;

			/* menu contextual */
			nk_layout_row_static(ctx, 30, 160, 1);
			bounds = nk_widget_bounds(ctx);
			nk_label(ctx, "Right click me for menu", NK_TEXT_LEFT);

			if (nk_contextual_begin(ctx, 0, nk_vec2(100, 300), bounds))
			{
				static size_t prog = 40;
				static int slider = 10;

				nk_layout_row_dynamic(ctx, 25, 1);
				nk_checkbox_label(ctx, "Menu", &show_menu);
				nk_progress(ctx, &prog, 100, NK_MODIFIABLE);
				nk_slider_int(ctx, 0, &slider, 16, 1);
				if (nk_contextual_item_label(ctx, "About", NK_TEXT_CENTERED))
					show_app_about = nk_true;
				nk_selectable_label(ctx, select[0] ? "Unselect" : "Select", NK_TEXT_LEFT,
				                    &select[0]);
				nk_selectable_label(ctx, select[1] ? "Unselect" : "Select", NK_TEXT_LEFT,
				                    &select[1]);
				nk_selectable_label(ctx, select[2] ? "Unselect" : "Select", NK_TEXT_LEFT,
				                    &select[2]);
				nk_selectable_label(ctx, select[3] ? "Unselect" : "Select", NK_TEXT_LEFT,
				                    &select[3]);
				nk_contextual_end(ctx);
			}

			/* color contextual */
			nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
			nk_layout_row_push(ctx, 120);
			nk_label(ctx, "Right Click here:", NK_TEXT_LEFT);
			nk_layout_row_push(ctx, 50);
			bounds = nk_widget_bounds(ctx);
			nk_button_color(ctx, color);
			nk_layout_row_end(ctx);

			if (nk_contextual_begin(ctx, 0, nk_vec2(350, 60), bounds))
			{
				nk_layout_row_dynamic(ctx, 30, 4);
				color.r = (nk_byte)nk_propertyi(ctx, "#r", 0, color.r, 255, 1, 1);
				color.g = (nk_byte)nk_propertyi(ctx, "#g", 0, color.g, 255, 1, 1);
				color.b = (nk_byte)nk_propertyi(ctx, "#b", 0, color.b, 255, 1, 1);
				color.a = (nk_byte)nk_propertyi(ctx, "#a", 0, color.a, 255, 1, 1);
				nk_contextual_end(ctx);
			}

			/* popup */
			nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
			nk_layout_row_push(ctx, 120);
			nk_label(ctx, "Popup:", NK_TEXT_LEFT);
			nk_layout_row_push(ctx, 50);
			if (nk_button_label(ctx, "Popup"))
				popup_active = 1;
			nk_layout_row_end(ctx);

			if (popup_active)
			{
				static struct nk_rect s = {20, 100, 220, 90};
				if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Error", 0, s))
				{
					nk_layout_row_dynamic(ctx, 25, 1);
					nk_label(ctx, "A terrible error as occured", NK_TEXT_LEFT);
					nk_layout_row_dynamic(ctx, 25, 2);
					if (nk_button_label(ctx, "OK"))
					{
						popup_active = 0;
						nk_popup_close(ctx);
					}
					if (nk_button_label(ctx, "Cancel"))
					{
						popup_active = 0;
						nk_popup_close(ctx);
					}
					nk_popup_end(ctx);
				}
				else
					popup_active = nk_false;
			}

			/* tooltip */
			nk_layout_row_static(ctx, 30, 150, 1);
			bounds = nk_widget_bounds(ctx);
			nk_label(ctx, "Hover me for tooltip", NK_TEXT_LEFT);
			if (nk_input_is_mouse_hovering_rect(in, bounds))
				nk_tooltip(ctx, "This is a tooltip");

			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Layout", NK_MINIMIZED))
		{
			if (nk_tree_push(ctx, NK_TREE_NODE, "Widget", NK_MINIMIZED))
			{
				float ratio_two[] = {0.2f, 0.6f, 0.2f};
				float width_two[] = {100, 200, 50};

				nk_layout_row_dynamic(ctx, 30, 1);
				nk_label(ctx,
				         "Dynamic fixed column layout with generated position and size:",
				         NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 30, 3);
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");

				nk_layout_row_dynamic(ctx, 30, 1);
				nk_label(ctx,
				         "static fixed column layout with generated position and size:",
				         NK_TEXT_LEFT);
				nk_layout_row_static(ctx, 30, 100, 3);
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");

				nk_layout_row_dynamic(ctx, 30, 1);
				nk_label(ctx,
				         "Dynamic array-based custom column layout with generated "
				         "position and custom size:",
				         NK_TEXT_LEFT);
				nk_layout_row(ctx, NK_DYNAMIC, 30, 3, ratio_two);
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");

				nk_layout_row_dynamic(ctx, 30, 1);
				nk_label(ctx,
				         "Static array-based custom column layout with generated "
				         "position and custom size:",
				         NK_TEXT_LEFT);
				nk_layout_row(ctx, NK_STATIC, 30, 3, width_two);
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");

				nk_layout_row_dynamic(ctx, 30, 1);
				nk_label(ctx,
				         "Dynamic immediate mode custom column layout with generated "
				         "position and custom size:",
				         NK_TEXT_LEFT);
				nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 3);
				nk_layout_row_push(ctx, 0.2f);
				nk_button_label(ctx, "button");
				nk_layout_row_push(ctx, 0.6f);
				nk_button_label(ctx, "button");
				nk_layout_row_push(ctx, 0.2f);
				nk_button_label(ctx, "button");
				nk_layout_row_end(ctx);

				nk_layout_row_dynamic(ctx, 30, 1);
				nk_label(ctx,
				         "Static immediate mode custom column layout with generated "
				         "position and custom size:",
				         NK_TEXT_LEFT);
				nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
				nk_layout_row_push(ctx, 100);
				nk_button_label(ctx, "button");
				nk_layout_row_push(ctx, 200);
				nk_button_label(ctx, "button");
				nk_layout_row_push(ctx, 50);
				nk_button_label(ctx, "button");
				nk_layout_row_end(ctx);

				nk_layout_row_dynamic(ctx, 30, 1);
				nk_label(ctx, "Static free space with custom position and custom size:",
				         NK_TEXT_LEFT);
				nk_layout_space_begin(ctx, NK_STATIC, 60, 4);
				nk_layout_space_push(ctx, nk_rect(100, 0, 100, 30));
				nk_button_label(ctx, "button");
				nk_layout_space_push(ctx, nk_rect(0, 15, 100, 30));
				nk_button_label(ctx, "button");
				nk_layout_space_push(ctx, nk_rect(200, 15, 100, 30));
				nk_button_label(ctx, "button");
				nk_layout_space_push(ctx, nk_rect(100, 30, 100, 30));
				nk_button_label(ctx, "button");
				nk_layout_space_end(ctx);

				nk_layout_row_dynamic(ctx, 30, 1);
				nk_label(ctx, "Row template:", NK_TEXT_LEFT);
				nk_layout_row_template_begin(ctx, 30);
				nk_layout_row_template_push_dynamic(ctx);
				nk_layout_row_template_push_variable(ctx, 80);
				nk_layout_row_template_push_static(ctx, 80);
				nk_layout_row_template_end(ctx);
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");
				nk_button_label(ctx, "button");

				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Group", NK_MINIMIZED))
			{
				static int group_titlebar = nk_false;
				static int group_border = nk_true;
				static int group_no_scrollbar = nk_false;
				static int group_width = 320;
				static int group_height = 200;

				nk_flags group_flags = 0;
				if (group_border)
					group_flags |= NK_WINDOW_BORDER;
				if (group_no_scrollbar)
					group_flags |= NK_WINDOW_NO_SCROLLBAR;
				if (group_titlebar)
					group_flags |= NK_WINDOW_TITLE;

				nk_layout_row_dynamic(ctx, 30, 3);
				nk_checkbox_label(ctx, "Titlebar", &group_titlebar);
				nk_checkbox_label(ctx, "Border", &group_border);
				nk_checkbox_label(ctx, "No Scrollbar", &group_no_scrollbar);

				nk_layout_row_begin(ctx, NK_STATIC, 22, 3);
				nk_layout_row_push(ctx, 50);
				nk_label(ctx, "size:", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 130);
				nk_property_int(ctx, "#Width:", 100, &group_width, 500, 10, 1);
				nk_layout_row_push(ctx, 130);
				nk_property_int(ctx, "#Height:", 100, &group_height, 500, 10, 1);
				nk_layout_row_end(ctx);

				nk_layout_row_static(ctx, (float)group_height, group_width, 2);
				if (nk_group_begin(ctx, "Group", group_flags))
				{
					int i = 0;
					static int selected[16];
					nk_layout_row_static(ctx, 18, 100, 1);
					for (i = 0; i < 16; ++i)
						nk_selectable_label(ctx,
						                    (selected[i]) ? "Selected" : "Unselected",
						                    NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(ctx);
				}
				nk_tree_pop(ctx);
			}
			if (nk_tree_push(ctx, NK_TREE_NODE, "Tree", NK_MINIMIZED))
			{
				static int root_selected = 0;
				int sel = root_selected;
				if (nk_tree_element_push(ctx, NK_TREE_NODE, "Root", NK_MINIMIZED, &sel))
				{
					static int selected[8];
					int i = 0, node_select = selected[0];
					if (sel != root_selected)
					{
						root_selected = sel;
						for (i = 0; i < 8; ++i)
							selected[i] = sel;
					}
					if (nk_tree_element_push(ctx, NK_TREE_NODE, "Node", NK_MINIMIZED,
					                         &node_select))
					{
						int j = 0;
						static int sel_nodes[4];
						if (node_select != selected[0])
						{
							selected[0] = node_select;
							for (i = 0; i < 4; ++i)
								sel_nodes[i] = node_select;
						}
						nk_layout_row_static(ctx, 18, 100, 1);
						for (j = 0; j < 4; ++j)
							nk_selectable_symbol_label(ctx, NK_SYMBOL_CIRCLE_SOLID,
							                           (sel_nodes[j]) ? "Selected"
							                                          : "Unselected",
							                           NK_TEXT_RIGHT, &sel_nodes[j]);
						nk_tree_element_pop(ctx);
					}
					nk_layout_row_static(ctx, 18, 100, 1);
					for (i = 1; i < 8; ++i)
						nk_selectable_symbol_label(ctx, NK_SYMBOL_CIRCLE_SOLID,
						                           (selected[i]) ? "Selected"
						                                         : "Unselected",
						                           NK_TEXT_RIGHT, &selected[i]);
					nk_tree_element_pop(ctx);
				}
				nk_tree_pop(ctx);
			}
			if (nk_tree_push(ctx, NK_TREE_NODE, "Notebook", NK_MINIMIZED))
			{
				static int current_tab = 0;
				struct nk_rect bounds;
				float step = (2 * 3.141592654f) / 32;
				enum chart_type
				{
					CHART_LINE,
					CHART_HISTO,
					CHART_MIXED
				};
				const char* names[] = {"Lines", "Columns", "Mixed"};
				float id = 0;
				int i;

				/* Header */
				nk_style_push_vec2(ctx, &ctx->style.window.spacing, nk_vec2(0, 0));
				nk_style_push_float(ctx, &ctx->style.button.rounding, 0);
				nk_layout_row_begin(ctx, NK_STATIC, 20, 3);
				for (i = 0; i < 3; ++i)
				{
					/* make sure button perfectly fits text */
					const struct nk_user_font* f = ctx->style.font;
					float text_width =
					    f->width(f->userdata, f->height, names[i], nk_strlen(names[i]));
					float widget_width = text_width + 3 * ctx->style.button.padding.x;
					nk_layout_row_push(ctx, widget_width);
					if (current_tab == i)
					{
						/* active tab gets highlighted */
						struct nk_style_item button_color = ctx->style.button.normal;
						ctx->style.button.normal = ctx->style.button.active;
						current_tab = nk_button_label(ctx, names[i]) ? i : current_tab;
						ctx->style.button.normal = button_color;
					}
					else
						current_tab = nk_button_label(ctx, names[i]) ? i : current_tab;
				}
				nk_style_pop_float(ctx);

				/* Body */
				nk_layout_row_dynamic(ctx, 140, 1);
				if (nk_group_begin(ctx, "Notebook", NK_WINDOW_BORDER))
				{
					nk_style_pop_vec2(ctx);
					switch (current_tab)
					{
					default:
						break;
					case CHART_LINE:
						nk_layout_row_dynamic(ctx, 100, 1);
						bounds = nk_widget_bounds(ctx);
						if (nk_chart_begin_colored(ctx, NK_CHART_LINES, nk_rgb(255, 0, 0),
						                           nk_rgb(150, 0, 0), 32, 0.0f, 1.0f))
						{
							nk_chart_add_slot_colored(ctx, NK_CHART_LINES,
							                          nk_rgb(0, 0, 255),
							                          nk_rgb(0, 0, 150), 32, -1.0f, 1.0f);
							for (i = 0, id = 0; i < 32; ++i)
							{
								nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
								nk_chart_push_slot(ctx, (float)cos(id), 1);
								id += step;
							}
						}
						nk_chart_end(ctx);
						break;
					case CHART_HISTO:
						nk_layout_row_dynamic(ctx, 100, 1);
						bounds = nk_widget_bounds(ctx);
						if (nk_chart_begin_colored(ctx, NK_CHART_COLUMN,
						                           nk_rgb(255, 0, 0), nk_rgb(150, 0, 0),
						                           32, 0.0f, 1.0f))
						{
							for (i = 0, id = 0; i < 32; ++i)
							{
								nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
								id += step;
							}
						}
						nk_chart_end(ctx);
						break;
					case CHART_MIXED:
						nk_layout_row_dynamic(ctx, 100, 1);
						bounds = nk_widget_bounds(ctx);
						if (nk_chart_begin_colored(ctx, NK_CHART_LINES, nk_rgb(255, 0, 0),
						                           nk_rgb(150, 0, 0), 32, 0.0f, 1.0f))
						{
							nk_chart_add_slot_colored(ctx, NK_CHART_LINES,
							                          nk_rgb(0, 0, 255),
							                          nk_rgb(0, 0, 150), 32, -1.0f, 1.0f);
							nk_chart_add_slot_colored(ctx, NK_CHART_COLUMN,
							                          nk_rgb(0, 255, 0),
							                          nk_rgb(0, 150, 0), 32, 0.0f, 1.0f);
							for (i = 0, id = 0; i < 32; ++i)
							{
								nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
								nk_chart_push_slot(ctx, (float)fabs(cos(id)), 1);
								nk_chart_push_slot(ctx, (float)fabs(sin(id)), 2);
								id += step;
							}
						}
						nk_chart_end(ctx);
						break;
					}
					nk_group_end(ctx);
				}
				else
					nk_style_pop_vec2(ctx);
				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Simple", NK_MINIMIZED))
			{
				nk_layout_row_dynamic(ctx, 300, 2);
				if (nk_group_begin(ctx, "Group_Without_Border", 0))
				{
					int i = 0;
					char buffer[64];
					nk_layout_row_static(ctx, 18, 150, 1);
					for (i = 0; i < 64; ++i)
					{
						sprintf(buffer, "0x%02x", i);
						nk_labelf(ctx, NK_TEXT_LEFT, "%s: scrollable region", buffer);
					}
					nk_group_end(ctx);
				}
				if (nk_group_begin(ctx, "Group_With_Border", NK_WINDOW_BORDER))
				{
					int i = 0;
					char buffer[64];
					nk_layout_row_dynamic(ctx, 25, 2);
					for (i = 0; i < 64; ++i)
					{
						sprintf(buffer, "%08d",
						        ((((i % 7) * 10) ^ 32)) + (64 + (i % 2) * 2));
						nk_button_label(ctx, buffer);
					}
					nk_group_end(ctx);
				}
				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Complex", NK_MINIMIZED))
			{
				int i;
				nk_layout_space_begin(ctx, NK_STATIC, 500, 64);
				nk_layout_space_push(ctx, nk_rect(0, 0, 150, 500));
				if (nk_group_begin(ctx, "Group_left", NK_WINDOW_BORDER))
				{
					static int selected[32];
					nk_layout_row_static(ctx, 18, 100, 1);
					for (i = 0; i < 32; ++i)
						nk_selectable_label(ctx,
						                    (selected[i]) ? "Selected" : "Unselected",
						                    NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(ctx);
				}

				nk_layout_space_push(ctx, nk_rect(160, 0, 150, 240));
				if (nk_group_begin(ctx, "Group_top", NK_WINDOW_BORDER))
				{
					nk_layout_row_dynamic(ctx, 25, 1);
					nk_button_label(ctx, "#FFAA");
					nk_button_label(ctx, "#FFBB");
					nk_button_label(ctx, "#FFCC");
					nk_button_label(ctx, "#FFDD");
					nk_button_label(ctx, "#FFEE");
					nk_button_label(ctx, "#FFFF");
					nk_group_end(ctx);
				}

				nk_layout_space_push(ctx, nk_rect(160, 250, 150, 250));
				if (nk_group_begin(ctx, "Group_buttom", NK_WINDOW_BORDER))
				{
					nk_layout_row_dynamic(ctx, 25, 1);
					nk_button_label(ctx, "#FFAA");
					nk_button_label(ctx, "#FFBB");
					nk_button_label(ctx, "#FFCC");
					nk_button_label(ctx, "#FFDD");
					nk_button_label(ctx, "#FFEE");
					nk_button_label(ctx, "#FFFF");
					nk_group_end(ctx);
				}

				nk_layout_space_push(ctx, nk_rect(320, 0, 150, 150));
				if (nk_group_begin(ctx, "Group_right_top", NK_WINDOW_BORDER))
				{
					static int selected[4];
					nk_layout_row_static(ctx, 18, 100, 1);
					for (i = 0; i < 4; ++i)
						nk_selectable_label(ctx,
						                    (selected[i]) ? "Selected" : "Unselected",
						                    NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(ctx);
				}

				nk_layout_space_push(ctx, nk_rect(320, 160, 150, 150));
				if (nk_group_begin(ctx, "Group_right_center", NK_WINDOW_BORDER))
				{
					static int selected[4];
					nk_layout_row_static(ctx, 18, 100, 1);
					for (i = 0; i < 4; ++i)
						nk_selectable_label(ctx,
						                    (selected[i]) ? "Selected" : "Unselected",
						                    NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(ctx);
				}

				nk_layout_space_push(ctx, nk_rect(320, 320, 150, 150));
				if (nk_group_begin(ctx, "Group_right_bottom", NK_WINDOW_BORDER))
				{
					static int selected[4];
					nk_layout_row_static(ctx, 18, 100, 1);
					for (i = 0; i < 4; ++i)
						nk_selectable_label(ctx,
						                    (selected[i]) ? "Selected" : "Unselected",
						                    NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(ctx);
				}
				nk_layout_space_end(ctx);
				nk_tree_pop(ctx);
			}

			if (nk_tree_push(ctx, NK_TREE_NODE, "Splitter", NK_MINIMIZED))
			{
				const struct nk_input* in = &ctx->input;
				nk_layout_row_static(ctx, 20, 320, 1);
				nk_label(ctx, "Use slider and spinner to change tile size", NK_TEXT_LEFT);
				nk_label(ctx, "Drag the space between tiles to change tile ratio",
				         NK_TEXT_LEFT);

				if (nk_tree_push(ctx, NK_TREE_NODE, "Vertical", NK_MINIMIZED))
				{
					static float a = 100, b = 100, c = 100;
					struct nk_rect bounds;

					float row_layout[5];
					row_layout[0] = a;
					row_layout[1] = 8;
					row_layout[2] = b;
					row_layout[3] = 8;
					row_layout[4] = c;

					/* header */
					nk_layout_row_static(ctx, 30, 100, 2);
					nk_label(ctx, "left:", NK_TEXT_LEFT);
					nk_slider_float(ctx, 10.0f, &a, 200.0f, 10.0f);

					nk_label(ctx, "middle:", NK_TEXT_LEFT);
					nk_slider_float(ctx, 10.0f, &b, 200.0f, 10.0f);

					nk_label(ctx, "right:", NK_TEXT_LEFT);
					nk_slider_float(ctx, 10.0f, &c, 200.0f, 10.0f);

					/* tiles */
					nk_layout_row(ctx, NK_STATIC, 200, 5, row_layout);

					/* left space */
					if (nk_group_begin(ctx, "left",
					                   NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER |
					                       NK_WINDOW_NO_SCROLLBAR))
					{
						nk_layout_row_dynamic(ctx, 25, 1);
						nk_button_label(ctx, "#FFAA");
						nk_button_label(ctx, "#FFBB");
						nk_button_label(ctx, "#FFCC");
						nk_button_label(ctx, "#FFDD");
						nk_button_label(ctx, "#FFEE");
						nk_button_label(ctx, "#FFFF");
						nk_group_end(ctx);
					}

					/* scaler */
					bounds = nk_widget_bounds(ctx);
					nk_spacing(ctx, 1);
					if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
					     nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
					    nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
					{
						a = row_layout[0] + in->mouse.delta.x;
						b = row_layout[2] - in->mouse.delta.x;
					}

					/* middle space */
					if (nk_group_begin(ctx, "center",
					                   NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
					{
						nk_layout_row_dynamic(ctx, 25, 1);
						nk_button_label(ctx, "#FFAA");
						nk_button_label(ctx, "#FFBB");
						nk_button_label(ctx, "#FFCC");
						nk_button_label(ctx, "#FFDD");
						nk_button_label(ctx, "#FFEE");
						nk_button_label(ctx, "#FFFF");
						nk_group_end(ctx);
					}

					/* scaler */
					bounds = nk_widget_bounds(ctx);
					nk_spacing(ctx, 1);
					if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
					     nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
					    nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
					{
						b = (row_layout[2] + in->mouse.delta.x);
						c = (row_layout[4] - in->mouse.delta.x);
					}

					/* right space */
					if (nk_group_begin(ctx, "right",
					                   NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
					{
						nk_layout_row_dynamic(ctx, 25, 1);
						nk_button_label(ctx, "#FFAA");
						nk_button_label(ctx, "#FFBB");
						nk_button_label(ctx, "#FFCC");
						nk_button_label(ctx, "#FFDD");
						nk_button_label(ctx, "#FFEE");
						nk_button_label(ctx, "#FFFF");
						nk_group_end(ctx);
					}

					nk_tree_pop(ctx);
				}

				if (nk_tree_push(ctx, NK_TREE_NODE, "Horizontal", NK_MINIMIZED))
				{
					static float a = 100, b = 100, c = 100;
					struct nk_rect bounds;

					/* header */
					nk_layout_row_static(ctx, 30, 100, 2);
					nk_label(ctx, "top:", NK_TEXT_LEFT);
					nk_slider_float(ctx, 10.0f, &a, 200.0f, 10.0f);

					nk_label(ctx, "middle:", NK_TEXT_LEFT);
					nk_slider_float(ctx, 10.0f, &b, 200.0f, 10.0f);

					nk_label(ctx, "bottom:", NK_TEXT_LEFT);
					nk_slider_float(ctx, 10.0f, &c, 200.0f, 10.0f);

					/* top space */
					nk_layout_row_dynamic(ctx, a, 1);
					if (nk_group_begin(ctx, "top",
					                   NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
					{
						nk_layout_row_dynamic(ctx, 25, 3);
						nk_button_label(ctx, "#FFAA");
						nk_button_label(ctx, "#FFBB");
						nk_button_label(ctx, "#FFCC");
						nk_button_label(ctx, "#FFDD");
						nk_button_label(ctx, "#FFEE");
						nk_button_label(ctx, "#FFFF");
						nk_group_end(ctx);
					}

					/* scaler */
					nk_layout_row_dynamic(ctx, 8, 1);
					bounds = nk_widget_bounds(ctx);
					nk_spacing(ctx, 1);
					if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
					     nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
					    nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
					{
						a = a + in->mouse.delta.y;
						b = b - in->mouse.delta.y;
					}

					/* middle space */
					nk_layout_row_dynamic(ctx, b, 1);
					if (nk_group_begin(ctx, "middle",
					                   NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
					{
						nk_layout_row_dynamic(ctx, 25, 3);
						nk_button_label(ctx, "#FFAA");
						nk_button_label(ctx, "#FFBB");
						nk_button_label(ctx, "#FFCC");
						nk_button_label(ctx, "#FFDD");
						nk_button_label(ctx, "#FFEE");
						nk_button_label(ctx, "#FFFF");
						nk_group_end(ctx);
					}

					{
						/* scaler */
						nk_layout_row_dynamic(ctx, 8, 1);
						bounds = nk_widget_bounds(ctx);
						if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
						     nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
						    nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
						{
							b = b + in->mouse.delta.y;
							c = c - in->mouse.delta.y;
						}
					}

					/* bottom space */
					nk_layout_row_dynamic(ctx, c, 1);
					if (nk_group_begin(ctx, "bottom",
					                   NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
					{
						nk_layout_row_dynamic(ctx, 25, 3);
						nk_button_label(ctx, "#FFAA");
						nk_button_label(ctx, "#FFBB");
						nk_button_label(ctx, "#FFCC");
						nk_button_label(ctx, "#FFDD");
						nk_button_label(ctx, "#FFEE");
						nk_button_label(ctx, "#FFFF");
						nk_group_end(ctx);
					}
					nk_tree_pop(ctx);
				}
				nk_tree_pop(ctx);
			}
			nk_tree_pop(ctx);
		}
	}
	nk_end(ctx);
	// nk_window_is_closed(ctx, "Overview");
}

} // namespace gui
