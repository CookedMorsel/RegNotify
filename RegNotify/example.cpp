#include <iostream>
#include <thread>

#include "reg_notify.hpp"


int main() {
	using namespace reg_notify;

	try {
		RegistryListener listener;

		// Start listening for changes, supplying a lambda as callback
		std::thread th{ [&] {listener.Subscribe("HKLM\\Software\\WinRAR", [] {
			std::cout << "Change was detected!" << std::endl;
		}); } };

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10s);

		listener.StopAll();
		th.join();
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
	}

	return 0;
}
