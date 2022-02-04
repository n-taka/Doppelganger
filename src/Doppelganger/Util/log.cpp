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
			const nlohmann::json& config)
		{
			fs::path logTextFile(config.at("dataDir").get<std::string>());
			logTextFile.append("log");
			logTextFile.append("log.txt");
			const nlohmann::json& logLevelJson = config.at("log").at("level");
			const nlohmann::json& logTypeJson = config.at("log").at("type");

			if (logLevelJson.contains(level) && logLevelJson.at(level).get<bool>())
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
				if (logTypeJson.contains("STDOUT") && logTypeJson.at("STDOUT").get<bool>())
				{
					std::cout << logText.str();
				}
				if (logTypeJson.contains("FILE") && logTypeJson.at("FILE").get<bool>())
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
			const nlohmann::json& config)
		{
			fs::path logDir(config.at("dataDir").get<std::string>());
			logDir.append("log");
			const nlohmann::json& logLevelJson = config.at("log").at("level");
			const nlohmann::json& logTypeJson = config.at("log").at("type");

			if (logLevelJson.contains(level) && logLevelJson.at(level).get<bool>())
			{
				try
				{
					fs::path logFile(logDir);
					logFile /= path.filename();
					fs::rename(path, logFile);
					std::stringstream ss;
					ss << "temporary file " << path.filename().string() << " is stored in " << logDir.string();
					log(ss.str(), level, config);
				}
				catch (const fs::filesystem_error &e)
				{
					std::stringstream ss;
					ss << e.what();
					log(ss.str(), "ERROR", config);
				}
			}
		}
	}
}

#endif
