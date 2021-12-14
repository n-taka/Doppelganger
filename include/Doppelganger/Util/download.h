#ifndef DOWNLOAD_H
#define DOWNLOAD_H

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
		bool DECLSPEC download(const std::string &targetUrl, const fs::path &destPath);
	};
} // namespace

#endif
