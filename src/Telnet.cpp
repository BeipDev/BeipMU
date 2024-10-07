//
// Telnet Processing
//
// https://tools.ietf.org/html/rfc854 - Telnet Protocol Specification
// https://tools.ietf.org/html/rfc855 - Telnet Option Specification
// https://tools.ietf.org/html/rfc2066 - Telnet Charset
// http://www.zuggsoft.com/zmud/mxp.htm - MXP

#include "Main.h"
#include "Telnet.h"

template<unsigned char... values>
constexpr const char MakeString[sizeof...(values)+1]={values..., 0};

// Telnet commands (pulled from the internet)

const uint8 TELNET_IAC   = 255; // FF Interpret As Command
const uint8 TELNET_DONT  = 254; // FE 
const uint8 TELNET_DO    = 253; // FD
const uint8 TELNET_WONT  = 252; // FC
const uint8 TELNET_WILL  = 251; // FB
const uint8 TELNET_SB    = 250; // FA Subnegotiation
const uint8 TELNET_GA    = 249; // F9 Go Ahead
const uint8 TELNET_EL    = 248; // F8 Erase Line
const uint8 TELNET_EC    = 247; // F7 Erase Character
const uint8 TELNET_AYT   = 246; // F6 Are You There
const uint8 TELNET_AO    = 245; // F5 Abort Output
const uint8 TELNET_IP    = 244; // F4 Interrupt Process
const uint8 TELNET_BREAK = 243; // F3 Break
const uint8 TELNET_DM    = 242; // F2 Data Mark
const uint8 TELNET_NOP   = 241; // F1 No op
const uint8 TELNET_SE    = 240; // F0 End Subnegotiation
const uint8 TELNET_EOR   = 239; // EF End Of Record
const uint8 TELNET_ABORT = 238; // EE Abort
const uint8 TELNET_SUSP  = 237; // ED Suspend Process
const uint8 TELNET_EOF   = 236; // EC End of File

const uint8 TELNET_SYNCH = 242; // F2 Synchronize

