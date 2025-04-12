#include "Main.h"
#include "TextToLine.h"
#include "CodePages.h"

TextToLine::TextToLine(Wnd_Main &wndMain)
 : m_wnd_main(wndMain),
   m_parserAnsi(g_ppropGlobal->propAnsi())
{
}

TextToLine::~TextToLine()
{
}

UniquePtr<Text::Line> TextToLine::Parse(ConstString string, bool use_pueblo, Prop::Server::Encoding encoding)
{
   Streams::Input stream(string);
   Text::HTMLParser html(stream, m_line_builder);

   if(g_ppropGlobal->propAnsi().fResetOnNewLine())
      m_parserAnsi.Reset();
   else
      m_line_builder.SetStyleState(m_prev_line_state);

   while(unsigned char c=stream.CharGet())
   {
      switch(c)
      {
         case CHAR_ESC: // ESC
            if(stream.CharSpy()=='[')
               m_parserAnsi.Parse(stream, m_line_builder);
            break;

         case CHAR_BEEP: // Beep
            g_ppropGlobal->propAnsi().PlayBeep();
            break;

         case CHAR_TAB: m_line_builder.AppendTab(); break;

         // Take a sequence of CR +LF and turn it into a CRLF record
         case CHAR_CR:
            stream.CharSkip(CHAR_LF);
            // Fallthrough
         case CHAR_LF: m_line_builder.AppendNewLine(); break;

         case '&': // Possible entity
            if(use_pueblo)
            {
               if(auto entity=html.ParseEntity())
               {
                  m_line_builder.AppendString(entity);
                  break;
               }
            }
            m_line_builder.AppendChar(c);
            break;

         case '<': // Style tag?
         {
            if(use_pueblo)
            {
               if(HandlePuebloTag(stream, html))
                  break;

               if(html.ParseTag())
                  break;
            }

            __fallthrough;
         }

         default:
         {
            // All charsets are the same in this range
            if(c<128)
               m_line_builder.AppendChar(c);
            else
            {
               switch(encoding)
               {
                  case Prop::Server::Encoding::UTF8:
                     stream.CharBack(); // Put back the first UTF8 char so it parses ok
                     if(!m_line_builder.ParseUTF8Emoji(stream))
                     {
                        stream.CharGet();
                        m_line_builder.AppendString("(Invalid UTF8 code)�");
                     }
                     break;

                  case Prop::Server::Encoding::CP1252:
                     m_line_builder.AppendString(CP1252toUTF8(c));
                     break;

                  case Prop::Server::Encoding::CP437:
                     m_line_builder.AppendString(CP437toUTF8(c));
                     break;
               }
            }
            break;
         }
      }
   }

   m_prev_line_state=m_line_builder.GetStyleState();
   return m_line_builder.Create();
}

void *Pueblo_Send::QueryInterface(TypeID id)
{
   if(id==GetTypeID<Pueblo_Send>())
      return this;
   return nullptr;
}

bool Pueblo_Send::GetTooltip(StringBuilder &string) const
{
   ConstString tip;
   ConstString remaining=m_hint;
   if(remaining.Split('|', tip, remaining))
      string(tip);
   else
      string(m_hint);

   string(" (Sends: ", OnLButton(), ')', CRLF);
   return true;
}

ConstString Pueblo_Send::OnLButton() const
{
   ConstString defSend;
   ConstString remaining=m_send;
   if(remaining.Split('|', defSend, remaining))
      return defSend;

   return m_send;
}

