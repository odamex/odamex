
#include "gui_element.h"

#include "v_text.h"
#include "v_textcolors.h"
#include "v_video.h"
#include "w_wad.h"

/**
 * @brief Used to indicate an unset layout ID.
 */
const lay_id LAYOUT_NOID = std::numeric_limits<lay_id>::max();

/*
 * Base abstract element.
 */

IMPLEMENT_CLASS(DGUIElement, DObject)

/*
 * Dim element
 */

IMPLEMENT_CLASS(DGUIDim, DGUIElement)

void DGUIDim::layout()
{
	m_layoutID = lay_item(m_ctx.layoutAddr());
}

void DGUIDim::render()
{
	if (m_layoutID == LAYOUT_NOID)
		return;

	lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
	::screen->Dim(vec[0], vec[1], vec[2], vec[3]);
}

/*
 * Flat element
 */

IMPLEMENT_CLASS(DGUIFlat, DGUIElement)

void DGUIFlat::layout()
{
	m_layoutID = lay_item(m_ctx.layoutAddr());
}

void DGUIFlat::render()
{
	if (m_layoutID == LAYOUT_NOID)
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
	m_layoutID = lay_item(m_ctx.layoutAddr());
}

void DGUIPatch::render()
{
	if (m_layoutID == LAYOUT_NOID)
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
    : DGUIElement(ctx), m_layoutID(LAYOUT_NOID), m_text(text)
{
}

void DGUIText::layout()
{
	// [AM] TODO: Introduce font height checker later.
	const int FONT_HEIGHT = 7;

	// Get an ID for our text.
	m_layoutID = lay_item(m_ctx.layoutAddr());

	// Set the width and height of our text.
	int width = V_StringWidth(m_text.c_str());
	int height = FONT_HEIGHT;
	lay_set_size_xy(m_ctx.layoutAddr(), m_layoutID, width, height);
}

void DGUIText::render()
{
	if (m_layoutID == LAYOUT_NOID)
		return;

	// Draw text sourced at the upper-left corner of our layout item.
	lay_vec4 vec = lay_get_rect(m_ctx.layoutAddr(), m_layoutID);
	::screen->DrawText(CR_GREY, vec[0], vec[1], m_text.c_str());
}
