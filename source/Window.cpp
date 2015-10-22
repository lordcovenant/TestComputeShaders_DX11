#include "Window.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Window* window=(Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	return window->handle_message(hWnd, message, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Window::Window()
{
	_window = NULL;
	_width=_height = 0;
}

Window::Window(const std::wstring& title, const int width, const int height)
{
	init(title,width,height);
}


Window::~Window()
{
	if (_window)
	{
		DestroyWindow(_window);
		_window = NULL;
	}
}

bool Window::init(const std::wstring& title, const int width, const int height)
{
	WNDCLASSEX wc;

	// clear out the window class for use
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	// fill in the struct with the needed information
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
//	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"MainWindow";

	// register the window class
	RegisterClassEx(&wc);

	// create the window and use the result as the handle
	_window = CreateWindowEx(NULL,L"MainWindow",    // name of the window class
		title.c_str(),   // title of the window
		WS_OVERLAPPEDWINDOW,    // window style
		0,    // x-position of the window
		0,    // y-position of the window
		width,    // width of the window
		height,    // height of the window
		NULL,    // we have no parent window, NULL
		NULL,    // we aren't using menus, NULL
		GetModuleHandle(NULL),    // application handle
		NULL);    // used with multiple windows, NULL

	// display the window on the screen
	ShowWindow(_window, SW_SHOW);

	SetWindowLongPtr(_window, GWLP_USERDATA,(LONG)this);

	_width = width;
	_height = height;

	return true;
}

LRESULT Window::handle_message(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// sort through and find what code to run for the message given
	switch (message)
	{
		// this message is read when the window is closed
	case WM_DESTROY:
	{
		// close the application entirely
		PostQuitMessage(0);
		return 0;
	} break;
	}

	// Handle any messages the switch statement didn't
	return DefWindowProc(hWnd, message, wParam, lParam);
}

bool Window::do_message_pump()
{
	// wait for the next message in the queue, store the result in 'msg'
	MSG msg;
//	int n_msg = 0;

	while (PeekMessage(&msg,NULL, 0, 0, PM_REMOVE))
	{
		// translate keystroke messages into the right format
		TranslateMessage(&msg);

		// send the message to the WindowProc function
		DispatchMessage(&msg);

//		n_msg++;
	}

/*	if (n_msg > 0)
	{
		char buffer[512];
		swprintf_s((wchar_t*)&buffer, 512, L"Msg=%i\n", n_msg);

		OutputDebugString(buffer);
	}				*/

	return true;
}
