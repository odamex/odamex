#include "odamex.h"

#include <algorithm>

#include "cl_responderkeys.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "i_system.h"
#include "i_time.h"
#include "i_video.h"
#include "m_menu.h"
#include "m_options_valuesets.h"
#include "m_videomodes.h"
#include "m_widgets.h"
#include "v_palette.h"
#include "v_text.h"
#include "v_video.h"
#include "w_wad.h"

EXTERN_CVAR(vid_widescreen)
EXTERN_CVAR(vid_maxfps)
EXTERN_CVAR(vid_overscan)
EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_32bpp)
EXTERN_CVAR(vid_vsync)

extern short indicatorAnimCounter;

namespace
{
#ifndef GCONSOLE
	constexpr int VM_FULLSCREENITEM = 0;
#endif
	constexpr int VM_WIDESCREENITEM = 1;
	constexpr int VM_VSYNCITEM = 2;
	constexpr int VM_FRAMERATEITEM = 3;
	constexpr int VM_32BPPITEM = 4;
	constexpr int VM_RESSTART = 6;
	constexpr int VM_ENTERLINE = 15;
	constexpr int VM_TESTLINE = 17;

	uint16_t oldWidth = 0;
	uint16_t oldHeight = 0;

#ifdef GCONSOLE
	const char VMEnterText[] = "Press A to set mode";
	const char VMTestText[] = "Press X to test mode for 5 seconds";
#else
	const char VMEnterText[] = "Press ENTER to set mode";
	const char VMTestText[] = "Press T to test mode for 5 seconds";
#endif

	const char VMTestWaitText[] = "Please wait 5 seconds...";

	value_t VidFPSCaps[] = {
		{35.0, "35fps"},
		{60.0, "60fps"},
		{70.0, "70fps"},
		{105.0, "105fps"},
		{120.0, "120fps"},
		{140.0, "140fps"},
		{144.0, "144fps"},
		{240.0, "240fps"},
		{0.0, "Unlimited"},
	};

	value_t FullScreenOptions[] = {
		{WINDOW_Windowed, "Window"},
		{WINDOW_Fullscreen, "Full Screen Exclusive"},
		{WINDOW_DesktopFullscreen, "Full Screen Window"},
	};

	value_t WidescreenMode[] = {
		{0.0, "Off"},
		{1.0, "Auto"},
		{2.0, "16:10"},
		{3.0, "16:9"},
		{4.0, "21:9"},
		{5.0, "32:9"},
	};

