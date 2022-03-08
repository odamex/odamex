// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	C_CONSOLE
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <stdarg.h>

#include "m_alloc.h"
#include "m_memio.h"
#include "g_game.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "i_system.h"
#include "i_video.h"
#include "v_palette.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_main.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "cl_responderkeys.h"
#include "cl_download.h"
#include "g_gametype.h"
#include "m_fileio.h"

#include <list>
#include <algorithm>

#ifdef __SWITCH__
#include "nx_io.h"
#endif

// These functions are standardized in C++11, POSIX standard otherwise
#if defined(_WIN32) && (__cplusplus <= 199711L)
#	define vsnprintf _vsnprintf
#	define snprintf  _snprintf
#endif

static const int MAX_LINE_LENGTH = 8192;

static bool ShouldTabCycle = false;
static size_t NextCycleIndex = 0;
static size_t PrevCycleIndex = 0;

static IWindowSurface* background_surface;

extern int		gametic;

static unsigned int		ConRows, ConCols, PhysRows;

static bool				cursoron = false;
static int				ConBottom = 0;
static int		RowAdjust = 0;

int			CursorTicker, ScrollState = 0;
constate_e	ConsoleState = c_up;

extern byte *ConChars;

BOOL		KeysShifted;
BOOL		KeysCtrl;

static bool midprinting;

#define SCROLLUP 1
#define SCROLLDN 2
#define SCROLLNO 0

EXTERN_CVAR(con_coloredmessages)
EXTERN_CVAR(con_buffersize)
EXTERN_CVAR(show_messages)
EXTERN_CVAR(print_stdout)
EXTERN_CVAR(con_notifytime)

EXTERN_CVAR(message_showpickups)
EXTERN_CVAR(message_showobituaries)

static unsigned int TickerAt, TickerMax;
static const char *TickerLabel;

#define NUMNOTIFIES 4

int V_TextScaleXAmount();
int V_TextScaleYAmount();

static struct NotifyText
{
	int timeout;
	int printlevel;
	byte text[256];
} NotifyStrings[NUMNOTIFIES];

// Default Printlevel
#define PRINTLEVELS 9 //(5 + 3)
int PrintColors[PRINTLEVELS] = 
{	CR_RED,		// Pickup
	CR_GOLD,	// Obituaries
	CR_GRAY,	// Messages
	CR_GREEN,	// Chat 
	CR_GREEN,	// Team chat

	CR_GOLD,	// Server chat
	CR_YELLOW,	// Warning messages
	CR_RED,		// Critical messages
	CR_GOLD,	// Centered Messages
};	


// ============================================================================
//
// ConsoleLine class interface
//
// Stores the console's output text.
//
// ============================================================================

class ConsoleLine
{
public:
	ConsoleLine();
	ConsoleLine(const std::string& _text, const std::string& _color_code = "\034-",	// TEXTCOLOR_ESCAPE
			int _print_level = PRINT_HIGH);

	void join(const ConsoleLine& other);
	ConsoleLine split(size_t max_width);
	bool expired() const;

	std::string		text;
	std::string		color_code;
	bool			wrapped;
	int				print_level;
	int				timeout;
};


// ============================================================================
//
// ConsoleLine class implementation
//
// ============================================================================

ConsoleLine::ConsoleLine() :
	color_code("\034-"), wrapped(false), print_level(PRINT_HIGH),	// TEXTCOLOR_ESCAPE
	timeout(gametic + con_notifytime.asInt() * TICRATE)
{ }

ConsoleLine::ConsoleLine(const std::string& _text, const std::string& _color_code,
			int _print_level) :
	text(_text), color_code(_color_code), wrapped(false), print_level(_print_level),
	timeout(gametic + con_notifytime.asInt() * TICRATE)
{ }


//
// ConsoleLine::join
//
// Appends the other console line to this line.
//
void ConsoleLine::join(const ConsoleLine& other)
{
	text.append(other.text);
	wrapped = other.wrapped;
}

//
// ConsoleLine::split
//
// Splits this line into a second line with this line having a maximum
// pixel width of max_width.
//
ConsoleLine ConsoleLine::split(size_t max_width)
{
	char wrapped_color_code[4] = { 0 };
	size_t width = 0;
	size_t break_pos = 0;

	const char* s = text.c_str();
	while (s)
	{
		if (s[0] == TEXTCOLOR_ESCAPE && s[1] != '\0')
		{
			strncpy(wrapped_color_code, s, 2);
			s += 2;
			continue;
		}

		if (*s == ' ' || *s == '\n' || *s == '\t')
			break_pos = s - text.c_str();
		else if (*s == '-')
			break_pos = s + 1 - text.c_str();

		const size_t character_width = 8;
		if (width + character_width > max_width)
		{
			// Is this word is too long to fit on the line?
			// Breaking it here is the only option.
			if (break_pos == 0)
				break_pos = s - text.c_str();

			ConsoleLine new_line(text.substr(break_pos, std::string::npos));
			TrimStringStart(new_line.text);

			// continue the color markup onto the next line
			if (*wrapped_color_code)
				new_line.text = wrapped_color_code + new_line.text;

			new_line.wrapped = wrapped;
			new_line.print_level = print_level;
			new_line.timeout = timeout;
			new_line.color_code = color_code;

			text = text.substr(0, break_pos);
			wrapped = true;

			return new_line;
		}

		s++;
		width += character_width;
	}

	// didn't have to wrap
	return ConsoleLine();
}

bool ConsoleLine::expired() const
{
	return gametic > timeout;
}



// ============================================================================
//
// ConsoleCommandLine class interface
//
// Stores the text and cursor position for the console's command line. Also
// provides basic functions to manipulate the cursor or insert and remove
// characters.
//
// ============================================================================

class ConsoleCommandLine
{
public:
	ConsoleCommandLine() : cursor_position(0), scrolled_columns(0) { }

	void clear();
	void moveCursorLeft();
	void moveCursorRight();
	void moveCursorLeftWord();
	void moveCursorRightWord();
	void moveCursorHome();
	void moveCursorEnd();

	void insertCharacter(char c);
	void insertString(const std::string& str);
	void replaceString(const std::string& str);
	void deleteCharacter();
	void backspace();

	std::string		text;
	size_t			cursor_position;
	size_t			scrolled_columns;

private:
	void doScrolling();
};


// ============================================================================
//
// ConsoleCommandLine class implementation
//
// ============================================================================

//
// ConsoleCommandLine::doScrolling
//
// Helper function that recalculates the scrolled_columns variable after
// the cursor has been moved or text has been inserted or deleted.
//
void ConsoleCommandLine::doScrolling()
{
	int n = scrolled_columns;

	// Start of visible line is beyond end of line
	if (scrolled_columns >= text.length())
		n = cursor_position - ConCols + 2;

	// The cursor_position is beyond the visible part of the line
	if (((int)cursor_position - (int)scrolled_columns) >= (int)(ConCols - 2))
		n = cursor_position - ConCols + 2;

	// The cursor_positionor is in front of the visible part of the line
	if (scrolled_columns > cursor_position)
		n = cursor_position;

	if (n < 0)
		n = 0;

	scrolled_columns = n;
}

