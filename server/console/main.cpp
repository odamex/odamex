#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Window.H>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

template <typename T>
class result_t
{
	T result;
	std::string error;

  public:
	result_t(T res) : result(res), error("") {}
	result_t(T res, const std::string& err) : result(res), error(err) {}

	bool ok() const
	{
		return error.empty();
	}
};

struct subproc_t
{
	HANDLE process;
	HANDLE thread;
	HANDLE stdIn;
	HANDLE stdOut;
	HANDLE stdErr;
};

result_t<bool> WinStartServer(subproc_t& subproc)
{
	// Command to run.
	TCHAR appName[128];
	ZeroMemory(appName, sizeof(appName));
	CopyMemory(appName, "Debug\\odasrv.exe", sizeof("Debug\\odasrv.exe"));

	// Full command to run, with arguments.
	TCHAR cmdLine[128];
	ZeroMemory(cmdLine, sizeof(cmdLine));
	cmdLine[0] = '\0';

	// Set up pipes
	SECURITY_ATTRIBUTES secAttrs;
	ZeroMemory(&secAttrs, sizeof(secAttrs));
	secAttrs.nLength = sizeof(secAttrs);
	secAttrs.bInheritHandle = TRUE;
	secAttrs.lpSecurityDescriptor = NULL;

	HANDLE stdInR, stdInW;
	if (!CreatePipe(&stdInR, &stdInW, &secAttrs, 0))
	{
		return result_t<bool>(false, "Could not initialize midiproc stdin");
	}

	if (!SetHandleInformation(stdInW, HANDLE_FLAG_INHERIT, 0))
	{
		return result_t<bool>(false, "Could not disinherit midiproc stdin");
	}

	HANDLE stdOutR, stdOutW;
	if (!CreatePipe(&stdOutR, &stdOutW, &secAttrs, 0))
	{
		return result_t<bool>(false, "Could not initialize midiproc stdout");
	}

	if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0))
	{
		return result_t<bool>(false, "Could not disinherit midiproc stdout");
	}

	HANDLE stdErrR, stdErrW;
	if (!CreatePipe(&stdErrR, &stdErrW, &secAttrs, 0))
	{
		return result_t<bool>(false, "Could not initialize midiproc stderr");
	}

	if (!SetHandleInformation(stdErrR, HANDLE_FLAG_INHERIT, 0))
	{
		return result_t<bool>(false, "Could not disinherit midiproc stderr");
	}

	// Startup info.
	STARTUPINFOA startupInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo); 
	startupInfo.dwFlags = STARTF_USESTDHANDLES;
	startupInfo.hStdInput = stdInR;
	startupInfo.hStdOutput = stdOutW;
	startupInfo.hStdError = stdErrW;

	// Process info.
	PROCESS_INFORMATION procInfo;
	ZeroMemory(&procInfo, sizeof(procInfo));

	if (!CreateProcess(appName, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL,
	                   &startupInfo, &procInfo))
	{
		DWORD error = GetLastError();
		return false;
	}

	subproc.process = procInfo.hProcess;
	subproc.thread = procInfo.hThread;
	subproc.stdIn = stdInR;
	subproc.stdOut = stdOutW;
	subproc.stdErr = stdErrW;

	return true;
}

#endif

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	subproc_t proc = {0};
	WinStartServer(proc);

	const int WIDTH = 640;
	const int HEIGHT = 480;
	const int INPUT_HEIGHT = 24;

	Fl_Window* window = new Fl_Window(WIDTH, HEIGHT);
	{
		Fl_Text_Display* text = new Fl_Text_Display(0, 0, WIDTH, HEIGHT - INPUT_HEIGHT);
		window->add(text);

		Fl_Input* input = new Fl_Input(0, HEIGHT - INPUT_HEIGHT, WIDTH, INPUT_HEIGHT);
		window->add(input);
	}
	window->end();
	window->show(0, NULL);
	return Fl::run();
}
