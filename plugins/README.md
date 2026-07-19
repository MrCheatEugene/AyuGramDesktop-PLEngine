# Plugins top-level README

This directory contains plugin-related build targets and examples.

- loader/ - simple cross-platform plugin loader (dlopen/LoadLibrary wrapper).
- sample_plugin/ - example plugin built as a shared library.

Build instructions
1. From the repository root:

```bash
mkdir -p build && cd build
cmake .. -DBUILD_PLUGINS=ON
cmake --build . --target plugin_loader
cmake --build . --target sample_plugin
```

2. The plugin shared library will be in build/plugins/ and the loader static library will be built into the build tree as well.

Runtime example (pseudo-code)

```cpp
PluginLoader loader;
if (loader.load("./build/plugins/sample_plugin.so")) {
    using init_t = int(*)(void*);
    auto init_sym = (init_t)loader.getSymbol("plugin_init");
    if (init_sym) init_sym(nullptr);
    // ... later:
    auto shutdown_sym = (void(*)())loader.getSymbol("plugin_shutdown");
    if (shutdown_sym) shutdown_sym();
    loader.unload();
}
```
