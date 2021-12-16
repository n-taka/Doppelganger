#ifndef UUID_H
#define UUID_H

#if defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#define DECLSPEC
#elif defined(__linux__)
#define DECLSPEC
#endif

#include <string>

namespace Doppelganger
{
	namespace Util
	{
		std::string DECLSPEC uuid(const std::string& prefix);
	};
} // namespace

#endif