	menuitem_t videoModeItems[] = {
#ifdef GCONSOLE
		{slider, "Overscan", {&vid_overscan}, {0.84375}, {1.0}, {0.03125}, {NULL}},
#else
		{discrete, "Fullscreen", {&vid_fullscreen}, {3.0}, {0.0}, {0.0}, {FullScreenOptions}},
#endif
		{discrete, "Widescreen", {&vid_widescreen}, {6.0}, {0.0}, {0.0}, {WidescreenMode}},
		{discrete, "VSync", {&vid_vsync}, {2.0}, {0.0}, {0.0}, {nullptr}},
		{discrete, "Framerate", {&vid_maxfps}, {9.0}, {0.0}, {0.0}, {VidFPSCaps}},
		{discrete, "32-bit color", {&vid_32bpp}, {2.0}, {0.0}, {0.0}, {nullptr}},
		{redtext, "", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{whitetext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
		{yellowtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
	};

	menu_t videoModesMenu = {
		"M_VIDMOD",
		0,
		static_cast<int>(ARRAY_LENGTH(videoModeItems)),
		130,
		videoModeItems,
		0,
		0,
		NULL,
	};

	bool VideoModeSelectable(const menuitem_t& item)
	{
		if (item.type == redtext || item.type == whitetext || item.type == yellowtext ||
		    item.type == orangetext)
		{
			return false;
		}

		if (item.type == screenres && item.b.res1 == nullptr)
		{
			return false;
		}

		return true;
	}

	void EnsureSelectedResolutionColumn(int currentItem)
	{
		if (currentItem >= 0 && currentItem < videoModesMenu.numitems &&
		    videoModesMenu.items[currentItem].type == screenres &&
		    videoModesMenu.items[currentItem].a.selmode < 0)
		{
			videoModesMenu.items[currentItem].a.selmode = 0;
		}
	}

	void SetVideoMode(uint16_t width, uint16_t height)
	{
		oldWidth = I_GetVideoWidth();
		oldHeight = I_GetVideoHeight();

		AddCommandString(fmt::format("vid_setmode {} {}", width, height));
	}

	void BuildModesList(int hiwidth, int hiheight)
	{
		using MenuModeList = std::vector<std::pair<uint16_t, uint16_t>>;

		const bool fullscreen = I_GetWindow()->getVideoMode().isFullScreen();
		MenuModeList menuModeList;
		const IVideoModeList* videoModeList = I_GetVideoCapabilities()->getSupportedVideoModes();

		for (const auto& mode : *videoModeList)
		{
			if (mode.isFullScreen() == fullscreen)
			{
				menuModeList.emplace_back(mode.width, mode.height);
			}
		}

		menuModeList.erase(std::unique(menuModeList.begin(), menuModeList.end()), menuModeList.end());

		auto modeIt = menuModeList.begin();
		for (int i = VM_RESSTART; videoModeItems[i].type == screenres; ++i)
		{
			videoModeItems[i].e.highlight = -1;
			videoModeItems[i].a.selmode = -1;

			char** columns[] = {&videoModeItems[i].b.res1, &videoModeItems[i].c.res2,
			                    &videoModeItems[i].d.res3};
			for (int col = 0; col < 3; ++col)
			{
				if (modeIt != menuModeList.end())
				{
					const auto [width, height] = *modeIt++;
					if (width == hiwidth && height == hiheight)
					{
						videoModeItems[i].e.highlight = col;
						videoModeItems[i].a.selmode = col;
					}

					char resolution[32];
					snprintf(resolution, sizeof(resolution), "%dx%d", width, height);
					ReplaceString(columns[col], resolution);
				}
				else
				{
					ReplaceString(columns[col], nullptr);
				}
			}
		}
	}

	bool GetSelectedSize(int line, int* width, int* height)
	{
		if (videoModeItems[line].type != screenres)
		{
			return false;
		}

		const char* resolution = nullptr;
		switch (videoModeItems[line].a.selmode)
		{
		case 0:
			resolution = videoModeItems[line].b.res1;
			break;
		case 1:
			resolution = videoModeItems[line].c.res2;
			break;
		case 2:
			resolution = videoModeItems[line].d.res3;
			break;
		default:
			break;
		}

		if (resolution == nullptr)
		{
			return false;
		}

		const char* xpos = strchr(resolution, 'x');
		if (xpos == nullptr)
		{
			xpos = strchr(resolution, 'X');
		}
		if (xpos == nullptr)
		{
			return false;
		}

		*width = atoi(std::string(resolution, xpos - resolution).c_str());
		*height = atoi(xpos + 1);
		return true;
	}

	void SetModesMenu(int width, int height)
	{
		if (!testingmode)
		{
			videoModeItems[VM_ENTERLINE].label = VMEnterText;
			videoModeItems[VM_TESTLINE].label = VMTestText;
		}
		else
		{
			static char enterText[64];
			snprintf(enterText, sizeof(enterText), "TESTING %dx%d", width, height);
			videoModeItems[VM_ENTERLINE].label = enterText;
			videoModeItems[VM_TESTLINE].label = VMTestWaitText;
		}

		BuildModesList(width, height);
	}

	bool ApplySelectedMode(int currentItem)
	{
		int width = I_GetVideoWidth();
		int height = I_GetVideoHeight();

		if (videoModeItems[currentItem].type == screenres)
		{
			GetSelectedSize(currentItem, &width, &height);
		}

		SetVideoMode(width, height);
		SetModesMenu(width, height);
		return true;
	}

	bool TestSelectedMode(int currentItem)
	{
		int width = I_GetVideoWidth();
		int height = I_GetVideoHeight();

		if (videoModeItems[currentItem].type == screenres)
		{
			GetSelectedSize(currentItem, &width, &height);
		}

		testingmode = I_MSTime() * TICRATE / 1000 + 5 * TICRATE;
		SetVideoMode(width, height);
		SetModesMenu(width, height);
		return true;
	}

	void MoveSelection(int direction, int& currentItem)
	{
		int next = currentItem;
		do
		{
			next += direction;
			if (next >= videoModesMenu.numitems)
			{
				next = 0;
			}
			else if (next < 0)
			{
				next = videoModesMenu.numitems - 1;
			}
		} while (!VideoModeSelectable(videoModeItems[next]));

		if (videoModeItems[currentItem].type == screenres)
		{
			videoModeItems[currentItem].a.selmode = -1;
		}

		currentItem = next;
		EnsureSelectedResolutionColumn(currentItem);
		videoModesMenu.lastOn = currentItem;
	}

	void ChangeSelectedValue(menuitem_t& item, int direction)
	{
		if (item.type == slider)
		{
			float newValue = item.a.cvar->value() + (direction > 0 ? item.d.step : -item.d.step);
			if (item.b.leftval < item.c.rightval)
			{
			newValue = std::clamp(newValue, item.b.leftval, item.c.rightval);
			}
			else
			{
			newValue = std::clamp(newValue, item.c.rightval, item.b.leftval);
			}
			item.a.cvar->Set(newValue);
			return;
		}

		if (item.type == discrete)
		{
			const int numValues = static_cast<int>(item.b.leftval);
			int currentValue = M_FindCurVal(item.a.cvar->value(), item.e.values, numValues);
			currentValue += direction > 0 ? 1 : -1;
			if (currentValue < 0)
			{
				currentValue = numValues - 1;
			}
			else if (currentValue >= numValues)
			{
				currentValue = 0;
			}

			item.a.cvar->Set(item.e.values[currentValue].value);
		}
	}
} // namespace

int testingmode = 0;

void M_VideoModesInit()
{
	int yesNoCount = 0;
	value_t* yesNo = M_OptionValueSet("YesNo", yesNoCount);
	videoModeItems[VM_VSYNCITEM].e.values = yesNo;
	videoModeItems[VM_32BPPITEM].e.values = yesNo;

#ifndef GCONSOLE
	switch (I_GetVideoCapabilities()->getDisplayType())
	{
	case DISPLAY_FullscreenOnly:
		videoModeItems[VM_FULLSCREENITEM].type = nochoice;
		videoModeItems[VM_FULLSCREENITEM].b.leftval = 1.0f;
		break;

	case DISPLAY_WindowOnly:
		videoModeItems[VM_FULLSCREENITEM].type = nochoice;
		videoModeItems[VM_FULLSCREENITEM].b.leftval = 0.0f;
		break;

	default:
		break;
	}
#endif
}

void M_RestoreVideoMode()
{
	testingmode = 0;
	SetVideoMode(oldWidth, oldHeight);
	SetModesMenu(oldWidth, oldHeight);
}

void M_ModeFlashTestText()
{
	videoModeItems[VM_TESTLINE].label = (I_MSTime() & 256) ? VMTestWaitText : "";
}

void M_RefreshModesList()
{
	BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
}

void M_VideoModesOpen(int& currentItem)
{
	SetModesMenu(I_GetVideoWidth(), I_GetVideoHeight());
	currentItem = videoModesMenu.lastOn;
	EnsureSelectedResolutionColumn(currentItem);
}

void M_VideoModesRestore(int& currentItem)
{
	currentItem = videoModesMenu.lastOn;
	EnsureSelectedResolutionColumn(currentItem);
}

void M_VideoModesDrawer(int currentItem)
{
	const OFont* smallFont = OFonts.small();
	const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
	const EColorRange titleColor = M_MenuTextColor("title");
	const EColorRange itemColor = M_MenuTextColor("item");
	const EColorRange itemHighlightColor = M_MenuTextColor("itemHighlight");
	const EColorRange valueColor = M_MenuTextColor("value");
	const EColorRange labelColor = M_MenuTextColor("label");
	int y = 15;

	if (W_CheckNumForName(videoModesMenu.title) >= 0)
	{
		const patch_t* title = W_CachePatch(videoModesMenu.title);
		screen->DrawPatchCleanWithPalette(title, 160 - title->width() / 2, 10, palette);
		y += title->height();
	}

	for (int i = 0; i < videoModesMenu.numitems; ++i, y += M_SmallFontLineHeight())
	{
		menuitem_t& item = videoModeItems[i];

		if (item.type == screenres)
		{
			const char* columns[] = {item.b.res1, item.c.res2, item.d.res3};
			for (int column = 0; column < 3; ++column)
			{
				if (columns[column] == nullptr)
				{
					continue;
				}

				const int color = column == item.e.highlight ? itemHighlightColor : itemColor;
				screen->DrawTextCleanMove(smallFont, color, 104 * column + 20, y, columns[column]);
			}

			if (i == currentItem &&
			    (((item.a.selmode != -1) && indicatorAnimCounter < 6) || testingmode != 0))
			{
				const patch_t* cursor = M_MenuCursorPatch();
				screen->DrawPatchCleanWithPalette(cursor, item.a.selmode * 104 + 8,
				                                  y + M_MenuCursorOffsetY(), palette);
			}
			continue;
		}

		const int width = item.label != nullptr ? V_StringWidth(smallFont, item.label) : 0;
		int x = videoModesMenu.indent - width;
		int color = itemColor;

		switch (item.type)
		{
		case redtext:
			x = 160 - width / 2;
			color = itemColor;
			break;
		case whitetext:
			x = 160 - width / 2;
			color = itemHighlightColor;
			break;
		case yellowtext:
			x = 160 - width / 2;
			color = labelColor;
			break;
		default:
			break;
		}

		screen->DrawTextCleanMove(smallFont, color, x, y, item.label);

		switch (item.type)
		{
		case discrete:
		{
			const int numValues = static_cast<int>(item.b.leftval);
			const int currentValue = M_FindCurVal(item.a.cvar->value(), item.e.values, numValues);
			const char* value = currentValue == numValues ? "Unknown" : item.e.values[currentValue].name;
			screen->DrawTextCleanMove(smallFont, valueColor, videoModesMenu.indent + 14, y, value);
			break;
		}

		case nochoice:
			screen->DrawTextCleanMove(smallFont, labelColor, videoModesMenu.indent + 14, y,
			                          item.e.values[static_cast<int>(item.b.leftval)].name);
			break;

		case slider:
			M_DrawSlider(videoModesMenu.indent + 8, y, item.b.leftval, item.c.rightval,
			             item.a.cvar->value(), item.d.step);
			break;

		default:
			break;
		}

		if (i == currentItem && indicatorAnimCounter < 6)
		{
			const patch_t* cursor = M_MenuCursorPatch();
			screen->DrawPatchCleanWithPalette(cursor, videoModesMenu.indent + 3,
												y + M_MenuCursorOffsetY(), palette);
		}
	}
}

void M_VideoModesResponder(int keyCode, int typedChar, bool numlock, int& currentItem)
{
	menuitem_t& item = videoModeItems[currentItem];

	if (Key_IsDownKey(keyCode, numlock))
	{
		MoveSelection(1, currentItem);
		M_PlayMenuSound("navigate");
		return;
	}

	if (Key_IsUpKey(keyCode, numlock))
	{
		MoveSelection(-1, currentItem);
		M_PlayMenuSound("navigate");
		return;
	}

	if (Key_IsLeftKey(keyCode, numlock))
	{
		if (item.type == screenres)
		{
			if (item.a.selmode > 0)
			{
				item.a.selmode--;
				M_PlayMenuSound("navigate");
			}
			else if (currentItem > VM_RESSTART &&
			         videoModeItems[currentItem - 1].type == screenres &&
			         videoModeItems[currentItem - 1].b.res1 != nullptr)
			{
				item.a.selmode = -1;
				--currentItem;
				videoModeItems[currentItem].a.selmode =
				    videoModeItems[currentItem].d.res3 != nullptr ? 2 :
				    videoModeItems[currentItem].c.res2 != nullptr ? 1 :
				                                             0;
				M_PlayMenuSound("navigate");
			}
		}
		else if (item.type == discrete || item.type == slider)
		{
			ChangeSelectedValue(item, -1);
			M_PlayMenuSound("changeValue");
		}

		videoModesMenu.lastOn = currentItem;
		return;
	}

	if (Key_IsRightKey(keyCode, numlock))
	{
		if (item.type == screenres)
		{
			const bool hasNextColumn =
			    (item.a.selmode == 0 && item.c.res2 != nullptr) ||
			    (item.a.selmode == 1 && item.d.res3 != nullptr);
			if (hasNextColumn)
			{
				item.a.selmode++;
				M_PlayMenuSound("navigate");
			}
			else if (currentItem + 1 < videoModesMenu.numitems &&
			         videoModeItems[currentItem + 1].type == screenres &&
			         videoModeItems[currentItem + 1].b.res1 != nullptr)
			{
				item.a.selmode = -1;
				++currentItem;
				videoModeItems[currentItem].a.selmode = 0;
				M_PlayMenuSound("navigate");
			}
		}
		else if (item.type == discrete || item.type == slider)
		{
			ChangeSelectedValue(item, 1);
			M_PlayMenuSound("changeValue");
		}

		videoModesMenu.lastOn = currentItem;
		return;
	}

	if (Key_IsAcceptKey(keyCode))
	{
		if (item.type == discrete || item.type == slider)
		{
			ChangeSelectedValue(item, 1);
			M_PlayMenuSound("changeValue");
		}
		else if (ApplySelectedMode(currentItem))
		{
			M_PlayMenuSound("select");
		}

		videoModesMenu.lastOn = currentItem;
		return;
	}

	if (Key_IsCancelKey(keyCode))
	{
		videoModesMenu.lastOn = currentItem;
		M_PopMenuStack();
		return;
	}

#ifdef GCONSOLE
	if (typedChar == 't' || keyCode == OKEY_JOY3)
#else
	if (typedChar == 't')
#endif
	{
		if (TestSelectedMode(currentItem))
		{
			M_PlayMenuSound("select");
		}
	}
}
