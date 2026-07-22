#include <array>
#include <string>

#include "linenoise.h"

#include <unistd.h>
#include <poll.h>

#include "odamex.h"
#include "c_dispatch.h"

bool HasInput()
{
    pollfd pfd{
        .fd = STDIN_FILENO,
        .events = POLLIN,
        .revents = 0
    };

    return poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN);
}

class ConsoleLineEditor
{
public:
	explicit ConsoleLineEditor(std::string prompt = "> ")
		: m_prompt(std::move(prompt))
	{
		linenoiseHistorySetMaxLen(50);
		Start();
	}

	~ConsoleLineEditor()
	{
		Stop();
	}

	ConsoleLineEditor(const ConsoleLineEditor&) = delete;
	ConsoleLineEditor& operator=(const ConsoleLineEditor&) = delete;

	std::string GetCommand()
	{
		if (!HasInput())
			return {};

		char* line = linenoiseEditFeed(&m_state);

		if (line == linenoiseEditMore)
			return {};

		Stop();

		std::string command;
		if (line)
		{
			command = line;
			linenoiseHistoryAdd(line);
		}

		free(line);

		Start();

		m_active = true;

		return command;
	}

	void Hide()
	{
		if (m_active)
			linenoiseHide(&m_state);
	}

	void Show()
	{
		if (m_active)
			linenoiseShow(&m_state);
	}

	void Start()
	{
		if (m_active)
			return;

		linenoiseEditStart(
			&m_state,
			m_buffer.data(),
			m_buffer.size(),
			m_prompt.c_str()
		);

		m_active = true;
	}

	void Stop()
	{
		if (!m_active)
			return;

		linenoiseEditStop(&m_state);
		m_active = false;
	}

	void Pause()
	{
		Hide();
	}

	void Resume()
	{
		linenoiseEditStop(&m_state, false);
		m_active = false;
		Start();
		Show();
	}

	void Clear()
	{
		if (m_active)
			linenoiseClearScreen(&m_state);
	}

	static ConsoleLineEditor& getInstance()
	{
		static ConsoleLineEditor editor;
		return editor;
	}

private:
	linenoiseState m_state{};
	std::array<char, 1024> m_buffer{};

	std::string m_prompt;
	bool m_active = false;
};

void linenoiseresume()
{
	ConsoleLineEditor::getInstance().Resume();
}

void linenoisepause()
{
	ConsoleLineEditor::getInstance().Pause();
}

void linenoisehide()
{
	ConsoleLineEditor::getInstance().Hide();
}

void linenoiseshow()
{
	ConsoleLineEditor::getInstance().Show();
}

std::string M_LineEditorInput()
{
	return ConsoleLineEditor::getInstance().GetCommand();
}

BEGIN_COMMAND(clear)
{
	ConsoleLineEditor::getInstance().Clear();
}
END_COMMAND(clear)