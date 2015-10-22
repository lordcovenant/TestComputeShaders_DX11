
#include "Window.h"
#include "Gfx.h"
#include "Shader.h"
#include "Material.h"
#include "Mesh.h"

struct PCVertex
{
	float		x, y, z;
	D3DXCOLOR	color;

	PCVertex(const float _x, const float _y, const float _z, const D3DXCOLOR _c) { x = _x; y = _y; z = _z; color = _c; }
	DECL_LAYOUT(2)
	{
		"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
	},
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	DECL_LAYOUT_END()
};

struct CSData
{
	float val;

	EMPTY_LAYOUT();
};

#define N_ELEMS		4096*8192
#define N_TEST		1000

float sum_array_cpu(float* values, uint64_t n)
{
	double sum = 0.0f;
	for (int i = 0; i < n; i++)
	{
		sum += (double)values[i];
	}
	return (float)sum;
}

#define RING_BUFFER_SIZE	4

class GPUSum
{
public:
	Shader*			_compute;
	int				_n_threads;
	Mesh<CSData>*	_data_input;
	Mesh<CSData>*	_data_output[RING_BUFFER_SIZE];
	Material*		_material;
	int				_elems;

	void setup(float* values, int n)
	{
		_n_threads = 1024;
		_elems = n;

		_compute = new Shader;
		_compute->set_const("THREAD_X", _n_threads);
		_compute->set_const("THREAD_Y", 1);
		_compute->set_const("THREAD_Z", 1);

		if (_compute->load(Shader::COMPUTE, "compute_shader.shader", "entry_point"))
		{
			_data_input = new Mesh<CSData>(BaseMesh::COMPUTE_DATA, false);
			_data_input->load((CSData*)values, n);

			for (int i = 0; i < RING_BUFFER_SIZE; i++) _data_output[i] = new Mesh<CSData>(BaseMesh::COMPUTE_DATA, false);

			_material = new Material;
		}
	}
	void shutdown()
	{
		delete _data_input;
		delete _material;
		for (int i = 0; i < RING_BUFFER_SIZE; i++) delete _data_output[i];
		delete _compute;
	}
};

