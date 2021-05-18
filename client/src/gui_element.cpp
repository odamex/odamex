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
//	Layout-based GUI elements.
//
//-----------------------------------------------------------------------------

#include "gui_element.h"

#include "cmdlib.h"
#include "v_text.h"
#include "v_textcolors.h"
#include "v_video.h"
#include "w_wad.h"

/**
 * @brief Draw a debug box around the given layout ID.
 *
 * @param ctx Context.
 * @param id Layout ID.
 */
void DrawDebugBox(OGUIContext& ctx, lay_id id, const char* color, const char* border)
{
	lay_vec4 vec = lay_get_rect(ctx.layoutAddr(), id);
	::screen->Dim(vec[0], vec[1], vec[2], vec[3], border, 1.0);
	if (vec[2] - 2 <= 0 || vec[3] - 2 <= 0)
	{
		return;
	}
	::screen->Dim(vec[0] + 1, vec[1] + 1, vec[2] - 2, vec[3] - 2, color, 1.0);
}

/*
 * Base abstract element.
 */

IMPLEMENT_CLASS(DGUIElement, DObject)

/**
 * @brief Destroy all passed elements.
 *
 * @param eles Elements to destroy.
 */
void DGUIElement::destroyElements(Elements& eles)
{
	for (Elements::iterator it = eles.begin(); it != eles.end(); ++it)
	{
		delete *it;
	}
}

/**
 * @brief Layout all passed elements.
 *
 * @param eles Elements to layout.
 */
void DGUIElement::layoutElements(Elements& eles)
{
	lay_id lastID = LAY_INVALID_ID;
	for (Elements::iterator it = eles.begin(); it != eles.end(); ++it)
	{
		(*it)->layout();
		lay_id iterID = (*it)->getID();

		// This next bit is recommended by the library docs.
		if (lastID == LAY_INVALID_ID)
		{
			// Establish parent-child item relationship.
			lay_insert(m_ctx.layoutAddr(), m_layoutID, iterID);
		}
		else
		{
			// Append this child after previous one.
			lay_append(m_ctx.layoutAddr(), lastID, iterID);
		}

		lastID = iterID;
	}
}

/**
 * @brief Render all passed elements.
 *
 * @param eles Elements to render.
 */
void DGUIElement::renderElements(Elements& eles)
{
	for (Elements::iterator it = eles.begin(); it != eles.end(); ++it)
	{
		(*it)->render();
	}
}

/**
 * @brief A default layout method that does a few common things needed for
 *        most elements.
 */
void DGUIElement::layout()
{
	m_layoutID = lay_item(m_ctx.layoutAddr());
	lay_set_size(m_ctx.layoutAddr(), m_layoutID, m_size);
	lay_set_contain(m_ctx.layoutAddr(), m_layoutID, m_containFlags);
	lay_set_behave(m_ctx.layoutAddr(), m_layoutID, m_behaveFlags);
	lay_set_margins(m_ctx.layoutAddr(), m_layoutID, m_margins);
}

/*
 * Container element - holds multiple children.
 */

IMPLEMENT_CLASS(DGUIContainer, DGUIElement)

DGUIContainer::DGUIContainer(OGUIContext& ctx) : DGUIElement(ctx)
{
}

DGUIContainer::~DGUIContainer()
{
	destroyElements(m_children);
}

void DGUIContainer::layout()
{
	DGUIElement::layout();
	layoutElements(m_children);
}

void DGUIContainer::render()
{
	renderElements(m_children);
}

/**
 * @brief Push a GUI element into the container.
 *
 * @param ele Element to push into the container.  The container takes
 *            ownership of this element, so the caller should not free this
 *            pointer.  The element must have been heap-allocated with "new".
 */
void DGUIContainer::push_back(DGUIElement* ele)
{
	m_children.push_back(ele);
}

/*
 * Dim element
 */

IMPLEMENT_CLASS(DGUIDim, DGUIElement)

DGUIDim::DGUIDim(OGUIContext& ctx, const std::string& color, float amount)
    : DGUIElement(ctx), m_color(color), m_amount(amount)
{
}

DGUIDim::~DGUIDim()
{
	destroyElements(m_children);
}

void DGUIDim::layout()
{
	DGUIElement::layout();
	layoutElements(m_children);
}

void DGUIDim::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	// Render the dim area.
	lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
	::screen->Dim(vec[0], vec[1], vec[2], vec[3], m_color.c_str(), m_amount);

	renderElements(m_children);
}

/**
 * @brief Push a GUI element into the container.
 *
 * @param ele Element to push into the container.  The container takes
 *            ownership of this element, so the caller should not free this
 *            pointer.  The element must have been heap-allocated with "new".
 */
void DGUIDim::push_back(DGUIElement* ele)
{
	m_children.push_back(ele);
}

/*
 * Flat element
 */

IMPLEMENT_CLASS(DGUIFlat, DGUIElement)

DGUIFlat::DGUIFlat(OGUIContext& ctx, const std::string& flatLump)
    : DGUIElement(ctx), m_flatLump(flatLump)
{
}

DGUIFlat::~DGUIFlat()
{
	destroyElements(m_children);
}

