#include "Main.h"
#include "Automation.h"
#pragma comment(lib, "gdi32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "winmm.lib") // For PlaySound
#pragma comment(lib, "shcore.lib") // GetDpiForMonitor

void SetGenerateMinidumpOnCrash();
void ApplyTheme();

std::array<ConstString, 3> g_encoding_names{ "UTF8 - Unicode", "CP-1252 - Windows", "CP-437 - DOS" };
RandomKISS g_random;

//
// Global Variables
//
static OwnedString g_starting_path=GetCurrentDirectory();

ConstString GetResourcePath()
{
   static OwnedString s_path;
   if(!s_path)
   {
      // The resource path is relative to the executable
      File::Path path{ModuleFileName()};
      // Remove the exe from the path to get the directory
      path.RemoveFilename();

      if(IsStoreApp())
         path.PopDirectory();
      path/="Assets";
      s_path=path;
   }

   return s_path;
}

ConstString GetAppDataPath()
{
   static OwnedString s_path;
   if(!s_path)
   {
      auto path=ExpandEnvironmentStrings("%APPDATA%\\BeipMU\\");

      // Ensure the directory exists
      CreateDirectory(path);

      s_path=path;
   }

   return s_path;
}

ConstString GetLocalAppDataPath()
{
   static OwnedString s_path;
   if(!s_path)
   {
      auto path=ExpandEnvironmentStrings("%LOCALAPPDATA%\\BeipMU\\");

      // Ensure the directory exists
      CreateDirectory(path);

      s_path=path;
   }

   return s_path;
}

ConstString GetStartupPath()
{
   return g_starting_path;
}

GlobalEvents::GlobalEvents()
{
   InitNewDayTimer();
   CallAtShutdown([](){ delete &GetInstance(); });
}

void GlobalEvents::InitNewDayTimer()
{
   uint64 filetime=Time::File(Time::Local());
   // We want a timer to fire right at midnight, so to do the math easily we'll set the time to midnight of the day before then add 24 hours.
   filetime=filetime-(filetime%Time::File::cDay)+Time::File::cDay;
   m_timer_new_day.Submit(Time::LocalToUTC(filetime), 0);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t *pCmdLine, int nCmdShow)
{
   g_random.SeedRandomDevice();
   SetGenerateMinidumpOnCrash();

   AppInit _;
   InitCommonControls();
   CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); CallAtShutdown([](){ CoUninitialize(); });
//   DebugOnly(MallocSpy_Register()); // In case of failure: http://support.microsoft.com/kb/894194/
   Sockets::Startup();
#if YARN
   DirectX::D3D _;
#endif

   if(!InitDWriteFactory() || !InitD2D1Factory1() || !InitWICImagingFactory() || !LoadConfig())
      return 0;
   Windows::g_dark_mode=g_ppropGlobal->fDarkMode(); // Set once at startup
   ApplyTheme();

   CreateWindow_Root(UTF8(WzToString(pCmdLine)), nCmdShow);

   return MessagePump();
}
