//
// Wnd_Main
//
#include "Logging.h"
#include "Connection.h"
#include "Wnd_Input.h"

namespace OM
{
class MainWindow;
};

struct Scripter;
struct Wnd_HistoryGraph;
struct Wnd_Taskbar;
struct Wnd_Image;
namespace Maps { struct Wnd; }
namespace Stats { struct Wnd; }
struct TileMaps;
struct IYarn_TileMap;
struct SpawnTabsWindow;
struct Wnd_WebView;

enum CommandIDs : int
{
   ID_FILE_NEWWINDOW = 1000,
   ID_FILE_NEWEDIT,
   ID_FILE_NEWTAB,
   ID_FILE_NEWINPUT,
   ID_FILE_CLOSETAB,
   ID_FILE_CLOSEWINDOW,
   ID_FILE_QUIT,
   ID_EDIT_PASTE,
   ID_EDIT_FIND,
   ID_EDIT_SELECTALL,
   ID_EDIT_COPYDOCKING,
   ID_EDIT_PASTEDOCKING,
   ID_EDIT_SMARTPASTE,
   ID_EDIT_FINDINPUTHISTORY,
   ID_HELP_ABOUT,
   ID_HELP_CONTENTS,
   ID_HELP_CHANGES,
   ID_HELP_DEBUG_NETWORK,
   ID_HELP_DEBUG_TRIGGERS,
   ID_OPTIONS_PREFERENCES,
   ID_OPTIONS_MACROS,
   ID_OPTIONS_ALIASES,
   ID_OPTIONS_TRIGGERS,
   ID_OPTIONS_INPUT_HISTORY,
   ID_OPTIONS_CHARNOTESWINDOW,
   ID_OPTIONS_IMAGEWINDOW,
   ID_OPTIONS_MAPWINDOW,
   ID_OPTIONS_SHOWHIDDENCAPTIONS,
   ID_CONNECTION_CONNECT,
   ID_CONNECTION_DISCONNECT,
   ID_CONNECTION_RECONNECT,
   ID_LOGGING_START,
   ID_LOGGING_STOP,
   ID_LOGGING,
   ID_LOGGING_FROMBEGINNING,
   ID_LOGGING_FROMWINDOW,
   ID_STATIC_ABOUTINFO,
   ID_TABCOLOR,
   ID_TABCOLORDEFAULT,
   ID_MUTE_ACTIVITY,
   ID_MUTE_AUDIO,
};

struct ClientLayoutHelper : AL::Object
{
   ClientLayoutHelper(Wnd_Main &main, Object &child) : m_main{main}, mp_object{&child} { }

   const int2 &GetMinSize() const noexcept { return mp_object->GetMinSize(); }
   const int2 &CalcMinSize() noexcept { return mp_object->CalcMinSize(); }
   void Layout(DeferredWindowPos &wp, const Rect &rc);

private:
   Wnd_Main &m_main;
   UniquePtr<Object> mp_object;
};

struct SpawnWindow
 : DLNode<SpawnWindow>,
   SuperImpl,
   Text::IHost,
   Events::Sends_Deleted,
   Events::ReceiversOf<SpawnWindow, GlobalTextSettingsModified>
{
   SpawnWindow(Wnd_Main &wnd_main); // Null Spawn
   SpawnWindow(SpawnWindow *p_insert_after, Wnd_Main &wnd_main, ConstString title, Prop::TextWindow &props, SpawnTabsWindow *p_spawn_tabs_window=nullptr);
   ~SpawnWindow();

   operator bool() const { return mp_docking; }

   bool On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Text::Records::URLData &url, const Message &msg) override;
   void On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Msg::RButtonDown &msg, const Text::List::LocationInfo &location_info) override;
   void On(const GlobalTextSettingsModified &event);

   void ShowTab();
   void AfterRestore();

   LRESULT WndProc(const Message &msg) override;
   LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
   LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }

   Wnd_Docking &GetDocking() const { return *mp_docking; }

   OwnedString m_title;
   Wnd_Main &m_wnd_main;
   Text::Wnd *mp_text{};
   CntPtrTo<Prop::TextWindow> mp_prop_text_window;
   Wnd_Docking *mp_docking{};
   UniquePtr<AL::ChildWindow> mp_child_window;

   SpawnTabsWindow *mp_spawn_tabs_window{}; // If we're in a spawn tab, this is set

   CntPtrTo<Prop::Trigger> mp_capture_until;
   bool m_gag_log{}; // Updated each time a spawn trigger hits

   using DLNode<SpawnWindow>::Link, DLNode<SpawnWindow>::Unlink, DLNode<SpawnWindow>::Prev;
};

