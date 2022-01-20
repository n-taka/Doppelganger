#ifndef WRITEBASE64TOFILE_CPP
#define WRITEBASE64TOFILE_CPP

#include "Doppelganger/Util/writeBase64ToFile.h"

#include <string>
#include <fstream>
#include <boost/beast/core/detail/base64.hpp>

namespace Doppelganger
{
	namespace Util
	{
		void writeBase64ToFile(const std::string &base64Str, const fs::path &destPath)
		{
			if (base64Str.length() > 0)
			{
				std::vector<unsigned char> binData;
				const size_t len = boost::beast::detail::base64::decoded_size(base64Str.size());
				binData.resize(len);
				const std::pair<size_t, size_t> lenWrittenRead = boost::beast::detail::base64::decode(&(binData[0]), base64Str.data(), base64Str.size());

				std::ofstream ofs(destPath.string(), std::ios::out | std::ios::binary | std::ios::trunc);
				if (ofs)
				{
					ofs.write(reinterpret_cast<char *>(&(binData[0])), lenWrittenRead.first);
					ofs.close();
				}
			}
		}
	}
}

#endif
