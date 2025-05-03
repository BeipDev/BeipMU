//
// Text processing class
//

#include "Main.h"
#include "JSON.h"
#include "OM_Help.h"
#include "Wnd_Main.h"
#include "Wnd_MuckNet.h"
#include "Wnd_Stats.h"
#include "WebView.h"

#include "Connection.h"

#include "App_OM.h" // For ArrayUInt
#include "Wnd_Main_OM.h" // For GetWindow_Main
#include "Wnd_Text_OM.h"
#include "Connection_OM.h"
#include "FindString.h"
#include "Scripter.h"
#include "NotificationIcon.h"
#include "Emoji.h"
#include "CodePages.h"
#include "AnimatedGif.h"
#include "ImagePane.h"
#include "ImageBanner.h"
#include "Sounds.h"
#include "Speech.h"

#include "MCP.h"
#include "MCMP.h"
#include "GMCP.h"

//
// Connection
//
Events::SendersOf<Connection::Event_Created> Connection::s_sender_of_created;

DLNodeList<Connection> Connection::s_root_node;

Connection::Connection(Wnd_Main &wnd_main)
 : DLNode<Connection>(s_root_node.Prev()),
   m_wnd_main(wnd_main),
   m_propConnections{g_ppropGlobal->propConnections()},
   m_ievent_prepare(wnd_main)
{
   AttachTo<Event_NewDay>(GlobalEvents::GetInstance());
   s_sender_of_created.Send(Event_Created(*this));
}

Connection::~Connection()
{
}

void NetworkDebugHost::Format(StringBuilder &string, Array<const uint8> data, bool recv)
{
   if(m_show_hex)
   {
      for(auto &c : data)
         string(Strings::Hex32(c, 2), ' ');
      string(CRLF);
   }
   if(m_show_telnet)
   {
      auto &telnet=recv ? m_telnet_recv : m_telnet_send;
      telnet.Parse(string, data);
   }
}

void NetworkDebugHost::On(Text::Wnd &wnd_text, Text::Wnd_View &wnd_view, const Msg::RButtonDown &msg, const Text::List::LocationInfo &location_info)
{
   ContextMenu menu;

   if(!wnd_text.GetTextList().SelectionGet())
   {
      menu.Append(m_show_hex ? MF_CHECKED : 0, "Show hex", [&]() { m_show_hex^=true; });
      menu.Append(m_show_telnet ? MF_CHECKED : 0, "Show telnet + ascii", [&]() { m_show_telnet^=true; });
      menu.AppendSeparator();
      menu.Append(0, "Find...", [&]() { CreateDialog_Find(wnd_text, wnd_text); });
      menu.Append(wnd_text.IsUserPaused() ? MF_CHECKED : 0, "Pause", [&]() { wnd_text.SetUserPaused(!wnd_text.IsUserPaused()); });
      menu.Append(wnd_text.IsSplit() ? MF_CHECKED : 0, "Split", [&]() { wnd_text.ToggleSplit(); });
      menu.Append(0, "Copy screen to clipboard", [&]() { wnd_view.ScreenToClipboard(); });
      menu.AppendSeparator();
      menu.Append(0, "Clear", [&]() { wnd_text.Clear(); });
   }

   menu.Do(TrackPopupMenu(menu, TPM_RETURNCMD, GetCursorPos(), wnd_text, nullptr));
}


void Connection::Output(Array<const BYTE> data)
{
   if(mp_network_debug)
   {
      HybridStringBuilder<> string("<p background-color='maroon' stroke-color='white' stroke-width='2' border-style='round' border='4' padding-bottom='4'> Received ", data.Count(), " bytes");
      mp_network_debug->AddHTML(string); string.Clear();
      m_network_debug_host.Format(string, data, true);
      mp_network_debug->AddHTML(string);
   }

   Receive(Array((const char *)&*data.begin(), data.Count()));
}

void Connection::OnTransmit(Array<const BYTE> data)
{
   if(!mp_network_debug)
      return;

   HybridStringBuilder<> string("<p background-color='blue' stroke-color='white' stroke-width='2' border-style='round' border='4' padding-bottom='4'> Sent ", data.Count(), " bytes");
   mp_network_debug->AddHTML(string); string.Clear();
   m_network_debug_host.Format(string, data, false);
   mp_network_debug->AddHTML(string);
}

void Connection::Receive(Array<const char> text)
{
   Assert(!m_in_receive);
   RestorerOf _(m_in_receive); m_in_receive=true;
   Assert(text.Last()!=0); // Someone accidentally had a C string turned into an array, pass any C strings wrapped in ConstString

   if(m_ping_timer)
   {
      Text(FixedStringBuilder<256>("<icon information> <font color='lime'>Ping response time: <font color='white'>", m_ping_timer.SecondsSinceStart(), " seconds"));
      m_ping_timer.Stop();
   }

   if(m_ppropCharacter && !m_ppropPuppet)
      m_ppropCharacter->BytesReceived(m_ppropCharacter->BytesReceived()+text.Count());

   m_telnet_parser.Parse(text);
   if(m_ppropServer && m_ppropServer->fPrompts() && m_telnet_parser.HasPartial() && !m_timer_prompt)
      m_timer_prompt.Set(0.05f);
}

void Connection::OnLine(ConstString string)
{
   if(!mp_MCP && m_ppropServer && m_ppropServer->fMCP())
   {
      if(MCP::HasMCP(string))
      {
         RestorerOf _(m_in_MCP); m_in_MCP=true;
         mp_MCP=MakeUnique<MCP::Parser>(*this, string);
         return;
      }
   }

   if(mp_MCP && !m_in_MCP)
   {
      RestorerOf _(m_in_MCP); m_in_MCP=true;
      mp_MCP->OnLine(string);
      return;
   }

   if(!m_pueblo && m_ppropServer && m_ppropServer->fPueblo())
   {
      // http://pueblo.sourceforge.net/doc/manual/pueblo_enhancers_guide.html
      if(string.StartsWith("This world is Pueblo "))
         Send("PUEBLOCLIENT 2.01\r\n");

      if(string.Find(ConstString("</xch_mudtext>"))!=Strings::Result::Not_Found)
         m_pueblo=true;
   }

   if(m_ppropCharacter && !m_ppropPuppet)
   {
      if(auto index=m_ppropCharacter->RestoreLogIndex();index!=-1)
         gp_restore_logs->WriteReceived(index, string);
   }

   if(m_propConnections.fStripTrailingSpaces())
   {
      while(string.EndsWith(' '))
         string=string.WithoutLast(1);
   }

   // Check for Event Hooks
   if(m_events.Get<Event_Receive>())
   {
      m_ievent_prepare.Event_Prepare();

      Event_Receive event(string);
      if(m_events.Send(event, event))
         return;
   }

   Display(string);
}

void Connection::OnTelnet(ConstString string)
{
   // Simply send the telnet codes directly to the server
   if(mp_client)
      mp_client->Input(string);
}

void Connection::OnPrompt(ConstString string)
{
   if(m_ppropServer && m_ppropServer->fPrompts())
   {
      RemovePrompt();

      m_timer_prompt.Reset();
      auto p_line=m_text_to_line.Parse(string, m_ppropServer->fHTMLTags(), m_ppropServer->eEncoding());
      mp_prompt=*p_line;
      GetOutput().Add(std::move(p_line));
   }
   else
   {
      OnLine(string);
      m_telnet_parser.Reset();
   }
}

void Connection::OnEncoding(Prop::Server::Encoding encoding)
{
   Text(FixedStringBuilder<256>("<icon information> <font color='lime'>Charset negotiated: ", g_encoding_names[int(encoding)]));
   m_ppropServer->eEncoding(encoding);
}

void Connection::MCMP_Info()
{
   if(mp_MCMP)
      mp_MCMP->DumpInfo(GetOutput());
   else
      Text("<icon error> MCMP not active");
}

void Connection::MCMP_Flush()
{
   if(mp_MCMP)
      mp_MCMP=nullptr;
}

GMCP::Avatars &Connection::GetAvatars()
{
   if(!mp_avatars)
      mp_avatars=MakeUnique<GMCP::Avatars>(*this);
   return *mp_avatars;
}

struct JSON_Beip_Line_IDs : JSON::Element
{
   JSON_Beip_Line_IDs(Connection &connection) : m_connection{connection} { }

   void OnString(ConstString name, ConstString value) override
   {
      m_connection.mp_line_avatar_info=&m_connection.GetAvatars().Lookup(value);
   }

   Connection &m_connection;
};

struct JSON_Beip_Image_URL : JSON::Element
{
   JSON_Beip_Image_URL(Wnd_Main &wnd_main) : m_wnd{wnd_main} { }

   void OnString(ConstString name, ConstString value) override
   {
      m_wnd.GetConnection().m_line_image_url=value;
   }

   Wnd_Main &m_wnd;
};

struct JSON_WebView_Headers : JSON::Element
{
   void OnString(ConstString name, ConstString value) override
   {
      m_headers.Push(name, value);
   }

   Collection<Wnd_WebView::Header> m_headers;
};

//  webview.open { "id":"Character editor", "dock":"right", "url":"value", "http-request-headers":{ "name1":"value1", "name2":"value2" } }

struct JSON_WebView_Open : JSON::Element
{
   void OnString(ConstString name, ConstString value) override
   {
      if(name=="url")
         m_url=value;
      else if(name=="id")
         m_id=value;
      else if(name=="source")
         m_source=value;
      else if(name=="dock")
      {
         if(value=="left")
            m_docking_side=Docking::Side::Left;
         else if(value=="right")
            m_docking_side=Docking::Side::Right;
         else if(value=="top")
            m_docking_side=Docking::Side::Top;
         else if(value=="bottom")
            m_docking_side=Docking::Side::Bottom;
         else
         {
            Assert(false);
            return;
         }
         m_docked=true;
      }
      else
         Assert(false);
   }