struct SpawnTabsWindow
 : DLNode<SpawnTabsWindow>,
   Wnd_Dialog,
   Windows::AutoLayout::Tab::INotify
{
   SpawnTabsWindow(SpawnTabsWindow *p_insert_after, Wnd_Main &wnd_main, ConstString title);
   ~SpawnTabsWindow();

   void SetAway(bool fAway); // Call SetAway on all text windows
   unsigned GetUnreadCount() const;
   void AfterRestore();

   SpawnWindow &GetTab(ConstString title, Prop::TextWindow *pprops=nullptr, bool hilight=true);
   void SetVisible(SpawnWindow &window);
   bool SetVisible(ConstString title);
   OwnedString m_title;

   Wnd_Docking &GetDocking() const { return *mp_docking; }

   // Windows::AutoLayout::Tab::INotify
   void TabsClosed() override { Close(); }
   void TabChange(unsigned tabOld) override;
   void TabClosed(unsigned tab) override;
   void TabMoved(unsigned from, unsigned to) override;

   DLNodeList<SpawnWindow> &GetTabs() { return m_spawn_windows; }

private:

   friend TWindowImpl;
   LRESULT WndProc(const Message &msg) override;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Close &msg);
   LRESULT On(const Msg::EraseBackground &msg);
   LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
   LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }

   Wnd_Main &m_wnd_main;
   Wnd_Docking *mp_docking;
   AL::Tab *mp_tabs;
   DLNodeList<SpawnWindow> m_spawn_windows;
};

