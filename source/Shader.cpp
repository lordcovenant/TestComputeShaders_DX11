#include "Shader.h"
#include "Material.h"
#include "Gfx.h"

Shader::Shader()
{
	_shader._vs = NULL;
	_shader._ps= NULL;
	_shader._cs = NULL;
}

Shader::~Shader()
{
	switch (_type)
	{
		case VERTEX: if (_shader._vs) _shader._vs->Release(); _shader._vs = NULL; break;
		case PIXEL: if (_shader._ps) _shader._ps->Release(); _shader._ps = NULL; break;
		case COMPUTE: if (_shader._cs) _shader._cs->Release(); _shader._cs = NULL; break;
	}

	for (size_t i = 0; i < _defines.size(); i++)
	{
		if (_defines[i].Name)
		{
			free((void*)_defines[i].Name);
			free((void*)_defines[i].Definition);
		}
	}
	_defines.clear();
}

bool Shader::load(const ShaderType type,const std::string& filename, const std::string& entrypoint)
{
	const char*		t;
	switch (type)
	{ 
		case VERTEX: t = "vs_4_0";
			break;
		case PIXEL: t = "ps_4_0";
			break;
		case COMPUTE: t = "cs_5_0";
			break;
	}

	ID3D10Blob*			err_msg=NULL;
	D3D10_SHADER_MACRO*	macros = NULL; 

	if (_defines.size() > 0)
	{
		if (_defines[_defines.size() - 1].Name != NULL)
		{
			D3D10_SHADER_MACRO	m;
			m.Name = NULL;
			m.Definition = NULL;

			_defines.push_back(m);
		}

		macros = &(_defines[0]);
	}

	UINT compile_flags = (type==COMPUTE)?(D3DCOMPILE_IEEE_STRICTNESS):(0);

	if (D3DX11CompileFromFile(filename.c_str(), macros, NULL, entrypoint.c_str(), t, compile_flags, NULL, NULL, &_shader_raw, &err_msg, NULL) != S_OK)
	{
		if (err_msg)
		{
			OutputDebugString((char*)err_msg->GetBufferPointer());
			OutputDebugString("\n");
			
			err_msg->Release();
		}
		return false;
	}

	if (err_msg) err_msg->Release();

	_type = type;

	switch (_type)
	{
		case VERTEX:
			GFX->_device->CreateVertexShader(_shader_raw->GetBufferPointer(), _shader_raw->GetBufferSize(), NULL, &_shader._vs);
			break;
		case PIXEL:
			GFX->_device->CreatePixelShader(_shader_raw->GetBufferPointer(), _shader_raw->GetBufferSize(), NULL, &_shader._ps);
			break;
		case COMPUTE:
			GFX->_device->CreateComputeShader(_shader_raw->GetBufferPointer(), _shader_raw->GetBufferSize(), NULL, &_shader._cs);
			break;
	}

	parse_constants();

	return true;
}

void Shader::set(Material* material)
{
	switch (_type)
	{
		case VERTEX: GFX->_context->VSSetShader(_shader._vs, 0, 0); break;
		case PIXEL: GFX->_context->PSSetShader(_shader._ps, 0, 0); break;
	}
	if (material)
	{
		material->set_constants(_type, &_constants);
	}
}

void Shader::set_const(const char* name, const int val)
{
	D3D10_SHADER_MACRO	m;
	m.Name = _strdup(name);
	m.Definition = _strdup(string_format("%i", val).c_str());

	_defines.push_back(m);
}

void Shader::run(const int tx, const int ty, const int tz,Material* material)
{
	if (_type != COMPUTE)
	{
		return;
	}

	GFX->_context->CSSetShader(_shader._cs,NULL,0);
	if (material)
	{
		material->set_constants(_type,&_constants);
	}

	GFX->_context->Dispatch(tx,ty,tz);


	ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
	ID3D11ShaderResourceView* ppSRVNULL[2] = { NULL, NULL };

	GFX->_context->CSSetShaderResources(0, 1, ppSRVNULL);
	GFX->_context->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL, NULL);
}

void Shader::parse_constants()
{
	ID3D11ShaderReflection* reflection = NULL;
	D3D11Reflect(_shader_raw->GetBufferPointer(), _shader_raw->GetBufferSize(), &reflection);

	if (reflection)
	{
		D3D11_SHADER_DESC desc;
		reflection->GetDesc(&desc);

		for (size_t i = 0; i < desc.ConstantBuffers; i++)
		{
			unsigned int register_index = 0;
			ID3D11ShaderReflectionConstantBuffer* buffer = NULL;
			buffer = reflection->GetConstantBufferByIndex((UINT)i);

			D3D11_SHADER_BUFFER_DESC bdesc;
			buffer->GetDesc(&bdesc);

			for (unsigned int k = 0; k < desc.BoundResources; ++k)
			{
				D3D11_SHADER_INPUT_BIND_DESC ibdesc;
				reflection->GetResourceBindingDesc(k, &ibdesc);

				if (!strcmp(ibdesc.Name, bdesc.Name))
				{
					ShaderConstant c;
					c._index = ibdesc.BindPoint;
					c._name = bdesc.Name;
					c._bind_type = SHADER_MATERIAL_VARIABLE;
					c._type = get_constant_type(ibdesc.Type);
					c._size = bdesc.Size;

					_constants.push_back(c);
				}
			}

		}

		reflection->Release();
	}
}

Shader::DataType Shader::get_constant_type(const D3D_SHADER_INPUT_TYPE t)
{
	switch (t)
	{
		case D3D_SIT_STRUCTURED:
			return DATA_ARRAY_R;
			break;
		case D3D_SIT_UAV_RWSTRUCTURED:
			return DATA_ARRAY_RW;
			break;
	}
	__debugbreak();

	return DATA_SCALAR;
}
