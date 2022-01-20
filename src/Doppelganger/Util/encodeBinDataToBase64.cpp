#ifndef ENCODEBINDATATOBASE64_CPP
#define ENCODEBINDATATOBASE64_CPP

#include "Doppelganger/Util/encodeBinDataToBase64.h"

namespace Doppelganger
{
	namespace Util
	{
		std::string encodeBinDataToBase64(const std::vector<unsigned char> &binData)
		{
			if (binData.size() > 0)
			{
				const size_t len = boost::beast::detail::base64::encoded_size(binData.size() * sizeof(unsigned char));
				std::vector<unsigned char> destVec;
				destVec.resize(len);
				const size_t lenWritten = boost::beast::detail::base64::encode(&(destVec[0]), &(binData[0]), binData.size() * sizeof(unsigned char));
				return std::string(destVec.begin(), destVec.begin() + lenWritten);
			}
			else
			{
				return std::string("");
			}
		}
	};
} // namespace

#endif
