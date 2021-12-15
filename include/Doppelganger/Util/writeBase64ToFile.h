#ifndef WRITEBASE64TOFILE_H
#define WRITEBASE64TOFILE_H

#if defined(_WIN32) || defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#else
#define DECLSPEC
#endif

#if defined(__APPLE__)
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace Doppelganger
{
	namespace Util
	{
		void DECLSPEC writeBase64ToFile(const std::string &base64Str, const fs::path &destPath);
	};
}

#endif
