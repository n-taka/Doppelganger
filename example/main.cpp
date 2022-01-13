#include "Doppelganger/Core.h"

#include <boost/asio/signal_set.hpp>
#include <thread>

int main(int argv, char *argc[])
{
	const int threads = std::max<int>(1, std::thread::hardware_concurrency());

	boost::asio::io_context ioc{threads};
	boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12};

	std::shared_ptr<Doppelganger::Core> core = std::make_shared<Doppelganger::Core>(ioc, ctx);
	core->setup();
	core->run();

	boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
	signals.async_wait(
		[&ioc](boost::system::error_code const &, int)
		{
			ioc.stop();
		});

	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for (auto i = threads - 1; i > 0; --i)
		v.emplace_back(
			[&ioc]
			{
				ioc.run();
			});
	ioc.run();

	// (If we get here, it means we got a SIGINT or SIGTERM)

	// Block until all the threads exit
	for (auto &t : v)
	{
		t.join();
	}

	return 0;
}
