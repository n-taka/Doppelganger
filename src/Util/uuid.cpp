#ifndef UUID_CPP
#define UUID_CPP

#include "Util/uuid.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/lexical_cast.hpp>

namespace Doppelganger
{
	namespace Util
	{
		std::string uuid()
		{
			return boost::lexical_cast<std::string>(boost::uuids::random_generator()());
		}
	}
} // namespace

#endif
