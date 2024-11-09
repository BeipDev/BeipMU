#include "LibWin32\LibWin32.h"
#include "Objects_Sockets2.h"
#include "Objects_Sockets_TLS.h"
#include "Text\TextWindow.h"
#include "Resource.h"
#include "BuildNumber.h"
#include "Keys.h"

// OLE
#include <OleCtl.h>
#include <dispex.h>
#define INITGUID
#include <initguid.h>

#undef CreateDialog
#undef PlaySound

#define WINDOWS_10_BUILD 1
#ifdef _DEBUG
#define BETA_BUILD 0
#define SCRIPTING 0
#define YARN 1
#define DWRITE_TEST 1
#else
#define BETA_BUILD 3
#define SCRIPTING 0
#define YARN 0
#define DWRITE_TEST 0
#endif

namespace OM
{
#include "OM.h"
};

using Sockets::Socket;

#include "Str_English.h"
//#include "Str_German.h"

#if BETA_BUILD!=0
#define TITLE_SUFFIX " - " STRINGIZE(BUILD_NUMBER) " BETA " STRINGIZE(BETA_BUILD)
#else
#define TITLE_SUFFIX
#endif

//#define gd_pcTitle "OmniMU"
#define gd_pcTitle "BeipMU" TITLE_SUFFIX
const unsigned g_ciVersion=400;
#define g_szVersion "4." STRINGIZE(BUILD_NUMBER)

#define GMCP_BEGIN "\xFF\xFA\xC9"
#define GMCP_END   "\xFF\xF0"

#if YARN
#undef gd_pcTitle
#define gd_pcTitle "Cyberfall" TITLE_SUFFIX
#endif

ConstString GetResourcePath();
ConstString GetAppDataPath();
ConstString GetLocalAppDataPath();
ConstString GetStartupPath();
ConstString GetConfigPath(); // Where the config file wound up being (startup or appdata)

//
// Error reporting interfaces (that use ConstStrings)
//
interface IError
{
   virtual void Error(Strings::ConstString string)=0;
   virtual void Text(Strings::ConstString string)=0;
};

struct ErrorConsole : IError
{
   void Error(ConstString string);
   void Text(ConstString string);

   unsigned m_count{};
};

struct ErrorCollection : IError, Collection<OwnedString>
{
   void Error(ConstString string);
   void Text(ConstString string);
};

void ConsoleHTML(ConstString string);
void ConsoleText(ConstString string);
void ConsoleDelete();

#include "RegEx.h"
#include "WinFunc.h"
#include "Imaging.h"
#include "ConfigParser.h"
#include "Extensions.h"
#include "Props.h"

struct Variable
{
   int operator<=>(ConstString name) const { return m_name<=>name; }
   bool operator==(ConstString name) const { return m_name==name; }

   OwnedString m_name, m_value;
};

extern RandomKISS g_random;
extern std::array<ConstString, 3> g_encoding_names;

UINT StateToShowCmd(Prop::Position::State state) noexcept;

[[nodiscard]] bool LoadConfig(); // Loads default config
void LoadConfig(ConstString filename, bool fImportingConfig=false);
void LoadConfig(Prop::Global &global, ConstString filename, IError &error);
bool SaveConfig(Window wnd, bool allow_gui=true);
void BackupConfig(IError &error);
void ResetConfig();

Prop::Global &GetSampleConfig();

struct GlobalTextSettingsModified { Prop::TextWindow &prop; };
struct GlobalInputSettingsModified { Prop::InputWindow &prop; };
extern Events::SendersOf<GlobalTextSettingsModified, GlobalInputSettingsModified> g_text_events;

Prop::TextWindow &GlobalTextSettings();
Prop::InputWindow &GlobalInputSettings();
void OnGlobalTextSettingsModified(Prop::TextWindow &prop); // Called by the text dialog when changing settings

void CreateDialog_KeyboardMacros(Window wnd, Prop::Server *ppropServer, Prop::Character *ppropCharacter);
void CloseDialog_KeyboardMacros();
void CreateDialog_Triggers(Window wnd, Prop::Server *ppropServer, Prop::Character *ppropCharacter, Prop::Trigger *ppropTrigger);
void CloseDialog_Triggers();
void CreateDialog_Aliases(Window wnd, Prop::Server *ppropServer, Prop::Character *ppropCharacter);
void CloseDialog_Aliases();
void CreateWindow_Root(ConstString cmdLine={}, int nCmdShow=SW_SHOWDEFAULT); // Create the First Window

void CreateWindow_Subscribe(Window wndParent);

bool ParseTimeInSeconds(ConstString word, float &seconds);


//
// Global Variables
//
struct Wnd_Main;
struct Wnd_MDI;
struct RestoreLogs;

extern Prop::Global *g_ppropGlobal;
extern UniquePtr<RestoreLogs> gp_restore_logs;

struct Event_NewWindow
{
   Event_NewWindow(Wnd_Main &window) : m_window(window) { }
   Wnd_Main &m_window;
};

struct Event_NewDay { };

struct GlobalEvents
 : Singleton<GlobalEvents>,
   Events::SendersOf<Event_NewWindow, Event_NewDay>
{
   GlobalEvents();

private:
   void InitNewDayTimer();

   [[no_unique_address]] UIThreadMessagePoster m_poster;
   Kernel::PoolTimer m_timer_new_day{[this](TP_CALLBACK_INSTANCE *pInstance, TP_TIMER *pTimer) { m_poster.Post([this]() { Send(Event_NewDay{}); InitNewDayTimer(); }); } };
};

void Global_PropChange();


// Called to prepare to handle events
interface IEvent_Prepare { virtual void Event_Prepare()=0; };