   JSON::Element &OnObject(ConstString name) override
   {
      if(name=="http-request-headers")
         return m_headers;
      return __super::OnObject(name);
   }

   OwnedString m_url, m_id, m_source;

   bool m_docked{};
   Docking::Side m_docking_side;

   JSON_WebView_Headers m_headers;
};

void Connection::OnGMCP(ConstString string)
{
   if(m_GMCP_dump)
   {
      HybridStringBuilder<> text("<font color='red'>GMCP Received:</font> ", Text::NoHTML{string});
      ConsoleHTML(text);
   }

   if(OnGMCP_Parse(string))
   {
      if(m_ppropCharacter && !m_ppropPuppet)
      {
         if(auto index=m_ppropCharacter->RestoreLogIndex();index!=-1)
            gp_restore_logs->WriteReceived_GMCP(index, string);
      }
   }

   if(m_events.Get<Event_GMCP>())
   {
      Event_GMCP event(string);
      m_events.Send(event);
   }
}

void Connection::OnDoNAWS()
{
   m_wnd_main.SendNAWS();
}

bool Connection::OnGMCP_Parse(ConstString string)
{
   ConstString package;
   if(!string.Split(' ', package, string))
      return false;

   // Package names are case insensitive, so convert to lowercase to simplify all later comparisons
   HybridStringBuilder<256> package_copy{package}; package_copy.ToLower(); package=package_copy;

   ConstString name;
   if(!package.Split('.', name, package))
      return false;

   try
   {
      if(name=="webview")
      {
         if(package=="open")
         {
            JSON_WebView_Open element;
            JSON::ParseObject(element, string);

            switch(m_ppropServer->GMCP_WebView())
            {
               case 0: return false; // Ignore
               case 1: break; // Allow
               case 2:
               {
                  ConstString link=element.m_url;
                  if(!link)
                     link=element.m_source.First(std::min(50U, element.m_source.Count()));
                  switch(MessageBoxCustom(m_wnd_main, "Allow WebView?", HybridStringBuilder<>("The server wants to open \n\n", link, "\n"),
                     std::array<ConstString, 4>{"Ignore Once", "Allow Once", "Allow All", "Ignore All"}))
                  {
                     case 0: return false; // Ignore
                     case 1: break; // Allow
                     case 2: m_ppropServer->GMCP_WebView(1); break; // Always allow
                     case 3: m_ppropServer->GMCP_WebView(0); return false; // Always ignore
                  }
                  break;
               }
            }

            Wnd_WebView *p_webview{};
            if(element.m_id)
               p_webview=m_wnd_main.FindWebView(element.m_id);
            if(!p_webview)
            {
               p_webview=new Wnd_WebView(m_wnd_main, element.m_id);
               if(element.m_docked)
                  p_webview->GetDocking().Dock(element.m_docking_side);
            }

            if(element.m_source)
               p_webview->SetSource(element.m_source);
            else
               p_webview->SetURL(element.m_url, element.m_headers.m_headers);
            return false;
         }
         return false;
      }

      if(name=="beip")
      {
         if(package=="stats")
         {
            GMCP::On_Stats(m_wnd_main, string);
            return false;
         }
         else if(auto command=package.RightOf("tilemap."))
         {
            m_wnd_main.On_TileMap(command, string);
            return false;
         }
         else if(package=="ids")
         {
            GetAvatars().OnGMCP(string);
            return false;
         }
         else if(auto command=package.RightOf("line."))
         {
            if(command=="id")
            {
               JSON_Beip_Line_IDs element{*this};
               JSON::Parse(element, string);
               return false;
            }

            if(command=="image-url")
            {
               JSON_Beip_Image_URL element{m_wnd_main};
               JSON::Parse(element, string);
               return true;
            }
         }
      }
      else if(name=="client")
      {
         ConstString module;
         if(!package.Split('.', module, package))
            return false;

         if(module=="media")
         {
            if(!m_ppropServer->fMCMP())
               return false;
            if(!mp_MCMP)
               mp_MCMP=MakeUnique<Client_Media>();

            mp_MCMP->OnGMCP(package, string);
            return false;
         }
      }
      else if(name=="room")
      {
         if(package=="info")
         {
            GMCP::On_RoomInfo(m_wnd_main, string);
            return false;
         }
      }
   }
   catch(const std::exception &message)
   {
      ConsoleText(SzToString(message.what()));
   }

   return false;
}

void Connection::RemovePrompt()
{
   if(mp_prompt)
   {
      GetOutput().RemoveLine(*mp_prompt);
      Assert(!mp_prompt);
   }
}

bool Connection::Reconnect()
{
   if(IsConnected() || !m_ppropServer)
      return false;

   Connect(false);
   return true;
}

void Connection::Send(ConstString string, bool send_event, bool raw)
{
   // Fake out local echo for prompts since the mud will not send a newline in this case.
   if(HasMudPrompt() && !m_in_MCP && !m_in_send && !m_in_receive)
   {
      RestorerOf _(m_in_send); m_in_send=true; // To prevent infinite recursion since Receive calls triggers on the prompt text.
      if(m_wnd_main.GetActiveInputWindow().GetProps().fLocalEcho())
         Receive(string);
      Receive(ConstString(CRLF));
   }

   if(m_timer_connect)
   {
      Text(STR_TryingToReconnect);
      return;
   }

   if(!IsConnected())
   {
      Text(STR_HTML_NotConnected);
      if(m_ppropServer && MessageBox(m_wnd_main, STR_AskReconnect, STR_NotConnected, MB_ICONEXCLAMATION|MB_YESNO)==IDYES)
         Reconnect();
      return;
   }

   // Check for Event Hooks
   if(send_event && m_events.Get<Event_Send>())
   {
      m_ievent_prepare.Event_Prepare();

      Event_Send event(string);
      if(m_events.Send(event, event))
         return;
   }

   if(IsLogging() && !raw)
      LogSent(string);
   if(m_ppropCharacter && !m_ppropPuppet)
      m_ppropCharacter->BytesSent(m_ppropCharacter->BytesSent()+string.Count()+2); // 2 for the CRLF

   // Unidle us
   m_qpc_last_send=Time::QueryPerformanceCounter();
   if(raw)
      mp_client->Input(string); // Raw data is sent as binary
   else
   {
      switch(m_ppropServer->eEncoding())
      {
         default:
         case Prop::Server::Encoding::UTF8: mp_client->Input(string); break;
         case Prop::Server::Encoding::CP1252: mp_client->Input(UTF8ToCodePage(string, 1252)); break;
         case Prop::Server::Encoding::CP437: mp_client->Input(UTF8ToCodePage(string, 437)); break;
      }

      if(!m_ppropPuppet)
         mp_client->Input(ConstString(CRLF));
   }
}

void Connection::SendGMCP(ConstString package, ConstString json)
{
   RawSend(ConstString{GMCP_BEGIN});
   RawSend(package);
   RawSend(ConstString(" "));
   RawSend(json);
   RawSend(ConstString{GMCP_END});

   if(m_GMCP_dump)
   {
      HybridStringBuilder<> string("<font color='blue'>GMCP Sent:</font> ", Text::NoHTML{package}, " ", Text::NoHTML{json});
      ConsoleHTML(string);
   }
}

void Connection::Text(ConstString string)
{
   GetOutput().AddHTML(string);
}

void Connection::Text(ConstString string, Color clrText)
//
// Do not parse the text, apply clrText to the whole line of text
{
   auto p_line=Text::Line::CreateFromText(string);
   p_line->SetColor(uint2(), clrText);
   GetOutput().Add(std::move(p_line));
}

void Connection::OnHostLookup(const Sockets::Address &address)
//
// Asynchronous DNS lookup callback, will be called after Connnect(host, port) is called,
// and before the private Connect is called.
{
   Text(FixedStringBuilder<256>(STR_HostAddressFound "<font color='#8080FF'>", Sockets::AddressToString(address), STR_Connecting));
   Connect(address);
}

bool Connection::CanRetryConnecting() const noexcept
{
   return m_connect_retries+1<m_propConnections.ConnectRetry() || m_propConnections.fRetryForever();
}

bool Connection::IsConnected() const noexcept
{
   return mp_get_host || mp_client && mp_client->IsConnected() || m_timer_connect;
}

void Connection::OnConnectTimeout()
{
   RestorerOf _{m_connect_retry}; m_connect_retry=CanRetryConnecting();

   // If we got here by a connection timing out, disconnect
   if(mp_client)
   {
      if(mp_client->IsConnecting())
      {
         Text(STR_ConnectionTimedOut);
         if(!m_connect_retry)
            Text(STR_NoMoreRetries);
      }
      Disconnect();
   }

   // Keep trying?
   if(m_connect_retry)
   {
      m_connect_retries++;
      FixedStringBuilder<256> string(STR_Retrying, m_connect_retries+1, " of ");
      if(m_propConnections.fRetryForever())
         string("unlimited");
      else
         string(m_propConnections.ConnectRetry());
      Text(string);
      Connect(false);
   }
}

void Connection::Connect(const Sockets::Address &address)
//
// Lowest level connect, connects to a given physical address and a port
{
   // In case we are disconnected, then called directly (not on a host name lookup)
   ConstString host_name=m_ppropServer->pclHost().BeforeFirst(':'); // Remove the port

   // Connection Timeout Timer
   m_timer_connect.Set(std::max(m_propConnections.ConnectTimeout()/1000.0f, 1.0f));

   mp_client->Connect(address, host_name, m_ppropServer->fVerifyCertificate());
}