float sum_array_gpu(GPUSum* gpusum, float& inner_loop, float& fetch_time, Mesh<CSData>* output_data=NULL)
{
	float	ret = 0.0f;

	int				output_index = 0;
	Mesh<CSData>*	next_output = gpusum->_data_output[output_index];

	inner_loop = TIMER.get();

	gpusum->_material->add_var("Buffer0", gpusum->_data_input, false);
	gpusum->_material->add_var("BufferOut", next_output, true);

	int n_disp = gpusum->_elems / gpusum->_n_threads;

	while (1)
	{
		int dn = (n_disp <= 0)?(1):(n_disp);

		next_output->reserve(dn);
			
		gpusum->_compute->run(dn, 1, 1, gpusum->_material);

		if ((n_disp == 0) || (dn==1))
		{
			inner_loop = TIMER.get() - inner_loop;

			if (output_data)
			{
				next_output->get_async(output_data);
				ret = FLT_MAX;
			}
			else
			{
				CSData	temp;
				fetch_time = TIMER.get();
				next_output->get(&temp, 1);
				ret = temp.val;
				fetch_time = TIMER.get() - fetch_time;
			}
			break;
		}
		else
		{
			gpusum->_material->add_var("Buffer0", gpusum->_data_output[output_index], false);
			output_index = (output_index + 1) % RING_BUFFER_SIZE;
				
			gpusum->_material->add_var("BufferOut", gpusum->_data_output[output_index], true);
			next_output = gpusum->_data_output[output_index];
		}
		n_disp = n_disp / gpusum->_n_threads;
	}

	return ret;
}
/*
float sum_array_gpu(float* values, int n, float& inner_loop, float& fetch_time)
{
	float	ret = 0.0f;
	int		n_threads = 1024;

	Shader* compute = new Shader;
	compute->set_const("THREAD_X", n_threads);
	compute->set_const("THREAD_Y", 1);
	compute->set_const("THREAD_Z", 1);
	compute->set_const("ELEMS_PER_GROUP", n / n_threads);

	if (compute->load(Shader::COMPUTE, "compute_shader.shader", "entry_point"))
	{
		Mesh<CSData>	data_input(BaseMesh::COMPUTE_DATA, false);
		data_input.load((CSData*)values, n);

		Mesh<CSData>*	data_output[2];
		data_output[0] = new Mesh<CSData>(BaseMesh::COMPUTE_DATA, false);
		data_output[1] = new Mesh<CSData>(BaseMesh::COMPUTE_DATA, false);

		Mesh<CSData>	final_output(BaseMesh::COMPUTE_DATA, false);
		final_output.reserve(1);
		final_output.set_fetch(true, 1);

		Mesh<CSData>*	next_output;

		int output_index = 0;
		next_output = data_output[output_index];

		Material*		material = new Material;

		inner_loop = TIMER.get();

		material->add_var("Buffer0", &data_input, false);
		material->add_var("BufferOut", next_output, true);

		int n_disp = n / n_threads;
		next_output->reserve(n_disp);

		while (1)
		{
			int dn = (n_disp <= 0) ? (1) : (n_disp);

			compute->run(dn, 1, 1, material);

			if (n_disp == 0)
			{
				inner_loop = TIMER.get() - inner_loop;

				__declspec(align(64)) CSData	temp;
				fetch_time = TIMER.get();
				final_output.get(&temp, 1);
				ret = temp.val;
				fetch_time = TIMER.get() - fetch_time;
				break;
			}
			else
			{
				material->add_var("Buffer0", data_output[output_index], false);
				output_index = (output_index + 1) % 2;
				if ((n_disp / n_threads) == 0)
				{
					material->add_var("BufferOut", &final_output, true);
					next_output = &final_output;
				}
				else
				{
					material->add_var("BufferOut", data_output[output_index], true);
					next_output = data_output[output_index];

					int dn = (n_disp <= 0) ? (1) : (n_disp);

					next_output->reserve(dn);
				}
			}
			n_disp = n_disp / n_threads;
		}

		delete material;
		delete data_output[0];
		delete data_output[1];
	}

	delete compute;

	return ret;
}
*/
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	Window* window = new Window("Test",1600,900);
	
	new Gfx;

	if (!GFX->init(window))
	{
		return -1;
	}

	// Load shaders
	Material* material = new Material; 
	material->_vs = new Shader; material->_vs->load(Shader::VERTEX, "simple_shader.shader", "VShader");
	material->_ps = new Shader; material->_ps->load(Shader::PIXEL, "simple_shader.shader", "PShader");

	Mesh<PCVertex>*	mesh = new Mesh < PCVertex >(BaseMesh::NORMAL, false);
	mesh->add(PCVertex(0.0f, 0.5f, 0.0f, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f)));
	mesh->add(PCVertex(0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f)));
	mesh->add(PCVertex(-0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f)));

	srand(0);

	// Create sum array
	float* values = new float[N_ELEMS];
	for (int i = 0; i < N_ELEMS; i++)
	{
		values[i] = ((float)(rand() % 100000)) / 100000.0f;
	}

/*	float	t0, t1, inner_loop_time, fetch_time;
	char	buffer[512];
	t0 = TIMER.get();
	float	val = sum_array_cpu(values, N_ELEMS);
	t1 = TIMER.get();
	sprintf_s((char*)&buffer, 512, "Value CPU=%f || Time=%f ms || Elems=%u\n",val,t1-t0,N_ELEMS);
	OutputDebugString("------------------------------------------------------------------------------------\n");
	OutputDebugString(buffer);
	OutputDebugString("------------------------------------------------------------------------------------\n");
	*/
	GPUSum s;
	s.setup(values, N_ELEMS);

/*	t0 = TIMER.get();
	val = sum_array_gpu(&s,values, N_ELEMS, inner_loop_time, fetch_time);
	t1 = TIMER.get();
	sprintf_s((char*)&buffer, 512, "Value GPU=%f || Time=%f ms (%f ms / fetch_time=%f ms)|| Elems=%u\n", val, t1 - t0, inner_loop_time, fetch_time, N_ELEMS);
	OutputDebugString("------------------------------------------------------------------------------------\n");
	OutputDebugString(buffer);
	OutputDebugString("------------------------------------------------------------------------------------\n");
	*/
	// Compute avg fetch time
