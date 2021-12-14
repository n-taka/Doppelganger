#ifndef WRITEBASE64TOFILE_H
#define WRITEBASE64TOFILE_H

#if defined(_WIN32) || defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#endif

#include <memory>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

namespace Doppelganger
{
	namespace Util
	{
		void DECLSPEC writeBase64ToFile(const std::string &base64Str, const fs::path &destPath);
	};
}

#endif