const uint8 TELOPT_BINARY         =  0;        /* 8-bit data path */
const uint8 TELOPT_ECHO           =  1;        /* echo */
const uint8 TELOPT_RCP            =  2;        /* prepare to reconnect */
const uint8 TELOPT_SGA            =  3;        /* suppress go ahead */
const uint8 TELOPT_NAMS           =  4;        /* approximate message size */
const uint8 TELOPT_STATUS         =  5;        /* give status */
const uint8 TELOPT_TM             =  6;        /* timing mark */
const uint8 TELOPT_RCTE           =  7;        /* remote controlled transmission and echo */
const uint8 TELOPT_NAOL           =  8;        /* negotiate about output line width */
const uint8 TELOPT_NAOP           =  9;        /* negotiate about output page size */
const uint8 TELOPT_NAOCRD         = 10;        /* negotiate about CR disposition */
const uint8 TELOPT_NAOHTS         = 11;        /* negotiate about horizontal tabstops */
const uint8 TELOPT_NAOHTD         = 12;        /* negotiate about horizontal tab disposition */
const uint8 TELOPT_NAOFFD         = 13;        /* negotiate about formfeed disposition */
const uint8 TELOPT_NAOVTS         = 14;        /* negotiate about vertical tab stops */
const uint8 TELOPT_NAOVTD         = 15;        /* negotiate about vertical tab disposition */
const uint8 TELOPT_NAOLFD         = 16;        /* negotiate about output LF disposition */
const uint8 TELOPT_XASCII         = 17;        /* extended ascic character set */
const uint8 TELOPT_LOGOUT         = 18;        /* force logout */
const uint8 TELOPT_BM             = 19;        /* uint8 macro */
const uint8 TELOPT_DET            = 20;        /* data entry terminal */
const uint8 TELOPT_SUPDUP         = 21;        /* supdup protocol */
const uint8 TELOPT_SUPDUPOUTPUT   = 22;        /* supdup output */
const uint8 TELOPT_SNDLOC         = 23;        /* send location */
const uint8 TELOPT_TTYPE          = 24;        /* terminal type */
const uint8 TELOPT_EOR            = 25;        /* end or record */
const uint8 TELOPT_TUID           = 26;        /* TACACS user identification */
const uint8 TELOPT_OUTMRK         = 27;        /* output marking */
const uint8 TELOPT_TTYLOC         = 28;        /* terminal location number */
const uint8 TELOPT_3270REGIME     = 29;        /* 3270 regime */
const uint8 TELOPT_X3PAD          = 30;        /* X.3 PAD */
const uint8 TELOPT_NAWS           = 31;        /* window size */
const uint8 TELOPT_TSPEED         = 32;        /* terminal speed */
const uint8 TELOPT_LFLOW          = 33;        /* remote flow control */
const uint8 TELOPT_LINEMODE       = 34;        /* Linemode option */
const uint8 TELOPT_XDISPLOC       = 35;        /* X Display Location */
const uint8 TELOPT_OLD_ENVIRON    = 36;        /* Old - Environment variables */
const uint8 TELOPT_AUTHENTICATION = 37;        /* Authenticate */
const uint8 TELOPT_ENCRYPT        = 38;        /* Encryption option */
const uint8 TELOPT_NEW_ENVIRON    = 39;        /* New - Environment variables */
const uint8 TELOPT_CHARSET        = 42;
const uint8 TELOPT_MSDP           = 69;
const uint8 TELOPT_MSSP           = 70;        // MUD Server Status Protocol
const uint8 TELOPT_MCCP1          = 85;
const uint8 TELOPT_MCCP2          = 86;        // Mud Client Compression Protocol 2
const uint8 TELOPT_MCCP3          = 87;
const uint8 TELOPT_ZMP            = 93;        // Zenith Mud Protocol
const uint8 TELOPT_GMCP           = 201;       // C9
const uint8 TELOPT_EXOPL         = 255;        /* extended-options-list */

const uint8 TELQUAL_IS        = 0;        /* option is... */
const uint8 TELQUAL_SEND      = 1;        /* send option */
const uint8 TELQUAL_INFO      = 2;        /* ENVIRON: informational version of IS */

TelnetParser::TelnetParser(INotify &notify)
: m_notify(notify)
{
}

void TelnetParser::Reset()
{
   m_buffer.Empty();
   m_state=State::Normal;
   m_do_naws=false;
}

