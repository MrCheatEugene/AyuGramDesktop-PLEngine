# Plugin engine

This is a documentation for AyuGram Plugin engine.<br>
We expect that you're familiar with Building Ayugram Desktop, or with C/C++ in general. <br>
A sample plugin can be found here - https://github.com/MrCheatEugene/AyuSamplePlugin<br>
You need to change the project file to have Ayugram source folders defined accordingly. <br>
For proper development of the plugins, you also need to build this version of Ayugram locally.<br>

## How it works?
It works really simple. 
- We have a DLL, that defines certain functions and exports them
- We load that DLL in a separate thread, and run everything in a separate thread
- We execute those functions, in runtime. While Plugin still has some access to internal functions, and API's.

## How a plugin works?
Plugin defines [AyuPlugin](/Telegram/SourceFiles/AyuPlugin.h) structure, a pointer to that structure, and a function to return it.  <br>
```cpp
AyuPlugin _pluginInfo{
    L"Sample Plugin",
    L"A plugin, demonstrating how AyuGram can accept dynamic-library plugins, and do cool stuff with it.",
    L"AyuSamplePlugin.dll",
    NULL // NULL, because we're re-defining allocated memory space, yk?
}; // end structure

AyuPlugin* _ppluginInfo = &_pluginInfo; // static ptr

EXTERN_DLL_EXPORT AyuPlugin* pluginInfo() { // helper for external access, SHOULD ALWAYS BE pluginInfo
    return _ppluginInfo;
}
```

AyuPlugin struct defines like that, usually: <br>
```cpp 
struct MemData
{
	uintptr_t applicationAddr; 
	uintptr_t activeUserPtr; // a pointer of a current User - Core::App().activeAccount().session().user()
};

struct MemData
{
	uintptr_t applicationAddr; // a pointer to Core::App()
	uintptr_t activeUserPtr; // a pointer of a current User - Core::App().activeAccount().session().user()
	uintptr_t activeSessionPtr; // a pointer of a current Session - Core::App().activeAccount().session()
	ExAddToQueue addToQueue; // a pointer to addToQueue function
};

struct AyuPlugin
{
	wchar_t name[128]; // plugin name (will be shown when plugin loads)
	wchar_t description[255]; // plugin description (shown in user GUI)
	wchar_t moduleName[128]; // module name, for future use
	MemData memData; // MemData struct 
};
// 2/5/2026: bool sharedFiltersEnabled is now deprecated and removed.
```

When a plugin loads, AyuGram process gets that structure and:
- sets MemData
- shows the user whatever plugin is loaded (by name)
- if it fails, it shows them the error code

## API information, declarations of functions

### DLLMain
DLLMain should return True if the plugin wants to load. It can be set to false, if you want to restrict load, for example.

### GUI operations
If you want for the user to be able to open a page with your Plugin's GUI, you can do so by declaring a `doDrawGUI` function.<br>
```cpp
// will be run once user goes to plugin settings
// no "addToQueue" is required, since we're doing it already on main thread
EXTERN_DLL_EXPORT void doDrawGUI(Settings::Builder::SectionBuilder& builder, Settings::AyBuilder::AyuSectionBuilder& ayu, Settings::PLEPlugins* ple){
    builder.addDivider();
    builder.addDividerText(rpl::single(QString("Hello from PLEngine!")));
    builder.addButton({ 
        .title = rpl::single(QString("Test button from plugin")),
        .label = rpl::single(QString("I'm cool")),
        .onClick = [=] {
            MessageBoxA(NULL, "Hello!", "user interaction!!", MB_OK);
         }
        });
    builder.addSkip();
    builder.addDivider();
}
```
Please note: not all builder features are exported, and with forced linking, they can cause Access Violation exceptions.<br>
This is why, in this example (for now), we're using MessageBox, and not `builder.controller()->showToast`.<br>
See [Settings::Builder](/Telegram/SourceFiles/ayu/features/settings/builder.h) for more details on what you can do with it, and what features are exported.<br>
Or, take a look at code using Settings::Builder, and try to replicate it by yourself. <br>
This is a new and not fully developed feature, so expect some crashes if you try to use it, and report them if you do.<br>
This function will be executed on the main thread, when the user clicks the Plugin button in settings, so you don't have to worry about thread-safety here.<br>
Below your GUI, an informational piece of GUI will be always shown for user experience purposes. You can't do anything about it. <br>

