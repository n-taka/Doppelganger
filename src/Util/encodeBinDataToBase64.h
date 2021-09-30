#ifndef ENCODEBINDATATOBASE64_H
#define ENCODEBINDATATOBASE64_H

#include <string>
#include <vector>
#include <boost/beast/core/detail/base64.hpp>

namespace Doppelganger
{
	namespace Util
	{
		std::string encodeBinDataToBase64(const std::vector<unsigned char> &binData);
	};
} // namespace

#endif