void ConsoleCommandLine::clear()
{
	text.clear();
	cursor_position = 0;
	scrolled_columns = 0;
}

void ConsoleCommandLine::moveCursorLeft()
{
	if (cursor_position > 0)
		cursor_position--;
	doScrolling();
}

void ConsoleCommandLine::moveCursorRight()
{
	if (cursor_position < text.length())
		cursor_position++;
	doScrolling();
}

void ConsoleCommandLine::moveCursorLeftWord()
{
	if (cursor_position > 0)
		cursor_position--;

	const char* str = text.c_str();
	while (cursor_position > 0 && str[cursor_position - 1] != ' ')
		cursor_position--;

	doScrolling();
}

void ConsoleCommandLine::moveCursorRightWord()
{
	// find first non-space character after the next space(s)
	cursor_position = text.find_first_not_of(' ', text.find_first_of(' ', cursor_position));

	if (cursor_position == std::string::npos)
		cursor_position = text.length();
	
	doScrolling();
}

void ConsoleCommandLine::moveCursorHome()
{
	cursor_position = 0;
	doScrolling();
}

void ConsoleCommandLine::moveCursorEnd()
{
	cursor_position = text.length();
	doScrolling();
}

void ConsoleCommandLine::insertCharacter(char c)
{
	text.insert(cursor_position, &c, 1);

	cursor_position++;
	doScrolling();
}

void ConsoleCommandLine::insertString(const std::string& str)
{
	text.insert(cursor_position, str);

	cursor_position += str.length();
	doScrolling();
}

void ConsoleCommandLine::replaceString(const std::string& str)
{
	text = str;

	cursor_position = str.length();
	doScrolling();
}

void ConsoleCommandLine::deleteCharacter()
{
	if (cursor_position < text.length())
	{
		text.erase(cursor_position, 1);
		doScrolling();
	}
}

void ConsoleCommandLine::backspace()
{
	if (cursor_position > 0)
	{
		text.erase(cursor_position - 1, 1);
		moveCursorLeft();
	}
}


// ============================================================================
//
// ConsoleHistory class interface
//
// Stores a copy of each line of text entered on the command line and provides
// iteration functions to recall previous command lines entered. 
//
// ============================================================================

class ConsoleHistory
{
public:
	ConsoleHistory();

	void clear();

	void resetPosition();

	void addString(const std::string& str);
	const std::string& getString() const;

	void movePositionUp();
	void movePositionDown();

	void dump();

private:
	static const size_t MAX_HISTORY_ITEMS = 50;

	typedef std::list<std::string> ConsoleHistoryList;
	ConsoleHistoryList					history;

	ConsoleHistoryList::const_iterator	history_it;
};

// ============================================================================
//
// ConsoleHistory class implementation
//
// ============================================================================

ConsoleHistory::ConsoleHistory()
{
	resetPosition();
}

void ConsoleHistory::clear()
{
	history.clear();
	resetPosition();
}

void ConsoleHistory::resetPosition()
{
	history_it = history.end();
}

void ConsoleHistory::addString(const std::string& str)
{
	// only add the string if it's different from the most recent in history
	if (!str.empty() && (history.empty() || str.compare(history.back()) != 0))
	{
		while (history.size() >= MAX_HISTORY_ITEMS)
			history.pop_front();
		history.push_back(str);
	}
}

const std::string& ConsoleHistory::getString() const
{
	if (history_it == history.end())
	{
		static std::string blank_string;
		return blank_string;
	}

	return *history_it;
}

void ConsoleHistory::movePositionUp()
{
	if (history_it != history.begin())
		--history_it;
}

void ConsoleHistory::movePositionDown()
{
	if (history_it != history.end())
		++history_it;
}
		
void ConsoleHistory::dump()
{
	for (ConsoleHistoryList::const_iterator it = history.begin(); it != history.end(); ++it)
		Printf(PRINT_HIGH, "   %s\n", it->c_str());
}

class ConsoleCompletions
{
	std::vector<std::string> _completions;
	size_t _maxlen;

  public:
	void add(const std::string& completion)
	{
		_completions.push_back(completion);
		if (_maxlen < completion.length())
			_maxlen = completion.length();
	}

	const std::string& at(size_t index) const
	{
		return _completions.at(index);
	}

	void clear()
	{
		_completions.clear();
		_maxlen = 0;
	}

	bool empty() const
	{
		return _completions.empty();
	}

	//
	// Get longest common substring of completions.
	//
	std::string getCommon() const
	{
		bool diff = false;
		std::string common;

		for (size_t index = 0;; index++)
		{
			char compare = '\xFF';

			std::vector<std::string>::const_iterator it = _completions.begin();
			for (; it != _completions.end(); ++it)
			{
				if (index >= it->length())
				{
					// End of string, this is an implicit failed match.
					diff = true;
					break;
				}

				if (compare == '\xFF')
				{
					// Set character to compare against.
					compare = it->at(index);
				}
				else if (compare != it->at(index))
				{
					// Found a different character.
					diff = true;
					break;
				}
			}

			// If we found a different character, break, otherwise add it
			// to the string we're going to return and try the next index.
			if (diff == true)
				break;
			else
				common += compare;
		}

		return common;
	}

	size_t getMaxLen() const
	{
		return _maxlen;
	}

	size_t size() const
	{
		return _completions.size();
	}
};

// ============================================================================
//
// Console object definitions
//
// ============================================================================

typedef std::list<ConsoleLine> ConsoleLineList;
static ConsoleLineList Lines;

static ConsoleCommandLine CmdLine;

static ConsoleHistory History;

static ConsoleCompletions CmdCompletions;

/****** Tab completion code ******/

typedef std::map<std::string, size_t> tabcommand_map_t; // name, use count
tabcommand_map_t& TabCommands()
{
	static tabcommand_map_t _TabCommands;
	return _TabCommands;
}

void C_AddTabCommand(const char* name)
{
	tabcommand_map_t::iterator it = TabCommands().find(StdStringToLower(name));

	if (it != TabCommands().end())
		TabCommands()[name]++;
	else
		TabCommands()[name] = 1;
}

void C_RemoveTabCommand(const char* name)
{
	tabcommand_map_t::iterator it = TabCommands().find(StdStringToLower(name));

	if (it != TabCommands().end())
		if (!--it->second)
			TabCommands().erase(it);
}

//
// Start tab cycling.
//
// Note that this initial state points to the front and back of the completions
// index, which is a unique state that is not possible to get into after you
// start hitting tab.
//
static void TabCycleStart()
{
	::ShouldTabCycle = true;
	::NextCycleIndex = 0;
	::PrevCycleIndex = CmdCompletions.size() - 1;
}

//
// Given an specific completion index, determine the next and previous index.
//
static void TabCycleSet(size_t index)
{
	// Find the next index.
	if (index >= CmdCompletions.size() - 1)
		::NextCycleIndex = 0;
	else
		::NextCycleIndex = index + 1;

	// Find the previous index.
	if (index == 0)
		::PrevCycleIndex = CmdCompletions.size() - 1;
	else
		::PrevCycleIndex = index - 1;
}

