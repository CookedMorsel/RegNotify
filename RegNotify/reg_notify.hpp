#pragma once

#include <Windows.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace reg_notify {

	// TODO
	// Warn about 64 / 32 bit registry

	using namespace std::chrono_literals;

	enum CallbackTriggers {
		OnSubkeyChange = 1,
		OnKeyAttributesChange = 2,
		OnValueChange = 4,
		OnKeySecurityDescriptorChange = 8,
		OnAnyChange = 15
	};

	class RegistryListener {

	public:
		RegistryListener()
			: mStopNotifyEvent(nullptr),
			mSubscribeWakeupEvents()
		{
			// Initialize stopping event
			mStopNotifyEvent = CreateEventA(
				NULL,
				TRUE,	// manual reset
				FALSE,	// initialy not signaled
				NULL	// no name
			);

			if (nullptr == mStopNotifyEvent) {
				throw std::runtime_error("failed opening event: " + std::to_string(GetLastError()));
			}
		}

		~RegistryListener()
		{
			for (const auto& ev : mSubscribeWakeupEvents) {
				if (nullptr != ev && INVALID_HANDLE_VALUE != ev) {
					CloseHandle(ev);
				}
			}
		}

		/*
		 * Call Subscribe() to subscribe for specific registry key changes. This function is blocking.
		 *
		 * keyPath: The full path of the registry key, seperated by '\' and starting with registry root keys in the format of HKLM or HKCU.
		 *	 Case insensitive.
		 *	 Example: "HKLM\\Software\\WinRAR"
		 * callback: The function to be called on the specified trigger events.
		 *   Callback is called within the thread context of the calling thread.
		 *   While callback() is being executed, no monitoring is performed so multiple changes in the key will not result in a same number of calls to callback.
		 * includeSubkeys: If this parameter is true, the funtion reports changes in the specified key and also its subkeys.
		 * callbackTriggers: This is a mask of any CallbackTriggers.
		 *	 Example:  OnValueChange | OnKeyAttributesChange
		 * duration: The duration for Subscribe() to monitor, after which it returns.
		 *
		 * Throws exception on error.
		 */
		void Subscribe(std::wstring keyPath, std::function<void()> callback, std::chrono::milliseconds duration = 0ms, bool includeSubkeys = true, std::uint16_t callbackTriggers = OnAnyChange) {
			// Get start time
			auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());

			// Classify the root key
			auto s_root_key = keyPath.substr(0, 4);
			HKEY root_key = nullptr;
			if (L"HKLM" == s_root_key) { root_key = HKEY_LOCAL_MACHINE; }
			else if (L"HKCU" == s_root_key) { root_key = HKEY_CURRENT_USER; }
			else if (L"HKCR" == s_root_key) { root_key = HKEY_CLASSES_ROOT; }
			else if (L"HKCC" == s_root_key) { root_key = HKEY_CURRENT_CONFIG; }
			else if (L"HKPD" == s_root_key) { root_key = HKEY_PERFORMANCE_DATA; }
			else if (L"HKU" == s_root_key) { root_key = HKEY_USERS; }
			else { throw std::invalid_argument("incorrect root key"); }

			// Open the registry key with notify permissions
			HKEY reg_key = nullptr;
			auto status = RegOpenKeyExW(
				root_key,
				keyPath.substr(keyPath.find_first_of('\\') + 1).c_str(),
				0, KEY_NOTIFY, &reg_key
			);
			if (ERROR_SUCCESS != status) {
				if (ERROR_ACCESS_DENIED == status) throw std::runtime_error("access denied");
				if (ERROR_FILE_NOT_FOUND == status) throw std::invalid_argument("key path");
				throw std::runtime_error("failed opening registry key: " + std::to_string(status));
			}

			// CloseHandle will be called on function exit
			auto HandleDeleter = [](HANDLE h) {
				if (INVALID_HANDLE_VALUE != h) { CloseHandle(h); }
			};
			// unique_ptr ctor is noexcept
			std::unique_ptr<std::remove_pointer_t<decltype(reg_key)>, decltype(HandleDeleter)> pRegKey{ reg_key, HandleDeleter };


			// Initialize event via which RegNotifyChangeKeyValue will notify us
			auto ev = CreateEventA(
				NULL,
				FALSE,	// no manual reset
				FALSE,	// initialy not signaled
				NULL	// no name
			);
			if (nullptr == ev) {
				throw std::runtime_error("failed opening event: " + std::to_string(GetLastError()));
			}
			try {
				mSubscribeWakeupEvents.emplace_back(ev);
			}
			catch (const std::exception& err) {
				// If CreateEvent succeeds but emplace_back throws we should not leak handles
				CloseHandle(ev);
				throw err;
			}

			// Construct our array of events that wake us up
			std::vector<HANDLE> wakeups;
			wakeups.emplace_back(mStopNotifyEvent);
			wakeups.emplace_back(ev);

			while (1) {

				// Subscribe for changes
				status = RegNotifyChangeKeyValue(
					reg_key,
					includeSubkeys,
					callbackTriggers,
					ev,
					TRUE
				);

				if (ERROR_SUCCESS != status) {
					throw std::runtime_error("failed subscribing to registry key: " + std::to_string(status));
				}

				std::uint64_t time_to_wait = INFINITE;
				if (duration.count() != 0) {
					// Calculate how much waiting time we have left
					auto ms_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
					time_to_wait = (duration - (ms_now - start_ms)).count();
					time_to_wait = time_to_wait < 0 ? 0 : time_to_wait;
				}

				// Wait for either registry change notify, exit signal or specified duration timeout
				auto res = WaitForMultipleObjects(2, &wakeups[0], FALSE, static_cast<DWORD>(time_to_wait));

				switch (res) {
				case (WAIT_TIMEOUT):
				{
					return;
				}
				case (WAIT_OBJECT_0):
				{
					// Signaled to exit
					return;
				}
				case (WAIT_OBJECT_0 + 1):
				{
					// Registry key was changed
					callback();
					break;
				}
				case (WAIT_FAILED):
				{
					throw std::runtime_error("WaitForMultipleObjects failed: " + std::to_string(GetLastError()));
				}
				default:
				{
					// Someone killed our mutex holder thread
					throw std::runtime_error("WaitForMultipleObjects wait abandoned");
				}
				}
			}
		}

		inline void Subscribe(std::string keyPath, std::function<void()> callback, std::chrono::milliseconds duration = 0ms, bool includeSubkeys = true, std::uint16_t callbackTriggers = OnAnyChange) {
			std::wstring ws(keyPath.cbegin(), keyPath.cend());
			this->Subscribe(ws, callback, duration, includeSubkeys, callbackTriggers);
		}

		/*
		 * Stops all asynchronous operations and deletes all subscriptions to registry keys
		 */
		void StopAll() {
			if (0 == SetEvent(mStopNotifyEvent)) {
				throw std::runtime_error("Failed stopping registry subscriptions: " + std::to_string(GetLastError()));
			}
		}

	private:

		// Contains events that are used by the RegNotifyChangeKeyValue function to notify us of a registry change
		std::vector<HANDLE> mSubscribeWakeupEvents;

		// We use Win32 event instead of std::cv because we use WaitForMultipleObjects
		HANDLE mStopNotifyEvent;



	};

}
