#pragma once

#include <Windows.h>

#include <functional>

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
		RegistryListener();
		~RegistryListener();

		/*
		 * Call Subscribe() to subscribe for specific registry key changes.
		 * keyPath:				The full path of the registry key, seperated by '\' and starting with registry root keys in the format of HKLM or HKCU.
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
			HKEY key = nullptr;
			RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_NOTIFY, &key);

			// Subscribe for changes
			RegNotifyChangeKeyValue(
				key,
				false,	// subtree
				REG_NOTIFY_CHANGE_LAST_SET,
				NULL,
				FALSE
			);
		}

	private:


	};

}
