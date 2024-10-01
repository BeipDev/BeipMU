// Main Window Routines

#include "Main.h"
#include "Wnd_Taskbar.h"

#include "Wnd_Main.h"
#include "Scripter.h"
#include "Scripter\CScript.h"

#include "Wnd_Main_OM.h"
#include "NotificationIcon.h"
#include "Wnd_MuckNet.h"
#include "Wnd_HistoryGraph.h"

#include "Emoji.h"
#include "TileMap.h"
#include "AnimatedGif.h"
#include "ImagePane.h"
#include "Wnd_Map.h"
#include "Wnd_Stats.h"
#include "Speller.h"
#include "FindString.h"
#ifdef YARN
#include "DirectX\DirectX.h"
#include "Server\Protocol.h"
#include "Yarn\IYarn_Tilemap.h"
namespace Yarn { struct Wnd_Windows; }
#endif
#include "WebView.h"

Prop::Docking g_propDockingClipboard;
static CntPtrTo<Prop::MainWindowSettings> g_ppropMainWindowSettingsClipboard=MakeCounting<Prop::MainWindowSettings>();

void SetBadgeNumber(int num);
void CreateSplashScreen(Window wndParent);
void CreateDialog_TextWindow(Window wndParent, Text::Wnd &text_window, Prop::TextWindow &propTextWindow);
void CreateDialog_Connect(Window wnd, Wnd_MDI &wndMDI);
void CreateDialog_SmartPaste(Window wndParent, Connection &connection, Prop::Connections &propConnections);
void CreateDialog_About(Window wnd);
void CreateDialog_InputWindow(Window wndParent, InputControl &input_window, Prop::InputWindow &propInputWindow);
void CreateDialog_Find(Window wnd, Text::Wnd &wndText);
void CreateDialog_Settings(Window wndParent);

void CreateWindow_About(Window wndParent);
void D2DTest();

void ShowStatistics(Text::Wnd &wnd)
{
   uint64 totalSecondsConnected=0;
   uint64 totalBytes=0;
   uint64 totalConnections=0;

   for(auto &pServer : g_ppropGlobal->propConnections().propServers())
      for(auto &pCharacter : pServer->propCharacters())
      {
         totalSecondsConnected+=pCharacter->iSecondsConnected();
         totalBytes+=pCharacter->iBytesReceived()+pCharacter->iBytesSent();
         totalConnections+=pCharacter->iConnectionCount();
      }

   HybridStringBuilder string(
      R"(<p background-color='#004000' stroke-color='#008000' stroke-width='2' border='10' border-style='round' indent='5' padding='2'><font color='white' face='Arial'>)"
      "<font color='yellow'>Total time connected: <font color='white'>");
   Time::SecondsToStringAbbreviated(string, totalSecondsConnected);
   string(CRLF "<font color='yellow'>Total data: <font color='white'>"); ByteCountToStringAbbreviated(string, totalBytes);
   string(CRLF "<font color='yellow'>Total connections: <font color='white'>", Strings::CommaUInt64(totalConnections));

   wnd.AddHTML(string);
}

void Append(PopupMenu &menu, UINT_PTR id, ConstString label, Wnd_Main::Keys key_id)
{
   if(auto &key=g_ppropGlobal->propKeys().Get(key_id))
   {
      FixedStringBuilder<256> string{label, '\t'};
      key.KeyNameWithModifiers(string);
      menu.Append(MF_STRING, id, string);
   }
   else
      menu.Append(MF_STRING, id, label);
}

namespace Text
{

struct IconInfo
{
   ConstString m_tag; bool m_fSystem; WORD m_resourceID; ConstString m_altText;
};

static constinit const IconInfo s_infos[]=
{
   { "information",  true,  (intptr_t)IDI_INFORMATION, "ℹ️" },
   { "exclamation",  true,  (intptr_t)IDI_EXCLAMATION, "⚠️" },
   { "error",        true,  (intptr_t)IDI_ERROR,       "⛔" },
   { "app",          false, IDI_APP,                   "🦎" },
};

HICON s_icons[_countof(s_infos)]{};

Handle<HBITMAP> CreateBitmap(ConstString source)
{
   return Imaging::LoadImageFromFile(source);
}

unsigned GetIconCount() noexcept { return _countof(s_infos); }
ConstString GetIconTag(unsigned index) noexcept { return s_infos[index].m_tag; }
ConstString GetIconAltText(unsigned index) noexcept { return s_infos[index].m_altText; }
HICON GetIcon(unsigned index) noexcept
{
   Assert(index<GetIconCount());

   if(!s_icons[index])
      s_icons[index]=LoadIcon(s_infos[index].m_fSystem ? 0 : g_hInst, MAKEINTRESOURCE(s_infos[index].m_resourceID));

   return s_icons[index];
}

}

void ErrorConsole::Error(ConstString string)
{
   ConsoleHTML(HybridStringBuilder("<icon error><font color='red'> ", string));
   m_count++;
}

void ErrorConsole::Text(ConstString string)
{
   ConsoleHTML(string);
   m_count++;
}

void ErrorCollection::Error(ConstString string)
{
   OwnedString &str=Push(string);
   str.Insert(0, "<icon error><font color='red'>");
}

void ErrorCollection::Text(ConstString string)
{
   Push(string);
}

static bool g_fUseStartupPathForConfig{};
Prop::Global *g_ppropGlobal{};
UniquePtr<RestoreLogs> gp_restore_logs;
UniquePtr<Prop::Global> g_ppropSample;

File::File g_config_lock;

bool LoadConfig()
{
   auto name=ConstString("Config.txt");

   // First look for a config.txt in the startup folder, as that's how people used to use it
   File::Path path(name);
   if(!IsStoreApp() && path.Exists())
      g_fUseStartupPathForConfig=true;
   else
   {
      // Look in the %APPDATA%\BeipMU folder
      path.Clear();
      path(GetAppDataPath(), name);

      if(!path.Exists())
      {
         // At this point there is no user config.txt file, so copy the default config in it's place if it exists
         File::Path defaultConfig(GetResourcePath(), "DefaultConfig.txt");
         if(defaultConfig.Exists() && !CopyFile(defaultConfig, path, false /*FailIfExists*/))
         {
            MessageBox(nullptr, HybridStringBuilder("Could not copy default config file: (", LastError{}, ")"), "Error", MB_ICONERROR|MB_OK);
            return false; // Couldn't copy to the default, so give up now
         }
      }
   }

   if(!g_config_lock.Open(path) && GetLastError()==ERROR_SHARING_VIOLATION)
   {
      MessageBox(nullptr, "The config file is already in use. The most likely cause is that BeipMU is already running.", "Error", MB_ICONERROR|MB_OK);
      return false;
   }
   g_config_lock.Close();
   LoadConfig(path);
   AssertReturned<true>()==g_config_lock.Open(path);
   return true;
}

void LoadConfig(ConstString filename, bool fImportingConfig)
{
   // On first time creating, schedule deletion
   if(!g_ppropGlobal)
      CallAtShutdown([]() { if(g_ppropGlobal) { delete g_ppropGlobal; g_ppropGlobal=nullptr; } });

   delete g_ppropGlobal;
   g_ppropGlobal=new Prop::Global();
   g_ppropGlobal->iVersion(g_ciBuildNumber);
   gp_restore_logs=nullptr;

   ErrorConsole error;
   LoadConfig(*g_ppropGlobal, filename, error);
   if(error.m_count)
      error.Text(FixedStringBuilder<512>("<icon error> While loading config file: \"<font color='aqua'>", Text::NoHTML{filename}, "</font>\" <font color='white'>"));

   // On a new version, make a backup of the config file
   if(g_ppropGlobal->fUpgraded() && !fImportingConfig)
      BackupConfig(error);

   gp_restore_logs=RestoreLogs::Create();
   if(fImportingConfig)
      gp_restore_logs->EraseAll();
   else
      gp_restore_logs->CheckAndRepair();

   Global_PropChange();

   SetUIFont(g_ppropGlobal->pclUIFontName(), g_ppropGlobal->iUIFontSize());
}

void LoadConfig(Prop::Global &global, ConstString filename, IError &error)
{
   auto data=File::Load(filename);
   if(!data)
   {
      // Does the file not exist?
      if(GetFileAttributes(filename)==INVALID_FILE_ATTRIBUTES)
         return;

      error.Text(FixedStringBuilder<512>("<icon error> Configuration file could not be loaded: ", Text::NoHTML_Start, LastError{}, Text::NoHTML_End));
      return;
   }

   // Load the configuration file
   global.fLoadErrors(!ConfigImport(data, &global, error));

   if(global.iVersion()<215) // In build 215 port merged into host
   {
      for(auto &ppropServer : global.propConnections().propServers())
      {
         ppropServer->pclHost(FixedStringBuilder<256>(ppropServer->pclHost(), ':', ppropServer->iPort()));
         ppropServer->iPort(0);
      }
   }
   if(global.iVersion()<216) // In build 216 I removed all saved window positions as they weren't used anymore
   {
      global.propWindows().propPositions().Empty();
   }
   if(global.iVersion()<258) // Server & character name was removed in favor of just having the shortcut
   {
      // Go through all the names, remove duplicates and put anything unique into the 'Info' field
      for(auto &pServer : global.propConnections().propServers())
      {
         // Iterate through the characters on the server
         for(auto &pCharacter : pServer->propCharacters())
         {
            // If there was no old name or the name matched the shortcut, skip
            if(!pCharacter->pclDeprecatedName() || pCharacter->pclDeprecatedName().ICompare(pCharacter->pclName())==0)
               continue;

            FixedStringBuilder<1024> newInfo(pCharacter->pclInfo());
            if(pCharacter->pclInfo() && !pCharacter->pclInfo().EndsWith(CHAR_LF)) // Add a crlf at the end if there is text and it doesn't end with one already
               newInfo(CRLF);
            newInfo("Name:", pCharacter->pclDeprecatedName());
            pCharacter->pclInfo(newInfo);
         }

         // If there was no old name or the name matched the shortcut, skip
         if(!pServer->pclDeprecatedName() || pServer->pclDeprecatedName().ICompare(pServer->pclName())==0)
            continue;

         FixedStringBuilder<1024> newInfo(pServer->pclInfo());
         if(pServer->pclInfo() && !pServer->pclInfo().EndsWith(CHAR_LF)) // Add a crlf at the end if there is text and it doesn't end with one already
            newInfo(CRLF);
         newInfo("Name:", pServer->pclDeprecatedName());
         pServer->pclInfo(newInfo);
      }
   }
   if(global.iVersion()<261) // GUID based server types changed instead to just a 'UseSSL' flag
   {
      // Map the old guidClient over to just the flag
      for(auto &pServer : global.propConnections().propServers())
         pServer->fTLS(pServer->guidClient()==__uuidof(IClient_TLS));
   }
   if(global.iVersion()<292)
   {
      // Transfer saved position tab colors to per character tab colors
      auto &propPositions=global.propWindows().propPositions();

      for(auto &pPosition : propPositions)
      {
         for(auto &pTab : pPosition->propTabs())
         {
            if(pTab->clrColor()==Colors::White) // Set to a custom color?
               continue;

            // See if we have a server
            auto ppropServer=global.propConnections().propServers().FindByName(pTab->pclServer());
            if(!ppropServer)
               continue;

            auto ppropCharacter=ppropServer->propCharacters().FindByName(pTab->pclCharacter());
            if(!ppropCharacter)
               continue;

            ppropCharacter->clrTabColor(pTab->clrColor());
         }
      }
   }
   if(global.iVersion()<309)
   {
      // Per server/char window settings were added, so a new structure called 'WindowSettings' was made to hold it all.
      // This moves the global settings in Prop::Windows into the new Prop::WindowSettings so that everything is consistent
      auto &windows=global.propWindows();
      auto &window_settings=global.propWindows().propMainWindowSettings();

      window_settings.propInput()=windows.propOldInput(); windows.ResetOldInput();
      window_settings.propOutput()=windows.propOldOutput(); windows.ResetOldOutput();
      window_settings.propHistory()=windows.propOldHistory(); windows.ResetOldHistory();

      window_settings.iInputSize(windows.iOldInputSize());
      window_settings.iHistorySize(windows.iOldHistorySize());
      window_settings.fHistory(windows.fOldHistory());

      CntPtrTo<Prop::TextWindow> ppropTextWindow_spawns=windows.fSpawnsUseOutputSettings() ? &window_settings.propOutput() : &windows.propOldSpawnWindows();
      windows.ResetOldSpawnWindows();

      auto ApplySettings=[&](Prop::DockedWindow &window)
      {
         switch(window.iType())
         {
            case 0: // InputWindow
            {
               auto &props=window.propInputWindow();
               auto prefix=props.pclPrefix();
               props=window_settings.propInput();
               props.pclPrefix(std::move(prefix));
               break;
            }

            case 1: // SpawnWindow
            {
               window.propSpawnWindow().propTextWindow()=*ppropTextWindow_spawns;
               break;
            }

            case 2: // SpawnTabsWindow
            {
               auto &props=window.propSpawnTabsWindow();
               for(auto &pTab : props.propTabs())
                  pTab->propTextWindow()=*ppropTextWindow_spawns;

               break;
            }
         }
      };

      // Apply global window settings to every character
      for(auto &p_server : global.propConnections().propServers())
         for(auto &p_character : p_server->propCharacters())
         {
            p_character->propMainWindowSettings()=g_ppropGlobal->propWindows().propMainWindowSettings();

            auto &propDocking=p_character->propDocking();
            for(Prop::DockedPane *p_frame : propDocking.propDockedPanes())
            {
               for(Prop::DockedWindow *p_propWindow : p_frame->propWindows())
                  ApplySettings(*p_propWindow);
            }

            for(Prop::DockedWindow *p_propWindow : propDocking.propFloatingWindows())
               ApplySettings(*p_propWindow);
         }
   }
   if(global.iVersion()<312)
   {
      // Local echo color moved from output window to input window
      auto &window_settings=global.propWindows().propMainWindowSettings();
      window_settings.propInput().clrLocalEchoColor(window_settings.propOutput().clrLocalEchoColor());

      // 'Use global settings' was added for text windows, so go through all text window settings and if they match the global ones, reset them so they'll auto use this
      auto &text_settings=GlobalTextSettings();
      auto &input_settings=GlobalInputSettings();

      auto ApplySettings=[&](Prop::DockedWindow &window)
      {
         switch(window.iType())
         {
            case 0: // InputWindow
            {
               if(window.fPropInputWindow() && window.propInputWindow()==input_settings)
                  window.ResetInputWindow();
               break;
            }

            case 1: // SpawnWindow
            {
               if(window.propSpawnWindow().fPropTextWindow() && window.propSpawnWindow().propTextWindow()==text_settings)
                  window.propSpawnWindow().ResetTextWindow();
               break;
            }

            case 2: // SpawnTabsWindow
            {
               auto &props=window.propSpawnTabsWindow();
               for(auto &pTab : props.propTabs())
               {
                  if(pTab->fPropTextWindow() && pTab->propTextWindow()==text_settings)
                     pTab->ResetTextWindow();
               }
               break;
            }
         }
      };

      for(auto &p_server : global.propConnections().propServers())
         for(auto &p_character : p_server->propCharacters())
         {
            auto &char_settings=p_character->propMainWindowSettings();
            char_settings.propInput().clrLocalEchoColor(char_settings.propOutput().clrLocalEchoColor());

            if(char_settings.fPropOutput() && char_settings.propOutput()==text_settings)
               char_settings.ResetOutput();
            if(char_settings.fPropHistory() && char_settings.propHistory()==text_settings)
               char_settings.ResetHistory();
            if(char_settings.fPropInput() && char_settings.propInput()==input_settings)
               char_settings.ResetInput();

            auto &propDocking=p_character->propDocking();
            for(Prop::DockedPane *p_frame : propDocking.propDockedPanes())
            {
               for(Prop::DockedWindow *p_propWindow : p_frame->propWindows())
                  ApplySettings(*p_propWindow);
            }

            for(Prop::DockedWindow *p_propWindow : propDocking.propFloatingWindows())
               ApplySettings(*p_propWindow);
         }
   }
   if(global.iVersion()<313)
   {
      // Trigger processing order was removed, and replaced with ability to have triggers above & below their sub-trees

      // New change is like the trigger order always being global/server/character, so we only need to modify if we're the opposite
      if(global.propConnections().iTriggerOrder()==0)
      {
         auto ChangeTriggers=[&](Prop::Triggers &triggers)
         {
            triggers.iAfterCount(triggers.Count());
         };

         ChangeTriggers(global.propConnections().propTriggers());
         for(auto &p_server : global.propConnections().propServers())
         {
            ChangeTriggers(p_server->propTriggers());
            for(auto &p_character : p_server->propCharacters())
               ChangeTriggers(p_character->propTriggers());
         }
      }
   }
   if(global.iVersion()<318)
   {
      if(!IsWindows11OrGreater())
         global.fTaskbarBadge(false); // Turn off for Windows 10 or lower
   }
   if(global.iVersion()<323 && global.iUIFontSize()!=13)
   {
      // Convert font size from cell height to character height
      Prop::Font font;
      font.pclName(global.pclUIFontName());
      font.iSize(-global.iUIFontSize());

      auto hf=font.CreateFont();
      TEXTMETRIC tm;
      ScreenDC dc;
      DC::FontSelector _(dc, hf);
      dc.GetTextMetrics(tm);

      global.iUIFontSize((tm.tmHeight-tm.tmInternalLeading)/g_dpiScale);
   }
   if(global.iVersion()<327)
   {
      global.iTheme(global.fDarkMode() ? 1 : 0);
   }

   // Validate iAfterCount for all triggers
   {
      auto Validate=[&](this auto &&self, Prop::Triggers &triggers) -> void
      {
         if(triggers.iAfterCount()>triggers.Count())
            triggers.iAfterCount(triggers.Count());

         for(auto &p_trigger : triggers)
         {
            if(p_trigger->fPropTriggers())
               self(p_trigger->propTriggers());
         }
      };

      Validate(global.propConnections().propTriggers());
      for(auto &p_server : global.propConnections().propServers())
      {
         Validate(p_server->propTriggers());
         for(auto &p_character : p_server->propCharacters())
            Validate(p_character->propTriggers());
      }
   }

   if(global.iVersion()>g_ciBuildNumber)
   {
      error.Error("Config file is from a newer version, this is not supported. Unless you're sure, it is recommended you quit without saving.");
      global.fLoadErrors(true);
   }

   global.fUpgraded(global.iVersion()<g_ciBuildNumber);
   global.iVersion(g_ciBuildNumber); // Set the current version on the property set
}

