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

#include "doomtype.h"
#include "i_system.h"
#include "i_video.h"

struct rawfb_context
{
	nk_context ctx;
	struct nk_rect scissors;
	IWindowSurface* surface;
	IWindowSurface* fontSurface;
	nk_font_atlas atlas;
};

static void CTXSetpixel(const rawfb_context* rawfb, const short x0, const short y0,
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

static void LineHorizontal(const rawfb_context* rawfb, const short x0, const short y,
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

static void Scissor(rawfb_context* rawfb, const float x, const float y, const float w,
                    const float h)
{
	rawfb->scissors.x = MIN(MAX(x, 0.f), (float)rawfb->surface->getWidth());
	rawfb->scissors.y = MIN(MAX(y, 0.f), (float)rawfb->surface->getHeight());
	rawfb->scissors.w = MIN(MAX(w + x, 0.f), (float)rawfb->surface->getWidth());
	rawfb->scissors.h = MIN(MAX(h + y, 0.f), (float)rawfb->surface->getHeight());
}

static void StrokeLine(const rawfb_context* rawfb, short x0, short y0, short x1, short y1,
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

static void FillPolygon(const rawfb_context* rawfb, const struct nk_vec2i* pnts,
                        int count, const nk_color col)
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

static void StrokeArc(const rawfb_context* rawfb, short x0, short y0, short w, short h,
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

static void FillArc(const rawfb_context* rawfb, short x0, short y0, short w, short h,
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

static void StrokeRect(const rawfb_context* rawfb, const short x, const short y,
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

static void FillRect(const rawfb_context* rawfb, const short x, const short y,
                     const short w, const short h, const short r, const nk_color col)
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

void DrawRectMultiColor(const rawfb_context* rawfb, const short x, const short y,
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

static void FillTriangle(const rawfb_context* rawfb, const short x0, const short y0,
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

static void StrokeTriangle(const rawfb_context* rawfb, const short x0, const short y0,
                           const short x1, const short y1, const short x2, const short y2,
                           const unsigned short line_thickness, const nk_color col)
{
	StrokeLine(rawfb, x0, y0, x1, y1, line_thickness, col);
	StrokeLine(rawfb, x1, y1, x2, y2, line_thickness, col);
	StrokeLine(rawfb, x2, y2, x0, y0, line_thickness, col);
}

static void StrokePolygon(const rawfb_context* rawfb, const struct nk_vec2i* pnts,
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

static void StrokePolyline(const rawfb_context* rawfb, const struct nk_vec2i* pnts,
                           const int count, const unsigned short line_thickness,
                           const nk_color col)
{
	int i;
	for (i = 0; i < count - 1; ++i)
		StrokeLine(rawfb, pnts[i].x, pnts[i].y, pnts[i + 1].x, pnts[i + 1].y,
		           line_thickness, col);
}

static void FillCircle(const rawfb_context* rawfb, short x0, short y0, short w, short h,
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

static void StrokeCircle(const rawfb_context* rawfb, short x0, short y0, short w, short h,
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

static void StrokeCurve(const rawfb_context* rawfb, const struct nk_vec2i p1,
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

static void Clear(const rawfb_context* rawfb, const nk_color col)
{
	FillRect(rawfb, 0, 0, rawfb->surface->getWidth(), rawfb->surface->getHeight(), 0,
	         col);
}

static rawfb_context* Init(IWindowSurface* surface)
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
	rawfb_context* rawfb = new rawfb_context;
	memset(rawfb, 0, sizeof(rawfb_context));
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

static void DrawText(const rawfb_context* rawfb, const nk_user_font* font,
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

static void DrawImage(const rawfb_context* rawfb, const int x, const int y, const int w,
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

static void Shutdown(rawfb_context* rawfb)
{
	if (rawfb)
	{
		delete rawfb->fontSurface;
		nk_free(&rawfb->ctx);

		memset(rawfb, 0, sizeof(rawfb_context));
		delete rawfb;
	}
}

static void Render(const rawfb_context* rawfb, const nk_color clear,
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
			Scissor((rawfb_context*)rawfb, s->x, s->y, s->w, s->h);
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

static rawfb_context* ctx;

void I_InitGUI(IWindowSurface* surface)
{
	if (::ctx != NULL)
		return;

	::ctx = Init(surface);
}

void I_QuitGUI()
{
	Shutdown(::ctx);
	::ctx = NULL;
}

void I_DrawGUI()
{
	nk_color color = {0x7F, 0x7F, 0x00, 0x00};
	Render(::ctx, color, 1);
}

enum
{
	EASY,
	HARD
};
static int op = EASY;
static float value = 0.6f;
static int i = 20;

#define GUICTX &::ctx->ctx

void UI_SelectIWAD()
{
	if (nk_begin(GUICTX, "Show", nk_rect(50, 50, 220, 220),
	             NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE))
	{
		/* fixed widget pixel width */
		nk_layout_row_static(GUICTX, 30, 80, 1);
		if (nk_button_label(GUICTX, "button"))
		{
			/* event handling */
		}

		/* fixed widget window ratio width */
		nk_layout_row_dynamic(GUICTX, 30, 2);
		if (nk_option_label(GUICTX, "easy", op == EASY))
			op = EASY;
		if (nk_option_label(GUICTX, "hard", op == HARD))
			op = HARD;

		/* custom widget pixel width */
		nk_layout_row_begin(GUICTX, NK_STATIC, 30, 2);
		{
			nk_layout_row_push(GUICTX, 50);
			nk_label(GUICTX, "Volume:", NK_TEXT_LEFT);
			nk_layout_row_push(GUICTX, 110);
			nk_slider_float(GUICTX, 0, &value, 1.0f, 0.1f);
		}
		nk_layout_row_end(GUICTX);
		nk_end(GUICTX);
	}
}
