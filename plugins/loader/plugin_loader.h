#pragma once

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <string>

class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();

    // Load shared library at path. Returns true on success.
    bool load(const std::string& path);

    // Get symbol from loaded library, or nullptr if not found.
    void* getSymbol(const std::string& name) const;

    // Unload the library if loaded.
    void unload();

    // Check whether a library is loaded.
    bool isLoaded() const;

private:
    void* handle_;
};

#endif // PLUGIN_LOADER_H