ConstString GetConfigPath()
{
   if(g_fUseStartupPathForConfig)
      return GetStartupPath();
   else
      return GetAppDataPath();
}

void BackupConfig(IError &error)
{
   File::Path filenameBak{GetConfigPath(), "Config.bak"};
   File::Path filename{GetConfigPath(), "Config.txt"};

   // The current config might not exist, we only error if we can't copy the file
   if(!CopyFile(filename, filenameBak, false /*FailIfExists*/) && GetLastError()!=ERROR_FILE_NOT_FOUND)
      error.Error(HybridStringBuilder("Couldn't copy config.txt to config.bak: (", Text::NoHTML_Start, LastError{}, Text::NoHTML_End, ')'));
}

void ImportConfig(ConstString filename)
{
   // Close all main windows
   while(Wnd_MDI::s_root_node.Linked())
   {
      Wnd_MDI &wnd=*Wnd_MDI::s_root_node.Next();
      if(Msg::Close().Send(wnd)==Msg::Close::Failure())
      {
         MessageBox(wnd, "Importing config aborted since not all windows could be closed", "Note", MB_ICONINFORMATION|MB_OK);
         return;
      }
   }

   MessagePump(); // Eat the WM_QUIT that gets posted

   LoadConfig(filename, true);
   g_ppropGlobal->fLoadErrors(true);
   CreateWindow_Root();
}

Prop::Global &GetSampleConfig()
{
   if(!g_ppropSample)
   {
      g_ppropSample=MakeUnique<Prop::Global>();

#ifdef _DEBUG
      File::Path filename{"C:\\programming\\BeipMU\\MSIX\\assets\\SampleConfig.txt"};
#else
      File::Path filename{GetResourcePath(), "SampleConfig.txt"};
#endif

      ErrorConsole error;
      LoadConfig(*g_ppropSample, filename, error);
   }

   return *g_ppropSample;
}

Prop::Position::State ShowCmdToState(UINT showCmd) noexcept
{
   switch(showCmd)
   {
      case SW_MAXIMIZE: return Prop::Position::State::Maximized;
      case SW_SHOWMINIMIZED:
      case SW_MINIMIZE: return Prop::Position::State::Minimized;
      default: Assert(false);
      case SW_NORMAL: return Prop::Position::State::Normal;
   }
}

UINT StateToShowCmd(Prop::Position::State state) noexcept
{
   switch(state)
   {
      case Prop::Position::State::Maximized: return SW_MAXIMIZE;
      case Prop::Position::State::Minimized: return SW_MINIMIZE;
      default: return SW_NORMAL;
   }
}

void SaveWindowInformationInConfig()
{
   for(auto &connection : Connection::s_root_node)
      connection.GetMainWindow().Save();

   // Save the floating window positions and all of their tabs
   {
      auto &positions=g_ppropGlobal->propWindows().propPositions();
      positions.Empty();

      for(auto &window : Wnd_MDI::s_root_node)
      {
         auto &propPosition=*positions.Push(MakeUnique<Prop::Position>());

         WindowPlacement wp{window};
         propPosition.eState(ShowCmdToState(wp.showCmd));

         Rect& rcNormal=wp.NormalPosition();
         rcNormal.pt2-=rcNormal.pt1; // We save this rectangle as (point, size) vs (point1, point2)
         if(propPosition.eState()==Prop::Position::Normal)
            rcNormal.pt2=window.WindowSize(); // If the window is snapped, this will remember the size properly so things don't shrink on restart
         propPosition.rcPosition(rcNormal);

         for(auto &tab : window.GetRootWindow())
         {
            auto &propTab=*propPosition.propTabs().Push(MakeUnique<Prop::Tab>());
            auto &connection=tab.GetConnection();

            auto *pServer=connection.GetServer();
            if(!pServer)
               continue;
            propTab.pclServer(pServer->pclName());

            auto *pCharacter=connection.GetCharacter();
            if(!pCharacter)
               continue;
            propTab.pclCharacter(pCharacter->pclName());

            auto *pPuppet=connection.GetPuppet();
            if(!pPuppet)
               continue;
            propTab.pclPuppet(pPuppet->pclName());
         }

         propPosition.iActiveTab(window.GetActiveWindowIndex());
      }
   }
}

bool SaveConfig(Window wnd, bool allow_gui)
{
   if(g_ppropGlobal->fLoadErrors())
   {
      if(!wnd || !allow_gui || MessageBox(wnd, STR_CouldLoadConfigFile, STR_Warning, MB_ICONWARNING|MB_YESNO|MB_DEFBUTTON2)==IDNO)
         return false;
      g_ppropGlobal->fLoadErrors(false); // Only ask once
   }

   SaveWindowInformationInConfig();

   // Save the old style MDIPosition setting (obsoleted by SaveWindowInformation, should this just be removed?)
   if(wnd)
   {
      WindowPlacement wp{wnd};
      Rect& rcNormal=wp.NormalPosition();
      rcNormal.pt2-=rcNormal.pt1; // We save this rectangle as (point, size) vs (point1, point2)
      g_ppropGlobal->propWindows().rcMDIPosition(rcNormal);
   }

   File::Path filenameNew{GetConfigPath(), "Config.new"};
   File::Path filename   {GetConfigPath(), "Config.txt"};

   // Save out the current config as config.new
   {
      DeleteFile(filenameNew); // Just in case there was a straggler, delete it
      ConfigExport(filenameNew, g_ppropGlobal, g_ppropGlobal->fShowDefaults(), !allow_gui);
   }

   g_config_lock.Close(); // Release lock on file

   // Now replace config.txt with config.new
   if(!MoveFileEx(filenameNew, filename, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH|MOVEFILE_COPY_ALLOWED))
   {
      LastError error;
      DeleteFile(filenameNew); // We failed to create the new file, so delete the new one
      if(wnd && allow_gui)
         MessageBox(wnd, HybridStringBuilder("Failed to replace Config.txt with Config.new : ", error), STR_Error, MB_ICONEXCLAMATION|MB_OK);
      return false;
   }

   AssertReturned<true>()==g_config_lock.Open(filename);
   return true;
}

void ResetConfig()
{
   delete g_ppropGlobal;
   g_ppropGlobal=new Prop::Global();
   g_ppropGlobal->iVersion(g_ciBuildNumber);
   g_ppropGlobal->fLoadErrors(true);
   Global_PropChange();
}

void Global_PropChange()
{
   Text::Wnd::ShowTip_SelectionCopy(g_ppropGlobal->fShowTip_SelectionCopy());
   Text::Wnd::SetShowNewContent(g_ppropGlobal->fNewContentMarker());

   for(auto &window : Wnd_MDI::s_root_node)
      window.OnPropChange();
}

Events::SendersOf<GlobalTextSettingsModified, GlobalInputSettingsModified> g_text_events;

void OnGlobalTextSettingsModified()
{
   g_text_events.Send(GlobalTextSettingsModified{});
}

void OnGlobalInputSettingsModified(Prop::InputWindow &prop)
{
   g_text_events.Send(GlobalInputSettingsModified{prop});
}

Prop::TextWindow &GlobalTextSettings()
{
   return g_ppropGlobal->propWindows().propMainWindowSettings().propOutput();
}

Prop::InputWindow &GlobalInputSettings()
{
   return g_ppropGlobal->propWindows().propMainWindowSettings().propInput();
}

void SetTextWindowProperties(Text::Wnd &window, Prop::TextWindow &prop)
{
   window.SetFont(prop.propFont().pclName(), prop.propFont().iSize(), prop.propFont().byCharSet());
   window.SetTimeFormat(prop.iTimeFormat());
   window.SetTimeFormatToolTip(prop.iTimeFormatToolTip());
   window.SetScrollToBottomOnAdd(prop.fScrollToBottomOnAdd());
   window.SetSplitOnPageUp(prop.fSplitOnPageUp());
   window.SetSmoothScrolling(prop.fSmoothScrolling());
   window.SetHistoryLimit(prop.iHistory());
   window.SetFixedWidth(prop.fFixedWidth() ? prop.iFixedWidthChars() : 0);
   window.SetMargins(prop.rcMargins());
   window.SetWrappedLineIndent(prop.iLineWrappedIndent());
   window.SetParagraphSpacing(prop.iParagraphSpacing());

   Text::ColorSet &colors=window.GetColorSet();
   colors.SetInvertBrightness(prop.fInvertBrightness());
   colors.SetForeground(prop.clrFore());
   colors.SetBackground(prop.clrBack(), 0);
   colors.SetBackground(prop.clrFanFold1(), 1);
   colors.SetBackground(prop.clrFanFold2(), 2);
   colors.SetHyperlink(prop.clrLink());
   colors.SetFanFold(prop.fFanFold());
   window.Invalidate(true); // To redraw after changing the colors
}

CntPtrTo<Prop::TextWindow> CopyIfNotGlobal(Prop::TextWindow &props)
{
   if(&props==&GlobalTextSettings()) // Don't copy?
      return &props;
   return MakeCounting<Prop::TextWindow>(props);
}

void ToggleGlobalSettings(Text::Wnd &wnd, CntPtrTo<Prop::TextWindow> &p_props)
{
   auto &global=GlobalTextSettings();
   if(p_props==&global) // Using global, so make a copy so edits are no longer global
      p_props=MakeCounting<Prop::TextWindow>(*p_props);
   else
   {
      if(*p_props!=global && MessageBox(wnd, "Switching to global settings will overwrite your custom changes; are you sure?", "Note", MB_ICONQUESTION|MB_YESNO)!=IDYES)
         return;

      p_props=&global;
      ::SetTextWindowProperties(wnd, *p_props); // We switched to global properties, so redo settings
   }
}


void ClientLayoutHelper::Layout(DeferredWindowPos &wp, const Rect &rc)
{
   m_main.Wnd_Container::Layout(rc);
   auto rect=m_main.Wnd_Container::clientRect();
   rect.pt2=Greater(rect.pt2, rect.pt1+mp_object->GetMinSize());
   mp_object->Layout(wp, rect);
}

SpawnWindow::SpawnWindow(Wnd_Main &wndMain)
 : m_wnd_main{wndMain},
  mp_text{&wndMain.GetOutputWindow()}
{
}

SpawnWindow::SpawnWindow(SpawnWindow *pInsertAfter, Wnd_Main &wndMain, ConstString title, Prop::TextWindow &props, SpawnTabsWindow *p_spawn_tabs_window)
 : DLNode<SpawnWindow>{pInsertAfter},
   m_title{title},
   m_wnd_main{wndMain},
   mp_text{new Text::Wnd{p_spawn_tabs_window ? *p_spawn_tabs_window : Window{}, *this}},
   mp_prop_text_window{&props},
   mp_spawn_tabs_window{p_spawn_tabs_window}
{
   AttachTo<GlobalTextSettingsModified>(g_text_events);
   Subclass(*mp_text);

   mp_text->SetText(m_title);
   mp_text->SetAway(!wndMain.IsActive());

   if(!mp_spawn_tabs_window)
   {
      mp_text->SetLongPtr(GWLP_HWNDPARENT, wndMain.GetMDI().hWnd());
      mp_text->SetLong(GWL_EXSTYLE, mp_text->GetLong(GWL_EXSTYLE)|WS_EX_APPWINDOW);

      mp_docking=&wndMain.CreateDocking(*this);

      // Limit window to 1/3 the size of the client area
      mp_text->SetSize(wndMain.clientRect().size()/3);
   }

   ::SetTextWindowProperties(*mp_text, *mp_prop_text_window);
}

SpawnWindow::~SpawnWindow()
{
}

LRESULT SpawnWindow::WndProc(const Message &msg)
{
   return Dispatch<SuperImpl, Msg::_GetTypeID, Msg::_GetThis>(this, msg);
}

bool SpawnWindow::On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Text::Records::URLData &url, const Message &msg)
{
   // Just forward it to the main window
   return m_wnd_main.On(wndText, wndView, url, msg);
}

void SpawnWindow::On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Msg::RButtonDown &msg, const Text::List::LocationInfo &location_info)
{
   m_wnd_main.On(wndText, wndView, msg, location_info, mp_prop_text_window);
}

void SpawnWindow::On(const GlobalTextSettingsModified &event)
{
   if(mp_prop_text_window==&GlobalTextSettings())
      ::SetTextWindowProperties(*mp_text, *mp_prop_text_window);
}

void SpawnWindow::ShowTab()
{
   if(!mp_spawn_tabs_window)
      return;
   mp_spawn_tabs_window->SetVisible(*this);
}

void SpawnWindow::AfterRestore()
{
   mp_text->MarkAsRead();
   Msg::VScroll(SB_BOTTOM).Post(*mp_text);
}

SpawnWindow &Wnd_Main::GetSpawnWindow(const Prop::Trigger_Spawn &trigger, ConstString title, bool fHilight)
{
   if(!title)
   {
      if(!mp_null_spawn)
         mp_null_spawn=MakeUnique<SpawnWindow>(*this);
      return *mp_null_spawn;
   }

   if(trigger.pclTabGroup())
   {
      SpawnTabsWindow *pTabWindow{};

      for(auto &tabWindow : m_spawn_tabs_windows)
      {
         if(tabWindow && tabWindow.m_title==trigger.pclTabGroup())
         {
            pTabWindow=&tabWindow;
            break;
         }
      }

      if(!pTabWindow)
      {
         pTabWindow=new SpawnTabsWindow(m_spawn_tabs_windows.Prev(), *this, trigger.pclTabGroup());
         pTabWindow->GetDocking().Dock(Docking::Side::Right);
      }

      return pTabWindow->GetTab(title, nullptr, fHilight);
   }

   for(auto &window : m_spawn_windows)
   {
      if(window.m_title==title)
         return window;
   }

   auto &spawn=*new SpawnWindow(m_spawn_windows.Prev(), *this, title, *CopyIfNotGlobal(*mp_prop_output));
   if(spawn)
      spawn.GetDocking().Dock(Docking::Side::Right);
   return spawn;
}

Wnd_Image &Wnd_Main::EnsureImageWindow()
{
   if(!mp_wnd_image)
   {
      mp_wnd_image=CreateImageWindow(*this);
      mp_wnd_image->GetDocking().Dock(Docking::Side::Top);
   }

   return *mp_wnd_image;
}

Maps::Wnd &Wnd_Main::EnsureMapWindow()
{
   if(!mp_wnd_map)
   {
      mp_wnd_map=MakeUnique<Maps::Wnd>(*this);
      mp_wnd_map->GetDocking().Dock(Docking::Side::Right);
   }
   return *mp_wnd_map;
}

TileMaps &Wnd_Main::EnsureTileMaps()
{
   if(!mp_tile_maps)
      mp_tile_maps=MakeUnique<TileMaps>(*this);
   return *mp_tile_maps;
}

IYarn_TileMap &Wnd_Main::EnsureYarn_TileMap()
{
#if YARN
   if(!mp_yarn_tilemap)
   {
      mp_yarn_tilemap=IYarn_TileMap::Create(*this);
      mp_yarn_tilemap->GetDocking().Dock(Docking::Side::Left);
   }
#endif

   return *mp_yarn_tilemap;
}

