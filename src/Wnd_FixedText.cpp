#include "Main.h"
#include "WindowEvents.h"
#include "Wnd_FixedText.h"

ATOM Wnd_FixedText::Register()
{
   WndClass wc(L"FixedText");
   wc.hCursor = LoadCursor(nullptr, IDC_ARROW); 
   return wc.Register();
}

Wnd_FixedText::Wnd_FixedText(Window wndParent, int2 size)
 : m_size(size),
   m_pLines(size.y)
{
   for(int h=0;h<m_size.y;h++)
      m_pLines.Push(MakeUnique<char[]>(m_size.x));

   m_font=m_propFont.CreateFont();

   {
      ScreenDC dc;
      DC::FontSelector _(dc, m_font);
      dc.GetTextMetrics(m_tm);

      m_sizeText.y=m_tm.tmHeight;
      m_sizeText.x=m_tm.tmAveCharWidth;
   }

   Create(STR_Title_FixedText, WS_OVERLAPPED | WS_SYSMENU, Position_Default, wndParent);

   SetSize(FrameSize()+int2(m_size.x*m_tm.tmAveCharWidth, m_size.y*m_tm.tmHeight));
   Clear();

   Show(SW_SHOWNOACTIVATE);
}

LRESULT Wnd_FixedText::WndProc(const Message &msg)
{
   EventsWndProc(msg);
   return Dispatch<WindowImpl, Msg::Create, Msg::Paint>(msg);
}

void Wnd_FixedText::Clear()
{
   for(auto &line : m_pLines)
      for(int w=0;w<m_size.x;w++)
         line[w]=' ';
   
   m_ptCursor=int2(0, 0);
   m_ptDirtyStart=m_ptCursor;
   Invalidate(false);
}

void Wnd_FixedText::Write(ConstString string)
{
   for(char c : string)
   {
      if(m_ptCursor.x==m_size.x)
      {
         CarriageReturn();
         LineFeed();
      }

      if(c==CHAR_CR)
      {
         CarriageReturn();
         continue;
      }

      if(c==CHAR_LF)
      {
         LineFeed();
         continue;
      }

      m_pLines[m_ptCursor.y][m_ptCursor.x]=c;
      m_ptCursor.x++;
   }

   DirtyFlush();
   Update();
}

void Wnd_FixedText::CarriageReturn()
{
   DirtyFlush();
   m_ptCursor.x=0;
}

void Wnd_FixedText::LineFeed()
{
   m_ptCursor.y++;
   if(m_ptCursor.y==m_size.y)
      ScrollUp();
}

void Wnd_FixedText::DirtyFlush()
{
   if(m_ptDirtyStart.y==m_ptCursor.y)
   {
      // Invalidate the undrawn portion of the line
      Rect rcInvalid(int2(m_ptDirtyStart.x, m_ptDirtyStart.y), int2(m_ptCursor.x, m_ptCursor.y+1));
      Invalidate(rcInvalid*m_sizeText, false);
   }
   else // Invalidate the lines between the start and current
   {
      Rect rcInvalid(int2(0, m_ptDirtyStart.y), int2(m_size.x, m_ptCursor.y+1));
      Invalidate(rcInvalid*m_sizeText, false);
   }

   m_ptDirtyStart=m_ptCursor;
}

void Wnd_FixedText::ScrollUp()
{
   auto temp=m_pLines.Delete(0);
   MemoryFill(Array<char>(temp, m_size.x), ' ');
   m_pLines.Push(std::move(temp));

   DirtyFlush();
   m_ptCursor.y--;
   m_ptDirtyStart.y--;
   ScrollY(-m_sizeText.y, nullptr, nullptr);
}

LRESULT Wnd_FixedText::On(const Msg::Create &msg)
{
   return msg.Success();
}

LRESULT Wnd_FixedText::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);
   DC::FontSelector _(ps, m_font);

   ps.SetTextColor(Colors::White);
   ps.SetBackgroundColor(Colors::Black);

   for(int h=0;h<m_size.y;h++)
      ps.TextOut(int2(0, h*m_sizeText.y), ConstString(m_pLines[h], m_size.x));

   return msg.Success();
}
