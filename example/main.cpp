#include "Doppelganger/Core.h"

#include <boost/asio/signal_set.hpp>
#include <thread>

int main(int argv, char *argc[])
{
	std::shared_ptr<Doppelganger::Core> core = std::make_shared<Doppelganger::Core>();
	core->setup();
	core->run();

	boost::asio::signal_set signals(core->ioc_, SIGINT, SIGTERM);
	signals.async_wait(
		[&core](boost::system::error_code const &, int)
		{
			core->ioc_.stop();
		});

	const int threads = std::max<int>(1, std::thread::hardware_concurrency());
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for (auto i = threads - 1; i > 0; --i)
		v.emplace_back(
			[&core]
			{
				core->ioc_.run();
			});
	core->ioc_.run();

	// (If we get here, it means we got a SIGINT or SIGTERM)

	// Block until all the threads exit
	for (auto &t : v)
	{
		t.join();
	}

	return 0;
}