struct Wnd_Main
:  DLNode<Wnd_Main>,
   private IError,
   TWindowImpl<Wnd_Main>,
   IEvent_Prepare,
   Wnd_Container,
   Text::IHost,
   Events::ReceiversOf<Wnd_Main, Connection::Event_Log, Connection::Event_Activity,
      Connection::Event_Connect, Connection::Event_Disconnect, Connection::Event_Send,
      GlobalTextSettingsModified, GlobalInputSettingsModified>
{
   static ATOM Register();

   Wnd_Main(Wnd_MDI &wndMDI);
   Wnd_Main(Wnd_MDI &wndMDI, Prop::Server *ppropServer, Prop::Character *ppropCharacter, Prop::Puppet *ppropPuppet, bool offline=false);

   void Redock(Wnd_MDI &wnd_MDI);

   void ParseCommandLine(ConstString params);
   void PopupTabMenu(int2 position);
   void InitMenu(Menu &menu);

   // IError
   void Error(ConstString string);
   void Text(ConstString string);

   // IEvent_Prepare
   void Event_Prepare() { SetScripterWindow(); }

   // Wnd_Text::IHost
   bool On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Text::Records::URLData &url, const Message &msg) override;
   void On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Msg::RButtonDown &msg, const Text::List::LocationInfo &location_info) override;

   void On(Text::Wnd &wndText, Text::Wnd_View &wndView, const Msg::RButtonDown &msg, const Text::List::LocationInfo &location_info, CntPtrTo<Prop::TextWindow> &p_props);

   // Wnd_Docking
   void DockingChange() override;

   void Save();
   void SaveDockingConfiguration(Prop::Docking &propDocking);
   void RestoreDockingConfiguration(Prop::Docking &propDocking);

   Wnd_Docking *RestoreDockedWindowSettings(Prop::DockedWindow &propWindow);

   void SaveMainWindowSettings(Prop::MainWindowSettings &settings);
   void ApplyMainWindowSettings();
   void ApplyHistoryVisibility(); // Note: Called in ApplyMainWindowSettings

   Connection &GetConnection() noexcept { return *mp_connection; }
   void DelaySend(ConstString string, float seconds) { new DelayTimer(*this, string, seconds); }
   void SetScripterWindow();
   Scripter *GetScripter();

   InputControl &GetInputWindow() noexcept { return m_input; }
   InputControl &GetActiveInputWindow() { return *mp_input_active; }
   void CheckInputHeight();
   Text::Wnd &GetOutputWindow() { return *mp_wnd_text; }
   Prop::TextWindow &GetOutputProps() { return *mp_prop_output; }
   Wnd_InputPane* CreateInputWindow(Prop::InputWindow &props); // Simply create the window and return it, no docking
   Wnd_InputPane* AddInputWindow(ConstString prefix, bool fUnique);
   void RemoveInputPane(Wnd_InputPane &pane);
   void SetActiveInputWindow(InputControl &input) { mp_input_active=&input; }
   void ActivateNextInputWindow();

   Wnd_EditPane& CreateEditPane(ConstString title, bool fDockable, bool fSpellCheck);
   Wnd_EditPane& GetEditPane(ConstString title, bool fDockable, bool fSpellCheck); // Gets existing pane with same title, otherwise creates new one
   void RemoveEditPane(Wnd_EditPane &pane);

   void ShowCharacterNotesPane();
   void UpdateCharacterIdleTimer();

   void TabColor(Color color);

   SpawnWindow &GetSpawnWindow(const Prop::Trigger_Spawn &trigger, ConstString title, bool fHilight);
   Wnd_Image *GetImageWindow() { return mp_wnd_image; }
   Wnd_Image &EnsureImageWindow();
   Stats::Wnd &GetStatsWindow(ConstString title, bool fDock=true);

   Maps::Wnd *GetMapWindow() { return mp_wnd_map; }
   Maps::Wnd &EnsureMapWindow();

   TileMaps &EnsureTileMaps();

   IYarn_TileMap *GetYarn_TileMap() { return mp_yarn_tilemap; }
   IYarn_TileMap &EnsureYarn_TileMap();

   OM::MainWindow *GetDispatch();
   Wnd_MDI &GetMDI() noexcept { return *mp_wnd_MDI; }

   Wnd_WebView *FindWebView(ConstString id);
   Wnd_WebView* GetWebViewRoot() { return m_webview_windows.begin(); }

   bool IsActive() const noexcept { return m_active; }
   bool Flashed() const noexcept { return m_flash_state; }
   bool HasActivity() const noexcept { return m_flash_state || m_timer_tab_flash; }
   void SetActive(bool fActive); // Used by the Wnd_MDI
   ConstString GetTitlePrefix() const { return m_title_prefix; }
   unsigned GetUnreadCount() const;
   bool ShowActivityOnTaskbar();
   void ToggleShowActivityOnTaskbar();

   unsigned GetImportantActivityCount() const { return m_important_activity_count; }
   void AddImportantActivity();

   Prop::MainWindowSettings &GetWindowSettings() { return *mp_prop_main_window_settings; }
   void ResetWindowSettings();
   void ApplyInputProperties();
   void BroadcastHTML(ConstString message); // Adds the HTML formatted message to the output window + all spawns
   void OnReplayComplete();

   void SendLine(ConstString string, ConstString prefix=ConstString());
   void SendLines(ConstString string, ConstString prefix=ConstString());
   void SendNAWS();

   void ParseCommand(ConstString string);
   Collection<Variable> &GetVariables() { return m_variables; }

   void On_TileMap(ConstString command, ConstString json);
   void ShowStatistics();
   void History_AddToHistory(ConstString string, ConstString prefix, uint64 time=0);

   bool EditChar(InputControl &edit, char c);
   bool EditKey(InputControl &edit, const Msg::Key &msg);

   enum Keys
   {
      Key_Minimize,
      Key_Hide,
      Key_ClearActivity,

      Key_Input_Send,
      Key_Input_RepeatLastLine,
      Key_Input_LineUp,
      Key_Input_LineDown,
      Key_Input_Clear,
      Key_Input_NextInput,
      Key_Input_PushToHistory,
      Key_Input_Autocomplete,
      Key_Input_AutocompleteWholeLine,

      Key_Output_PageUp,
      Key_Output_PageDown,
      Key_Output_LineUp,
      Key_Output_LineDown,
      Key_Output_Top,
      Key_Output_Bottom,

      Key_History_PageUp,
      Key_History_PageDown,
      Key_History_SelectUp,
      Key_History_SelectDown,
      Key_History_Toggle,

      Key_Imaging_Toggle,

      Key_Window_Next,
      Key_Window_Prev,
      Key_Window_Close,
      Key_Window_CloseAll,

      Key_NewTab,
      Key_NewWindow,

      Key_Edit_Find,
      Key_Edit_FindHistory,
      Key_Edit_SelectAll,
      Key_Edit_Paste,
      Key_Edit_Pause,
      Key_Edit_SmartPaste,
      Key_Edit_ConvertReturns,
      Key_Edit_ConvertTabs,
      Key_Edit_ConvertSpaces,

      Key_Connect,
      Key_Disconnect,
      Key_Reconnect,

      Key_Logging_Toggle,
      Key_SendTelnet_IP,
      Key_Max
   };

   //
   // Events
   //
   struct Event_Command : Events::Stoppable
   {
      Event_Command(ConstString string, ConstString lstrParam) : m_lstr(string), m_lstrParam(lstrParam) { }

      ConstString GetCommand() const { return m_lstr; }
      ConstString GetParams() const { return m_lstrParam; }

   private:
      ConstString m_lstr;
      ConstString m_lstrParam;
   };

   struct Event_Activate
   {
      Event_Activate(bool fActivated) : m_fActivated(fActivated) { }

      bool fActivated() const { return m_fActivated; }

   private:
      bool m_fActivated;
   };

   struct Event_Close { };

   struct Event_Key : Events::Stoppable
   {
      Event_Key(int iVKey) : m_iVKey(iVKey) { }

      int GetVKey() const { return m_iVKey; }

   private:
      int m_iVKey;
   };

   struct Event_InputChanged { };

   Events::SendersOf<Event_Command, Event_Activate, Event_Close, Event_Key, Event_InputChanged> m_events;
   template<typename TEvent> operator Events::SenderOf<TEvent> &() { return m_events.Get<TEvent>(); }

   // Events
   void On(const Connection::Event_Log &event);
   void On(const Connection::Event_Connect &event);
   void On(const Connection::Event_Disconnect &event);
   void On(const Connection::Event_Activity &event);
   void On(const Connection::Event_Send &event);
   void On(const GlobalTextSettingsModified &event);
   void On(const GlobalInputSettingsModified &event);

