
#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <D3DCompiler.inl>

#include <windows.h>
#include <stdint.h>
#include <wchar.h>
#include <string>
#include <vector>
#include <map>

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "winmm.lib")


struct Color
{
	Color() { ; }
	Color(const float _r, const float _g, const float _b, const float _a) { r = _r; g = _g; b = _b; a = _a; }
	operator FLOAT * () { return &r; }
	operator CONST FLOAT * () const { return (const float*)&r; }
	float r, g, b, a;
};

inline std::wstring string_format(const wchar_t* fmt, ...)
{
	wchar_t      buffer[8192];
	va_list		argptr;

	va_start(argptr, fmt);
	vswprintf_s(buffer, 8192, fmt, argptr);
	va_end(argptr);

	return buffer;
}

inline std::string string_format_mb(const char* fmt, ...)
{
	char		buffer[8192];
	va_list		argptr;

	va_start(argptr, fmt);
	sprintf_s(buffer, 8192, fmt, argptr);
	va_end(argptr);

	return buffer;
}

class TimerHelper
{
public:
	TimerHelper()
	{
#if defined(_WIN32) || defined(_WIN64)
		unsigned __int64 frequency;

		if (QueryPerformanceFrequency((LARGE_INTEGER*)&frequency))
		{
			_pc = true;
			_timer_resolution = (float)(1000.0 / (double)frequency);
		}
		else
		{
			_pc = false;
			_timer_resolution = 1.0f;
		}

		_timer_base = get_raw();
#elif defined(_LINUX)
#if defined(CLOCK_MONOTONIC)
		struct timespec ts;

		if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
		{
			_monotonic = true;
			_timer_resolution = 1e-6;
		}
		else
#endif
		{
			_monotonic = false;
			_timer_resolution = 1e-3;
		}

		_timer_base = get_raw();
#elif defined(_MACOS)
		mach_timebase_info_data_t info;
		mach_timebase_info(&info);

		_timer_resolution = 1000.0f*(info.numer / (info.denom*1.0e9));
		_timer_base = get_raw();
#else
#error "Platform not defined!"
#endif
	}
	inline	float	get_raw()
	{
#if defined(_WIN32) || defined(_WIN64)
		if (_pc)
		{
			unsigned __int64 time;
			QueryPerformanceCounter((LARGE_INTEGER*)&time);
			return (float)time;
		}
		else return (float)timeGetTime();
#elif defined(_LINUX)
#if defined(CLOCK_MONOTONIC)
		if (_monotonic)
		{
			struct timespec ts;

			clock_gettime(CLOCK_MONOTONIC, &ts);
			return (float)ts.tv_sec * (float) 1000000000.0f + (float)ts.tv_nsec;
		}
		else
#endif
		{
			struct timeval tv;

			gettimeofday(&tv, NULL);
			return (float)tv.tv_sec * (float) 1000000.0f + (float)tv.tv_usec;
		}
#elif defined(_MACOS)
		return mach_absolute_time();
#else
#error "Platform not defined!"
#endif
	}
	inline	float	get()
	{
		return (get_raw() - _timer_base)*_timer_resolution;
	}
public:
	float				_timer_base;
	float				_timer_resolution;
#if defined(_WIN32) || defined(_WIN64)
	bool				_pc;
#elif defined(_LINUX)
	bool				_monotonic;
#endif
};

extern TimerHelper	TIMER;
