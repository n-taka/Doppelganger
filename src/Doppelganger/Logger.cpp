#ifndef LOGGER_CPP
#define LOGGER_CPP

#include "Doppelganger/Logger.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"

#include <sstream>
#include <iostream>
#include <fstream>

namespace Doppelganger
{
	void Logger::initialize(
		const std::variant<std::shared_ptr<Doppelganger::Core>, std::shared_ptr<Doppelganger::Room>> parent)
	{
		parent_ = parent;

		std::visit([](const auto &p)
				   {
						// create directory for log
						fs::path logDir = p->dataDir;
						logDir.append("log");
						fs::create_directories(logDir); },
				   parent_);
	}

	void Logger::log(
		const std::string &content,
		const std::string &level) const
	{
		nlohmann::json config;
		fs::path logDir;
		fs::path logFile;
		std::visit([&config, &logDir, &logFile](const auto &p)
				   {
					   	{
							std::lock_guard<std::mutex> lock(p->mutexConfig);
							p->getCurrentConfig(config);
					   	}
				   		fs::path logDir = p->dataDir;
						logDir.append("log");
						logFile = logDir;
						logFile.append("log.txt"); },
				   parent_);
		std::unordered_map<std::string, bool> logLevel;
		std::unordered_map<std::string, bool> logType;
		getLevelAndType(config, logLevel, logType);

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

	void Logger::log(const fs::path &path, const std::string &level, const bool removeOriginal) const
	{
		nlohmann::json config;
		fs::path logDir;
		fs::path logFile;
		std::visit([&config, &logDir, &logFile](const auto &p)
				   {
					   	{
							std::lock_guard<std::mutex> lock(p->mutexConfig);
							p->getCurrentConfig(config);
					   	}
				   		fs::path logDir = p->dataDir;
						logDir.append("log");
						logFile = logDir;
						logFile.append("log.txt"); },
				   parent_);
		std::unordered_map<std::string, bool> logLevel;
		std::unordered_map<std::string, bool> logType;
		getLevelAndType(config, logLevel, logType);

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

	void Logger::getLevelAndType(
		const nlohmann::json &config,
		std::unordered_map<std::string, bool> &logLevel,
		std::unordered_map<std::string, bool> &logType)
	{
		// set of strings -> boolean
		// {"SYSTEM", "APICALL", "WSCALL", "ERROR", "MISC", "DEBUG"} -> boolean
		logLevel.clear();
		// set of strings -> boolean
		// {"STDOUT", "FILE"} -> boolean
		logType.clear();

		for (const auto &level : config.at("log").at("level"))
		{
			logLevel[level.get<std::string>()] = true;
		}
		for (const auto &type : config.at("log").at("type"))
		{
			logType[type.get<std::string>()] = true;
		}
	}
}

#endif
