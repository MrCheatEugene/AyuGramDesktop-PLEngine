#include <queue>
#include <functional>
#include <mutex>

std::queue<std::function<void()>> MainQueue;
std::mutex queueMutex;

__declspec(dllexport) std::queue<std::function<void()>> *GetQueuePtr();

__declspec(dllexport) void AddToQueue(const std::function<void()> &func);
