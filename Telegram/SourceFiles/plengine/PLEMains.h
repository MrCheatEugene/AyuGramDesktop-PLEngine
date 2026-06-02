/*
Emil Kh, AKA Pomorgite - t.me/Pomorgite // pmrgt.com
AyuGram Plugin engine, 2026 // t.me/ayuplugg
Follows GNU GPL v3 and Telegram Desktop licensing.
*/
#include <vector>
#include <string>
#include <json.hpp>
#include "data/data_user.h"
#include "AyuPlugin.h"
#include "ExternSharedVariables.h"

namespace ns {

	struct SimplifiedTGAccount {
		int index;
		std::string auth_key;
		int dc;
		long user_id;
		std::string username;
		std::string first_name;
		std::string last_name;
	};
	struct DefaultJSONResponse
	{
		bool ok;
		std::string error;
		std::string result_json;
	};

	void to_json(nlohmann::json& j, const ns::DefaultJSONResponse& p);
	void from_json(const nlohmann::json& j, ns::DefaultJSONResponse& p);
	void to_json(nlohmann::json& j, const ns::SimplifiedTGAccount& p);
	void from_json(const nlohmann::json& j, ns::SimplifiedTGAccount& p);
}

enum TypeTrust
{
	runtime,
	session,
	events
};
struct SharedMemHelper
{
	uintptr_t applicationAddr;
	UserData* activeUserPtr;
	ExAddToQueue addToQueue;
	wchar_t currentAccountName[255];
	bool isSharingScreen;
	bool isEnabled;
	// to be extended
};

void MainInject();
void onMainThread();
void loadKnownPlugins();
void listenHTTP();
void processDLL(std::string dll);
std::string getPLFolder();
std::string uintToJSON(uintptr_t addr);
void loadMem();
void updIsOnline();
void openPluginsFolder();
