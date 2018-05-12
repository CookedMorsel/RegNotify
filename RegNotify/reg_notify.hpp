#pragma once

#include <Windows.h>

#include <functional>
#include <stdexcept>
#include <string>

namespace reg_notify {

	// TODO
	// Warn about 64 / 32 bit registry


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
			: mKey(nullptr)
		{

		}

		~RegistryListener()
		{
			if (this->mKey != nullptr) {
				RegCloseKey(this->mKey);
				this->mKey = nullptr;
			}
		}

		/*
		 * Call Subscribe() to subscribe for specific registry key changes.
		 * keyPath:				The full path of the registry key, seperated by '\' and starting with registry root keys in the format of HKLM or HKCU.
		 *						Case insensitive.
		 *						Example: "HKLM\\Software\\WinRAR"
		 * callback:			The function to be called on the specified trigger events.
		 * includeSubkeys:		If this parameter is true, the funtion reports changes in the specified key and also its subkeys.
		 * callbackTriggers:	This is a mask of any CallbackTriggers.
		 *						Example:  OnValueChange | OnKeyAttributesChange
		 * Throws exception on error.
		 */
		void Subscribe(std::string keyPath, std::function<void()> callback, bool includeSubkeys=true, std::uint16_t callbackTriggers=OnAnyChange) {

			// TODO
			// overload for wstring

			// Classify the root key
			auto s_root_key = keyPath.substr(0, 4);
			HKEY root_key = nullptr;
			if ("HKLM" == s_root_key) {	root_key = HKEY_LOCAL_MACHINE; }
			else if ("HKCU" == s_root_key) { root_key = HKEY_CURRENT_USER; }
			else if ("HKCR" == s_root_key) { root_key = HKEY_CLASSES_ROOT; }
			else if ("HKCC" == s_root_key) { root_key = HKEY_CURRENT_CONFIG; }
			else if ("HKPD" == s_root_key) { root_key = HKEY_PERFORMANCE_DATA; }
			else if ("HKU" == s_root_key) { root_key = HKEY_USERS; }
			else { throw std::invalid_argument("incorrect root key"); }

			// Open the registry key with notify permissions
			auto status = RegOpenKeyExA(
				root_key,
				keyPath.substr(keyPath.find_first_of('\\') + 1).c_str(),
				0, KEY_NOTIFY, &this->mKey
			);
			if (ERROR_SUCCESS != status) {
				if (ERROR_ACCESS_DENIED == status) throw std::runtime_error("access denied");
				if (ERROR_FILE_NOT_FOUND == status) throw std::invalid_argument("key path");
				throw std::runtime_error("failed opening registry key with code " + std::to_string(status));
			}

			while (1) {

				// Subscribe for changes
				status = RegNotifyChangeKeyValue(
					this->mKey,
					includeSubkeys,
					callbackTriggers,
					nullptr,
					FALSE
				);

				if (ERROR_SUCCESS != status) {
					RegCloseKey(this->mKey);
					this->mKey = nullptr;
					throw std::runtime_error("failed subscribing to registry key with code " + std::to_string(status));
				}

				// Registry key was changed
				callback();
			}
		}

	private:

		HKEY mKey;

	};

}
