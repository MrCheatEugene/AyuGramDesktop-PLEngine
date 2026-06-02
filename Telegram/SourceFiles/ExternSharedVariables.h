#pragma once

/*
Emil Kh, AKA Pomorgite - t.me/Pomorgite // pmrgt.com
AyuGram Plugin engine, 2026 // t.me/ayuplugg
Follows GNU GPL v3 and Telegram Desktop licensing.
*/

#include <vector>
#include <AyuPlugin.h>

struct TrustedUAs
{
	std::vector<std::string> runtime;
	std::vector<std::string> session;
	std::vector<std::string> events; // todo
};

struct PluginData {
	std::string name;
	std::string description;
	std::string modulePath;
	bool usesHooks = false;
	std::string hooksList;
	InternalDrawGUI drawGUI = nullptr;
};

inline extern std::vector<InternalDoFilterHistoryItem> FunctionsOnFilter{};
inline extern std::vector<InternalDoPreProcessMessage> FunctionsOnPrepare{};
inline extern std::vector<InternalIsOnline> FunctionsOnIsOnline{};
inline extern std::vector<InternalExcludeDeletion> FunctionsExcludeDeleted{};
inline extern std::vector<InternalDrawGUI> FunctionsDrawGUI{};
inline extern std::vector<PluginData> pluginsData{};
inline extern TrustedUAs trusted{};