//
// Get out of the tab cycle state.
//
static void TabCycleClear()
{
	::ShouldTabCycle = false;
	::NextCycleIndex = 0;
	::PrevCycleIndex = 0;
}

enum TabCompleteDirection
{
	TAB_COMPLETE_FORWARD,
	TAB_COMPLETE_BACKWARD
};

//
// Handle tab-completion and cycling.
//
static void TabComplete(TabCompleteDirection dir)
{
	// Pressing tab twice in a row starts cycling the completion.
	if (::ShouldTabCycle && ::CmdCompletions.size() > 0)
	{
		if (dir == TAB_COMPLETE_FORWARD)
		{
			size_t index = ::NextCycleIndex;
			::CmdLine.replaceString(::CmdCompletions.at(index));
			TabCycleSet(index);
		}
		else if (dir == TAB_COMPLETE_BACKWARD)
		{
			size_t index = ::PrevCycleIndex;
			::CmdLine.replaceString(::CmdCompletions.at(index));
			TabCycleSet(index);
		}
		return;
	}

	// Clear the completions.
	::CmdCompletions.clear();

	// Figure out what we need to autocomplete.
	size_t tabStart = ::CmdLine.text.find_first_not_of(' ', 0);
	if (tabStart == std::string::npos)
		tabStart = 0;
	size_t tabEnd = ::CmdLine.text.find(' ', 0);
	if (tabEnd == std::string::npos)
		tabEnd = ::CmdLine.text.length();
	size_t tabLen = tabEnd - tabStart;

	// Don't complete if the cursor is past the command.
	if (::CmdLine.cursor_position >= tabEnd + 1)
		return;

	// Find all substrings.
	std::string sTabPos = StdStringToLower(::CmdLine.text.substr(tabStart, tabLen));
	const char* cTabPos = sTabPos.c_str();
	tabcommand_map_t::iterator it = TabCommands().lower_bound(sTabPos);
	for (; it != TabCommands().end(); ++it)
	{
		if (strncmp(cTabPos, it->first.c_str(), sTabPos.length()) == 0)
			::CmdCompletions.add(it->first.c_str());
	}

	if (::CmdCompletions.size() > 1)
	{
		// Get common substring of all completions.
		std::string common = ::CmdCompletions.getCommon();
		::CmdLine.replaceString(common);
	}
	else if (::CmdCompletions.size() == 1)
	{
		// We found a single completion, use it.
		::CmdLine.replaceString(::CmdCompletions.at(0));
		::CmdCompletions.clear();
	}

	// If we press tab twice, cycle the command.
	TabCycleStart();
}

static void setmsgcolor(int index, const char *color);

cvar_t msglevel("msg", "0", "", CVARTYPE_INT, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE);

CVAR_FUNC_IMPL(msg0color)
{
	setmsgcolor(0, var.cstring());
}

CVAR_FUNC_IMPL(msg1color)
{
	setmsgcolor(1, var.cstring());
}

CVAR_FUNC_IMPL(msg2color)
{
	setmsgcolor(2, var.cstring());
}

CVAR_FUNC_IMPL(msg3color)
{
	setmsgcolor(3, var.cstring());
}

CVAR_FUNC_IMPL(msg4color)
{
	setmsgcolor(4, var.cstring());
}

CVAR_FUNC_IMPL(msgmidcolor)
{
	setmsgcolor(PRINTLEVELS-1, var.cstring());
}

// NES - Activating this locks the scroll position in place when
//       scrolling up. Otherwise, any new messages will
//       automatically pull the console back to the bottom.
//
// con_scrlock 0 = All new lines bring scroll to the bottom.
// con_scrlock 1 = Only input commands bring scroll to the bottom.
// con_scrlock 2 = Nothing brings scroll to the bottom.
EXTERN_CVAR(con_scrlock)

//
// C_InitConCharsFont
//
// Loads the CONCHARS lump from disk and converts it to the format used by
// the console for printing text.
//
void C_InitConCharsFont()
{
	static palindex_t transcolor = 0xF7;

	// Load the CONCHARS lump and convert it from patch_t format
	// to a raw linear byte buffer with a background color of 'transcolor'
	IWindowSurface* temp_surface = I_AllocateSurface(128, 128, 8);
	temp_surface->lock();

	// fill with color 'transcolor'
	for (int y = 0; y < 128; y++)
		memset(temp_surface->getBuffer() + y * temp_surface->getPitchInPixels(), transcolor, 128);

	// paste the patch into the linear byte bufer
	DCanvas* canvas = temp_surface->getDefaultCanvas();
	canvas->DrawPatch(W_CachePatch("CONCHARS"), 0, 0);

	ConChars = new byte[256*8*8*2];
	byte* dest = ConChars;	

	for (int y = 0; y < 16; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			const byte* source = temp_surface->getBuffer() + x * 8 + (y * 8 * temp_surface->getPitch());
			for (int z = 0; z < 8; z++)
			{
				for (int a = 0; a < 8; a++)
				{
					byte val = source[a];
					if (val == transcolor)
					{
						dest[a] = 0x00;
						dest[a + 8] = 0xff;
					}
					else
					{
						dest[a] = val;
						dest[a + 8] = 0x00;
					}
				}

				dest += 16;
				source += temp_surface->getPitch();
			}
		}
	}

	temp_surface->unlock();
	I_FreeSurface(temp_surface);
}


//
// C_ShutdownConCharsFont
//
void STACK_ARGS C_ShutdownConCharsFont()
{
	delete [] ConChars;
	ConChars = NULL;
}

//
// C_StringWidth
//
// Determines the width of a string in the CONCHARS font
//
static int C_StringWidth(const char* str)
{
	int width = 0;
	
	while (*str)
	{
		// skip over color markup escape codes
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		width += 8; 
		str++;
	}

	return width;
}

void C_ClearCommand()
{
	::CmdLine.clear();
	::CmdCompletions.clear();
}

//
// C_InitConsoleBackground
//
// Allocates background surface and draws the appropriate background patch
//
void C_InitConsoleBackground()
{
	const patch_t* bg_patch = W_CachePatch(W_GetNumForName("CONBACK"));

	background_surface = I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
	background_surface->lock();
	background_surface->getDefaultCanvas()->DrawPatch(bg_patch, 0, 0);
	background_surface->unlock();
}


//
// C_ShutdownConsoleBackground
//
// Frees the background_surface
//
void STACK_ARGS C_ShutdownConsoleBackground()
{
	I_FreeSurface(background_surface);
}


//
// C_ShutdownConsole
//
void STACK_ARGS C_ShutdownConsole()
{
	Lines.clear();
	History.clear();
	CmdLine.clear();
	CmdCompletions.clear();
}


//
// C_InitConsole
//
void C_InitConsole()
{
	C_NewModeAdjust();
	C_FullConsole();
}