Wnd_WebView *Wnd_Main::FindWebView(ConstString id)
{
   for(auto &view : m_webview_windows)
   {
      if(view.GetID()==id)
         return &view;
   }
   return nullptr;
}

Stats::Wnd &Wnd_Main::GetStatsWindow(ConstString title, bool fDock)
{
   for(auto &stat : m_stat_windows)
   {
      if(stat.GetTitle()==title)
         return stat;
   }

   auto *pStat=new Stats::Wnd(*this, title);
   pStat->DLNode<Stats::Wnd>::Link(m_stat_windows.Prev());
   if(fDock)
      pStat->GetDocking().Dock(Docking::Side::Right);

   return *pStat;
}

void Wnd_Main::BroadcastHTML(ConstString string)
{
   mp_wnd_text->AddHTML(string);

   for(auto &tabWindow : m_spawn_tabs_windows)
   {
      for(auto &tab : tabWindow.GetTabs())
         tab.mp_text->AddHTML(string);
   }

   for(auto &window : m_spawn_windows)
      window.mp_text->AddHTML(string);

}

void Wnd_Main::OnReplayComplete()
{
   for(auto &tabWindow : m_spawn_tabs_windows)
      tabWindow.AfterRestore();

   for(auto &window : m_spawn_windows)
      window.AfterRestore();

   Msg::VScroll(SB_BOTTOM).Post(*mp_wnd_text);
   Msg::VScroll(SB_BOTTOM).Post(*mp_wnd_text_history);
}

SpawnTabsWindow::SpawnTabsWindow(SpawnTabsWindow *p_insert_after, Wnd_Main &wnd_main, ConstString title)
 : DLNode<SpawnTabsWindow>{p_insert_after},
   m_title{title},
   m_wnd_main{wnd_main}
{
   Create(title, WS_OVERLAPPEDWINDOW, 0 /*WS_EX_NOACTIVATE*/, wnd_main);
   mp_docking=&wnd_main.CreateDocking(*this);

   // Limit window to 1/3 the size of the client area
   SetSize(m_wnd_main.clientRect().size()/3);
}

SpawnTabsWindow::~SpawnTabsWindow()
{
}

SpawnWindow &SpawnTabsWindow::GetTab(ConstString title, Prop::TextWindow *pprops, bool hilight)
{
   // Look for an existing tab
   for(unsigned i=0;i<m_spawn_windows.Count();i++)
   {
      if(m_spawn_windows[i].m_title==title)
      {
         if(hilight)
            mp_tabs->SetHilight(i);
         return m_spawn_windows[i];
      }
   }

   // Create a new tab, make a copy of the text window settings using a previous tab or the output settings
   CntPtrTo<Prop::TextWindow> p_prop_text_window{pprops};
   if(!p_prop_text_window)
      p_prop_text_window=CopyIfNotGlobal(m_spawn_windows.Linked() ? *m_spawn_windows.Prev()->mp_prop_text_window : m_wnd_main.GetOutputProps());
   auto &spawn_window=*new SpawnWindow(m_spawn_windows.Prev(), m_wnd_main, title, *p_prop_text_window, this);

   // TODO: Kinda stupid, but without this the ChildWindow gets leaked as it's not owned
   spawn_window.mp_child_window=UniquePtr<AL::ChildWindow>(m_layout.AddChildWindow(*spawn_window.mp_text));

   mp_tabs->AddWindow(m_layout.AddObjectWindow(*spawn_window.mp_text, *spawn_window.mp_child_window));
   m_layout.Update();
   // If there is more than one tab, this new tab isn't visibile, so hilight it and set it to away
   if(mp_tabs->GetCount()>1)
   {
      if(hilight)
         mp_tabs->SetHilight(m_spawn_windows.Count()-1);
      spawn_window.mp_text->SetAway(true);
   }
   mp_tabs->Invalidate(true);
   return spawn_window;
}

void SpawnTabsWindow::SetAway(bool fAway)
{
   for(auto &p_window : m_spawn_windows)
      p_window.mp_text->SetAway(fAway);
}

unsigned SpawnTabsWindow::GetUnreadCount() const
{
   unsigned unread_count=0;

   for(auto &p_window : m_spawn_windows)
      unread_count+=p_window.mp_text->GetUnreadCount();

   return unread_count;
}

void SpawnTabsWindow::AfterRestore()
{
   for(unsigned i=0;i<m_spawn_windows.Count();i++)
   {
      m_spawn_windows[i].AfterRestore();
      mp_tabs->SetHilight(i, false);
   }
}

void SpawnTabsWindow::TabChange(unsigned tabOld)
{
   // Old visible window is now away
   m_spawn_windows[tabOld].mp_text->SetAway(true);
   // New window is not
   m_spawn_windows[mp_tabs->GetVisible()].mp_text->SetAway(false);
}

void SpawnTabsWindow::TabClosed(unsigned tab)
{
   delete &m_spawn_windows[tab];
}

void SpawnTabsWindow::TabMoved(unsigned from_index, unsigned to_index)
{
   auto &from=m_spawn_windows[from_index];
   from.Unlink();
   from.Link(m_spawn_windows[to_index].Prev());
}

void SpawnTabsWindow::SetVisible(SpawnWindow &window)
{
   for(unsigned i=0; i<m_spawn_windows.Count(); i++)
   {
      if(&m_spawn_windows[i]==&window)
      {
         auto old_visible=mp_tabs->GetVisible();
         mp_tabs->SetVisible(i);
         TabChange(old_visible);
         return;
      }
   }
   Assert(false);
}

bool SpawnTabsWindow::SetVisible(ConstString title)
{
   for(unsigned i=0;i<m_spawn_windows.Count();i++)
   {
      if(m_spawn_windows[i].m_title==title)
      {
         auto old_visible=mp_tabs->GetVisible();
         mp_tabs->SetVisible(i);
         TabChange(old_visible);
         return true;
      }
   }
   return false;
}

LRESULT SpawnTabsWindow::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::EraseBackground, Msg::_GetTypeID, Msg::_GetThis>(msg);
}

LRESULT SpawnTabsWindow::On(const Msg::Create &msg)
{
   m_layout.SetWindowPadding(0);

   mp_tabs=m_layout.CreateTab();
   mp_tabs->SetNotify(this);
   mp_tabs->SetClosable();
   mp_tabs->SetMovable();
   mp_tabs->SetPadding(false);
   m_layout.SetRoot(mp_tabs);
   return msg.Success();
}

LRESULT SpawnTabsWindow::On(const Msg::EraseBackground &msg)
{
   return msg.Success();
}

Wnd_InputPane* Wnd_Main::CreateInputWindow(Prop::InputWindow &props)
{
   return m_input_panes.Push(new Wnd_InputPane(*this, props));
}

Wnd_InputPane* Wnd_Main::AddInputWindow(ConstString prefix, bool unique)
{
   if(unique)
   {
      for(auto &p_pane : m_input_panes)
         if(p_pane->m_input.GetProps().pclPrefix()==prefix)
            return p_pane;
   }

   CntPtrTo<Prop::InputWindow> p_props=&m_input.GetProps();
   if(p_props!=&GlobalInputSettings() || prefix)
      p_props=MakeCounting<Prop::InputWindow>(*p_props);
   p_props->pclPrefix(prefix);
   auto *p_input=CreateInputWindow(*p_props);
   p_input->GetDocking().Dock(Docking::Side::Bottom);
   return p_input;
}

void Wnd_Main::RemoveInputPane(Wnd_InputPane &pane)
{
   // If the active input is deleted, set the input to the default one just in case
   if(mp_input_active==&pane.m_input)
      mp_input_active=&m_input;

   m_input_panes.Delete(m_input_panes.Find(&pane));
}

void Wnd_Main::ActivateNextInputWindow()
{
   if(&m_input==mp_input_active)
   {
      if(m_input_panes.Count())
         m_input_panes.First()->m_input.SetFocus();
      return;
   }

   for(unsigned i=0;i<m_input_panes.Count();i++)
   {
      if(&m_input_panes[i]->m_input==mp_input_active)
      {
         if(++i<m_input_panes.Count())
            m_input_panes[i]->m_input.SetFocus();
         else
            m_input.SetFocus();
         return;
      }
   }
}

Wnd_EditPane& Wnd_Main::CreateEditPane(ConstString title, bool dockable, bool spellcheck)
{
   return *m_edit_panes.Push(new Wnd_EditPane(*this, title, dockable, spellcheck));
}

Wnd_EditPane& Wnd_Main::GetEditPane(ConstString title, bool dockable, bool spellcheck)
{
   if(title) // Only search if title is set
   {
      for(auto &pPane : m_edit_panes)
      {
         if(pPane->GetTitle()==title)
            return *pPane;
      }
   }

   return CreateEditPane(title, dockable, spellcheck);
}

void Wnd_Main::RemoveEditPane(Wnd_EditPane &pane)
{
   m_edit_panes.Delete(m_edit_panes.Find(&pane));
}

void Wnd_Main::ShowCharacterNotesPane()
{
   Assert(!mp_character_notes_pane);

   if(auto *pCharacter=mp_connection->GetCharacter())
   {
      if(mp_connection->GetPuppet())
      {
         MessageBox(*this, "Only works on the character, not puppets. If you think it could be useful to puppets, please let us know!", "Note", MB_OK);
         return;
      }

      mp_character_notes_pane=MakeUnique<Wnd_EditPropertyPane>(*this, *pCharacter);
      mp_character_notes_pane->GetDocking().Dock(Docking::Side::Right);
   }
   else
      MessageBox(*this, "You must connect with a character first", "Note", MB_OK|MB_ICONINFORMATION);
}

void Wnd_Main::UpdateCharacterIdleTimer()
{
   auto *p_character=mp_connection->GetCharacter();
   Assert(p_character);

   if(!p_character->fIdleEnabled())
   {
      m_idle_timer.Reset();
      return;
   }

   m_idle_delay=p_character->iIdleTimeout();
   PinAbove(m_idle_delay, 1U); // Must be at least 1 minute
   m_idle_string=p_character->pclIdleString();
   m_idle_timer.Set(m_idle_delay*60.0f, true);
}

void Wnd_Main::TabColor(Color color)
{
   if(auto *p_puppet=mp_connection->GetPuppet())
      p_puppet->clrTabColor(color);
   else if(auto *p_character=mp_connection->GetCharacter())
      p_character->clrTabColor(color);

   GetMDI().RefreshTaskbar(*this);
}

bool Wnd_Main::ShowActivityOnTaskbar()
{
   if(auto *p_puppet=mp_connection->GetPuppet())
      return p_puppet->fShowActivityOnTaskbar();
   else if(auto *p_character=mp_connection->GetCharacter())
      return p_character->fShowActivityOnTaskbar();

   return true;
}

void Wnd_Main::ToggleShowActivityOnTaskbar()
{
   if(auto *p_puppet=mp_connection->GetPuppet())
      p_puppet->fShowActivityOnTaskbar(!p_puppet->fShowActivityOnTaskbar());
   else if(auto *p_character=mp_connection->GetCharacter())
      p_character->fShowActivityOnTaskbar(!p_character->fShowActivityOnTaskbar());
}

//=============================================================================
LRESULT Wnd_Main::WndProc(const Message &msg)
{
   return Dispatch<WindowImpl, Msg::Create, Msg::Close, Msg::Notify, Msg::Command, Msg::Char, Msg::Paint, Msg::Size, Msg::Activate, Msg::SetFocus, Msg::SetText, Msg::EraseBackground, Msg::LButtonDown, Msg::XButtonDown>(msg);
}

CntPtrTo<Prop::MainWindowSettings> GetMainWindowSettings(Prop::Character *p_prop_character, Prop::Puppet *p_prop_puppet)
{
   if(p_prop_puppet)
      return &p_prop_puppet->propMainWindowSettings();

   if(p_prop_character)
      return &p_prop_character->propMainWindowSettings();

   return MakeCounting<Prop::MainWindowSettings>(g_ppropGlobal->propWindows().propMainWindowSettings());
}

Wnd_Main::Wnd_Main(Wnd_MDI &wnd_MDI, Prop::Server *ppropServer, Prop::Character *ppropCharacter, Prop::Puppet *ppropPuppet, bool offline)
 : m_wnd_MDI{wnd_MDI},
   mp_prop_main_window_settings{GetMainWindowSettings(ppropCharacter, ppropPuppet)},
   m_input{*this, GlobalInputSettings()},
   mp_connection{MakeUnique<Connection>(*this)}
{
   mp_connection->Associate(ppropServer, ppropCharacter, ppropPuppet);
   Create("", WS_CHILD, Position_Zero, m_wnd_MDI);
   GetMDI().AddWindow(*this);

   // We must send this event because if a script creates a new window, it expects the OnNewWindow event
   // to have been processed before the next line.  If we post this event that won't happen.
   if(GlobalEvents::GetInstance().Get<Event_NewWindow>())
   {
      SetScripterWindow();
      GlobalEvents::GetInstance().Send(Event_NewWindow(*this));
   }

   if(ppropServer)
      mp_connection->Connect(offline || IsKeyPressed(VK_CONTROL));
   else
   {
      FixedStringBuilder<1024> string{
         R"(<p align='center' background-color='#004000' stroke-color='#008000' stroke-width='2' border='10' border-style='round' indent='5' padding='2'>)"
            R"(<font color='white' face='Arial' size='16'>To connect to a world, click the 🌎 icon in )",
            g_ppropGlobal->fTaskbarOnTop() ? ConstString("top") : ConstString("bottom"), " left"};
      mp_wnd_text->AddHTML(string);
   }
}

Wnd_Main::Wnd_Main(Wnd_MDI &wnd_MDI) : Wnd_Main(wnd_MDI, nullptr, nullptr, nullptr, false)
{
}

void Wnd_Main::ParseCommandLine(ConstString cmdLine)
{
   ConstString strTelnet("telnet://");

   // Skip past the telnet:// prefix
   if(cmdLine.StartsWith(strTelnet))
   {
      if(cmdLine.EndsWith('/'))
         cmdLine=cmdLine.WithoutLast(1);

      auto ppropServer=MakeCounting<Prop::Server>();
      ppropServer->pclHost(cmdLine.WithoutFirst(strTelnet.Length()));
      mp_connection->Associate(ppropServer, nullptr, nullptr);
      mp_connection->Connect(false);
   }
}

void Wnd_Main::On(const Connection::Event_Connect &event)
{
}

void Wnd_Main::On(const Connection::Event_Disconnect &event)
{
   SetText("");
   m_idle_timer.Reset();
   m_delay_timers.Empty();
}

void Wnd_Main::On(const Connection::Event_Send &event)
{
   // Reset the idle timer
   if(m_idle_timer)
      m_idle_timer.Set(m_idle_delay*60.0f, true);
}

void Wnd_Main::On(const Connection::Event_Log &event)
{
   GetMDI().OnWindowChanged(*this);
}

void Wnd_Main::On(const Connection::Event_Activity &event)
{
   if(m_active)
      return;

   if(!m_timer_tab_flash && g_ppropGlobal->propConnections().eActivityNotify()==Prop::Connections::Blink)
   {
      m_timer_tab_flash.Set(2.0f, true);
      FlashTab();
   }

   if(!m_flash_state && g_ppropGlobal->propConnections().eActivityNotify()==Prop::Connections::Solid)
      FlashTab();

   m_wnd_MDI.RefreshBadgeCount();
   GetMDI().RefreshTaskbar(*this); // Either way, redraw the window's unread count
}

