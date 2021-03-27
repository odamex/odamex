#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Window.H>

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Fl_Window* window = new Fl_Window(300, 180);
	{
		Fl_Box* box = new Fl_Box(20, 40, 260, 100, "Hello, World!");
		box->box(FL_UP_BOX);
		box->labelsize(36);
		box->labelfont(FL_BOLD + FL_ITALIC);
		box->labeltype(FL_SHADOW_LABEL);
	}
	window->end();
	window->show(0, NULL);
	return Fl::run();
}