Connection *Connection::FindCharacterConnection(const Prop::Character *ppropCharacter)
{
   for(auto &connection : s_root_node)
   {
      if(connection.GetCharacter()==ppropCharacter && !connection.GetPuppet())
         return &connection;
   }

   return nullptr;
}

void Connection::Associate(Prop::Server *ppropServer, Prop::Character *ppropCharacter, Prop::Puppet *ppropPuppet)
{
   Assert(!m_ppropServer);

   m_ppropServer=ppropServer;
   m_ppropCharacter=ppropCharacter;
   m_ppropPuppet=ppropPuppet;
}

void Connection::Connect(bool offline)
//
// Connect to a server and also automatically connects a character if ppropCharacter is non nullptr
{
   Assert(!mp_client);

   if(!m_setup)
   {
      if(m_ppropPuppet)
         SetupPuppet();
      else if(m_ppropCharacter)
         SetupCharacter();
   }

   if(offline)
      return;

   if(m_ppropPuppet)
   {
      Connection *p_character_connection=FindCharacterConnection(m_ppropCharacter);
      if(!p_character_connection)
      {
         Text(STR_PuppetCantConnect);
         return;
      }

      if(!p_character_connection->IsConnected())
         return;

      auto pPuppet=MakeUnique<Puppet>(m_ppropPuppet, p_character_connection, this);
      pPuppet->Link(p_character_connection->m_puppets.Prev());
      mp_puppet=pPuppet;

      mp_client=std::move(pPuppet);

      // Simulate a Connected event
      Connected(); // Act like a regular connection
      return;
   }

   // No host name?
   if(!m_ppropServer->pclHost())
   {
      Text(FixedStringBuilder<256>(STR_CantConnectTo, m_ppropServer->pclName(), STR_BecauseEmptyHostAddress));
      return;
   }

#if defined(_DEBUG)
   if(m_ppropServer->pclName()=="Entrepreneur Jungle")
      mp_client=Extensions::Client::Create_Entrepreneur(*this);
   else
#endif
#if YARN
   if(m_ppropServer->pclHost().IStartsWith("yarn://"))
   {
      mp_client=Extensions::Client::Create_Yarn(*this);
      m_pueblo=true;
   }  
   else
#endif
      mp_client=Extensions::Client::Create_Standard(*this, m_ppropServer->fTLS());

   if(!mp_client)
   {
      Text(FixedStringBuilder<256>(STR_ModuleNotInstalled, m_ppropServer->pclModule()));
      return;
   }

   Text(FixedStringBuilder<256>(STR_LookingUpHostName, m_ppropServer->pclHost()));

   Assert(!mp_get_host);
   String host, port;
   if(!m_ppropServer->pclHost().RSplit(':', host, port))
   {
      Text("<icon error> <font color='teal'>Missing Port, host should be in the form of <font color='aqua'>example.com:1234");
      Disconnect();
      return;
   }

   if(auto yarn_host=host.RightOf("yarn://"))
      host=yarn_host;

   new Sockets::GetHost(mp_get_host, *this, host, port, m_ppropServer->fIPV4());
   UpdateMainWindowTitle();

   // Set these to zero when we start connecting so we don't show odd values, we'll reset them again once we connect
   m_qpc_last_send=m_qpc_connect=Time::QueryPerformanceCounter();
}

void Connection::Disconnect()
{
   m_qpc_disconnect=Time::QueryPerformanceCounter();
   m_text_to_line.ResetAnsi(); // If we don't, current settings will affect new connections

   if(mp_get_host)
      mp_get_host.Cancel();

   while(m_puppets)
      m_puppets.Prev()->Disconnect(); // Implicitly removes puppet from m_puppets

   if(mp_client && mp_client->IsConnected())
   {
      // No module should keep IsConnected true otherwise we loop...
      mp_client->Disconnect(); // This should call back here on a notify, causing us to fall through next time
      return;
   }

   mp_client=nullptr;

   // Stop auto logging as soon as we disconnect
   if(mp_auto_log)
      StopAutoLog();

   // Update the last time used on the character
   if(m_ppropCharacter && !m_ppropPuppet)
   {
      m_ppropCharacter->timeLastUsed(Time::Local());
      m_ppropCharacter->SecondsConnected(m_ppropCharacter->SecondsConnected()+TimeConnectedInSeconds());
   }

   KillSpawnCapture();

   m_timer_connect.Reset();
   mp_puppet=nullptr;
   mp_MCP=nullptr;
   mp_MCMP=nullptr;
   m_pueblo=false;
   m_line_image_url.Clear();
   mp_line_avatar_info=nullptr;

   m_telnet_parser.Reset();
   m_telnet_parser.m_do_naws=false;
   m_network_debug_host.m_telnet_recv.Reset();
   m_network_debug_host.m_telnet_send.Reset();
   RemovePrompt();
   if(!m_connect_retry)
      m_connect_retries=0;

   if(!m_ppropPuppet && !m_connect_retry)
   {
      if(g_ppropGlobal->fNetworkMessagesInSpawns())
         m_wnd_main.BroadcastHTML(STR_Disconnected);
      else
         Text(STR_Disconnected);
      m_events.Send(Event_Activity());
   }

   // Here rather than on the Disconnected() event so that we can also
   // call it a disconnect when we abort a DNS lookup
   if(m_events.Get<Event_Disconnect>())
   {
      m_ievent_prepare.Event_Prepare();
      m_events.Send(Event_Disconnect());
   }
}

const Prop::KeyboardMacro *Connection::MacroKey(const KEY_ID *pKeyID)
{
   const Prop::KeyboardMacro *p_macro{};

   // Check for Character Macros
   if(m_ppropCharacter && m_ppropCharacter->fPropKeyboardMacros2())
      p_macro=m_ppropCharacter->propKeyboardMacros2().Macro(*pKeyID);
   // Server Macros
   if(!p_macro && m_ppropServer && m_ppropServer->fPropKeyboardMacros2())
      p_macro=m_ppropServer->propKeyboardMacros2().Macro(*pKeyID);
   // Global Macros
   if(!p_macro)
      p_macro=m_propConnections.propKeyboardMacros2().Macro(*pKeyID);

   if(!p_macro)
      return nullptr;

   if(p_macro->fType())
      return p_macro;

   if(auto &props=m_wnd_main.GetActiveInputWindow().GetProps();props.fLocalEcho())
      Text(p_macro->pclMacro(), props.clrLocalEchoColor());

   return p_macro;
}

void Connection::GetWorldTitle(StringBuilder &string, unsigned unread_count)
{
   FixedCollection<ConstString, 3> strings;

   if(m_ppropPuppet && m_ppropPuppet->pclName())
      strings.Push(m_ppropPuppet->pclName());

   if(m_ppropCharacter)
      strings.Push(m_ppropCharacter->pclName());

   if(m_ppropServer)
      strings.Push(m_ppropServer->pclName());

   if(strings.Count())
      string(strings[0]);
   if(unread_count)
      string(" (+", unread_count, ')');
   for(unsigned i=1;i<strings.Count();i++)
      string(" - ", strings[i]);

   // Only show host name if other names are blank
   if(!string && m_ppropServer)
      string('[', m_ppropServer->pclHost(), ']');

   if(!string)
      string("(New Tab)");
}

void Connection::UpdateMainWindowTitle()
{
   FixedStringBuilder<256> string; GetWorldTitle(string, 0);
   m_wnd_main.SetText(string);
}

uint32 Connection::TimeConnectedInSeconds()
{
   return (GetQPC()-m_qpc_connect)/Time::g_performance_frequency;
}

uint32 Connection::TimeIdleInSeconds()
{
   return (GetQPC()-m_qpc_last_send)/Time::g_performance_frequency;
}

// IError
void Connection::Error(ConstString string)
{
   Text(string);
}
//

void Connection::Character(Prop::Character &prop_character)
{
   Assert(!m_ppropCharacter && !m_ppropPuppet);
   m_ppropCharacter=&prop_character;
   SetupCharacter();
   ConnectCharacter();
}

void Connection::SetupCharacter()
{
   Assert(!m_setup);
   m_setup=true;

   m_wnd_main.RestoreDockingConfiguration(m_ppropCharacter->propDocking());
   ReplayRestoreLog();
}

void Connection::SetupPuppet()
{
   Assert(!m_setup);
   m_setup=true;

   m_wnd_main.RestoreDockingConfiguration(m_ppropPuppet->propDocking());
   // Puppets don't have restore logs
}

void Connection::ConnectCharacter()
{
   Assert(IsConnected());
   Assert(mp_client);

   // Start a new log if it's enabled and we don't have one
   if(m_ppropCharacter->fRestoreLog() && m_ppropCharacter->RestoreLogIndex()==-1)
      gp_restore_logs->Allocate(*m_ppropCharacter);

   // Write the start indicator
   if(auto index=m_ppropCharacter->RestoreLogIndex();index!=-1)
      gp_restore_logs->WriteStart(index);

   // If we don't have a module or we're disconnected or still connecting then wait
   // we'll call here again once we get the connect notify
   if(!mp_client->IsConnected() || mp_client->IsConnecting())
      return;

   Text(FixedStringBuilder<256>(STR_ConnectingCharacter, m_ppropCharacter->pclName()));

   if(ConstString connect_string=m_ppropCharacter->pclConnect())
   {
      OwnedString string=connect_string;

      while(auto range=string.FindRange("%NAME%"))
         string.Replace(range, m_ppropCharacter->pclName());

      if(m_ppropCharacter->pclPassword())
         while(auto range=string.FindRange("%PASSWORD%"))
            string.Replace(range, m_ppropCharacter->pclPassword());

      m_wnd_main.SendLines(string);
   }

   m_ppropCharacter->ConnectionCount(m_ppropCharacter->ConnectionCount()+1);
   m_ppropCharacter->timeLastUsed(Time::Running());
   m_wnd_main.UpdateCharacterIdleTimer();
   StartAutoLog();

   if(m_ppropCharacter->fPropPuppets())
   {
      for(auto &pPropPuppet : m_ppropCharacter->propPuppets())
         if(pPropPuppet->fConnectWithPlayer())
            m_wnd_main.GetMDI().Connect(m_ppropServer, m_ppropCharacter, pPropPuppet, false);
   }

   UpdateMainWindowTitle();
}