//
// C_SetConsoleDimensions
//
static void C_SetConsoleDimensions(int width, int height)
{
	static int old_width = -1, old_height = -1;

	if (width != old_width || height != old_height)
	{
		ConCols = width / 8 - 2;
		PhysRows = height / 8;

		// ConCols has changed so any lines of text that are currently wrapped
		// need to be adjusted.
		for (ConsoleLineList::iterator current_line_it = Lines.begin();
			current_line_it != Lines.end(); ++current_line_it)
		{
			if (current_line_it->wrapped)
			{
				// The current line has wrapped around to the next line. Append the
				// next line to the current line for now.

				ConsoleLineList::iterator next_line_it = current_line_it;
				++next_line_it;

				if (next_line_it != Lines.end())
				{
					current_line_it->join(*next_line_it);
					Lines.erase(next_line_it);
				}
			}

			if ((unsigned)C_StringWidth(current_line_it->text.c_str()) > ConCols*8)
			{
				ConsoleLineList::iterator next_line_it = current_line_it;
				++next_line_it;

				ConsoleLine new_line = current_line_it->split(ConCols*8);
				Lines.insert(next_line_it, new_line);
			}
		}

		old_width = width;
		old_height = height;
	}
}

static void setmsgcolor(int index, const char *color)
{
	int i = atoi(color);
	if (i < 0 || i >= NUM_TEXT_COLORS)
		i = 0;
	PrintColors[index] = i;
}


//
// C_AddNotifyString
//
// Prioritise messages on top of screen
// Break up the lines so that they wrap around the screen boundary
//
void C_AddNotifyString(int printlevel, const char* color_code, const char* source)
{
	static enum
	{
		NEWLINE,
		APPENDLINE,
		REPLACELINE
	} addtype = NEWLINE;

	char work[MAX_LINE_LENGTH];
	brokenlines_t *lines;

	int len = strlen(source);

	if ((printlevel != 128 && !show_messages) || len == 0 ||
		(gamestate != GS_LEVEL && gamestate != GS_INTERMISSION) )
		return;

	// Do not display filtered chat messages
	if (printlevel == PRINT_FILTERCHAT)
		return;

	int width = I_GetSurfaceWidth() / V_TextScaleXAmount();

	if (addtype == APPENDLINE && NotifyStrings[NUMNOTIFIES-1].printlevel == printlevel)
	{
		sprintf(work, "%s%s", NotifyStrings[NUMNOTIFIES-1].text, source);
		lines = V_BreakLines(width, work);
	}
	else
	{
		lines = V_BreakLines(width, source);
		addtype = (addtype == APPENDLINE) ? NEWLINE : addtype;
	}

	if (!lines)
		return;

	for (int i = 0; lines[i].width != -1; i++)
	{
		if (addtype == NEWLINE)
			memmove(&NotifyStrings[0], &NotifyStrings[1], sizeof(struct NotifyText) * (NUMNOTIFIES-1));
		strcpy((char *)NotifyStrings[NUMNOTIFIES-1].text, lines[i].string);
		NotifyStrings[NUMNOTIFIES-1].timeout = gametic + (con_notifytime.asInt() * TICRATE);
		NotifyStrings[NUMNOTIFIES-1].printlevel = printlevel;
		addtype = NEWLINE;
	}

	V_FreeBrokenLines(lines);

	switch (source[len-1])
	{
	case '\r':
		addtype = REPLACELINE;
		break;
	case '\n':
		addtype = NEWLINE;
		break;
	default:
		addtype = APPENDLINE;
		break;
	}
}

//
// C_PrintStringStdOut
//
// Prints the given string to stdout, stripping away any color markup
// escape codes.
//
static int C_PrintStringStdOut(const char* str)
{
	std::string sanitized_str(str);
	StripColorCodes(sanitized_str);

	printf("%s", sanitized_str.c_str());
	fflush(stdout);

	return sanitized_str.length();
}


//
// C_PrintString
//
// Provide our own Printf() that is sensitive of the
// console status (in or out of game).
// 
static int C_PrintString(int printlevel, const char* color_code, const char* outline)
{
	if (I_VideoInitialized() && !midprinting)
	{
		const bool noPickups = printlevel == PRINT_PICKUP && !::message_showpickups;
		const bool noObits = printlevel == PRINT_OBITUARY && !::message_showobituaries;

		if (!noPickups && !noObits)
			C_AddNotifyString(printlevel, color_code, outline);
	}

	// Revert filtered chat to a normal chat to display to the console
	if (printlevel == PRINT_FILTERCHAT)
		printlevel = PRINT_CHAT;

	const char* line_start = outline;
	const char* line_end = line_start;

	// [SL] the user's message color preference overrides the given color_code
	// ...unless it's supposed to be formatted bold.
	static char printlevel_color_code[3];

	if (color_code && color_code[1] != '+' && printlevel >= 0 && printlevel < PRINTLEVELS)
	{
		snprintf(printlevel_color_code, sizeof(printlevel_color_code), "\034%c", 'a' + PrintColors[printlevel]);
		color_code = printlevel_color_code;
	}

	while (*line_start)
	{
		// Find the next line-breaking character (\n or \0) and set
		// line_end to point to it.
		line_end = line_start;
		while (*line_end != '\n' && *line_end != '\0')
			 line_end++;

		const size_t len = line_end - line_start;

		char str[MAX_LINE_LENGTH + 1];
		strncpy(str, line_start, len);
		str[len] = '\0';

		bool wrap_new_line = *line_end != '\n';
		ConsoleLine new_line(str, color_code, wrap_new_line); 
		
		// Add a new line to ConsoleLineList if the last line in ConsoleLineList
		// ends in \n, or add onto the last line if does not.
		if (!Lines.empty() && Lines.back().wrapped)
			Lines.back().join(new_line);
		else
			Lines.push_back(new_line);
 
		// Wrap the current line if it's too long.
		unsigned int line_width = C_StringWidth(Lines.back().text.c_str());
		if (line_width > ConCols*8)
		{
			new_line = Lines.back().split(ConCols*8);
			Lines.push_back(new_line);
		}
		
		if (con_scrlock > 0 && RowAdjust != 0)
			RowAdjust++;
		else
			RowAdjust = 0;

		line_start = line_end;
		if (*line_end == '\n')
			line_start++;
	}

	return strlen(outline);
}

static int BasePrint(const int printlevel, const char* color_code, const std::string& str)
{
	extern BOOL gameisdead;
	if (gameisdead)
		return 0;

	std::string newStr = str;

	// denis - 0x07 is a system beep, which can DoS the console (lol)
	// ToDo: there may be more characters not allowed on a consoleprint, 
	// maybe restrict a few ASCII stuff later on ?
	for (int i = 0; i < newStr.length(); i++)
	{
		if (newStr[i] == 0x07)
			newStr[i] = '.';
	}

	// Prevents writing a whole lot of new lines to the log file
	if (gamestate != GS_FORCEWIPE)
	{
		std::string logStr = newStr;

		// [Nes] - Horizontal line won't show up as-is in the logfile.
		for (int i = 0; i < logStr.length(); i++)
		{
			if (logStr[i] == '\35' || logStr[i] == '\36' || logStr[i] == '\37')
				logStr[i] = '=';
		}

		// Up the row buffer for the console.
		// This is incremented here so that any time we
		// print something we know about it.  This feels pretty hacky!

		// We need to know if there were any new lines being printed
		// in our string.

		int newLineCount = std::count(logStr.begin(), logStr.end(), '\n');

		if (ConRows < (unsigned int)con_buffersize.asInt())
			ConRows += (newLineCount > 1) ? newLineCount + 1 : 1;
	}

	if (print_stdout && gamestate != GS_FORCEWIPE)
		C_PrintStringStdOut(newStr.c_str());

	if (!con_coloredmessages)
		StripColorCodes(newStr);

	C_PrintString(printlevel, color_code, newStr.c_str());

	// Once done, log 
	if (LOG.is_open())
	{
		// Strip if not already done
		if (con_coloredmessages)
			StripColorCodes(newStr);

		LOG << newStr;
		LOG.flush();
	}

#if defined (_WIN32) && defined(_DEBUG)
	// [AM] Since we don't have stdout/stderr in a non-console Win32 app,
	//      this outputs the string to the "Output" window.
	OutputDebugStringA(newStr.c_str());
#endif

	return newStr.length();
}

