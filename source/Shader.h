#pragma once

#include "Defs.h"

class Material;

class Shader
{
public:
	typedef enum { VERTEX, PIXEL, COMPUTE } ShaderType;
	typedef enum { SHADER_MATERIAL_VARIABLE } BindType;
	typedef enum { DATA_SCALAR, DATA_TEXTURE, DATA_ARRAY_R, DATA_ARRAY_RW } DataType;
	struct ShaderConstant
	{
		uint16_t	_index;
		std::string	_name;
		BindType	_bind_type;
		DataType	_type;
		size_t		_size;
	};
	typedef std::vector<ShaderConstant>	ShaderConstants;
protected:
	typedef std::vector<D3D10_SHADER_MACRO>	Defines;
public:
	Shader();
	virtual ~Shader();

	bool load(const ShaderType type, const std::string& filename, const std::string& entrypoint);
	void set(Material* material = NULL);
	void set_const(const char* name, const int val);
	void run(const int tx, const int ty, const int tz,Material* material=NULL);

protected:
			void		parse_constants();
	static	DataType	get_constant_type(const D3D_SHADER_INPUT_TYPE t);
public:
	ShaderType		_type;
	ID3D10Blob*		_shader_raw;
	union	
	{
		ID3D11VertexShader*		_vs;
		ID3D11PixelShader*		_ps;
		ID3D11ComputeShader*	_cs;
	} _shader;
	Defines			_defines;
	ShaderConstants	_constants;
};

