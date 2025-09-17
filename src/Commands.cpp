//
// Command line interface
//

#include "Main.h"
#include "Connection.h"
#include "Wnd_Taskbar.h"
#include "Wnd_Main.h"
#include "Scripter.h"
#include "Automation.h"
#include "MCP.h"
#include "Wnd_MuckNet.h"
#include "Sounds.h"
#include "CodePages.h"
#include "XML.h"
#include "Wnd_Map.h"
#include "ImageBanner.h"
#include "WebView.h"

void StopSpeech();

bool ParseFlag(ConstString value)
{
   if(IEquals(value, "false") || IEquals(value, "f"))
      return false;
   else if(IEquals(value, "true") || IEquals(value, "t"))
      return true;
   else
      throw std::runtime_error{"Value is not t/f or true/false"};
}

struct WordList : Collection<ConstString>
{
   WordList(ConstString string);
};

WordList::WordList(ConstString string) : Collection<ConstString>(5)
{
   for(unsigned i=0;i<string.Length();i++)
   {
      char c=string[i];
      if(c==' ') // Skip over whitespace between words
         continue;

      char delimeter=' ';
      if(c=='\'' || c=='"' || c=='`')
      {
         delimeter=c;
         if(++i==string.Length())
            return;
      }

      unsigned index=string.FindFirstAt(delimeter, i, string.Length());
      Push(string.Sub(i, index));
      i=index;
   }
}

// Given an input string with wildcards, turns them into Prop::FindString settings
bool StringToFindString(ConstString word, Prop::FindString &fs) noexcept
{
   if(word.StartsWith('*')) // End of Line?
   {
      fs.fEndsWith(true); word=word.WithoutFirst(1);
   }
   if(word.EndsWith('*')) // Start of Line?
   {
      fs.fStartsWith(true); word=word.WithoutLast(1);
   }

   if(!word)
      return false;

   fs.pclMatchText(word);

   // Wildcards at start and end means anywhere again... (so it's useless to type, but still legal)
   if(fs.fStartsWith() && fs.fEndsWith())
   {
      fs.fStartsWith(false);
      fs.fEndsWith(false);
   }
   return true;
}

// Parse a time string with a time suffix and return the result in seconds (ex.. 5s = 5s, 10m = 600s)
bool ParseTimeInSeconds(ConstString word, float &seconds)
{
   if(!word)
      return false;

   float multiplier=1; // Assume seconds
   if(!IsNumber(word.Last()))
   {
      switch(word.Last())
      {
         case 's': multiplier=1; break;
         case 'm': multiplier=60; break;
         case 'h': multiplier=60*60; break;
         default:
            return false;
      }
      word=word.WithoutLast(1);
   }

   bool range=false;
   float end_range=0;
   ConstString end_string;
   if(word.Split('-', word, end_string))
   {
      if(!end_string.To(end_range))
         return false;
      range=true;
   }

   if(!word.To(seconds))
      return false;

   if(range)
   {
      if(end_range<seconds)
         return false;
      seconds=g_random(seconds, end_range);
   }

   seconds*=multiplier;
   return true;
}

