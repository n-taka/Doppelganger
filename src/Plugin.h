#ifndef PLUGIN_H
#define PLUGIN_H

#include <memory>
#include <nlohmann/json.hpp>

// #if defined(_WIN32) || defined(_WIN64)
// #include <filesystem>
// namespace fs = std::filesystem;
// #else
// #include "boost/filesystem.hpp"
// namespace fs = boost::filesystem;
// #endif

namespace Doppelganger
{
	class Room;
	class triangleMesh;

	class Plugin
	{
	public:
		Plugin();
		~Plugin() {}

		////
		// typedef for API
		//   all API have this signature, and other parameters (e.g. meshUUID) are supplied within the parameter json.
		typedef std::function<void(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &)> API_t;
		
		API_t func;

	private:
		////
		// typedef for loading API from dll/lib
#if defined(_WIN32) || defined(_WIN64)
		typedef void(__stdcall *APIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &);
#else
		typedef void (*APIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &);
#endif
	};
} // namespace

#endif
