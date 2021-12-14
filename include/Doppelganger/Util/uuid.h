#ifndef UUID_H
#define UUID_H

#if defined(_WIN32) || defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
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
