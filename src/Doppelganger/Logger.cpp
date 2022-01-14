#ifndef LOGGER_CPP
#define LOGGER_CPP

#include "Doppelganger/Logger.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"

#include <sstream>
#include <iostream>
#include <fstream>

namespace Doppelganger
{
	void Logger::initialize(
		const fs::path &dataDir,
		const nlohmann::json &config)
	{
		logLevel.clear();
		logType.clear();

		logDir = dataDir;
		logDir.append("log");
		logDir.make_preferred();
		fs::create_directories(logDir);

		for (const auto &level : config.at("level"))
		{
			logLevel[level.get<std::string>()] = true;
		}
		for (const auto &type : config.at("type"))
		{
			logType[type.get<std::string>()] = true;
		}

		// prepare log directory and log.txt
		if (logType["FILE"])
		{
			logFile = logDir;
			logFile.append("log.txt");
		}
	}

	void Logger::log(
		const std::string &content,
		const std::string &level)
	{
		if (logLevel[level])
		{
			std::stringstream logText;

			// time
			logText << Util::getCurrentTimestampAsString(true);
			logText << " ";

			// logLevel
			logText << "[" << level << "]";
			// time + logLevel + spacing == 30
			while (logText.str().size() < 30)
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

			if (logType["STDOUT"])
			{
				std::cout << logText.str();
			}
			if (logType["FILE"])
			{
				std::ofstream ofs(logFile.string(), std::ios_base::out | std::ios_base::app);
				ofs << logText.str();
				ofs.close();
			}
		}
	}

	void Logger::log(const fs::path &path, const std::string &level, const bool removeOriginal)
	{
		if (logLevel[level])
		{
			{
				try
				{
					fs::copy(path, logDir);
					if (removeOriginal)
					{
						fs::remove_all(path);
					}
					std::stringstream s;
					s << "temporary file " << path.filename().string() << " is stored in " << logDir.string();
					log(s.str(), level);
				}
				catch (const fs::filesystem_error &e)
				{
					std::stringstream s;
					s << e.what();
					log(s.str(), "ERROR");
				}
			}
		}
	}
}

#endif