private:
   Wnd_Main(const Wnd_Main &)=delete;
   ~Wnd_Main() noexcept;

   void InitScripter();

   bool ProcessEditKey(InputControl &edInput, const Msg::Key &msg);
   void SendInput(InputControl &edInput); // Sends what's in the input line to the muck
   void History_CheckIfInputModified(InputControl &edInput);
   void History_SelectUp(InputControl &edInput);
   void History_SelectDown(InputControl &edInput);
   void History_SelectLine(InputControl &edInput); // Put the current m_history_pos into the input window and select it in the historyw window
   void Input_Autocomplete();
   void ActivateWindow(Direction::PN dir);
   void HandleKey(InputControl &edInput, Keys key);
   void FlashTab();

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Close &msg);
   LRESULT On(const Msg::Notify &msg);
   LRESULT On(const Msg::Char &msg);
   LRESULT On(const Msg::Paint &msg);
public:
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Size &msg);
private:
   LRESULT On(const Msg::Activate &msg);
   LRESULT On(const Msg::SetFocus &msg);
   LRESULT On(const Msg::SetText &msg);
   LRESULT On(const Msg::EraseBackground &msg) { return msg.Success(); }
   LRESULT On(const Msg::CtlColor &msg);
   LRESULT On(const Msg::LButtonDown &msg);
   LRESULT On(const Msg::XButtonDown &msg);

   void DestroyPanes();

   Wnd_MDI *mp_wnd_MDI{};
   CntPtrTo<Prop::MainWindowSettings> mp_prop_main_window_settings;

   InputControl m_input;
   InputControl *mp_input_active{};

   Text::Wnd *mp_wnd_text{};
   Text::Wnd *mp_wnd_text_history{};
   CntPtrTo<Prop::TextWindow> mp_prop_output, mp_prop_history;

   friend struct ClientLayoutHelper;
   ClientLayoutHelper *mp_client_layout_helper;
   AL::Splitter *mp_splitter;
   AL::Splitter *mp_splitter_input;

   // Idle handling
   Time::Timer m_idle_timer{[this]()
   {
      if(m_idle_string)
         SendLines(m_idle_string);
      else
         mp_connection->Send("\xFF\xF1", true, true); // Telnet NOP
   }};

   unsigned m_idle_delay{}; // Idle time (in milliseconds)
   OwnedString m_idle_string;    // String to send

   struct DelayTimer : Time::Timer, DLNode<DelayTimer>
   {
      DelayTimer(Wnd_Main &wnd_main, ConstString string, float seconds, bool repeating=false)
       : Time::Timer([this]() { OnHit(); }),
         DLNode<DelayTimer>(wnd_main.m_delay_timers.Prev()), m_wnd_main(wnd_main), m_string(string)
      {
         Set(seconds, repeating);
      }

      void OnHit()
      {
         m_wnd_main.SendLines(m_string);
         if(!IsRepeating())
            delete this;
      }

      Wnd_Main &m_wnd_main;
      OwnedString m_string;
      unsigned m_id{m_wnd_main.m_next_delay_id++};
   };

   OwnedDLNodeList<DelayTimer> m_delay_timers;
   unsigned m_next_delay_id{1};

   // Window/Tab Flashing
   Time::Timer m_timer_tab_flash{[this]() { FlashTab(); }};
   bool m_active{};     // true if we are the active Window
   bool m_flash_state{}; // If the window has been flashed

   unsigned m_important_activity_count{};

   bool m_ignore_next_char{}; // Makes the next EditChar get ignored when true

   UniquePtr<Connection> mp_connection;

   TEXTMETRIC m_tmInput;
   int m_last_input_height{};

   unsigned m_history_pos{~0U};  // Currently displayed item from the History Window (~0U if none)
   OwnedString m_strText; // Current line of text (if you go into the queue)

   UniquePtr<AL::LayoutEngine> mp_layout;
   bool m_suspend_layout{}; // Suspend layout when restoring a docking configuration until all is finished

   DLNodeList<SpawnWindow> m_spawn_windows;
   UniquePtr<SpawnWindow> mp_null_spawn;
   DLNodeList<SpawnTabsWindow> m_spawn_tabs_windows;
   Collection<Wnd_InputPane*> m_input_panes;
   Collection<Wnd_EditPane*> m_edit_panes;
   OwnerPtr<Wnd_EditPropertyPane> mp_character_notes_pane;
   UniquePtr<TileMaps> mp_tile_maps;
   bool m_tile_maps_enabled{true};
   OwnerPtr<Wnd_Image> mp_wnd_image;
   OwnerPtr<Maps::Wnd> mp_wnd_map;
