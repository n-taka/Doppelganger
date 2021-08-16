#ifndef LOGGER_CPP
#define LOGGER_CPP

#include "Logger.h"

#include <sstream>
#include <iostream>
#include <fstream>

namespace Doppelganger
{
	Logger &Logger::getInstance()
	{
		static Logger obj;
		return obj;
	}

	void Logger::initialize(
		const fs::path &logDir_,
		const std::unordered_map<std::string, bool> &logLevel_,
		const std::unordered_map<std::string, bool> &logType_)
	{
		logLevel = logLevel_;
		logType = logType_;

		// prepare log directory and log.txt
		if (logType["FILE"])
		{
			logDir = logDir_;
			fs::create_directories(logDir);

			logFile = logDir;
			logFile /= "log.txt";
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
			logText << getCurrentTimestampAsString(true);
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

	void Logger::log(const fs::path &path, const std::string &level)
	{
		if (logLevel[level])
		{
			std::stringstream s;
			{
				try
				{
					fs::copy(path, logDir);
					s << "temporary file " << path.filename().string() << " is stored in " << logDir.string();
				}
				catch (const fs::filesystem_error &e)
				{
					s << e.what();
				}
			}
			log(s.str(), level);
		}
	}

	std::string Logger::getCurrentTimestampAsString(bool separator)
	{
		time_t t = time(nullptr);
		const tm *localTime = localtime(&t);
		std::stringstream s;
		s << "20" << localTime->tm_year - 100;
		if (separator)
		{
			s << "/";
		}
		s << std::setw(2) << std::setfill('0') << localTime->tm_mon + 1;
		if (separator)
		{
			s << "/";
		}
		s << std::setw(2) << std::setfill('0') << localTime->tm_mday;
		if (separator)
		{
			s << "/";
		}
		s << std::setw(2) << std::setfill('0') << localTime->tm_hour;
		if (separator)
		{
			s << ":";
		}
		s << std::setw(2) << std::setfill('0') << localTime->tm_min;
		if (separator)
		{
			s << ":";
		}
		s << std::setw(2) << std::setfill('0') << localTime->tm_sec;
		return s.str();
	}
}

#endif
