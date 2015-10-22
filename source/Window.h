#pragma once

#include "Defs.h"

class Window
{
public:
	Window();
	Window(const std::string& title, const int width, const int height);
	virtual ~Window();

			bool		init(const std::string& title,const int width,const int height);
			LRESULT		handle_message(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	inline	int			get_width() { return _width; }
	inline	int			get_height() { return _height; }

public:
	static	bool	do_message_pump();

public:
	HWND	_window;
protected:
	int		_width, _height;
};