void DGUIFlat::layout()
{
	DGUIElement::layout();
	layoutElements(m_children);
}

void DGUIFlat::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	// Find the flat to render.
	int index = W_CheckNumForName(m_flatLump.c_str(), ns_flats);
	if (index)
	{
		// Only attempt to render the flat if it was found.
		byte* flat = (byte*)W_CacheLumpNum(index, PU_CACHE);
		lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
		::screen->FlatFill(vec[0], vec[1], vec[0] + vec[2], vec[1] + vec[3], flat);
	}

	// Render children.
	renderElements(m_children);
}

/**
 * @brief Push a GUI element into the container.
 *
 * @param ele Element to push into the container.  The container takes
 *            ownership of this element, so the caller should not free this
 *            pointer.  The element must have been heap-allocated with "new".
 */
void DGUIFlat::push_back(DGUIElement* ele)
{
	m_children.push_back(ele);
}

/*
 * Patch element
 */

IMPLEMENT_CLASS(DGUIPatch, DGUIElement)

DGUIPatch::DGUIPatch(OGUIContext& ctx, const std::string& patchLump, const namespace_t ns)
    : DGUIElement(ctx), m_patchLump(patchLump), m_namespace(ns)
{
}

void DGUIPatch::layout()
{
	DGUIElement::layout();

	// Find the patch to render.
	int index = W_CheckNumForName(m_patchLump.c_str(), m_namespace);
	if (index)
	{
		// Set the width and height of our patch.
		patch_t* patch = (patch_t*)W_CacheLumpNum(index, PU_CACHE);
		lay_set_size_xy(m_ctx.layoutAddr(), m_layoutID, patch->width(), patch->height());
	}
}

void DGUIPatch::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	// Find the flat to render.
	int index = W_CheckNumForName(m_patchLump.c_str(), m_namespace);
	if (index)
	{
		// Only attempt to render the flat if it was found.
		patch_t* patch = (patch_t*)W_CacheLumpNum(index, PU_CACHE);
		lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
		::screen->DrawPatch(patch, vec[0], vec[1]);
	}
}

/*
 * Text element
 */

IMPLEMENT_CLASS(DGUIText, DGUIElement)

DGUIText::DGUIText(OGUIContext& ctx, const std::string& text, const FontParams& params)
    : DGUIElement(ctx), m_text(text), m_params(params)
{
}

void DGUIText::layout()
{
	// Default layout is fine...mostly.  We override the size later.
	DGUIElement::layout();

	// Set the width and height of our text.
	Vec2<int> extent = V_TextExtent(m_text.c_str(), m_params);
	lay_set_size_xy(m_ctx.layoutAddr(), m_layoutID, extent.x, extent.y);
}

void DGUIText::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	// Draw text sourced at the upper-left corner of our layout item.
	lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
	::screen->DrawText(m_text.c_str(), Vec2<int>(vec[0], vec[1]), m_params);
}

IMPLEMENT_CLASS(DGUIParagraph, DGUIElement)

/**
 * @brief Chop the text into individual words with an element for each one.
 *
 * @param text Text to contain in the paragraph element.
 */
void DGUIParagraph::updateText(const std::string& text)
{
	m_text = text;
	m_children.clear();

	StringTokens tokens = TokenizeString(text, " ");
	for (StringTokens::const_iterator it = tokens.begin(); it != tokens.end(); ++it)
	{
		m_children.push_back(DGUIText(m_ctx, *it, m_params));
	}
}

DGUIParagraph::DGUIParagraph(OGUIContext& ctx, const std::string& text,
                             const FontParams& params)
    : DGUIElement(ctx), m_params(params)
{
	updateText(text);
}

void DGUIParagraph::layout()
{
	// Hardcode these flags so the text flows as expected.
	m_containFlags = LAY_ROW | LAY_FLEX | LAY_WRAP | LAY_START;
	m_behaveFlags = LAY_FILL;

	DGUIElement::layout();

	Vec2<int> spacing = V_TextExtent(" ", m_params);

	// Lay out text elements - they are a vector of like-kinded text
	// elements that were not allocated with "new", so we can't use
	// layoutElements.
	lay_id lastID = LAY_INVALID_ID;
	for (std::vector<DGUIText>::iterator it = m_children.begin(); it != m_children.end();
	     ++it)
	{
		it->layout();
		lay_id iterID = it->getID();

		// Words should have a right margin.
		lay_set_margins_ltrb(m_ctx.layoutAddr(), it->getID(), 0, 0, spacing.x, 0);

		// This next bit is recommended by the library docs.
		if (lastID == LAY_INVALID_ID)
		{
			// Establish parent-child item relationship.
			lay_insert(m_ctx.layoutAddr(), m_layoutID, iterID);
		}
		else
		{
			// Append this child after previous one.
			lay_append(m_ctx.layoutAddr(), lastID, iterID);
		}

		lastID = iterID;
	}
}

void DGUIParagraph::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	// Render text elements - they are a vector of like-kinded text
	// elements that were not allocated with "new", so we can't use
	// renderElements.
	for (std::vector<DGUIText>::iterator it = m_children.begin(); it != m_children.end();
	     ++it)
	{
		it->render();
	}
}