#ifdef YARN
   OwnerPtr<IYarn_TileMap> mp_yarn_tilemap;
#endif
   DLNodeList<Stats::Wnd> m_stat_windows;
   DLNodeList<Wnd_WebView> m_webview_windows;

   void UpdateTitle();

   OwnedString m_title_prefix; // Prefix before window title
   OwnedString m_title; // Window title (we save it so we can change prefixes)

   void AddVariable(ConstString name, ConstString value);
   Collection<Variable> m_variables;

   CntPtrTo<OM::MainWindow> mp_dispatch;
   friend class OM::MainWindow;
};

struct Wnd_MDI : TWindowImpl<Wnd_MDI>, DLNode<Wnd_MDI>
{
   static ATOM Register();

   Wnd_MDI(Prop::Position *pPosition=nullptr);
   static Wnd_MDI &GetInstance() noexcept { return *s_root_node.Next(); } // Return the first node
   static DLNode<Wnd_MDI> s_root_node;
   bool IsLast() noexcept { return Prev()==&s_root_node && Next()==&s_root_node; }

   void AddWindow(Wnd_Main &window);
   void DeleteWindow(Wnd_Main &window);
   void Connect(Prop::Server *ppropServer, Prop::Character *ppropCharacter, Prop::Puppet *ppropPuppet, bool fSetActiveWindow);

   Wnd_Main &GetActiveWindow() { AssumeAssert(mp_active_wnd_main); return *mp_active_wnd_main; }
   int GetActiveWindowIndex() const;
   Wnd_Main &GetWindow(unsigned index) noexcept;
   void SetActiveWindow(Wnd_Main &window, bool active=true);

   void PopupMainMenu(int2 position);

   void OnPropChange();
   void OnWindowChanged(Wnd_Main &window);
   void WindowFlash(Wnd_Main &window, bool flash);
   void RefreshTaskbar(Wnd_Main &window); // Redraw window title (eg. to show unread lines)
   static void RefreshBadgeCount();

   Wnd_Taskbar &GetTaskbar() noexcept { return *mp_wnd_taskbar; }

   DLNode<Wnd_Main> &GetRootWindow() noexcept { return m_root_wnd_main; }
   unsigned GetWindowCount() const noexcept { return m_window_count; }

private:

   ~Wnd_MDI() noexcept;

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Close &msg);
   LRESULT On(const Msg::QueryEndSession &msg);
   LRESULT On(const Msg::EndSession &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::SysCommand &msg);
   LRESULT On(const Msg::Size &msg);
   LRESULT On(const Msg::ExitSizeMove &msg);
   LRESULT On(const Msg::Activate &msg);
   LRESULT On(const Msg::SetFocus &msg);

   Rect m_rect_client;
   Wnd_Main *mp_active_wnd_main{};

   DLNodeList<Wnd_Main> m_root_wnd_main;
   unsigned m_window_count{};

   static unsigned s_badge_number;

   OwnerPtr<Wnd_HistoryGraph> m_pHistoryGraph;
   Wnd_Taskbar *mp_wnd_taskbar{};
   bool m_in_WM_CLOSE{};
};