void Connection::ConnectPuppet()
{
   StartAutoLog();
   UpdateMainWindowTitle();
}

// IClient
void Connection::Connected()
{
   m_connect_retries=0;
   m_timer_connect.Reset();
   m_qpc_last_send=m_qpc_connect=Time::QueryPerformanceCounter();

   if(!m_ppropPuppet)
   {
      if(g_ppropGlobal->fNetworkMessagesInSpawns())
         m_wnd_main.BroadcastHTML(STR_Connected);
      else
         Text(STR_Connected);
      m_events.Send(Event_Activity());
   }

   if(m_ppropPuppet)
      ConnectPuppet();
   else if(m_ppropCharacter)
      ConnectCharacter();

   if(m_events.Get<Event_Connect>())
   {
      m_ievent_prepare.Event_Prepare();
      m_events.Send(Event_Connect());
   }
#if 0
   if(MuckNet::HasInstance() && !mp_prop_puppet)
   {
      MuckNet::GetInstance().Connection(m_ppropServer->pclHost(), true);
      m_fMuckNetConnected=true;
   }
#endif
}

void Connection::Disconnected(DWORD error)
{
   if(m_propConnections.fActivateDisconnect())
      m_wnd_main.GetMDI().SetActiveWindow(m_wnd_main);

#if 0
   if(MuckNet::HasInstance() && m_fMuckNetConnected)
      MuckNet::GetInstance().Connection(m_ppropServer->pclHost(), false);
   m_fMuckNetConnected=false;
#endif

   if(!error)
   {
      Disconnect();
      return;
   }

   switch(error)
   {
      case ERROR_SEM_TIMEOUT: error=WSAETIMEDOUT; break;
      case ERROR_NETNAME_DELETED: error=WSAENETUNREACH; break;
      case ERROR_CONNECTION_REFUSED: error=WSAECONNREFUSED; break;
   }

   Text(HybridStringBuilder("<icon error> <font color='red'>Error: <font color='aqua'>", SystemError{error}));
   if(m_connect_retries==0)
      m_events.Send(Event_Activity());

   bool retryable=
      error==WSATRY_AGAIN ||
      error==WSAENETUNREACH ||
      error==WSAECONNREFUSED ||
      error==WSAETIMEDOUT ||
      m_propConnections.fIgnoreErrors();

   RestorerOf _{m_connect_retry}; m_connect_retry=retryable && CanRetryConnecting();
   if(!m_connect_retry && retryable)
      Text(STR_NoMoreRetries);

   Disconnect();
   if(!m_connect_retry)
      return;

   float seconds=std::max(m_propConnections.ConnectTimeout()/1000.0f, 1.0f);
   m_timer_connect.Set(seconds);
   Text(FixedStringBuilder<256>("<icon information> <font color='aqua'>Will try to reconnect in ", int(seconds), " seconds"));
}

OM::IWindow_Main *Connection::GetWindow_Main()
{
   return m_wnd_main.GetDispatch();
}

// Display the given line of text in the output window, so that it can be processed
// by the triggers 
void Connection::Display(ConstString string)
{
   // Disabled due to the ansi test sending through the Receive function and winding up
   // here, as it must act as though the text was sent from a server!
   // Assert(m_ppropServer); // We should always have at least one of these
   auto pTL=m_text_to_line.Parse(string, m_pueblo, m_ppropServer ? m_ppropServer->eEncoding() : Prop::Server::Encoding::UTF8);

   if(mp_line_avatar_info)
   {
      auto p_image=MakeCounting<ImageAvatar>(*mp_line_avatar_info);
      mp_line_avatar_info=nullptr;
      pTL->SetParagraphRecord(Text::Records::ImageLeft(&*p_image));
   }

   if(m_line_image_url)
   {
      auto p_image=MakeCounting<ImageAvatar>(Text::ImageURL{std::move(m_line_image_url)});
      pTL->SetParagraphRecord(Text::Records::ImageLeft(&*p_image));
   }

   // Puppet text routing
   for(auto &puppet : m_puppets)
   {
      if(puppet.fPuppetRegEx(std::move(pTL)))
         return; // Handled by the puppet
   }

   // Autoconnect a puppet? (If we're not a puppet and the char has puppets)
   if(!m_ppropPuppet && m_ppropCharacter && m_ppropCharacter->fPropPuppets())
   {
      for(auto &pPropPuppet : m_ppropCharacter->propPuppets())
      {
         if(pPropPuppet->fAutoConnect() && Puppet::FindRegEx(*pPropPuppet, *pTL).end!=0)
         {
            m_wnd_main.GetMDI().Connect(m_ppropServer, m_ppropCharacter, pPropPuppet, false);

            // Now we can find the puppet and send the text to it
            for(auto &puppet : m_puppets)
            {
               if(puppet.GetPuppet()==pPropPuppet)
               {
                  AssertReturned<true>()==puppet.fPuppetRegEx(std::move(pTL));
                  return; // AutoConnected
               }
            }
         }
      }
   }

   Display(std::move(pTL));
}

void Connection::TriggerDebugText(ConstString background_color, ConstString stroke_color, ConstString indent, ConstString message)
{
   Assert(mp_trigger_debug);
   mp_trigger_debug->AddHTML(HybridStringBuilder("<p background-color='", background_color, "' stroke-color='", stroke_color, "' stroke-width='2' indent='", indent, "' border-style='round' align='center' padding='2'>", message));
}

void Connection::AliasDebugText(ConstString background_color, ConstString stroke_color, ConstString indent, ConstString message)
{
   Assert(mp_alias_debug);
   mp_alias_debug->AddHTML(HybridStringBuilder("<p background-color='", background_color, "' stroke-color='", stroke_color, "' stroke-width='2' indent='", indent, "' border-style='round' align='center' padding='2'>", message));
}

