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
			const std::unordered_map<std::string, bool> &logLevels,
			const std::unordered_map<std::string, bool> &logTypes)
		{
			fs::path logTextFile(dataDir);
			logTextFile.append("log");
			logTextFile.append("log.txt");

			if ((logLevels.find(level) != logLevels.end()) && logLevels.at(level))
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
				if ((logTypes.find("STDOUT") != logTypes.end()) && logTypes.at("STDOUT"))
				{
					std::cout << logText.str();
				}
				if ((logTypes.find("FILE") != logTypes.end()) && logTypes.at("FILE"))
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
			const std::unordered_map<std::string, bool> &logLevels,
			const std::unordered_map<std::string, bool> &logTypes)
		{
			fs::path logDir(dataDir);
			logDir.append("log");

			if ((logLevels.find(level) != logLevels.end()) && logLevels.at(level))
			{
				try
				{
					fs::path logFile(logDir);
					logFile /= path.filename();
					fs::rename(path, logFile);
					std::stringstream ss;
					ss << "temporary file " << path.filename().string() << " is stored in " << logDir.string();
					log(ss.str(), level, dataDir, logLevels, logTypes);
				}
				catch (const fs::filesystem_error &e)
				{
					std::stringstream ss;
					ss << e.what();
					log(ss.str(), "ERROR", dataDir, logLevels, logTypes);
				}
			}
		}
	}
}

#endif
