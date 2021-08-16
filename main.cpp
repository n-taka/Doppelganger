// #include "include/Doppel/Pantry.h"
// #include "include/Doppel/Gengar.h"

// #include <boost/asio/signal_set.hpp>

#include <iostream>

int main(int argv, char *argc[])
{
	// boost::asio::io_context ioc;

	// std::shared_ptr<Doppel::Pantry> pantry = std::make_shared<Doppel::Pantry>(ioc);
	// std::shared_ptr<Doppel::Gengar> gengar = std::make_shared<Doppel::Gengar>(pantry, ioc);

	// gengar->setup();

	// gengar->run();

	// // Capture SIGINT and SIGTERM to perform a clean shutdown
	// boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
	// signals.async_wait(
	// 	[&ioc](boost::system::error_code const &, int) {
	// 		// Stop the io_context. This will cause run()
	// 		// to return immediately, eventually destroying the
	// 		// io_context and any remaining handlers in it.
	// 		ioc.stop();
	// 	});

	// // Run the I/O service on the main thread
	// ioc.run();

	std::cout << "a" << std::endl;

	return 0;
}