void Connection::Display(UniquePtr<Text::Line> &&p_line)
{
   // Handle \grab prefixes
   if(m_grab_prefix && p_line->GetText().StartsWith(m_grab_prefix))
   {
      m_wnd_main.GetInputWindow().SetText(p_line->GetText().WithoutFirst(m_grab_prefix.Count()));
      m_wnd_main.GetInputWindow().SetSelEnd();
      m_grab_prefix=nullptr;
      return;
   }

   if(mp_trigger_debug)
   {
      TriggerDebugText("#004000", "green", "0", "Starting triggers, original line:");
      mp_trigger_debug->Add(MakeUnique<Text::Line>(*p_line));
   }

   TriggerState state;
   if(Prop::Trigger *p_capture_until=mp_captured_spawn_window ? mp_captured_spawn_window->mp_capture_until : nullptr)
   {
      if(p_capture_until->propSpawn().fOnlyChildrenDuringCapture())
      {
         RunTriggers(*p_line, p_capture_until->propTriggers(), state);
         state.m_stop=true; // Don't process any other triggers
      }
   }

   // Process Triggers
   if(m_propConnections.propTriggers().fActive())
   {
      for(unsigned i=0;i<m_multiline_triggers.Count();i++)
      {
         auto &multiline=*m_multiline_triggers[i];
         RunTriggers(*p_line, *multiline.mp_triggers, state);
         if(multiline.m_line_limit!=0 && ++multiline.m_line_count>=multiline.m_line_limit)
         {
            m_multiline_triggers.Delete(i--);
            if(mp_trigger_debug)
               TriggerDebugText("#000040", "blue", "10", "Multiline child triggers hit line limit");
         }
      }

      RunTriggers(*p_line, m_propConnections.propTriggers().Pre(), state);

      if(m_ppropServer && m_ppropServer->propTriggers().fActive())
      {
         RunTriggers(*p_line, m_ppropServer->propTriggers().Pre(), state);

         if(m_ppropCharacter && m_ppropCharacter->propTriggers().fActive())
            RunTriggers(*p_line, m_ppropCharacter->propTriggers(), state);

         RunTriggers(*p_line, m_ppropServer->propTriggers().Post(), state);
      }

      RunTriggers(*p_line, m_propConnections.propTriggers().Post(), state);

      // Add new multiline triggers to beginning of list, such that the last one added to the list will be first
      while(m_new_multiline_triggers)
         m_multiline_triggers.Insert(0, m_new_multiline_triggers.Delete(0));
   }
   else
   {
      if(mp_trigger_debug)
         TriggerDebugText("#000040", "blue", "5", "Triggers are not enabled (Global folder has 'Process Child Triggers' unchecked) ");
   }

   if(mp_trigger_debug)
      TriggerDebugText("#400000", "red", "0", "Complete");

   p_line->ParseURLs();

   // Handle spawn windows
   SpawnWindow *p_spawn_window{};
   if(state.mp_spawn_trigger)
   {
      Assert(!mp_captured_spawn_window); // Should be nullptr as we can only be here if we're starting a new spawn

      auto &prop=state.mp_spawn_trigger->propSpawn();
      if(p_spawn_window=GetMainWindow().GetSpawnWindow(prop, state.m_spawn_title, !state.m_no_activity && !state.m_gag, m_raw_log_replay))
      {
         if(prop.fClear())
            p_spawn_window->mp_text->Clear();
         if(prop.fShowTab())
            p_spawn_window->ShowTab();

         // If there is no capture until set, and we have one, set this trigger as the capture until
         if(prop.pclCaptureUntil())
         {
            p_spawn_window->mp_capture_until=state.mp_spawn_trigger;
            mp_captured_spawn_window=p_spawn_window;
            m_timer_spawn_capture.Set(5.0f);
         }
      }
   }
   else if(mp_captured_spawn_window) // If existing capture, set the state spawn window to it
   {
      p_spawn_window=mp_captured_spawn_window;
      state.mp_spawn_trigger=mp_captured_spawn_window->mp_capture_until;

      if(RegEx::Expression *p_regex=state.mp_spawn_trigger->propSpawn().GetCaptureUntilRegEx();p_regex && p_regex->Find(p_line->GetText(), 0).has_value())
         KillSpawnCapture();
   }

   // Spawns have a separate gag from log
   state.m_gag_log|=state.mp_spawn_trigger && state.mp_spawn_trigger->propSpawn().fGagLog();
   if(!state.m_gag_log)
   {
      if(IsLogging())
         LogTextLine(*p_line);

      if(mp_puppet && mp_puppet->GetConnectionMaster().IsLogging() && mp_puppet->GetPuppet()->fCharacterLog())
      {
         auto line=Text::Line::CreateFromText(mp_puppet->GetPuppet()->pclCharacterLogPrefix());
         line->InsertLine(line->GetText().Length(), *p_line);
         mp_puppet->GetConnectionMaster().LogTextLine(*line);
      }
   }

   if(state.m_gag)
      return;

   // Look for image URLs
   if(!m_raw_log_replay && (m_wnd_main.GetImageWindow() || g_ppropGlobal->fAutoImageViewer()))
   {
      if(auto urls=p_line->GetImageUrls())
      {
         for(auto &url : urls)
         {
            if(url.m_youtube_id || url.m_e621_id || url.m_tenor_id)
               continue;

            auto &image_viewer=m_wnd_main.EnsureImageWindow();
            image_viewer.OpenUrl(std::move(url));
         }
      }
   }

   UniquePtr<Text::Line> p_image_line;

   if(g_ppropGlobal->fInlineImages())
   {
      if(auto urls=p_line->GetImageUrls())
      {
         auto p_banner=MakeCounting<ImageBanner>();
         for(auto &url : urls)
            p_banner->AddURL(std::move(url));

         Text::LineBuilder line_builder;
         line_builder.AppendBanner(p_banner);
         p_image_line=line_builder.Create();
      }
   }

   if(g_ppropGlobal->fParseEmoticons())
   {
      // Parse Emojis
      Emoji &emoji=Emoji::Get();
      unsigned index=0;
      while(Emoji::Result result=emoji.Search(p_line->GetText().WithoutFirst(index)))
      {
         index+=result.m_range.begin;

         if(result.m_range.end-result.m_range.begin) // Delete range?
            p_line->DeleteText(index, result.m_range.end-result.m_range.begin);

         p_line->InsertEmoji(index, result.m_replacement);
         index+=result.m_replacement.Count(); // Skip past the inserted emoji
      }
   }

   if(m_events.Get<Event_Display>())
   {
      m_ievent_prepare.Event_Prepare();

      Event_Display event(p_line);
      if(m_events.Send(event, event))
         return;
   }

   RemovePrompt();
   if(!state.m_no_activity)
      m_events.Send(Event_Activity());

   Text::Wnd *p_wnd=&GetOutput();

   // Redirect into spawn window?
   if(p_spawn_window)
   {
      if(state.mp_spawn_trigger && state.mp_spawn_trigger->propSpawn().fCopy())
         GetOutput().Add(MakeUnique<Text::Line>(*p_line));

      p_wnd=p_spawn_window->mp_text;
   }

   p_wnd->Add(std::move(p_line));
   if(p_image_line)
      p_wnd->Add(std::move(p_image_line));
}

bool Connection::KillSpawnCapture()
{
   if(!mp_captured_spawn_window)
      return false;

   if(mp_trigger_debug)
      TriggerDebugText("#000040", "blue", "10", "Spawn Capture Until Hit, Capture Stopped");

   mp_captured_spawn_window->mp_capture_until=nullptr;
   mp_captured_spawn_window=nullptr;
   mp_captureabort_window=nullptr;
   return true;
}


struct Wnd_CaptureAbort : Wnd_Dialog, OwnerPtrData
{
   Wnd_CaptureAbort(Connection &connection) : m_connection{connection}
   {
      // Handle null spawn to main output window
      Window parent=*connection.mp_captured_spawn_window;
      if(!parent)
         parent=connection.GetMainWindow().GetOutputWindow();

      Create("", WS_POPUP|WS_DLGFRAME, 0, parent);
      CenterInWindow(parent);
      EnsureOnScreen();
      Show(SW_SHOW);
   }

private:
   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   Connection &m_connection;
};

LRESULT Wnd_CaptureAbort::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command>(msg);
}

LRESULT Wnd_CaptureAbort::On(const Msg::Create &msg)
{
   m_layout.SetWindowPadding(10*g_dpiScale);
   auto pG=m_layout.CreateGroup_Horizontal(); m_layout.SetRoot(pG);

   auto pbtAbort=m_layout.CreateButton(IDOK, "  Cancel current spawn capture  ");
   pbtAbort->szMinimum().y*=2;
   pG->Add(pbtAbort);

   return msg.Success();
}

LRESULT Wnd_CaptureAbort::On(const Msg::Command &msg)
{
   if(msg.iID()==IDOK)
      m_connection.KillSpawnCapture();

   return msg.Success();
}

void Connection::ShowSpawnCancel()
{
   if(!mp_captured_spawn_window)
      return;

   mp_captureabort_window=MakeUnique<Wnd_CaptureAbort>(*this);
}

Color ColorHash(ConstString text, uint2 range, Array<const uint2> ranges, float brightness)
{
   if(range.end==0) // If whole line, use the first () range if it exists, otherwise the regular range
      range=ranges.Count()>1 ? ranges[1] : ranges[0];

   uint32 hash=Hash_FNV1a_32(text.Sub(range));
   return HSVtoRGB(float3((hash&0xFFF)/4095.0f, ((hash>>12)&0xFFF)/8190.0f+0.5f, brightness));
};

void ApplyParagraphToLine(const Prop::Trigger_Paragraph &p, Text::Line &line, Array<const uint2> ranges)
{
   if(p.fUseIndent_Left())
      line.SetIndentLeft(p.Indent_Left()/100.0f);
   if(p.fUseIndent_Right())
      line.SetIndentRight(p.Indent_Right()/100.0f);
   if(p.fUsePadding_Top())
      line.SetPaddingTop(p.Padding_Top());
   if(p.fUsePadding_Bottom())
      line.SetPaddingBottom(p.Padding_Bottom());
   if(p.fUseBorder())
      line.SetBorder(p.Border());
   if(p.fUseBackgroundColor())
   {
      if(p.fBackgroundHash())
         line.SetParagraphColor(ColorHash(line.GetText(), {}, ranges, 0.5f));
      else
         line.SetParagraphColor(p.clrBackground());
   }
   if(p.fUseStroke())
   {
      if(p.fStrokeHash())
         line.SetParagraphStrokeColor(ColorHash(line.GetText(), {}, ranges, 1.0f));
      else
         line.SetParagraphStrokeColor(p.clrStroke());
      line.SetParagraphStrokeWidth(p.StrokeWidth());
      line.SetStrokeStyle(Text::Records::StrokeStyle(p.StrokeStyle()));
   }
   if(p.fUseBorderStyle())
      line.SetBorderStyle(Text::Records::BorderStyle(p.BorderStyle()));
   if(p.fUseAlignment())
      line.SetAlignment(Text::Records::Alignment(p.Alignment()));
}

void Connection::OnMultilineTriggerTimeout(MultilineTrigger &v)
{
   for(unsigned i=0; i<m_multiline_triggers.Count(); i++)
      if(m_multiline_triggers[i]==&v)
      {
         if(mp_trigger_debug)
            TriggerDebugText("#000040", "blue", "10", "Multiline child triggers hit time limit");
         m_multiline_triggers.Delete(i);
         return;
      }
   Assert(false); // Why didn't we find the trigger?
}

