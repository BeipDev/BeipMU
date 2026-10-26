//
// MXP (MUD eXtension Protocol)
//
// The parts of the spec this implements:
//  - The line modes ESC [ n z: 0 open, 1 secure, 2 locked, 3 reset, 4 temp secure,
//    5/6/7 lock open/secure/locked (set the default mode).
//  - Open tags anywhere a tag may be parsed: B BOLD STRONG I ITALIC EM U UNDERLINE S STRIKEOUT C COLOR
//  - Secure tags on secure lines only: SEND A BR IMAGE VERSION SUPPORT
//
// Not implemented: ELEMENT/ENTITY definitions, FRAME, DEST, EXPIRE, GAUGE, STAT, SOUND, H/HIGH, FONT.
// SUPPORT says so.
//
// Two rules close the stretches of trust a server opens, and both are load-bearing.
// The mode goes back to the default at the end of every line, and every tag opened on an
// open line is closed there too. Without the first, one ESC[1z the server forgets to clear
// makes a player's <SEND> clickable for the rest of the session. Without the second, a
// player's unclosed <COLOR FORE=black BACK=black> paints every later line black on black.
//
// A tag that is refused is shown as typed: ParseTag returns false and TextToLine prints the '<'
// and carries on, so the rest of the tag comes out as ordinary text.
//

#include "Main.h"
#include "TextToLine.h"
#include "MXP.h"