bool Wnd_Main::On(Text::Wnd &wnd_text, Text::Wnd_View &wnd_view, const Text::Records::URLData &url, const Message &_msg)
{
   if(_msg.uMessage()==Msg::LButtonUp::ID)
   {
      switch(url.m_type)
      {
         case Text::Records::URLType::TELNET:
            (new Wnd_Main(GetMDI()))->ParseCommandLine(url.m_url);
            return true;

         case Text::Records::URLType::Custom:
            if(url.m_pCustom)
            {
               if(auto *pPueblo=url.m_pCustom->QueryInterface<Pueblo_Send>())
               {
                  if(auto send=pPueblo->OnLButton())
                  {
                     History_AddToHistory(send, ConstString());
                     mp_connection->Send(send);
                  }
               }
            }
            else
               SendLines(url.m_url);
            return true;

         default:
            return Text::IHost::On(wnd_text, wnd_view, url, _msg);
      }
   }
   else if(_msg.uMessage()==Msg::RButtonDown::ID)
   {
      if(url.m_pCustom)
      {
         if(auto *pPueblo=url.m_pCustom->QueryInterface<Pueblo_Send>())
         {
            auto &msg=_msg.Cast<Msg::RButtonDown>();
            if(auto send=pPueblo->OnRButton(wnd_view, msg.position()))
            {
               History_AddToHistory(send, ConstString());
               mp_connection->Send(send);
            }
            return true;
         }
      }

      enum struct Commands : UINT_PTR
      {
         Cancel=0,
         Open,
         OpenInPane,
         Copy,
         ExistingPane,
      };

      PopupMenu menu;
      menu.Append(0, (UINT_PTR)Commands::Open, "Open in Browser");
      menu.Append(0, (UINT_PTR)Commands::Open, "Open in incognito mode");
      menu.Append(0, (UINT_PTR)Commands::OpenInPane, "Open in Pane");

      if(m_webview_windows.Linked())
      {
         PopupMenu menu_panes;
         auto index=static_cast<UINT_PTR>(Commands::ExistingPane);
         for(auto &pane : m_webview_windows)
         {
            menu_panes.Append(0, index++, pane.GetText());
         }

         menu.Append(std::move(menu_panes), "Open in Existing Pane");
      }

      menu.Append(0, (UINT_PTR)Commands::Copy, "Copy");

      int id=TrackPopupMenu(menu, TPM_RETURNCMD, GetCursorPos(), wnd_view, nullptr);
      if(id>=static_cast<int>(Commands::ExistingPane))
      {
         m_webview_windows[id-static_cast<int>(Commands::ExistingPane)].SetURL(url.m_url);
         return true;
      }

      switch(Commands(id))
      {
         case Commands::Cancel: break;
         case Commands::Open: OpenURLAsync(url.m_url); break;
         case Commands::OpenInPane:
         {
            auto &wnd=*new Wnd_WebView(*this);
            wnd.SetURL(url.m_url);
            wnd.GetDocking().Dock(Docking::Side::Right);
            break;
         }
         case Commands::Copy: ::Clipboard::SetText(url.m_url); break;
      }

      return true;
   }

   return Text::IHost::On(wnd_text, wnd_view, url, _msg);
}

ConstString FindWord(ConstString string, unsigned index)
{
   Assert(index<string.Count());

   uint2 range=index;

   while(range.begin && !IsWordBreak(string[range.begin-1]))
      range.begin--;

   while(range.end<string.Count() && !IsWordBreak(string[range.end]))
      range.end++;

   return string.Sub(range);
}

void Wnd_Main::On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Msg::RButtonDown &msg, const Text::List::LocationInfo &location_info)
{
   On(wndText, wndView, msg, location_info, &wndText==mp_wnd_text ? mp_prop_output : mp_prop_history);
}

void Wnd_Main::On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Msg::RButtonDown &msg, const Text::List::LocationInfo &location_info, CntPtrTo<Prop::TextWindow> &p_props)
{
   auto *p_line=location_info.p_line;

   HybridStringBuilder text;
   ContextMenu menu;

   if(!wndText.GetTextList().SelectionGet())
   {
      menu.Append(0, "Find...", [&]() { CreateDialog_Find(*this, wndText); });
      menu.Append(wndText.IsUserPaused() ? MF_CHECKED : 0, "Pause", [&]() { wndText.SetUserPaused(!wndText.IsUserPaused()); });
      menu.Append(wndText.IsSplit() ? MF_CHECKED : 0, "Split", [&]() { wndText.ToggleSplit(); });
      menu.Append(0, "Copy screen to clipboard", [&]() { wndView.ScreenToClipboard(); });
      menu.AppendSeparator();
      menu.Append(0, "Clear", [&]() { wndText.Clear(); });
      menu.Append(p_line ? 0 : MF_DISABLED, "Delete Line", [&]() { wndText.RemoveLine(UnconstRef(*p_line)); });
#ifdef _DEBUG
      menu.Append(p_line ? 0 : MF_DISABLED, "Dump Line", [&]() { p_line->Dump(); });
#endif
      menu.AppendSeparator();
      menu.Append(p_props==&GlobalTextSettings() ? MF_CHECKED : MF_UNCHECKED, "Use global settings", [&]() { ToggleGlobalSettings(wndText, p_props); });
      menu.Append(0, "Settings...", [&]() { CreateDialog_TextWindow(*this, wndText, *p_props); });

      if(p_line)
         text(FindWord(p_line->GetText(), location_info.m_find_element.m_text_index));
   }
   else
   {
      menu.Append(0, "Open as URL", [&]()
      {
         const Text::List::Selection &selection=wndText.GetTextList().SelectionGet();
         if(!selection || selection.m_start.m_line!=selection.m_end.m_line)
            return;

         HybridStringBuilder<> string("http:\\\\");
         wndText.GetTextList().SelectionToText(string);
         OpenURLAsync(string);
      });
      menu.Append(0, "Copy as HTML", [&]()
      {
         HeapStringBuilder string;
         wndText.GetTextList().SelectionToHTML(string);
         ::Clipboard::SetText(string);
      });
      menu.AppendSeparator();
      menu.Append(0, "Delete selected lines", [&]()
         {
            wndText.RemoveSelectedLines();
         });

      wndText.GetTextList().SelectionToText(text);
   }

   if(auto *p_banner=location_info.m_find_element.mp_image)
      p_banner->AddContextMenu(menu, location_info.m_find_element.m_position, location_info.m_find_element.m_rect);
   else if(text && text.Count()<64 && mp_connection->GetServer())
   {
      menu.AppendSeparator();
      menu.Append(0, FixedStringBuilder<256>("Trigger on text: ", text), [&]()
      {
         CntPtrTo<Prop::Trigger> p_word_trigger;

         Prop::FindString propFindString;
         propFindString.pclMatchText(text);
         propFindString.fWholeWord(true);
         propFindString.fMatchCase(false);

         for(auto &p_trigger : mp_connection->GetServer()->propTriggers())
         {
            FindStringSearch search(p_trigger->propFindString(), text);
            if(search.Next())
            {
               p_word_trigger=p_trigger;
               break;
            }
         }

         if(!p_word_trigger)
         {
            p_word_trigger=MakeCounting<Prop::Trigger>();
            p_word_trigger->propFindString()=std::move(propFindString);
            mp_connection->GetServer()->propTriggers().Push(p_word_trigger);
            CloseDialog_Triggers(); // Close the dialog if we created a new trigger as it won't be in the tree view
         }

         CreateDialog_Triggers(*this, mp_connection->GetServer(), nullptr, p_word_trigger);
      });
   }

   menu.Do(TrackPopupMenu(menu, TPM_RETURNCMD, GetCursorPos(), *this, nullptr));
}

void Wnd_Main::SetScripterWindow()
{
   if(Scripter::HasInstance())
   {
      GetDispatch();
      Scripter::GetInstance().SetWindow(mp_dispatch);
   }
}

Scripter *Wnd_Main::GetScripter()
{
   if(!Scripter::HasInstance())
      InitScripter();
   if(!Scripter::HasInstance())
   {
      mp_wnd_text->AddHTML(STR_NoScriptingEngine);
      return nullptr;
   }

   SetScripterWindow();
   return &Scripter::GetInstance();
}

OM::MainWindow *Wnd_Main::GetDispatch()
{
   if(!mp_dispatch) mp_dispatch=MakeCounting<OM::MainWindow>(this);
   return mp_dispatch;
}

void Wnd_Main::InitScripter()
{
   if(!g_ppropGlobal->pclScriptLanguage())
      return; // No language

   try
   {
      new Scripter(g_ppropGlobal->pclScriptLanguage());
   }
   catch(const std::runtime_error &)
   {
      MessageBox(*this, STR_ErrorInitingScript, STR_Note, MB_OK|MB_ICONEXCLAMATION);
   }
}

Wnd_Main::~Wnd_Main()
{
   if(mp_dispatch)
      mp_dispatch->Destroyed();

   if(mp_connection->IsConnected())
      mp_connection->Disconnect();

   Save();

   // Any non docked edit panes will need to be manually destroyed now
   while(m_edit_panes)
      m_edit_panes.Pop()->Destroy();

   DestroyPanes();
   Assert(!m_spawn_windows);
   Assert(!m_spawn_tabs_windows);

   GetMDI().DeleteWindow(*this);
}

bool SaveDockedWindowSettings(Prop::DockedWindow &propWindow, Wnd_Docking &window)
{
   if(auto typeID=Msg::_GetTypeID().Send(window))
   {
      void *p_this=Msg::_GetThis().Send(window);

      if(typeID==GetTypeID<Wnd_InputPane>())
      {
         propWindow.iType(0);
         auto *pWnd=reinterpret_cast<Wnd_InputPane*>(p_this);

         if(&pWnd->m_input.GetProps()==&GlobalInputSettings())
            propWindow.ResetInputWindow();
         else
            propWindow.propInputWindow()=pWnd->m_input.GetProps();
         return true;
      }
      else if(typeID==GetTypeID<SpawnWindow>())
      {
         propWindow.iType(1);
         auto *pWnd=reinterpret_cast<SpawnWindow*>(p_this);
         Prop::SpawnWindow &propSpawnWindow=propWindow.propSpawnWindow();
         propSpawnWindow.pclTitle(pWnd->m_title);

         if(pWnd->mp_prop_text_window==&GlobalTextSettings())
            propSpawnWindow.ResetTextWindow();
         else
            propSpawnWindow.propTextWindow()=*pWnd->mp_prop_text_window;
         return true;
      }
      else if(typeID==GetTypeID<SpawnTabsWindow>())
      {
         propWindow.iType(2);
         auto *pWnd=reinterpret_cast<SpawnTabsWindow*>(p_this);
         Prop::SpawnTabsWindow &propSpawnTabsWindow=propWindow.propSpawnTabsWindow();
         propSpawnTabsWindow.pclTitle(pWnd->m_title);

         for(auto &tab : pWnd->GetTabs())
         {
            Prop::SpawnWindow &propSpawnWindow=*propSpawnTabsWindow.propTabs().Push(MakeUnique<Prop::SpawnWindow>());
            propSpawnWindow.pclTitle(tab.m_title);
            if(tab.mp_prop_text_window==&GlobalTextSettings())
               propSpawnWindow.ResetTextWindow();
            else
               propSpawnWindow.propTextWindow()=*tab.mp_prop_text_window;
         }
         return true;
      }
      else if(typeID==GetTypeID<Wnd_Image>())
      {
         propWindow.iType(3);
         return true;
      }
      else if(typeID==GetTypeID<Wnd_TileMap>())
      {
         propWindow.iType(4);
         auto *pWnd=reinterpret_cast<IWnd_TileMap*>(p_this);
         Prop::TileMapWindow &propTileMapWindow=propWindow.propTileMapWindow();
         propTileMapWindow.pclTitle(pWnd->GetTitle());
         return true;
      }
      else if(typeID==GetTypeID<Stats::Wnd>())
      {
         propWindow.iType(5);
         auto *pWnd=reinterpret_cast<Stats::Wnd*>(p_this);
         Prop::StatsWindow &propStatsWindow=propWindow.propStatsWindow();
         propStatsWindow.pclTitle(pWnd->GetTitle());
         propStatsWindow.clrBackground(pWnd->GetBackground());
         return true;
      }
      else if(typeID==GetTypeID<Wnd_EditPropertyPane>())
      {
         propWindow.iType(6);
         return true;
      }
      else if(typeID==GetTypeID<Maps::Wnd>())
      {
         propWindow.iType(7);
         auto *pWnd=reinterpret_cast<Maps::Wnd*>(p_this);
         Prop::MapWindow &propMapWindow=propWindow.propMapWindow();
         propMapWindow.pclFileName(pWnd->m_filename);
         propMapWindow.iMapIndex(pWnd->GetMapIndex());
         propMapWindow.iCurrentMapIndex(pWnd->GetCurrentMapIndex());
         propMapWindow.iCurrentRoomIndex(pWnd->GetCurrentRoomIndex());
         propMapWindow.Scale(pWnd->m_scale);
         propMapWindow.Origin(pWnd->m_origin);
         propMapWindow.fSelectionFilter_Rooms(pWnd->m_selection_filter.m_rooms);
         propMapWindow.fSelectionFilter_Rectangles(pWnd->m_selection_filter.m_rectangles);
         propMapWindow.fSelectionFilter_Images(pWnd->m_selection_filter.m_images);
         propMapWindow.fSelectionFilter_Labels(pWnd->m_selection_filter.m_labels);
         return true;
      }
#ifdef YARN
      else if(typeID==GetTypeID<Yarn::Wnd_Windows>())
      {
         propWindow.iType(8);
         return true;
      }
#endif
      else if(typeID==GetTypeID<Wnd_WebView>())
      {
         propWindow.iType(9);
         auto *pWnd=reinterpret_cast<Wnd_WebView *>(p_this);
         Prop::WebViewWindow &prop=propWindow.propWebViewWindow();
         prop.pclURL(pWnd->GetURL());
         prop.pclID(pWnd->GetID());
         prop.fHostObjects(pWnd->HostObjects());
         return true;
      }
   }

   Assert(false); // A window that we don't understand is docked, this shouldn't happen
   return false;
}

Wnd_Docking *Wnd_Main::RestoreDockedWindowSettings(Prop::DockedWindow &propWindow)
{
   switch(propWindow.iType())
   {
      case 0: // Wnd_InputPane
      {
         Wnd_InputPane &wnd=*CreateInputWindow(propWindow.fPropInputWindow() ? propWindow.propInputWindow() : GlobalInputSettings());
         return &wnd.GetDocking();
      }

      case 1: // SpawnWindow
      {
         auto &props=propWindow.propSpawnWindow();
         auto &wnd=*new SpawnWindow(m_spawn_windows.Prev(), *this, propWindow.propSpawnWindow().pclTitle(), props.fPropTextWindow() ? props.propTextWindow() : GlobalTextSettings());
         return &wnd.GetDocking();
      }

      case 2: // SpawnTabsWindow
      {
         auto &props=propWindow.propSpawnTabsWindow();
         auto &wnd=*new SpawnTabsWindow(m_spawn_tabs_windows.Prev(), *this, props.pclTitle());

         for(auto &pTab : props.propTabs())
            wnd.GetTab(pTab->pclTitle(), pTab->fPropTextWindow() ? &pTab->propTextWindow() : nullptr, false);

         return &wnd.GetDocking();
      }

      case 3: // ImageWindow
      {
         Assert(!mp_wnd_image); // Shouldn't be any yet
         if(mp_wnd_image)
            return nullptr;

         mp_wnd_image=CreateImageWindow(*this);
         return &mp_wnd_image->GetDocking();
      }

      case 4: // Wnd_TileMap
      {
         auto &props=propWindow.propTileMapWindow();
         auto &tile_maps=EnsureTileMaps();

         auto &wnd=tile_maps.Create(props.pclTitle());
         return &wnd.GetDocking();
      }

      case 5: // StatsWindow
      {
         auto &props=propWindow.propStatsWindow();
         auto &wnd=GetStatsWindow(props.pclTitle(), false);
         wnd.SetBackground(props.clrBackground());

         return &wnd.GetDocking();
      }

      case 6: // Character Notes Window
      {
         auto *pCharacter=mp_connection->GetCharacter();
         Assert(mp_connection->GetCharacter());
         if(!pCharacter)
            return nullptr;

         Assert(!mp_character_notes_pane);
         mp_character_notes_pane=MakeUnique<Wnd_EditPropertyPane>(*this, *pCharacter);
         return &mp_character_notes_pane->GetDocking();
      }

      case 7: // Map window
      {
         Assert(!mp_wnd_map); // Shouldn't be any yet
         if(mp_wnd_map)
            return nullptr;

         try { mp_wnd_map=MakeUnique<Maps::Wnd>(*this); }
         catch(const std::runtime_error &)
         {
            return nullptr; // Probably on Wine where we failed to create Direct2D
         }

         auto &props=propWindow.propMapWindow();
         if(props.pclFileName())
         {
            mp_wnd_map->Open(props.pclFileName());
            mp_wnd_map->SetMapIndex(props.iMapIndex());
            mp_wnd_map->SetCurrentPosition(props.iCurrentMapIndex(), props.iCurrentRoomIndex());
            mp_wnd_map->m_scale=props.Scale();
            mp_wnd_map->m_origin=props.Origin();
            mp_wnd_map->m_selection_filter.m_rooms=props.fSelectionFilter_Rooms();
            mp_wnd_map->m_selection_filter.m_rectangles=props.fSelectionFilter_Rectangles();
            mp_wnd_map->m_selection_filter.m_images=props.fSelectionFilter_Images();
            mp_wnd_map->m_selection_filter.m_labels=props.fSelectionFilter_Labels();
            mp_wnd_map->UpdateSelectionFilter();
         }

         return &mp_wnd_map->GetDocking();
      }

#if YARN
      case 8: // Yarn Map Window
      {
         Assert(!mp_yarn_tilemap);
         if(mp_yarn_tilemap)
            return nullptr;

         try { mp_yarn_tilemap=IYarn_TileMap::Create(*this); }
         catch(const std::runtime_error &)
         {
            return nullptr; // Probably failed to create Direct2D
         }

         return &mp_yarn_tilemap->GetDocking();
      }
#endif

      case 9: // WebView
      {
         auto &props=propWindow.propWebViewWindow();
         auto &wnd=*new Wnd_WebView(*this, props.pclID());
         wnd.SetURL(props.pclURL());
         return &wnd.GetDocking();
      }
   }

   Assert(false);
   return nullptr;
}