void Connection::RunTriggers(Text::Line &line, Array<CopyCntPtrTo<Prop::Trigger>> triggers, TriggerState &state)
{
   if(state.m_stop)
      return;

   for(auto &ppropTrigger : triggers)
   {
      bool hit=false;
      bool stop=false; // Local variable so that we can apply it to m_stop after we've processed child triggers
      if(!ppropTrigger->fDisabled())
      {
         if(ppropTrigger->fCooldown() && ppropTrigger->m_lastHit.SecondsSinceStart()<ppropTrigger->CooldownTime())
            continue;

         if(ppropTrigger->fAwayPresent() && ( // Is this an away/present trigger?
            // Skip if trigger is for away and we're not away or if the trigger is for away, only once, and we've already notified
            (ppropTrigger->fAway() && !m_away) || (ppropTrigger->fAway() && ppropTrigger->fAwayPresentOnce() && m_away_notified) ||
            // or if the trigger is for present and we're away
            (!ppropTrigger->fAway() && m_away) ) )
            continue;

         FindStringSearch search(ppropTrigger->propFindString(), line);
         while(search.Next())
         {
            hit=true;

            if(mp_trigger_debug)
               TriggerDebugText("#400040", "magenta", "5", HybridStringBuilder("<b>", Text::NoHTML{ppropTrigger->pclDescription()}, "</b> Matcharoo: <b>", Text::NoHTML{ppropTrigger->propFindString().pclMatchText()}));

            if(ppropTrigger->fCooldown())
            {
               ppropTrigger->m_lastHit.Start();
               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", "Starting cooldown");
            }

            static constexpr uint2 c_wholeLine[]={ uint2() };

            // Color
            if(ppropTrigger->fPropColor())
            {
               const Prop::Trigger_Color &color=ppropTrigger->propColor();

               auto ranges=search.ranges();
               if(ranges.Count()>1) // () matches available?
                  ranges=ranges.WithoutFirst(1);
               if(color.fWholeLine())
                  ranges=c_wholeLine;

               for(auto &range : ranges)
               {
                  if(color.fFontDefault())
                     line.SetFontDefault(range);
                  else if(color.FontSize()!=0 && color.pclFontFace())
                     line.SetFont(range, *Text::FontCache::GetInstance().Get(color.pclFontFace(), color.FontSize()));

                  if(color.fFore())
                  {
                     Color text_color;
                     if(color.fForeDefault())
                        text_color=Colors::Foreground;
                     else if(color.fForeHash())
                        text_color=ColorHash(line.GetText(), range, search.ranges(), 1.0f);
                     else
                        text_color=color.clrFore();

                     line.SetColor(range, text_color);
                  }

                  if(color.fBack())
                  {
                     Color text_color;
                     if(color.fBackDefault())
                        text_color=Colors::Transparent;
                     else if(color.fBackHash())
                        text_color=ColorHash(line.GetText(), range, search.ranges(), 0.5f);
                     else
                        text_color=color.clrBack();

                     line.SetBackgroundColor(range, text_color);
                  }
               }
            }

            // Style
            if(ppropTrigger->fPropStyle())
            {
               Prop::Trigger_Style &style=ppropTrigger->propStyle();

               auto ranges=search.ranges();
               if(ranges.Count()>1) // () matches available?
                  ranges=ranges.WithoutFirst(1);
               if(style.fWholeLine())
                  ranges=c_wholeLine;

               for(auto &range : ranges)
               {
                  if(style.fFlash())
                  {
                     if(style.fFlashFast())
                        line.SetFlash(range, 2, 1);
                     else
                        line.SetFlash(range);
                  }

                  if(style.fSetBold())
                     line.SetBold(range, style.fBold());

                  if(style.fSetItalic())
                     line.SetItalic(range, style.fItalic());

                  if(style.fSetUnderline())
                     line.SetUnderline(range, style.fUnderline());

                  if(style.fSetStrikeout())
                     line.SetStrikeout(range, style.fStrikeout());
               }
            }

            // Paragraph
            if(ppropTrigger->fPropParagraph())
               ApplyParagraphToLine(ppropTrigger->propParagraph(), line, search.ranges());

            // Gag
            if(ppropTrigger->fPropGag())
            {
               auto &prop=ppropTrigger->propGag();

               if(prop.fActive())
               {
                  state.m_gag=true;
                  if(mp_trigger_debug)
                     TriggerDebugText("#000040", "blue", "10", "Line gagged");
               }

               if(prop.fLog())
               {
                  state.m_gag_log=true;
                  if(mp_trigger_debug)
                     TriggerDebugText("#000040", "blue", "10", "Line gagged from log");
               }
            }

            // Activate
            if(ppropTrigger->fPropActivate() && !m_raw_log_replay)
            {
               if(ppropTrigger->propActivate().fActive())
               {
                  m_wnd_main.GetMDI().SetActiveWindow(m_wnd_main);
                  if(mp_trigger_debug)
                     TriggerDebugText("#000040", "blue", "10", "Setting window active");
               }

               if(ppropTrigger->propActivate().fImportantActivity())
               {
                  m_wnd_main.AddImportantActivity();
                  if(mp_trigger_debug)
                     TriggerDebugText("#000040", "blue", "10", "Showing as important activity");
               }

               if(ppropTrigger->propActivate().fActivity())
               {
                  state.m_no_activity=false;
                  if(mp_trigger_debug)
                     TriggerDebugText("#000040", "blue", "10", "Showing as activity");
               }

               if(ppropTrigger->propActivate().fNoActivity())
               {
                  state.m_no_activity=true;
                  if(mp_trigger_debug)
                     TriggerDebugText("#000040", "blue", "10", "Not showing as activity");
               }
            }

            // Spawn
            if(ppropTrigger->fPropSpawn())
            {
               auto &prop=ppropTrigger->propSpawn();
               if(prop.fActive() && !mp_captured_spawn_window && !state.mp_spawn_trigger)
               {
                  FindStringReplacement replacement(search, prop.pclTitle());
                  replacement.ExpandVariables(GetVariables());
                  state.m_spawn_title=replacement;
                  state.mp_spawn_trigger=ppropTrigger;

                  if(mp_trigger_debug)
                  {
                     FixedStringBuilder<256> string("Redirecting to spawn window: <b>", Text::NoHTML{replacement}, "</b>");
                     if(prop.pclTabGroup())
                        string(" on tab: <b>", Text::NoHTML{prop.pclTabGroup()});
                     TriggerDebugText("#000040", "blue", "10", string);
                     if(prop.pclCaptureUntil())
                        TriggerDebugText("#000040", "blue", "10", FixedStringBuilder<256>("Capture until set: <b>", Text::NoHTML{prop.pclCaptureUntil()}));
                  }
               }
            }

            // Stat
            if(ppropTrigger->fPropStat())
            {
               auto &prop=ppropTrigger->propStat();

               FindStringReplacement name(search, prop.pclName());
               FindStringReplacement value(search, prop.pclValue());

               if(name) // Only get a stat if it has a non empty name
               {
                  FindStringReplacement title(search, prop.pclTitle());

                  Stats::Wnd &stats=m_wnd_main.GetStatsWindow(title);

                  FindStringReplacement prefix(search, prop.pclPrefix());
                  HybridStringBuilder fullname(prefix, name);
                  Stats::Item &stat=stats.Get(fullname);
                  stat.m_prefix_length=prefix.Count();

                  switch(prop.Type())
                  {
                     case 0:
                     {
                        int int_value;
                        if(value.To(int_value))
                        {
                           if(prop.propInt().fAdd() && stat.m_type.Is<Stats::Types::Int>())
                              stat.m_type.Get<Stats::Types::Int>().m_value+=int_value;
                           else
                              stat.m_type=Stats::Types::Int{int_value};
                        }
                        break;
                     }

                     case 1: stat.m_type=Stats::Types::String{value}; break;

                     case 2:
                     {
                        auto &propRange=prop.propRange();
                        FindStringReplacement lower(search, propRange.pclLower());
                        FindStringReplacement upper(search, propRange.pclUpper());

                        int int_value, int_lower, int_upper;
                        if(value.To(int_value) && lower.To(int_lower) && upper.To(int_upper))
                           stat.m_type=Stats::Types::Range{int_value, int_lower, int_upper, propRange.clrColor()};
                        break;
                     }
                  }

                  if(prop.fUseFont())
                     stat.SetFont(prop.propFont());
                  else
                     stat.SetFont();

                  stat.m_name_color=stat.m_value_color=prop.fUseColor() ? prop.clrColor() : Colors::White;
                  stat.m_name_alignment=Stats::Alignment(prop.NameAlignment());

                  if(mp_trigger_debug)
                     TriggerDebugText("#000040", "blue", "10", "Stat set");
               }
            }

            // Sound
            if(!m_raw_log_replay && ppropTrigger->fPropSound() && ppropTrigger->propSound().fActive())
            {
               if(!m_mute_audio)
                  PlaySound(ppropTrigger->propSound().pclSound());
               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", "Playing sound");
            }

            // Speech
            if(!m_raw_log_replay && ppropTrigger->fPropSpeech() && ppropTrigger->propSpeech().fActive())
            {
               if(!m_mute_audio)
               {
                  if(ppropTrigger->propSpeech().fWholeLine())
                     SAPI::GetInstance().Say(line);
                  else
                     SAPI::GetInstance().Say(FindStringReplacement(search, ppropTrigger->propSpeech().pclSay()));
               }

               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", "Saying text");
            }

            // Send
            if(!m_raw_log_replay && ppropTrigger->fPropSend() && ppropTrigger->propSend().fActive() && ppropTrigger->propSend().pclSend())
            {
               FindStringReplacement sendString(search, ppropTrigger->propSend().pclSend());
               if(ppropTrigger->propSend().fExpandVariables())
                  sendString.ExpandVariables(GetVariables());

               if(ppropTrigger->propSend().fSendOnClick())
               {
                  unsigned capture_index=ppropTrigger->propSend().CaptureIndex();
                  auto range=search.ranges().Count()>capture_index ? search.ranges()[capture_index] : search.ranges()[0];
                  line.SetUrl(range, MakeUnique<Text::Records::URLData>(Text::Records::URLType::Custom, sendString));
               }
               else
               {
                  RestorerOf _(m_in_send); m_in_send=true; // To prevent prompt bugs, as we don't want to receive fake data while sending data
                  m_wnd_main.SendLines(sendString);
                  if(mp_trigger_debug)
                     TriggerDebugText("#000040", "blue", "10", FixedStringBuilder<256>("Sending text: <b>", Text::NoHTML{sendString}));
               }
            }

            // Toast
            if(!m_raw_log_replay && ppropTrigger->fPropToast() && ppropTrigger->propToast().fActive() && m_ppropServer)
            {
               FixedStringBuilder<256> stringTitle("On ", m_ppropServer->pclName());
               if(m_ppropCharacter)
                  stringTitle(" as ", m_ppropCharacter->pclName());

               Toast(line.GetText(), stringTitle, ToastIconInfo);
               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", "System Notification Sent");
            }

            // Filter (do this after all other line changers since it changes the positions of the search results)
            if(ppropTrigger->fPropFilter() && ppropTrigger->propFilter().fActive())
            {
               int searchLength=search.End()-search.Start();

               FindStringReplacement replacement(search, ppropTrigger->propFilter().pclReplace(), ppropTrigger->propFilter().fHTML());
               replacement.ExpandVariables(GetVariables());

               auto p_line=Text::Line::Create(replacement, ppropTrigger->propFilter().fHTML());
               line.InsertLine(search.Start(), *p_line);
               line.DeleteText(search.Start()+p_line->GetText().Count(),searchLength);
               search.HandleReplacement(p_line->GetText().Count(), line);

               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", "Filtering text");
            }

            // Avatar
            if(ppropTrigger->fPropAvatar() && ppropTrigger->propAvatar().pclURL())
            {
               FindStringReplacement replacement(search, ppropTrigger->propAvatar().pclURL());
               replacement.ExpandVariables(GetVariables());

               auto p_image=MakeCounting<ImageAvatar>(Text::ImageURL{replacement});
               line.SetParagraphRecord(Text::Records::ImageLeft(&*p_image));

               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", FixedStringBuilder<256>("Setting Avatar URL:", Text::NoHTML{replacement}));
            }

            // Script
            if(!m_raw_log_replay && ppropTrigger->fPropScript() && ppropTrigger->propScript().fActive() && ppropTrigger->propScript().pclFunction())
            {
               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", "Running script");

               if(Scripter *p_scripter=m_wnd_main.GetScripter())
               {
                  auto p_line=MakeCounting<OM::TextWindowLine>(line);
                  auto p_array=MakeCounting<OM::ArrayUInt>();
                  
                  p_array->m_array.Allocate(search.ranges().Count()*2);

                  for(unsigned i=0; i<search.ranges().Count(); i++)
                  {
                     p_array->m_array[i*2]=search.ranges()[i].begin;
                     p_array->m_array[i*2+1]=search.ranges()[i].end;
                  }

                  p_scripter->Call(OwnedBSTR(ppropTrigger->propScript().pclFunction()), static_cast<IDispatch*>(p_array), static_cast<IDispatch *>(p_line), m_wnd_main.GetDispatch());
               }
            }

            if(mp_trigger_debug)
               mp_trigger_debug->Add(MakeUnique<Text::Line>(line));

            if(ppropTrigger->fAwayPresentOnce())
               m_away_notified=true;

            stop=ppropTrigger->fStopProcessing();
            if(mp_trigger_debug && stop)
               TriggerDebugText("#000040", "blue", "10", "Stopping processing");

            if(ppropTrigger->fOncePerLine())
            {
               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", "Once per line");
               break;
            }
         }
      }

      if(hit || ppropTrigger->fDisabled())
      {
         if(ppropTrigger->fMultiline())
         {
            float time=ppropTrigger->Multiline_Time();
            if(time<=0.0f && ppropTrigger->Multiline_Limit()==0)
            {
               if(mp_trigger_debug)
                  TriggerDebugText("#FF0000", "red", "10", "Multiline can't be activated with no limit and no time");
            }
            else
            {
               auto FindMultiline=[](Collection<UniquePtr<MultilineTrigger>> &list, Prop::Triggers &triggers)
                  {
                     for(unsigned i=0; i<list.Count(); i++)
                        if(list[i]->mp_triggers==&triggers)
                           return i;
                     return ~0U;
                  };

               // Look for existing trigger
               if(auto existing=FindMultiline(m_multiline_triggers, ppropTrigger->propTriggers()); existing!=~0U)
                  m_new_multiline_triggers.Push(m_multiline_triggers.Delete(existing));
               else if(auto existing=FindMultiline(m_new_multiline_triggers, ppropTrigger->propTriggers()); existing!=~0U)
                  m_new_multiline_triggers.Push(m_new_multiline_triggers.Delete(existing));
               else
               {
                  auto multi=MakeUnique<MultilineTrigger>();
                  multi->mp_triggers=&ppropTrigger->propTriggers();
                  m_new_multiline_triggers.Push(std::move(multi));
               }

               auto &multi=*m_new_multiline_triggers.Last();
               multi.m_line_count=0;
               multi.m_line_limit=ppropTrigger->Multiline_Limit();
               if(time>0.0f)
               {
                  multi.m_timeout.SetCallback([this, &multi]() { OnMultilineTriggerTimeout(multi); });
                  multi.m_timeout.Set(time);
               }
               if(mp_trigger_debug)
                  TriggerDebugText("#000040", "blue", "10", "Multiline child triggers activated");
            }
         }
         else
         {
            // Do the nested triggers
            if(ppropTrigger->fPropTriggers() && ppropTrigger->propTriggers().fActive())
               RunTriggers(line, ppropTrigger->propTriggers(), state);
         }
      }

      state.m_stop|=stop;
      if(state.m_stop) // In case someone deeper stopped us
         break;
   }
}

