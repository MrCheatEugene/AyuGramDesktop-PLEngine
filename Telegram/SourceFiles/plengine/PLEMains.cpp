/*
Emil Kh, AKA Pomorgite - t.me/Pomorgite // pmrgt.com
AyuGram Plugin engine, 2026 // t.me/ayuplugg
Follows GNU GPL v3 and Telegram Desktop licensing.
*/
#include "PLEMains.h"
#include "scheme.h"
#include <core/application.h>
#include "apiwrap.h"
#include <base/unixtime.h>
#include <Shlwapi.h>
#include "shellapi.h"
#undef small // since Small is used by data_session in TGAPI, and it's conflicting with shellapi
#include "MainQueue.h"
#include <ayu/utils/telegram_helpers.h>
#include "main/main_session.h"
#include "api/api_updates.h"
#include <stddef.h>
#include "httplib.h"
#include <thread>
#include "main/main_domain.h"
#include "crl/crl_on_main.h"
#include "main/main_account.h"
#include "data/data_changes.h"
#include "data/data_session.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

using json = nlohmann::json;
using namespace ns;
std::condition_variable cv;

namespace ns {
	void to_json(nlohmann::json& j, const ns::DefaultJSONResponse& p) {
		j = nlohmann::json{ {"ok", p.ok}, {"error", p.error}, {"result_json", p.result_json} };
	}

	void from_json(const nlohmann::json& j, ns::DefaultJSONResponse& p) {
		j.at("ok").get_to(p.ok);
		j.at("error").get_to(p.error);
		j.at("result_json").get_to(p.result_json);
	}

	void to_json(nlohmann::json& j, const ns::SimplifiedTGAccount& p) {
		j = json{ {"index", p.index},
				 {"auth_key", p.auth_key},
				 {"dc", p.dc},
				 {"user_id", p.user_id},
				 {"username", p.username},
				 {"first_name", p.first_name},
				 {"last_name", p.last_name}
		};
	}

	void from_json(const nlohmann::json& j, ns::SimplifiedTGAccount& p) {
		j.at("index").get_to(p.index);
		j.at("auth_key").get_to(p.auth_key);
		j.at("dc").get_to(p.dc);
		j.at("user_id").get_to(p.user_id);
		j.at("username").get_to(p.username);
		j.at("first_name").get_to(p.first_name);
		j.at("last_name").get_to(p.last_name);
	}
}

std::vector<std::string> fileNames = {};
int pollingRate = 1000; // ms
httplib::Server svr;

ApiWrap* activeAccWrapper;
SharedMemHelper memHelper;

std::queue<std::function<void()>>* GetQueuePtr() { return &MainQueue; }

void AddToQueue(const std::function<void()>& func) {
	std::cout << "Added function at address " << uintToJSON((uintptr_t)&func) << " to main queue.\n";
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		MainQueue.push(func);
	}
	cv.notify_one();
}

std::string OKResponse(bool ok = true, std::string errors = "", std::string res = "") {
	ns::DefaultJSONResponse aok = { ok, errors, res };
	json aokj = aok;
	return aokj.dump().c_str();
}

std::string to_hex_string(bytes::const_span data) {
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');
	for (auto b : data) {
		oss << std::setw(2) << static_cast<int>(gsl::to_integer<unsigned char>(b));
	}
	return oss.str();
}

std::string GetTrustedNameByType(TypeTrust t) {
	return std::string(t == TypeTrust::runtime ? "Remote Code Execution" : t == TypeTrust::session
		? "Sessions and accounts"
		: "Updates & Events");
}

std::vector<std::string>* GetTrustedByType(TypeTrust t) {
	return &(t == TypeTrust::runtime ? trusted.runtime : t == TypeTrust::session ? trusted.session : trusted.events);
}

std::string getUA(const httplib::Request& req) {
	return (req.has_header("User-Agent")
		? req.get_header_value("User-Agent")
		: (req.has_header("user-agent") ? req.get_header_value("user-agent") : "No user agent provided"));
}

