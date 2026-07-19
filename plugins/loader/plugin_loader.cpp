#include "plugin_loader.h"
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

PluginLoader::PluginLoader()
  : handle_(nullptr) {}

PluginLoader::~PluginLoader() {
  unload();
}

bool PluginLoader::load(const std::string& path) {
  unload();
#ifdef _WIN32
  handle_ = (void*)LoadLibraryA(path.c_str());
  if (!handle_) {
    std::cerr << "plugin_loader: LoadLibrary failed for " << path << "\n";
    return false;
  }
#else
  handle_ = dlopen(path.c_str(), RTLD_NOW);
  if (!handle_) {
    std::cerr << "plugin_loader: dlopen failed for " << path << ": " << dlerror() << "\n";
    return false;
  }
#endif
  return true;
}

void* PluginLoader::getSymbol(const std::string& name) const {
  if (!handle_) return nullptr;
#ifdef _WIN32
  return (void*)GetProcAddress((HMODULE)handle_, name.c_str());
#else
  return dlsym(handle_, name.c_str());
#endif
}

void PluginLoader::unload() {
  if (!handle_) return;
#ifdef _WIN32
  FreeLibrary((HMODULE)handle_);
#else
  dlclose(handle_);
#endif
  handle_ = nullptr;
}

bool PluginLoader::isLoaded() const {
  return handle_ != nullptr;
}