void Connection::ProcessAlias(StringBuilder &string, Prop::Alias &propAlias, Connection::AliasState &state)
{
   bool aliased=false;
   if(!propAlias.fFolder() && propAlias.propFindString().pclMatchText())
   {
      FindStringSearch search(propAlias.propFindString(), string);
      while(search.Next())
      {
         FindStringReplacement replacement(search, propAlias.pclReplace());
         if(propAlias.fExpandVariables())
            replacement.ExpandVariables(state.m_variables);
         string.Replace(search.RangeFound(), replacement);
         search.HandleReplacement(replacement.Length(), string);
         state.m_aliased=true;
         aliased=true;

         if(mp_alias_debug)
         {
            AliasDebugText("#400040", "magenta", "5", HybridStringBuilder("<b>", Text::NoHTML{propAlias.pclDescription()}, "</b> Matcharoo: <b>", Text::NoHTML{propAlias.propFindString().pclMatchText()}));
            mp_alias_debug->Add(Text::Line::CreateFromText(string));
         }
      }
   }

   // Process contained aliases if we have them and then only if we're a folder or this alias hit
   if(propAlias.fPropAliases() && propAlias.propAliases().Count()>0 && (propAlias.fFolder() | aliased))
      ProcessAliases(string, propAlias.propAliases(), state);

   if(aliased && propAlias.fStopProcessing())
      state.m_stop=true;
}

void Connection::ProcessAliases(StringBuilder &string, Array<CopyCntPtrTo<Prop::Alias>> aliases, Connection::AliasState &state)
{
   if(state.m_stop)
      return;

   for(auto &ppropAlias : aliases)
   {
      ProcessAlias(string, *ppropAlias, state);
      if(state.m_stop)
         break;
   }
}

bool Connection::ProcessAliases(StringBuilder &string)
{
   // Don't process Aliases?
   if(!m_propConnections.propAliases().fActive())
      return false;

   AliasState state{GetVariables()};

   if(mp_alias_debug)
   {
      AliasDebugText("#004000", "green", "0", "Starting aliases, original line:");
      mp_alias_debug->Add(Text::Line::CreateFromText(string));
   }

   ProcessAliases(string, m_propConnections.propAliases().Pre(), state);

   if(m_ppropServer)
   {
      ProcessAliases(string, m_ppropServer->propAliases().Pre(), state);

      if(m_ppropCharacter)
         ProcessAliases(string, m_ppropCharacter->propAliases(), state);

      ProcessAliases(string, m_ppropServer->propAliases().Post(), state);
   }

   ProcessAliases(string, m_propConnections.propAliases().Post(), state);

   if(mp_alias_debug)
      AliasDebugText("#400000", "red", "0", "Complete");

   return state.m_aliased;
}

void Connection::On(const Event_NewDay &event)
{
   if(!mp_auto_log || !mp_auto_log->fDateLog())
      return;

   StopAutoLog();
   StartAutoLog();
}

void Connection::StartAutoLog() try
{
   if(mp_auto_log)
      return; // Don't start the autolog if we're already manually logging

   ConstString filename;
   int timeFormat{};

   if(m_ppropPuppet)
   {
      if(m_ppropPuppet->pclLogFileName())
      {
         filename=m_ppropPuppet->pclLogFileName();
         timeFormat=m_ppropPuppet->LogFileNameTimeFormat();
      }
   }
   else if(m_ppropCharacter)
   {
      if(m_ppropCharacter->pclLogFileName())
      {
         filename=m_ppropCharacter->pclLogFileName();
         timeFormat=m_ppropCharacter->LogFileNameTimeFormat();
      }
      else
         filename=g_ppropGlobal->propConnections().propLogging().pclDefaultLogFileName();
   }

   if(filename)
   {
      StopAutoLog();
      mp_auto_log=MakeUnique<Log>(*this, m_propConnections.propLogging(), *this, filename, timeFormat);
      m_events.Send(Event_Log());
   }
} catch(const std::exception &)
{
}

void Connection::LogTyped(ConstString string)
{
   if(mp_auto_log)
      mp_auto_log->LogTyped(string);
   for(auto &p_log : m_logs)
      p_log->LogTyped(string);
}

void Connection::LogSent(ConstString string)
{
   if(mp_auto_log)
      mp_auto_log->LogSent(string);
   for(auto &p_log : m_logs)
      p_log->LogSent(string);
}

void Connection::LogTextLine(Text::Line &line)
{
   if(mp_auto_log)
      mp_auto_log->LogTextLine(line);
   for(auto &p_log : m_logs)
      p_log->LogTextLine(line);
}

void Connection::StartLog(ConstString filename, unsigned type) try
{
   Text::Pauser _(GetOutput()); // Without this, log from window doesn't work due to scrolling down due to the log creation message
   auto &log=*m_logs.Push(MakeUnique<Log>(*this, m_propConnections.propLogging(), *this, filename, 0));
   const Text::Line *p_start{};
   switch(type)
   {
      case ID_LOGGING_FROMNOW:
         break;

      case ID_LOGGING_FROMBEGINNING:
      {
         auto &lines=GetOutput().GetTextList().GetLines();
         if(lines.Count())
            p_start=lines.begin();
         break;
      }

      case ID_LOGGING_FROMWINDOW:
         p_start=GetOutput().GetTopLine();
         break;
   }

   if(p_start)
      log.LogTextList(GetOutput().GetTextList(), p_start);
   m_events.Send(Event_Log());
} catch(const std::exception &)
{
}

