
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
	for (std::vector<DGUIElement*>::iterator it = m_children.begin();
	     it != m_children.end(); ++it)
	{
		delete *it;
	}
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

void DGUIContainer::layout()
{
	// Default layout is fine.
	DGUIElement::layout();

	lay_id lastID = LAY_INVALID_ID;
	for (std::vector<DGUIElement*>::iterator it = m_children.begin();
	     it != m_children.end(); ++it)
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

void DGUIContainer::render()
{
	for (std::vector<DGUIElement*>::iterator it = m_children.begin();
	     it != m_children.end(); ++it)
	{
		(*it)->render();
	}
}

/*
 * Dim element
 */

IMPLEMENT_CLASS(DGUIDim, DGUIElement)

DGUIDim::DGUIDim(OGUIContext& ctx, const std::string& color, float amount)
    : DGUIElement(ctx), m_color(color), m_amount(amount), m_child(NULL)
{
}

DGUIDim::DGUIDim(OGUIContext& ctx, const std::string& color, float amount,
                 DGUIElement* child)
    : DGUIElement(ctx), m_color(color), m_amount(amount), m_child(child)
{
}

DGUIDim::~DGUIDim()
{
	delete m_child;
	m_child = NULL;
}

void DGUIDim::layout()
{
	// Default layout is fine.
	DGUIElement::layout();

	if (m_child != NULL)
	{
		m_child->layout();

		// Establish parent-child item relationship.
		lay_id childID = m_child->getID();
		lay_insert(m_ctx.layoutAddr(), m_layoutID, childID);
	}
}

void DGUIDim::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
	::screen->Dim(vec[0], vec[1], vec[2], vec[3], m_color.c_str(), m_amount);

	// Render child after parent.
	if (m_child != NULL)
		m_child->render();
}

/*
 * Flat element
 */

IMPLEMENT_CLASS(DGUIFlat, DGUIElement)

void DGUIFlat::layout()
{
	// Default layout is fine.
	DGUIElement::layout();
}

void DGUIFlat::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
	::screen->FlatFill(vec[0], vec[1], vec[2], vec[3],
	                   (byte*)W_CacheLumpName(m_flatLump.c_str(), PU_CACHE));
}

/*
 * Patch element
 */

IMPLEMENT_CLASS(DGUIPatch, DGUIElement)

void DGUIPatch::layout()
{
	// Default layout is fine.
	DGUIElement::layout();
}

void DGUIPatch::render()
{
	if (m_layoutID == LAY_INVALID_ID)
		return;

	patch_t* patch = W_CachePatch(m_patchLump.c_str());
	if (patch == NULL)
		return;

	lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
	::screen->DrawPatch(patch, vec[0], vec[1]);
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
