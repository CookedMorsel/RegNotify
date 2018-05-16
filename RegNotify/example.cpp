#include <iostream>
#include <thread>

#include "reg_notify.hpp"



using namespace reg_notify;
using namespace std::chrono_literals;


void SimpleMonitorExample() {

	try {
		RegistryListener listener;

		auto callback = [] {
			std::cout << "Registry change was detected!" << std::endl;
		};

		// Start listening for changes, supplying a lambda as callback, returning after 10s
		listener.Subscribe("HKLM\\Software\\WinRAR", callback, 10s);
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
	}
}


void MultipleKeysExample() {
	try {
		RegistryListener foo_listener, security_listener;

		auto security_callback = [] {
			std::cout << "Security Change was detected!" << std::endl;
		};

		// Start monitoring with no subkey notifications
		std::thread th1{ [&] {security_listener.Subscribe("HKLM\\Software\\Microsoft", security_callback, 0ms, false); } };
		// Start notifying only on security descriptor changes
		std::thread th2{ [&] {security_listener.Subscribe("HKLM\\Software\\KasperskyLab", security_callback, 0ms, true, CallbackTriggers::OnKeySecurityDescriptorChange); } };

		std::thread th3{ [&] {foo_listener.Subscribe("HKLM\\Software\\WinRAR", [] {
			std::cout << "WinRAR Change was detected!" << std::endl;
		}); } };

		std::this_thread::sleep_for(10s);
		// Stop foo monitoring
		foo_listener.StopAll();
		th3.join();

		std::this_thread::sleep_for(10s);
		// Stop security monitoring
		security_listener.StopAll();
		th1.join();
		th2.join();
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
	}
}

int main() {

	SimpleMonitorExample();
	//MultipleKeysExample();

	return 0;
}
