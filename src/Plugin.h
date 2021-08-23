#ifndef PLUGIN_H
#define PLUGIN_H

#include <memory>
#include <boost/any.hpp>
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
		// typedef for Room API
		typedef std::function<void(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &)> RoomAPI_t;
#if defined(_WIN32) || defined(_WIN64)
		typedef void(__stdcall *RoomAPIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &);
#else
		typedef void (*RoomAPIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &);
#endif
		// typedef for Mesh API
		typedef std::function<void(const std::shared_ptr<Doppelganger::triangleMesh> &, const nlohmann::json &, nlohmann::json &)> MeshAPI_t;
#if defined(_WIN32) || defined(_WIN64)
		typedef void(__stdcall *MeshAPIPtr_t)(const std::shared_ptr<Doppelganger::triangleMesh> &, const nlohmann::json &, nlohmann::json &);
#else
		typedef void (*MeshAPIPtr_t)(const std::shared_ptr<Doppelganger::triangleMesh> &, const nlohmann::json &, nlohmann::json &);
#endif

	public:
		boost::any func;
	};
} // namespace

#endif