void TelnetParser::Parse(Array<const char> buffer)
{
   for(unsigned char c : buffer)
   {
      if(m_state!=State::Normal)
      {
         switch(m_state)
         {
            case State::IAC:
            {
               switch(c)
               {
                  case TELNET_GA:
                  case TELNET_EOR:
                  {
                     if(auto prompt=GetPartial())
                        m_notify.OnPrompt(prompt); // (don't reset the buffer because we continue)
                     m_state=State::Normal;
                     continue;
                  }

                  case TELNET_DONT: m_state=State::Dont; continue;
                  case TELNET_DO:   m_state=State::Do; continue;
                  case TELNET_WONT: m_state=State::Wont; continue;
                  case TELNET_WILL: m_state=State::Will; continue;

                  case TELNET_NOP:  m_state=State::Normal; continue;
                  case TELNET_IAC:  m_state=State::Normal; m_buffer.Push(char(c)); continue;
                  case TELNET_SE:   m_state=State::Normal; continue;
                  case TELNET_SB:   m_state=State::SB; continue;
               }
               break;
            }

            case State::Will:
               switch(c)
               {
                  case TELOPT_BINARY:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_DO, TELOPT_BINARY>);
                     m_state=State::Normal;
                     continue;

                  case TELOPT_CHARSET:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_DO, TELOPT_CHARSET>);
                     m_state=State::Normal;
                     continue;

                  case TELOPT_EOR:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_DO, TELOPT_EOR>);
                     m_state=State::Normal;
                     continue;

                  case TELOPT_SGA:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_DONT, TELOPT_SGA>);
                     m_state=State::Normal;
                     continue;

                  case TELOPT_GMCP:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_DO, TELOPT_GMCP>);
                     m_state=State::Normal;
                     m_notify.OnTelnet("\xFF\xFA\xC9" R"(Core.Hello {"client":"Beip", "version":")" STRINGIZE(BUILD_NUMBER) R"("})" "\xFF\xF0"
                        "\xFF\xFA\xC9" R"(Core.Supports.Set [ "WebView 1", "Beip.Stats 1", "Beip.Tilemap 1", "Beip.Id 1", "Client.Media 1" ])" "\xFF\xF0");
                     continue;
               }
               #if _DEBUG
               OutputDebugString(FixedStringBuilder<256>("Unsupported Will Telnet option:", (int)(c), " ", Strings::Hex32(c, 2), '\n'));
               #endif
               break;

            case State::Do:
            {
               switch(c)
               {
                  case TELOPT_BINARY:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_WILL, TELOPT_BINARY>);
                     m_state=State::Normal;
                     continue;

                  case TELOPT_CHARSET:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_WILL, TELOPT_CHARSET>);
                     m_state=State::Normal;
                     continue;

                  case TELOPT_NAWS:
                     if(!m_do_naws)
                     {
                        m_do_naws=true;
                        m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_WILL, TELOPT_NAWS>);
                        m_notify.OnDoNAWS();
                     }
                     m_state=State::Normal;
                     continue;

                  case TELOPT_TTYPE:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_WILL, TELOPT_TTYPE>);
                     m_state=State::Normal;
                     continue;

#if 0
                  case TELOPT_EOR:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_WILL, TELOPT_EOR>);
                     m_state=State::Normal;
                     continue;

                  case TELOPT_SGA:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_WILL, TELOPT_SGA>);
                     m_state=State::Normal;
                     continue;
