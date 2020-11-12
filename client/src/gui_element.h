#ifndef __GUI_ELEMENT_H__
#define __GUI_ELEMENT_H__

#include "layout.h"

#include <limits>
#include <vector>

#include "dobject.h"

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
	 * @brief Shared layout pointer that all elements of a single widget use.
	 */
	OGUIContext& m_ctx;

	/**
	 * @brief Size of the element.
	 *
	 * @detail Some elements, like text, always know their own size and do
	 *         not use this information.
	 */
	lay_vec2 m_size;

	/**
	 * @brief Containment flags that are passed to lay_set_contain.
	 */
	uint32_t m_containFlags;

	/**
	 * @brief Behavior flags that are passed to lay_set_behave.
	 */
	uint32_t m_behaveFlags;

	/**
	 * @brief Margins to apply to the item in left, top, right, bottom order.
	 */
	lay_vec4 m_margins;

	/**
	 * @brief The most recent layout ID of this element.  Returned bu getID().
	 *
	 * @detail This ID is usually set by the layout() method.  In the case
	 *         of elements that consist of multiple items, this should contain
	 *         the "root" ID that encompasses the others.
	 */
	lay_id m_layoutID;

	DGUIElement(OGUIContext& ctx)
	    : m_ctx(ctx), m_containFlags(0), m_behaveFlags(0), m_layoutID(LAY_INVALID_ID)
	{
		m_size = {0, 0};
		m_margins = {0, 0, 0, 0};
	}

  public:
	virtual ~DGUIElement()
	{
	}

	/**
	 * @brief Set size of element.
	 *
	 * @param w Element width.  Use 0 for growable width.
	 * @param h Element height.  Use 0 for growable height.
	 */
	void size(lay_scalar w, lay_scalar h)
	{
		m_size = {w, h};
	}

	/**
	 * @brief Set containment layout flags.
	 *
	 * @param flags Flags to set.
	 */
	void contain(uint32_t flags)
	{
		m_containFlags = flags;
	}

	/**
	 * @brief Set containment layout flags.
	 *
	 * @param flags Flags to set.
	 */
	void behave(uint32_t flags)
	{
		m_behaveFlags = flags;
	}

	/**
	 * @brief Set margin around an element.
	 *
	 * @param all Margin size around element.
	 */
	void margin(lay_scalar all)
	{
		m_margins = {all, all, all, all};
	}

	/**
	 * @brief Set margin around an element.
	 *
	 * @param tb Margin size on top and bottom.
	 * @param lr Margin size on left and right.
	 */
	void margin(lay_scalar tb, lay_scalar lr)
	{
		m_margins = {lr, tb, lr, tb};
	}

	/**
	 * @brief Set margin around an element.
	 *
	 * @param t Margin size on top.
	 * @param lr Margin size on left and right.
	 * @param b Margin size on bottom.
	 */
	void margin(lay_scalar t, lay_scalar lr, lay_scalar b)
	{
		m_margins = {lr, t, lr, b};
	}

	/**
	 * @brief Set margin around an element.
	 *
	 * @param t Margin size on top.
	 * @param r Margin size on right.
	 * @param b Margin size on bottom.
	 * @param l Margin size on left.
	 */
	void margin(lay_scalar t, lay_scalar r, lay_scalar b, lay_scalar l)
	{
		m_margins = {l, t, r, b};
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
 * @brief An invisible container that exists only to contain other elements.
 */
class DGUIContainer : public DGUIElement
{
	DECLARE_CLASS(DGUIContainer, DGUIElement)
	std::vector<DGUIElement*> m_children;

  public:
	DGUIContainer(OGUIContext& ctx);
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
	std::vector<DGUIElement*> m_children;

  public:
	DGUIDim(OGUIContext& ctx, const std::string& color, float amount);
	~DGUIDim();
	void layout();
	void render();
	void push_back(DGUIElement* ele);
};

/**
 * @brief Wraps a flat drawer that draws a repeating flat to an area of
 *        the screen, like the screenblocks border or endscreen.
 */
class DGUIFlat : public DGUIElement
{
	DECLARE_CLASS(DGUIFlat, DGUIElement)
	std::string m_flatLump;
	std::vector<DGUIElement*> m_children;

  public:
	DGUIFlat(OGUIContext& ctx, const std::string& flatLump);
	void layout();
	void render();
	void push_back(DGUIElement* ele);
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
