#ifndef HTTPSESSION_H
#define HTTPSESSION_H

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

#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace Doppelganger
{
	class Core;

	class DECLSPEC HTTPSession : public std::enable_shared_from_this<HTTPSession>
	{
		boost::asio::ip::tcp::socket socket;
		boost::beast::flat_buffer buffer;
		boost::beast::http::request<boost::beast::http::string_body> req;
		const std::shared_ptr<Core> core;

		void fail(boost::system::error_code ec, char const *what);
		void onRead(boost::system::error_code ec, std::size_t);
		void onWrite(boost::system::error_code ec, std::size_t, bool close);

	public:
		HTTPSession(boost::asio::ip::tcp::socket socket, const std::shared_ptr<Core> &core_);
		void run();
	};
}

#endif
