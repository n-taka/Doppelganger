#ifndef UUID_CPP
#define UUID_CPP

#include "Doppelganger/Util/uuid.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/lexical_cast.hpp>

namespace Doppelganger
{
	namespace Util
	{
		std::string uuid(const std::string &prefix)
		{
			std::string uuid(prefix);
			uuid += boost::lexical_cast<std::string>(boost::uuids::random_generator()());
			return uuid;
		}
	}
} // namespace

#endif
