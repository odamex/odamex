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
//  Server window cvar table
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

#include "server_cvar_table.h"

#include "FL/fl_draw.H"

/******************************************************************************/

enum cvarColumn_e
{
	CCOL_VARIABLE,
	CCOL_VALUE,
};

static const int MAX_CVAR_COLUMNS = CCOL_VALUE + 1;

static const char* CVAR_HEADER_STRINGS[] = {"Variable", "Value"};

/******************************************************************************/

ServerCvarTable::ServerCvarTable(int X, int Y, int W, int H, const char* l)
    : Fl_Table(X, Y, W, H, l)
{
	cols(MAX_CVAR_COLUMNS);
	col_header(1);
	col_resize(1);
}

/******************************************************************************/

void ServerCvarTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W,
                                int H)
{
	switch (context)
	{
	case CONTEXT_STARTPAGE:
		break;
	case CONTEXT_ENDPAGE:
		break;
	case CONTEXT_ROW_HEADER:
		break;
	case CONTEXT_COL_HEADER:
		if (C >= ARRAY_LENGTH(::CVAR_HEADER_STRINGS))
			break;

		fl_push_clip(X, Y, W, H);
		{
			fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, ::FL_BACKGROUND_COLOR);
			fl_font(::FL_HELVETICA_BOLD, 14);
			fl_color(::FL_BLACK);
			fl_draw(::CVAR_HEADER_STRINGS[C], X, Y, W, H, ::FL_ALIGN_CENTER, 0, 0);
		}
		fl_pop_clip();
		break;
	case CONTEXT_CELL:
		if (C >= ARRAY_LENGTH(::CVAR_HEADER_STRINGS))
			break;

		fl_push_clip(X, Y, W, H);
		{
			fl_color(::FL_WHITE);

			fl_rectf(X, Y, W, H);

			fl_font(::FL_HELVETICA, 14);
			fl_color(::FL_BLACK);

			switch (C)
			{
			case CCOL_VARIABLE:
				fl_draw(m_cvars[R].key.c_str(), X, Y, W, H, ::FL_ALIGN_LEFT, 0, 0);
				break;
			case CCOL_VALUE:
				fl_draw(m_cvars[R].value.c_str(), X, Y, W, H, ::FL_ALIGN_LEFT, 0, 0);
				break;
			default:
				break;
			}
		}
		fl_pop_clip();
		break;
	case CONTEXT_RC_RESIZE:
		break;
	}
}

/******************************************************************************/

void ServerCvarTable::refreshInfo()
{
	DB_GetServerCvars(m_cvars, m_strAddress);
	rows(int(m_cvars.size()));
}
