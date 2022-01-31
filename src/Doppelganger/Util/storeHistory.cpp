#ifndef STOREHISTORY_CPP
#define STOREHISTORY_CPP

#include "Doppelganger/Util/storeHistory.h"

namespace Doppelganger
{
	namespace Util
	{
		void storeHistory(
			const nlohmann::json &currentHistory,
			const nlohmann::json &diff,
			const nlohmann::json &diffInv,
			nlohmann::json &updatedHistory)
		{
			updatedHistory = currentHistory;

			// erase entries later than currentIndex
			const int currentIndex = updatedHistory.at("index").get<int>();
			if ((currentIndex + 0) < updatedHistory.at("diffFromNext").size())
			{
				updatedHistory.at("diffFromNext").erase(updatedHistory.at("diffFromNext").begin() + (currentIndex + 0), updatedHistory.at("diffFromNext").end());
			}
			if ((currentIndex + 1) < updatedHistory.at("diffFromPrev").size())
			{
				updatedHistory.at("diffFromPrev").erase(updatedHistory.at("diffFromPrev").begin() + (currentIndex + 1), updatedHistory.at("diffFromPrev").end());
			}
			// store diff/diffInv
			updatedHistory.at("diffFromNext").push_back(diffInv);
			updatedHistory.at("diffFromPrev").push_back(diff);
			updatedHistory.at("index") = updatedHistory.at("diffFromNext").size();
		}
	}
} // namespace

#endif
