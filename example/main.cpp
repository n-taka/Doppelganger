#include "Doppelganger/Core.h"

#include <boost/asio/signal_set.hpp>

#include <iostream>

int main(int argv, char *argc[])
{
	boost::asio::io_context ioc;

	std::shared_ptr<Doppelganger::Core> core = std::make_shared<Doppelganger::Core>(ioc);
	core->setup();
	core->run();

	// Capture SIGINT and SIGTERM to perform a clean shutdown
	boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
	signals.async_wait(
		[&ioc](boost::system::error_code const &, int)
		{
			// Stop the io_context. This will cause run()
			// to return immediately, eventually destroying the
			// io_context and any remaining handlers in it.
			ioc.stop();
		});

	// Run the I/O service on the main thread
	ioc.run();

	return 0;
}
