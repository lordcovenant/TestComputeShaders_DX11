#include "Gfx.h"
#include "Window.h"

Gfx* GFX=NULL;

Gfx::Gfx()
{
	GFX = this;
}


Gfx::~Gfx()
{
	_swap_chain->SetFullscreenState(FALSE,NULL);
	if (_swap_chain)
	{
		_swap_chain->Release();
		_swap_chain = NULL;
	}
	if (_back_buffer)
	{
		_back_buffer->Release();
		_back_buffer = NULL;
	}
	_current_target = NULL;
	if (_device)
	{
		_device->Release();
		_device = NULL;
	}
	if (_context)
	{
		_context->Release();
		_context = NULL;
	}


	GFX = NULL;
}

bool Gfx::init(Window* window)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = window->_window;                                // the window to be used
	scd.SampleDesc.Count = 1;                               // how many multisamples
	scd.SampleDesc.Quality = 0;                               // how many multisamples
	scd.BufferDesc.Width = window->get_width();
	scd.BufferDesc.Height = window->get_height();
	scd.Windowed = TRUE;                                    // windowed/full-screen mode
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	UINT create_device_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
	create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;

	// create a device, device context and swap chain using the information in the scd struct
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, create_device_flags, NULL, NULL, D3D11_SDK_VERSION, &scd, &_swap_chain, &_device, NULL, &_context) != S_OK)
	{
		return false;
	}

	// get the address of the back buffer
	ID3D11Texture2D*		back_buffer_texture;
	_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&back_buffer_texture);

	// use the back buffer address to create the render target
	_device->CreateRenderTargetView(back_buffer_texture, NULL, &_back_buffer);
	back_buffer_texture->Release();

	// set the render target as the back buffer
	_context->OMSetRenderTargets(1,&_back_buffer,NULL);

	_current_target = _back_buffer;

	// Set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX=0;
	viewport.TopLeftY=0;
	viewport.Width=(float)window->get_width();
	viewport.Height = (float)window->get_height();

	_context->RSSetViewports(1,&viewport);

	return true;
}

void Gfx::clear(const float r, const float g, const float b, const float a)
{
    float color[4] = {r, g, b, a};
	_context->ClearRenderTargetView(_current_target, &color[0]);
}

void Gfx::swap_buffers()
{
	_swap_chain->Present(1, 0);
}