### Main Loop and Thread-safe operations
Remember: plugins always work in a separate thread, and have VERY LITTLE ACCESS to AyuGram's memory.<br>
This is why, for example, when you work with ApiWrap, you should always wrap that into a void function, and pass it onto the Main Queue.<br>
Function will be passed onto the main thread, by dispatching it inside an instant-firing QTimer [(see dispatchToMainThread definition)](/Telegram/SourceFiles/ayu/utils/telegram_helpers.cpp#L85).<br>

For you, it's as simple as:
```cpp
// app, session definitions..

auto x = [app, session] {
	// your unsafe operation is now safe here!
};
_pluginInfo.memData.addToQueue(x);
```
While you **can ignore main queue**, and think it's useless, remember: your operations **may and will fail unexpectedly**, if you run it "as is" in your own thread.<br>
Following operations are usually more stable in main thread:
- API calls
- Reading, and calling functions within any deep-level structures, like Sessions
- ANYTHING that involves updating the UI, in the function chain (e.g sending a message via the API, causes HistoryItem list to be updated, which in the end involves animations, which are not thread-safe)

### Debugging tips
1. Enable `/DEBUG` when building the Plugin, and Load Debug Symbols if the autoload doesn't pick it up - [guide](https://learn.microsoft.com/en-us/visualstudio/debugger/how-to-use-the-modules-window?view=visualstudio)
2. Check call stacks, and calls between the threads. You can find a lot of useful details there.
3. Null pointers are the most frequent issue you'll be facing, so when you see "access violation", and you have a bunch of one-liner code that's causing it, split it into variables and inspect it in debugger then.

### Direct API (ApiWrap, limited) calls
You can call some ApiWrap functions. Not all of them are exported, due to them making the application unstable, see `Explaining why we can't use ApiWrap -> request` below for more details.<br>
See [apiwrap.h](/Telegram/SourceFiles/apiwrap.h), and look for `__declspec(dllexport)` declarared-functions. Those can technically be used, via ApiWrap.<br>
Not all of them were tested: if Linking fails, due to it requiring a class that's not exported, you can submit an Issue, or try exporting the class or needed methods yourself with the same `__declspec(dllexport` declaration.<br>
ApiWrap calls are not recommended, though, as they may require more exported classes/wrappers on linkage. Stick to MTProto calls when possible.<br>
Example of an ApiWrap API call:
```
PeerId peerID = s->userPeerId(); // a shortcut to saved messages (chat with your active account); you can get chat or channel, like: peerFromChannel(ChannelId(3871594897ULL)); 
History* history = s->data().history(peerID); // Get the history 
if (!history) { 
	MessageBoxA(nullptr, "Couldn't load history.", "Debug", MB_OK ); 
	return;
}
auto message = Api::MessageToSend(Api::SendAction(history)); // Construct a simple message, ...
message.textWithTags = { "Hi from Plugin Engine! ✨", {} };  // ..then fill it up with actual content! textWithTags is a Struct, by the way, so we can do it in one line, yay!
s->api().sendMessage(std::move(message)); // then we call the Active Session API, and send the message! (yeah it needs std::move)
```

### Direct API (MTProto) calls
You can make different MTProto calls via: `s->api().instancePtr()->sendReq(request, callback)`<br>

Request should not be Serialized: but wrapped in `tl::boxed`, like:<br>
```cpp
using MTPaccountupdateProfile = tl::boxed<MTPaccount_updateProfile>;
// using YourLocalName = tl::boxed<MTPrequestNameThatsNotWrapped>;

// ...
s->api().instancePtr()->sendReq(MTPaccountupdateProfile(...));
```

Some requests don't have to be wrapped in tl::boxed, **as they already are**. Check scheme.cpp, scheme.h for that reason (those are auto-generated by CMake).

Example of `tl::boxed` request:
```cpp
using MTPaccountupdateProfile = tl::boxed<MTPaccount_updateProfile>; // boxed is required here, because sendReq doesn't understand unboxed requests
auto r = MTPaccountupdateProfile(
    MTP_flags(MTPaccountupdateProfile::Flag::f_about),
    MTP_string(),
    MTP_string(),
    MTP_string("Updated bio via @ayuplugg ✨")
);
s->api().instancePtr()->sendReq(
    r
);
```

Example of an already `tl::boxed` request:
```cpp
auto r = MTPaccount_SaveMusic( // scheme.h has "using MTPaccount_SaveMusic = tl::boxed<MTPaccount_saveMusic>;", making it already boxed.
    MTP_flags(0),
    mI,
    MTPInputDocument()
); // form the request

i->sendReq(r);
```

#### Explaining why we can't use ApiWrap -> request
Telegram wraps every MTProto call in ApiWrap. ApiWrap is a parent of MTP::Sender. <br>
It pushes the request into the queue, where it processes them from now on.<br>
Issue is, we can't just export MTP::Sender in .lib file (which's needed for the plugin to link properly), because other functions will attempt to copy RequestWrap, instead of moving it.<br>
To use a certain Class or type in External DLL, it has to be exported for linking. Exporting ApiWrap, makes RequestWrap exportable, too, which leads to a lot of dumb issues I'm too lazy of solving.

