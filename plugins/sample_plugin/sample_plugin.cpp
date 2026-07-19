#include "sample_plugin.h"
#include <cstdio>

const char* plugin_name() {
    return "sample_plugin";
}

int plugin_init(void* host_context) {
    // Простейшая реализация: логирование в stdout.
    (void)host_context; // пока не используется
    std::puts("[sample_plugin] initialized");
    return 0; // 0 == success
}

void plugin_shutdown() {
    std::puts("[sample_plugin] shutdown");
}
