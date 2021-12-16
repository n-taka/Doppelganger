#ifndef LOGGER_H
#define LOGGER_H

#if defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#define DECLSPEC
#elif defined(__linux__)
#define DECLSPEC
#endif

#if defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__APPLE__)
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#elif defined(__linux__)
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	class DECLSPEC Logger
	{
	public:
		Logger(){};
		~Logger(){};

		void initialize(
			const std::string& UUID_,
			const nlohmann::json &config);

		void log(
			const std::string &content,
			const std::string &level);
		void log(
			const fs::path &path,
			const std::string &level,
			const bool remove);

		static std::string getCurrentTimestampAsString(bool separator);

		std::unordered_set<std::string> suppressedAPICall;

		// where we put log
		fs::path logDir;
		fs::path logFile;

	private:
		std::string UUID;
		// set of strings -> boolean
		// {"SYSTEM", "APICALL", "WSCALL", "ERROR", "MISC", "DEBUG"} -> boolean
		std::unordered_map<std::string, bool> logLevel;
		// set of strings -> boolean
		// {"STDOUT", "FILE"} -> boolean
		std::unordered_map<std::string, bool> logType;
	};
}

#endif
