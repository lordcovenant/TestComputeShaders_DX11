#include "Mesh.h"

BaseMesh::Layouts	BaseMesh::g_layouts;

bool operator<(const BaseMesh::LayoutKey& lk1, const BaseMesh::LayoutKey& lk2)
{
	if (lk1._shader == lk2._shader)
	{
		return lk1._desc < lk2._desc;
	}
	return lk1._shader < lk2._shader;
}

BaseMesh::BaseMesh(const Type type,const bool dynamic) 
{ 
	_type = type; 
	_shader_resource= NULL; 
	_unordered_access_view = NULL;
	_vb = NULL;
	_n_vertex = 0;
	_vb_size = 0;
	_dynamic = dynamic;
	_cpu_buffer = NULL;
}

BaseMesh::~BaseMesh()
{
	clear();
}

ID3D11InputLayout* BaseMesh::get_layout(Shader* shader, D3D11_INPUT_ELEMENT_DESC* desc, const int len)
{
	auto it = g_layouts.find(LayoutKey(shader, desc));
	if (it != g_layouts.end()) return it->second;

	ID3D11InputLayout* layout;

	GFX->_device->CreateInputLayout(desc, len, shader->_shader_raw->GetBufferPointer(), shader->_shader_raw->GetBufferSize(),&layout);

	g_layouts.insert(std::make_pair(LayoutKey(shader, desc), layout));

	return layout;
}

ID3D11ShaderResourceView* BaseMesh::get_shader_resource()
{
	if (!_shader_resource)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
		ZeroMemory(&view_desc, sizeof(view_desc));
		view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		view_desc.BufferEx.FirstElement = 0;
		view_desc.Format = DXGI_FORMAT_UNKNOWN;
		view_desc.BufferEx.NumElements = (UINT)_n_vertex;

		GFX->_device->CreateShaderResourceView(_vb, &view_desc, &_shader_resource);
	}

	return _shader_resource;
}

ID3D11UnorderedAccessView* BaseMesh::get_unordered_access_view()
{
	if (!_unordered_access_view)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC  view_desc;
		ZeroMemory(&view_desc, sizeof(view_desc));
		view_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		view_desc.Buffer.FirstElement = 0;
		view_desc.Format = DXGI_FORMAT_UNKNOWN;
		view_desc.Buffer.NumElements = (UINT)_n_vertex;

		GFX->_device->CreateUnorderedAccessView(_vb, &view_desc, &_unordered_access_view);
	}

	return _unordered_access_view;
}

void BaseMesh::clear()
{
	if (_shader_resource)
	{
		_shader_resource->Release();
		_shader_resource = NULL;
	}
	if (_unordered_access_view)
	{
		_unordered_access_view->Release();
		_unordered_access_view = NULL;
	}
	if (_vb)
	{
		_vb->Release();
		_vb = NULL;
	}
	if (_cpu_buffer)
	{
		_cpu_buffer->Release();
		_cpu_buffer = NULL;
	}
}

void BaseMesh::set_fetch(const bool b,const size_t max_len)
{
	if (_cpu_buffer)
	{
		_cpu_buffer->Release();
		_cpu_buffer = NULL;
	}
	if (b)
	{
		size_t struct_size = get_data_size();

		// Create staging based resource to copy from GPU to CPU
		D3D11_BUFFER_DESC			desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		desc.StructureByteStride = (UINT)struct_size;
		desc.ByteWidth = (UINT)(struct_size*max_len);

		HRESULT hRet = GFX->_device->CreateBuffer(&desc, NULL, &_cpu_buffer);
	}
}
