#include <iostream>
#include "reg_notify.hpp"


int main() {
	using namespace reg_notify;

	try {
		RegistryListener listener;

		// Start listening for changes, supplying a lambda as callback
		listener.Subscribe("HKLM\\Software\\WinRAR", [] {
			std::cout << "Change was detected!" << std::endl;
		});

	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
	}

	return 0;
}