int Printf(fmt::CStringRef format, fmt::ArgList args)
{
	return BasePrint(PRINT_HIGH, TEXTCOLOR_NORMAL, fmt::sprintf(format, args));
}

int Printf(const int printlevel, fmt::CStringRef format, fmt::ArgList args)
{
	return BasePrint(printlevel, TEXTCOLOR_NORMAL, fmt::sprintf(format, args));
}

int Printf_Bold(fmt::CStringRef format, fmt::ArgList args)
{
	return BasePrint(PRINT_HIGH, TEXTCOLOR_BOLD, fmt::sprintf(format, args));
}

int DPrintf(fmt::CStringRef format, fmt::ArgList args)
{
	if (developer || devparm)
	{
		return BasePrint(PRINT_WARNING, TEXTCOLOR_NORMAL, fmt::sprintf(format, args));
	}

	return 0;
}

void C_FlushDisplay()
{
	for (int i = 0; i < NUMNOTIFIES; i++)
		NotifyStrings[i].timeout = 0;
}

void C_Ticker()
{
	int surface_height = I_GetSurfaceHeight();

	static int lasttic = 0;

	if (lasttic == 0)
		lasttic = gametic - 1;

	if (ConsoleState != c_up)
	{
		if (ScrollState == SCROLLUP)
		{
			if (KeysCtrl)
			{
				RowAdjust += 16;
				ScrollState = SCROLLNO;
			}
			else
				RowAdjust++;

			if (RowAdjust > ConRows - ConBottom / 8)
				RowAdjust = ConRows - ConBottom / 8;
		}
		else if (ScrollState == SCROLLDN)
		{
			if (KeysCtrl)
			{
				RowAdjust-=16;				
				ScrollState = SCROLLNO;
			} else 
				RowAdjust--;

			if (RowAdjust < 0)
				RowAdjust = 0;
		}

		if (ConsoleState == c_falling)
		{
			ConBottom += (gametic - lasttic) * (surface_height * 2 / 25);
			if (ConBottom >= surface_height / 2)
			{
				ConBottom = surface_height / 2;
				ConsoleState = c_down;
			}
		}
		else if (ConsoleState == c_fallfull)
		{
			ConBottom += (gametic - lasttic) * (surface_height * 2 / 15);
			if (ConBottom >= surface_height)
			{
				ConBottom = surface_height;
				ConsoleState = c_down;
			}
		}
		else if (ConsoleState == c_rising)
		{
			ConBottom -= (gametic - lasttic) * (surface_height * 2 / 25);
			if (ConBottom <= 0)
			{
				ConsoleState = c_up;
				ConBottom = 0;
			}
		}
		else if (ConsoleState == c_risefull)
		{
			ConBottom -= (gametic - lasttic) * (surface_height * 2 / 15);
			if (ConBottom <= 0)
			{
				ConsoleState = c_up;
				ConBottom = 0;
			}
		}

		if (RowAdjust + (ConBottom/8) + 1 > (unsigned int)con_buffersize.asInt())
			RowAdjust = con_buffersize.asInt() - ConBottom;
	}

	if (--CursorTicker <= 0)
	{
		cursoron ^= 1;
		CursorTicker = C_BLINKRATE;
	}

	lasttic = gametic;
}


//
// C_DrawNotifyText
//
// Prints the HUD notification messages to the top of the HUD. Each of these
// messages will be displayed for a fixed amount of time before being removed.
//
static void C_DrawNotifyText()
{
	if ((gamestate != GS_LEVEL && gamestate != GS_INTERMISSION) || menuactive)
		return;

	int ypos = 0;
	for (int i = 0; i < NUMNOTIFIES; i++)
	{
		if (NotifyStrings[i].timeout > gametic)
		{
			if (!show_messages && NotifyStrings[i].printlevel != 128)
				continue;

			int color;
			if (NotifyStrings[i].printlevel >= PRINTLEVELS)
				color = CR_RED;
			else
				color = PrintColors[NotifyStrings[i].printlevel];

			screen->DrawTextStretched(color, 0, ypos, NotifyStrings[i].text,
						V_TextScaleXAmount(), V_TextScaleYAmount());
			ypos += 8 * V_TextScaleYAmount();
		}
	}
}


void C_InitTicker(const char *label, unsigned int max)
{
	TickerMax = max;
	TickerLabel = label;
	TickerAt = 0;
}

void C_SetTicker(unsigned int at)
{
	TickerAt = at > TickerMax ? TickerMax : at;
}


//
// C_UseFullConsole
//
// Returns true if the console should be drawn fullscreen. This should be
// any time the client is not in a level, intermission or playing the demo loop.
static bool C_UseFullConsole()
{
	return (gamestate != GS_LEVEL && gamestate != GS_DEMOSCREEN && gamestate != GS_INTERMISSION);
}


//
// C_AdjustBottom
//
// Changes ConBottom based on the console's state. In a fullscreen console,
// ConBottom should be the bottom of the screen. Otherwise, it should be
// the middle of the screen.
//
void C_AdjustBottom()
{
	int surface_height = I_GetSurfaceHeight();

	if (C_UseFullConsole())
		ConBottom = surface_height; 
	else if (ConsoleState == c_up)
		ConBottom = 0;
	else if (ConsoleState == c_down || ConBottom > surface_height / 2)
		ConBottom = surface_height / 2;

	// don't adjust if the console is raising or lowering because C_Ticker
	// handles that already
}


//
// C_NewModeAdjust
//
// Reinitializes the console after a video mode change.
//
void C_NewModeAdjust()
{
	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

	if (I_VideoInitialized())
		C_SetConsoleDimensions(surface_width, surface_height);
	else
		C_SetConsoleDimensions(80 * 8, 25 * 8);

	// clear HUD notify text
	C_FlushDisplay();

	C_AdjustBottom();
}