void Wnd_Main::Save()
{
   if(auto *p_prop_character=mp_connection->GetCharacter())
   {
      if(auto *p_prop_puppet=mp_connection->GetPuppet())
         SaveDockingConfiguration(p_prop_puppet->propDocking());
      else
         SaveDockingConfiguration(p_prop_character->propDocking());
   }

   if(mp_character_notes_pane)
      mp_character_notes_pane->Save();

   SaveMainWindowSettings(GetWindowSettings());
}

void Wnd_Main::SaveDockingConfiguration(Prop::Docking &propDocking)
{
   Prop::DockedPanes &propPanes=propDocking.propDockedPanes();
   propPanes.Empty();

   propDocking.ClientSize(ClientSize());

   for(auto &frame : GetFrames())
   {
      auto &propPane=*propPanes.Push(MakeUnique<Prop::DockedPane>());
      propPane.iSide(frame.side());
      propPane.iSize(frame.Size());
      auto &propWindows=propPane.propWindows();

      for(auto &window : frame.GetWindows())
      {
         auto &propWindow=*propWindows.Push(MakeUnique<Prop::DockedWindow>());
         int2 windowSize=window.WindowSize();
         propWindow.iSize(frame.direction()==Direction::Horizontal ? windowSize.x : windowSize.y);
         propWindow.fVerticalCaption(window.IsVerticalCaption());
         propWindow.fHideCaption(window.IsCaptionHidden());

         if(!SaveDockedWindowSettings(propWindow, window))
            propWindows.Pop();
      }
   }

   Prop::FloatingWindows &propFloatingWindows=propDocking.propFloatingWindows();
   propFloatingWindows.Empty();

   for(auto &window : GetFloating())
   {
      auto &propWindow=*propFloatingWindows.Push(MakeUnique<Prop::DockedWindow>());
      propWindow.rcRect(window.WindowRect());

      if(!SaveDockedWindowSettings(propWindow, window))
         propFloatingWindows.Pop();
   }
}

void Wnd_Main::DestroyPanes()
{
   // Close all existing spawns in favor of the new one
   while(GetFrames().Linked())
      delete GetFrames().Next();
   while(GetFloating().Linked())
      GetFloating().Next()->Destroy();
   m_input_panes.Empty();
}

void Wnd_Main::RestoreDockingConfiguration(Prop::Docking &propDocking)
{
   DestroyPanes();
   {
      RestorerOf _(m_suspend_layout); m_suspend_layout=true;

      Prop::DockedPanes &propPanes=propDocking.propDockedPanes();

      int2 clientSize=ClientRect().size();
      // The client size can't be zero, otherwise we'll crash on layout (not sure how this can happen, but it has)
      PinAbove(clientSize.x, 128);
      PinAbove(clientSize.y, 128);

      int2 originalClient=propDocking.ClientSize();
      if(originalClient.x<1 || originalClient.y<1)
         return; // Bogus size or default initialized, either way exit

      for(auto &propFrame : propPanes)
      {
         auto side=Docking::Side(propFrame->iSide());
         int2 frameSize=clientSize;
         int2 originalFrameSize=originalClient;
         bool isHorizontal=Docking::ToDirection(side)==Direction::Horizontal;

         // Setup size so that size.x corresponds to the frame's size, and size.y to the space for docked windows in it
         // So for a vertical (left/right) frame, nothing changes, but for a horizontal one x and y swap
         if(isHorizontal)
         {
            std::swap(frameSize.x, frameSize.y);
            std::swap(originalFrameSize.x, originalFrameSize.y);
         }

         frameSize.x=max(propFrame->iSize()*frameSize.x/originalFrameSize.x, 10*g_dpiScale);
         Docking::Frame &frame=CreateFrame(side, frameSize.x);

         for(auto &ppropWindow : propFrame->propWindows())
         {
            Wnd_Docking *pWnd=RestoreDockedWindowSettings(*ppropWindow);
            if(!pWnd)
               continue;

            pWnd->SetVerticalCaption(ppropWindow->fVerticalCaption());
            pWnd->SetHideCaption(ppropWindow->fHideCaption());
            int2 size(frameSize.x, ppropWindow->iSize()*frameSize.y/originalFrameSize.y);
            if(isHorizontal)
               std::swap(size.x, size.y);

            frame.Add(*pWnd, nullptr, &size);
         }

         if(frame.m_window_count==0)
         {
            Assert(false); // Only happens if we have a screwed up config
            delete &frame;
         }
      }

      for(auto &ppropWindow : propDocking.propFloatingWindows())
      {
         Wnd_Docking *pWnd=RestoreDockedWindowSettings(*ppropWindow);
         if(!pWnd)
            continue;

         pWnd->SetPosition(ppropWindow->rcRect());
         pWnd->EnsureOnScreen();
         pWnd->Show(SW_SHOWNOACTIVATE);
      }
   }
   DockingChange();
}

// IError
void Wnd_Main::Error(ConstString string)
{
   mp_connection->Text(string, Colors::Red);
}

void Wnd_Main::Text(ConstString string)
{
   mp_connection->Text(string, Colors::Green);
}

void Wnd_Main::SendInput(InputControl &edInput)
{
   m_history_pos=~0U; // Reset the current history item
   mp_wnd_text_history->SetUserPaused(false);
   Assert(!mp_wnd_text_history->IsPaused());

   auto string=edInput.GetText();

   if(mp_input_active->GetProps().fSticky())
      edInput.SetSelAll();
   else
      edInput.SetText("");

   if(auto *pCharacter=mp_connection->GetCharacter();pCharacter && !mp_connection->InReplay())
   {
      if(auto index=pCharacter->iRestoreLogIndex();index!=-1)
         gp_restore_logs->WriteSent(index, string);
   }

   m_strText.Clear();
   History_AddToHistory(string, edInput.GetProps().pclPrefix());

   if(edInput.GetProps().fLocalEcho())
      mp_connection->Text(string, edInput.GetProps().clrLocalEchoColor());

   SendLines(string, edInput.GetProps().pclPrefix());
}

void Wnd_Main::History_CheckIfInputModified(InputControl &edInput)
{
   // If someone edits a history item while navigating the history we want to now add that to the history
   const Text::Lines &lines=mp_wnd_text_history->GetTextList().GetLines();

   // If there's no text or we're past the last line of the history, do nothing
   if(edInput.GetTextLength()==0 || m_history_pos==lines.Count())
      return;

   auto string=edInput.GetText();

   edInput.SetText("");
   History_AddToHistory(string, edInput.GetProps().pclPrefix());
}

void Wnd_Main::History_AddToHistory(ConstString string, ConstString prefix, uint64 time)
{
   if(!string || string.Count()>65536) // Empty or huge strings don't get put into the history
      return;

   auto pLine=Text::Line::CreateFromText(string);

   const Text::Lines &lines=mp_wnd_text_history->GetTextList().GetLines();
   // Is there a duplicate item in the history? If so, don't add this line
   if(lines.Count())
   {
      unsigned iCurrentLine=m_history_pos==~0 ? lines.Count()-1 : m_history_pos;
      auto lineText=lines[iCurrentLine].GetText();

      if(lineText==*pLine || lineText.Count()>prefix.Count() && lineText.WithoutFirst(prefix.Count())==*pLine)
         return;
   }

   if(prefix)
   {
      pLine->InsertText(0, prefix);
      pLine->SetBold(uint2(0, prefix.Count()), true);
   }

   mp_wnd_text_history->SetUserPaused(false);
   Assert(!mp_wnd_text_history->IsPaused());

   if(time)
      pLine->Time()=Time::Time{Time::UTCToLocal(time)};
   mp_wnd_text_history->Add(std::move(pLine));
}

void Wnd_Main::History_SelectUp(InputControl &edInput)
{
   const Text::Lines &lines=mp_wnd_text_history->GetTextList().GetLines();
   if(lines.Count()==0)
      return; // No history, nothing to do

   if(m_history_pos==~0U)
   {
      // Save the current line of text (that is not yet on the queue)
      if(!m_strText)
         m_strText=edInput.GetText();

      m_history_pos=lines.Count();
   }

   // Are we before the last item of the history?
   if(m_history_pos>0)
   {
      History_CheckIfInputModified(edInput);
      m_history_pos--;
      History_SelectLine(edInput);
   }

   CheckInputHeight();
}

void Wnd_Main::History_SelectDown(InputControl &edInput)
{
   if(m_history_pos==~0)  // Save current item
   {
      History_CheckIfInputModified(edInput);
      return;
   }

   const Text::Lines &lines=mp_wnd_text_history->GetTextList().GetLines();

   if(m_history_pos<lines.Count())
   {
      History_CheckIfInputModified(edInput);
      m_history_pos++;

      // If we're at the end of the list, restore the old line of text (that is not on the Queue)
      if(m_history_pos==lines.Count())
      {
         m_history_pos=~0U;  // No history item is selected now
         edInput.SetText(m_strText);
         edInput.SetSelEnd();
         m_strText.Clear();
         mp_wnd_text_history->SetUserPaused(false);
      }
      else
         History_SelectLine(edInput);
   }

   CheckInputHeight();
}

void Wnd_Main::History_SelectLine(InputControl &edInput)
{
   HybridStringBuilder string;
   mp_wnd_text_history->GetTextList().GetLines()[m_history_pos].TextCopy(string);
   edInput.SetText(string);
   edInput.SetSelEnd();

   mp_wnd_text_history->SetUserPaused(true);
   mp_wnd_text_history->SelectLine(m_history_pos);
}

void Wnd_Main::ActivateWindow(Direction::PN dir)
{
   Wnd_Main *p=nullptr;
   if(dir==Direction::Next)
   {
      p=DLNode<Wnd_Main>::Next();
      if(p==&m_wnd_MDI.GetRootWindow())
         p=p->DLNode<Wnd_Main>::Next();
   }
   else
   {
      p=DLNode<Wnd_Main>::Prev();
      if(p==&m_wnd_MDI.GetRootWindow())
         p=p->DLNode<Wnd_Main>::Prev();
   }

   GetMDI().SetActiveWindow(*p);
}

//
// Window Messages
//
LRESULT Wnd_Main::On(const Msg::Create &msg)
{
   Wnd_Container::SetWindow(*this);
   mp_layout=MakeUnique<AL::LayoutEngine>(*this);

   mp_wnd_text=new Text::Wnd(*this, *this);
   mp_wnd_text_history=new Text::Wnd(*this, *this);

   AttachTo<Connection::Event_Log>(*mp_connection);
   AttachTo<Connection::Event_Activity>(*mp_connection);
   AttachTo<Connection::Event_Connect>(*mp_connection);
   AttachTo<Connection::Event_Disconnect>(*mp_connection);
   AttachTo<Connection::Event_Send>(*mp_connection);
   AttachTo<GlobalTextSettingsModified>(g_text_events);
   AttachTo<GlobalInputSettingsModified>(g_text_events);

   m_input.Create(*this);

   int2 minSize(10, 10);
   mp_wnd_text->SetSize(minSize);
   mp_wnd_text_history->SetSize(minSize);
   m_input.SetSize(minSize);

   auto *pOutput=mp_layout->AddChildWindow(*mp_wnd_text);
   auto *pHistory=mp_layout->AddChildWindow(*mp_wnd_text_history); pHistory->weight(0);
   auto *pInput=mp_layout->AddChildWindow(m_input); pInput->weight(0);

   mp_splitter=mp_layout->CreateSplitter(Direction::Vertical);
   *mp_splitter << pOutput << pHistory;

   mp_splitter_input=mp_layout->CreateSplitter(Direction::Vertical);
   *mp_splitter_input << mp_splitter << pInput;

   AL::Group_Vertical *pGV=mp_layout->CreateGroup_Vertical(); *pGV << AL::Style::NoPadding;
   {
      mp_client_layout_helper=new ClientLayoutHelper(*this, *mp_splitter_input);
      *pGV << mp_client_layout_helper;
   }
   mp_layout->SetRoot(pGV);

   ApplyMainWindowSettings();
   return msg.Success();
}

LRESULT Wnd_Main::On(const Msg::Close &msg)
{
   if(g_ppropGlobal->propWindows().fAskBeforeDisconnecting() && mp_connection->IsConnected())
   {
      if(MessageBox(hWnd(), STR_StillConnected, STR_Note, MB_ICONQUESTION|MB_YESNO)==IDNO)
         return msg.Failure();
   }
   else if(g_ppropGlobal->propWindows().fAskBeforeClosing())
   {
      if(MessageBox(hWnd(), STR_AreYouSure, STR_CloseWindow, MB_ICONWARNING|MB_YESNO)==IDNO)
         return msg.Failure();
   }

   if(m_events.Get<Event_Close>())
   {
      SetScripterWindow();
      m_events.Send(Event_Close());
   }

   Destroy();
   return msg.Success();
}

void Wnd_Main::SaveMainWindowSettings(Prop::MainWindowSettings &settings)
{
   if(mp_prop_output==&GlobalTextSettings())
      settings.ResetOutput();
   else
      settings.propOutput()=*mp_prop_output;

   if(mp_prop_history==&GlobalTextSettings())
      settings.ResetHistory();
   else
      settings.propHistory()=*mp_prop_history;

   if(&m_input.GetProps()==&GlobalInputSettings())
      settings.ResetInput();
   else
      settings.propInput()=m_input.GetProps();

   settings.iHistorySize(mp_splitter->GetSize(1));
   settings.iInputSize(mp_splitter_input->GetSize(1));
}

void Wnd_Main::ApplyMainWindowSettings()
{
   auto &settings=GetWindowSettings();
   mp_splitter->SetSize(1, settings.iHistorySize());
   mp_splitter_input->SetSize(1, settings.iInputSize());

   ApplyHistoryVisibility();

   mp_prop_output=settings.fPropOutput() ? &settings.propOutput() : &GlobalTextSettings();
   mp_prop_history=settings.fPropHistory() ? &settings.propHistory() : &GlobalTextSettings();
   m_input.SetProps(settings.fPropInput() ? settings.propInput() : GlobalInputSettings());

   SetTextWindowProperties(*mp_wnd_text_history, *mp_prop_history);
   SetTextWindowProperties(*mp_wnd_text, *mp_prop_output);
   ApplyInputProperties();
}

void Wnd_Main::ApplyHistoryVisibility()
{
   auto &settings=GetWindowSettings();
   mp_splitter->SetVisibility(1, settings.fHistory());
   mp_wnd_text_history->Show(settings.fHistory() ? SW_SHOW : SW_HIDE);
   mp_wnd_text_history->GetTextList().SelectionClear();

   mp_layout->CalcMinSize();
   mp_layout->Update();
}

void Wnd_Main::On(const GlobalTextSettingsModified &event)
{
   // If any of our text windows use the global settings, refresh them
   if(mp_prop_output==&GlobalTextSettings())
      ::SetTextWindowProperties(*mp_wnd_text, *mp_prop_output);
   if(mp_prop_history==&GlobalTextSettings())
      ::SetTextWindowProperties(*mp_wnd_text_history, *mp_prop_history);
}

void Wnd_Main::On(const GlobalInputSettingsModified &event)
{
   if(&m_input.GetProps()==&event.prop)
      ApplyInputProperties();
}

LRESULT Wnd_Main::On(const Msg::Notify &msg)
{
   if(Window{msg.pnmh()->hwndFrom}==m_input)
      return m_input.On(msg);

   return 0;
}

