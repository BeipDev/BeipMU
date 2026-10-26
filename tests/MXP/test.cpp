#include "Main.h"
#include "MXP.h"
#include <cassert>
#include <iostream>

// The MXP arms of TextToLine::Parse, as patched
static std::string Line(MXP::Parser &mxp, Text::LineBuilder &lb, std::string_view text)
{
   std::string copy(text);
   Streams::Input stream{ConstString{std::string_view{copy}}};
   Text::HTMLParser html(stream, lb);
   lb.out.clear();
   mxp.BeginLine();
   while(unsigned char c=stream.CharGet())
   {
      if(mxp.PayBlank(c)) continue;
      if(c!='<') mxp.NotATag();
      switch(c)
      {
         case 27:
            if(mxp.ParseLineTag(stream, lb)) break;
            if(stream.CharSkip('[')) { while(stream.CharSpy() && !isalpha(stream.CharSpy())) stream.CharGet(); stream.CharGet(); break; } // Stands in for AnsiParser
            lb.AppendString("<ESC>"); break;
         case '<':
            if(mxp.ParseTag(stream, html, lb)) break;
            lb.AppendChar(c); break;
         default: lb.AppendChar(c);
      }
   }
   mxp.EndLine(lb);
   return lb.out;
}

static int failures=0;
static void Expect(const char *name, const std::string &got, const std::string &want)
{
   bool ok=got==want;
   if(!ok) failures++;
   std::cout<<(ok?"PASS ":"FAIL ")<<name<<"\n";
   if(!ok) std::cout<<"   got:  "<<got<<"\n   want: "<<want<<"\n";
}