//
//	C_FullConsole
//
void C_FullConsole()
{
	// SoM: disconnect effect.
	if ((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && ConsoleState == c_up && !menuactive)
		screen->Dim(0, 0, I_GetSurfaceWidth(), I_GetSurfaceHeight());

	ConsoleState = c_down;

	TabCycleClear();

	C_AdjustBottom();
}


//
// C_HideConsole
//
// Sets the console's state to hidden.
//
void C_HideConsole()
{
	ConsoleState = c_up;
	ConBottom = 0;
	CmdLine.clear();
	CmdCompletions.clear();
	History.resetPosition();
}


//
// C_ToggleConsole
//
// Toggles the console's state (if possible). This has no effect in fullscreen
// console mode.
//
void C_ToggleConsole()
{
	if (C_UseFullConsole())
		return;

	bool bring_console_down =
				(ConsoleState == c_up || ConsoleState == c_rising || ConsoleState == c_risefull);

	if (bring_console_down)
	{
		if (C_UseFullConsole())
			ConsoleState = c_fallfull;
		else
			ConsoleState = c_falling;

		TabCycleClear();
	}
	else
	{
		if (ConBottom == I_GetSurfaceHeight())
			ConsoleState = c_risefull;
		else
			ConsoleState = c_rising;

		C_FlushDisplay();
	}

	CmdLine.clear();
	CmdCompletions.clear();
	History.resetPosition();
}


//
// C_DrawConsole
//
void C_DrawConsole()
{
	IWindowSurface* primary_surface = I_GetPrimarySurface();
	int primary_surface_width = primary_surface->getWidth();
	int primary_surface_height = primary_surface->getHeight();

	int left = 8;
	int lines = (ConBottom - 12) / 8;

	int offset;
	if (lines * 8 > ConBottom - 16)
		offset = -16;
	else
		offset = -12;

	if (ConsoleState == c_up || ConBottom == 0)
	{
		C_DrawNotifyText();
		return;
	}

	if (!C_UseFullConsole())
	{
		// Non-fullscreen console. Overlay a translucent background.
		screen->Dim(0, 0, primary_surface_width, ConBottom);
	}
	else if (::background_surface != NULL)
	{
		// Fullscreen console. Blit the image in the center of a black background.
		screen->Clear(0, 0, primary_surface_width, primary_surface_height, argb_t(0, 0, 0));

		int x = (primary_surface_width - background_surface->getWidth()) / 2;
		int y = (primary_surface_height - background_surface->getHeight()) / 2;

		background_surface->lock();

		primary_surface->blit(background_surface, 0, 0,
				background_surface->getWidth(), background_surface->getHeight(),
				x, y, background_surface->getWidth(), background_surface->getHeight());

		background_surface->unlock();
	}

	if (ConBottom >= 12)
	{
		const char* version = NiceVersion();

		// print the Odamex version in gold in the bottom right corner of console
		screen->PrintStr(primary_surface_width - 8 - C_StringWidth(version),
		                 ConBottom - 12, version, CR_ORANGE);

		// Amount of space remaining.
		int remain = primary_surface_width - 16 - C_StringWidth(version);

		if (CL_IsDownloading())
		{
			// Use the remaining space for a download bar.
			size_t chars = remain / C_StringWidth(" ");
			std::string download;

			// Stamp out the text bits.
			std::string filename = CL_DownloadFilename();
			if (filename.empty())
				filename = "...";
			OTransferProgress progress = CL_DownloadProgress();
			std::string dlnow;
			StrFormatBytes(dlnow, progress.dlnow);
			std::string dltotal;
			StrFormatBytes(dltotal, progress.dltotal);
			StrFormat(download, "%s: %s/%s", filename.c_str(), dlnow.c_str(),
			          dltotal.c_str());

			// Avoid divide by zero.
			if (progress.dltotal == 0)
				progress.dltotal = 1;

			// Stamp out the bar...if we have enough room - if we at tiny
			// resolutions we may not.
			size_t dltxtlen = download.length();
			ptrdiff_t barchars = chars - dltxtlen;

			if (barchars >= 2)
			{
				download.resize(chars);
				for (size_t i = 0; i < barchars; i++)
				{
					char ch = '\30'; // empty middle
					if (i == 0)
						ch = '\27'; // empty left
					else if (i == barchars - 1)
						ch = '\31'; // empty right

					double barpct = i / (double)barchars;
					double dlpct = progress.dlnow / (double)progress.dltotal;

					if (dlpct > barpct)
						ch += 3; // full bar

					download.at(i + dltxtlen) = ch;
				}
			}

			// Draw the thing.
			screen->PrintStr(left + 2, ConBottom - 12, download.c_str(), CR_GREEN);
		}

		if (TickerMax)
		{
			char tickstr[256];
			unsigned int i, tickend = ConCols - primary_surface_width / 90 - 6;
			unsigned int tickbegin = 0;

			if (TickerLabel)
			{
				tickbegin = strlen(TickerLabel) + 2;
				tickend -= tickbegin;
				sprintf(tickstr, "%s: ", TickerLabel);
			}
			if (tickend > 256 - 8)
				tickend = 256 - 8;
			tickstr[tickbegin] = -128;
			memset(tickstr + tickbegin + 1, 0x81, tickend - tickbegin);
			tickstr[tickend + 1] = -126;
			tickstr[tickend + 2] = ' ';
			i = tickbegin + 1 + (TickerAt * (tickend - tickbegin - 1)) / TickerMax;
			if (i > tickend)
				i = tickend;
			tickstr[i] = -125;
			sprintf(tickstr + tickend + 3, "%u%%", (TickerAt * 100) / TickerMax);
			screen->PrintStr(8, ConBottom - 12, tickstr);
		}
	}

	if (menuactive)
		return;

	if (lines > 0)
	{
		// First draw any completions, if we have any.
		if (!::CmdCompletions.empty())
		{
			// True if we have too many completions to render all of them.
			bool cOverflow = false;

			// We want at least 8-space tabs.
			size_t cTabLen = (::CmdCompletions.getMaxLen() + 1);
			if (cTabLen < 8)
				cTabLen = 8;

			// How many columns can we fit on the screen at one time?
			size_t cColumns = ::ConCols / cTabLen;
			if (cColumns == 0)
				cColumns += 1;

			// Given the number of columns, how many lines do we need?
			size_t cLines = ::CmdCompletions.size() / cColumns;
			if (::CmdCompletions.size() % cColumns != 0)
				cLines += 1;

			// Currently we cap the number of completion lines to 5
			if (cLines > 5)
			{
				cLines = 5;
				cOverflow = true;
			}

			// Offset our standard console printing.
			if (cOverflow)
				lines -= cLines + 1;
			else
				lines -= cLines;

			static char rowstring[MAX_LINE_LENGTH];

			// Completions are rendered top to bottom in columns like a
			// backwards "N".
			for (size_t l = 0; l < cLines; l++)
			{
				// Prepare a row string to copy completions into.
				memset(rowstring, ' ', ARRAY_LENGTH(rowstring));
				unsigned int col = 0;

				for (size_t c = 0; c < cColumns; c++)
				{
					// Turn our current line/column into an index.
					size_t index = (c * cLines) + l;
					if (index >= ::CmdCompletions.size())
					{
						rowstring[col] = '\0';
						break;
					}

					// Copy our completion into the row.
					const std::string& str = ::CmdCompletions.at(index);
					memcpy(&rowstring[col], str.c_str(), str.length());
					col += cTabLen;

					if (c + 1 == cColumns)
						rowstring[col - 1] = '\0';
					else
						rowstring[col - 1] = ' ';
				}

				screen->PrintStr(left, offset + (lines + l + 1) * 8, rowstring,
				                 CR_YELLOW);
			}

			// Render an overflow message if necessary.
			if (cOverflow)
			{
				snprintf(rowstring, ARRAY_LENGTH(rowstring), "...and %lu more...",
				         ::CmdCompletions.size() - (cLines * cColumns));
				screen->PrintStr(left, offset + (lines + cLines + 1) * 8, rowstring,
				                 CR_YELLOW);
			}
		}

		// find the ConsoleLine that will be printed to bottom of the console
		ConsoleLineList::reverse_iterator current_line_it = Lines.rbegin();
		for (unsigned i = 0; i < RowAdjust && current_line_it != Lines.rend(); i++)
			++current_line_it;
	
		// print as many ConsoleLines as will fit in the screen, starting at the bottom
		for (; lines > 1 && current_line_it != Lines.rend(); lines--, ++current_line_it)
		{
			const char* str = current_line_it->text.c_str();
			const char* color_code = current_line_it->color_code.c_str();
			int color = color_code[0] != '\0' ? V_GetTextColor(color_code) : CR_GRAY;
			screen->PrintStr(left, offset + lines * 8, str, color);
		}

		if (ConBottom >= 20)
		{
			screen->PrintStr(left, ConBottom - 20, "]", CR_TAN);

			size_t cmdline_len = std::min<size_t>(CmdLine.text.length() - CmdLine.scrolled_columns, ConCols - 1);
			if (cmdline_len)
			{
				char str[MAX_LINE_LENGTH];
				strncpy(str, CmdLine.text.c_str() + CmdLine.scrolled_columns, cmdline_len);
				str[cmdline_len] = '\0';
				bool use_color_codes = false;
				screen->PrintStr(left + 8, ConBottom - 20, str, CR_GRAY, use_color_codes);
			}

			if (cursoron)
			{
				const char str[] = "_";
				size_t cursor_offset = CmdLine.cursor_position - CmdLine.scrolled_columns;
				screen->PrintStr(left + 8 + 8 * cursor_offset, ConBottom - 20, str, CR_TAN);
			}

			if (RowAdjust && ConBottom >= 28)
			{
				// Indicate that the view has been scrolled up (10)
				// and if we can scroll no further (12)
				const char scrolled_up_str[] = "\012";		// 10 = \012 octal
				const char no_scroll_str[] = "\014";		// 12 = \014 octal
				const char* str = (RowAdjust + ConBottom/8 < ConRows) ? scrolled_up_str : no_scroll_str;
				screen->PrintStr(0, ConBottom - 28, str);
			}
		}
	}
}


static bool C_HandleKey(const event_t* ev)
{
	int ch = ev->data1;
	const char* cmd = Bindings.GetBind(ev->data1).c_str();

	if (Key_IsMenuKey(ch) || (cmd && stricmp(cmd, "toggleconsole") == 0))
	{
		// don't eat the Esc key if we're in full console
		// let it be processed elsewhere (to bring up the menu)
		if (C_UseFullConsole())
			return false;

		C_ToggleConsole();
		return true;
	}

#ifdef __SWITCH__
	if (ev->data1 == OKEY_JOY3)
{
	char oldtext[64], text[64], fulltext[65];		

		strcpy (text, CmdLine.text.c_str());

		// Initiate the console
		NX_SetKeyboard(text, 64);

		// No action if no text or same as earlier
		if (!strlen(text) || !strcmp(oldtext, text))
			return true;

				// add command line text to history
		History.addString(text);
		History.resetPosition();
	
		Printf(127, "]%s\n", text);
		AddCommandString(text);
		CmdLine.clear();
}
#endif

	switch (ch)
	{
	case OKEY_HOME:
		CmdLine.moveCursorHome();
		return true;
	case OKEY_END:
		CmdLine.moveCursorEnd();
		return true;		
	case OKEY_BACKSPACE:
		CmdLine.backspace();
		TabCycleClear();
		return true;
	case OKEY_DEL:
		CmdLine.deleteCharacter();
		TabCycleClear();
		return true;
	case OKEY_LALT:
	case OKEY_RALT:
		// Do nothing
		return true;
	case OKEY_LCTRL:
	case OKEY_RCTRL:
		KeysCtrl = true;
		return true;
	case OKEY_LSHIFT:
	case OKEY_RSHIFT:
		// SHIFT was pressed
		KeysShifted = true;
		return true;
	case OKEY_MOUSE3:
		// Paste from clipboard - add each character to command line
		CmdLine.insertString(I_GetClipboardText());
		CmdCompletions.clear();
		TabCycleClear();
		return true;
	}

// General keys used by all systems
	{
		if (Key_IsPageUpKey(ch))
		{
			if ((int)(ConRows) > (int)(ConBottom / 8))
			{
				if (KeysShifted)
					// Move to top of console buffer
					RowAdjust = ConRows - ConBottom / 8;
				else
					// Start scrolling console buffer up
					ScrollState = SCROLLUP;
			}
			return true;
		}
		else if (Key_IsPageDownKey(ch))
		{
			if (KeysShifted)
				// Move to bottom of console buffer
				RowAdjust = 0;
			else
				// Start scrolling console buffer down
				ScrollState = SCROLLDN;
			return true;
		}
		else if (Key_IsLeftKey(ch))
		{
			if (KeysCtrl)
				CmdLine.moveCursorLeftWord();
			else
				CmdLine.moveCursorLeft();
			return true;
		}
		else if (Key_IsRightKey(ch))
		{
			if (KeysCtrl)
				CmdLine.moveCursorRightWord();
			else
				CmdLine.moveCursorRight();
			return true;
		}
		else if (Key_IsUpKey(ch))
		{
			// Move to previous entry in the command history
			History.movePositionUp();
			CmdLine.clear();
			CmdLine.insertString(History.getString());
			CmdCompletions.clear();
			TabCycleClear();
			return true;
		}
		else if (Key_IsDownKey(ch))
		{
			// Move to next entry in the command history
			History.movePositionDown();
			CmdLine.clear();
			CmdLine.insertString(History.getString());
			CmdCompletions.clear();
			TabCycleClear();
			return true;
		}
		else if (Key_IsAcceptKey(ch))
		{
			// Execute command line (ENTER)
			if (con_scrlock == 1) // NES - If con_scrlock = 1, send console scroll to bottom.
				RowAdjust = 0;   // con_scrlock = 0 does it automatically.

			// add command line text to history
			History.addString(CmdLine.text);
			History.resetPosition();
		
			Printf(127, "]%s\n", CmdLine.text.c_str());
			AddCommandString(CmdLine.text.c_str());
			CmdLine.clear();
			CmdCompletions.clear();

			TabCycleClear();
			return true;
		}
		else if (Key_IsTabulationKey(ch))
		{
			if (::KeysShifted)
				TabComplete(TAB_COMPLETE_BACKWARD);
			else
				TabComplete(TAB_COMPLETE_FORWARD);
			return true;
		}
	}

	const char keytext = ev->data3;

	if (KeysCtrl)
	{
		// handle key combinations
		// NOTE: we have to use ev->data1 here instead of the
		// localization-aware ev->data3 since SDL2 does not send a SDL_TEXTINPUT
		// event when Ctrl is held down.

		// Go to beginning of line
 		if (tolower(ev->data1) == 'a')
			CmdLine.moveCursorHome();

		// Go to end of line
 		if (tolower(ev->data1) == 'e')
			CmdLine.moveCursorEnd();

		// Paste from clipboard - add each character to command line
 		if (tolower(ev->data1) == 'v')
		{
			CmdLine.insertString(I_GetClipboardText());
			TabCycleClear();
		}
		return true;
	}

	if (keytext)
	{	
		// Add keypress to command line
		CmdLine.insertCharacter(keytext);
		TabCycleClear();
	}
	return true;
}

BOOL C_Responder(event_t *ev)
{
	if (ConsoleState == c_up || ConsoleState == c_rising || ConsoleState == c_risefull || menuactive)
		return false;

	if (ev->type == ev_keyup)
	{
		// General Keys used by all systems
		if (Key_IsPageUpKey(ev->data1) || Key_IsPageDownKey(ev->data1))
		{
			ScrollState = SCROLLNO;
		}
		else
		{
			// Keyboard keys only
			switch (ev->data1)
			{
			case OKEY_LCTRL:
			case OKEY_RCTRL:
				KeysCtrl = false;
				break;
			case OKEY_LSHIFT:
			case OKEY_RSHIFT:
				KeysShifted = false;
				break;
			default:
				return false;
			}
		}
	}
	else if (ev->type == ev_keydown)
	{
		return C_HandleKey(ev);
	}

	if(ev->type == ev_mouse)
		return true;

	return false;
}

BEGIN_COMMAND(history)
{
	History.dump();
}
END_COMMAND(history)

BEGIN_COMMAND(clear)
{
	RowAdjust = 0;
	C_FlushDisplay();
	Lines.clear();
	History.resetPosition();
	CmdLine.clear();
	CmdCompletions.clear();
}
END_COMMAND(clear)

BEGIN_COMMAND(echo)
{
	if (argc > 1)
	{
		std::string str = C_ArgCombine(argc - 1, (const char **)(argv + 1));
		Printf(PRINT_HIGH, "%s\n", str.c_str());
	}
}
END_COMMAND(echo)


BEGIN_COMMAND(toggleconsole)
{
	C_ToggleConsole();
}
END_COMMAND(toggleconsole)

/* Printing in the middle of the screen */

static brokenlines_t *MidMsg = NULL;
static int MidTicker = 0, MidLines;
EXTERN_CVAR(con_midtime)

void C_MidPrint(const char *msg, player_t *p, int msgtime)
{
	unsigned int i;

	if (!msgtime)
		msgtime = con_midtime.asInt();

	if (MidMsg)
		V_FreeBrokenLines(MidMsg);

	if (msg)
	{
		midprinting = true;

		// [Russell] - convert textual "\n" into the binary representation for
		// line breaking
		std::string str = msg;

		for (size_t pos = str.find("\\n"); pos != std::string::npos; pos = str.find("\\n", pos))
		{
			str[pos] = '\n';
			str.erase(pos+1, 1);
		}

		char *newmsg = strdup(str.c_str());

		Printf(PRINT_HIGH, "%s\n", newmsg);
		midprinting = false;

		if ( (MidMsg = V_BreakLines(I_GetSurfaceWidth() / V_TextScaleXAmount(), (byte *)newmsg)) )
		{
			MidTicker = (int)(msgtime * TICRATE) + gametic;

			for (i = 0; MidMsg[i].width != -1; i++)
				;

			MidLines = i;
		}

		free(newmsg);
	}
	else
		MidMsg = NULL;
}

void C_DrawMid()
{
	if (MidMsg)
	{
		int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

		int xscale = V_TextScaleXAmount();
		int yscale = V_TextScaleYAmount();

		const int line_height = 8 * yscale;

		int bottom = R_StatusBarVisible()
			? ST_StatusBarY(surface_width, surface_height) : surface_height;

		int x = surface_width / 2;
		int y = (bottom - line_height * MidLines) / 2;

		for (int i = 0; i < MidLines; i++, y += line_height)
		{
			screen->DrawTextStretched(PrintColors[PRINTLEVELS-1],
					x - xscale * (MidMsg[i].width / 2),
					y, (byte *)MidMsg[i].string, xscale, yscale);
		}

		if (gametic >= MidTicker)
		{
			V_FreeBrokenLines(MidMsg);
			MidMsg = NULL;
		}
	}
}

static brokenlines_t *GameMsg = NULL;
static int GameTicker = 0, GameColor = CR_GREY, GameLines;

// [AM] This is literally the laziest excuse of a copy-paste job I have ever
//      done, but I really want CTF messages in time for 0.6.2.  Please replace
//      me eventually.  The two statics above, the two functions below, and
//      any direct calls to these two functions are all you need to remove.
void C_GMidPrint(const char* msg, int color, int msgtime)
{
	unsigned int i;

	if (!msgtime)
		msgtime = con_midtime.asInt();

	if (GameMsg)
		V_FreeBrokenLines(GameMsg);

	if (msg)
	{
		// [Russell] - convert textual "\n" into the binary representation for
		// line breaking
		std::string str = msg;

		for (size_t pos = str.find("\\n");pos != std::string::npos;pos = str.find("\\n", pos))
		{
			str[pos] = '\n';
			str.erase(pos + 1, 1);
		}

		char *newmsg = strdup(str.c_str());

		if ((GameMsg = V_BreakLines(I_GetSurfaceWidth() / V_TextScaleXAmount(), (byte *)newmsg)) )
		{
			GameTicker = (int)(msgtime * TICRATE) + gametic;

			for (i = 0;GameMsg[i].width != -1;i++)
				;

			GameLines = i;
		}

		GameColor = color;
		free(newmsg);
	}
	else
	{
		GameMsg = NULL;
		GameColor = CR_GREY;
	}
}

void C_DrawGMid()
{
	if (GameMsg)
	{
		int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

		int xscale = V_TextScaleXAmount();
		int yscale = V_TextScaleYAmount();

		const int line_height = 8 * yscale;

		int bottom = R_StatusBarVisible()
			? ST_StatusBarY(surface_width, surface_height) : surface_height;

		int x = surface_width / 2;
		int y = (bottom / 2 - line_height * GameLines) / 2;

		for (int i = 0; i < GameLines; i++, y += line_height)
		{
			screen->DrawTextStretched(GameColor,
					x - xscale * (GameMsg[i].width / 2),
					y, (byte*)GameMsg[i].string, xscale, yscale);
		}

		if (gametic >= GameTicker)
		{
			V_FreeBrokenLines(GameMsg);
			GameMsg = NULL;
		}
	}
}

// denis - moved secret discovery message to this function
EXTERN_CVAR(hud_revealsecrets)
void C_RevealSecret()
{
	if(!hud_revealsecrets || !G_IsCoopGame() || !show_messages) // [ML] 09/4/06: Check for hud_revealsecrets
		return;                      // NES - Also check for deathmatch

	C_MidPrint("A secret is revealed!");

	if (hud_revealsecrets == 1 || hud_revealsecrets == 3)
		S_Sound(CHAN_INTERFACE, "misc/secret", 1, ATTN_NONE);
}

VERSION_CONTROL (c_console_cpp, "$Id$")
