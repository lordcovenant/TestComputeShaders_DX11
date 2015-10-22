#pragma once

#include "Gfx.h"
#include "Material.h"


#define DECL_LAYOUT(X)																\
	static const int _layout_size=(X);												\
	static D3D11_INPUT_ELEMENT_DESC* get_layout()									\
	{																				\
		static D3D11_INPUT_ELEMENT_DESC	_layout[2] =								\
			{																	

#define DECL_LAYOUT_END()															\
			};																		\
			return (D3D11_INPUT_ELEMENT_DESC*)&_layout;								\
		}					

#define EMPTY_LAYOUT()																\
	static const int _layout_size=0;												\
	static D3D11_INPUT_ELEMENT_DESC* get_layout()									\
	{																				\
		return NULL;																\
	}

class BaseMesh
{
public:
	typedef enum { NORMAL, COMPUTE_DATA } Type;
public:
	BaseMesh(const Type type, const bool dynamic);
	virtual ~BaseMesh();

	virtual	void	draw(Material* material) =0;
	virtual void	close() = 0;
			void	clear();
	virtual size_t	get_data_size() = 0;
			void	set_fetch(const bool b, const size_t max_len);

public:
	ID3D11ShaderResourceView*	get_shader_resource();
	ID3D11UnorderedAccessView*	get_unordered_access_view();

public:
	ID3D11Buffer*				_vb;
	Type						_type;
	size_t						_n_vertex;
	D3D11_BUFFER_DESC			_desc;
	size_t						_vb_size;
	bool						_dynamic;
	ID3D11ShaderResourceView*	_shader_resource;
	ID3D11UnorderedAccessView*	_unordered_access_view;
	ID3D11Buffer*				_cpu_buffer;

public:
	static	ID3D11InputLayout*	get_layout(Shader* shader, D3D11_INPUT_ELEMENT_DESC* desc, const int len);
	struct LayoutKey
	{
		LayoutKey(Shader* shader, D3D11_INPUT_ELEMENT_DESC* desc) { _shader = shader; _desc = desc; }

		Shader*						_shader;
		D3D11_INPUT_ELEMENT_DESC*	_desc;
	};
	typedef std::map<LayoutKey, ID3D11InputLayout*>	Layouts;
	static Layouts	g_layouts;
};

template <class T>
class Mesh : public BaseMesh
{
public:
	inline Mesh(const Type type, const bool dynamic);
	inline virtual ~Mesh();

	inline	void	add(const T& v) { _data.push_back(v); }
	virtual	void	draw(Material* material);
	virtual void	close();
			void	load(T* val, const unsigned int n_elems);
			void	reserve(const unsigned int n_elems);
			void	get(std::vector<T>* output);
			void	get(T* output, const size_t elems);
			void	get_async(Mesh<T>* dest);
			bool	get_async(T* output, const size_t elems);
			virtual size_t	get_data_size() { return sizeof(T); }

public:
	std::vector<T>				_data;
};

