#include "Main.h"
#include "Wnd_Main.h"

Prop::Global *g_ppropGlobal{};

static bool g_fUseStartupPathForConfig{};
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

         Rect &rcNormal=wp.NormalPosition();
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
      Rect &rcNormal=wp.NormalPosition();
      rcNormal.pt2-=rcNormal.pt1; // We save this rectangle as (point, size) vs (point1, point2)
      g_ppropGlobal->propWindows().rcMDIPosition(rcNormal);
   }

   File::Path filenameNew{GetConfigPath(), "Config.new"};
   File::Path filename{GetConfigPath(), "Config.txt"};

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