LRESULT Wnd_Main::On(const Msg::Command &msg)
{
   if(msg.wndCtl()==m_input)
      return m_input.On(msg);

   switch(msg.iID())
   {
      // This is sent by the taskbar context menu, when sent here it closes a tab
      case ID_FILE_CLOSETAB: On(Msg::Close()); return msg.Success();

      case ID_FILE_NEWINPUT:
         AddInputWindow("", false);
         return msg.Success();

      case ID_FILE_NEWEDIT:
         GetEditPane({}, false, true);
         return msg.Success();

      case ID_TABCOLOR:
      {
         if(auto *p_puppet=mp_connection->GetPuppet())
         {
            Color color=p_puppet->clrTabColor();
            if(ChooseColorSimple(*this, &color))
               TabColor(color);
         }
         else if(auto *p_character=mp_connection->GetCharacter())
         {
            Color color=p_character->clrTabColor();
            if(ChooseColorSimple(*this, &color))
               TabColor(color);
         }
         else
            MessageBox(*this, "Only tabs with characters or puppets can have tab colors", "Note:", MB_ICONINFORMATION|MB_OK);
         return msg.Success();
      }

      case ID_TABCOLORDEFAULT: TabColor(Colors::Transparent); return msg.Success();

      case ID_MUTE: ToggleShowActivityOnTaskbar(); return msg.Success();
#if 0
      case ID_CONNECTION_STATISTICS: CreateDialog_Statistics(*this); return msg.Success();
#endif

      case ID_EDIT_SMARTPASTE:
         if(mp_connection->IsConnected())
            CreateDialog_SmartPaste(*this, *mp_connection, g_ppropGlobal->propConnections());
         return msg.Success();

      case ID_EDIT_PASTE:
         Msg::Paste().Send(*mp_input_active);
         return msg.Success();

      case ID_EDIT_COPY:
         mp_wnd_text->SelectionToClipboard();
         return msg.Success();

      case ID_EDIT_FIND:
         CreateDialog_Find(*this, *mp_wnd_text);
         return msg.Success();

      case ID_EDIT_FINDINPUTHISTORY:
         CreateDialog_Find(*this, *mp_wnd_text_history);
         return msg.Success();

      case ID_EDIT_SELECTALL:
         mp_wnd_text->SelectAll();
         return msg.Success();

      case ID_EDIT_COPYDOCKING:
         SaveDockingConfiguration(g_propDockingClipboard);
         *g_ppropMainWindowSettingsClipboard=*mp_prop_main_window_settings;
         SaveMainWindowSettings(*g_ppropMainWindowSettingsClipboard); // Save the parts that don't make it on a copy
         return msg.Success();

      case ID_EDIT_PASTEDOCKING:
         if(!mp_connection->GetServer()) // Doesn't work until we have a server
            return msg.Success();
         RestoreDockingConfiguration(g_propDockingClipboard);
         *mp_prop_main_window_settings=*g_ppropMainWindowSettingsClipboard;
         ApplyMainWindowSettings();
         return msg.Success();

      case ID_CONNECTION_CONNECT:
         CreateDialog_Connect(*this, GetMDI());
         return msg.Success();

      case ID_CONNECTION_DISCONNECT:
         if(mp_connection->IsConnected())
            mp_connection->Disconnect();
         return msg.Success();

      case ID_CONNECTION_RECONNECT:
         if(!mp_connection->Reconnect())
            mp_connection->Text(STR_CantReconnect);
         return msg.Success();

      case ID_LOGGING:
         if(mp_connection->IsLogging())
         {
            mp_connection->LogStop();
            return msg.Success();
         }
         // Otherwise fall through to start logging!
      case ID_LOGGING_START:
      case ID_LOGGING_FROMBEGINNING:
      case ID_LOGGING_FROMWINDOW:
         {
            Assert(!mp_connection->IsLogging());

            auto &propLogging=g_ppropGlobal->propConnections().propLogging();

            File::Chooser cf;
            cf.SetTitle(STR_LogToFile);
            cf.SetFilter(STR_LogFileFilter, g_ppropGlobal->iLogFileFilter());
            cf.SetDirectory(propLogging.pclPath());

            FixedStringBuilder<256> fileName;
            Time::Local().FormatDate(fileName, propLogging.pclFileDateFormat());

            if(cf.Choose(*this, fileName, true))
            {
               g_ppropGlobal->iLogFileFilter(cf.GetFilterIndex());
               mp_connection->LogStart(fileName, msg.iID());
               propLogging.pclPath(ConstString(fileName.begin(), cf.GetFileOffset()));
            }
         }
         return msg.Success();

      case ID_LOGGING_STOP:
         Assert(mp_connection->IsLogging());
         mp_connection->LogStop();
         return msg.Success();

      case ID_OPTIONS_IMAGEWINDOW:
         if(!mp_wnd_image)
            EnsureImageWindow();
         else
            mp_wnd_image=nullptr;
         return msg.Success();

      case ID_OPTIONS_MAPWINDOW:
         if(!mp_wnd_map)
         {
            try
            {
               EnsureMapWindow();
            }
            catch(const std::runtime_error &)
            {
               // Nothing to do, the user would have already seen the Direct2D error on startup
            }
         }
         else
            mp_wnd_map=nullptr;
         return msg.Success();

      case ID_OPTIONS_CHARNOTESWINDOW:
         if(!mp_character_notes_pane)
            ShowCharacterNotesPane();
         else
            mp_character_notes_pane=nullptr;
         return msg.Success();

      case ID_OPTIONS_INPUT_HISTORY:
      {
         auto &settings=GetWindowSettings();
         settings.fHistory(!settings.fHistory());
         ApplyHistoryVisibility();
         return msg.Success();
      }

      case ID_OPTIONS_MACROS:
         CreateDialog_KeyboardMacros(*this, mp_connection->GetServer(), mp_connection->GetCharacter());
         return msg.Success();

      case ID_OPTIONS_TRIGGERS:
         CreateDialog_Triggers(*this, mp_connection->GetServer(), mp_connection->GetCharacter(), nullptr);
         return msg.Success();

      case ID_OPTIONS_ALIASES:
         CreateDialog_Aliases(*this, mp_connection->GetServer(), mp_connection->GetCharacter());
         return msg.Success();

      case ID_OPTIONS_PREFERENCES: CreateDialog_Settings(*this); return msg.Success();

      case ID_HELP_DEBUG_NETWORK: mp_connection->OpenNetworkDebugWindow(); return msg.Success();
      case ID_HELP_DEBUG_TRIGGERS: mp_connection->OpenTriggerDebugWindow(); return msg.Success();

      case ID_HELP_CONTENTS:
      {
         OpenHelpURL("README.md");
//         OpenURLAsync("https://www.reddit.com/r/BeipMU/wiki/index");
         return msg.Success();
      }

      case ID_HELP_CHANGES:
      {
         File::Path path{GetResourcePath(), "Changes.txt"};

         if(!path.Exists())
         {
            OpenURLAsync("https://raw.githubusercontent.com/BeipDev/BeipMU/master/Assets/Changes.txt");
            return msg.Success();
         }

         OpenURLAsync(path);
         return msg.Success();
      }
   }

   return msg.Failure();
}

LRESULT Wnd_Main::On(const Msg::Activate &msg)
{
   if(msg.uState()!=WA_INACTIVE && msg.fMinimized()==false)
      SetActive(true);

   if(msg.uState()==WA_INACTIVE)
      SetActive(false);

   return msg.Success();
}

unsigned Wnd_Main::GetUnreadCount() const
{
   unsigned unread_count=mp_wnd_text->GetUnreadCount();
   for(auto &window : m_spawn_windows)
      unread_count+=window.mp_text->GetUnreadCount();
   for(auto &tabs : m_spawn_tabs_windows)
      unread_count+=tabs.GetUnreadCount();

   return unread_count;
}

void Wnd_Main::AddImportantActivity()
{
   if(m_active)
      return;

   m_important_activity_count++;
   GetMDI().RefreshTaskbar(*this);
}

void Wnd_Main::SetActive(bool active)
{
   if(m_active==active)
      return;

   m_active=active;

   {
      bool fAway=!m_active;

      mp_wnd_text->SetAway(fAway);
      // Propagate away to all spawn panes
      for(auto &window : m_spawn_windows)
         window.mp_text->SetAway(fAway);
      for(auto &window : m_spawn_tabs_windows)
         window.SetAway(fAway);
   }

   if(m_active)
   {
      mp_connection->Away(false);

      FlashTab();
      m_wnd_MDI.RefreshBadgeCount();
      m_input.SetFocus();

      // Bring all of the floating panes to the top
      for(auto &window : GetFloating())
         window.Window::InsertAfter(HWND_TOP);

      if(m_events.Get<Event_Activate>())
      {
         SetScripterWindow();
         m_events.Send(Event_Activate(true));
      }
   }
   else
   {
      mp_connection->Away(true);

      GetMDI().RefreshTaskbar(*this); // Either way, redraw the window's unread count
      m_important_activity_count=0;

      if(m_events.Get<Event_Activate>())
      {
         SetScripterWindow();
         m_events.Send(Event_Activate(false));
      }
   }
}

LRESULT Wnd_Main::On(const Msg::SetFocus &msg)
{
   m_input.SetFocus();
   return __super::WndProc(msg);
}

LRESULT Wnd_Main::On(const Msg::LButtonDown &msg)
{
   SetFocus(); // To generate a setfocus message which gets the focus into the edit window
   return msg.Success();
}

LRESULT Wnd_Main::On(const Msg::XButtonDown &msg)
{
   if(msg.button()==XBUTTON1)
      ActivateWindow(Direction::Previous);

   if(msg.button()==XBUTTON2)
      ActivateWindow(Direction::Next);

   return msg.Success();
}

bool Wnd_Main::EditChar(InputControl &edInput, char ch)
{
   // Only allow Ctrl+Enter to go through
   if((ch==CHAR_CR || ch==CHAR_LF) && !(GetKeyState(VK_CONTROL)&0x80))
      return true;

   if(m_ignore_next_char)
   {
      m_ignore_next_char=false;
      return true;
   }

   return false;
}

LRESULT Wnd_Main::On(const Msg::Char &msg)
{
   Assert(0);
   return msg.Success();
}

bool Wnd_Main::EditKey(InputControl &edInput, const Msg::Key &msg)
{
   bool fProcessed=!ProcessEditKey(edInput, msg);
   m_ignore_next_char=fProcessed; // If we handled this char ignore the next char message

   if(msg.direction()==Direction::Up)
   {
      m_events.Send(Event_InputChanged());
      if(g_ppropGlobal->fTaskbarShowTyped())
         m_wnd_MDI.GetTaskbar().Refresh();
   }
   CheckInputHeight();

   return fProcessed;
}

void Wnd_Main::CheckInputHeight()
{
   // We only auto resize the primary input
   if(mp_input_active!=&m_input)
      return;

   const Prop::InputWindow &propInputWindow=mp_input_active->GetProps();
   if(!propInputWindow.fAutoSizeVertically())
      return;

   int iInputHeight=std::clamp(int(m_input.GetLineCount()), propInputWindow.iMinimumHeight(), propInputWindow.iMaximumHeight());
   if(iInputHeight==m_last_input_height)
      return;

   m_last_input_height=iInputHeight;
   int linesHeight=iInputHeight*m_tmInput.tmHeight;
   int padding=4+propInputWindow.rcMargins().top+propInputWindow.rcMargins().bottom;

   if(mp_splitter_input->GetSize(1) != linesHeight+padding)
   {
      mp_splitter_input->SetSize(1, linesHeight+padding);
      Update();
      mp_layout->Update();
      Update();
   }
}

//
bool Wnd_Main::ProcessEditKey(InputControl &edInput, const Msg::Key &msg)
//
// Returns true if Key should be processed by the Edit Control
{
   bool fDown=msg.direction()==Direction::Down;

   KEY_ID key;
   key.iVKey=msg.iVirtKey();
   key.fControl=IsKeyPressed(VK_CONTROL);
   key.fShift=IsKeyPressed(VK_SHIFT);
   key.fAlt=IsKeyPressed(VK_MENU);

   // Refresh the taskbar when Alt is pressed/released to show the tab numbers
   if(key.iVKey==VK_MENU && !(fDown && msg.fRepeating()))
      m_wnd_MDI.GetTaskbar().DrawTabNumbers(fDown);

   // Ignore these keys as we don't do anything on them
   if(key.iVKey==VK_CONTROL || key.iVKey==VK_SHIFT || key.iVKey==VK_MENU)
      return true;

   // Eat smart quote keys if they're enabled
   if(key.iVKey==VK_OEM_7 && key.fControl && key.fShift && g_ppropGlobal->fPreventSmartQuotes())
      return false;

   // Don't process macros when the key is A-Z and no control or alt keys are pressed
   if(fDown && (key.fControl || key.fAlt || !IsBetween<int>(key.iVKey, 'A', 'Z')) &&
       g_ppropGlobal->propConnections().propKeyboardMacros().fActive())
   {
      const CKeyMacro *pMacro=mp_connection->MacroKey(&key);

      if(pMacro)
      {
         // If we get a macro with an fType, we must type it in
         if(pMacro->fType)
            edInput.ReplaceSel(pMacro->pclMacro);
         else
            SendLines(pMacro->pclMacro);

         // Windows generates number keys for these entries, so if we hit a macro, ignore the next
         // char message received
         if(IsBetween(key.iVKey, VK_NUMPAD0, VK_NUMPAD9))
            m_ignore_next_char=true;

         return false;
      }
   }

   // Handle Alt+# tab switching
   if(fDown && key.fAlt && !key.fControl && IsBetween<int>(key.iVKey, '0', '9'))
   {
      unsigned index=key.iVKey=='0' ? 10 : key.iVKey-'0';
      if(key.fShift)
         index+=10;
      index--; // Go from one based to zero based

      if(index<GetMDI().GetWindowCount())
         GetMDI().SetActiveWindow(GetMDI().GetWindow(index));

      return false;
   }

   // Send off a key event in case someone is listening
   if(fDown)
   {
      Event_Key event(key.iVKey);
      if(m_events.Send(event, event)) // If handler process key it'll return true, so we return false (meaning processed)
         return false;
   }

   const Prop::Keys &propKeys=g_ppropGlobal->propKeys();
   Assert(Key_Max==propKeys.Count());

   Keys eKey;
   {
      unsigned int iKey=0;
      for(iKey=0;iKey<Key_Max;iKey++)
         if(propKeys.Get(iKey).Matches(key))
            break;
      eKey=(Keys)iKey;
   }

   if(eKey==Key_Max)
   {
      // Ignore superscript/subscript
      if(key.iVKey==VK_OEM_PLUS && key.fControl && !key.fAlt)
         return false;

      return true;
   }

   if(fDown) // Only handle the key on the down press
      HandleKey(edInput, eKey);

   return false;
}

void Wnd_Main::HandleKey(InputControl &edInput, Keys key)
{
   switch(key)
   {
      case Key_Minimize:
      {
         GetMDI().Show(SW_MINIMIZE);
         return;
      }

      case Key_Hide:
      {
         MessageBeep(MB_ICONASTERISK);
#if 0 // Disabled since the notification icon menu is gone now
         if(!g_ppropGlobal->fNotificationIcon())
         {
            MessageBeep(MB_ICONASTERISK);
            return;
         }

         GetMDI().Show(SW_HIDE);
#endif
         return;
      }

      case Key_ClearActivity:
         // Clear activity markers by pretending we click away then back in the window
         SetActive(false);
         SetActive(true);
         return;

      case Key_Input_Send: SendInput(edInput); return;
      case Key_Input_RepeatLastLine:
      {
         const Text::Lines &lines=mp_wnd_text_history->GetTextList().GetLines();
         if(lines.Count())
            SendLines(lines.back().GetText());
         return;
      }

      case Key_Input_LineUp: Msg::_Edit_LineMove(Direction::Up).Post(edInput); return;
      case Key_Input_LineDown: Msg::_Edit_LineMove(Direction::Down).Post(edInput); return;
      case Key_Input_Clear: edInput.SetText(""); return;
      case Key_Input_NextInput: ActivateNextInputWindow(); return;
      case Key_Input_PushToHistory:
         History_CheckIfInputModified(*mp_input_active);
         if(m_history_pos!=~0U)
         {
            mp_wnd_text_history->SetUserPaused(true);
            mp_wnd_text_history->SelectLine(m_history_pos);
         }
         return;

      case Key_Input_Autocomplete:
      case Key_Input_AutocompleteWholeLine:
      {
         // Build up a list of all of the text to scan through, like output, history, and spawns
         Collection<const Text::Lines*> linesCollection;
         linesCollection.Push(&mp_wnd_text->GetTextList().GetLines());
         linesCollection.Push(&mp_wnd_text_history->GetTextList().GetLines());

         for(auto &tabWindow : m_spawn_tabs_windows)
            for(auto &window : tabWindow.GetTabs())
               linesCollection.Push(&window.mp_text->GetTextList().GetLines());
         for(auto &window : m_spawn_windows)
            linesCollection.Push(&window.mp_text->GetTextList().GetLines());

         mp_input_active->Autocomplete(linesCollection, key==Key_Input_AutocompleteWholeLine);
         return;
      }

      case Key_Output_PageUp: Msg::VScroll(SB_PAGEUP).Post(*mp_wnd_text); return;
      case Key_Output_PageDown: Msg::VScroll(SB_PAGEDOWN).Post(*mp_wnd_text); return;
      case Key_Output_LineUp: Msg::VScroll(SB_LINEUP).Post(*mp_wnd_text); return;
      case Key_Output_LineDown: Msg::VScroll(SB_LINEDOWN).Post(*mp_wnd_text); return;
      case Key_Output_Top: Msg::VScroll(SB_TOP).Post(*mp_wnd_text); return;
      case Key_Output_Bottom: Msg::VScroll(SB_BOTTOM).Post(*mp_wnd_text); return;

      case Key_History_PageUp: Msg::VScroll(SB_PAGEUP).Post(*mp_wnd_text_history); return;
      case Key_History_PageDown: Msg::VScroll(SB_PAGEDOWN).Post(*mp_wnd_text_history); return;
      case Key_History_SelectUp: History_SelectUp(edInput); return;
      case Key_History_SelectDown: History_SelectDown(edInput); return;
      case Key_History_Toggle: Msg::Command(ID_OPTIONS_INPUT_HISTORY, nullptr, 0).Post(GetMDI()); return;

      case Key_Imaging_Toggle: Msg::Command(ID_OPTIONS_IMAGEWINDOW, nullptr, 0).Post(GetMDI()); return;

      case Key_Window_Next: ActivateWindow(Direction::Next); return;
      case Key_Window_Prev: ActivateWindow(Direction::Previous); return;
      case Key_Window_Close: Close(); return;
      case Key_Window_CloseAll: Msg::Command(ID_FILE_QUIT, nullptr, 0).Post(GetMDI()); return;

      case Key_NewTab: Msg::Command(ID_FILE_NEWTAB, nullptr, 0).Post(GetMDI()); return;
      case Key_NewWindow: Msg::Command(ID_FILE_NEWWINDOW, nullptr, 0).Post(GetMDI()); return;

      case Key_Edit_Find: Msg::Command(ID_EDIT_FIND, nullptr, 0).Post(*this); return;
      case Key_Edit_FindHistory: Msg::Command(ID_EDIT_FINDINPUTHISTORY, nullptr, 0).Post(*this); return;
      case Key_Edit_SelectAll: edInput.SetSel(0, -1); return;
      case Key_Edit_Paste: Msg::Command(ID_EDIT_PASTE, nullptr, 0).Post(*this); return;
      case Key_Edit_Pause: mp_wnd_text->SetUserPaused(!mp_wnd_text->IsUserPaused()); return;
      case Key_Edit_SmartPaste: Msg::Command(ID_EDIT_SMARTPASTE, nullptr, 0).Post(*this); return;
      case Key_Edit_ConvertReturns: mp_input_active->ConvertReturns(); return;
      case Key_Edit_ConvertTabs: mp_input_active->ConvertTabs(); return;
      case Key_Edit_ConvertSpaces: mp_input_active->ConvertSpaces(); return;

      case Key_Connect: Msg::Command(ID_CONNECTION_CONNECT, nullptr, 0).Post(*this); return;
      case Key_Disconnect: Msg::Command(ID_CONNECTION_DISCONNECT, nullptr, 0).Post(*this); return;
      case Key_Reconnect: Msg::Command(ID_CONNECTION_RECONNECT, nullptr, 0).Post(*this); return;

      case Key_Logging_Toggle: Msg::Command(ID_LOGGING, nullptr, 0).Post(*this); return;

      case Key_SendTelnet_IP: mp_connection->Send("\xFF\xF4", true, true); return;
   }

   Assert(0);
}

