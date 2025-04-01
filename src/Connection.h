#pragma once
//
// Connection processing class
//
struct Connection;
struct Puppet;
struct Wnd_Main;
struct SpawnWindow;
struct Wnd_CaptureAbort;
struct Log;
struct Client_Media;
namespace GMCP
{
struct Avatar_Info;
struct Avatars;
}

namespace MCP
{
   struct Parser;
};

#include "Telnet.h"
#include "TextToLine.h"

struct NetworkDebugHost : Text::IHost
{
   void On(Text::Wnd &wndText, Text::Wnd_View &wnd_view, const Msg::RButtonDown &msg, const Text::List::LocationInfo &location_info) override;

   void Format(StringBuilder &string, Array<const uint8> data, bool recv);

   TelnetDebugger m_telnet_send;
   TelnetDebugger m_telnet_recv;

   bool m_show_hex{false};
   bool m_show_telnet{true};
};

struct Connection
 : DLNode<Connection>,
   IError,
   Events::Sends_Deleted,
   private IClient_Notify,
   private TelnetParser::INotify,
   private Sockets::GetHost::INotify,
   Events::ReceiversOf<Connection, Event_NewDay>
{
   static DLNodeList<Connection> s_root_node;
   struct Event_Created
   {
      Event_Created(Connection &connection) : m_connection(connection) { }
      Connection &m_connection;
   };
   static Events::SendersOf<Event_Created> s_sender_of_created;

   Connection(Wnd_Main &wnd_main);
   ~Connection();

   void Receive(Array<const char> text); // Packet of network data
   bool IsInReceive() const { return m_in_receive; }

   // TelnetParser::INotify
   void TelnetParser::INotify::OnLine(ConstString string) override; // Single line of network data
   void TelnetParser::INotify::OnTelnet(ConstString string) override;
   void TelnetParser::INotify::OnPrompt(ConstString string) override;
   void TelnetParser::INotify::OnEncoding(Prop::Server::Encoding encoding) override;
   void TelnetParser::INotify::OnGMCP(ConstString string) override;
   void TelnetParser::INotify::OnDoNAWS() override;

   void Display(ConstString string); // Parses ansi, html tags, etc.. turns it into a Text::Line, then puppets, then passes to Display
   void Display(UniquePtr<Text::Line> &&line); // Does triggers and then displays the line

   // When event is true, an OnSend event will be generated.
   void Send(ConstString string, bool send_event=true, bool raw=false);
   void SendGMCP(ConstString package, ConstString json);
   void RawSend(Array<const uint8> data) { mp_client->Input(data); }

   // Send the input line to the output window, styles and everything get parsed
   void Text(ConstString string);
   // Same as above Text function except no style parsing is involved and text is set to clr
   void Text(ConstString string, Color color);

   // Used by the connect code to know whether or not to try connecting a puppet
   static Connection *FindCharacterConnection(const Prop::Character *p_prop_character);

   void Associate(Prop::Server *p_prop_server, Prop::Character *p_prop_character, Prop::Puppet *p_prop_puppet);

   bool IsConnected() const noexcept;
   void Connect(bool offline);
   bool Reconnect(); // We must have a last server to work.  Returns true if we tried, false otherwise
   void Disconnect();
   void GetWorldTitle(StringBuilder &string, unsigned unread_count);

   void Character(Prop::Character &prop_character);
   void SetupCharacter();
   void SetupPuppet();

   const Prop::KeyboardMacro *MacroKey(const KEY_ID *pKeyID);

   struct AliasState
   {
      Collection<Variable> &m_variables;

      bool m_stop{}; // Stop processing further alises
      bool m_aliased{}; // Set to true if any alias hits
   };

   bool ProcessAliases(StringBuilder &string);
   void ProcessAlias(StringBuilder &string, Prop::Alias &propAlias, Connection::AliasState &state);
   void ProcessAliases(StringBuilder &string, Prop::Aliases &propAliases, Connection::AliasState &state);

   uint32 TimeConnectedInSeconds();
   uint32 TimeIdleInSeconds();
   bool KillSpawnCapture();
   void ShowSpawnCancel();

   bool m_mute_audio{};
   bool IsLogging() const { return mp_log!=nullptr; }
   Log &GetLog() { Assert(mp_log); return *mp_log; }
   void LogStart(ConstString filename, unsigned type);
   void LogStop();
   void AutoLogStart(); // If someone stops a log and wants to restart the autolog, call this

   bool InReplay() const { return m_raw_log_replay; }

   void StartPing() { m_ping_timer.Start(); }
   bool HasMudPrompt() const;
   void ResetAnsi() { m_text_to_line.ResetAnsi(); }

   Prop::Server    *GetServer()    { return m_ppropServer; }
   Prop::Character *GetCharacter() { return m_ppropCharacter; }
   Prop::Puppet    *GetPuppet()    { return m_ppropPuppet; }

   MCP::Parser *GetMCP_Parser() { return mp_MCP; }
   Wnd_Main &GetMainWindow() const { return m_wnd_main; }
   OM::IWindow_Main *STDMETHODCALLTYPE GetWindow_Main();

   Time::Stopwatch m_ping_timer;

   TelnetParser &GetTelnet() { return m_telnet_parser;  }

   void OpenNetworkDebugWindow();
   void OpenTriggerDebugWindow();
   void OpenAliasDebugWindow();

   void ShowConnectionInfo();

   bool OnGMCP_Parse(ConstString gmcp); // Return true if the GMCP should be written to the restore log
   void GMCP_Dump(bool dump) { m_GMCP_dump=dump; }
   bool m_GMCP_dump{};

   void MCMP_Info();
   void MCMP_Flush();
   UniquePtr<Client_Media> mp_MCMP;
   UniquePtr<GMCP::Avatars> mp_avatars;

   GMCP::Avatars &GetAvatars();

   void Away(bool away) { m_away=away; m_away_notified=false; }

   //
   // Events
   //
   struct Event_Receive : Events::Stoppable
   {
      Event_Receive(ConstString string) : m_string{string} { }

      ConstString GetString() const { return m_string; }

   private:
      ConstString m_string;
   };

   struct Event_Display : Events::Stoppable
   {
      Event_Display(UniquePtr<Text::Line> &ptl) : m_ptl(ptl) { }

      const Text::Line &GetTextLine() const { return *m_ptl; }
      Text::Line *Extract() { Assert(Stop()); return m_ptl.Extract(); }

   private:
      UniquePtr<Text::Line> &m_ptl;
   };

   struct Event_Send : Events::Stoppable
   {
      Event_Send(ConstString string) : m_lstr(string) { }

      ConstString GetString() const { return m_lstr; }

   private:
      ConstString m_lstr;
   };

   struct Event_GMCP
   {
      Event_GMCP(ConstString string) : m_lstr(string) { }

      ConstString GetString() const { return m_lstr; }

   private:
      ConstString m_lstr;
   };

   struct Event_Activity { };
   struct Event_Connect { };
   struct Event_Disconnect { };
   struct Event_Log { };

   Events::SendersOf<Event_Log, Event_Send, Event_Receive, Event_Display, Event_Activity, Event_Connect, Event_Disconnect, Event_GMCP> m_events;
   template<typename TEvent> operator Events::SenderOf<TEvent> &() { return m_events.Get<TEvent>(); }

   OwnedString m_grab_prefix; // If non empty, we're watching for a grab prefix to put into the input window

   // Apply these to the next line that comes in
   OwnedString m_line_image_url; // Avatar URL for the next incoming line
   GMCP::Avatar_Info *mp_line_avatar_info{};

   // Events
   void On(const Event_NewDay &event);
   void OnConnectTimeout();
   void OnPromptTimer();

private:

   void Connect(const Sockets::Address &address);
   void ConnectCharacter();
   void ConnectPuppet();
   void ReplayRestoreLog();
   void UpdateMainWindowTitle();

   // Sockets::GetHost::INotify
   void OnHostLookupFailure(DWORD error) { Disconnected(error); }
   void OnHostLookup(const Sockets::Address &address);

   bool CanRetryConnecting() const noexcept; // Returns true if it will retry a connection failure on this connection

   // IError
   void Error(ConstString string);

   // IClient_Notify
   void Connected() override;
   void Disconnected(DWORD error) override;
   void Output(Array<const BYTE> data) override;
   void ModuleMessage(ConstString text) override { Text(text); }
   void OnTransmit(Array<const BYTE> data) override;
   Wnd_Main &GetWndMain() override { return m_wnd_main; }

   struct MultilineTrigger
   {
      CntPtrTo<Prop::Triggers> mp_triggers;
      unsigned m_line_count{}, m_line_limit{};
      Time::Timer m_timeout;
   };

   Collection<UniquePtr<MultilineTrigger>> m_multiline_triggers, m_new_multiline_triggers;
   void OnMultilineTriggerTimeout(MultilineTrigger &v);

   struct TriggerState
   {
      bool m_gag{}, m_gag_log{}, m_stop{};
      bool m_no_activity{}; // Don't show as activity

      CntPtrTo<Prop::Trigger> mp_spawn_trigger;
      OwnedString m_spawn_title;
   };
   void RunTriggers(Text::Line &line, Array<CopyCntPtrTo<Prop::Trigger>> triggers, TriggerState &state);
   void TriggerDebugText(ConstString background_color, ConstString stroke_color, ConstString indent, ConstString message);
   void AliasDebugText(ConstString background_color, ConstString stroke_color, ConstString indent, ConstString message);

   Text::Wnd &GetOutput();

   UniquePtr<IClient> mp_client; // Current Client Module
   AsyncOwner<Sockets::GetHost> mp_get_host; // Asynchronous DNS lookup host

   bool m_auto_log{}; // True when the log was started automatically (not by the user)

   bool m_away{true}; // False if the user has this connection on the screen right now (window is active)
   bool m_away_notified{true}; // True if we've already notified the user of activity while the window is not active

   Wnd_Main &m_wnd_main;

   TextToLine m_text_to_line{m_wnd_main};
   TelnetParser m_telnet_parser{*this};

   int64 m_qpc_disconnect{}; // Tick when we disconnected (ignored if we're still connected)
   int64 m_qpc_connect{};    // Tick when connection was made
   int64 m_qpc_last_send{};   // Tick from last sent data (for how long we have been idle)
   int64 GetQPC() const noexcept { return IsConnected() ? Time::QueryPerformanceCounter() : m_qpc_disconnect; }
#if 0
   bool m_fMuckNetConnected{};
#endif

   Time::Timer m_timer_connect{[this]() { OnConnectTimeout(); }}; // Connect Timeout
   Time::Timer m_timer_prompt{[this]() { OnPromptTimer(); }};
   Text::NotifiedLinePtr mp_prompt; // Prompt is visible in the output window
   void RemovePrompt();
   NotifiedPtrTo<SpawnWindow> mp_captured_spawn_window; // If set, capture all incoming text here until the capture is finished
   Time::Timer m_timer_spawn_capture{[this]() { ShowSpawnCancel(); }};
   OwnerPtr<Wnd_CaptureAbort> mp_captureabort_window; // If set, capture all incoming text here until the capture is finished

   UniquePtr<MCP::Parser> mp_MCP; // Valid when connection supports MCP
   bool m_in_MCP{}; // True when we have passed data to the MCP loop (to prevent reentrency)
   bool m_in_send{};
   bool m_in_receive{};

   bool m_pueblo{};
   bool m_raw_log_replay{}; // True when we're replaying a raw log (so no trigger sounds/sends/etc)

   int m_connect_retries{};
   bool m_connect_retry{}; // Briefly set when Disconnecting on a retry

   Prop::Connections &m_propConnections;
   CntPtrTo<Prop::Server>    m_ppropServer;
   CntPtrTo<Prop::Character> m_ppropCharacter;   // Should always be from current m_ppropServer!
   CntPtrTo<Prop::Puppet>    m_ppropPuppet;
   bool m_setup{}; // Character or puppet data is restored and set

   Puppet *mp_puppet{}; // If we're the puppet
   DLNodeList<Puppet> m_puppets; // Currently in use Puppets

   IEvent_Prepare &m_ievent_prepare;

   UniquePtr<Log> mp_log;
   OwnerPtr<Text::Wnd> mp_network_debug;
   OwnerPtr<Text::Wnd> mp_trigger_debug;
   OwnerPtr<Text::Wnd> mp_alias_debug;
   NetworkDebugHost m_network_debug_host;

   friend struct Puppet;
   friend struct Wnd_CaptureAbort;
};

struct Puppet : IClient, DLNode<Puppet>
{
   Puppet(Prop::Puppet *p_prop_puppet, Connection *p_connection_master, Connection *p_connection_puppet) noexcept;
   ~Puppet();

   // IModule
   void Connect(const Sockets::Address &address, ConstString serverName, bool verify_certificate) override;
   void Disconnect() override;
   void Input(Array<const BYTE> data) override;

   bool IsConnected() const override { return mp_connection_puppet!=nullptr; }
   bool IsConnecting() const override { return false; }

   bool fPuppetRegEx(UniquePtr<Text::Line> &&pLine);

   Prop::Puppet *GetPuppet() noexcept { return mp_prop_puppet; }
   Connection &GetConnectionMaster() noexcept { return *mp_connection_master; }

   static uint2 FindRegEx(const Prop::Puppet &propPuppet, ConstString string);

private:

   CntPtrTo<Prop::Puppet> mp_prop_puppet;
   Connection *mp_connection_master, *mp_connection_puppet;
};