bool IsTrustedUA(std::string ua, TypeTrust trust) {
	
	if (getenv("AYUPL_DISABLESECURITYFEATURES") != nullptr&& std::string(getenv("AYUPL_DISABLESECURITYFEATURES")) == "1") {
		printf("Trust checks are skipped, since AYUPL_DISABLESECURITYFEATURES is set.\n Requested trust elevation type: %s\n", GetTrustedNameByType(trust).c_str());
		return true;
	}
	auto trustedScope = GetTrustedByType(trust);
	bool good = std::find(trustedScope->begin(), trustedScope->end(), ua) != trustedScope->end();
	if (!good) {
		wchar_t a[2048];
		mbstowcs(a,
			std::string("Some application or plug-in is trying to elevate their permissions. Type of elevation: " +
				GetTrustedNameByType(trust) +
				".  Please confirm, or "
				"decline this request. U/A:" +
				ua)
			.c_str(),
			2048);

		good = IDOK == MessageBox(NULL, a, L"Security warning", MB_OKCANCEL);
		if (good) {
			trustedScope->push_back(ua);
		}
	}

	return good;
}

void updMemHelp() {
	while (1) {
		memHelper.applicationAddr = (uintptr_t)Core::Application::Instance;
		memHelper.isSharingScreen = Core::App().isSharingScreen();
		memHelper.addToQueue = &AddToQueue;
		memHelper.activeUserPtr = Core::App().activeAccount().session().user();
		mbstowcs(memHelper.currentAccountName, memHelper.activeUserPtr->name().toStdString().c_str(), 255);
		Sleep(pollingRate);
	}
}

void updIsOnline() {
	while (true) {
		for (size_t i = 0; i < FunctionsOnIsOnline.size(); i++) {
			if ((bool)FunctionsOnIsOnline.at(i)() == true) {
				if (const auto controller = Core::App().activeWindow()->sessionController()) {
					controller->session()
						.updates()
						.api()
						.request(MTPaccount_UpdateStatus(MTP_bool(false)))
						.send();
				}
				break;
			}
		}
		Sleep(2000);
	}
}

void openPluginsFolder()
{
	std::system(("explorer \""+ getPLFolder()+"\"").c_str());
}

void loadMem() {
	if (memHelper.isEnabled == false) {
		memHelper.isEnabled = true;
		std::thread th(updMemHelp);
		th.detach();
	}
}

std::string uintToJSON(uintptr_t addr) {
	json b;
	char addrv[128];
	sprintf(addrv, "%p", addr);
	b["address"] = addrv;
	return b.dump();
}

std::string getPLFolder() {
	char* path = new char[2048];
	GetCurrentDirectoryA(2048, path);
	std::string s = path;
	return s + "\\plugins";
}

