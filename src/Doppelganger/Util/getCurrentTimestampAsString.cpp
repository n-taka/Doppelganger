#ifndef GETCURRENTTIMESTAMPASSTRING_CPP
#define GETCURRENTTIMESTAMPASSTRING_CPP

#include "Doppelganger/Util/getCurrentTimestampAsString.h"

#include <sstream>
#include <iomanip>

namespace Doppelganger
{
	namespace Util
	{
		std::string getCurrentTimestampAsString(bool separator)
		{
			time_t t = time(nullptr);
			const tm *localTime = localtime(&t);
			std::stringstream s;
			s << "20" << localTime->tm_year - 100;
			if (separator)
			{
				s << "/";
			}
			s << std::setw(2) << std::setfill('0') << localTime->tm_mon + 1;
			if (separator)
			{
				s << "/";
			}
			s << std::setw(2) << std::setfill('0') << localTime->tm_mday;
			if (separator)
			{
				s << "/";
			}
			s << std::setw(2) << std::setfill('0') << localTime->tm_hour;
			if (separator)
			{
				s << ":";
			}
			s << std::setw(2) << std::setfill('0') << localTime->tm_min;
			if (separator)
			{
				s << ":";
			}
			s << std::setw(2) << std::setfill('0') << localTime->tm_sec;
			return s.str();
		}
	}
}

#endif