void Wnd_Main::SendNAWS()
{
   if(auto *p_server=mp_connection->GetServer(); p_server && p_server->fNAWSOnResize())
   {
      auto size=mp_wnd_text->GetSizeInChars();
      mp_connection->GetTelnet().SendNAWS(size);
   }
}

void Wnd_Main::SendLine(ConstString _string, ConstString prefix)
//
// Send a line of text to the muck and handle aliases
//
{
   HybridStringBuilder string(_string);

   if(mp_connection->IsLogging())
      mp_connection->GetLog().LogTyped(string);

   if(mp_connection->ProcessAliases(string))
   {
      if(g_ppropGlobal->propConnections().propAliases().fEcho())
         mp_connection->Text(string, Colors::Green);

      bool process_commands=g_ppropGlobal->propConnections().propAliases().fProcessCommands();
      ConstString lines=string;
      while(lines)
      {
         ConstString line;
         if(!lines.Split(CRLF, line, lines))
         {
            line=lines;
            lines={};
         }

         if(process_commands && line.Length()>=2 && line.First()=='/') // Command line?
            ParseCommand(line);
         else
            mp_connection->Send(HybridStringBuilder{prefix, line});
      }
   }
   else
      mp_connection->Send(HybridStringBuilder{prefix, string});
}

void Wnd_Main::SendLines(ConstString string, ConstString prefix)
{
   // Look for script
   if(string.StartsWith("/@"))
   {
      if(auto *pScripter=GetScripter())
         pScripter->Run(OwnedBSTR(string.WithoutFirst(2)));
      return;
   }

   do
   {
      ConstString line;
      if(!string.Split(CRLF, line, string))
      {
         line=string;
         string={};
      }

      if(line.Length()>=2 && line.First()=='/') // Command line?
         ParseCommand(line);
      else
         SendLine(line, prefix);
   }
   while(string);
}

void Wnd_Main::ResetWindowSettings()
{
   mp_prop_main_window_settings=GetMainWindowSettings(mp_connection->GetCharacter(), mp_connection->GetPuppet());
   ApplyMainWindowSettings();
}

void Wnd_Main::ApplyInputProperties()
{
   m_input.ApplyProps();

   auto hfInput=m_input.GetProps().propFont().CreateFont();

   ScreenDC dc;
   dc.SelectFont(hfInput);
   dc.GetTextMetrics(m_tmInput);

   m_last_input_height=0; // To force a resize
   CheckInputHeight();
}

void Wnd_Main::FlashTab()
{
   if(m_active)
   {
      if(m_timer_tab_flash)
         m_timer_tab_flash.Reset();

      if(m_flash_state)
      {
         GetMDI().WindowFlash(*this, false);
         m_flash_state=false;
      }
   }
   else
   {
      m_flash_state^=true;
      GetMDI().WindowFlash(*this, true);
   }
}

void Wnd_Main::On_TileMap(ConstString command, ConstString json)
{
   if(!m_tile_maps_enabled)
      return;

   EnsureTileMaps().On(command, json);
}

void Wnd_Main::ShowStatistics()
{
   ::ShowStatistics(*mp_wnd_text);
}

// ----------------------------------------------------------------------------
LRESULT Wnd_Main::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);
   return msg.Success();
}

void Wnd_Main::PopupTabMenu(int2 position)
{
   PopupMenu menu;
   Append(menu, ID_CONNECTION_DISCONNECT, "Disconnect", Keys::Key_Disconnect);
   Append(menu, ID_CONNECTION_RECONNECT, "Reconnect", Keys::Key_Reconnect);
   menu.AppendSeparator();
   menu.Append(MF_STRING, ID_MUTE, "Show activity on Taskbar");
   menu.Append(MF_STRING, ID_TABCOLOR, "Set color...");
   menu.Append(MF_STRING, ID_TABCOLORDEFAULT, "Reset to default color");
   menu.AppendSeparator();
   menu.Append(MF_STRING, ID_FILE_CLOSETAB, "Close\tControl+F4");

   bool connected=GetConnection().IsConnected();
   bool reconnect=GetConnection().GetServer() && !connected;

   menu.Enable(ID_CONNECTION_DISCONNECT, connected);
   menu.Enable(ID_CONNECTION_RECONNECT, reconnect);
   menu.Check(ID_MUTE, ShowActivityOnTaskbar());

   TrackPopupMenu(menu, g_ppropGlobal->fTaskbarOnTop() ? 0 : TPM_BOTTOMALIGN, position, *this, nullptr);
}

void Wnd_Main::InitMenu(Menu &menu)
{
   auto &settings=GetWindowSettings();
   bool fConnected=mp_connection->IsConnected();
//   bool fLocalEcho=mp_input_active->GetProps().fLocalEcho(); TODO: Delete
//   bool fPaused=mp_wnd_text->IsPaused();
   bool fLogging=mp_connection->IsLogging();

//   CheckItem(hmenu, ID_EDIT_PAUSE, fPaused);
   menu.Enable(ID_EDIT_SMARTPASTE, fConnected);
//   menu.Enable(ID_CONNECTION_DISCONNECT, fConnected);
//   menu.Enable(ID_CONNECTION_RECONNECT, !fConnected);
   menu.Enable(ID_LOGGING_START, !fLogging);
   menu.Enable(ID_LOGGING_FROMBEGINNING, !fLogging);
   menu.Enable(ID_LOGGING_FROMWINDOW, !fLogging);
   menu.Enable(ID_LOGGING_STOP,   fLogging);
//   CheckItem(hmenu, ID_OPTIONS_LOCALECHO, fLocalEcho); TODO: Delete
   menu.Check(ID_OPTIONS_INPUT_HISTORY, settings.fHistory());
   menu.Check(ID_OPTIONS_IMAGEWINDOW, mp_wnd_image);
   menu.Check(ID_OPTIONS_CHARNOTESWINDOW, mp_character_notes_pane);
   menu.Check(ID_OPTIONS_MAPWINDOW, mp_wnd_map);
   menu.Check(ID_OPTIONS_SHOWHIDDENCAPTIONS, Wnd_Docking::s_show_hidden_captions);
}

LRESULT Wnd_Main::On(const Msg::SetText &msg)
{
   m_title=msg.GetText();
   UpdateTitle();
   return msg.Success();
}

void Wnd_Main::UpdateTitle()
{
   FixedStringBuilder<256> sBuffer(m_title_prefix, m_title);
   __super::WndProc(Msg::SetText(UTF16(sBuffer).stringz()));

   GetMDI().OnWindowChanged(*this);
}

LRESULT Wnd_Main::On(const Msg::Size &msg)
{
   // When we're minimized, don't bother
   if(msg.uState()!=SIZE_MINIMIZED)
      mp_layout->Update();
   return __super::WndProc(msg);
}

void Wnd_Main::DockingChange()
{
   if(m_suspend_layout)
      return;

   mp_layout->Update();
}

void Wnd_Main::AddVariable(ConstString name, ConstString value)
{
   m_variables.Insert(m_variables.LowerBound(name), name, value);
}

static Wnd_Main *FindExistingWindow(Prop::Server *ppropServer, Prop::Character *ppropCharacter, Prop::Puppet *ppropPuppet)
{
   for(auto &mainWindow : Wnd_MDI::s_root_node)
   {
      for(auto &window : mainWindow.GetRootWindow())
      {
         Connection &connection=window.GetConnection();
         if(connection.GetServer()==ppropServer &&
            connection.GetCharacter()==ppropCharacter &&
            connection.GetPuppet()==ppropPuppet)
         {
            // Only reuse this window if it's disconnected or we have a character (puppets also have characters)
            if(ppropCharacter || !connection.IsConnected())
               return &window;
         }
      }
   }
   return nullptr;
}

DLNode<Wnd_MDI> Wnd_MDI::s_root_node;

Wnd_MDI::Wnd_MDI(Prop::Position *p_position)
 : DLNode<Wnd_MDI>(s_root_node.Prev())
{
   Create(gd_pcTitle, WS_OVERLAPPEDWINDOW, Window::Position_Default, nullptr);

   mp_wnd_taskbar=new Wnd_Taskbar(*this);

   Rect rcPosition=p_position ? p_position->rcPosition() : g_ppropGlobal->propWindows().rcMDIPosition();
   if(rcPosition.right>0 && rcPosition.bottom>0)
   {
      WindowPlacement wp;
      wp.flags=0;
      wp.showCmd=p_position ? StateToShowCmd(p_position->eState()) : SW_HIDE;
      wp.MinPosition()=int2();
      wp.MaxPosition()=int2();
      wp.NormalPosition()=rcPosition;
      wp.NormalPosition().pt2+=wp.NormalPosition().pt1;
      wp.Set(*this);
   }

   if(p_position)
   {
      for(auto &p_tab : p_position->propTabs())
      {
         // See if we have a server
         auto ppropServer=g_ppropGlobal->propConnections().propServers().FindByName(p_tab->pclServer());
         if(!ppropServer)
            continue;

         auto ppropCharacter=ppropServer->propCharacters().FindByName(p_tab->pclCharacter());
         if(!ppropCharacter)
            continue;

         auto ppropPuppet=ppropCharacter->propPuppets().FindByName(p_tab->pclPuppet());
         // If there's an existing window, don't do it
         if(FindExistingWindow(ppropServer, ppropCharacter, ppropPuppet))
            continue;

         new Wnd_Main(*this, ppropServer, ppropCharacter, ppropPuppet, true);
      }

      int activeTab=p_position->iActiveTab();
      if(IsBetween(activeTab, 0, int(m_window_count-1)))
         SetActiveWindow(GetWindow(activeTab));
   }

   // If we didn't restore any windows, create a window
   if(GetWindowCount()==0)
      new Wnd_Main(*this);
}

LRESULT Wnd_MDI::On(const Msg::Create &msg)
{
   if(Windows::g_dark_mode)
      SetImmersiveDarkMode(true);
   return msg.Success();
}

Wnd_MDI::~Wnd_MDI()
{
   // If any main windows exist, destroy them
   while(m_root_wnd_main.Linked())
      m_root_wnd_main.Prev()->Destroy();

   // If we're the last window to close, we quit
   if(IsLast())
   {
#if 0
      if(Wnd_MuckNet::HasInstance())
         delete &Wnd_MuckNet::GetInstance();
      if(MuckNet::HasInstance())
         delete &MuckNet::GetInstance();
#endif

      CloseDialog_Triggers();
      CloseDialog_Aliases();
      CloseDialog_KeyboardMacros();

      if(Scripter::HasInstance())
         delete &Scripter::GetInstance();
      ConsoleDelete();

      if(IsStoreApp())
         SetBadgeNumber(0);

      PostQuitMessage(0);
   }
}

LRESULT Wnd_MDI::WndProc(const Message &msg)
{
   return Dispatch<WindowImpl, Msg::QueryEndSession, Msg::EndSession, Msg::Create, Msg::Close, Msg::Activate, Msg::Size, Msg::ExitSizeMove, Msg::Command, Msg::SysCommand, Msg::SetFocus>(msg);
}

void Wnd_MDI::AddWindow(Wnd_Main &window)
{
   window.DLNode<Wnd_Main>::Link(m_root_wnd_main.Prev());
   m_window_count++;

   mp_wnd_taskbar->Refresh();

   window.SetPosition(m_rect_client);
   SetActiveWindow(window, true);
}

void Wnd_MDI::DeleteWindow(Wnd_Main &window)
{
   if(&window==mp_active_wnd_main)
      mp_active_wnd_main=nullptr;

   window.DLNode<Wnd_Main>::Unlink();
   m_window_count--;

   mp_wnd_taskbar->Refresh();

   if(!m_root_wnd_main.Linked())
   {
      if(!m_in_WM_CLOSE) // In case we're already mid-close
         Close(); 
   }
   else
      SetActiveWindow(*m_root_wnd_main.Next(), true);
}

void Wnd_MDI::OnWindowChanged(Wnd_Main &window)
{
   // Update our window title if this is the active tab
   if(mp_active_wnd_main==&window)
   {
      FixedStringBuilder<256> title(window.GetText());
      if(title.Length())
         title(" - ");
      title(gd_pcTitle);
      SetText(title);
   }
   mp_wnd_taskbar->Refresh(window);
}

void Wnd_MDI::WindowFlash(Wnd_Main &window, bool flash)
{
   mp_wnd_taskbar->Refresh(window);

   // If we're not showing activity on the taskbar for this window, don't do a flash
   if(flash && !window.ShowActivityOnTaskbar())
      return;

   // If we're a store app with badges enabled, don't flash
   if(IsStoreApp() && g_ppropGlobal->fTaskbarBadge())
      return;

   // If we're already the foreground window, don't flash
   if(GetForegroundWindow()==*this)
      return;

   Flash(flash);
}

void Wnd_MDI::RefreshTaskbar(Wnd_Main &window)
{
   mp_wnd_taskbar->Refresh(window);
}

unsigned Wnd_MDI::s_badge_number{};

void Wnd_MDI::RefreshBadgeCount()
{
   if(!IsStoreApp())
      return;

   unsigned count=0;
   if(g_ppropGlobal->fTaskbarBadge()) // If no badge, leave count at 0 (so turning off badges hides the badge)
   {
      for(auto &connection : Connection::s_root_node)
      {
         auto &wnd=connection.GetMainWindow();
         if(wnd.ShowActivityOnTaskbar())
            count+=wnd.HasActivity();
      }
   }

   if(count!=s_badge_number)
   {
      s_badge_number=count;
      SetBadgeNumber(count);
   }
}

Wnd_Main &Wnd_MDI::GetWindow(unsigned index) noexcept
{
   Assert(index<m_window_count);
   return m_root_wnd_main[index];
}

int Wnd_MDI::GetActiveWindowIndex() const
{
   int index=0;
   for(auto &window : m_root_wnd_main)
   {
      if(&window==mp_active_wnd_main)
         return index;
      index++;
   }

   return 0; // We can hit this if the multiple close messages get posted before the window gets activated
}

void Wnd_MDI::SetActiveWindow(Wnd_Main &window, bool fActive)
{
   if(!fActive || mp_active_wnd_main==&window) return;

   Wnd_Main *p_old=mp_active_wnd_main;

   mp_active_wnd_main=&window;
   mp_active_wnd_main->Show(SW_SHOW);
   mp_active_wnd_main->SetActive(true);

   if(p_old)
   {
      p_old->SetActive(false);
      p_old->Show(SW_HIDE);
   }
   mp_wnd_taskbar->SetActiveWindow(window, fActive);
   OnWindowChanged(window);
   WindowFlash(window, false);

   if(IsMinimized())
      Show(SW_RESTORE);
   if(!IsVisible())
      Show(SW_SHOW);
}