void processDLL(std::string dll) {
	char* path = new char[2048];
	PathCombineA(path, getPLFolder().c_str(), dll.c_str());
	std::string plFolder = path;
	wchar_t b[2048];
	mbstowcs(b, plFolder.c_str(), 2048);
	HINSTANCE dllInstance =
		LoadLibrary(b);

	if (dllInstance == NULL) {
		wchar_t a[512];
		std::string err = std::string("DLL ") + dll + std::string(" failed to load with code ") + std::to_string(GetLastError());
		mbstowcs(a, err.c_str(),
			512);
		std::cout << ((err + std::string("\n")).c_str());
		MessageBox(NULL, a, L"Plugin load error", MB_OK);
		return;
	}

	AyuPlugin* pl = ((InternalPluginInfo)GetProcAddress(dllInstance, "pluginInfo"))();
	if (pl == NULL) {
		wchar_t a[512];
		std::string err = std::string("DLL ") + dll + std::string(" 's global structure failed to load with code  ") + std::to_string(GetLastError());
		std::cout << ((err + std::string("\n")).c_str());
		mbstowcs(
			a, err.c_str(), 512);

		MessageBox(NULL, a, L"Plugin execution error", MB_OK);
		return;
	}
	
	InternalSetup set_func = (InternalSetup)GetProcAddress(dllInstance, "internalSetup");
	InternalLoop loop_func = (InternalLoop)GetProcAddress(dllInstance, "internalLoop");
	InternalDoFilterHistoryItem func = (InternalDoFilterHistoryItem)GetProcAddress(dllInstance, "doFilterHistoryItem");
	InternalDoPreProcessMessage func2 =
		(InternalDoPreProcessMessage)GetProcAddress(dllInstance, "doPreProcessMessage");

	InternalIsOnline func4 = (InternalIsOnline)GetProcAddress(dllInstance, "doReturnIsOnline");
	InternalExcludeDeletion func5 = (InternalExcludeDeletion)GetProcAddress(dllInstance, "doExcludeDeleted");
	InternalDrawGUI func6 = (InternalDrawGUI)GetProcAddress(dllInstance, "doDrawGUI");

	PluginData d;
	char _b[2048];
	wcstombs(_b, pl->name, 2048);
	char __b[2048];
	wcstombs(__b, pl->description, 2048);
	d.name = _b;
	d.description = __b;
	d.usesHooks = func2 != NULL || func != NULL || func4 != NULL|| func5 != NULL;
	d.modulePath = dll;
	d.hooksList = "";

	if (func != NULL) {
		FunctionsOnFilter.push_back(func);
		d.hooksList += "Shared filters hook (can filter out messages before shared filters) \n";
		std::cout << ("Got shared filters function handle!\n");
	}
	if (func2 != NULL) {
		FunctionsOnPrepare.push_back(func2);
		d.hooksList += "Message preparation hook (can intercept your messages, and change them on send time) \n";
		std::cout << ("Got message preparation function handle!\n");
	}
	if (func4 != NULL) {
		FunctionsOnIsOnline.push_back(func4);
		d.hooksList += "Online hook (can modify your online state) \n";
		std::cout << ("Got online function handle!\n");
	}
	if (func5 != NULL) {
		FunctionsExcludeDeleted.push_back(func5);
		d.hooksList += "Exclude deletion hook (can exclude messages out of \"save deleted\") \n";
		std::cout << ("Got InternalExcludeDeletion function handle!\n");
	}
	if (func6 != NULL) {
		FunctionsDrawGUI.push_back(func6);
		d.drawGUI = func6;
		d.hooksList += "Can draw GUI \n";
		std::cout << ("Got InternalExcludeDeletion function handle!\n");
	}
	pl->memData.activeUserPtr = (uintptr_t)(UserData*)(Core::App().activeAccount().session().user());
	pl->memData.activeSessionPtr = (uintptr_t)(&Core::App().activeAccount().session());
	pl->memData.addToQueue = &AddToQueue;
	pl->memData.applicationAddr = (uintptr_t)Core::Application::Instance;
	std::cout << ("Shared memory pointers successfully!\n");

	if (set_func != NULL) {
		d.hooksList += "Has Setup function (will run once on startup) \n";
	}
	if (loop_func != NULL) {
		d.hooksList += "Has Loop function (will run in a loop) \n";
	}
	pluginsData.push_back(d);

	if(set_func!=NULL){
		std::cout << ("Exec-ing setup..\n");
		set_func();
	}
	if (loop_func != NULL) {
		while (true) {
			loop_func();
		}
	}
}

void listenHTTP() {
	svr.Get("/api/ping",
		[](const httplib::Request&, httplib::Response& res)
		{ res.set_content(OKResponse(), "application/json"); });

	svr.Get("/api/runtime/setPolling",
		[](const httplib::Request& req, httplib::Response& res)
		{
			std::string ua = getUA(req);
			std::vector<ns::SimplifiedTGAccount> accs_2;
			bool good = IsTrustedUA(ua, TypeTrust::runtime);
			if (good && memHelper.isEnabled) {
				int r = std::stoi(req.get_param_value("rate"));
				r = r <= 10 ? 10 : r;
				pollingRate = r;
			}
			res.set_content(
				OKResponse(good,
					good ? (memHelper.isEnabled ? "" : "Nobody's updating MemHelper, though.") : "User rejected trust elevation request.",
					good ? uintToJSON((uintptr_t)&memHelper) : "null"),
				"application/json");
		});

	svr.Get("/api/runtime/load",
		[](const httplib::Request& req, httplib::Response& res)
		{
			std::string ua = getUA(req);
			std::vector<ns::SimplifiedTGAccount> accs_2;
			bool good = IsTrustedUA(ua, TypeTrust::runtime);
			if (good) {
				loadMem();
				std::string libName = req.get_param_value("name");
				std::thread t(processDLL, libName);
				t.detach();
			}
			res.set_content(OKResponse(good,
				good ? "" : "User rejected trust elevation request.",
				good ? uintToJSON((uintptr_t)&memHelper) : "null"),
				"application/json");
		});

	svr.Get("/api/runtime/export",
		[](const httplib::Request& req, httplib::Response& res)
		{
			std::string ua = getUA(req);
			std::vector<ns::SimplifiedTGAccount> accs_2;
			bool good = IsTrustedUA(ua, TypeTrust::runtime);
			if (good) {
				loadMem();
			}
			res.set_content(OKResponse(good,
				good ? "" : "User rejected trust elevation request.",
				good ? uintToJSON((uintptr_t)&memHelper) : "null"),
				"application/json");
		});

	svr.Get("/api/session/get",
		[](const httplib::Request& req, httplib::Response& res)
		{
			std::string ua = getUA(req);
			std::vector<ns::SimplifiedTGAccount> accs_2;
			bool good = IsTrustedUA(ua, TypeTrust::session);
			if (good) {
				for (const auto& [index, account] : Core::App().domain().accounts()) {
					if (!account->sessionExists()) {
						continue;
					}
					auto session = &account->session();
					auto userdata = session->user().get();
					auto keylist = session->mtp().getKeysForWrite();
					auto dcid = session->mainDcId();
					std::string hexedKey = "";
					for (int ii = 0; ii < keylist.size(); ii++) {
						if (keylist.at(ii)->dcId() == dcid) {
							hexedKey = to_hex_string(keylist.at(ii)->data());
						}
					}
					ns::SimplifiedTGAccount a{ index,
											  hexedKey,
											  dcid,
											  session->userPeerId().value & UserId::kShift,
											  userdata->username().toStdString(),
											  userdata->firstName.toStdString(),
											  userdata->lastName.toStdString() };
					accs_2.push_back(a);
				}
			}
			json response = accs_2;
			res.set_content(
				OKResponse(good, good ? "" : "User rejected trust elevation request.", good ? response.dump() : "null"),
				"application/json");
		});
	char host[128];
	GetEnvironmentVariableA("AYUPL_HOST", host, 127);
	char port[6];
	GetEnvironmentVariableA("AYUPL_PORT", port, 5);
	int portx = atoi(port);
	bool val = portx >= 1 && portx < 65535;
	if (!val) {
		std::cout << "Port is not specified, API is disabled\r\n";
		return;
	}
	svr.listen(host, val ? portx : 8080);
}