int main()
{
   {
      MXP::Parser m; Text::LineBuilder lb;
      Expect("open line shows SEND as typed", Line(m, lb, "<send href=\"kill\">x</send>"), "<send href=\"kill\">x</send>");
      Expect("open tags work on open lines", Line(m, lb, "a <b>bold</b> c"), "a {B}bold{/B} c");
      Expect("secure line makes SEND a link", Line(m, lb, "\x1b[1z<send href=\"look\" hint=\"Look\">here</send>"), "{link send:look|hint:Look}{U}here{/U}{/link}");
      Expect("mode reverts at the line end", Line(m, lb, "<send href=\"kill\">x</send>"), "<send href=\"kill\">x</send>");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      Expect("unclosed open color is closed at the line end", Line(m, lb, "<color fore=white back=navy>dark"), "{fg 255,255,255}{bg 0,0,128}dark{fg 0,0,0}{bg 0,0,0}");
      // The stub's default colour is 0,0,0, so check state rather than markers
      Line(m, lb, "<color fore=red>red");
      Expect("and the next line starts in the old colour", std::to_string(lb.c.v.r), "0");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      Line(m, lb, "\x1b[6z");
      Expect("lock secure lasts past the line", Line(m, lb, "<send>look north</send>"), "{link send:look north|hint:}{U}look north{/U}{/link}");
      Line(m, lb, "\x1b[3z");
      Expect("reset goes back to open", Line(m, lb, "<send>x</send>"), "<send>x</send>");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      Expect("temp secure covers the next tag and its close", Line(m, lb, "\x1b[4z<send>go</send> then <send>no</send>"), "{link send:go|hint:}{U}go{/U}{/link} then <send>no</send>");
      Expect("temp secure is spent by text", Line(m, lb, "\x1b[4zx<send>no</send>"), "x<send>no</send>");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      Expect("locked parses nothing", Line(m, lb, "\x1b[2z<b>x</b>"), "<b>x</b>");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      std::string out=Line(m, lb, "\x1b[1z<send href=\"map\"><image map.png url=\"https://example.com/img/\" h=120></send>");
      Expect("image inside send carries its command", out, "{link send:map|hint:}{U}🖼️map.png{/U}{/link}");
      Expect("image url resolved", m.m_images.Count()==1 ? m.m_images[0].m_url.s : "", "https://example.com/img/map.png");
      Expect("image height read", m.m_images.Count()==1 ? std::to_string(m.m_images[0].m_height) : "", "120");
      Line(m, lb, "\x1b[1z<image evil.png url=\"file:///etc/\">");
      Expect("non-web image is dropped", std::to_string(m.m_images.Count()), "0");
      Line(m, lb, "\x1b[1z<image fname=\"https://cdn.example/a.webp\" T=\"x\">");
      Expect("absolute fname", m.m_images.Count()==1 ? m.m_images[0].m_url.s : "", "https://cdn.example/a.webp");
      Line(m, lb, "\x1b[1z<image map.png url=\"https://example.com/img\" T=\"maps\">");
      Expect("url plus type", m.m_images.Count()==1 ? m.m_images[0].m_url.s : "", "https://example.com/img/maps/map.png");
      Line(m, lb, "<image map.png url=\"https://example.com/\">");
      Expect("image refused on an open line", std::to_string(m.m_images.Count()), "0");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      Line(m, lb, "\x1b[1z<version>");
      Expect("version reply", m.m_replies.Count()==1 ? m.m_replies[0].s : "", "\x1b[1z<VERSION MXP=1.0 CLIENT=BeipMU VERSION=4.999>\r\n");
      Line(m, lb, "</version>");
      Expect("a player's </version> sends nothing", std::to_string(m.m_replies.Count()), "0");
      Line(m, lb, "\x1b[1z<support send image>");
      Expect("support reply", std::to_string(m.m_replies.Count()), "1");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      Expect("no '>' is text, with no side effects", Line(m, lb, "\x1b[1z<send href=\"x\" and so on"), "<send href=\"x\" and so on");
      Expect("a < b is text", Line(m, lb, "a < b > c"), "a < b > c");
      Expect("closing with nothing open is text", Line(m, lb, "x</b>"), "x</b>");
      Expect("unknown secure tag is dropped", Line(m, lb, "\x1b[1z<frame name=x>y"), "y");
      Expect("prompt send", Line(m, lb, "\x1b[1z<send href=\"say \" prompt>talk</send>"), "{link prompt:say }{U}talk{/U}{/link}");
      Expect("&text; in href", Line(m, lb, "\x1b[1z<send href=\"get &text;\">sword</send>"), "{link send:get sword|hint:}{U}sword{/U}{/link}");
      Expect("a href web only", Line(m, lb, "\x1b[1z<a href=\"javascript:x\">j</a>"), "<a href=\"javascript:x\">j</a>");
      Expect("a href http", Line(m, lb, "\x1b[1z<a href=\"https://x.org\">j</a>"), "{link http:https://x.org}{U}j{/U}{/link}");
      Expect("secure send left open ends at the line", Line(m, lb, "\x1b[1z<send href=\"x\">open"), "{link send:x|hint:}{U}open{/U}{/link}");
      Expect("and does not reach the next line", Line(m, lb, "plain"), "plain");
      Expect("br", Line(m, lb, "\x1b[1za<br>b"), "a{BR}b");
      Expect("nested close unwinds inner", Line(m, lb, "<b><i>x</b>y"), "{B}{I}x{/I}{/B}y");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      Expect("a cell-sized label takes the place of blanks", Line(m, lb, "\x1b[1z|<image a.png url=\"https://x/\" w=10c h=4c>          |"), "|🖼️a.png   |");
      Expect("and records its rows", m.m_images.Count()==1 ? std::to_string(m.m_images[0].m_rows) : "", "4");
      Expect("a long name is elided to the width", Line(m, lb, "\x1b[1z|<image longname.png url=\"https://x/\" w=6c h=2c>      |"), "|🖼️lon…|");
      Expect("text where the blanks should be is kept", Line(m, lb, "\x1b[1z<image a.png url=\"https://x/\" w=10c>xyz  q"), "🖼️a.pngxyz  q");
      Expect("no cell width swallows nothing", Line(m, lb, "\x1b[1z<image a.png url=\"https://x/\">   q"), "🖼️a.png   q");
      Expect("the debt ends with the line", Line(m, lb, "     q"), "     q");
   }
   {
      MXP::Parser m; Text::LineBuilder lb;
      Expect("a colour change among the blanks does not end them", Line(m, lb, "\x1b[1z|<image a.png url=\"https://x/\" w=10c h=2c>  \x1b[0m        |"), "|🖼️a.png   |");
      Expect("eliding never cuts a character in half", Line(m, lb, "\x1b[1z|<image caf\u00e9caf\u00e9.png url=\"https://x/\" w=8c h=2c>        |"), "|🖼️caf\u00e9c…|");
      Expect("a non-ASCII name is counted in characters", Line(m, lb, "\x1b[1z|<image \u00e9.png url=\"https://x/\" w=10c h=2c>          |"), "|🖼️\u00e9.png   |");
      Expect("pixel-sized figure: the label still takes the blanks", Line(m, lb, "\x1b[1z|<image a.png url=\"https://x/\" w=320 h=24>            |"), "|🖼️a.png     |");
      Expect("a few spaces after a picture are left alone", Line(m, lb, "\x1b[1z<image a.png url=\"https://x/\"> and text"), "🖼️a.png and text");
      Line(m, lb, "\x1b[1z<image \"my map.png\" url=\"https://x/\" t=\"/maps\">");
      Expect("spaces encoded, leading slash on T dropped", m.m_images.Count()==1 ? m.m_images[0].m_url.s : "", "https://x/maps/my%20map.png");
      Line(m, lb, "\x1b[1z<image fname=\"https://x/pics/\">");
      Expect("trailing slash: label is the last segment", m.m_images.Count()==1 ? m.m_images[0].m_name.s : "", "pics");
   }
   {
      MXP::Parser m2; Text::LineBuilder lb2;
      Expect("a player's </b> cannot close the server's bold", Line(m2, lb2, "\x1b[1z<b>\x1b[0zplayer</b> text"), "{B}player</b> text");
      Expect("a player's </send> cannot end the server's link", Line(m2, lb2, "\x1b[1z<send href=\"x\">\x1b[0za</send>b"), "{link send:x|hint:}{U}a</send>b{/U}{/link}");
   }
   {
      // A figure with H=3c in a bordered box: tag row, two more rows, then the bottom edge
      auto run=[](std::vector<std::string> lines, int rows)
      {
         MXP::Figures figures;
         MXP::Image image; image.m_url.s="u"; image.m_rows=rows;
         figures.Add(std::move(image));
         std::string where;
         for(unsigned i=0;i<lines.size();i++)
         {
            Collection<MXP::Image> above, below;
            figures.Step(ConstString(std::string_view(lines[i])), above, below);
            if(above.Count()) where+="above "+std::to_string(i)+";";
            if(below.Count()) where+="below "+std::to_string(i)+";";
         }
         return where;
      };
      Expect("figure waits for the bottom edge", run({"\u2502 Fountain \u2502", "\u2502 Benches  \u2502", "\u2514\u2500\u2500\u2518", "next"}, 3), "below 2;");
      Expect("box text under the figure is waited out", run({"\u2502 a \u2502", "\u2502 b \u2502", "\u2502 more \u2502", "  \u2570\u2500\u256f", "x"}, 3), "below 3;");
      Expect("no border: drawn above the first line after", run({"| a |", "| b |", "You see a fountain."}, 3), "above 2;");
      Expect("H=1c in a box: under the bottom edge", run({"\u255a\u2550\u255d"}, 1), "below 0;");
   }
   std::cout<<(failures ? "FAILURES: " : "all passed ")<<failures<<"\n";
   return failures!=0;
}
