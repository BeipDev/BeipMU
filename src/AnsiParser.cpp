//
// Text Parsing
//

#include "Main.h"
#include "TextToLine.h"

//
//
//

static constexpr const Color s_defaultColors[16]=
{
   RGB(  0,  0,  0), RGB(128,  0,  0), RGB(  0,128,  0), RGB(128,128,  0), RGB(  0,  0,128), RGB(128,  0,128), RGB(  0,128,128), RGB(192,192,192), 
   RGB(128,128,128), RGB(255,  0,  0), RGB(  0,255,  0), RGB(255,255,  0), RGB(  0,  0,255), RGB(255,  0,255), RGB(  0,255,255), RGB(255,255,255), 
};

Color Translate256(unsigned i)
{
   if(i<=15)
      return s_defaultColors[i];
   if(i<=231) // 6x6x6 cube
   {
      i-=16;
      uint3 color(i/36, (i/6)%6, i%6);
      static const unsigned intensities[]={ 0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF };
      return Color(intensities[color.x], intensities[color.y], intensities[color.z]);
   }
   if(i<=255) // Grayscale ramp
   {
      i-=232;
      unsigned value=0x08+i*10;
      return Color(value, value, value);
   }

   return Colors::Red; // Invalid input
}

bool Translate8BitOr24BitColor(Streams::Input &input, Color &result)
{
   unsigned attribute;
   if(input.CharGet()!=';' || !input.Parse(attribute))
      return false;
   if(attribute==5) // 256-color ansi
   {
      unsigned color;
      if(input.CharGet()!=';' || !input.Parse(color))
         return false;
      result=Translate256(color);
      return true;
   }
   else if(attribute==2) // 24-bit color ansi
   {
      unsigned red, green, blue;
      if(input.CharGet()!=';' || !input.Parse(red) || input.CharGet()!=';' || !input.Parse(green) || input.CharGet()!=';' || !input.Parse(blue))
         return false;
      result=Color(red, green, blue);
      return true;
   }
   return false;
}

struct StateChanger
{
   StateChanger(AnsiParser::State &state) : m_state(state) { }

   void Reset()
   {
      Bold(false);
      Faint(false);
      Italic(false);
      Underline(false);
      Strikeout(false);
      Blink(AnsiParser::Blink_None);
      Reverse(false);
   }

   void Foreground(unsigned i) { m_change_foreground=true; m_state.m_foreground=i; }
   void Foreground(Color clr) { m_change_foreground=true; m_state.m_foreground=100; m_state.m_color_foreground=clr; }

   void Bold(bool f) { if(m_state.m_bold!=f) { m_change_foreground=true; m_state.m_bold=f; } }
   void Faint(bool f) { if(m_state.m_faint!=f) { m_change_foreground=true; m_state.m_faint=f; } }
   void Italic(bool f) { if(m_state.m_italic!=f) { m_change_italic=true; m_state.m_italic=f; } }
   void Underline(bool f) { if(m_state.m_underline!=f) { m_change_underline=true; m_state.m_underline=f; } }
   void Strikeout(bool f) { if(m_state.m_strikeout!=f) { m_change_strikeout=true; m_state.m_strikeout=f; } }
   void Blink(AnsiParser::eBlink f) { if(m_state.m_blink!=f) { m_change_blink=true; m_state.m_blink=f; } }
   void Reverse(bool f) { m_state.m_reverse=f; }

   bool m_change_underline{};
   bool m_change_italic{};
   bool m_change_blink{};
   bool m_change_foreground{};
   bool m_change_strikeout{};

private:
   AnsiParser::State &m_state;
};

