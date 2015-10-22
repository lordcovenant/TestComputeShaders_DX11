#pragma once

#include "Shader.h"

class BaseMesh;

class Material
{
protected:
	struct MaterialVar
	{
		Shader::DataType	_type;
		union
		{
			float		_scalar[4];
			BaseMesh*	_mesh;
		};
	};
	typedef std::map<std::string, MaterialVar>	MaterialVars;
public:
	Material();
	virtual ~Material();

	void	set();
	void	set_constants(const Shader::ShaderType shader_type, Shader::ShaderConstants* c);
	void	add_var(const std::string& var_name, BaseMesh* mesh, const bool is_rw);

public:
	Shader*			_vs;
	Shader*			_ps;
	MaterialVars	_vars;
};
