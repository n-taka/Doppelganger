#ifndef STOREHISTORY_H
#define STOREHISTORY_H

#include <nlohmann/json.hpp>

namespace Doppelganger
{
	namespace Util
	{
		void storeHistory(
			const nlohmann::json& currentHistory,
			const nlohmann::json& diff,
			const nlohmann::json& diffInv,
			nlohmann::json& updatedHistory
		);
	};
} // namespace

#endif
