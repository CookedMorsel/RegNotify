#include <iostream>
#include <thread>

#include "reg_notify.hpp"


int main() {
	using namespace reg_notify;

	try {
		RegistryListener listener;

		// Start listening for changes, supplying a lambda as callback
		std::thread th1{ [&] {listener.Subscribe("HKLM\\Software\\WinRAR", [] {
			std::cout << "WinRAR Change was detected!" << std::endl;
		}); } };
		std::thread th2{ [&] {listener.Subscribe("HKLM\\Software\\KasperskyLab", [] {
			std::cout << "Kaspersky Change was detected!" << std::endl;
		}); } };

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10s);

		listener.StopAll();
		th1.join();
		th2.join();
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
	}

	return 0;
}
