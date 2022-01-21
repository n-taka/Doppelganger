#ifndef LOG_CPP
#define LOG_CPP

#include "Doppelganger/Util/log.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"

#include <sstream>
#include <iostream>
#include <fstream>

namespace Doppelganger
{
	namespace Util
	{
		void log(
			const std::string &content,
			const std::string &level,
			const nlohmann::json &config)
		{
			fs::path logTextFile(config.at("data").at("dir").get<std::string>());
			logTextFile.append("log");
			logTextFile.append("log.txt");

			if (config.at("log").at("level").contains(level) && config.at("log").at("level").at(level).get<bool>())
			{
				std::stringstream logText;
				// time
				logText << Util::getCurrentTimestampAsString(true);
				logText << " ";
				// logLevel
				logText << "[" << level << "]";
				// time + logLevel + spacing == 30
				for (int i = 0; i < 30 - logText.str().size(); ++i)
				{
					logText << " ";
				}
				// separator
				logText << ": ";
				// content
				logText << content;
				logText << " ";
				// new line
				logText << std::endl;
				if (config.at("log").at("type").at("STDOUT").get<bool>())
				{
					std::cout << logText.str();
				}
				if (config.at("log").at("type").at("FILE").get<bool>())
				{
					std::ofstream ofs(logTextFile.string(), std::ios_base::out | std::ios_base::app);
					ofs << logText.str();
					ofs.close();
				}
			}
		}

		void log(
			const fs::path &path,
			const std::string &level,
			const nlohmann::json &config)
		{
			fs::path logDir(config.at("data").at("dir").get<std::string>());
			logDir.append("log");

			if (config.at("log").at("level").contains(level) && config.at("log").at("level").at(level).get<bool>())
			{
				try
				{
					fs::path logFile(logDir);
					logFile /= path.filename();
					fs::rename(path, logFile);
					std::stringstream s;
					s << "temporary file " << path.filename().string() << " is stored in " << logDir.string();
					log(s.str(), level, config);
				}
				catch (const fs::filesystem_error &e)
				{
					std::stringstream s;
					s << e.what();
					log(s.str(), "ERROR", config);
				}
			}
		}
	}
}

#endif
