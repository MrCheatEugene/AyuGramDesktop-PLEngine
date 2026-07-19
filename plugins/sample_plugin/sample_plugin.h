#pragma once

#ifndef SAMPLE_PLUGIN_H
#define SAMPLE_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

// Имя плагина. Возвращает строковый литерал.
const char* plugin_name();

// Инициализация плагина.
// host_context — указатель на контекст хоста (может быть NULL).
// Возвращает 0 при успехе, ненулевое значение при ошибке.
int plugin_init(void* host_context);

// Завершение работы плагина.
void plugin_shutdown();

#ifdef __cplusplus
}
#endif

#endif // SAMPLE_PLUGIN_H