void Connection::StopAutoLog()
{
   if(!mp_auto_log)
      return;

   mp_auto_log=nullptr;
   m_events.Send(Event_Log());
}

void Connection::StopLog(Log &log)
{
   if(&log==mp_auto_log)
   {
      StopAutoLog();
      return;
   }

   for(unsigned i=0;i<m_logs.Count();i++)
      if(&log==m_logs[i])
      {
         m_logs.Delete(i);
         m_events.Send(Event_Log());
         return;
      }
   Assert(false); // Log not found? Why?
}

void Connection::StopLogs()
{
   if(!m_logs)
      return;

   m_logs.Empty();
   m_events.Send(Event_Log());
}

void Connection::ReplayRestoreLog()
{
   if(!m_ppropCharacter)
      return;

   // Replay the log if it exists
   auto index=m_ppropCharacter->RestoreLogIndex();
   if(index==-1)
      return;

   RestoreLogReplay replay(*gp_restore_logs, index);
   RestorerOf _(m_raw_log_replay); m_raw_log_replay=true;
   if(m_ppropServer->fPueblo()) // Assume Pueblo is enabled at the beginning
      m_pueblo=true;
#if YARN
   bool pueblo_default=m_ppropServer->pclHost().IStartsWith("yarn://");
#else
   bool pueblo_default=false;
#endif

   uint64 time=0;
   while(auto entry=replay.Read())
   {
      switch(entry.m_type)
      {
         case RestoreLogs::EntryType::Start:
            KillSpawnCapture();
            m_text_to_line.ResetAnsi();
            m_pueblo=pueblo_default;
            break;

         case RestoreLogs::EntryType::Received:
         {
            auto line=entry.GetDataString();
            if(!m_pueblo && m_ppropServer->fPueblo() && line.Find(ConstString("</xch_mudtext>"))!=Strings::Result::Not_Found)
               m_pueblo=true;
            auto pTL=m_text_to_line.Parse(line, m_pueblo, m_ppropServer->eEncoding());
            time=entry.m_time;
            pTL->Time()=Time::Time{Time::UTCToLocal(time)};

            Display(std::move(pTL));
            break;
         }

         case RestoreLogs::EntryType::Received_GMCP:
         {
            OnGMCP_Parse(entry.GetDataString());
            break;
         }

         case RestoreLogs::EntryType::Sent:
         {
            m_wnd_main.History_AddToHistory(entry.GetDataString(), ConstString(), time);

            if(auto &props=m_wnd_main.GetInputWindow().GetProps();props.fLocalEcho())
               Text(entry.GetDataString(), props.clrLocalEchoColor());
            break;
         }
      }
   }

   KillSpawnCapture(); // Just in case we were left mid capture after the replay, as this will always be wrong after connecting again
   m_text_to_line.ResetAnsi();
   m_pueblo=pueblo_default;

   Time::File deltaTime(Time::System().GetFileTime()-time);

   HybridStringBuilder string("<p border='5' padding='5' stroke-color='#B90000' stroke-width='2' stroke-style='top' align='center'><font color='#B90000' face='arial'><b>Content restored — Last entry ");
   if(time==0)
      string("(no last entry)");
   else
   {
      Time::SecondsToStringAbbreviated(string, deltaTime.ToSeconds());
      string(" ago</b>");
   }
   m_wnd_main.BroadcastHTML(string);
   m_wnd_main.OnReplayComplete();
}

bool Connection::HasMudPrompt() const
{
   return m_ppropServer && m_ppropServer->fPrompts() && m_telnet_parser.HasPartial();
}

void Connection::OnPromptTimer()
{
   if(m_telnet_parser.HasPartial())
      OnPrompt(m_telnet_parser.GetPartial());
}

void Connection::OpenNetworkDebugWindow()
{
   if(mp_network_debug)
   {
      mp_network_debug->InsertAfter(HWND_TOP);
      return;
   }

   mp_network_debug=MakeUnique<Text::Wnd>(nullptr, m_network_debug_host);

   Prop::TextWindow &prop=g_ppropGlobal->propWindows().propMainWindowSettings().propOutput();
   mp_network_debug->SetFont(prop.propFont().pclName(), prop.propFont().Size(), prop.propFont().CharSet());
   mp_network_debug->SetSize(uint2(640,480));

   FixedStringBuilder<256> string("Network Debugger - "); GetWorldTitle(string, 0);
   mp_network_debug->SetText(string);
   mp_network_debug->Show(SW_SHOWNOACTIVATE);
   mp_network_debug->AddHTML("Showing network activity, right click for options");
}

void Connection::OpenTriggerDebugWindow()
{
   if(mp_trigger_debug)
   {
      mp_trigger_debug->InsertAfter(HWND_TOP);
      return;
   }

   static Text::IHost s_dummyHost;
   mp_trigger_debug=MakeUnique<Text::Wnd>(nullptr, s_dummyHost);

   Prop::TextWindow &prop=g_ppropGlobal->propWindows().propMainWindowSettings().propOutput();
   mp_trigger_debug->SetFont(prop.propFont().pclName(), prop.propFont().Size(), prop.propFont().CharSet());
   mp_trigger_debug->SetSize(uint2(640,480));

   FixedStringBuilder<256> string("Trigger Debugger - "); GetWorldTitle(string, 0);
   mp_trigger_debug->SetText(string);
   mp_trigger_debug->Show(SW_SHOWNOACTIVATE);
}

void Connection::OpenAliasDebugWindow()
{
   if(mp_alias_debug)
   {
      mp_alias_debug->InsertAfter(HWND_TOP);
      return;
   }

   static Text::IHost s_dummyHost;
   mp_alias_debug=MakeUnique<Text::Wnd>(nullptr, s_dummyHost);

   Prop::TextWindow &prop=g_ppropGlobal->propWindows().propMainWindowSettings().propOutput();
   mp_alias_debug->SetFont(prop.propFont().pclName(), prop.propFont().Size(), prop.propFont().CharSet());
   mp_alias_debug->SetSize(uint2(640, 480));

   FixedStringBuilder<256> string("Alias Debugger - "); GetWorldTitle(string, 0);
   mp_alias_debug->SetText(string);
   mp_alias_debug->Show(SW_SHOWNOACTIVATE);
}

void Connection::ShowConnectionInfo()
{
   if(!mp_client)
   {
      GetOutput().AddHTML("<icon error> No connection");
      return;
   }

   HybridStringBuilder<1024> string;
   mp_client->GetConnectionInfo(string);
   if(string)
      GetOutput().AddHTML(string);
   else
      GetOutput().AddHTML("<icon information> Connection is not secure");
}

Text::Wnd &Connection::GetOutput()
{
   return m_wnd_main.GetOutputWindow();
}

//
// Puppet
//

Puppet::Puppet(Prop::Puppet *ppropPuppet, Connection *pConnectionMaster, Connection *pConnectionPuppet) noexcept
: mp_prop_puppet(ppropPuppet), mp_connection_master(pConnectionMaster), mp_connection_puppet(pConnectionPuppet)
{
}

Puppet::~Puppet()
{
}

void Puppet::Connect(const Sockets::Address &address, ConstString serverName, bool fVerifyCertificate)
{
   Assert(0);
}

void Puppet::Disconnect()
{
   Unlink(); // Remove us from puppet list in master connection
   // Should only be called by the master connection

   Connection *pC=mp_connection_puppet;
   mp_connection_puppet=nullptr;
   pC->Disconnected(0);
}

void Puppet::Input(Array<const BYTE> data)
// Input from the text window going to the server
{
   // For every newline, send another pclSendPrefix
   ConstString stringData((const char *)data.begin(), data.Count());

   if(mp_prop_puppet->fRemoveAccidentalPrefix() && stringData.StartsWith(mp_prop_puppet->pclSendPrefix()))
      stringData=stringData.WithoutFirst(mp_prop_puppet->pclSendPrefix().Count());

   HybridStringBuilder<> string(mp_prop_puppet->pclSendPrefix(), stringData);
   mp_connection_master->Send(string);
}

bool Puppet::fPuppetRegEx(UniquePtr<Text::Line> &&p_line)
{
   auto range=FindRegEx(*mp_prop_puppet, *p_line);
   if(range.end==0)
      return false;

   if(mp_prop_puppet->fHideReceivePrefix())
      p_line->DeleteText(range.begin, range.end-range.begin);

   mp_connection_puppet->Display(std::move(p_line));
   return true;
}

uint2 Puppet::FindRegEx(const Prop::Puppet &propPuppet, ConstString string)
{
   if(!propPuppet.pclReceivePrefix())
      return {}; // An empty receive prefix?  That would catch everything! Give up now instead

   if(!propPuppet.fRegularExpression())
   {
      if(string.IStartsWith(propPuppet.pclReceivePrefix()))
         return uint2(0, propPuppet.pclReceivePrefix().Count());
      return {};
   }

   if(!propPuppet.mp_regex_cache)
      propPuppet.mp_regex_cache=MakeUnique<RegEx::Expression>(propPuppet.pclReceivePrefix(), PCRE2_UTF);

   FixedArray<uint2, 15> ranges_buffer;
   auto ranges=propPuppet.mp_regex_cache->Find(string, 0, ranges_buffer);
   if(ranges.Count()==0)
      return {};

   if(ranges.Count()>1)
      return ranges[1];
   return ranges[0];
}
