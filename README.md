# RegNotify
Modern C++ wrapper for subscribing to registry changes using `RegNotifyChangeKeyValue()`

### Example
```c++
RegistryListener listener;

auto callback = [] {
	std::cout << "Registry change was detected!" << std::endl;
};

listener.Subscribe("HKLM\\Software\\WinRAR", callback);
```