namespace MXP
{

static bool IsOpenTag(ConstString tag)
{
   static constexpr ConstString c_open[]={ "b", "bold", "strong", "i", "italic", "em", "u", "underline", "s", "strikeout", "c", "color" };
   for(auto &open : c_open)
      if(IEquals(tag, open))
         return true;
   return false;
}

static bool IsWebURL(ConstString url)
{
   return url.IStartsWith("http://") || url.IStartsWith("https://");
}

OwnedString ResolveImageURL(ConstString fname, ConstString url, ConstString type)
{
   if(!fname)
      return {};

   // No URL: the file name has to be a whole address on its own
   if(!url)
      return IsWebURL(fname) ? OwnedString(fname) : OwnedString();

   if(!IsWebURL(url))
      return {};

   // An absolute file name wins over the directory, the way a browser resolves it
   if(IsWebURL(fname))
      return OwnedString(fname);
   if(fname.StartsWith('/'))
      return {}; // Host-relative names need a real URL parser, refuse rather than guess

   // URL is a directory, T a subdirectory of it, FNAME the file in that
   HybridStringBuilder<> full(url);
   if(!url.EndsWith('/'))
      full('/');
   while(type.StartsWith('/'))
      type=type.WithoutFirst(1);
   if(type)
   {
      full(type);
      if(!type.EndsWith('/'))
         full('/');
   }

   // A space is the one character file names have that a URL cannot carry as is
   for(unsigned i=0;i<fname.Count();i++)
   {
      if(fname[i]==' ')
         full("%20");
      else
         full(fname[i]);
   }
   return OwnedString(full);
}

// A short label for the picture: the last path segment of its name
static ConstString DisplayName(ConstString fname)
{
   ConstString left, right;
   ConstString name=fname;
   if(name.Split('?', left, right))
      name=left;
   while(name.EndsWith('/'))
      name=name.WithoutLast(1);
   if(name.RSplit('/', left, right))
      name=right;
   if(!name)
      return "image";
   return name;
}

// Cells a UTF-8 string takes in a fixed-width font, counting each code point as one
static unsigned CellCount(ConstString text)
{
   unsigned cells=0;
   for(unsigned i=0;i<text.Count();i++)
      if((uint8(text[i])&0xC0)!=0x80) // Not a continuation byte
         cells++;
   return cells;
}

// The first 'cells' code points of a UTF-8 string, never cutting one in half
static ConstString FirstCells(ConstString text, unsigned cells)
{
   unsigned i=0;
   for(;i<text.Count();i++)
   {
      if((uint8(text[i])&0xC0)!=0x80 && cells--==0)
         break;
   }
   return text.First(i);
}

// #rrggbb, or one of the sixteen names every MUD uses
static bool ParseColor(ConstString value, Color &result)
{
   if(value.StartsWith('#') && value.Count()==7)
   {
      unsigned rgb=0;
      for(unsigned i=1;i<7;i++)
      {
         char c=value[i];
         unsigned digit;
         if(c>='0' && c<='9') digit=c-'0';
         else if(c>='a' && c<='f') digit=c-'a'+10;
         else if(c>='A' && c<='F') digit=c-'A'+10;
         else return false;
         rgb=rgb*16+digit;
      }
      result=Color((rgb>>16)&0xFF, (rgb>>8)&0xFF, rgb&0xFF);
      return true;
   }

   struct Named { ConstString m_name; Color m_color; };
   static const Named c_names[]=
   {
      { "black",   Color(  0,  0,  0) }, { "maroon", Color(128,  0,  0) }, { "green",  Color(  0,128,  0) }, { "olive",  Color(128,128,  0) },
      { "navy",    Color(  0,  0,128) }, { "purple", Color(128,  0,128) }, { "teal",   Color(  0,128,128) }, { "silver", Color(192,192,192) },
      { "gray",    Color(128,128,128) }, { "red",    Color(255,  0,  0) }, { "lime",   Color(  0,255,  0) }, { "yellow", Color(255,255,  0) },
      { "blue",    Color(  0,  0,255) }, { "magenta",Color(255,  0,255) }, { "cyan",   Color(  0,255,255) }, { "white",  Color(255,255,255) },
      { "grey",    Color(128,128,128) }, { "fuchsia",Color(255,  0,255) }, { "aqua",   Color(  0,255,255) },
   };
   for(auto &named : c_names)
   {
      if(IEquals(value, named.m_name))
      {
         result=named.m_color;
         return true;
      }
   }
   return false;
}

void Parser::Reset()
{
   m_default_mode=Mode::Open;
   m_line_mode=Mode::Open;
   m_temp_secure=false;
   m_stack.Empty();
   m_replies.Empty();
   m_images.Empty();
}

void Parser::BeginLine()
{
   m_replies.Empty();
   m_images.Empty();
}

void Parser::EndLine(Text::LineBuilder &line_builder)
{
   m_owed_blanks=0;
   CloseOpenModeTags(line_builder);

   // A link cannot reach into the next Text::Line, so a SEND or A left open ends here too
   for(unsigned i=0;i<m_stack.Count();i++)
   {
      if(m_stack[i].m_tag!=Tag::Send && m_stack[i].m_tag!=Tag::Anchor)
         continue;
      while(m_stack.Count()>i)
      {
         Restore(m_stack[m_stack.Count()-1], line_builder);
         m_stack.Pop();
      }
      break;
   }

   m_line_mode=m_default_mode;
   m_temp_secure=false;
}

bool Parser::ParseLineTag(Streams::Input &stream, Text::LineBuilder &line_builder)
{
   Streams::Input::PosRestorer restorer(stream);

   unsigned mode;
   if(!stream.CharSkip('[') || !stream.Parse(mode) || !stream.CharSkip('z'))
      return false;
   restorer.NoRestore();

   // Leaving open mode closes what was opened in it, before the new mode applies
   if(m_line_mode==Mode::Open && mode!=0 && mode!=4)
      CloseOpenModeTags(line_builder);

   switch(mode)
   {
      case 0: m_line_mode=Mode::Open; break;
      case 1: m_line_mode=Mode::Secure; break;
      case 2: m_line_mode=Mode::Locked; break;
      case 3: // Reset: close everything, back to open
         while(m_stack.Count())
         {
            Restore(m_stack[m_stack.Count()-1], line_builder);
            m_stack.Pop();
         }
         m_default_mode=m_line_mode=Mode::Open;
         break;
      case 4: m_temp_secure=true; break;
      case 5: m_default_mode=m_line_mode=Mode::Open; break;
      case 6: m_default_mode=m_line_mode=Mode::Secure; break;
      case 7: m_default_mode=m_line_mode=Mode::Locked; break;
      default: break; // 10-99 are user-defined line tags, which need ELEMENT, ignore them
   }
   return true;
}

bool Parser::ParseTag(Streams::Input &stream, Text::HTMLParser &html, Text::LineBuilder &line_builder)
{
   // TEMP SECURE is spent by the very next tag whatever happens to it
   bool secure=IsSecure();
   m_temp_secure=false;

   if(m_line_mode==Mode::Locked && !secure)
      return false;

   // Handlers act as they read, so a tag with no '>' must be refused before any of them runs
   ConstString rest=stream.GetText().WithoutFirst(stream.PosGet());
   ConstString inside, after;
   if(!rest.Split('>', inside, after))
      return false;

   Streams::Input::PosRestorer restorer(stream);

   bool start=!stream.CharSkip('/');

   ConstString tag;
   if(!html.ParseName(tag))
      return false;

   // A secure tag on an open line is shown as typed. Closing a link is let through: Close
   // only lets it end a link opened on an open line, which is a server's ESC[4z<SEND>, and
   // refusing it would leave that link running on into the player's text after it.
   bool closes_link=!start && (IEquals(tag, "send") || IEquals(tag, "a"));
   if(!secure && !IsOpenTag(tag) && !closes_link)
      return false;

   Frame frame{};
   bool handled=true;

   if(IEquals(tag, "b") || IEquals(tag, "bold") || IEquals(tag, "strong"))
   {
      if(start) Open(Tag::Bold, line_builder, frame); else handled=Close(Tag::Bold, line_builder, secure);
   }
   else if(IEquals(tag, "i") || IEquals(tag, "italic") || IEquals(tag, "em"))
   {
      if(start) Open(Tag::Italic, line_builder, frame); else handled=Close(Tag::Italic, line_builder, secure);
   }
   else if(IEquals(tag, "u") || IEquals(tag, "underline"))
   {
      if(start) Open(Tag::Underline, line_builder, frame); else handled=Close(Tag::Underline, line_builder, secure);
   }
   else if(IEquals(tag, "s") || IEquals(tag, "strikeout"))
   {
      if(start) Open(Tag::Strikeout, line_builder, frame); else handled=Close(Tag::Strikeout, line_builder, secure);
   }
   else if(IEquals(tag, "c") || IEquals(tag, "color"))
   {
      if(start) handled=HandleColor(html, line_builder, frame); else handled=Close(Tag::Color, line_builder, secure);
   }
   else if(IEquals(tag, "send"))
   {
      if(start) handled=HandleSend(stream, html, line_builder); else handled=Close(Tag::Send, line_builder, secure);
   }
   else if(IEquals(tag, "a"))
   {
      if(start) handled=HandleAnchor(html, line_builder); else handled=Close(Tag::Anchor, line_builder, secure);
   }
   else if(!start)
      ; // </BR>, </VERSION>, an unknown close: nothing to do
   else if(IEquals(tag, "br"))
      line_builder.AppendNewLine();
   else if(IEquals(tag, "image"))
      handled=HandleImage(html, line_builder, CountBlanks(after));
   else if(IEquals(tag, "version"))
      HandleVersion();
   else if(IEquals(tag, "support"))
   {
      Text::HTMLParser::Attribute attribute;
      while(html.ParseAttribute(attribute) || html.ParseValue(attribute.m_value)) { } // The list of tags asked about, we answer for all of them
      HandleSupport();
   }
   else
   {
      // An unknown tag on a secure line is dropped, as the spec asks
      Text::HTMLParser::Attribute attribute;
      while(html.ParseAttribute(attribute) || html.ParseValue(attribute.m_value)) { }
   }

   if(!handled)
      return false;

   // Past the '>', whatever is left before it (<BR/>, an attribute we did not read). It is there: checked above.
   if(!stream.SkipUntilChar('>'))
      return false;

   restorer.NoRestore();
   return true;
}

void Parser::Open(Tag tag, Text::LineBuilder &line_builder, Frame &frame)
{
   frame.m_tag=tag;
   frame.m_opened_in_open_mode=m_line_mode==Mode::Open;
   frame.m_bold=line_builder.Get<Text::Records::Bold>();
   frame.m_italic=line_builder.Get<Text::Records::Italic>();
   frame.m_underline=line_builder.Get<Text::Records::Underline>();
   frame.m_strikeout=line_builder.Get<Text::Records::Strikeout>();
   frame.m_color=line_builder.Get<Text::Records::Color>();
   frame.m_color_bg=line_builder.Get<Text::Records::ColorBg>();

   switch(tag)
   {
      case Tag::Bold:      line_builder.Set(Text::Records::Bold{true}); break;
      case Tag::Italic:    line_builder.Set(Text::Records::Italic{true}); break;
      case Tag::Underline: line_builder.Set(Text::Records::Underline{true}); break;
      case Tag::Strikeout: line_builder.Set(Text::Records::Strikeout{true}); break;
      default: break; // The handler sets these
   }

   m_stack.Push(frame);
}

void Parser::Restore(const Frame &frame, Text::LineBuilder &line_builder)
{
   line_builder.Set(Text::Records::Bold{frame.m_bold});
   line_builder.Set(Text::Records::Italic{frame.m_italic});
   line_builder.Set(Text::Records::Underline{frame.m_underline});
   line_builder.Set(Text::Records::Strikeout{frame.m_strikeout});
   line_builder.Set(frame.m_color);
   line_builder.Set(frame.m_color_bg);
   if(frame.m_tag==Tag::Send || frame.m_tag==Tag::Anchor)
      line_builder.SetURL(nullptr);
}

// Closes the innermost open frame for this tag, and everything opened inside it.
// From an open line only a frame that was opened on an open line can be closed, so a
// player's </B> or </SEND> cannot end markup the server opened on a secure line.
bool Parser::Close(Tag tag, Text::LineBuilder &line_builder, bool secure)
{
   for(unsigned i=m_stack.Count();i-->0;)
   {
      if(m_stack[i].m_tag!=tag || (!secure && !m_stack[i].m_opened_in_open_mode))
         continue;

      while(m_stack.Count()>i)
      {
         Restore(m_stack[m_stack.Count()-1], line_builder);
         m_stack.Pop();
      }
      return true;
   }
   return false; // Nothing to close, show the tag as typed
}

void Parser::CloseOpenModeTags(Text::LineBuilder &line_builder)
{
   // Frames are restored innermost first, so the style ends up as it was before the first of them
   for(unsigned i=0;i<m_stack.Count();i++)
   {
      if(!m_stack[i].m_opened_in_open_mode)
         continue;

      while(m_stack.Count()>i)
      {
         Restore(m_stack[m_stack.Count()-1], line_builder);
         m_stack.Pop();
      }
      break;
   }
}

bool Parser::HandleColor(Text::HTMLParser &html, Text::LineBuilder &line_builder, Frame &frame)
{
   Color fore, back;
   bool has_fore=false, has_back=false;

   Text::HTMLParser::Attribute attribute;
   unsigned positional=0;
   while(true)
   {
      if(html.ParseAttribute(attribute))
      {
         if(IEquals(attribute.m_name, "fore"))
            has_fore=ParseColor(attribute.m_value, fore);
         else if(IEquals(attribute.m_name, "back"))
            has_back=ParseColor(attribute.m_value, back);
      }
      else if(ConstString value; html.ParseValue(value)) // <COLOR red blue>
      {
         if(positional==0) has_fore=ParseColor(value, fore);
         else if(positional==1) has_back=ParseColor(value, back);
         positional++;
      }
      else
         break;
   }

   Open(Tag::Color, line_builder, frame);
   if(has_fore)
      line_builder.Set(Text::Records::Color{fore});
   if(has_back)
      line_builder.Set(Text::Records::ColorBg{back});
   return true;
}

// <SEND href="cmd|cmd2" hint="tip|item|item2" PROMPT>text</SEND>
// With no href the command is the text between the tags. &text; in href is that text too.
bool Parser::HandleSend(Streams::Input &stream, Text::HTMLParser &html, Text::LineBuilder &line_builder)
{
   ConstString href, hint;
   bool prompt=false;

   Text::HTMLParser::Attribute attribute;
   while(true)
   {
      if(html.ParseAttribute(attribute))
      {
         if(IEquals(attribute.m_name, "href"))
            href=attribute.m_value;
         else if(IEquals(attribute.m_name, "hint"))
            hint=attribute.m_value;
         else if(IEquals(attribute.m_name, "prompt"))
            prompt=true;
         // EXPIRE and anything else: accepted and ignored
      }
      else if(ConstString value; html.ParseValue(value))
      {
         if(IEquals(value, "prompt"))
            prompt=true;
         else if(!href)
            href=value;
         else if(!hint)
            hint=value;
      }
      else
         break;
   }

   // The text up to </SEND>, with tags taken out. Only needed when href is missing or uses &text;
   HybridStringBuilder<> text;
   if(!href || href.Find(ConstString("&text;"))!=Strings::Result::Not_Found)
   {
      ConstString rest=stream.GetText().WithoutFirst(stream.PosGet());
      ConstString attributes;
      if(!rest.Split('>', attributes, rest)) // Step past whatever is left of this tag
         rest={};
      bool in_tag=false;
      for(unsigned i=0;i<rest.Count();i++)
      {
         char c=rest[i];
         if(c=='<')
         {
            if(rest.WithoutFirst(i).IStartsWith("</send>"))
               break;
            in_tag=true;
         }
         else if(c=='>')
            in_tag=false;
         else if(!in_tag)
            text(c);
      }
   }

   HybridStringBuilder<> command;
   if(!href)
      command(text);
   else
   {
      // Replace each &text; with the text
      ConstString remaining=href;
      ConstString before, after;
      while(remaining.Split(ConstString("&text;"), before, after))
      {
         command(before, text);
         remaining=after;
      }
      command(remaining);
   }

   Frame frame{};
   Open(Tag::Send, line_builder, frame);

   if(prompt)
   {
      auto p_command=MakeCounting<Command_Prompt>();
      p_command->m_command=command;
      line_builder.SetURL(MakeUnique<Text::Records::URLData>(*p_command));
   }
   else
      line_builder.SetURL(MakeUnique<Text::Records::URLData>(*MakeCounting<Pueblo_Send>(command, hint))); // Pueblo_Send already does the '|' menus
   line_builder.Set(Text::Records::Underline{true});
   return true;
}

bool Parser::HandleAnchor(Text::HTMLParser &html, Text::LineBuilder &line_builder)
{
   ConstString href;
   Text::HTMLParser::Attribute attribute;
   while(html.ParseAttribute(attribute))
   {
      if(IEquals(attribute.m_name, "href"))
         href=attribute.m_value;
   }

   // Only web links. A server's file:, javascript: or registered-app scheme is not something to hand the shell.
   if(!IsWebURL(href))
      return false;

   Frame frame{};
   Open(Tag::Anchor, line_builder, frame);
   line_builder.SetURL(MakeUnique<Text::Records::URLData>(Text::Records::URLType::HTTP, href));
   line_builder.Set(Text::Records::Underline{true});
   return true;
}

// <IMAGE fname URL="http://host/dir/" T="type" H=100 W=100 ALIGN=left>
// The label goes in the line. If a SEND is open the label carries its command,
// which is the spec's own <SEND showmap><IMAGE map.png></SEND> example.
// The picture is drawn under the line by the connection, in an ImageBanner.
static constexpr unsigned c_icon_cells=2; // 🖼️ is drawn two cells wide in a fixed font

bool Parser::HandleImage(Text::HTMLParser &html, Text::LineBuilder &line_builder, unsigned blanks_after)
{
   ConstString fname, url, type, height, width;

   Text::HTMLParser::Attribute attribute;
   while(true)
   {
      if(html.ParseAttribute(attribute))
      {
         if(IEquals(attribute.m_name, "fname")) fname=attribute.m_value;
         else if(IEquals(attribute.m_name, "url")) url=attribute.m_value;
         else if(IEquals(attribute.m_name, "t")) type=attribute.m_value;
         else if(IEquals(attribute.m_name, "h")) height=attribute.m_value;
         else if(IEquals(attribute.m_name, "w")) width=attribute.m_value;
         // HSPACE, VSPACE, ALIGN, ISMAP: read and ignored
      }
      else if(ConstString value; html.ParseValue(value))
      {
         if(!fname)
            fname=value;
      }
      else
         break;
   }

   auto resolved=ResolveImageURL(fname, url, type);
   if(!resolved)
      return true; // A tag we understood and refuse to fetch: drop it, don't show it

   Image image;
   image.m_url=std::move(resolved);
   image.m_name=DisplayName(fname);

   unsigned value;
   bool in_cells;

   if(ParseExtent(height, value, in_cells))
   {
      if(in_cells)
         image.m_rows=int(value);
      else
         image.m_height=int(value<16 ? 16 : value>600 ? 600 : value)*g_dpiScale;
   }

   // A server that lays a figure out in cells (W=16c H=8c) follows the tag with that many
   // blanks, and leaves the same columns blank on the rows under it. The label takes the
   // place of blanks rather than being added to them, so a box drawn round the figure keeps
   // its right edge. TextToLine swallows them through PayBlank.
   //
   // A figure the server could not size in cells (a one-row description, W and H in pixels)
   // still has its blanks, so when at least a label's worth follows the tag they are taken too.
   ConstString name=image.m_name;
   unsigned name_cells=CellCount(name);
   unsigned columns=0;
   if(ParseExtent(width, value, in_cells) && in_cells)
      columns=value;
   else if(blanks_after>=c_icon_cells+name_cells)
      columns=blanks_after;

   line_builder.AppendEmoji("🖼️");
   if(columns && columns<c_icon_cells+1)
      m_owed_blanks=columns; // Room for the icon alone, and not quite that
   else if(columns && c_icon_cells+name_cells>columns)
   {
      // Elide to fit: icon, as much of the name as fits, and an ellipsis
      line_builder.AppendString(FirstCells(name, columns-c_icon_cells-1));
      line_builder.AppendString("…");
      m_owed_blanks=columns;
   }
   else
   {
      line_builder.AppendString(name);
      m_owed_blanks=columns ? c_icon_cells+name_cells : 0;
   }

   m_images.Push(std::move(image));
   return true;
}

// "120" is pixels, "8c" is character cells. Percentages are not supported.
bool Parser::ParseExtent(ConstString text, unsigned &value, bool &in_cells)
{
   Streams::Input input(text);
   if(!input.Parse(value))
      return false;
   in_cells=input.CharSkip('c') || input.CharSkip('C');
   return !input.CharSpy();
}

// Spaces at the start of the text after a tag
unsigned Parser::CountBlanks(ConstString text)
{
   unsigned i=0;
   while(i<text.Count() && text[i]==' ')
      i++;
   return i;
}

bool Parser::PayBlank(char c)
{
   if(!m_owed_blanks)
      return false;
   if(c=='\x1B')
      return false; // A colour change among the blanks is not text, the blanks carry on after it
   if(c!=' ')
   {
      m_owed_blanks=0; // Text where the blanks should be: the server left no room, nothing more to swallow
      return false;
   }
   m_owed_blanks--;
   return true;
}

enum struct BoxLine { None, Edge, Bottom };

// Whether a line is part of a box drawn in U+2500..U+257F, and if so whether it is the bottom edge
static BoxLine ClassifyBoxLine(ConstString line)
{
   unsigned i=0;
   while(i<line.Count() && line[i]==' ')
      i++;
   if(i+2>=line.Count() || uint8(line[i])!=0xE2 || (uint8(line[i+1])!=0x94 && uint8(line[i+1])!=0x95))
      return BoxLine::None;

   uint8 second=uint8(line[i+1]), third=uint8(line[i+2]);
   bool bottom=(second==0x94 && (third==0x94 || third==0x97)) // └ ┗
            || (second==0x95 && (third==0x98 || third==0x99 || third==0x9A || third==0xB0)); // ╘ ╙ ╚ ╰
   return bottom ? BoxLine::Bottom : BoxLine::Edge;
}

void Figures::Add(Image &&image)
{
   image.m_rows--; // The tag's own line was the first reserved row
   m_waiting.Push(std::move(image));
}

void Figures::Step(ConstString line, Collection<Image> &above, Collection<Image> &below)
{
   auto box=ClassifyBoxLine(line);

   for(unsigned i=0;i<m_waiting.Count();)
   {
      auto &image=m_waiting[i];
      if(image.m_rows>0)
      {
         image.m_rows--; // Another reserved row: the picture's place is still inside the box
         i++;
         continue;
      }

      if(box==BoxLine::Edge)
      {
         i++; // The box goes on below the picture's rows (text under it), wait for its bottom
         continue;
      }

      // The bottom edge: draw after it. Anything else: the box has ended, draw before this line.
      (box==BoxLine::Bottom ? below : above).Push(std::move(image));
      m_waiting.Delete(i);
   }
}

static constexpr ConstString c_secure_line="\x1B[1z";

void Parser::HandleVersion()
{
   m_replies.Push(OwnedString(HybridStringBuilder<>(c_secure_line, "<VERSION MXP=1.0 CLIENT=BeipMU VERSION=", g_szVersion, ">\r\n")));
}

// Only tags ParseTag has a case for. Claiming one we drop would tell the server to send markup we throw away.
void Parser::HandleSupport()
{
   m_replies.Push(OwnedString(HybridStringBuilder<>(c_secure_line,
      "<SUPPORTS +b +bold +strong +i +italic +em +u +underline +s +strikeout +c +color"
      " +send +send.href +send.hint +send.prompt +a +a.href +br"
      " +image +image.fname +image.url +image.t +image.h +image.w"
      " +version +support>\r\n")));
}

}
