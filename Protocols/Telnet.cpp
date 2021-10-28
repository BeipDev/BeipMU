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

const BYTE TELNET_IAC   = 255; // FF Interpret As Command
const BYTE TELNET_DONT  = 254; // FE 
const BYTE TELNET_DO    = 253; // FD
const BYTE TELNET_WONT  = 252; // FC
const BYTE TELNET_WILL  = 251; // FB
const BYTE TELNET_SB    = 250; // FA Subnegotiation
const BYTE TELNET_GA    = 249; // F9 Go Ahead
const BYTE TELNET_EL    = 248; // F8 Erase Line
const BYTE TELNET_EC    = 247; // F7 Erase Character
const BYTE TELNET_AYT   = 246; // F6 Are You There
const BYTE TELNET_AO    = 245; // F5 Abort Output
const BYTE TELNET_IP    = 244; // F4 Interrupt Process
const BYTE TELNET_BREAK = 243; // F3 Break
const BYTE TELNET_DM    = 242; // F2 Data Mark
const BYTE TELNET_NOP   = 241; // F1 No op
const BYTE TELNET_SE    = 240; // F0 End Subnegotiation
const BYTE TELNET_EOR   = 239; // EF End Of Record
const BYTE TELNET_ABORT = 238; // EE Abort
const BYTE TELNET_SUSP  = 237; // ED Suspend Process
const BYTE TELNET_EOF   = 236; // EC End of File

const BYTE TELNET_SYNCH = 242; // F2 Synchronize

const BYTE TELOPT_BINARY         =  0;        /* 8-bit data path */
const BYTE TELOPT_ECHO           =  1;        /* echo */
const BYTE TELOPT_RCP            =  2;        /* prepare to reconnect */
const BYTE TELOPT_SGA            =  3;        /* suppress go ahead */
const BYTE TELOPT_NAMS           =  4;        /* approximate message size */
const BYTE TELOPT_STATUS         =  5;        /* give status */
const BYTE TELOPT_TM             =  6;        /* timing mark */
const BYTE TELOPT_RCTE           =  7;        /* remote controlled transmission and echo */
const BYTE TELOPT_NAOL           =  8;        /* negotiate about output line width */
const BYTE TELOPT_NAOP           =  9;        /* negotiate about output page size */
const BYTE TELOPT_NAOCRD         = 10;        /* negotiate about CR disposition */
const BYTE TELOPT_NAOHTS         = 11;        /* negotiate about horizontal tabstops */
const BYTE TELOPT_NAOHTD         = 12;        /* negotiate about horizontal tab disposition */
const BYTE TELOPT_NAOFFD         = 13;        /* negotiate about formfeed disposition */
const BYTE TELOPT_NAOVTS         = 14;        /* negotiate about vertical tab stops */
const BYTE TELOPT_NAOVTD         = 15;        /* negotiate about vertical tab disposition */
const BYTE TELOPT_NAOLFD         = 16;        /* negotiate about output LF disposition */
const BYTE TELOPT_XASCII         = 17;        /* extended ascic character set */
const BYTE TELOPT_LOGOUT         = 18;        /* force logout */
const BYTE TELOPT_BM             = 19;        /* byte macro */
const BYTE TELOPT_DET            = 20;        /* data entry terminal */
const BYTE TELOPT_SUPDUP         = 21;        /* supdup protocol */
const BYTE TELOPT_SUPDUPOUTPUT   = 22;        /* supdup output */
const BYTE TELOPT_SNDLOC         = 23;        /* send location */
const BYTE TELOPT_TTYPE          = 24;        /* terminal type */
const BYTE TELOPT_EOR            = 25;        /* end or record */
const BYTE TELOPT_TUID           = 26;        /* TACACS user identification */
const BYTE TELOPT_OUTMRK         = 27;        /* output marking */
const BYTE TELOPT_TTYLOC         = 28;        /* terminal location number */
const BYTE TELOPT_3270REGIME     = 29;        /* 3270 regime */
const BYTE TELOPT_X3PAD          = 30;        /* X.3 PAD */
const BYTE TELOPT_NAWS           = 31;        /* window size */
const BYTE TELOPT_TSPEED         = 32;        /* terminal speed */
const BYTE TELOPT_LFLOW          = 33;        /* remote flow control */
const BYTE TELOPT_LINEMODE       = 34;        /* Linemode option */
const BYTE TELOPT_XDISPLOC       = 35;        /* X Display Location */
const BYTE TELOPT_OLD_ENVIRON    = 36;        /* Old - Environment variables */
const BYTE TELOPT_AUTHENTICATION = 37;        /* Authenticate */
const BYTE TELOPT_ENCRYPT        = 38;        /* Encryption option */
const BYTE TELOPT_NEW_ENVIRON    = 39;        /* New - Environment variables */
const BYTE TELOPT_CHARSET        = 42;
const BYTE TELOPT_MSSP           = 70;        // MUD Server Status Protocol
const BYTE TELOPT_MCCP1          = 85;
const BYTE TELOPT_MCCP2          = 86;        // Mud Client Compression Protocol 2
const BYTE TELOPT_ZMP            = 93;        // Zenith Mud Protocol
const BYTE TELOPT_GMCP           = 201;       // C9
const BYTE TELOPT_EXOPL         = 255;        /* extended-options-list */

const BYTE TELQUAL_IS        = 0;        /* option is... */
const BYTE TELQUAL_SEND      = 1;        /* send option */
const BYTE TELQUAL_INFO      = 2;        /* ENVIRON: informational version of IS */

TelnetParser::TelnetParser(INotify &notify)
: m_notify(notify)
{
}

void TelnetParser::Reset()
{
   m_buffer.Empty();
   m_state=State::Normal;
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
                        "\xFF\xFA\xC9" R"(Core.Supports.Set [ "Beip.Stats 1", "Beip.Tilemap 1", "Client.Media 1" ])" "\xFF\xF0");
                     continue;
               }
               #if _DEBUG
               OutputDebugString(FixedStringBuilder<256>("Unsupported Will Telnet option:", (int)(c), " ", char(c), '\n'));
               #endif
               break;

            case State::Do:
            {
               switch(c)
               {
                  case TELOPT_CHARSET:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_WILL, TELOPT_CHARSET>);
                     m_state=State::Normal;
                     continue;

                  case TELOPT_NAWS:
                     m_notify.OnTelnet(MakeString<TELNET_IAC, TELNET_WILL, TELOPT_NAWS,
                                                  TELNET_IAC, TELNET_SB, TELOPT_NAWS,
                                                  0, 80, // These values are arbitrary as the window size can change dynamically
                                                  0, 100,
                                                  TELNET_IAC, TELNET_SE>);
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
                        Swap(charset, charsets);

                     if(charset=="UTF-8")
                     {
                        if(limit && limit_encoding!=Prop::Server::Encoding::UTF8)
                           continue;

                        m_notify.OnTelnet(FixedStringBuilder<256>(MakeString<TELNET_IAC, TELNET_SB, TELOPT_CHARSET, 2>, charset, MakeString<TELNET_IAC, TELNET_SE>));
                        m_notify.OnEncoding(Prop::Server::Encoding::UTF8);
                        break;
                     }
#ifdef _DEBUG
                     if(charset=="ISO-8859-1" || charset=="ISO-8859-15")
                        continue;
                     if(charset=="US-ASCII" || charset=="ASCII")
                        continue;
                     if(charset=="x-penn-def" || charset=="ANSI_X3.4-1968")
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
