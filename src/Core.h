#ifndef CORE_H
#define CORE_H

#include <memory>
#include <string>
#include <unordered_map>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif
#include <boost/asio.hpp>
#include <boost/any.hpp>
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	class Room;
	class triangleMesh;

	class Core : public std::enable_shared_from_this<Core>
	{
	public:
		Core(boost::asio::io_context &ioc_);

		void setup();
		void run();

		////
		// parameters for Doppel
		struct systemParameters
		{
			fs::path baseDir;
			std::mutex mutexSystemParams;
		};
		systemParameters systemParams;

		////
		// API
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

		nlohmann::json config;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Room>> rooms;
		std::unordered_map<std::string, boost::any> API;

	private:
		// for graceful shutdown
		boost::asio::io_context &ioc;
		boost::asio::ip::tcp::acceptor acceptor;
		boost::asio::ip::tcp::socket socket;

		void parseConfig(
			const fs::path &pathConfig,
			const fs::path &baseDir,
			nlohmann::json &config);

		void fail(
			boost::system::error_code ec,
			char const *what);
		void onAccept(
			boost::system::error_code ec);
	};
}

#endif
