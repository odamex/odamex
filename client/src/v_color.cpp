// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//   Color-specific functions and utilities.
//
//-----------------------------------------------------------------------------

#include "v_color.h"

#include <math.h>

// RGB in linear 0.0-1.0 space.
struct rgbLinear_t
{
	float r;
	float g;
	float b;
};

/**
 * @brief Measure distance between two colors.
 *
 * @sa https://stackoverflow.com/a/40950076/91642
 */
float V_ColorDistance(const argb_t& e1, const argb_t& e2)
{
	const int e1r = e1.getr();
	const int e1g = e1.getg();
	const int e1b = e1.getb();

	const int e2r = e2.getr();
	const int e2g = e2.getg();
	const int e2b = e2.getb();

	const int rmean = (e1r + e2r) / 2;
	const int r = e1r - e2r;
	const int g = e1g - e2g;
	const int b = e1b - e2b;

	return sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g +
	            (((767 - rmean) * b * b) >> 8));
}

/**
 * @brief Find the closest palette index given a palette and color to compare.
 */
palindex_t V_BestColor2(const argb_t* palette, const argb_t& color)
{
	float bestDist = INFINITY;
	int bestColor = 0;

	for (int i = 0; i < 256; i++)
	{
		argb_t palColor(palette[i]);

		float dist = V_ColorDistance(color, palColor);
		if (dist < bestDist)
		{
			if (dist == 0.0f)
			{
				return i; // perfect match
			}

			bestDist = dist;
			bestColor = i;
		}
	}

	return bestColor;
}

/**
 * @brief Turn a color into a greyscale index.
 *
 * @sa https://stackoverflow.com/a/9085524/91642
 */
byte V_ColorToGrey(const argb_t& color)
{
	float lr = color.getr() / 255.f;
	float lg = color.getg() / 255.f;
	float lb = color.getb() / 255.f;

	float grey = 0.2126 * lr + 0.7152 * lg + 0.0722 * lb;

	return grey * 255.f;
}

static float FromSRGBChannel(const byte x)
{
	const float y = x / 255.0;
	if (y <= 0.04045)
	{
		return y / 12.92;
	}
	else
	{
		return pow((y + 0.055) / 1.055, 2.4);
	}
}

/**
 * @brief Returns a linear value in the range [0,1] for sRGB input in [0,255].
 */
static rgbLinear_t FromSRGB(const argb_t& color)
{
	rgbLinear_t rvo;
	rvo.r = FromSRGBChannel(color.getr());
	rvo.g = FromSRGBChannel(color.getg());
	rvo.b = FromSRGBChannel(color.getb());
	return rvo;
}

static byte ToSRGBChannel(const float x)
{
	const float y = x <= 0.0031308 ? 12.92 * x : (1.055 * (pow(x, 1 / 2.4))) - 0.055;
	return 255.9999 * y;
}

/**
 * @brief Returns a sRGB value in the range [0,255] for linear input in [0,1]
 */
static argb_t ToSRGB(const rgbLinear_t& color)
{
	argb_t rvo;
	rvo.setr(ToSRGBChannel(color.r));
	rvo.setg(ToSRGBChannel(color.g));
	rvo.setb(ToSRGBChannel(color.b));
	return rvo;
}

/**
 * @brief Lerp all channels of linear color.
 */
static rgbLinear_t LerpLinear(const rgbLinear_t& a, const rgbLinear_t& b, const float f)
{
	rgbLinear_t rvo;
	rvo.r = lerp(a.r, b.r, f);
	rvo.g = lerp(a.g, b.g, f);
	rvo.b = lerp(a.b, b.b, f);
	return rvo;
}

/**
 * @brief Given a start and end color, return a color gradient.
 *
 * @sa https://stackoverflow.com/a/49321304/91642
 */
OGradient V_ColorGradient(const argb_t& color1, const argb_t& color2, const size_t len)
{
	OGradient rvo;
	rvo.resize(len);

	const float GAMMA = 0.43f;

	const rgbLinear_t linearOne = FromSRGB(color1);
	const rgbLinear_t linearTwo = FromSRGB(color2);

	const float brightOne = pow(linearOne.r + linearOne.g + linearOne.b, GAMMA);
	const float brightTwo = pow(linearTwo.r + linearTwo.g + linearTwo.b, GAMMA);

	for (size_t i = 0; i < len; i++)
	{
		const float mix = i / (len - 1.f);
		const float intensity = pow(lerp(brightOne, brightTwo, mix), 1 / GAMMA);
		rgbLinear_t color = LerpLinear(linearOne, linearTwo, mix);
		const float sumColor = color.r + color.g + color.b;
		if (sumColor != 0)
		{
			color.r = color.r * intensity / sumColor;
			color.g = color.g * intensity / sumColor;
			color.b = color.b * intensity / sumColor;
		}
		rvo.at(i) = ToSRGB(color);
	}

	return rvo;
}

#include "cmdlib.h"
#include "i_video.h"
#include "v_video.h"

void V_DebugGradient(const argb_t& a, const argb_t& b, const size_t len, const int y)
{
	OGradient grad = V_ColorGradient(a, b, len);

	std::string buffer;
	for (size_t i = 0; i < grad.size(); i++)
	{
		StrFormat(buffer, "%02x %02x %02x", grad.at(i).getr(), grad.at(i).getg(),
		          grad.at(i).getb());
		I_GetPrimaryCanvas()->Dim(y, 32 + i * 16, 32, 16, buffer.c_str(), 1.0);
	}
}