void Wnd_Main::ParseCommand(ConstString full_line)
try
{
   ConstString command_line=full_line.WithoutFirst(1);
   if(command_line.First()=='/') // Non command pass through?
      return SendLine(command_line);

   // See if the script wants to handle it
   if(m_events.Get<Event_Command>())
   {
      SetScripterWindow();

      ConstString command=command_line, parameters;
      command_line.Split(' ', command, parameters);
      Event_Command event(command, parameters);
      if(m_events.Send(event, event))
         return;
   }

   bool verbose=true;
   if(auto result=command_line.RightOf("silent/"))
   {
      command_line=result;
      verbose=false;
   }

   WordList wl(command_line);

   ConstString command;
   if(wl)
      command=wl[0];

   // Help!
   if(IEquals(command, "help") || IEquals(command, "?"))
   {
      if(wl.Count()==2)
      {
         OpenHelpURL(FixedStringBuilder<256>("CommandLine.md#", wl[1]));
         return;
      }

      static constexpr ConstString c_help_page[]={
         "",
         "<p align='center'><font color='silver'><b><u>  " gd_pcTitle " - Command Line Help  </u></b>",
         "/@ $ - Run an immediate script, $ can span multiple lines of text",
         "/silent/(command) - Prefix for any command that will suppress informational messages from it",
         "",
         "/ansireset - Reset ansi state to default (useful if the server misbehaves and leaves a style set)",
         "/autolog - If one is setup and was stopped, this restarts the autolog",
         "/capturecancel - Cancel any spawn capture in the current window",
         "/chars - List characters for the current server",
         "/clear - Clears the Output window",
         "/close - Close the current tab",
         "/connect ($)/$ - Connect to a server",
         "/connectioninfo - Shows connection information (like socket security information)",
         "/debugaliases - Opens up an alias debug window",
         "/debugnetwork - Opens up a network debug window",
         "/debugtriggers - Opens up a trigger debug window",
         "/delay - Runs given commands after given given number of seconds",
         "/disconnect - Disconnect from the current server",
         "/echo <on/off> - Command Echo",
         "/exit - Closes all windows and exits the app (same as File->Exit)",
         "/gag $ - Gag incoming lines that contain a string",
         "/gmcp $ - Generic MUD Communcation Protocol",
         "/mcmp $ - Mud Client Media Protocol",
         "/grab $/$ - Use @pemit to grab a property for easy editing",
         "/help ($) - This help list, if a parameter given it opens the web help for that command",
         "/idle # $ - Idle Message, entered after a specified time of idling in minutes",
         "/log $ - Log incoming text into a file",
         "/logall $ - Same as log, but includes all previously received text",
         "/logtop $ - Same as log, but from the top of the current window",
         "/stoplogs - Stop all logs",
         "/map_addroom - Add a room to the map (see online help)",
         "/map_addexit - Add an exit to the map (see online help)",
         "/map_guesslocation - Try to figure out where the player is on the map based on recent scrollback",
         "/naws - Send telnet NAWS",
         "/new - Open a new window",
         "/newtab - Open a new tab in the current window",
         "/newedit - Open a new edit window (see help for parameters)",
         "/newinput (/unique) ($)- Open a new input window with an optional prefix",
         "/opendialog (aliases/triggers/macros/worlds/settings) (parameters...) - Open a built in dialog",
         "/ping $ - Sends the given text to the server and times how long before a response is received",
         "/printenv - Print environment variables",
         "/puppet $ - Open a new window and connect through the given puppet",
         "/puppets - Show puppets for the current character",
         "/repeat # $ - Repeats a command multiple times",
         "/recall # $ - Search output buffer for a string",
         "/receive $ - Acts like whatever text after \"/receive \" was received from the server (plus a cr-lf after)",
         "/reconnect $ - Reconnects if disconnected (with 'all' parameter it applies to every window)",
         "/resetscript - Reset the scripting engine (possibly switching languages)",
         "/roll [count]d[sides](+bonus) - Dice roll. Example /roll 10d6",
         "/script $ - Run single line script",
         "/set $=$ - Set environment variable",
         "/setinput $ - Sets any text after the \"/setinput \" into the active input window",
         "/silence - Stop all playing sounds",
         "/slist - List all worlds",
         "/stats - List connection statistics",
         "/switchtab <tab group> <tab name> - Brings 'tab name' to top in the 'tab group' spawn window",
         "/tabcolor <color> - Sets the tab to the given HTML style color (#RRGGBB or a name)",
         "/ttype - Set Telnet Terminal Type (Default = \"Beip\")",
         "/unset $ - Delete an environment variable",
         "/wall $ - Send text to all connected windows",
         "",
         "<p align='center'>Type '/help &lt;command&gt;' to open up the web help for a particular command",
         "<p align='center'><A href='" HELP_URL "CommandLine.md'>Click here for the online version with more details</a>",
      };

      for(auto &line : c_help_page)
         mp_wnd_text->AddHTML(line);

      return;
   }

   if(IEquals(command, "makali"))
   {
      mp_wnd_text->AddHTML("<font color='#6314C2'>Even a man who is pure of heart," CRLF
                          "and says his prayers by night," CRLF
                          "may become a wolf when the wolfsbane blooms," CRLF
                          "and the autumn moon is bright.");
      return;
   }

   if(IEquals(command, "lizards"))
   {
      mp_wnd_text->AddHTML("<u>And now some messages from our Lizard supporters:</u>" CRLF
         "<font color='lime'>Cam-a-cam-mal, Pria-toi, Gan delah</font><font color='white'> - Snowglass</font>" CRLF
         "<font color='lime'>kweh</font><font color='white'> - Thistle</font>" CRLF
         "<font color='lime'>🦌 ☀️ ☀️ 🚙</font>" CRLF /* DeerInHeadlights */
         "🦎 🦎");
      return;
   }

   


   // Clear
   if(IEquals(command, "clear"))
   {
      mp_wnd_text->Clear();
      return;
   }

   if(IEquals(command, "webview"))
   {
      struct Options : XMLElement
      {
         void OnAttribute(ConstString name, ConstString value) override
         {
            if(IEquals(name, "url"))
            {
               url=value;
            }
            else if(IEquals(name, "source"))
            {
               source=value;
            }
            else if(IEquals(name, "position"))
            {
               int2 pos, size;

               ConstString left, top, width, height;
               if(value.Split(',', left, value) && value.Split(',', top, value) && value.Split(',', width, height) &&
                  left.To(pos.x) && top.To(pos.y) && width.To(size.x) && height.To(size.y))
               {
                  rect=Rect(pos, pos+size);
               }
               else
                  throw std::runtime_error{"Invalid rect"};
            }
            else if(IEquals(name, "state"))
            {
               if(IEquals(value, "maximized"))
                  show_command=SW_SHOWMAXIMIZED;
            }
            else
               throw std::runtime_error{FixedStringBuilder<256>("Unknown attribute: ", name).Terminate()};
         }

         OwnedString url, source;
         Rect rect;
         int show_command{SW_NORMAL};
         
      } options;

      if(command_line.Length()>command.Length()+1)
      {
         auto attributes=command_line.WithoutFirst(command.Length()+1);
         ParseXMLAttributes(attributes, options);
      }

      auto *p_view=new Wnd_WebView(*this);
      if(options.rect.size()!=int2())
      {
         p_view->SetPosition(options.rect);
         p_view->EnsureOnScreen();
      }
      if(options.show_command!=SW_NORMAL)
         p_view->Show(options.show_command);

      if(options.source)
         p_view->SetSource(options.source);
      else
         p_view->SetURL(options.url);
      return;
   }

   if(IEquals(command, "printenv"))
   {
      auto &variables=mp_connection->GetVariables();
      mp_wnd_text->AddHTML(FixedStringBuilder<256>("Environment Variables: (", variables.Count(), ")"));

      HybridStringBuilder string;
      for(auto &variable : variables)
      {
         string.Clear();
         string(variable->pclName(), "=", variable->pclValue());
         mp_wnd_text->Add(Text::Line::CreateFromText(string));
      }

      mp_wnd_text->AddHTML(FixedStringBuilder<256>("ℹ️ End of environment variables"));
      return;
   }

   if(IEquals(command, "set"))
   {
      ConstString ignore, params;
      ConstString name, value;

      if(!command_line.Split(' ', ignore, params) || !params.Split('=', name, value))
      {
         if(verbose)
            mp_wnd_text->AddHTML("<font color='red'>Syntax error, missing '='");
         return;
      }

      auto &variables=mp_connection->GetVariables();
      unsigned index=variables.Find(name);
      if(index==~0U)
      {
         variables.Add(name, value);
         if(verbose)
            mp_wnd_text->AddHTML("<font color='aqua'>New variable added");
      }
      else
      {
         variables[index]->pclValue(value);
         if(verbose)
            mp_wnd_text->AddHTML("<font color='aqua'>Existing variable set to new value");
      }
      return;
   }

   if(IEquals(command, "unset"))
   {
      ConstString ignore, name;
      if(!command_line.Split(' ', ignore, name))
      {
         mp_wnd_text->AddHTML("<font color='red'>Error, missing variable name");
         return;
      }

      auto &variables=mp_connection->GetVariables();
      unsigned index=variables.Find(name);
      if(index==~0U)
      {
         mp_wnd_text->AddHTML("<font color='red'>Variable not found");
         return;
      }

      variables.UnsortedDelete(index);
      if(verbose)
         mp_wnd_text->AddHTML("<font color='aqua'>Variable deleted");
      return;
   }

   // Echo
   if(IEquals(command, "echo"))
   {
      if(wl.Count()<=2)
      {
         bool fLocalEcho=m_input.GetProps().fLocalEcho();

         if(wl.Count()==1)
         {
            if(!fLocalEcho)
            {
               m_input.GetProps().fLocalEcho(true);
               if(verbose)
                  mp_wnd_text->AddHTML("<font color='green'>Echo turned ON");
            }
            return;
         }

         ConstString state=wl[1];

         if(IEquals(state, "on"))
         {
            if(!fLocalEcho)
            {
               m_input.GetProps().fLocalEcho(true);
               if(verbose)
                  mp_wnd_text->AddHTML("<font color='green'>Echo turned ON");
            }
            return;
         }

         if(IEquals(state, "off"))
         {
            if(fLocalEcho)
            {
               m_input.GetProps().fLocalEcho(false);
               if(verbose)
                  mp_wnd_text->AddHTML("<font color='red'>Echo turned OFF");
            }
            return;
         }
      }

      // Invalid parameter
      mp_wnd_text->AddHTML("<font color='aqua'>Usage: '/echo <ON/off>' Without on/off default is ON.");
      return;
   }

   if(IEquals(command, "repeat"))
   {
      if(wl.Count()==3)
      {
         unsigned count;
         if(wl[1].To(count))
         {
            for(unsigned i=0;i<count;i++)
               SendLines(wl[2]);
            return;
         }
      }

      mp_wnd_text->AddHTML("<font color='aqua'>Usage: '/repeat {count} {command}' (note that command should be in quotes if it's more than 1 word)");
      return;
   }

   if(IEquals(command, "debugtimers"))
   {
      HybridStringBuilder string;
      Time::TimerQueue::GetInstance().DebugInfo(string);
      mp_wnd_text->AddHTML(string);
      return;
   }

   if(IEquals(command, "delay"))
   {
      if(wl.Count()>=2)
      {
         if(IEquals(wl[1], "list"))
         {
            mp_wnd_text->AddHTML("<icon information> <font color='teal'><u>Listing delay actions:</font>");
            for(auto &v : m_delay_timers)
            {
               FixedStringBuilder<256> string("<font color='teal'><b>Hits in:</b>"); Time::SecondsToStringAbbreviated(string, v.TimeToHit());
               string("  <b>ID:</b>", v.m_id, "  <b>Send:</b>", Strings::EscapedString(v.m_string));
               mp_connection->Text(string);
            }
            mp_wnd_text->AddHTML("<icon information> <font color='teal'><u>Done listing delay actions</font>");
            return;
         }
         if(IEquals(wl[1], "killall"))
         {
            if(verbose)
               mp_wnd_text->AddHTML("<icon information> <font color='aqua'>All pending timers erased</font>");
            m_delay_timers.Empty();
            return;
         }
         if(IEquals(wl[1], "kill"))
         {
            unsigned id;
            if(wl.Count()<=2 || !wl[2].To(id))
            {
               mp_wnd_text->AddHTML("<icon error> Bad ID format, should be like: kill 43");
               return;
            }

            for(auto &v : m_delay_timers)
            {
               if(v.m_id==id)
               {
                  if(verbose)
                     mp_wnd_text->AddHTML("<icon information> Timer killed");
                  delete &v;
                  return;
               }
            }

            mp_wnd_text->AddHTML("<icon error> Timer ID not found");
            return;
         }

         unsigned wl_index=1;
         bool repeating=false;

         if(IEquals(wl[wl_index], "every"))
         {
            repeating=true;
            wl_index++;
         }

         float seconds{};
         if(ParseTimeInSeconds(wl[wl_index++], seconds) && wl.Count()==wl_index+1)
         {
            auto &timer=*new DelayTimer(*this, wl[wl_index], seconds, repeating);
            if(verbose)
            {
               FixedStringBuilder<256> string("<icon information> <font color='green'>Starting timer with ID:", timer.m_id, " in ");
               if(unsigned frac;seconds<60.0f && (frac=unsigned(seconds*100)%100))
                  string(int(seconds), '.', Strings::Int(frac, 2, '0'), "s");
               else
                  Time::SecondsToStringAbbreviated(string, seconds);
               string("</font>");
               mp_wnd_text->AddHTML(string);
            }
            return;
         }
      }

      mp_wnd_text->AddHTML("<icon information> Invalid usage, try '/help delay' to see help for this command");
      return;
   }

   // Idle Timer
   if(IEquals(command, "idle"))
   {
      if(!mp_connection->IsConnected())
      {
         mp_wnd_text->AddHTML("<icon error> <font color='red'>Must be connected to work");
         return;
      }

      if(wl.Count()==1)
      {
         if(m_idle_timer)
         {
            m_idle_timer.Reset();
            m_idle_string.Clear();
            if(verbose)
               mp_wnd_text->AddHTML("<icon information> <font color='aqua'>Idle timer removed");

            if(auto *p_character=mp_connection->GetCharacter())
               p_character->fIdleEnabled(false);
         }
         else if(verbose)
            mp_wnd_text->AddHTML("<icon information> No idle timer set");
         return;
      }

      if(wl.Count()==3)
      {
         ConstString time=wl[1];
         if(!time.To(m_idle_delay))
         {
            mp_wnd_text->AddHTML("<icon error> <font color='red'>Bad time");
            return;
         }

         PinAbove(m_idle_delay, 1U); // Must be at least 1 minute!
         m_idle_string=wl[2];
         m_idle_timer.Set(m_idle_delay*60.0f, true);
         if(verbose)
            mp_wnd_text->AddHTML(FixedStringBuilder<256>("<icon information> <font color='aqua'>Idle timer activated, sending <font color='white'>", m_idle_string, "</font> every ", m_idle_delay, " minutes"));

         if(auto *p_character=mp_connection->GetCharacter())
         {
            p_character->fIdleEnabled(true);
            p_character->IdleTimeout(m_idle_delay);
            p_character->pclIdleString(m_idle_string);
         }
         return;
      }

      // Invalid parameter
      mp_wnd_text->AddHTML("<icon information> <font color='aqua'>Usage:'/idle &lt;time in minutes&gt; &lt;string:Command to enter&gt;', no parameters turns it off</font>");
      return;
   }

   // Log
   if(IEquals(command, "log"))
   {
      if(wl.Count()!=2)
      {
         mp_wnd_text->AddHTML("<icon information> <font color='aqua'>Usage: '/log &lt;filename&gt;'</font>");
         return;
      }

      mp_connection->StartLog(wl[1], ID_LOGGING_FROMNOW);
      return;
   }

   if(IEquals(command, "stoplogs"))
   {
      mp_connection->StopLogs();
      return;
   }

   if(IEquals(command, "logall"))
   {
      if(wl.Count()!=2)
      {
         mp_wnd_text->AddHTML("<icon information> <font color='aqua'>Usage: '/logall &lt;filename&gt;', '/log' stops the log");
         return;
      }

      mp_connection->StartLog(wl[1], ID_LOGGING_FROMBEGINNING);
      return;
   }

   if(IEquals(command, "logtop"))
   {
      if(wl.Count()!=2)
      {
         mp_wnd_text->AddHTML("<icon information> <font color='aqua'>Usage: '/logtop &lt;filename&gt;', '/log' stops the log");
         return;
      }

      mp_connection->StartLog(wl[1], ID_LOGGING_FROMWINDOW);
      return;
   }

   if(IEquals(command, "autolog"))
   {
      if(mp_connection->GetAutoLog())
      {
         mp_wnd_text->AddHTML("<icon error> <font color='teal'>Log already running");
         return;
      }

      mp_connection->StartAutoLog();
      if(!mp_connection->GetAutoLog())
         mp_wnd_text->AddHTML("<icon information> No autolog setup");
      return;
   }

   if(IEquals(command, "ping"))
   {
      if(command_line.Length()>command.Length()+1)
         mp_connection->Send(command_line.WithoutFirst(command.Length()+1));
      else
         mp_connection->Send({}); // Send a blank line
      mp_connection->StartPing();
      return;
   }

   if(IEquals(command, "opendialog"))
   {
      if(wl.Count()>=2)
      {
         if(IEquals(wl[1], "aliases"))
            Msg::Command(ID_ALIASES, nullptr, 0).Post(GetMDI());
         else if(IEquals(wl[1], "triggers"))
            Msg::Command(ID_TRIGGERS, nullptr, 0).Post(GetMDI());
         else if(IEquals(wl[1], "macros"))
            Msg::Command(ID_MACROS, nullptr, 0).Post(GetMDI());
         else if(IEquals(wl[1], "worlds"))
            Msg::Command(ID_CONNECTION_CONNECT, nullptr, 0).Post(GetMDI());
         else if(IEquals(wl[1], "settings"))
            CreateDialog_Settings(*this, wl.Count()==3 ? wl[2] : ConstString());
         else if(IEquals(wl[1], "about"))
            CreateWindow_About(*this);
         else
            mp_wnd_text->AddHTML("<icon error> <font color='red'>Unknown dialog");
      }
      else
         mp_wnd_text->AddHTML("<icon error> <font color='red'>Usage: '/opendialog (aliases/triggers/macros/worlds/about)'");
      return;
   }

   // Server List
   if(IEquals(command, "slist"))
   {
      mp_wnd_text->AddHTML("<p background-color='#004000' stroke-color='green' stroke-width='2' border-style='round' align='center'><b>Server List");

      for(auto &pServer : g_ppropGlobal->propConnections().propServers())
      {
         FixedStringBuilder<256> string("<font color='#8080FF'>", pServer->pclName(), "  - <font color='silver'><b>Server:</b><font color='aqua'>",
                 (pServer->pclHost() ? ConstString{pServer->pclHost()} : ConstString{"<font color='red'>(Empty Hostname:port)"}));
         mp_wnd_text->AddHTML(string);
      }

      mp_wnd_text->AddHTML("<p background-color='#004000' stroke-color='green' stroke-width='2' border-style='round' align='center'> ");

      return;
   }

   if(IEquals(command, "stats"))
   {
      ShowStatistics();
      return;
   }

   if(IEquals(command, "script"))
   {
      auto *p_scripter=GetScripter();
      if(!p_scripter)
      {
         mp_wnd_text->AddHTML("<font color='red'>Can't run, no scripting engine");
         return;
      }

      if(command_line.Length()<=command.Length()+1)
      {
         mp_wnd_text->AddHTML("<font color='red'>Can't run, no script code given");
         return;
      }

      p_scripter->Run(OwnedBSTR(command_line.WithoutFirst(command.Length()+1)));
      return;
   }

   if(IEquals(command, "resetscript"))
   {
      if(Scripter::HasInstance())
      {
         delete &Scripter::GetInstance();
         mp_wnd_text->AddHTML("<font color='green'>Scripting Engine Reset");
      }
      else
         mp_wnd_text->AddHTML("<font color='green'>Can't reset scripting engine as it's not running");
      return;
   }

   // Recall
   if(IEquals(command, "recall"))
   {
      if(wl.Count()!=3)
      {
         mp_wnd_text->AddHTML("<font color='aqua'><b>Usage:</b>/recall (# of lines to go back) (text to search for)");
         return;
      }

      unsigned lineCount;
      if(!wl[1].To(lineCount))
      {
         mp_wnd_text->AddHTML("<font color='red'><b>Recall:</b>Could not understand number of lines entered.");
         return;
      }

      Prop::FindString fs;
      if(!StringToFindString(wl[2], fs))
      {
         mp_wnd_text->AddHTML("<font color='red'><b>Recall:</b>Invalid Search String");
         return;
      }

      Collection<UniquePtr<Text::Line>> stkTLine;

      const Text::Lines &lines=mp_wnd_text->GetTextList().GetLines();
      PinBelow(lineCount, lines.Count()-1);

      // We search the lines in reverse order, since we're putting them on a stack
      auto pEnd=lines.middle(lines.Count()-lineCount-1);
      for(auto pLine=--lines.end();pLine!=pEnd;--pLine)
      {
         uint2 rangeFound;
         if(fs.Find(pLine->GetText(), 0, rangeFound))
            stkTLine.Push(new Text::Line(*pLine));
      }

      mp_wnd_text->AddHTML("<font color='aqua'><b>Recall</b> - <font color='green'><u>Starting");

      // Take the lines we found and send them to the text window
      while(stkTLine)
         mp_wnd_text->Add(stkTLine.Pop());

      mp_wnd_text->AddHTML("<font color='aqua'><b>Recall</b> - <font color='red'>Finished");
      return;
   }

   if(IEquals(command, "grab"))
   {
      if(wl.Count()!=2)
      {
         mp_wnd_text->AddHTML("<font color='aqua'><b>Usage:</b>'/grab (object)/(property to grab)' - On a server with @pemit support, will grab the property on the object and send it to the input window for easy editing");
         return;
      }

      uint32 key=Time::QueryPerformanceCounter();
      FixedStringBuilder<16> prefix{Strings::Hex32(key, 8), ' '};
      mp_connection->m_grab_prefix=prefix;

      ConstString object, property;
      if(!wl[1].Split('/', object, property))
      {
         mp_wnd_text->AddHTML("<font color='red'><b>Grab:</b>Missing / in param, type /grab for help");
         return;
      }

      FixedStringBuilder<256> command{"@pemit me=", prefix, "&", property, ' ', object, "=[get(", object, '/', property, ")]"};
      mp_connection->Send(command);
      return;
   }

   // Gag
   if(IEquals(command, "gag"))
   {
      if(wl.Count()!=2)
      {
         mp_wnd_text->AddHTML("<font color='aqua'><b>Usage:</b>/gag (text to search for)");
         return;
      }

      Prop::FindString fs;
      if(!StringToFindString(wl[1], fs))
      {
         mp_wnd_text->AddHTML("<font color='red'><b>Gag:</b>Invalid Gag String");
         return;
      }

      Prop::Triggers *ppropTriggers=&g_ppropGlobal->propConnections().propTriggers();

      unsigned trigger=ppropTriggers->FindFirstIf([&](const Prop::Trigger *pTrigger) { return pTrigger->propFindString()==fs; });
      if(trigger==~0U)
      {
         auto ppropTrigger=MakeCounting<Prop::Trigger>();
         ppropTrigger->propFindString()=std::move(fs);
         trigger=0;
         ppropTriggers->Insert(0, std::move(ppropTrigger));
      }
      else if(verbose)
         mp_wnd_text->AddHTML("<font color='aqua'><b>Gag:</b> Trigger already exists, enabling gag on existing trigger");

      (*ppropTriggers)[trigger]->propGag().fActive(true);
      if(verbose)
         mp_wnd_text->AddHTML("<font color='lime'><b>Gag:</b> Trigger Activated");
      return;
   }

   if(IEquals(command, "gmcp"))
   {
      if(wl.Count()==2)
      {
         if(IEquals(wl[1], "dump_on"))
            return mp_connection->GMCP_Dump(true);
         if(IEquals(wl[1], "dump_off"))
            return mp_connection->GMCP_Dump(false);
      }

      mp_wnd_text->AddHTML("<font color='aqua'><b>GMCP</b> No parameter specified, available options are <b>dump_on</b> and <b>dump_off</b>");
      return;
   }

   if(IEquals(command, "mcmp"))
   {
      if(wl.Count()==2)
      {
         if(IEquals(wl[1], "flush"))
            return mp_connection->MCMP_Flush();
         if(IEquals(wl[1], "info"))
            return mp_connection->MCMP_Info();
      }

      mp_wnd_text->AddHTML("<font color='aqua'><b>MCMP</b> No parameter specified, available options are <b>flush</b> and <b>info</b>");
      return;
   }

   if(IEquals(command, "connectioninfo"))
   {
      mp_connection->ShowConnectionInfo();
      return;
   }

   // Chars
   if(IEquals(command, "chars"))
   {
      if(!mp_connection->GetServer())
      {
         mp_wnd_text->AddHTML("<icon error> <font color='red'>You need to connect through a server shortcut first");
         return;
      }

      if(!mp_connection->GetServer()->propCharacters())
      {
         mp_wnd_text->AddHTML("<icon error> <font color='magenta'>No characters");
         return;
      }

      for(auto &v : mp_connection->GetServer()->propCharacters())
         mp_wnd_text->AddHTML(FixedStringBuilder<256>("<font color='#8080FF'>", v->pclName()));

      return;
   }

   // Puppets
   if(IEquals(command, "puppets"))
   {
      if(!mp_connection->GetCharacter())
      {
         mp_wnd_text->AddHTML("<icon error> <font color='red'>You need to connect through a character first");
         return;
      }

      auto &puppets=mp_connection->GetCharacter()->propPuppets();
      if(!puppets)
      {
         mp_wnd_text->AddHTML("<font color='magenta'>No puppets");
         return;
      }

      for(auto &pPuppet : puppets)
      {
         ConstString pclName=pPuppet->pclName();
         mp_wnd_text->AddHTML(FixedStringBuilder<256>("<font color='#8080FF'>", pclName));
      }

      return;
   }

   // Puppets
   if(IEquals(command, "puppet"))
   {
      if(mp_connection->GetCharacter()==nullptr)
      {
         mp_wnd_text->AddHTML("<icon error> <font color='red'>You need to connect through a character first");
         return;
      }

      if(wl.Count()!=2)
      {
         mp_wnd_text->AddHTML("<icon error> <font color='red'>Need a puppet name to connect a puppet");
         return;
      };

      Prop::Puppet *ppropPuppet=mp_connection->GetCharacter()->propPuppets().FindByName(wl[1]);

      if(!ppropPuppet)
      {
         mp_wnd_text->AddHTML("<icon error> <font color='red'>No puppet by that name was found");
         return;
      }

      mp_wnd_MDI->Connect(mp_connection->GetServer(), mp_connection->GetCharacter(), ppropPuppet, true);
      return;
   }

   // Connect
   if(IEquals(command, "connect") || IEquals(command, "world"))
   {
      const int ciWord_Address=1;

      if(wl.Count()>3)
      {
         mp_wnd_text->AddHTML("<font color='aqua'>Usage: connect <server name or IP address>:<port> (or <server name> <character>)");
         return;
      }

      if(wl.Count()==1)
      {
         mp_wnd_text->AddHTML("<font color='aqua'>Missing Address");
         return;
      }

      ConstString lstrAddress=wl[ciWord_Address];

      // See if we have a server
      CntPtrTo<Prop::Server> ppropServer=g_ppropGlobal->propConnections().propServers().FindByName(lstrAddress);

      if(ppropServer)
      {
         Prop::Character *ppropCharacter=nullptr;

         if(wl.Count()==3)
         {
            ppropCharacter=ppropServer->propCharacters().FindByName(wl[2]);
            if(!ppropCharacter)
            {
               mp_wnd_text->AddHTML("Can't find character with this server");
               return;
            }
         }

         mp_wnd_MDI->Connect(ppropServer, ppropCharacter, nullptr, true);
         return;
      }

      ppropServer=new Prop::Server();
      ppropServer->pclHost(lstrAddress);
      mp_wnd_MDI->Connect(ppropServer, nullptr, nullptr, true);
      return;
   }

   if(IEquals(command, "reconnect"))
   {
      if(wl.Count()==2 && IEquals(wl[1], "all"))
      {
         for(auto &connection : Connection::s_root_node)
            connection.Reconnect();
         return;
      }

      mp_connection->Reconnect();
      return;
   }

   if(IEquals(command, "disconnect"))
   {
      if(wl.Count()==2 && IEquals(wl[1], "all"))
      {
         for(auto &connection : Connection::s_root_node)
            if(connection.IsConnected())
               connection.Disconnect();
         return;
      }

      if(mp_connection->IsConnected())
         mp_connection->Disconnect();
      else
         mp_wnd_text->AddHTML("<font color='aqua'>Already Disconnected");

      return;
   }

   // Close
   if(IEquals(command, "close"))
   {
      Msg::Close().Post(*this);
      return;
   }

   if(IEquals(command, "exit"))
   {
      Msg::Command(ID_FILE_QUIT, 0, 0).Post(*mp_wnd_MDI);
      return;
   }

   if(IEquals(command, "new"))
   {
      Msg::Command(ID_FILE_NEWWINDOW, 0, 0).Send(*mp_wnd_MDI);
      return;
   }

   if(IEquals(command, "newtab"))
   {
      Msg::Command(ID_FILE_NEWTAB, 0, 0).Send(*mp_wnd_MDI);
      return;
   }

   if(IEquals(command, "capturecancel"))
   {
      if(mp_connection->KillSpawnCapture())
         mp_wnd_text->AddHTML("<icon information><font color='aqua'>Spawn capture cancelled");
      else
         mp_wnd_text->AddHTML("<icon error><font color='red'>Spawn capture wasn't capturing");
      return;
   }

   if(IEquals(command, "silence"))
   {
      StopSounds();
      StopSpeech();
      return;
   }

   if(IEquals(command, "newinput"))
   {
      bool fUnique=false;
      ConstString prefix;
      if(auto params=wl.WithoutFirst(1))
      {
         for(auto &param : params)
         {
            if(param.StartsWith('/'))
            {
               if(IEquals(param, "/unique"))
               {
                  fUnique=true;
                  continue;
               }

               mp_wnd_text->AddHTML(FixedStringBuilder<256>("<icon error> Unknown option: ", param));
               return;
            }
         }

         if(!wl.Last().StartsWith('/'))
            prefix=wl.Last();
      }

      AddInputWindow(prefix, fUnique);
      return;
   }

   if(IEquals(command, "newedit"))
   {
      struct Options : XMLElement
      {
         void OnAttribute(ConstString name, ConstString value) override
         {
            if(IEquals(name, "title"))
            {
               title=value;
            }
            else if(IEquals(name, "capture"))
            {
               if(!value.To(capture_line_count))
                  throw std::runtime_error{"Capture attribute is not a number"};
            }
            else if(IEquals(name, "capture_skip"))
            {
               if(!value.To(capture_skip_count))
                  throw std::runtime_error{"Capture_skip attribute is not a number"};
            }
#if 0
            else if(IEquals(name, "dockable"))
            {
               dockable=ParseFlag(value);
            }
#endif
            else if(IEquals(name, "spellcheck"))
            {
               spellcheck=ParseFlag(value);
            }
            else if(IEquals(name, "prepend"))
               prepend=value;
            else if(IEquals(name, "append"))
               append=value;
            else
               throw std::runtime_error{FixedStringBuilder<256>("Unknown attribute: ", name).Terminate()};
         }

         OwnedString title;
         unsigned capture_line_count{};
         unsigned capture_skip_count{};
         bool dockable{};
         bool spellcheck{true};
         OwnedString prepend, append;
      } options;

      if(command_line.Length()>command.Length()+1)
      {
         auto attributes=command_line.WithoutFirst(command.Length()+1);
         ParseXMLAttributes(attributes, options);
      }

      auto &edit_pane=GetEditPane(options.title, options.dockable, options.spellcheck);
      if(options.capture_line_count>0)
      {
         HybridStringBuilder capture;

         capture(options.prepend);

         const Text::Lines &lines=mp_wnd_text->GetTextList().GetLines();
         unsigned capture_end=options.capture_line_count+options.capture_skip_count;
         PinBelow(capture_end, lines.Count());
         if(capture_end==0)
            return;

         // We search the lines in reverse order, since we're putting them on a stack
         auto pLine=lines.middle(lines.Count()-capture_end);
         auto pEnd=lines.end();

         for(unsigned i=0;i<options.capture_skip_count;i++)
            --pEnd;

         for(;pLine!=pEnd;++pLine)
         {
            pLine->TextCopy(capture);
            capture(CRLF);
         }

         capture(options.append);

         edit_pane.GetInput().SetText(capture);
      }

      edit_pane.GetInput().SetFocus();
      return;
   }

   if(IEquals(command, "setinput"))
   {
      if(command_line.Length()>command.Length()+1 && mp_input_active->GetTextLength()==0)
      {
         mp_input_active->SetText(command_line.WithoutFirst(command.Length()+1));
         mp_input_active->SetSelAll();
      }
      return;
   }

   if(IEquals(command, "map_look"))
   {
      if(!mp_wnd_map || !mp_wnd_map->mp_current_room)
      {
         mp_wnd_text->AddHTML("The map doesn't currently know your location");
         return;
      }

      auto *pRoom=mp_wnd_map->mp_current_room;
      HybridStringBuilder string("Location: ", pRoom->m_name, CRLF, "Exits: ");
      if(pRoom->mp_exits)
      {
         for(auto *pExit : pRoom->mp_exits)
         {
            auto &to=pExit->GetTo(*pRoom);
            if(!to.m_name.m_string)
               continue; // One way exit, and not this way

            auto &from=pExit->GetFrom(*pRoom);

            string(from.m_name.m_string, " <font color='green'>", to.mp_room->m_name, "</font>  ");
         }
      }
      else
         string("(none)");

      mp_wnd_text->AddHTML(string);
      return;
   }

   if(IEquals(command, "map_addroom"))
   {
      if(!mp_wnd_map || !mp_wnd_map->mp_current_room)
      {
         mp_wnd_text->AddHTML("The map doesn't currently know your location");
         return;
      }

      if(wl.Count()!=4)
      {
         mp_wnd_text->AddHTML("Command is in the form of <room name> <exit to get there> <exit to get back>");
         return;
      }

      mp_wnd_map->CreateRoomAndExit(wl[1], wl[2], wl[3]);
      return;
   }

   if(IEquals(command, "map_addexit"))
   {
      if(!mp_wnd_map || !mp_wnd_map->mp_current_room)
      {
         mp_wnd_text->AddHTML("The map doesn't currently know your location");
         return;
      }

      if(wl.Count()!=3)
      {
         mp_wnd_text->AddHTML("Command is in the form of <exit to get there> <exit to get back>");
         return;
      }

      if(!mp_wnd_map->CreateExit(wl[1], wl[2]))
         mp_wnd_text->AddHTML("Unknown direction or no room in that direction");

      return;
   }

   if(IEquals(command, "map_guesslocation"))
   {
      if(!mp_wnd_map)
      {
         mp_wnd_text->AddHTML("No map");
         return;
      }

      mp_wnd_map->GuessLocationOnMap();
      return;
   }

#if 0
   if(IEquals(command, "mucknet"))
   {
      if(!Wnd_MuckNet::HasInstance())
         new Wnd_MuckNet();
      return;
   }
#endif

   if(IEquals(command, "naws"))
   {
      uint2 size;

      if(wl.Count()==2 && IEquals(wl[1], "auto"))
      {
         size=mp_wnd_text->GetSizeInChars();
      }
      else if(wl.Count()==3)
      {
         if(!wl[1].To(size.x) || !wl[2].To(size.y))
            size={};
      }

      if(size.x>0 && size.y>0 && size.x<65535 && size.y<65535)
      {
         auto &telnet=mp_connection->GetTelnet();
         telnet.m_do_naws=false;
         telnet.SendNAWS(size);
         if(verbose)
            mp_wnd_text->AddHTML(FixedStringBuilder<256>("<icon information>TELOPT_NAWS <b>Width:</b>", size.x, " <b>Height:</b>", size.y, " sent"));
         return;
      }

      mp_wnd_text->AddHTML("<icon information>Invalid usage, try '/help naws' to see help for this command");
      return;
   }

   if(IEquals(command, "ttype"))
   {
      mp_wnd_text->AddHTML(FixedStringBuilder<256>("Current TType = ", g_ppropGlobal->pclTelnet_TType()));

      if(wl.Count()<2)
         return;
      g_ppropGlobal->pclTelnet_TType(wl[1]);
      mp_wnd_text->AddHTML(FixedStringBuilder<256>("New TType = ", g_ppropGlobal->pclTelnet_TType()));
      return;
   }

   if(IEquals(command, "restoreinfo"))
   {
      struct LogInfo
      {
         unsigned m_used;
      };

      mp_wnd_text->AddHTML("<b>Live Restore.dat information...</b> running CheckAndRepair first");

      auto &restoreLogs=*gp_restore_logs;
      restoreLogs.CheckAndRepair();

      auto bufferSize=restoreLogs.GetBufferSize();
      mp_wnd_text->AddHTML("<b>CheckAndRepair complete");
      mp_wnd_text->AddHTML(FixedStringBuilder<256>("Buffer Size: <b>", bufferSize/1024, "kb</b>   Total Buffers: <b>", restoreLogs.Count(), "</b>"));

      Collection<LogInfo> logs;

      for(unsigned i=0;i<restoreLogs.Count();i++)
         logs.Push(restoreLogs.GetUsed(i));

      // Go through all the names, remove duplicates and put anything unique into the 'Info' field
      for(auto &pServer : g_ppropGlobal->propConnections().propServers())
      {
         // Iterate through the characters on the server
         for(auto &pCharacter : pServer->propCharacters())
         {
            auto index=pCharacter->RestoreLogIndex();
            if(index==-1)
               continue;

            Assert(unsigned(index)<logs.Count());

            LogInfo &info=logs[index];
            mp_wnd_text->AddHTML(FixedStringBuilder<256>("<b>", index, "</b>  ", pServer->pclName(), " - ", pCharacter->pclName(), " - Used: <b>", info.m_used, "  ", info.m_used*100/bufferSize, "</b>%"));
         }
      }

      mp_wnd_text->AddHTML("<B>Summary complete");
      return;
   }

   if(IEquals(command, "removelast"))
   {
      if(mp_wnd_text->GetTextList().GetLines().Count())
         mp_wnd_text->RemoveLine(UnconstRef(mp_wnd_text->GetTextList().GetLines().back()));
      return;
   }

   if(IEquals(command, "receive"))
   {
      thread_local unsigned recursion{};
      RestorerOf _(recursion);
      if(++recursion==3)
      {
         mp_wnd_text->AddHTML("<icon exclamation> Receive recursion limit hit, check your triggers!");
         return;
      }

      if(command_line.Length()>command.Length()+1)
      {
         auto data=command_line.WithoutFirst(command.Length()+1);
         if(mp_connection->IsInReceive())
         {
            mp_connection->Display(data);
            return;
         }

         if(auto *pServer=mp_connection->GetServer())
         {
            switch(pServer->eEncoding())
            {
               default:
               case Prop::Server::Encoding::UTF8: mp_connection->Receive(data); break;
               case Prop::Server::Encoding::CP1252: mp_connection->Receive(UTF8ToCodePage(data, 1252)); break;
               case Prop::Server::Encoding::CP437: mp_connection->Receive(UTF8ToCodePage(data, 437)); break;
            }
         }
         else
            mp_connection->Receive(data);
      }
      mp_connection->Receive(ConstString(CRLF));
      return;
   }

   if(IEquals(command, "receivegmcp"))
   {
      auto data=command_line.WithoutFirst(command.Length()+1);
      mp_connection->OnGMCP(data);
      return;
   }

   if(IEquals(command, "wall"))
   {
      if(command_line.Length()<command.Length()+1)
         return;
      Strings::HeapStringBuilder param; param(command_line.WithoutFirst(command.Length()+1), CRLF);

      for(auto &connection : Connection::s_root_node)
      {
         if(!connection.IsConnected())
            continue;

         connection.GetMainWindow().SendLines(param);
      }
      return;
   }

   if(IEquals(command, "resetconfig"))
   {
      ResetConfig();
      return;
   }

   if(IEquals(command, "debugaliases"))
   {
      mp_connection->OpenAliasDebugWindow();
      return;
   }

   if(IEquals(command, "debugnetwork"))
   {
      mp_connection->OpenNetworkDebugWindow();
      return;
   }

   if(IEquals(command, "debugtriggers"))
   {
      mp_connection->OpenTriggerDebugWindow();
      return;
   }

   if(IEquals(command, "switchtab"))
   {
      if(wl.Count()!=3)
      {
         mp_wnd_text->AddHTML("<icon error> <font color='red'>Expected 'tab group' and 'tab name' as parameters");
         return;
      }

      for(auto &window : m_spawn_tabs_windows)
      {
         if(window.m_title==wl[1])
         {
            if(!window.SetVisible(wl[2]))
               mp_wnd_text->AddHTML("<icon error> <font color='red'>Tab not found");
            return;
         }
      }

      mp_wnd_text->AddHTML("<icon error> <font color='red'>Tab group not found");
      return;
   }

   if(IEquals(command, "tabcolor"))
   {
      Color color=Colors::White;
      if(wl.Count()==2)
         Streams::Input(wl[1]).Parse(color);

      if(mp_connection->GetCharacter() && !mp_connection->GetPuppet())
         TabColor(color);
      return;
   }

   if(IEquals(command, "ansireset"))
   {
      mp_connection->ResetAnsi();
      return;
   }

   if(IEquals(command, "roll"))
   {
      unsigned count, sides, bonus{};
      ConstString count_string, sides_string, bonus_string;
      if(wl.Count()!=2)
      {
         mp_wnd_text->AddHTML("🎲 Roll: Missing parameter");
         return;
      }

      ConstString param=wl[1];
      if(param.Split('+', param, bonus_string))
         bonus_string.To(bonus);

      if(wl.Count()!=2 || !wl[1].Split('d', count_string, sides_string) || !count_string.To(count) || !sides_string.To(sides) || count<1 || count>1000 || sides<1)
      {
         mp_wnd_text->AddHTML("🎲 Bad roll format, should be in the form of [count]d[sides](+bonus). The count must be <= 1000. For example, to roll a 6 sided die 10 times: /roll 10d6");
         return;
      }

      HybridStringBuilder result;

      result("🎲 Rolling ", count, " ", sides, "-sided dice");
      if(bonus)
         result(" with a +", bonus, " bonus");
      result("  Min: ", count+bonus, " Max: ", sides*count+bonus, " Ave: ", Strings::Float((count+sides*count)/2.0f+bonus, 2));
      mp_wnd_text->AddHTML(result);

      result.Clear();
      result("🎲 Rolled");

      unsigned total=0;
      for(unsigned i=0;i<count;i++)
      {
         unsigned roll=g_random(sides-1)+1;
         result(' ', roll);
         total+=roll;
      }

      mp_wnd_text->AddHTML(result);
      result.Clear();
      result("🎲 Average roll: ", Strings::Float(float(total)/float(count), 2), "  Roll Total");
      if(bonus)
         result(" +", bonus);
      result(": ", total+bonus);
      mp_wnd_text->AddHTML(result);
      return;
   }

   if(IEquals(command, "rolltest"))
   {
      constexpr unsigned side_count=6;
      unsigned sides[side_count]{};
      unsigned total=0;
      unsigned roll_count=1'000'000;
      for(unsigned i=0;i<roll_count;i++)
      {
         unsigned roll=g_random(side_count-1);
         sides[roll]++;
         total+=roll+1;
      }

      mp_wnd_text->AddHTML(FixedStringBuilder<256>("Die fairness test, for ", roll_count, " rolls:"));

      float odds=float(roll_count)/float(side_count);
      for(unsigned i=0;i<6;i++)
         mp_wnd_text->AddHTML(FixedStringBuilder<256>("Side ", i+1, " odds: ", sides[i]/odds));

      mp_wnd_text->AddHTML(FixedStringBuilder<256>("Average of all rolls: ", float(total)/float(roll_count)));
      return;
   }

   if(IEquals(command, "test"))
   {
      if(wl.Count()<2)
      {
         mp_wnd_text->AddHTML("<font color='aqua'>What do you want to test? (ansi/html/emoji/international/utf8)");
         return;
      }

      if(IEquals(wl[1], "ansi"))
      {
         mp_connection->Receive(ConstString(CRLF
            "\x01B[0;1;37;44m          Ansi color test          " CRLF CRLF
            "\x01B[0;1;4;37;41mNormal       Bold         Faint        Background   " CRLF
            "\x01B[0;30;40m30 - Black   \x01B[1m30 - Black   \x01B[22;2m30 - Black   \x01B[0;37;40m40 - Black" CRLF
            "\x01B[0;31;40m31 - Red     \x01B[1m31 - Red     \x01B[22;2m31 - Red     \x01B[0;37;41m41 - Red" CRLF
            "\x01B[0;32;40m32 - Green   \x01B[1m32 - Green   \x01B[22;2m32 - Green   \x01B[0;37;42m42 - Green" CRLF
            "\x01B[0;33;40m33 - Yellow  \x01B[1m33 - Yellow  \x01B[22;2m33 - Yellow  \x01B[0;37;43m43 - Yellow" CRLF
            "\x01B[0;34;40m34 - Blue    \x01B[1m34 - Blue    \x01B[22;2m34 - Blue    \x01B[0;37;44m44 - Blue" CRLF
            "\x01B[0;35;40m35 - Magenta \x01B[1m35 - Magenta \x01B[22;2m35 - Magenta \x01B[0;37;45m45 - Magenta" CRLF
            "\x01B[0;36;40m36 - Cyan    \x01B[1m36 - Cyan    \x01B[22;2m36 - Cyan    \x01B[0;37;46m46 - Cyan" CRLF
            "\x01B[0;37;40m37 - White   \x01B[1m37 - White   \x01B[22;2m37 - White   \x01B[0;30;47m47 - White" CRLF

            CRLF
            "Bright Colors" CRLF
            "\x01B[0;90;40m90 - Black   \x01B[1m90 - Black   \x01B[22;2m90 - Black   \x01B[0;30;100m100 - Black" CRLF
            "\x01B[0;91;40m91 - Red     \x01B[1m91 - Red     \x01B[22;2m91 - Red     \x01B[0;30;101m101 - Red" CRLF
            "\x01B[0;92;40m92 - Green   \x01B[1m92 - Green   \x01B[22;2m92 - Green   \x01B[0;30;102m102 - Green" CRLF
            "\x01B[0;93;40m93 - Yellow  \x01B[1m93 - Yellow  \x01B[22;2m93 - Yellow  \x01B[0;30;103m103 - Yellow" CRLF
            "\x01B[0;94;40m94 - Blue    \x01B[1m94 - Blue    \x01B[22;2m94 - Blue    \x01B[0;30;104m104 - Blue" CRLF
            "\x01B[0;95;40m95 - Magenta \x01B[1m95 - Magenta \x01B[22;2m95 - Magenta \x01B[0;30;105m105 - Magenta" CRLF
            "\x01B[0;96;40m96 - Cyan    \x01B[1m96 - Cyan    \x01B[22;2m96 - Cyan    \x01B[0;30;106m106 - Cyan" CRLF
            "\x01B[0;97;40m97 - White   \x01B[1m97 - White   \x01B[22;2m97 - White   \x01B[0;30;107m107 - White" CRLF

            CRLF CRLF
            "\x01B[0;1;4;37;41mAttributes          " CRLF
            "\x01B[0;40mAaBbCc 0 - Normal" CRLF
            "\x01B[0;1mAaBbCc 1 - Bold        \x01B[22m22 - no-bold no-faint" CRLF
            "\x01B[0;2mAaBbCc 2 - Faint       \x01B[22m22 - no-bold no-faint" CRLF
            "\x01B[0;3mAaBbCc 3 - Italics     \x01B[23m23 - no-italics" CRLF
            "\x01B[0;4mAaBbCc 4 - Underline   \x01B[24m24 - no-underline" CRLF
            "\x01B[0;5mAaBbCc 5 - Blink       \x01B[25m25 - no-blink" CRLF
            "\x01B[0;6mAaBbCc 6 - Blink Fast  \x01B[25m25 - no-blink" CRLF
            "\x01B[0;7mAaBbCc 7 - Reverse     \x01B[27m27 - no-reverse" CRLF
            "\x01B[0;8mAaBbCc 8 - Reverse (Non Standard?)" CRLF
            "\x01B[0;9mAaBbCc 9 - Strikeout   \x01B[29m29 - no-strikeout" CRLF
            CRLF
            "\x01B[0mNormal text - \x01B[7mReversed \x01B[34mBlue Foreground  \x01B[41mRed Background  \x01B[27m Unreversed  \x01B[0m And reset" CRLF
            CRLF
            "\x01B[0mTesting the [m code - \x01B[0;4;32;40mAttributes set\x01B[m Now Reset" CRLF
            "\x01B[0mTesting the [0m code - \x01B[0;4;32;40mAttributes set\x01B[0m Now Reset" CRLF
            "\x01B[0mReset, [1;;35m \x01B[1;;35m Test \x01B[0m Reset, [;;;35m \x01B[;;;35m Test" CRLF
            "\x01B[0;37;40mPersisting after a newline \x01B[0;1;34mBlue" CRLF
            "after newline, only works if non reset is set \x01B[9m Strikeout" CRLF
            "strikeout after newline" CRLF
            "\x01B[0;37;40mResetting back to defaults" CRLF
            "Blink test: \x01B[0;5;44mBlink with background color" CRLF
            "wrapping to next line 🐴\x01B[25m No blink\x01B[0;37;40m" CRLF
            CRLF
            "Tab\x09" "Tab\x09" "Tab\x09" "Tab\x09" "Tab\x09" "Tab" CRLF
            "12\x09" "1234\x09" "123456\x09" "12345678\x09" "End" CRLF
         ));

         mp_connection->Receive(ConstString(CRLF "8-bit Color Ansi Test" CRLF));
         for(unsigned i=0;i<16;i++)
         {
            FixedStringBuilder<256> buffer("\x01B[48;5;", i, 'm', Strings::Int(i, 4, ' '));
            if((i+1)%8==0) buffer(CRLF);
            mp_connection->Receive(buffer);
         }

         for(unsigned i=16;i<232;i+=6)
         {
            FixedStringBuilder<256> buffer("\x01B[0;30m");
            for(unsigned j=0;j<6;j++)
               buffer("\x01B[48;5;", i+j, 'm', Strings::Int(i+j, 4, ' '));
            buffer("\x01B[0m ");
            for(unsigned j=0;j<6;j++)
               buffer("\x01B[38;5;", i+j, 'm', Strings::Int(i+j, 4, ' '));
            buffer(CRLF);
            mp_connection->Receive(buffer);
         }

         for(unsigned i=232;i<256;i++)
         {
            FixedStringBuilder<256> buffer("\x01B[48;5;", i, 'm', Strings::Int(i, 4, ' '));
            if((i-231)%8==0) buffer(CRLF);
            mp_connection->Receive(buffer);
         }
         mp_connection->Receive(ConstString("\x01B[0;37;40mResetting back to defaults" CRLF));

         mp_connection->Receive(ConstString("24-bit Color Ansi Test" CRLF));
         for(unsigned row=1;row<8;row++)
         {
            for(unsigned x=0;x<80;x++)
            {
               Color color(HUEtoRGB(Modulus(x*row/80.0f, 1.0f)));
               mp_connection->Receive(FixedStringBuilder<256>("\x01B[38;2;", int(color.Red()), ';', int(color.Green()), ';', int(color.Blue()), 'm', "-=#="[x%4]));
            }
            mp_connection->Receive(ConstString(CRLF));
         }
         for(unsigned row=0;row<8;row++)
         {
            for(unsigned x=0;x<80;x++)
            {
               Color color(HUEtoRGB(x/80.0f)*(8.0f-row)/8.0f);
               mp_connection->Receive(FixedStringBuilder<256>("\x01B[48;2;", int(color.Red()), ';', int(color.Green()), ';', int(color.Blue()), 'm', ' '));
            }
            mp_connection->Receive(ConstString(CRLF));
         }
         for(unsigned i=0;i<80;i++)
            mp_connection->Receive(FixedStringBuilder<256>("\x01B[48;2;", i*256/80, ';', i*256/80, ';', i*256/80, 'm', ' '));
         mp_connection->Receive(ConstString(CRLF));
         for(unsigned i=0;i<80;i++)
            mp_connection->Receive(FixedStringBuilder<256>("\x01B[48;2;", i*256/80, ';', 0, ';', 0, 'm', ' '));
         mp_connection->Receive(ConstString(CRLF));
         for(unsigned i=0;i<80;i++)
            mp_connection->Receive(FixedStringBuilder<256>("\x01B[48;2;", 0, ';', i*256/80, ';', 0, 'm', ' '));
         mp_connection->Receive(ConstString(CRLF));
         for(unsigned i=0;i<80;i++)
            mp_connection->Receive(FixedStringBuilder<256>("\x01B[48;2;", 0, ';', 0, ';', i*256/80, 'm', ' '));
         mp_connection->Receive(ConstString(CRLF));

         mp_connection->Receive(ConstString("\x01B[0;37;40mResetting back to defaults" CRLF));
         return;
      }

      if(IEquals(wl[1], "html"))
      {
         mp_wnd_text->AddHTML(R"(<font face="Cambria" size="16">Text <font face="Cambria" size="24">Text <font face="Cambria" size="32">Text <font face="Cambria" size="40">Text <font face="Cambria" size="48">Text!)");
         mp_wnd_text->AddHTML(R"(<font size="4">Relative <font size="+4">Text <font size="+4">Text <font size="+4">Text <font size="+4">Text!)");
         mp_wnd_text->AddHTML(R"(<font face="Times New Roman" size="32">Times New Roman 32 in <font color='fuchsia'>fuchsia </font><font color='yellow'>yellow </font><font color='aqua'>aqua </font><font face="Webdings">Webdings!)");
         mp_wnd_text->AddHTML(R"(<b><font color="#FF0000">Red</font> <font color="#00FF00">Green</font> <font color="#0000FF">Blue</font> <font color="#D2691E">Chocolate</font> <font color="#FFE4C4">Bisque</font> <font color="#FFD700">Gold</font></b>)");
         mp_wnd_text->AddHTML("<icon app>App <icon exclamation>Exclamation <icon information>Information <icon error>Error");
         return;
      }

      if(IEquals(wl[1], "emoji"))
      {
         mp_connection->Receive(ConstString(
            R"(This is a dog, this is a dog face. End of line Zebra)" CRLF
            R"(Smiley test :) :P :| :( :'( stuffs :) ;). No cat here: 12:30, but here :3 )" CRLF
            R"(Smileys with noses :-) ;-) No sun emoji here: sun_name)" CRLF
            R"(Ear, and an ear of corn. Potato-test, cookie is tasty)" CRLF
            R"(This is an http://www.sample.com address, should have no :/ smileys)" CRLF
            R"(Parenthesis: (melon) (grapes) (tomato) (coconut))" CRLF
            R"(crocodile-turtle,lizard: snake,dragon face dragon sauropod T-Rex)" CRLF));

         mp_connection->Receive(ConstString(
            "UTF8 Emoji text:" CRLF
            "🌀🌁🌂🌃🌄🌅🌆🌇🌈🌉🌊🌋🌌🌍🌎🌏" CRLF
            "🐀🐁🐂🐃🐄🐅🐆🐇🐈🐉🐊🐋🐌🐍🐎🐏" CRLF
            "🐐🐑🐒🐓🐔🐕🐖🐗🐘🐙🐚🐛🐜🐝🐞🐟" CRLF
            "🐠🐡🐢🐣🐤🐥🐦🐧🐨🐩🐪🐫🐬🐭🐮🐯" CRLF
            "🐰🐱🐲🐳🐴🐵🐶🐷🐸🐹🐺🐻🐼🐽🐾" CRLF
            "😀😁😂😃😄😅😆😇😈😉😊😋😌😍😎😏" CRLF
            "😐😑😒😓😔😕😖😗😘😙😚😛😜😝😞😟" CRLF
            "😠😡😢😣😤😥😦😧😨😩😪😫😬😭😮😯" CRLF
            "😰😱😲😳😴😵😶😷😸😹😺😻😼😽😾😿" CRLF));
         return;
      }

      if(IEquals(wl[1], "international"))
      {
         // From http://kermitproject.org/utf8.html
         mp_wnd_text->AddHTML(
            "English: I can eat glass and it doesn't hurt me." CRLF
            "Chinese: 我能吞下玻璃而不伤身体。Done" CRLF
            "Japanese: 私はガラスを食べられます。それは私を傷つけません。" CRLF
            "Thai: ฉันกินกระจกได้ แต่มันไม่ทำให้ฉันเจ็บ " CRLF
            "Korean: 나는 유리를 먹을 수 있어요. 그래도 아프지 않아요" CRLF
            "Hindi: मैं काँच खा सकता हूँ और मुझे उससे कोई चोट नहीं पहुंचती." CRLF
            "English (Braille): ⠊⠀⠉⠁⠝⠀⠑⠁⠞⠀⠛⠇⠁⠎⠎⠀⠁⠝⠙⠀⠊⠞⠀⠙⠕⠑⠎⠝⠞⠀⠓⠥⠗⠞⠀⠍⠑" CRLF
            "Anglo-Saxon (Runes): ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ᛬" CRLF
            "Greek (monotonic): Μπορώ να φάω σπασμένα γυαλιά χωρίς να πάθω τίποτα." CRLF
            "Arabic(3): أنا قادر على أكل الزجاج و هذا لا يؤلمني."
//            "Zalgo: B͚̣̱e̺͔̙c͎̬a͉͔̼u̬̩̥s̗̕e̢͎̥ ̤̹̪w͙̗͞e̱ ͉̺a̟͞l͟l͚̥̞ ̪̲̺n̙̥̜e͈̱͡e̛̳̩d̺̜̕ ̮̖͟a͖̥͢ ͙͚͝l̜̯̩i̧̘̼ţ͓̬t͎̯͚l̤̫͘ḛ̢̼ ̥͎͝Z̟͉ͅal̹ͅg̸̝ó̘ ̘̟i͕̳̫n̩̮ͅ ̛͔͈o̪̯̠ųr̴̭͉ ̳̹̞l̡̬̼i̯̣̦v̢e͎̟͞s̢̞̬" CRLF
         );
         return;
      }

      if(IEquals(wl[1], "utf8"))
      {
         mp_connection->Receive(ConstString(
            "Valid 2 Octet Sequence \xc3\xb1" CRLF
            "Invalid 2 Octet Sequence \xc3\x28" CRLF
            "Invalid Sequence Identifier \xa0\xa1" CRLF
            "Valid 3 Octet Sequence \xe2\x82\xa1" CRLF
            "Invalid 3 Octet Sequence (in 2nd Octet) \xe2\x28\xa1" CRLF
            "Invalid 3 Octet Sequence (in 3rd Octet) \xe2\x82\x28" CRLF
            "Valid 4 Octet Sequence \xf0\x90\x8c\xbc" CRLF
            "Invalid 4 Octet Sequence (in 2nd Octet) \xf0\x28\x8c\xbc" CRLF
            "Invalid 4 Octet Sequence (in 3rd Octet) \xf0\x90\x28\xbc" CRLF
            "Invalid 4 Octet Sequence (in 4th Octet) \xf0\x28\x8c\x28" CRLF
#if 0
            "Valid 5 Octet Sequence (but not Unicode!) \xf8\xa1\xa1\xa1\xa1" CRLF
            "Valid 6 Octet Sequence (but not Unicode!) \xfc\xa1\xa1\xa1\xa1\xa1" CRLF
#endif
            ));
         return;
      }

#ifndef _DEBUG
      if(IEquals(wl[1], "crash"))
      {
         int *p=0;
         *p=5;
         return; // We'll never reach here
      }

#if 0
      if(IEquals(wl[1], "save"))
      {
         mp_wnd_text->AddHTML("<icon information><font color='green'>Saving config 100x");
         for(unsigned i=0;i<100;i++)
            Msg::Command(ID_FILE_SAVESETTINGS, nullptr, 0).Send(GetMDI());
         mp_wnd_text->AddHTML("<icon information><font color='green'>Done");
         return;
      }
#endif

      if(IEquals(wl[1], "deleteunnamed"))
      {
         for(auto &propServer : g_ppropGlobal->propConnections().propServers())
         {
            Collection<Prop::Character*> p_chars;
            for(auto &prop_char : propServer->propCharacters())
            {
               if(prop_char->pclName().StartsWith("Unnamed"))
                  p_chars.Push(prop_char);
            }

            for(auto *p_char : p_chars)
               propServer->propCharacters().Delete(*p_char);
         }
         return;
      }

      if(IEquals(wl[1], "connectall"))
      {
         for(auto &propServer : g_ppropGlobal->propConnections().propServers())
            mp_wnd_MDI->Connect(propServer, nullptr, nullptr, false);
         return;
      }

      if(IEquals(wl[1], "pad"))
      {
         auto ipsum=ConstString("<font face='Arial' size='16'>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n\r\nUt enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
         auto pLine=Text::Line::CreateFromHTML(ipsum);
         pLine->SetIndentLeft(0.20f);
         pLine->SetIndentRight(0.20f);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML(ipsum);
         pLine->SetAlignment(Text::Records::Alignment::Center);
         pLine->SetIndentLeft(0.20f);
         pLine->SetIndentRight(0.20f);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML(ipsum);
         pLine->SetAlignment(Text::Records::Alignment::Right);
         pLine->SetIndentLeft(0.20f);
         pLine->SetIndentRight(0.20f);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("A line with a background");
         pLine->SetParagraphColor(Colors::Blue);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("A centered line");
         pLine->SetAlignment(Text::Records::Alignment::Center);
         pLine->SetParagraphStrokeColor(Colors::White);
         pLine->SetStrokeStyle(Text::Records::StrokeStyle::Top);
         mp_wnd_text->Add(std::move(pLine));

         mp_wnd_text->AddHTML("<p border='5' padding='5' background-color='#800040' stroke-color='#FF0000' stroke-width='2' border-style='round' align='center'><font face='arial'><b>New Content</b>");
         mp_wnd_text->AddHTML("<p border='5' padding='5' background-color='#800040' stroke-color='#FF0000' stroke-width='2' stroke-style='bottom' align='center'><font face='arial'><b>New Content</b>");
         mp_wnd_text->AddHTML("<p border='5' padding='5' stroke-color='#FF0000' stroke-width='2' stroke-style='bottom' align='center'><font color='#FF0000' face='arial'><b>New Content</b>");
         mp_wnd_text->AddHTML("<p border='5' padding='5' stroke-color='#FF8080' stroke-width='2' stroke-style='bottom' align='center'><font color='#FF0000' face='arial'><b>New Content</b>");
         mp_wnd_text->AddHTML("<p border='5' padding='5' stroke-color='#FF0000' stroke-width='2' stroke-style='bottom' align='center'><font color='#FF8080' face='arial'><b>New Content</b>");

         pLine=Text::Line::CreateFromHTML("A centered line");
         pLine->SetAlignment(Text::Records::Alignment::Center);
         pLine->SetParagraphStrokeColor(Colors::White);
         pLine->SetParagraphStrokeWidth(4);
         pLine->SetStrokeStyle(Text::Records::StrokeStyle::Bottom);
         pLine->SetIndentLeft(0.40f);
         pLine->SetIndentRight(0.40f);
         pLine->SetParagraphColor(Color(118, 70, 254));
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("A right aligned line");
         pLine->SetAlignment(Text::Records::Alignment::Right);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("<font face='Arial' size='16'>indent-right: 30% padding-top:10 <blink>Blinking!</blink> padding-bottom:20 border:15 border-style:round alignment:right");
         pLine->SetAlignment(Text::Records::Alignment::Right);
         pLine->SetParagraphColor(Color(118, 70, 254));
         pLine->SetParagraphStrokeColor(Colors::White);
         pLine->SetParagraphStrokeWidth(4);
         pLine->SetIndentRight(0.30f);
         pLine->SetBorder(15);
         pLine->SetBorderStyle(Text::Records::BorderStyle::Round);
         mp_wnd_text->Add(std::move(pLine));


         pLine=Text::Line::CreateFromHTML("<font face='Arial' size='16'>indent-right: 30% padding-top:1 padding-bottom:1 border:20 border-style:round alignment:left");
         pLine->SetParagraphColor(Color(118, 70, 254));
         pLine->SetParagraphStrokeColor(Colors::White);
         pLine->SetParagraphStrokeWidth(4);
         pLine->SetIndentRight(0.30f);
         pLine->SetPaddingTop(1);
         pLine->SetPaddingBottom(1);
         pLine->SetBorder(20);
         pLine->SetBorderStyle(Text::Records::BorderStyle::Round);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("A solid line");
         pLine->SetAlignment(Text::Records::Alignment::Center);
         pLine->SetParagraphColor(Color(118, 70, 254));
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("A centered line");
         pLine->SetAlignment(Text::Records::Alignment::Center);
         pLine->SetParagraphColor(Color(118, 70, 254));
//         pLine->SetParagraphStrokeColor(Colors::White);
         pLine->SetParagraphStrokeWidth(4);
         pLine->SetPaddingTop(1);
         pLine->SetPaddingBottom(1);
         pLine->SetBorder(20);
         pLine->SetBorderStyle(Text::Records::BorderStyle::Round);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("A solid line");
         pLine->SetAlignment(Text::Records::Alignment::Center);
         pLine->SetParagraphColor(Color(118, 70, 254));
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("<font face='Arial' size='16'>indent-left: 30% padding-top:1 padding-bottom:1 border:10 border-style:round alignment:right");
         pLine->SetAlignment(Text::Records::Alignment::Right);
         pLine->SetParagraphColor(Color(0, 132, 255));
         pLine->SetParagraphStrokeColor(Colors::White);
         pLine->SetParagraphStrokeWidth(4);
         pLine->SetIndentLeft(0.30f);
         pLine->SetPaddingTop(1);
         pLine->SetPaddingBottom(1);
         pLine->SetBorder(10);
         pLine->SetBorderStyle(Text::Records::BorderStyle::Round);
         mp_wnd_text->Add(std::move(pLine));

         mp_wnd_text->Add(Text::Line::CreateFromText("A simple line without any paragraph styles. Just regular text here."));

         pLine=Text::Line::CreateFromHTML("<font face='Arial' size='16'>indent-right: 30% padding-top:1 padding-bottom:1 border:20 border-style:square");
         pLine->SetParagraphColor(Color(118, 70, 254));
         pLine->SetParagraphStrokeColor(Colors::White);
         pLine->SetParagraphStrokeWidth(2);
         pLine->SetIndentRight(0.30f);
         pLine->SetPaddingTop(1);
         pLine->SetPaddingBottom(1);
         pLine->SetBorder(20);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("<font face='Arial' size='16'>indent-right: 40%");
         pLine->SetParagraphColor(Colors::Blue);
         pLine->SetIndentRight(0.40f);
//         pLine->SetPaddingTop(2);
//         pLine->SetPaddingBottom(2);
//         pLine->SetBorderStyle(Text::Records::BorderStyle::Round);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("<font face='Arial' size='16'>indent-left: 50% indent-right: 50% No room!");
         pLine->SetParagraphColor(Color(133, 161, 185));
         pLine->SetIndentLeft(0.50f);
         pLine->SetIndentRight(0.50f);
         mp_wnd_text->Add(std::move(pLine));

         pLine=Text::Line::CreateFromHTML("<b>\\1 generates a test case!</b>");
         pLine->InsertEmoji(0, "❇");
         pLine->InsertEmoji(pLine->GetText().Count(), "❇");
         pLine->SetAlignment(Text::Records::Alignment::Center);
         mp_wnd_text->Add(std::move(pLine));

         return;
      }

      if(IEquals(wl[1], "linefuzz"))
      {
         RandomKISS random;
         random.SeedRandomDevice();

         for(unsigned i=0;i<100'000;i++)
         {
            uint32 garbage[256];
            for(auto &g : garbage)
               g=random();

            unsigned length=random()%255+1;
            String garbageString((char *)garbage, length);
            if(garbageString.Last()==0)
               garbageString.Last()=' '; // Don't make this look like a null terminated string, as it'll set off a debug assert

            mp_connection->Display(garbageString);
//            m_pConnection->Receive(ConstString(CRLF));
         }
         return;
      }

      static constexpr const ConstString ipsum_text("Lorem ipsum dolor sit amet,    consectetur adipiscing elit, 🤞sed do eiusmod tempor incididunt     ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat 🐱cupidatat non proident, 🐱‍👤sunt in culpa qui officia deserunt    mollit anim id est laborum. \r\n");
//      static constexpr const ConstString ipsum_text("🐱‍👤a🐱‍👤b🐱‍👤c🐱‍👤d🐱‍👤e🐱‍👤f🐱‍👤g🐱‍👤h🐱‍👤i🐱‍👤j🐱‍👤k");

      if(IEquals(wl[1], "timedspam"))
      {
         RandomKISS random;
         random.SeedRandomDevice();

         for(unsigned i=0;i<1000;i++)
         {
            HybridStringBuilder text("/receive <blink>Testing</blink>", ipsum_text.First(random(ipsum_text.Count())));
            new DelayTimer(*this, text, i/10.0f);
         }
         return;
      }

      if(IEquals(wl[1], "timedgarbage"))
      {
         RandomKISS random;
//         random.SeedRandomDevice();

         for(unsigned i=0;i<1000;i++)
         {
            HybridStringBuilder text("/receive ");
            for(unsigned j=0;j<500;j++)
               text(char(random()));
            new DelayTimer(*this, text, i/10.0f);
         }
         return;
      }

      if(IEquals(wl[1], "line"))
      {
         RandomKISS random;
         random.SeedRandomDevice();
         auto pLine=Text::Line::CreateFromText(ipsum_text);

         for(unsigned i=0;i<100;i++)
         {
            uint2 range(random(pLine->GetText().Count()), random(pLine->GetText().Count()));
            Sort(range.x, range.y);
            
            switch(random(14))
            {
               case 0: pLine->SetColor(range, Color(random(255), random(255), random(255))); break;
               case 1: pLine->SetBackgroundColor(range, Color(random(255), random(255), random(255))); break;
               case 2: pLine->SetItalic(range, random(1)!=0); break;
               case 3: pLine->SetBold(range, random(1)!=0); break;
               case 4: pLine->SetUnderline(range, random(1)!=0); break;
               case 5: pLine->SetStrikeout(range, random(1)!=0); break;
               case 7: pLine->SetFlash(range); break;
               case 8: pLine->SetIndentLeft(random(0.4f)); break;
               case 9: pLine->SetIndentRight(random(0.4f)); break;
               case 10: pLine->SetPaddingTop(random(100)); break;
               case 11: pLine->SetPaddingBottom(random(100)); break;
               case 12: pLine->SetBorder(random(10)); break;
               case 13: pLine->SetParagraphColor(Color(random(255), random(255), random(255))); break;
            }
         }
         mp_wnd_text->Add(std::move(pLine));
         return;
      }

      if(IEquals(wl[1], "image_left"))
      {
         auto p_line=Text::Line::CreateFromText(ipsum_text);

         auto url=Text::ImageURL{ConstString("https://store-images.s-microsoft.com/image/apps.48673.13510798887612985.0c40d942-c868-4b97-878d-b20546241d99.d399712f-eec1-4b5e-bd11-f586e83a8a0c?mode=scale&q=90&h=300&w=300")};
         auto p_image=MakeCounting<ImageAvatar>(std::move(url));
         p_image->m_size/=2;

         p_line->SetParagraphRecord(Text::Records::ImageLeft(&*p_image));
         p_line->SetParagraphStrokeColor(Colors::Blue);
         p_line->SetParagraphStrokeWidth(5);
         p_line->SetBorder(7);
//         p_line->SetPaddingTop(0);
//         p_line->SetPaddingBottom(30);

         mp_wnd_text->Add(std::move(p_line));
         return;
      }

      if(IEquals(wl[1], "image_left2"))
      {
         auto p_line=Text::Line::CreateFromText("Nuku sings, \"Hey. Look over here! It's me, Nuku. I'm being entertaining. Look at my head. It's all painted! Look at my head! I got a big, old head, hey! Ho! Alright, the show's over, I'm tired.\"");

         auto url=Text::ImageURL{ConstString("https://flexiblesurvival.com/resources/avatars/render.php?arms=Naga&head=Naga&torso=Naga&skin=Naga&legs=Naga&groin=Naga&ass=Naga&male=0&pg=1&breasts=1&gr2[0]=0;0;0&gr2[150]=255;255;255&gr2[255]=0;255;255&.png")};
         auto p_image=MakeCounting<ImageAvatar>(std::move(url));
         p_image->m_size/=2;

         p_line->SetParagraphRecord(Text::Records::ImageLeft(&*p_image));
         mp_wnd_text->Add(std::move(p_line));
         return;
      }

      if(IEquals(wl[1], "image_left3"))
      {
         auto p_line=Text::Line::CreateFromText("An animated avatar image");

         auto url=Text::ImageURL{ConstString("https://d.facdn.net/art/feve/1531437959/1531437959.feve_thomasicon.gif")};
         auto p_image=MakeCounting<ImageAvatar>(std::move(url));
         p_image->m_size/=2;

         p_line->SetParagraphRecord(Text::Records::ImageLeft(&*p_image));
         mp_wnd_text->Add(std::move(p_line));
         return;
      }

      if(IEquals(wl[1], "image_left4"))
      {
         auto p_line=Text::Line::CreateFromText("Is it a cup?\nA ro?\nA hastes?");

//         auto url=Text::ImageURL{ConstString("https://d.facdn.net/art/feve/1531437959/1531437959.feve_thomasicon.gif")};
         auto url=Text::ImageURL{ConstString("https://i.imgur.com/I0p8SvB.png")};
         auto p_image=MakeCounting<ImageAvatar>(std::move(url));
         p_image->m_size/=2;

         p_line->SetParagraphRecord(Text::Records::ImageLeft(&*p_image));
         mp_wnd_text->Add(std::move(p_line));
         return;
      }

      if(IEquals(wl[1], "spam"))
      {
//         for(unsigned i=0;i<100'000;i++)
         for(unsigned i=0;i<10'000;i++)
            mp_connection->Receive(ipsum_text);
         return;
      }
#endif

#ifdef _DEBUG
      if(IEquals(wl[1], "gmcp_avatar"))
      {
         mp_connection->Receive(ConstString(GMCP_BEGIN R"+(beip.line.image-url "https://flexiblesurvival.com/resources/avatars/render.php?&male=0&breasts=0&&.png")+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_fs"))
      {
         mp_connection->SendGMCP("beip.line.id.request", "235");
         return;
      }

      if(IEquals(wl[1], "gesslar_text"))
      {
         mp_connection->Receive(ConstString("[\x01B[38;5;40m▬▬▬▬▬▬\x01B[38;5;192m▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬\x01B[38;5;124m▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬\x01B[38;5;15m●\x01B[38;5;124m▬▬▬\x01B[0m]" CRLF));
         return;
      }
#endif

      if(IEquals(wl[1], "gmcp_beiptest1"))
      {
         mp_connection->Receive(ConstString(GMCP_BEGIN R"+(beiptest1.example.subexample { "hello":"hello" })+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_beiptest2"))
      {
         mp_connection->Receive(ConstString(GMCP_BEGIN R"+(beiptest2.example.subexample { "key":"value", "other":123.456 })+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_webview"))
      {
         mp_connection->Receive(ConstString(GMCP_BEGIN R"+(webview.open { "id":"Character editor", "url":"https://beta.flexiblesurvival.com/embedded/c/aleric", "dock":"right", "http-request-headers":{ "Session Key":"Example key", "Username":"Example username" } })+" GMCP_END));
//         mp_connection->Receive(ConstString(GMCP_BEGIN R"+(webview.open { "id":"Example WebView", "url":"https://www.example.com", "dock":"right", "http-request-headers":{ "Session Key":"Example key", "Username":"Example username" } })+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_webviewsource"))
      {
         mp_connection->Receive(ConstString(GMCP_BEGIN R"+(webview.open { "id":"Table Test", "source":"<title>Table</title><table><tr><th>Company</th><th>Contact</th><th>Country</th></tr><tr><td>Alfreds Futterkiste</td><td>Maria Anders</td><td>Germany</td></tr><tr><td>Centro comercial Moctezuma</td><td>Francisco Chang</td><td>Mexico</td></tr></table>", "dock":"right"})+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_stats"))
      {
         mp_connection->Receive(ConstString(

GMCP_BEGIN R"+(beip.stats {
   "Player":
   {
      "values":
      {
         "0_Name": { "prefix-length": 2, "string": "Bennet", "name-color":"Ansi256(56)" },
         "1_Hit Points": { "prefix-length": 2, "range": { "value":823, "max": 1000, "fill-color": "#00FF00" }, "value-color": "#345678" },
         "2_Energy Points": { "prefix-length": 2, "range": { "value": 60, "max": 100, "fill-color": "#8080FF" }, "color":"#FF0000" },
         "2_Zellas": { "prefix-length": 2, "range": { "value": 60, "max": 100, "fill-color": "#8080FF", "empty-color": "#404080", "outline-color":"#FF0000" } },
         "3_": { "prefix-length":2, "string":"" },
         "3_PP": { "prefix-length":2, "string":"30/60 \u0041 \u2648\u2640 \u849c\u8089" },
         "4_Money": { "prefix-length": 2, "int":123456, "color":"#FFFF00", "name-alignment":"right" },
         "5_Progress": { "prefix-length": 2, "progress": { "label": "75%", "value":0.75, "fill-color": "#FF0000" } },
         "6_Experience": { "prefix-length": 2, "progress": { "label": "1,234 XP", "value":0.65, "fill-color": "#C07070", "empty-color": "#804040", "outline-color":"transparent" } }
      },
      "background-color": "#002040"
   }
})+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_stats0"))
      {
         mp_connection->Receive(ConstString(
GMCP_BEGIN R"+(beip.stats {
   "Player":
   {
      "values":
      {
         "0_Name": { "prefix-length": 2, "string": "Bennie 🐀", "value-color":"#FF8080" }
      }
   }
})+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_mcmp"))
      {
         mp_connection->Receive(ConstString(
            GMCP_BEGIN R"+(Client.Media.Play {
            "name":"2020/12/UltimaV_Stones.mp3","url" : "https://pacati.net/wp-content/uploads/","type" : "music","loops" : 1,"continue" : false
         })+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_stats1"))
      {
         mp_connection->Receive(ConstString(
GMCP_BEGIN R"+(beip.stats {
   "Player":
   {
      "values": 
      {
         "0_Name": { "string": "Test2" },
         "1_Hit Points": null
      }
   }
})+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "gmcp_stats2"))
      {
         mp_connection->Receive(ConstString(GMCP_BEGIN R"+(beip.stats { "Player": { "values": null } })+" GMCP_END));
         return;
      }

      if(IEquals(wl[1], "tilemap1"))
      {
         mp_connection->Receive(ConstString(
GMCP_BEGIN R"+(beip.tilemap.info { "Castle":{"tile-url":"https://github.com/BeipDev/BeipMU/raw/master/images/Ultima5.png", "tile-size":"16,16", "map-size":"32,32", "encoding":"base64_8" } })+" GMCP_END
//GMCP_BEGIN R"+(beip.tilemap.data { "Castle":"hZM9UsMwEIUrHYaOkCu8fg+Q5E4BjBugyQyaydiM2nQ0CfgIHCUlu1rZ2k2c4TXW6tNa+yciVpgTZUIxxhs8AsKHSPPCIByI9INrnSghUTDnQ1jw/jL4vUBH6vr3rje86z/ZPmpcBMo/NFzMRJj4eoP1xvnz4jDxw3/+wOPds/Pf8/2VT/HzusY/cVOzb1PLK15z9vxBFTiPD8mjmBNXyY3Ylu+YkeFccXBLULsx4485/3uV1m1PQS0Tf4mb3NfyofxNeybml+NoXlp2WZxxXsr9zWvrOYszz/UnPEmejpfIlTdvrWRxWZ/qv73Ij/CbZ6jwFVa6Q2PPtCpUOGmFtJdjr/PRMh+5d2MtkNLN+Zf3kQaYJwNjpPx+UkKdGeZ1ljKxE7PLfOfer5f1V/0B" })+" GMCP_END

GMCP_BEGIN R"+(beip.tilemap.data { "Castle":"T09PT08FBQUFBQUFBQUFBQUFBQUFBQUFBQVPT09PTwVPpqamTwUFBQUFBQUFBQUFBQUFBQUFBQUFBU+mRERPBU+myKZPT09PT09PT09PT09PT09PT09PT09PT0TIRE8FT0REpk/HRERERERERERERERERERERERERMVPr0SvTwVPT09PT09PT09PTwUFMURERDMFBU9PT09PT09PT09PBQUFT8RPq6ydq6xPBQUxREREMwUFT6usrausT8RPBQUFBQVPRE9ERERERE8FBTFEREQzBQVPRERERK9PRE8FBQUFBU9ET1xdRFxdTwUFMURERDMFBU+rrERERLhETwUFBQUFT0S4RERERERPBQUxREREMwUFT0RERESvT0RPBQUFBQVPRE9ERJEpk08FBTFEREQzBQVPq6ypq6xPRE8FBQUFBU9ET09PT09PTwUFMU9ETzMFBU9PT09PT09ETwUFBQUFT0QFBQUFBQUFBQUFT8ZPBQUFBQUFBQUFBURPBQUFBQVPRAUFBQUFBQUFT09PT09PTwUFBQUFBQUFRE8FBQUFBU9EMjIyMjIyMgVPXF2lXF1PBTIyMjIyMgVETwUFBQUFT0RERERERERPT09ERJBERE9PT0RERERERERPBQUFBQVPRERERERERETFT0REyERET8dERERERERERE8FBQUFBU9ERERERERET09PRERERERPT09ERERERERETwUFBQUFT0QwMDAwMDAwBU+rrEREqU8FMDAwMDAFBURPBQUFBQVPRAVPT09PT08FT09PT09PTwVPT09PT08FRE8FBQUFBU9EBU/IRERETwUFBU/ETwUFBU/IRES/TwVETwUFBQUFT0QFT0SUlZZPT08x+ET4M09PT0SUm5ZPBURPBQUFBQVPRAVPRERERFxdTzFEREQzT0SSRESQRE8FRE8FBQUFBU9EBU9PRERERERPMURERDNPlJyWRERPTwVETwUFBQUFT0RERERERERcXU8xREREM09EkERERERERERPBQUFBQVPRE9E2ERPRERETzFEREQzT1tEW09E2ERPRE8FBQUFBU/GT0RERE9PT09PMURERDNPT09PT0RERE/GTwUFBU9PT09PT09EBQUFBQUxREREMwUFBQUFRE9PT09PT08FT0Svr0/HRERERERERERERERERERERERERMVPr0RETwVPr8hET09PT09PT09PT0RERE9PT09PT09PT0+vyERPBU+vr0RPBQUFBQUFBQUFREREBQUFBQUFBQUFT6+vRE8FT09PT08FBQUFBQUFogVEREQFogUFBQUFBQVPT09PTwUFBQUFBQUFBQUFBQUFBURERAUFBQUFBQUFBQUFBQUFBQ" })+" GMCP_END
));
         return;
      }

      if(IEquals(wl[1], "tilemap2"))
      {
         mp_connection->Receive(ConstString(
GMCP_BEGIN R"+(beip.tilemap.info { "Map1":{ "tile-url":"https://github.com/BeipDev/BeipMU/raw/master/images/Ultima5.png", "tile-size":"16,16", "encoding":"Hex_4", "map-size":"10,4" }})+" GMCP_END
GMCP_BEGIN R"+(beip.tilemap.data { "Map1":"0123456701234567012345670123456701234567" })+" GMCP_END
GMCP_BEGIN R"+(beip.tilemap.info { "Lighthouse 1":{ "tile-url":"https://github.com/BeipDev/BeipMU/raw/master/images/Ultima5.png", "tile-size":"16,16", "map-size":"32,32", "encoding":"zbase64_8" }})+" GMCP_END
GMCP_BEGIN R"+(beip.tilemap.data { "Lighthouse 1":"tZBBCsIwEEVXuc5g9Qqz9h9AvVNRcS0URGh13YXo1mM5bRI6xsmiWj80/eHNHz5xbjqRZWnq3fSfxqPYSMErtxdgBk5q5gPzs5+5QStywdwPLblabzrzwIXPmodEKbblcCQ8IjFbRt0YXL7rDry/H5DkvcSV4aibFXR/3xGt0T8OAIVcFvb7dUGX4xH8xGcixUn/i2HUzs+9cvy9Rk5Hk1Oap2/3W/wF" })+" GMCP_END
GMCP_BEGIN R"+(beip.tilemap.info { "Laboratory":{ "tile-url":"https://github.com/BeipDev/BeipMU/raw/master/images/FutureTiles.png", "tile-size":"32,32", "map-size":"16,16", "encoding":"zbase64_8" }})+" GMCP_END
GMCP_BEGIN R"+(beip.tilemap.data { "Laboratory":"k+ZFAXzaLKxIgE1/5ixHhtsMYBBzLFlMf+WqQIbXrxg4OLkYcq4Vi+lrg2R+MUhISjHUMDSA+FqLQULGIALCnw7RvnsnlL+cQRfEP30Syg9lsGW4fJmB4SZcngHIvwzXv50BLMBwB8yPZUgHG3eZ4Q2YX8vQzhDNcIQBZt9XBgQA8VVUkYCavo8vCvADAA" })+" GMCP_END
));
         return;
      }
// "eJyFkztSAzEMhisfho6QK/y9DpDkTgGWbYAmM3gms8u4TUeTwB6Bo6REsvyQk83wN7vyZ9l6mYjl5kSRkPf+BveA8MnTvDAJBzz94FonCgjkzH7nFry+dO2aoyMN4/swGj6Mn2wfNS4CxQMNFzMQCl9vsN40/vxzKPzwnz/wePfc+O/5/spL/Pxf4y/c1Ozb1PKK15xb/qBynMeH5JHMwlVyI7bpmzMynCsObglqN2b8Med/r9K67cmpZeJPcVPztXxKp2nPxPxqOLqXnl0WZ5yXcn/32recxZnH+hOeJM+Gp8iVd2+9ZHFZn+q/vciP8BtnKPEVVrpCuWdaFUqctELay9zruDXNR+xdrgVCuDn/8j7ClPemY4tCfD8hoM4M8zpLkdiJ2UW+s9PkWll/1R+3Ru65" })+" GMCP_END));
// "eJytkkEKwjAQRVe5TrB6hVn7D6DeqWhxXRBEaHXdhejWYzltEjrWGbpoPjT54f2ZDiHO5ZPXrFeCi3pnajjbK+N/EGT1BYiAq8j8YfoMmSekEmdMQ2hLl/2hN2/c6SZ5rCjZdhSXCU+IzZHQtArn73ECVa8zJvVB7Mq4NO0Ocv4wIzpl/hQACj5s9PvrC53FE1jEVyzBvdyLMarXr4Ms/juGpVrl4r0Ebj/Mmf4a/wJ4Ll9p" })+" GMCP_END
// "eJyT5kUBfNosrEiATX/mLEeG2wxgEHMsWUx/5apAhtevGDg4uRhyrhWL6WuDZH4xSEhKMdQwNID4WotBQsYgAsKfDtG+eyeUv5xBF8Q/fRLKD2WwZbh8mYHhJlyeAci/DNe/nQEswHAHzI9lSAcbd5nhDZhfy9DOEM1whAFm31cGBADxVVSRgJq+jy8K8AMAsu0ywg" })+" GMCP_END

      if(IEquals(wl[1], "tilemap3"))
      {
         for(unsigned i=0;i<10;i++)
            mp_connection->Receive(ConstString(GMCP_BEGIN R"+(beip.tilemap.info { "Map":{ "tile-url":"http://wiki.ultimacodex.com/images/5/55/Ultima_5_-_Tiles-pc.png", "tile-size":"16,16", "encoding":"Hex_4", "map-size":"8,5" }})+" GMCP_END GMCP_BEGIN R"+(beip.tilemap.data { "Map":"0123456701234567012345670123456701234567" })+" GMCP_END));
         return;
      }

      mp_wnd_text->AddHTML("<font color='aqua'>Unknown test");
      return;
   }

   if(IEquals(command, "tilemap"))
   {
      if(wl.Count()==2)
      {
         if(IEquals(wl[1], "on"))
         {
            m_tile_maps_enabled=true;
            mp_wnd_text->AddHTML("<icon information>TileMap tag parsing <font color='green'>ON</font>");
            return;
         }
         else if(IEquals(wl[1], "off"))
         {
            m_tile_maps_enabled=false;
            mp_wnd_text->AddHTML("<icon information>TileMap tag parsing <font color='red'>OFF</font>");
            return;
         }
      }
     mp_wnd_text->AddHTML("<icon information>Usage: '/tilemap on/off' to enable/disable tilemap tag parsing");
     return;
   }

   if(g_ppropGlobal->fSendUnrecognizedCommands())
      SendLine(full_line);
   else
      mp_wnd_text->AddHTML("<icon error> <font color='red'>Unrecognized Command, use // to send text directly to the mu*, /help for a list of commands, or set 'Send unrecognized commands' in settings/input window");
}
catch(const std::exception &message)
{
   HybridStringBuilder error("<icon error> <font color='red'>Command error: </font>", message);
   mp_wnd_text->AddHTML(error);
}