void AnsiParser::Parse(Streams::Input &ts, Text::LineBuilder &line_builder)
{
   if(!m_prop_ansi.fParse())
      return;

   StateChanger change(m_state);

   auto color   =line_builder.Get<Text::Records::Color>();
   auto color_bg=line_builder.Get<Text::Records::ColorBg>();

   if(m_state.m_reverse) // If colors were reversed, swap them back to pretend everything is normal for parsing
      std::swap<Windows::Color>(color, color_bg);

   switch(ts.CharGet())
   {
      case '[':
      {
         unsigned attribute=0;

         bool reset_hack=ts.CharSpy()=='m'; // When we see [m, we pretend it's [0m

         while(true)
         {
            if(ts.CharSkip(';'))
               continue;

            if(ts.Parse(attribute) || reset_hack)
            {
               // Attribute?
               if(IsBetween(attribute, 0U, 65U))
               {
                  switch(attribute)
                  {
                     case 0:
                        change.Reset();

                        // According to most web pages, the colors are supposed to reset
                        change.Foreground(7); change.m_change_foreground=false;
                        color=Colors::Foreground;
                        color_bg=Colors::Transparent;
                        break;

                     case 1: change.Bold(true); break;
                     case 2: change.Faint(true); break;
                     case 3: change.Italic(true); break;
                     case 4: change.Underline(true); break;
                     case 5: change.Blink(Blink_Slow); break; // (less than 150 per minute)
                     case 6: change.Blink(Blink_Fast); break; // Rapidly blinking (150 per minute)
                     case 7: change.Reverse(true); break; // Negative Image
                     case 8: change.Reverse(true); break; // Supposed to be invisible!
                     case 9: change.Strikeout(true); break;
                     // case 10: Primary font
                     // case 11-20: Alternate fonts
                     //case 21: Doubly underlined
                     case 22: change.Bold(false); change.Faint(false); break; // Neither bold nor faint
                     case 23: change.Italic(false); break; // not italicized, not fraktur
                     case 24: change.Underline(false); break;
                     case 25: change.Blink(Blink_None); break;
                     // case 26: Reserved for proportional spacing
                     case 27: change.Reverse(false); break; // Opposite of 7
                     case 28: change.Reverse(false); break; // Revealed characters (opposite of 8)
                     case 29: change.Strikeout(false); break;
                     // 30-37 - foreground color change (below)
                     case 38: // Look for a 38;[5 or 2];#m style code
                     {
                        Color color;
                        if(!Translate8BitOr24BitColor(ts, color))
                           return;
                        change.Foreground(color);
                        break;
                     }
                     case 39: // 39: Default foreground color (implementation defined)
                        change.Foreground(7); change.m_change_foreground=false;
                        color=Colors::Foreground;
                        break; 
                     // 40-47 - background colors (below)
                     case 48: // Look for a 48;[5 or 2];#m style code
                     {
                        if(!Translate8BitOr24BitColor(ts, color_bg))
                           return;
                        break;
                     }
                     case 49: // 49: Default background color (implementation defined)
                        color_bg=Colors::Transparent;
                        break;
                     // 50: Cancels effect 26 (reserved)
                     // 51: Framed
                     // 52: Encircled
                     // 53: Overlined
                     // 54: Not Framed, Not Encircled
                     // 55: Not overlined
                     // 56-59: Reserved
                     // 60-65: Ideograms
                  }
               }

               // Is this a Foreground Color?
               if(IsBetween(attribute, 30U, 37U))
                  change.Foreground(attribute-30);

               // Is this a Bright Foreground Color?
               if(IsBetween(attribute, 90U, 97U))
                  change.Foreground(attribute-90+8);

               // Background Color? (Backgrounds are never bold)
               if(IsBetween(attribute, 40U, 47U))
                  color_bg=m_prop_ansi.propColors().Get(attribute-40);

               // Bright Background Color? (Backgrounds are never bold)
               if(IsBetween(attribute, 100U, 107U))
                  color_bg=m_prop_ansi.propColors().Get(attribute-100+8);
            }

            if(!ts.CharSkip(';'))
               break;
         }

         if(ts.CharGet()!='m')
            return;
         break;
      }
      case ']':
      {
         // OSC sequences (Operating System Command). We handle OSC 8 which is the
         // hyperlink sequence: ESC ] 8 ; params ; URI ST  (where ST (string terminator)
         // is ESC '\' or BEL)
         unsigned osc;
         if(!ts.Parse(osc) || !ts.CharSkip(';'))
            return;

         if(osc!=8)
            return; // Only handle OSC 8 currently

         unsigned param_start=ts.PosGet();
         if(!ts.SkipUntilChar(';'))
            return;

         auto params=ts.GetText().Sub(param_start, ts.PosGet()-1);

         unsigned uri_start=ts.PosGet();
         unsigned uri_end=0;
         while(!uri_end)
         {
            switch(ts.CharGet())
            {
               case 0: // End of stream
                  return;
               case 7: // BEL
                  uri_end=ts.PosGet()-1;
                  break;
               case 27: // ESC, maybe ST
                  if(ts.CharSkip('\\'))
                     uri_end=ts.PosGet()-2;
                  break;
            }
         }

         ConstString uri=ts.GetText().Sub(uri_start, uri_end);

         // Terminate any previous hyperlink
         if(m_in_osc8)
         {
            m_in_osc8=false;
            line_builder.Set(Text::Records::Underline{false});
            line_builder.SetURL(nullptr);
         }

         // Ignore empty URIs
         if(!params && !uri)
            return;

         auto p_url_data=MakeUnique<Text::Records::URLData>();

         // Determine URL type by scheme
         if(uri.IStartsWith("http://") || uri.IStartsWith("https://"))
            p_url_data->m_type=Text::Records::URLType::HTTP;
         else if(uri.IStartsWith("ftp://"))
            p_url_data->m_type=Text::Records::URLType::FTP;
         else if(uri.IStartsWith("tel:"))
            p_url_data->m_type=Text::Records::URLType::TELNET;
         else if(uri.IStartsWith("send:"))
         {
            p_url_data->m_type=Text::Records::URLType::Custom;
            auto p_command=MakeCounting<Command_Send>();
            p_command->m_command=HybridStringBuilder<>{Strings::URLUnescapedString{uri.WithoutFirst(5)}};
            p_url_data->mp_custom=std::move(p_command);
         }
         else if(uri.IStartsWith("prompt:"))
         {
            p_url_data->m_type=Text::Records::URLType::Custom;
            auto p_command=MakeCounting<Command_Prompt>();
            p_command->m_command=HybridStringBuilder<>{Strings::URLUnescapedString{uri.WithoutFirst(7)}};
            p_url_data->mp_custom=std::move(p_command);
         }

         if(!p_url_data->mp_custom)
            p_url_data->m_url=uri;

         line_builder.SetURL(std::move(p_url_data));
         line_builder.Set(Text::Records::Underline{true});
         m_in_osc8=true;
         return;
      }
   }

   // Compute the foreground color based on the bold attribute
   if(change.m_change_foreground)
      color=GetForegroundColor();

   // Insert the color change record
   if(m_state.m_reverse)
      std::swap<Windows::Color>(color, color_bg);

   // Stop invisible text (when that option is set)
   if(m_prop_ansi.fPreventInvisible() && color==color_bg)
      color=(~color_bg)&0x00FFFFFF;

   line_builder.Set(color);
   line_builder.Set(color_bg);

   // If we are using real bold, make sure to set the bold attribute
   if(m_prop_ansi.fFontBold())
      line_builder.Set(Text::Records::Bold{m_state.m_bold});

   if(change.m_change_italic)
      line_builder.Set(Text::Records::Italic{m_state.m_italic});

   if(change.m_change_underline)
      line_builder.Set(Text::Records::Underline{m_state.m_underline});

   if(change.m_change_strikeout)
      line_builder.Set(Text::Records::Strikeout{m_state.m_strikeout});

   if(change.m_change_blink && m_prop_ansi.FlashSpeed()!=0)
   {
      Text::Records::Flash tr;
      switch(m_state.m_blink)
      {
         case Blink_None: tr=Text::Records::Flash{}; break;
         case Blink_Slow: tr=Text::Records::Flash{4, 0x03}; break;
         case Blink_Fast: tr=Text::Records::Flash{2, 0x01}; break;
      }
      line_builder.Set(tr);
   }
}

Color AnsiParser::GetForegroundColor() const
{
   // If font bold is on, the color ignores m_fBold
   bool fBold=m_prop_ansi.fFontBold() ? false : m_state.m_bold && m_state.m_foreground<8;

   Color color=m_state.m_foreground == 100 ? m_state.m_color_foreground : m_prop_ansi.propColors().Get( m_state.m_foreground + (fBold ? 8 : 0) );

   if(m_state.m_faint)
      return color.Darken(128);
   return color;
}
