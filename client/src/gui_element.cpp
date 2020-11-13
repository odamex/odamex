
#include "gui_element.h"

#include "v_text.h"
#include "v_textcolors.h"
#include "v_video.h"
#include "w_wad.h"

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

void DGUIPatch::layout()
{
	DGUIElement::layout();
}

void DGUIPatch::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	// Find the flat to render.
	int index = W_CheckNumForName(m_patchLump.c_str(), ns_global);
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

DGUIText::DGUIText(OGUIContext& ctx, const std::string& text)
    : DGUIElement(ctx), m_text(text)
{
}

void DGUIText::layout()
{
	// [AM] TODO: Introduce font height checker later.
	const int FONT_HEIGHT = 7;

	// Default layout is fine...mostly.  We override the size later.
	DGUIElement::layout();

	// Set the width and height of our text.
	int width = V_StringWidth(m_text.c_str());
	int height = FONT_HEIGHT;
	lay_set_size_xy(m_ctx.layoutAddr(), m_layoutID, width, height);
}

void DGUIText::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	// Draw text sourced at the upper-left corner of our layout item.
	lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
	::screen->DrawText(CR_GREY, vec[0], vec[1], m_text.c_str());
}