void loadKnownPlugins() {
	std::cout << ("Sleeping for 5 seconds, then loading the plugins..\n\n");
	Sleep(5000);
	for (auto b : fileNames) {
		std::cout << ((std::string("Loading ") + b + std::string("\n")).c_str());

		std::thread t(processDLL, b);
		t.detach();
	}
	std::cout << ("Finished loading plugins!\n\n");
}


void onMainThread() {
	while (true) {
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			cv.wait(lock, [] {
				return !MainQueue.empty();
			});

			task = MainQueue.front();
			MainQueue.pop();
		}

		if (task) {
			dispatchToMainThread(task); // thanks AlexeyZavar! 
		}
	}
}

void MainInject() {
	if (getenv("AYUPL_CONSOLE") != nullptr && std::string(getenv("AYUPL_CONSOLE"))=="1") {
		AllocConsole();
		SetConsoleTitleA("AyuGram Plugin Engine by Pomorgite");
		typedef struct
		{
			char* _ptr;
			int _cnt;
			char* _base;
			int _flag;
			int _file;
			int _charbuf;
			int _bufsiz;
			char* _tmpfname;
		} FILE_COMPLETE;
		*(FILE_COMPLETE*)stdout =
			*(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT), "w");
		*(FILE_COMPLETE*)stderr =
			*(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT), "w");
		*(FILE_COMPLETE*)stdin =
			*(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_TEXT), "r");
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
		setvbuf(stdin, NULL, _IONBF, 0);
	}


	std::cout << "Plugin engine is loading.\n\n";
	wchar_t plFolder[2048];
	std::string plF = getPLFolder();
	std::cout << (std::string("Plugin folder: ") + plF + "\n").c_str();
	mbstowcs(plFolder, plF.c_str(), 2048);
	wchar_t plFiles[2048];
	mbstowcs(plFiles, (plF + "\\*").c_str(), 2048);
	CreateDirectory(plFolder, NULL);
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(plFiles, &ffd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				char DefChar = ' ';
				char* szTo = new char[2048];
				WideCharToMultiByte(CP_ACP, 0, ffd.cFileName, -1, szTo, 260, &DefChar, NULL);

				char* path = new char[2048];
				PathCombineA(path, plF.c_str(), szTo);
				std::string x = path;
				fileNames.push_back(x);
				char to[4096];
				sprintf(to, "Found plugin: %s \n", szTo);
				std::cout << (to);
			}
		} while (FindNextFile(hFind, &ffd));

		FindClose(hFind);
	}
	else {
		std::cout << ("Plugins folder was not found: Invalid handle\n");
	}
	std::thread a2(loadKnownPlugins);
	a2.detach();

	std::thread t(listenHTTP);
	t.detach();
	std::thread t2(updIsOnline);
	t2.detach();
	std::thread t3(onMainThread); // main queue 
	t3.detach();
}