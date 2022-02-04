#ifndef GETPLUGINCATALOGUE_CPP
#define GETPLUGINCATALOGUE_CPP

#include <fstream>

#include "Doppelganger/Util/getPluginCatalogue.h"
#include "Doppelganger/Util/download.h"

namespace Doppelganger
{
	namespace Util
	{
		void getPluginCatalogue(
			const fs::path &pluginDir,
			const nlohmann::json &listURLJsonArray,
			nlohmann::json &catalogue)
		{
			catalogue = nlohmann::json::array();

			for (const auto &listURL : listURLJsonArray)
			{
				fs::path listJsonPath(pluginDir);
				listJsonPath.append("tmp.json");
				Util::download(listURL.get<std::string>(), listJsonPath);
				std::ifstream ifs(listJsonPath.string());
				if (ifs)
				{
					nlohmann::json list = nlohmann::json::parse(ifs);
					ifs.close();
					fs::remove_all(listJsonPath);
					catalogue.insert(catalogue.end(), list.begin(), list.end());
				}
			}
		}
	}
}

#endif