Instead, we use sendReq: it's similar to [MTP::Instance::send](/Telegram/SourceFiles/mtproto/mtp_instance.h), instead, it does all the job at once, instead of having multiple overloads, calling sendRequest in 10 steps, it does the exact same as ::send. Yet it's exported properly, and Linker on the plugin is happy with it.

### Setup Call
You can define a setup function.<br>
`EXTERN_DLL_EXPORT void internalSetup() {`<br>
It'll be ran once by the engine, with all of the memory pointers loaded in. Loop will not start, til the function ends.


### Internal Loop
You can define a loop. <br>
`EXTERN_DLL_EXPORT void internalLoop() {` <br>
That's it. It's a loop. It'll run in an another loop function in the thread, so you don't have to do: 
```cpp
EXTERN_DLL_EXPORT void internalLoop() {
  while (1) {
    // do stuff
  }
}
```
There's nothing much to say about it.

## Hooks

### Rules of hooks
1. Hook should always follow it's defined signature (they're defined in AyuPlugin.h, by the way) and name
2. "Hooks" are stored globally, and accessed in a `for` loop.
3. On BOOL return types of hook functions, the first hook to return TRUE stops the other hook functions from being called
4. There's no "mandatory" hooks. You can't force the plugin to ever, ever, be required to define any kind of hook!
5. Exception to rule 4: Hook chains. Although, currently unused, if there's some hook in the future that may depend on another hook, hook chains are allowed. In that case, ALL of the chained hooks should be present. 

### Shared Filters hook
Must be defined as: `EXTERN_DLL_EXPORT InternalDoFilterHistoryItem doFilterHistoryItem(HistoryItem* HistoryItem)`<br>
Remember the struct: 
```cpp
enum FilteredState
{
	NoMatch,
	Filtered,
	RejectFurtherFiltering
};
```
Hook should return:
- NoMatch if it didn't match anything (filtering function continues to run)
- Filtered if it matches/Item should be filtered (filtered function stops at that place)
- RejectFurtherFiltering if Item shouldn't be filtered, and the function should return that the item is NOT filtered (filtered function returns false)

