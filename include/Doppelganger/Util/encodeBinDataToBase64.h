#ifndef ENCODEBINDATATOBASE64_H
#define ENCODEBINDATATOBASE64_H

#if defined(_WIN32) || defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#else
#define DECLSPEC
#endif

#include <string>
#include <vector>
#include <boost/beast/core/detail/base64.hpp>

namespace Doppelganger
{
	namespace Util
	{
		std::string DECLSPEC encodeBinDataToBase64(const std::vector<unsigned char> &binData);
	};
} // namespace

#endif