/*	GFX->clear(1.0f, 0.0f, 0.0f, 1.0f);
	GFX->swap_buffers();

	Mesh<CSData>*	ret_buffers[N_TEST];
	for (int i = 0; i < N_TEST; i++) ret_buffers[i] = new Mesh<CSData>(BaseMesh::COMPUTE_DATA, false);

	float setup_time = TIMER.get();
	float fetch_sum_time = 0.0f;
	float inner_loop_time_sum_time = 0.0f;
	for (int i = 0; i < N_TEST; i++)
	{
		val = sum_array_gpu(&s, values, N_ELEMS, inner_loop_time, fetch_time, ret_buffers[i]);
		fetch_sum_time += fetch_time;
		inner_loop_time_sum_time += inner_loop_time;
	}
	setup_time = TIMER.get() - setup_time;

	GFX->clear(1.0f, 1.0f, 0.0f, 1.0f);
	GFX->swap_buffers();
	Sleep(1000);
	float clear_time = TIMER.get();
	GFX->clear(0.0f, 1.0f, 0.0f, 1.0f);
	GFX->swap_buffers();
//	GFX->clear(0.0f, 1.0f, 1.0f, 1.0f);
//	GFX->swap_buffers();
	clear_time = TIMER.get() - clear_time;
	// 
	CSData tmp_val;

	float first_one_ready = TIMER.get();
	while (!ret_buffers[0]->get_async(&tmp_val, 1))
	{
		;
	}
	first_one_ready = TIMER.get() - first_one_ready;

	float last_one_ready = TIMER.get();
	while (!ret_buffers[N_TEST-1]->get_async(&tmp_val, 1))
	{
		;
	}
	last_one_ready = TIMER.get() - last_one_ready;

	int first_one = INT_MAX;
	fetch_sum_time = TIMER.get();
	for (int i = 1; i < N_TEST-1; i++)
	{
		if (ret_buffers[i]->get_async(&tmp_val, 1))
		{
			if (i < first_one) first_one = i;
		}
	}
	fetch_sum_time = TIMER.get() - fetch_sum_time;

	OutputDebugString("------------------------------------------------------------------------------------\n");
	sprintf_s((char*)&buffer, 512, "Setup time=%f ms (%f/instance)\n", setup_time, setup_time / (float)N_TEST);
	OutputDebugString(buffer);
	sprintf_s((char*)&buffer, 512, "Clear time=%f ms\n", clear_time);
	OutputDebugString(buffer);
	sprintf_s((char*)&buffer, 512, "Time until first is ready=%f ms\n", first_one_ready);
	OutputDebugString(buffer);
	sprintf_s((char*)&buffer, 512, "Time until last is ready=%f ms\n", last_one_ready);
	OutputDebugString(buffer);
	sprintf_s((char*)&buffer, 512, "First I got ready=%i\n", first_one);
	OutputDebugString(buffer);
	sprintf_s((char*)&buffer, 512, "Avg. Fetch Time=%f ms\n", fetch_sum_time / (float)N_TEST);
	OutputDebugString(buffer);
	sprintf_s((char*)&buffer, 512, "Avg. Inner Loop Time=%f ms\n", inner_loop_time_sum_time / (float)N_TEST);
	OutputDebugString(buffer);
	OutputDebugString("------------------------------------------------------------------------------------\n");
	*/

	Mesh<CSData>*	prev_frame = NULL;
	float			inner_loop_time;
	float			fetch_time;
	char			buffer[512];

	while (Window::do_message_pump())
	{
/*		if (prev_frame)
		{
			CSData tmp_val;

			float first_one_ready = TIMER.get();
			while (!prev_frame->get_async(&tmp_val, 1))
			{
				;
			}
			first_one_ready = TIMER.get() - first_one_ready;

			sprintf_s((char*)&buffer, 512, "Value GPU=%f (%f ms)\r", tmp_val.val, first_one_ready);
//			OutputDebugString("------------------------------------------------------------------------------------\n");
			OutputDebugString(buffer);
//			OutputDebugString("------------------------------------------------------------------------------------\n");
		}
		else
		{
			prev_frame = new Mesh<CSData>(BaseMesh::COMPUTE_DATA, false);
		}*/
		float t = TIMER.get();
		float val=sum_array_gpu(&s, inner_loop_time, fetch_time);
		t = TIMER.get() - t;
		sprintf_s((char*)&buffer, 512, "Value GPU=%f (%f ms)\r", val, t);
		OutputDebugString(buffer);

		GFX->clear((float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, 1.0f);
		mesh->draw(material);
		GFX->swap_buffers();
	}

	s.shutdown();
	delete GFX;
	delete window;
	delete[] values;

	return 0;
}