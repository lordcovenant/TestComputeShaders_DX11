#pragma once

#include "Defs.h"

class Window;

class Gfx
{
public:
	Gfx();
	virtual ~Gfx();

	bool init(Window* window);

	void	clear(const float r, const float g, const float b, const float a);
	void	swap_buffers();

public:
	ID3D11DeviceContext*	_context;
	ID3D11Device*			_device;
protected:
	IDXGISwapChain*			_swap_chain;
	ID3D11RenderTargetView*	_back_buffer;
	ID3D11RenderTargetView*	_current_target;
public:
};

extern Gfx* GFX;

