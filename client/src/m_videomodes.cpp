#include "odamex.h"

#include "cl_responderkeys.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "i_system.h"
#include "i_time.h"
#include "i_video.h"
#include "m_menu.h"
#include "m_options_valuesets.h"
#include "m_videomodes.h"
#include "v_text.h"

EXTERN_CVAR(vid_widescreen)
EXTERN_CVAR(vid_maxfps)
EXTERN_CVAR(vid_overscan)
EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_32bpp)
EXTERN_CVAR(vid_vsync)

namespace
{
	constexpr int VM_DEPTHITEM = 0;
	constexpr int VM_RESSTART = 6;
	constexpr int VM_ENTERLINE = 15;
	constexpr int VM_TESTLINE = 17;

	bool GetSelectedSize(int line, int* width, int* height);
	void SetModesMenu(int w, int h);

	uint16_t old_width = 0;
	uint16_t old_height = 0;
	value_t Depths[22];

#ifdef GCONSOLE
	const char VMEnterText[] = "Press A to set mode";
	const char VMTestText[] = "Press X to test mode for 5 seconds";
#else
	const char VMEnterText[] = "Press ENTER to set mode";
	const char VMTestText[] = "Press T to test mode for 5 seconds";
#endif

	const char VMTestWaitText[] = "Please wait 5 seconds...";

	value_t VidFPSCaps[] = {
		{ 35.0, "35fps" },
		{ 60.0, "60fps" },
		{ 70.0, "70fps" },
		{ 105.0, "105fps" },
		{ 120.0, "120fps" },
		{ 140.0, "140fps" },
		{ 144.0, "144fps" },
		{ 240.0, "240fps" },
		{ 0.0, "Unlimited" }
	};

	value_t FullScreenOptions[] = {
		{ WINDOW_Windowed, "Window" },
		{ WINDOW_Fullscreen, "Full Screen Exclusive" },
		{ WINDOW_DesktopFullscreen, "Full Screen Window" }
	};

	value_t WidescreenMode[] = {
		{ 0.0, "Off" },
		{ 1.0, "Auto" },
		{ 2.0, "16:10" },
		{ 3.0, "16:9" },
		{ 4.0, "21:9" },
		{ 5.0, "32:9" }
	};

