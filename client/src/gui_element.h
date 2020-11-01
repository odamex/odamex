#ifndef __GUI_ELEMENT_H__
#define __GUI_ELEMENT_H__

#include "layout.h"

#include <limits>

#include "dobject.h"

/**
 * @brief Class for managing construction and destruction of layout contexts.
 */
class OGUIContext
{
	lay_context m_layout;
	int m_maxWidth;
	int m_maxHeight;
	double m_scaleWidth;
	double m_scaleHeight;

	// Contexts can't be copied.
	OGUIContext(const OGUIContext&);
	OGUIContext& operator=(const OGUIContext&);

  public:
	OGUIContext()
	    : m_maxWidth(320), m_maxHeight(200), m_scaleWidth(1.0), m_scaleHeight(1.0)
	{
		lay_init_context(&m_layout);
	}
	~OGUIContext()
	{
		lay_destroy_context(&m_layout);
	}
	lay_context* layoutAddr()
	{
		return &m_layout;
	}
};

/**
 * @brief Root node from which all other types of GUI objects inherit from.
 */
class DGUIElement : public DObject
{
	DECLARE_CLASS(DGUIElement, DObject)

  protected:
	/**
	 * @brief Shared layout pointer that all nodes of a widget use.
	 */
	OGUIContext& m_ctx;
	DGUIElement(OGUIContext& ctx) : m_ctx(ctx)
	{
	}

  public:
	/**
	 * @brief Functionality to be run after a layout is reset but before
	 *        it is run.  Usually allocates a fresh layout ID and applies
	 *        the proper settings.
	 */
	virtual void layout() = 0;

	/**
	 * @brief Functionality to actually render the element.
	 */
	virtual void render() = 0;
};

/**
 * @brief Wraps a "dim" that draws a solid block of color over an area of
 *        the screen.
 */
class DGUIDim : public DGUIElement
{
	DECLARE_CLASS(DGUIDim, DGUIElement)
	lay_id m_layoutID;

  public:
	DGUIDim(OGUIContext& ctx);
	void layout();
	void render();
};

/**
 * @brief Wraps a flat drawer that draws a repeating flat to an area of
 *        the screen, like the screenblocks border or endscreen.
 */
class DGUIFlat : public DGUIElement
{
	DECLARE_CLASS(DGUIFlat, DGUIElement)
	lay_id m_layoutID;
	std::string m_flatLump;

  public:
	DGUIFlat(OGUIContext& ctx);
	void layout();
	void render();
};

/**
 * @brief Wraps a patch drawer.
 */
class DGUIPatch : public DGUIElement
{
	DECLARE_CLASS(DGUIPatch, DGUIElement)
	lay_id m_layoutID;
	std::string m_patchLump;

  public:
	DGUIPatch(OGUIContext& ctx);
	void layout();
	void render();
};

/**
 * @brief Wraps a text drawer.
 */
class DGUIText : public DGUIElement
{
	DECLARE_CLASS(DGUIText, DGUIElement)
	lay_id m_layoutID;
	std::string m_text;

  public:
	DGUIText(OGUIContext& ctx, const std::string& text);
	void layout();
	void render();
};

#endif
