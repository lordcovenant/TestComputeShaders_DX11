#include "Material.h"
#include "Mesh.h"

Material::Material()
{
}


Material::~Material()
{
}

void Material::set()
{
	_vs->set();
	_ps->set();
}

void Material::set_constants(const Shader::ShaderType shader_type,Shader::ShaderConstants* c)
{
	auto it = c->begin();
	while (it != c->end())
	{
		switch (it->_bind_type)
		{
			case Shader::SHADER_MATERIAL_VARIABLE:
				{
					// It's a material variable (custom)
					auto jt = _vars.find(it->_name);
					if (jt != _vars.end())
					{
						if (jt->second._type != it->_type)
						{
							__debugbreak();
						}
						else
						{
							// Set
							switch (it->_type)
							{
								case Shader::DATA_ARRAY_R:
									{
										auto* res = jt->second._mesh->get_shader_resource();
										GFX->_context->CSSetShaderResources(it->_index, 1, &res);
									}
									break;
								case Shader::DATA_ARRAY_RW:
									{
										auto* res = jt->second._mesh->get_unordered_access_view();
										GFX->_context->CSSetUnorderedAccessViews(it->_index, 1, &res,NULL);
									}
									break;
								default:
									__debugbreak();
									break;
							}
						}
					}
					else
					{
						__debugbreak();
					}
				}
				break;
		}

		++it;
	}
}

void Material::add_var(const std::string& var_name, BaseMesh* mesh, const bool is_rw)
{
	auto it = _vars.find(var_name);
	if (it == _vars.end())
	{
		MaterialVar v;
		v._type = (is_rw) ? (Shader::DATA_ARRAY_RW) : (Shader::DATA_ARRAY_R);
		v._mesh = mesh;
		_vars.insert(std::make_pair(var_name, v));
	}
	else
	{
		it->second._type = (is_rw) ? (Shader::DATA_ARRAY_RW) : (Shader::DATA_ARRAY_R);
		it->second._mesh = mesh;
	}
}
