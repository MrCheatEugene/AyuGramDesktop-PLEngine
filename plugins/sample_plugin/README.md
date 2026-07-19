# Плагин-пример для AyuGramDesktop-PLEngine

Этот каталог содержит минимальный пример плагина (C-style API), который собирается как разделяемая библиотека (shared library) и выводится в папку build/plugins.

Файлы:
- sample_plugin.h/.cpp — простая реализация с функциями plugin_name, plugin_init и plugin_shutdown.
- plugin.json — минимальный манифест плагина (имя, версия, entrypoint).
- CMakeLists.txt — инструкция для сборки плагина как shared library.

Как собрать (локально)
1. В корне репозитория создайте каталог сборки и выполните cmake:

```bash
mkdir -p build && cd build
cmake .. -DBUILD_PLUGINS=ON
cmake --build . --target sample_plugin
```

2. После сборки артефакты окажутся в build/plugins/ (напр., sample_plugin.so / sample_plugin.dll / sample_plugin.dylib).

Как интегрировать в основной CMake проекта
- Добавьте строку в корневой CMakeLists.txt (например после основных опций):

  add_subdirectory(plugins)

- Параметр BUILD_PLUGINS (по умолчанию ON) позволяет временно отключать сборку плагинов.

Как загрузить плагин в рантайме
- На Linux/macOS используйте dlopen("./plugins/sample_plugin.so", RTLD_NOW) и dlsym для поиска symbol'ов (plugin_init, plugin_shutdown).
- На Windows используйте LoadLibrary/GetProcAddress.

Примечания
- Этот пример использует простой C-style API для максимальной совместимости. Если у вас есть существующий API/engine для плагинов в проекте, адаптируйте сигнатуры и манифест под него.