LRESULT Wnd_MDI::On(const Msg::Close &msg)
{
   // Must scope the restorer to go out of scope before the Destroy() call!
   {
      RestorerOf _{m_in_WM_CLOSE}; m_in_WM_CLOSE=true;

      // Save the config if we're the last window
      if(IsLast())
      {
         if(g_ppropGlobal->fSaveOnExit())
            SaveConfig(*this);
      }

      // Kindly close all child windows
      while(m_root_wnd_main.Linked())
      {
         Wnd_Main *p=m_root_wnd_main.Next();
         if(Msg::Close().Send(*p)==Msg::Close::Failure())
         {
            SetActiveWindow(*p);
            return msg.Failure();
         }
      }
   }

   Destroy();
   return msg.Success();
}

LRESULT Wnd_MDI::On(const Msg::QueryEndSession &msg)
{
   if(s_root_node.Next()==this && msg.lParam()==ENDSESSION_CLOSEAPP)
      RegisterApplicationRestart(L"", RESTART_NO_CRASH|RESTART_NO_HANG|RESTART_NO_REBOOT);

   return TRUE;
}

LRESULT Wnd_MDI::On(const Msg::EndSession &msg)
{
   if(!msg.IsEnding()) // False alarm?
   {
      if(s_root_node.Next()==this)
         UnregisterApplicationRestart();

      return msg.Success();
   }

   // Only the first window should save the config
   if(s_root_node.Next()==this)
      SaveConfig(*this, false);

   ExitProcess(0); // Everything is shutting down so terminate immediately
}

void Wnd_MDI::PopupMainMenu(int2 position)
{
   PopupMenu menu;

   {
      PopupMenu m;
      Append(m, ID_FILE_NEWTAB, "New &Tab", Wnd_Main::Key_NewTab);
      Append(m, ID_FILE_NEWWINDOW, "New &Window", Wnd_Main::Key_NewWindow);
      m.Append(MF_STRING, ID_FILE_NEWINPUT, "New Input Window");
      m.Append(MF_STRING, ID_FILE_NEWEDIT, "New Edit Window");
      m.AppendSeparator();
      m.Append(MF_STRING, ID_OPTIONS_INPUT_HISTORY, "Toggle Input &History Window");
      m.Append(MF_STRING, ID_OPTIONS_IMAGEWINDOW, "Toggle Image Window");
      m.Append(MF_STRING, ID_OPTIONS_MAPWINDOW, "Toggle Map Window");
      m.Append(MF_STRING, ID_OPTIONS_CHARNOTESWINDOW, "Toggle Character Notes Window");
      m.AppendSeparator();
	   m.Append(MF_STRING, ID_EDIT_COPYDOCKING, "Copy all window settings");
	   m.Append(MF_STRING, ID_EDIT_PASTEDOCKING, "Paste all window settings");
      m.AppendSeparator();
      m.Append(MF_STRING, ID_OPTIONS_SHOWHIDDENCAPTIONS, "Show Hidden Captions");

      menu.Append(std::move(m), "&Windows");
   }
   {
      PopupMenu m;

      m.Append(MF_STRING, ID_OPTIONS_TRIGGERS, "&Triggers...");
      m.Append(MF_STRING, ID_OPTIONS_MACROS, "&Macros...");
      m.Append(MF_STRING, ID_OPTIONS_ALIASES, "&Aliases...");
      m.AppendSeparator();
      m.Append(MF_STRING, ID_HELP_DEBUG_TRIGGERS, "Trigger Debugger");
      m.Append(MF_STRING, ID_HELP_DEBUG_NETWORK, "Network Debugger");
      m.Append(MF_STRING, ID_EDIT_SMARTPASTE, "&Smart Paste...");

      menu.Append(std::move(m), "&Tools");
   }
   {
      PopupMenu m;
      Append(m, ID_LOGGING_START, "&Start...", Wnd_Main::Key_Logging_Toggle);
      Append(m, ID_LOGGING_STOP, "S&top", Wnd_Main::Key_Logging_Toggle);
      m.Append(MF_STRING, ID_LOGGING_FROMBEGINNING, "Starting From Beginning...");
      m.Append(MF_STRING, ID_LOGGING_FROMWINDOW, "Starting From Top of Window...");
      menu.Append(std::move(m), "&Logging");
   }
   menu.Append(MF_STRING, ID_OPTIONS_PREFERENCES, "&Settings...");

   {
      PopupMenu m;
      m.Append(MF_STRING, ID_HELP_CONTENTS, "&Contents...");
   	m.Append(MF_STRING, ID_HELP_CHANGES, "Changes...");
      m.AppendSeparator();
      m.Append(MF_STRING, ID_HELP_ABOUT, "&About...");
      menu.Append(std::move(m), "&Help");
   }

   menu.AppendSeparator();
   Append(menu, ID_FILE_QUIT, "Close all Windows and E&xit", Wnd_Main::Key_Window_CloseAll);

   if(mp_active_wnd_main)
      mp_active_wnd_main->InitMenu(menu);

   TrackPopupMenu(menu, g_ppropGlobal->fTaskbarOnTop() ? 0 : TPM_BOTTOMALIGN, position, *this, nullptr);
}

LRESULT Wnd_MDI::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case ID_FILE_NEWTAB:
         new Wnd_Main(*this);
         return msg.Success();

      case ID_FILE_NEWWINDOW:
      {
         Wnd_MDI &wndMDI=*new Wnd_MDI();
         wndMDI.Show(SW_SHOW);
         return msg.Success();
      }

      case ID_FILE_QUIT:
      {
         bool fConnected=false;
         if(g_ppropGlobal->propWindows().fAskBeforeDisconnecting())
         {
            for(auto &connection : Connection::s_root_node)
               if(connection.IsConnected())
               {
                  fConnected=true;
                  break;
               }
         }

         if(fConnected)
         {
            if(MessageBox(hWnd(), STR_StillConnected, STR_Note, MB_ICONQUESTION|MB_YESNO)==IDNO)
               return msg.Success();
         }

         SaveConfig(*this);

         while(s_root_node.Linked())
            s_root_node.Prev()->Destroy();

         return msg.Success();
      }

      case ID_FILE_CLOSEWINDOW:
         On(Msg::Close());
         return msg.Success();

      case ID_OPTIONS_SHOWHIDDENCAPTIONS:
         Wnd_Docking::s_show_hidden_captions^=true;
         for(auto &mdi_window : s_root_node)
            for(auto &main_window : mdi_window.m_root_wnd_main)
               for(auto &frame : main_window.GetFrames())
               {
                  for(auto &window : frame.GetWindows())
                     window.FrameChanged();
               }
         return msg.Success();

//      case ID_HELP_SUBSCRIBE: CreateWindow_Subscribe(*this); return msg.Success();
      case ID_HELP_ABOUT: CreateWindow_About(*this); return msg.Success();
   }

   Assert(mp_active_wnd_main);
   if(mp_active_wnd_main)
      return mp_active_wnd_main->On(msg);
   return msg.Success();
}

LRESULT Wnd_MDI::On(const Msg::SysCommand &msg)
{
   if(msg.Command()==SC_KEYMENU)
   {
      mp_wnd_taskbar->OpenMenu();
      return msg.Success();
   }
   return __super::WndProc(msg);
}

LRESULT Wnd_MDI::On(const Msg::Size &msg)
{
   // When we're minimized, don't bother
   if(msg.uState()==SIZE_MINIMIZED) return msg.Success();

   m_rect_client=ClientRect();
   mp_wnd_taskbar->OnSize(m_rect_client);

   for(auto &window : m_root_wnd_main)
      window.SetPosition(m_rect_client, SWP_NOCOPYBITS); // NoCopyBits so that if the taskbar moved we don't get a redraw glitch

   return msg.Success();
}

LRESULT Wnd_MDI::On(const Msg::ExitSizeMove &msg)
{
   for(auto &window : m_root_wnd_main)
   {
      if(window.GetConnection().GetTelnet().m_do_naws)
         window.SendNAWS();
   }
   return msg.Success();
}

LRESULT Wnd_MDI::On(const Msg::Activate &msg)
{
   if(mp_active_wnd_main)
      msg.Send(*mp_active_wnd_main);

   if(msg.uState()==WA_INACTIVE)
      mp_wnd_taskbar->DrawTabNumbers(false);

   return msg.Success();
}

LRESULT Wnd_MDI::On(const Msg::SetFocus &msg)
{
   if(mp_active_wnd_main)
      mp_active_wnd_main->SetFocus();
   return msg.Success();
}

void Wnd_MDI::OnPropChange()
{
   On(Msg::Size(0, 0, 0)); // To move the taskbar if it changed
   Invalidate(true);
}

ATOM Wnd_Main::Register()
{
   WndClass wc(L"MainWnd");
   wc.hCursor=LoadCursor(nullptr, IDC_ARROW);

   // We don't want any specks of background showing up, so make them noticeable in DEBUG
   #ifdef _DEBUG
   wc.hbrBackground=(HBRUSH)CreateSolidBrush(Colors::Green);
   #endif

   return wc.Register();
}

ATOM Wnd_MDI::Register()
{
   WndClass wc(L"MainFrame");

   wc.LoadIcon(IDI_APP, g_hInst);
   wc.hCursor = LoadCursor(nullptr, IDC_ARROW); 
//   wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
   return wc.Register();
}

void Wnd_MDI::Connect(Prop::Server *ppropServer, Prop::Character *ppropCharacter, Prop::Puppet *ppropPuppet, bool fSetActiveWindow)
{
   if(ppropServer && !ppropCharacter)
   {
      auto &characters=ppropServer->propCharacters();
      FixedStringBuilder<256> name;
      unsigned duplicate=0;
      while(true)
      {
         name.Clear();
         name("Unnamed");
         if(duplicate>0)
            name(' ', duplicate);
         duplicate++;

         ppropCharacter=characters.FindByName(name);
         if(!ppropCharacter)
         {
            ppropCharacter=&characters.New();
            ppropCharacter->Rename(characters, name);
            break;
         }

         if(!FindExistingWindow(ppropServer, ppropCharacter, ppropPuppet))
            break;
      }
   }

   // Look for an existing window
   if(auto *pWndMain=FindExistingWindow(ppropServer, ppropCharacter, ppropPuppet))
   {
      Connection &connection=pWndMain->GetConnection();
      connection.Reconnect();
      if(fSetActiveWindow)
         pWndMain->GetMDI().SetActiveWindow(*pWndMain);
      return;
   }

   // Look for an empty window
   for(auto &window : m_root_wnd_main)
   {
      Connection &connection=window.GetConnection();
      if(!connection.IsConnected() && !connection.GetServer())
      {
         connection.Associate(ppropServer, ppropCharacter, ppropPuppet);
         window.ResetWindowSettings();
         connection.Connect(false);
         if(fSetActiveWindow)
            SetActiveWindow(window);
         return;
      }
   }

   Wnd_Main &window=*new Wnd_Main(*this, ppropServer, ppropCharacter, ppropPuppet);
   if(fSetActiveWindow)
      SetActiveWindow(window);
}

void Code_Test()
{
#if 1
   ConstString code{R"code(
Test() int
{
   DebugOutput(AddFive(3));

   string a="a string";
   string b="b string";
   DebugOutput(a+b);
   {
      string c="c string";
      string d="d string";
   }
   {
      string e="e string";
      {
         string f="f string";
         string g="g string";
      }
      string h="h string";
   }
   string i="i string";
   string j="j string";

   return 1;
}

)code"};
#else

   /*
   DebugOutput("Calling AddFive(3)=" + string(AddFive(3)));

   DebugOutput("Abs(-5)="+string(Abs(-5)));

   DebugOutput("Test of " + "concatenating strings");
   DebugOutput("2+(3*4)="+string(2+(3*4)));

   int i=0;
   while(++i<100)
   ;

   DebugOutput("While Loop completed with i=" + string(i) + " (should be 100)");

   i/=0;

   {
   int nested=0;

   for(int x=0;x<9;x++)
   for(int y=0;y<9;y++)
   for(int z=0;z<9;z++)
   nested++;

   DebugOutput("Nested for loops, total should be 9*9*9 aka 729=="+string(nested));
   }
   return 5;
   */

   ConstString code{R"code(
Test() int
{
   int i=0;
   while(++i<100)
      ;

   OutputDebug("While Loop completed with i=" + string(i) + " (should be 100)");

   OutputDebug("Testing Scripting Engine");
   OutputDebug("Command line:" + commandline);

   OutputDebug("1+2+3=" + string(1+2+3) + " (should be 6)");
   OutputDebug("2*3*4=" + string(2*3*4) + " (should be 24)");
   OutputDebug("2+(3*4)=" + string(2+(3*4)) + " (should be 14)");
   OutputDebug("FFFF hex = " + string("FFFF".HexToInt()));

   {
      int nested=0;

      for(int x=0;x<9;x++)
         for(int y=0;y<9;y++)
            for(int z=0;z<9;z++)
               nested++;

      OutputDebug("Nested for loops, total should be 9*9*9 aka 729=="+string(nested));
   }

   for(int z=0;++z<10;)
      OutputDebug(("Random value between -100,100:" + string(GetRandom(-100, 100)) + " Z:" + string(z)));
/*
   {
      string value="Alpha=Beta=Gamma=Delta=Epsilon=Success";
      string before;
      string after;

      while(value.Split('=', before, after))
      {
         OutputDebug(before);
         value=after;
      }
      OutputDebug(after);
   }
*/
}

)code"};
#endif
#if SCRIPTING

   auto p_c_script=CScript::Create();

   try
   {
#if 1
      auto function_1=p_c_script->Compile(R"code(AddFive(string v) string { return v+" "+string(5); })code");
      p_c_script->Disassemble(function_1);

      auto function=p_c_script->Compile(code);

      p_c_script->Disassemble(function);

      p_c_script->Run(function);
#endif
   }
   catch(const CScript::Exception_Compiler &e)
   {
      ConsoleText(FixedStringBuilder<256>("Compile error: ", e, "  On Line:", e.m_line_number));
   }
   catch(const Exceptions::Message &e)
   {
      ConsoleText(e);
   }
#endif
}

void CreateWindow_Root(ConstString command_line, int nCmdShow)
{
#if SCRIPTING
   Code_Test();
#endif

   if(!GlobalEvents::HasInstance())
      new GlobalEvents();

   if(g_ppropGlobal->fSpellCheck())
      Speller::SetLanguage(g_ppropGlobal->pclSpellLanguage());

   auto &prop_positions=g_ppropGlobal->propWindows().propPositions();
   for(auto &p_position : prop_positions)
   {
      if(!p_position->propTabs())
         continue;

      Wnd_MDI &wnd_MDI=*new Wnd_MDI(p_position);
      wnd_MDI.EnsureOnScreen();
      wnd_MDI.Show(SW_SHOW);
   }

   // If there were no positions or they each had no tabs we must create at least one window
   if(!Wnd_MDI::s_root_node.Linked())
   {
      Wnd_MDI &wnd_MDI=*new Wnd_MDI();
      wnd_MDI.Show(SW_SHOW);
   }

   // If there was a command line, create a tab and run it
   if(command_line)
   {
      Wnd_Main &wnd_main=*new Wnd_Main(Wnd_MDI::GetInstance());
      wnd_main.ParseCommandLine(command_line);
   }

   // Run startup script
   if(ConstString script_path=g_ppropGlobal->pclScriptStartup())
   {
      File::Path script_path_fixed;
      if(File::IsPathRelative(script_path))
      {
         script_path_fixed(GetConfigPath(), script_path);
         script_path=script_path_fixed;
      }

      if(auto *pScripter=Wnd_MDI::GetInstance().GetActiveWindow().GetScripter();pScripter && pScripter->RunFile(script_path)==false)
         ConsoleHTML(STR_CantLoadStartupScript);
   }

   for(auto &p_server : g_ppropGlobal->propConnections().propServers())
      for(auto &p_character : p_server->propCharacters())
      {
         if(p_character->fConnectAtStartup())
            Wnd_MDI::GetInstance().Connect(p_server, p_character, nullptr, false);
      }

   if(g_ppropGlobal->fUpgraded() || g_ppropGlobal->fShowWelcome() || BETA_BUILD!=0)
      CreateWindow_About(Wnd_MDI::GetInstance());

#if BETA_BUILD!=0
   MessageBox(*Wnd_MDI::s_root_node.Next(), "This is a beta build, expect things to not be perfect.\nAnd as always, please try to break it!", "BETA reminder", MB_ICONEXCLAMATION|MB_OK);
#endif
#if DWRITE_TEST
   D2DTest();
#endif
}