template<class T>
Mesh<T>::Mesh(const Type type, const bool dynamic) : BaseMesh(type,dynamic)
{
	ZeroMemory(&_desc, sizeof(_desc));
	_desc.Usage = (dynamic) ? (D3D11_USAGE_DYNAMIC) : (D3D11_USAGE_DEFAULT);
	_desc.BindFlags = (_type==NORMAL)?(D3D11_BIND_VERTEX_BUFFER):(D3D11_BIND_UNORDERED_ACCESS|D3D11_BIND_SHADER_RESOURCE);
	_desc.CPUAccessFlags = (dynamic)?(D3D11_CPU_ACCESS_WRITE):(0);
	_desc.MiscFlags = (_type == NORMAL) ? (0) : (D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	_desc.StructureByteStride = sizeof(T);
}

template<class T>
Mesh<T>::~Mesh()
{
	clear();
}

template<class T>
void Mesh<T>::close()
{
	if (_data.size() == 0) return;

	load(&_data[0],(unsigned int)_data.size());

	if (!_dynamic) _data.clear();
}

template<class T>
void Mesh<T>::reserve(const unsigned int n_elems)
{
	clear();
	load(NULL, n_elems);
}

template<class T>
void Mesh<T>::load(T* val, const unsigned int n_elems)
{
	if ((_dynamic) && (_vb))
	{
		if (_vb_size < n_elems)
		{
			_vb->Release();
			_vb = NULL;
		}
	}

	if ((!_vb) || (_dynamic))
	{
		if (!_vb)
		{
			_desc.ByteWidth = (UINT)(sizeof(T)*n_elems);

			// Fill in the subresource data.
			if (val)
			{
				D3D11_SUBRESOURCE_DATA init_data;
				init_data.pSysMem = val;
				init_data.SysMemPitch = 0;
				init_data.SysMemSlicePitch = 0;

				HRESULT hRet = GFX->_device->CreateBuffer(&_desc, &init_data, &_vb);
			}
			else
			{
				HRESULT hRet = GFX->_device->CreateBuffer(&_desc, NULL, &_vb);
			}

			_vb_size = n_elems;
		}
		else
		{
			if (val)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				GFX->_context->Map(_vb, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);   // map the buffer
				memcpy(ms.pData, val, sizeof(T)*n_elems);							// copy the data
				GFX->_context->Unmap(_vb, NULL);									// unmap the buffer
			}
		}

		_n_vertex = n_elems;
	}
}

template<class T>
void Mesh<T>::get(std::vector<T>* output)
{
	output->resize(_n_vertex);

	get(&output->at(0), _n_vertex);
}

template<class T>
void Mesh<T>::get(T* output,const size_t elems)
{
	ID3D11Buffer*				temp_vb = (_cpu_buffer) ? (_cpu_buffer) : (NULL);

	if (!temp_vb)
	{
		// Create staging based resource to copy from GPU to CPU
		D3D11_BUFFER_DESC			desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		desc.StructureByteStride = sizeof(T);
		desc.ByteWidth = (UINT)(sizeof(T)*elems);

		HRESULT hRet = GFX->_device->CreateBuffer(&desc, NULL, &temp_vb);
	}

	GFX->_context->CopyResource(temp_vb, _vb);

	D3D11_MAPPED_SUBRESOURCE ms;
	if (GFX->_context->Map(temp_vb, NULL, D3D11_MAP_READ, NULL, &ms) == S_OK)
	{
		//	memcpy(output, ms.pData, sizeof(T)*_n_vertex);				// copy the data
		GFX->_context->Unmap(temp_vb, NULL);						// unmap the buffer
	}
	
	if (!_cpu_buffer) temp_vb->Release();
}

template<class T>
void Mesh<T>::get_async(Mesh<T>* dest)
{
	if (!dest->_cpu_buffer)
	{
		// Create staging based resource to copy from GPU to CPU
		D3D11_BUFFER_DESC			desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		desc.StructureByteStride = sizeof(T);
		desc.ByteWidth = (UINT)(sizeof(T)*_n_vertex);

		HRESULT hRet = GFX->_device->CreateBuffer(&desc, NULL, &dest->_cpu_buffer);
	}

	GFX->_context->CopyResource(dest->_cpu_buffer, _vb);
}

template<class T>
bool Mesh<T>::get_async(T* output, const size_t elems)
{
	D3D11_MAPPED_SUBRESOURCE ms;
	if (GFX->_context->Map(_cpu_buffer, NULL, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &ms) == DXGI_ERROR_WAS_STILL_DRAWING)
	{
		return false;
	}
	memcpy(output, ms.pData, sizeof(T)*_n_vertex);		// copy the data
	GFX->_context->Unmap(_cpu_buffer, NULL);				// unmap the buffer

	return true;
}

template<class T>
void Mesh<T>::draw(Material* material)
{
	close();

	ID3D11InputLayout* layout = get_layout(material->_vs, T::get_layout(), T::_layout_size);

	GFX->_context->IASetInputLayout(layout);

	material->set();

	UINT len = sizeof(T);
	UINT offset = 0;
	GFX->_context->IASetVertexBuffers(0, 1, &_vb, &len,&offset);
	GFX->_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	GFX->_context->Draw((UINT)_n_vertex, 0);
}