ConstString Pueblo_Send::OnRButton(Window wnd, int2 position) const
{
   Collection<ConstString> sends;
   Collection<ConstString> hints;

   // Fill in the sends
   {
      ConstString send;
      ConstString remaining=m_send;
      while(remaining.Split('|', send, remaining))
         sends.Push(send);
      sends.Push(remaining);
   }

   // Fill in the hints
   {
      ConstString hint;
      ConstString remaining=m_hint;
      while(remaining.Split('|', hint, remaining))
         hints.Push(hint);
      hints.Push(remaining);
   }

   if(sends.Count()==hints.Count()-1) // There's an extra hint, it's the tooltip
      hints.Delete(0); // Remove it for the menu

   if(sends.Count()!=hints.Count())
   {
      sends.Empty();
      hints.Empty();

      sends.Push("");
      hints.Push("Send tag error, mismatch between send and hint counts");
   }

   PopupMenu menu;
   int index=0;
   for(auto hint : hints)
   {
      FixedStringBuilder<256> string(hint, " (Sends: ", sends[index], ')');
      menu.Append(0, (UINT_PTR)++index, string); // Index starts at 1
   }

   int id=TrackPopupMenu(menu, TPM_RETURNCMD, wnd.ClientToScreen(position), wnd, nullptr);
   if(id!=0)
      return sends[id-1];

   return {};
}

bool TextToLine::HandlePuebloTag(Streams::Input &stream, Text::HTMLParser &html)
{
   Streams::Input::PosRestorer restorer(stream);

   bool start=true;
   if(stream.CharSkip('/'))
      start=false;

   ConstString tag;
   if(!html.ParseName(tag))
      return false;

   if(IEquals(tag, "pre"))
   {
      if(!start && m_line_builder.Get<Text::Records::Underline>())
      {
         m_line_builder.Set(Text::Records::Underline{false});
         m_line_builder.SetURL(nullptr);
      }
   }
   else if(IEquals(tag, "xch_mudtext"))
   {
   }
   else if(IEquals(tag, "xch_page"))
   {
      // Eat all attributes
      Text::HTMLParser::Attribute attribute;
      while(html.ParseAttribute(attribute))
      {
      }
   }
   else if(IEquals(tag, "a"))
   {
      if(start)
      {
         Text::HTMLParser::Attribute attribute;
         while(html.ParseAttribute(attribute))
         {
            if(IEquals(attribute.m_name, "xch_cmd"))
            {
               m_line_builder.SetURL(MakeUnique<Text::Records::URLData>(*MakeCounting<Pueblo_Send>(attribute.m_value, "")));
               m_line_builder.Set(Text::Records::Underline{true});
               m_in_pueblo_A_tag=true;
            }
            else
               return false;
         }
      }
      else
      {
         if(!m_in_pueblo_A_tag)
            return false; // Default HTML tag handling
         m_in_pueblo_A_tag=false;

         m_line_builder.Set(Text::Records::Underline{false});
         m_line_builder.SetURL(nullptr);
      }
   }
   else if(IEquals(tag, "img"))
   {
      if(start)
      {
         Text::HTMLParser::Attribute attribute;
         while(html.ParseAttribute(attribute))
         {
            if(IEquals(attribute.m_name, "xch_mode"))
            {
            }
            else if(IEquals(attribute.m_name, "xch_graph"))
            {
            }
            else
               return false;
         }
      }
   }
   else if(IEquals(tag, "send"))
   {
      // Test cases:
      // <send href="+finger Maxie|+info Maxie|+kinks Maxie|look Maxie" hint="Right click to see menu|Finger Maxie|Info Maxie|Kinks Maxie|Look at Maxie">Maxie</send> 
      if(start)
      {
         ConstString text;
         ConstString hint;

         Text::HTMLParser::Attribute attribute;
         while(true)
         {
            if(html.ParseAttribute(attribute))
            {
               if(IEquals(attribute.m_name, "hint"))
                  hint=attribute.m_value;
               else if(IEquals(attribute.m_name, "href"))
                  text=attribute.m_value;
               else
                  return false;
            }
            else if(text || !html.ParseValue(text))
               break;
         }

         m_line_builder.SetURL(MakeUnique<Text::Records::URLData>(*MakeCounting<Pueblo_Send>(text, hint)));
         m_line_builder.Set(Text::Records::Underline{true});
      }
      else
      {
         m_line_builder.Set(Text::Records::Underline{false});
         m_line_builder.SetURL(nullptr);
      }
   }
   else if(IEquals(tag, "br"))
   {
   }
   else
      return false;


   if(stream.CharGet()!='>')
      return false;

   restorer.NoRestore();
   return true;
}
