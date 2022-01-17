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
// todo variant requires C++17
#include <variant>
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	class Core;
	class Room;

	class DECLSPEC Logger
	{
	public:
		Logger(){};
		~Logger(){};

		void initialize(
			const std::variant<std::shared_ptr<Doppelganger::Core>, std::shared_ptr<Doppelganger::Room>> parent);
		void log(
			const std::string &content,
			const std::string &level) const;
		void log(
			const fs::path &path,
			const std::string &level,
			const bool removeOriginal) const;

		// where we put log
		std::variant<std::shared_ptr<Doppelganger::Core>, std::shared_ptr<Doppelganger::Room>> parent_;

	private:
		static void getLevelAndType(
			const nlohmann::json &config,
			std::unordered_map<std::string, bool> &logLevel,
			std::unordered_map<std::string, bool> &logType);
	};
}

#endif
