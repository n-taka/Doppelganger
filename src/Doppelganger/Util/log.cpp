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
			const fs::path &dataDir,
			const LogConfig& logConfig)
		{
			fs::path logTextFile(dataDir);
			logTextFile.append("log");
			logTextFile.append("log.txt");

			if ((logConfig.level.find(level) != logConfig.level.end()) && logConfig.level.at(level))
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
				if ((logConfig.type.find("STDOUT") != logConfig.type.end()) && logConfig.type.at("STDOUT"))
				{
					std::cout << logText.str();
				}
				if ((logConfig.type.find("FILE") != logConfig.type.end()) && logConfig.type.at("FILE"))
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
			const fs::path &dataDir,
			const LogConfig& logConfig)
		{
			fs::path logDir(dataDir);
			logDir.append("log");

			if ((logConfig.level.find(level) != logConfig.level.end()) && logConfig.level.at(level))
			{
				try
				{
					fs::path logFile(logDir);
					logFile /= path.filename();
					fs::rename(path, logFile);
					std::stringstream ss;
					ss << "temporary file " << path.filename().string() << " is stored in " << logDir.string();
					log(ss.str(), level, dataDir, logConfig);
				}
				catch (const fs::filesystem_error &e)
				{
					std::stringstream ss;
					ss << e.what();
					log(ss.str(), "ERROR", dataDir, logConfig);
				}
			}
		}
	}
}

#endif