#endif
               }
               #if _DEBUG
               OutputDebugString(FixedStringBuilder<256>("Unsupported Do Telnet option:", (int)(c), '\n'));
               #endif

               m_notify.OnTelnet(FixedStringBuilder<16>(MakeString<TELNET_IAC, TELNET_WONT>, c));
               m_state=State::Normal;
               continue;
            }

            case State::Dont:
            case State::Wont:
               break;

            case State::SB:
               if(c==TELOPT_CHARSET) { m_state=State::SB_CHARSET; continue; }
               if(c==TELOPT_TTYPE) { m_state=State::SB_TTYPE; continue; }
               if(c==TELNET_IAC) { m_state=State::SB_IAC; continue; }
               if(c==TELOPT_GMCP) { m_state=State::SB_GMCP; m_sb_start=m_buffer.Count(); continue; }

               m_state=State::SB_WaitForIAC; // Unknown sideband, wait for the IAC and ignore it
               continue;

            case State::SB_WaitForIAC:
               if(c==TELNET_IAC) { m_state=State::SB_IAC; continue; }
               continue; // Eat unknown chars until se see the IAC

            case State::SB_IAC:
               if(c==TELNET_SE) { m_state=State::Normal; continue; }
               #if _DEBUG
               OutputDebugString("State::SB_IAC did not see a TELNET_SE\n");
               #endif
               break;

            case State::SB_GMCP:
               if(c==TELNET_IAC)
                  m_state=State::SB_GMCP_Request_IAC;
               else
                  m_buffer.Push(char(c));
               continue;
            case State::SB_GMCP_Request_IAC:
               if(c==TELNET_SE)
               {
                  m_notify.OnGMCP(ConstString(&m_buffer[m_sb_start], m_buffer.Count()-m_sb_start));
                  m_buffer.Pop(m_buffer.Count()-m_sb_start);
               }
               break;

            case State::SB_CHARSET:
               if(c==0x01) // Request
               {
                  m_state=State::SB_CHARSET_Request_List;
                  m_sb_start=m_buffer.Count();
                  continue;
               }
               break;

            case State::SB_CHARSET_Request_List:
               if(c==TELNET_IAC)
                  m_state=State::SB_CHARSET_Request_IAC;
               else
                  m_buffer.Push(char(c));
               continue;
            case State::SB_CHARSET_Request_IAC:
               if(c==TELNET_SE)
               {
                  if(m_buffer.Count()<m_sb_start+1)
                  {
                     Assert(false);
                     break; // No charsets?
                  }

                  char separator=m_buffer[m_sb_start];
                  auto charsets=GetPartial().WithoutFirst(m_sb_start+1);

                  bool limit=false;
                  auto limit_encoding=Prop::Server::Encoding::UTF8;
                  if(auto *p_server=m_notify.GetServer();p_server->fLimitTelnetCharset())
                  {
                     limit=true;
                     limit_encoding=p_server->eEncoding();
                  }

                  while(charsets)
                  {
                     ConstString charset;
                     if(!charsets.Split(separator, charset, charsets))
                        std::swap(charset, charsets);

                     if(IEquals(charset, "UTF-8"))
                     {
                        if(limit && limit_encoding!=Prop::Server::Encoding::UTF8)
                           continue;

                        m_notify.OnTelnet(FixedStringBuilder<256>(MakeString<TELNET_IAC, TELNET_SB, TELOPT_CHARSET, 2>, charset, MakeString<TELNET_IAC, TELNET_SE>));
                        m_notify.OnEncoding(Prop::Server::Encoding::UTF8);
                        break;
                     }
#ifdef _DEBUG
                     if(IEquals(charset, "ISO-8859-1") || IEquals(charset, "ISO-8859-15"))
                        continue;
                     if(IEquals(charset, "US-ASCII") || IEquals(charset, "ASCII"))
                        continue;
                     if(IEquals(charset, "x-penn-def") || IEquals(charset, "ANSI_X3.4-1968"))
                        continue;
#endif
                     Assert(false); // If this hits, add it to the list
                  }

                  m_buffer.Pop(m_buffer.Count()-m_sb_start);
                  m_state=State::Normal;
                  continue;
               }
               break;

            case State::SB_TTYPE:
               if(c==TELQUAL_SEND)
               {
                  m_notify.OnTelnet(FixedStringBuilder<256>(MakeString<TELNET_IAC, TELNET_SB, TELOPT_TTYPE, TELQUAL_IS>, g_ppropGlobal->pclTelnet_TType(), MakeString<TELNET_IAC, TELNET_SE>));
                  m_state=State::SB_WaitForIAC; continue;
               }
               break;

            case State::SE:
               m_state=State::Normal;
               continue;
         }
         m_state=State::Normal; // Don't know what to do, so reset state and fall through
         continue;
      }

      switch(c)
      {
         case 0: continue; // Eat NULL characters
         case TELNET_IAC: m_state=State::IAC; continue; // IAC
         case CHAR_CR: continue; // Eat these, as only the LineFeed moves us to the next line.
         case CHAR_LF: m_notify.OnLine(GetPartial()); m_buffer.Empty(); continue;
      }
      m_buffer.Push(char(c));
   }
}

void TelnetParser::SendNAWS(uint16_2 size)
{
   // TELNET_IAC TELNET_SB TELOPT_NAWS (16-bits X) (16-bits Y) TELNET_IAC TELNET_SE
   m_notify.OnTelnet(FixedStringBuilder<256>("\xFF\xFA\x1F", uint8(size.x>>8), uint8(size.x), uint8(size.y>>8), uint8(size.y), "\xFF\xF0"));
}