If it RETURNS TRUE, the [filtered function](/Telegram/SourceFiles/ayu/features/filters/filters_controller.cpp#L125C1-L125C51) will filter the matched historyItem. 

### ExcludeDeletion Hook
Must be defined as: `EXTERN_DLL_EXPORT InternalExcludeDeletion doExcludeDeleted(HistoryItem* i) {`<br>
If it returns TRUE, messages that are Deleted will NOT be captured and stored in the database by AyuGram. <br>
Example: 
```cpp
EXTERN_DLL_EXPORT InternalExcludeDeletion doExcludeDeleted(HistoryItem* i) {
	return (InternalExcludeDeletion)i->from()->username().contains("pmrgt"); // exclude deletion of messages from accounts with "pmrgt" in their username, just as a demo
}
```

### "Online" status hook
If at least one loaded plugin, returns `true`, Ayugram will send an MTProto request for the current user: <br>
`MTPaccount_UpdateStatus(MTP_bool(false))`

Must be defined as: `EXTERN_DLL_EXPORT InternalIsOnline doReturnIsOnline()` <br>
An example usage: <br>
```

DWORD64 GetIdleTimeMs()
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);

    if (!GetLastInputInfo(&lii))
        return (DWORD64)-1;

    return GetTickCount64() - lii.dwTime;
}

EXTERN_DLL_EXPORT InternalIsOnline doReturnIsOnline()
{
    /*
        Example function that demonstrates Idle online
    */
    const DWORD64 IDLE_LIMIT_MS = 10 * 1000; // 10 seconds

    DWORD64 idleMs = GetIdleTimeMs();
    if (idleMs == (DWORD64)-1)
        return (InternalIsOnline)false;

    return (InternalIsOnline)(idleMs < IDLE_LIMIT_MS);
}
```

### Message hook
It interrupts [this function](/Telegram/SourceFiles/apiwrap.h#L368) in ApiWrap:

```cpp
	void sendMessage(
		MessageToSend &&message,
		std::optional<MsgId> localMessageId = std::nullopt);
```
Must be defined as: `EXTERN_DLL_EXPORT InternalDoPreProcessMessage doPreProcessMessage(char* in, char* out)`  <br>
An example:
			
```cpp
EXTERN_DLL_EXPORT InternalDoPreProcessMessage doPreProcessMessage(char* in, char* out) {
    std::string i(in);
    i=std::regex_replace(i, std::regex("hello"), "replaced");
    strcpy(out, i.c_str());
    out[4096] = '\0';
}
```

## Guides 

### How to load a plugin (via Internal Loader)
1. Put the plugin into "plugins" folder
2. Launch a build of PLEngine AyuGram, and wait.
3. Done, it'll be loaded on startup

### How to load a plugin (via API)
1. Put the plugin into "plugins" folder
2. Launch a build of PLEngine AyuGram, and do an HTTP request to: `http://127.0.0.1:8080/api/runtime/load?name=AyuSamplePlugin.dll` , where name query param is your module name. You may be prompted by a message box, once per every launch. This is normal.
3. Done. Check AyuGram for messageboxes with the debug info.
4. If it crashes, sorry, use a debugger.

## ENV variables
|Variable | Meaning | Default|
| --- | --- | --- |
|`AYUPL_HOST` | Host that'll be used when launching the API | `127.0.0.1`|
|`AYUPL_PORT` | Port that'll be used when launching the API | `8080`|
|`AYUPL_CONSOLE` | If it exists, a separate console on a main thread showing debug info will be shown. | Not Set|

## HTTP API 
PLEngine HTTP API by default runs on 127.0.0.1:8080 (if not redefined).
It can be used for external applications (or plugins) to do stuff, externally. Or even do stuff, without loading a module into AyuGram. 

### Trust elevation 
Whenever you run a certain endpoint, your User-Agent isn't trusted by default.
You may be prompted by a MessageBox from Ayugram. This is made as a simple wall in case something accidentally calls the methods, or tries to maliciously exploit them (||tbh it's not a good security measure, just don't install junk on your PC||)
Trust elevated functions may fail ("ok": "false") if the trust request is rejected.
Each functions can be linked to a trust space: session, runtime, events. Once a user-agent is trusted in a space, it can call the API next time without prompting the user.

### Response
Expect a response of:
```json
{
  "ok": "true/false",
  "error": "human readable error",
  "result_json": "function-specific payload in JSON as string"
}
```

### `/api/ping` 
Ping it. It'll always return "ok": "true"
### `/api/runtime/setPolling?rate=INT`
Trust space "Runtime".
Set polling rate for pointers and updating MemHelper internal structure, where RATE is in miliseconds.
### `/api/runtime/load?name=module.dll`
Trust space "Runtime".
Load a module, see "How to load a plugin" above.
Returns an address of MemHelper.
### `/api/runtime/export`
Trust space "Runtime".
Load MemHelper, start polling pointers. Returns a pointer to the MemHelper internal structure, in case you're managing AyuGram externally.
### `/api/session/get`
Trust space "Session".
Returns a list of loaded Ayugram sessions with structs like:
```cpp
struct SimplifiedTGAccount
{
	int index;
	std::string auth_key;
	int dc;
	long user_id;
	std::string username;
	std::string first_name;
	std::string last_name;
};

void to_json(json &j, const SimplifiedTGAccount &p) {
	j = json{{"index", p.index},
			 {"auth_key", p.auth_key},
			 {"dc", p.dc},
			 {"user_id", p.user_id},
			 {"username", p.username},
			 {"first_name", p.first_name},
			 {"last_name", p.last_name}
	};
}
```


