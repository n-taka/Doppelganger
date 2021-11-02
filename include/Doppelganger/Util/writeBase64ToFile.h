#ifndef WRITEBASE64TOFILE_H
#define WRITEBASE64TOFILE_H

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
		void writeBase64ToFile(const std::string &base64Str, const fs::path &destPath);
	};
}

#endif
