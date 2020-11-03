#ifndef __GUI_ELEMENT_H__
#define __GUI_ELEMENT_H__

#include "layout.h"

#include <limits>
#include <vector>

#include "dobject.h"

/**
 * @brief Used to indicate an unset layout ID.
 */
const lay_id LAYOUT_NOID = std::numeric_limits<lay_id>::max();

/**
 * @brief Class for managing construction and destruction of layout contexts.
 */
class OGUIContext
{
	lay_context m_layout;
	int m_maxWidth;
	int m_maxHeight;
	float m_scaleWidth;
	float m_scaleHeight;

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

	/**
	 * @brief The most recent layout ID of this element.  Returned bu getID().
	 *
	 * @detail This ID is usually set by the layout() method.  The element
	 *         is
	 */
	lay_id m_layoutID;

	DGUIElement(OGUIContext& ctx) : m_ctx(ctx), m_layoutID(LAYOUT_NOID)
	{
	}

  public:
	virtual ~DGUIElement()
	{
	}

	/**
	 * @brief Gets the currently-set layout ID.
	 *
	 * @return Currently set Layout ID.
	 */
	virtual lay_id getID()
	{
		return m_layoutID;
	}

	/**
	 * @brief Functionality to be run after a layout is reset but before
	 *        it is run.
	 *
	 * @detail A typical implementation should create a new layout item,
	 *         apply settings to it, and set m_layoutID.  If this element
	 *         has children, their layout methods should also be run and
	 *         the proper relationships should be set up.
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
class DGUIContainer : public DGUIElement
{
	DECLARE_CLASS(DGUIContainer, DGUIElement)
	uint32_t m_containFlags;
	uint32_t m_behaveFlags;
	std::vector<DGUIElement*> m_children;

  public:
	DGUIContainer(OGUIContext& ctx, uint32_t containFlags, uint32_t behaviorFlags);
	~DGUIContainer();
	void layout();
	void render();
	void push_back(DGUIElement* ele);
};

/**
 * @brief Wraps a "dim" that draws a solid block of color over an area of
 *        the screen.
 */
class DGUIDim : public DGUIElement
{
	DECLARE_CLASS(DGUIDim, DGUIElement)
	std::string m_color;
	float m_amount;
	DGUIElement* m_child;

  public:
	DGUIDim(OGUIContext& ctx, const std::string& color, float amount);
	DGUIDim(OGUIContext& ctx, const std::string& color, float amount, DGUIElement* child);
	~DGUIDim();
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
	std::string m_text;

  public:
	DGUIText(OGUIContext& ctx, const std::string& text);
	void layout();
	void render();
};

#endif