	menuitem_t ModesItems[] = {
#ifdef GCONSOLE
		{ slider, "Overscan", {&vid_overscan}, {0.84375}, {1.0}, {0.03125}, {NULL} },
#else
		{ discrete, "Fullscreen", {&vid_fullscreen}, {3.0}, {0.0}, {0.0}, {FullScreenOptions} },
#endif
		{ discrete, "Widescreen", {&vid_widescreen}, {6.0}, {0.0}, {0.0}, {WidescreenMode} },
		{ discrete, "VSync", {&vid_vsync}, {2.0}, {0.0}, {0.0}, {YesNo} },
		{ discrete, "Framerate", {&vid_maxfps}, {9.0}, {0.0}, {0.0}, {VidFPSCaps} },
		{ discrete, "32-bit color", {&vid_32bpp}, {2.0}, {0.0}, {0.0}, {YesNo} },
		{ redtext, "", {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ screenres, NULL, {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ whitetext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
		{ yellowtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
	};

	menu_t ModesMenu = {
		"M_VIDMOD",
		0,
		static_cast<int>(ARRAY_LENGTH(ModesItems)),
		130,
		ModesItems,
		0,
		0,
		NULL
	};

	void SetVideoMode(uint16_t width, uint16_t height)
	{
		old_width = I_GetVideoWidth();
		old_height = I_GetVideoHeight();

		AddCommandString(fmt::format("vid_setmode {} {}", width, height));
		SetModesMenu(width, height);
	}

	void BuildModesList(int hiwidth, int hiheight)
	{
		const bool fullscreen = I_GetWindow()->getVideoMode().isFullScreen();
		typedef std::vector<std::pair<uint16_t, uint16_t>> MenuModeList;
		MenuModeList menumodelist;

		const IVideoModeList* videomodelist = I_GetVideoCapabilities()->getSupportedVideoModes();
		for (const auto& mode : *videomodelist)
		{
			if (mode.isFullScreen() == fullscreen)
			{
				menumodelist.emplace_back(mode.width, mode.height);
			}
		}
		menumodelist.erase(std::unique(menumodelist.begin(), menumodelist.end()), menumodelist.end());

		MenuModeList::const_iterator mode_it = menumodelist.begin();
		char** str = NULL;

		for (int i = VM_RESSTART; ModesItems[i].type == screenres; i++)
		{
			ModesItems[i].e.highlight = -1;
			for (int col = 0; col < 3; col++)
			{
				if (col == 0)
					str = &ModesItems[i].b.res1;
				else if (col == 1)
					str = &ModesItems[i].c.res2;
				else
					str = &ModesItems[i].d.res3;

				if (mode_it != menumodelist.end())
				{
					auto [width, height] = *mode_it;
					++mode_it;

					if (width == hiwidth && height == hiheight)
						ModesItems[i].e.highlight = ModesItems[i].a.selmode = col;

					char strtemp[32];
					snprintf(strtemp, 32, "%dx%d", width, height);
					ReplaceString(str, strtemp);
				}
				else
				{
					str = NULL;
				}
			}
		}
	}

	bool GetSelectedSize(int line, int* width, int* height)
	{
		if (ModesItems[line].type != screenres)
			return false;

		int mode_num = (line - VM_RESSTART) * 3 + ModesItems[line].a.selmode;
		const char* resolution_str = NULL;

		if (mode_num % 3 == 0)
			resolution_str = ModesItems[line].b.res1;
		else if (mode_num % 3 == 1)
			resolution_str = ModesItems[line].c.res2;
		else
			resolution_str = ModesItems[line].d.res3;

		if (!resolution_str)
			return false;

		size_t xpos = 0;
		for (const char* s = resolution_str; s; s++, xpos++)
			if (*s == 'x' || *s == 'X')
				break;

		char width_str[5] = {0}, height_str[5] = {0};
		strncpy(width_str, resolution_str, xpos);
		strncpy(height_str, resolution_str + xpos + 1, 4);

		*width = atoi(width_str);
		*height = atoi(height_str);
		return true;
	}

	void SetModesMenu(int w, int h)
	{
		if (!testingmode)
		{
			ModesItems[VM_ENTERLINE].label = VMEnterText;
			ModesItems[VM_TESTLINE].label = VMTestText;
		}
		else
		{
			static char enter_text[64];
			snprintf(enter_text, 64, "TESTING %dx%d", w, h);
			ModesItems[VM_ENTERLINE].label = enter_text;
			ModesItems[VM_TESTLINE].label = VMTestWaitText;
		}

		BuildModesList(w, h);
	}

	void SetVidMode()
	{
		SetModesMenu(I_GetVideoWidth(), I_GetVideoHeight());

		if (ModesMenu.items[ModesMenu.lastOn].type == screenres &&
		    ModesMenu.items[ModesMenu.lastOn].a.selmode == -1)
		{
			ModesMenu.items[ModesMenu.lastOn].a.selmode++;
		}

		M_ResetOptionsBuiltinState();
		M_SwitchMenu(&ModesMenu);
	}
}

int testingmode = 0;

void M_VideoModesInit()
{
	for (int i = 0; i < 22; i++)
	{
		Depths[i].value = i;
		Depths[i].name = NULL;
	}

	switch (I_GetVideoCapabilities()->getDisplayType())
	{
	case DISPLAY_FullscreenOnly:
		ModesItems[2].type = nochoice;
		ModesItems[2].b.leftval = 1.f;
		break;
	case DISPLAY_WindowOnly:
		ModesItems[2].type = nochoice;
		ModesItems[2].b.leftval = 0.f;
		break;
	default:
		break;
	}
}

void M_RestoreVideoMode()
{
	testingmode = 0;
	SetVideoMode(old_width, old_height);
}

void M_ModeFlashTestText()
{
	ModesItems[VM_TESTLINE].label = (I_MSTime() & 256) ? VMTestWaitText : "";
}

void M_OpenVideoModeScreen(void)
{
	OptionsActive = true;
	SetVidMode();
}

void M_RefreshModesList()
{
	BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
}

menu_t* M_VideoModesMenu()
{
	return &ModesMenu;
}

value_t* M_VideoModesDepths()
{
	return Depths;
}

bool M_VideoModesIsTesting()
{
	return testingmode != 0;
}

bool M_VideoModesOwnsMenu(const menu_t* menu)
{
	return menu == &ModesMenu;
}

void M_VideoModesDepthChanged()
{
	BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
}

static bool ApplySelectedMode(int currentItem, menuitem_t* item)
{
	if (item == nullptr)
	{
		return false;
	}

	int width = I_GetVideoWidth();
	int height = I_GetVideoHeight();

	if (item->type == screenres)
	{
		GetSelectedSize(currentItem, &width, &height);
	}

	SetVideoMode(width, height);
	return true;
}

static bool TestSelectedMode(int currentItem, menuitem_t* item)
{
	if (item == nullptr)
	{
		return false;
	}

	int width = I_GetVideoWidth();
	int height = I_GetVideoHeight();

	if (item->type == screenres)
	{
		GetSelectedSize(currentItem, &width, &height);
	}

	testingmode = I_MSTime() * TICRATE / 1000 + 5 * TICRATE;
	SetVideoMode(width, height);
	return true;
}

bool M_VideoModesResponder(int ch, int ch2, bool numlock, menuitem_t* item, int& currentItem)
{
	if (item == nullptr)
	{
		return false;
	}

	if (Key_IsLeftKey(ch, numlock) && item->type == screenres)
	{
		int col = item->a.selmode - 1;
		if (col < 0)
		{
			if (currentItem > 0 && ModesMenu.items[currentItem - 1].type == screenres)
			{
				item->a.selmode = -1;
				ModesMenu.items[--currentItem].a.selmode = 2;
			}
		}
		else
		{
			item->a.selmode = col;
		}
		return true;
	}

	if (Key_IsRightKey(ch, numlock) && item->type == screenres)
	{
		int col = item->a.selmode + 1;
		if ((col > 2) || (col == 2 && !item->d.res3) || (col == 1 && !item->c.res2))
		{
			if (ModesMenu.numitems - 1 > currentItem &&
			    ModesMenu.items[currentItem + 1].type == screenres &&
			    ModesMenu.items[currentItem + 1].b.res1)
			{
				item->a.selmode = -1;
				ModesMenu.items[++currentItem].a.selmode = 0;
			}
		}
		else
		{
			item->a.selmode = col;
		}
		return true;
	}

	if (Key_IsAcceptKey(ch))
	{
		return ApplySelectedMode(currentItem, item);
	}

#ifdef GCONSOLE
	if (ch2 == 't' || ch == OKEY_JOY3)
#else
	if (ch2 == 't')
#endif
	{
		return TestSelectedMode(currentItem, item);
	}

	return false;
}
