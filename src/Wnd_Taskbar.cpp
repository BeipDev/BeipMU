#include "Main.h"
#include "Wnd_Taskbar.h"
#include "Wnd_Main.h"

TaskbarTheme g_taskbar_theme;

struct ToolbarItem
{
   ConstString m_icon;
   ConstString m_tooltip;
};

static ToolbarItem c_toolbar_items[]=
{
   {"☰", "Menu"},
   {"👤", "Player Quick Connect"},
   {"🌎", "Worlds"},
};


Wnd_Taskbar::Wnd_Taskbar(Wnd_MDI &wndMDI)
 : m_wnd_MDI(wndMDI)
{
   Create("", WS_CHILD|WS_VISIBLE, Position_Zero, m_wnd_MDI);
}

Wnd_Taskbar::~Wnd_Taskbar()
{
}

LRESULT Wnd_Taskbar::WndProc(const Message &msg)
{
   return Dispatch<WindowImpl, Msg::Create, Msg::Size, Msg::LButtonDown, Msg::LButtonUp, Msg::MouseMove, Msg::RButtonUp, Msg::CaptureChanged, Msg::Paint, Msg::_GetTypeID, Msg::_GetThis>(msg);
}

void Wnd_Taskbar::OnSize(Rect &rcClient)
{
   int height(Controls::Button::m_tmButtons.tmHeight + 8*g_dpiScale);

   if(g_ppropGlobal->fTaskbarOnTop())
   {
      SetPosition(nullptr, int2(rcClient.left, rcClient.top), int2(rcClient.size().x, height), SWP_NOZORDER);
      rcClient.top+=height;
   }
   else
   {
      SetPosition(nullptr, int2(rcClient.left, rcClient.bottom-height), int2(rcClient.size().x, height), SWP_NOZORDER);
      rcClient.bottom-=height;
   }
   Refresh();
}

void Wnd_Taskbar::SetActiveWindow(Wnd_Main &wndMain, bool fActive)
{
   if(fActive)
      mp_active_window=&wndMain;
   Refresh();
}

Color Wnd_Taskbar::DrawWindow(RectF rcWindow, Wnd_Main &wnd, unsigned tabNumber)
{
   Connection &connection=wnd.GetConnection();

   bool active=&wnd==mp_active_window;
   bool flashed=wnd.Flashed();

   Color foreground=g_taskbar_theme.text;
   Color background=Colors::Transparent;
   if(auto *p_puppet=connection.GetPuppet())
      background=p_puppet->clrTabColor();
   else if(auto *p_character=connection.GetCharacter())
      background=p_character->clrTabColor();
   if(background==Colors::Transparent)
      background=g_taskbar_theme.background;

   if(flashed)
   {
      foreground=InvertLuma(foreground.ToFloat());
      background=g_taskbar_theme.flash;
   }

   if(active)
      background=background.Lerp(g_taskbar_theme.highlight, 96);

   mp_brush_dynamic->SetColor(ToD2D(background));
   mp_render_target->FillRectangle(ToD2D(rcWindow), mp_brush_dynamic);

   RectF rcText(rcWindow);
   rcText.Inset(3, 0, 3, 0);

   float emoji_top=(rcText.size().y-mp_font_emoji->Height()*m_font_size)/2.0f;
   float text_top=(rcText.size().y-mp_font_text->Height()*m_font_size)/2.0f;

   // Draw important activity indicator
   if(auto activity_count=connection.GetMainWindow().GetImportantActivityCount())
   {
      int height=rcText.size().y;
      RectF rcImportant{rcText}; rcImportant.right=rcImportant.left+height;
      rcImportant.top+=1;
      rcImportant.bottom-=1;

      D2D1_ROUNDED_RECT rect;
      rect.rect=ToD2D(RectF(rcImportant));
      rect.radiusX=rect.radiusY=4;

      mp_brush_dynamic->SetColor(ToD2D(Color(245, 64, 64)));
      mp_render_target->FillRoundedRectangle(rect, mp_brush_dynamic);
      mp_brush_dynamic->SetColor(ToD2D(Colors::White));
      mp_render_target->DrawRoundedRectangle(rect, mp_brush_dynamic, 1.0f);

      FixedStringBuilder<32> string;
      if(activity_count<10)
         string(activity_count);
      else
         string("9+");

      float2 text_size{mp_font_text->Measure(string, m_font_size), mp_font_text->Height()*m_font_size};
      mp_font_text->Draw(string, m_font_size, rcImportant.center()-text_size/2.0f, *mp_render_target, *mp_brush_dynamic);
      rcText.left+=height+2;
   }

   // Draw logging indicator
   if(connection.IsLogging() && g_ppropGlobal->fTaskbarShowLogging())
   {
      float width=mp_font_emoji->Measure("●", m_font_size);
      mp_brush_dynamic->SetColor(ToD2D(Color(225,0,0)));
      mp_font_emoji->Draw("●", m_font_size, rcText.ptLT()+float2(0, emoji_top), *mp_render_target, *mp_brush_dynamic);
      rcText.left+=width+1;
   }

   // Draw disconnected icon
   if(!connection.IsConnected())
   {
      float width=mp_font_emoji->Measure("⚡", m_font_size);
      mp_font_emoji->Draw("⚡", m_font_size, rcText.ptLT()+float2(0, emoji_top), *mp_render_target, *mp_brush_dynamic);
      rcText.left+=width+3.0f;
   }

   // Draw world name
   if(rcText.size().x>0) // Only if there's some space
   {
      FixedStringBuilder<256> string;
      GetWindowText(connection, string);
      if(connection.IsConnected() || flashed)
         mp_brush_dynamic->SetColor(ToD2D(foreground));
      else
         mp_brush_dynamic->SetColor(ToD2D(g_taskbar_theme.text_disconnected));

      auto &font=active ? *mp_font_text_bold : *mp_font_text;
      
      auto find_info=font.CharFromPosition(string, m_font_size, rcText.size().x);
      if(find_info.index!=string.Count()) // Doesn't completely fit? Add ellipsis
         find_info=font.CharFromPosition(string, m_font_size, rcText.size().x-m_ellipsis_width);
      float width=font.Draw(string.First(find_info.index), m_font_size, rcText.ptLT()+float2(0, text_top), *mp_render_target, *mp_brush_dynamic);
      if(find_info.index!=string.Count())
         font.Draw("…", m_font_size, rcText.ptLT()+float2(width, text_top), *mp_render_target, *mp_brush_dynamic);
   }

   // Draw alt+# indicators
   if(tabNumber!=0)
   {
      FixedStringBuilder<32> number;
      if(tabNumber==10) // Tab 10 uses the 0 key, so say '0'
         tabNumber=0;

      if(tabNumber>10)
      {
         number("↑+");
         tabNumber-=10;
      }
      number(tabNumber);

      float2 extent(mp_font_text->Measure(number, m_font_size)+6, rcText.size().y);
      float2 origin=rcText.ptRB()-extent;
      auto rcNumber=RectF(extent)+origin;

      mp_brush_dynamic->SetColor(ToD2D(g_taskbar_theme.text));
      mp_render_target->FillRectangle(ToD2D(rcNumber), mp_brush_dynamic);
      mp_brush_dynamic->SetColor(ToD2D(InvertLuma(g_taskbar_theme.text.ToFloat())));
      mp_font_text->Draw(number, m_font_size, rcNumber.ptLT()+float2(3, text_top), *mp_render_target, *mp_brush_dynamic);
   }

   return background;
}

void Wnd_Taskbar::GetWindowText(Connection &connection, StringBuilder &string)
{
   Wnd_Main &wnd=connection.GetMainWindow();

   string(wnd.GetTitlePrefix());
   connection.GetWorldTitle(string, wnd.GetUnreadCount());
}

RectF Wnd_Taskbar::CalculateTabRect(unsigned iWindow) const
{
   RectF rect;

   rect.top   =m_rcTabs.top;
   rect.bottom=m_rcTabs.bottom;

   rect.left=m_rcTabs.left+m_width*iWindow;
   rect.right=rect.left+m_width;

   if(iWindow==m_tab_clicked_index)
      rect+=float2(0, 2);

   return rect;
}

RectF Wnd_Taskbar::CalculateButtonRect(unsigned iButton) const
{
   RectF rect;

   rect.top=m_rcToolbar.top;
   rect.bottom=m_rcToolbar.bottom;
   rect.left=CalculateToolbarX(iButton);
   rect.right=rect.left+16;

   return rect;
}

LRESULT Wnd_Taskbar::On(const Msg::Create &msg)
{
   CreateD2DResources();
   if(!mp_render_target)
   {
      static bool s_failure=false;
      if(!s_failure)
      {
         s_failure=true;
         MessageBox(*this, "Direct2D failed to initialize, you're probably running on Wine or somehow have an old version of Direct2D on Windows. This is why the taskbar is blank.", "Drat", MB_OK);
      }
   }

   m_font_size=g_ppropGlobal->UIFontSize();
   mp_font_emoji=&Font::Get("Segoe UI Emoji");
   mp_font_text=&Font::Get(g_ppropGlobal->pclUIFontName());
   mp_font_text_bold=&Font::Get(g_ppropGlobal->pclUIFontName(), true);
   m_ellipsis_width=mp_font_text->Measure("…", m_font_size);

   return msg.Success();
}

void Wnd_Taskbar::CreateD2DResources()
{
   Wnd_D2D::CreateD2DResources();
}

LRESULT Wnd_Taskbar::On(const Msg::Size &msg)
{
   Wnd_D2D::On(msg);
   if(!mp_render_target)
      return msg.Success();

   m_rcToolbar=RectF(FromD2D(mp_render_target->GetSize()));

   m_rcTabs=m_rcToolbar;
   m_rcToolbar.right=m_rcToolbar.size().y*3.0f;
   m_rcTabs.left=m_rcToolbar.right;

   float time_width=mp_font_text->Measure(STR_Online " 30m 22s  " STR_Idle " 30m 22s", m_font_size);

   m_rcTime=m_rcTabs;
   m_rcTime.left=m_rcTime.right-time_width;
   if(g_ppropGlobal->fTaskbarShowTyped())
      m_rcTime.left-=mp_font_text->Measure("Typed 8888  ", m_font_size);

   m_rcTabs.right=m_rcTime.left-5.0f;
   return msg.Success();
}

LRESULT Wnd_Taskbar::On(const Msg::LButtonDown &msg)
{
   m_pt_clicked=msg/g_dpiScale;

   // Toolbar?
   if(m_pt_clicked.x<m_rcToolbar.right)
   {
      unsigned iButton=(m_pt_clicked.x-m_rcToolbar.left)/CalculateToolbarX(1);
      Assert(iButton<std::size(c_toolbar_items));

      m_toolbar_clicked_index=iButton;
      m_over_toolbar=true;
   }
   else if(m_pt_clicked.x<m_rcTabs.right)
   {
      // Tabs?
      unsigned iWindow=(m_pt_clicked.x-m_rcTabs.left)/m_width;
      if(iWindow>=m_wnd_MDI.GetWindowCount())
         return msg.Success();

      m_tab_clicked_index=iWindow;
   }
   else
      return msg.Success();

   SetCapture();
   Refresh();
   return msg.Success();
}

void Wnd_Taskbar::OpenMenu()
{
   m_toolbar_clicked_index=0;
   OnButton();
   m_toolbar_clicked_index=~0U;
}

void Wnd_Taskbar::OnButton()
{
   int2 position=ClientToScreen(int2(CalculateToolbarX(m_toolbar_clicked_index), g_ppropGlobal->fTaskbarOnTop() ? m_rcToolbar.bottom : 0)*g_dpiScale);

   switch(m_toolbar_clicked_index)
   {
      case 0: m_wnd_MDI.PopupMainMenu(position); break;
      case 1: PopupCharacterWindow(position); break;
      case 2: Msg::Command(ID_CONNECTION_CONNECT, *this, 0).Send(m_wnd_MDI.GetActiveWindow()); break;
   }
}

LRESULT Wnd_Taskbar::On(const Msg::LButtonUp &msg)
{
   if(m_toolbar_clicked_index<std::size(c_toolbar_items))
   {
      if(m_over_toolbar)
         OnButton();

      m_over_toolbar=false;
      m_toolbar_clicked_index=~0U;
   }
   else
   {
      if(m_tab_clicked_index>=m_wnd_MDI.GetWindowCount())
         return msg.Success(); // Nothing was really pressed

      m_wnd_MDI.SetActiveWindow(m_wnd_MDI.GetWindow(m_tab_clicked_index));

      if(m_dragging)
      {
         Wnd_Main &window=m_wnd_MDI.GetWindow(m_tab_clicked_index);

         // Are we not over any window?
         if(Window wnd_over=WindowFromPoint(ClientToScreen(msg)); wnd_over!=*this &&
            (GetWindowThreadProcessId(wnd_over.hWnd(), nullptr)!=GetCurrentThreadId() ||
               Msg::_GetTypeID().Send(wnd_over)!=GetTypeID<Wnd_Taskbar>()))
         {
            auto *p_MDI=new Wnd_MDI();
            p_MDI->SetPosition(ClientToScreen(msg)-m_wnd_MDI.ScreenToClient(ClientToScreen(int2{})));
            p_MDI->EnsureOnScreen(true);
            window.Redock(*p_MDI);
            p_MDI->GetWindow(0).Close(); // Close the new tab in the new window we don't want
         }
      }

      m_tab_clicked_index=~0U;
   }

   ReleaseCapture();
   Refresh();
   if(m_dragging)
   {
      SetCursor(LoadCursor(nullptr, IDC_ARROW));
      m_dragging=false;
   }
   return msg.Success();
}

LRESULT Wnd_Taskbar::On(const Msg::MouseMove &msg)
{
   if(Windows::Controls::HasToolTip(*this))
      OnTimerToolTip();
   else
      m_timer_tool_tip.Set(0.5f);

   if(m_toolbar_clicked_index<2)
   {
      bool over=CalculateButtonRect(m_toolbar_clicked_index).IsInside(msg/g_dpiScale);
      if(over==m_over_toolbar)
         return msg.Success();

      m_over_toolbar=over;
      Refresh();
   }
   else
   {
      if(m_tab_clicked_index>=m_wnd_MDI.GetWindowCount())
         return msg.Success();

      if(!m_dragging)
      {
         int cx_drag=abs(GetSystemMetrics(SM_CXDRAG));

         int2 delta=m_pt_clicked-msg.position()/g_dpiScale;
         if(abs(delta.x)>cx_drag || abs(delta.y)>cx_drag)
         {
            m_dragging=true;
            m_pt_dragged=m_pt_clicked-CalculateTabRect(m_tab_clicked_index).ptLT();
            SetCursor(LoadCursor(nullptr, IDC_HAND));
         }
      }

      if(m_dragging)
      {
         Wnd_Main &window=m_wnd_MDI.GetWindow(m_tab_clicked_index);

         // Are we over another taskbar?
         if(Window wnd_over=WindowFromPoint(ClientToScreen(msg));wnd_over!=*this &&
            GetWindowThreadProcessId(wnd_over.hWnd(), nullptr)==GetCurrentThreadId() &&
            Msg::_GetTypeID().Send(wnd_over)==GetTypeID<Wnd_Taskbar>())
         {
            Wnd_Taskbar &taskbar=*reinterpret_cast<Wnd_Taskbar*>(Msg::_GetThis().Send(wnd_over));
            window.Redock(taskbar.m_wnd_MDI);
            Msg::LButtonUp(int2(), 0).Post(*this); // End draggong on this taskbar
            taskbar.m_dragging=true;
            taskbar.m_pt_dragged=m_pt_dragged;
            PinBelow(taskbar.m_pt_dragged.x, taskbar.m_width); // If going from a longer tab window to a shorter tab one, ensure we're still on the tab
            taskbar.m_tab_clicked_index=window.GetMDI().GetWindowCount()-1;
            taskbar.SetCapture();
            taskbar.Refresh();
            return msg.Success();
         }
         else
         {
            unsigned tab_index=(msg.x()/g_dpiScale-m_rcTabs.left)/m_width;
            if(tab_index!=m_tab_clicked_index && tab_index<m_wnd_MDI.GetWindowCount())
            {
               Wnd_Main &window_insert_after=m_wnd_MDI.GetWindow(tab_index);
               if(m_tab_clicked_index<tab_index)
                  window.DLNode<Wnd_Main>::Link(&window_insert_after);
               else
                  window.DLNode<Wnd_Main>::Link(window_insert_after.DLNode<Wnd_Main>::Prev());

               m_tab_clicked_index=tab_index;
            }
         }
         Refresh();
      }
   }

   return msg.Success();
}

LRESULT Wnd_Taskbar::On(const Msg::RButtonUp &msg)
{
   // Right click the tabs?
   if(m_rcTabs.IsInside(msg/g_dpiScale))
   {
      unsigned index=(msg.x()/g_dpiScale-m_rcTabs.left)/m_width;
      if(index>=m_wnd_MDI.GetWindowCount())
         return msg.Success();
      Wnd_Main &wnd=m_wnd_MDI.GetWindow(index);

      m_wnd_MDI.SetActiveWindow(wnd);

      int2 position=ClientToScreen(int2(CalculateTabRect(index).left, g_ppropGlobal->fTaskbarOnTop() ? m_rcToolbar.bottom : 0)*g_dpiScale);
      wnd.PopupTabMenu(position);
   }

   if(m_rcTime.IsInside(msg/g_dpiScale))
   {
      PopupMenu menu;
      menu.Append(MF_STRING, 1, "Show typed letter count");

      menu.Check(1, g_ppropGlobal->fTaskbarShowTyped());

      int2 position=ClientToScreen(int2((msg/g_dpiScale).x, g_ppropGlobal->fTaskbarOnTop() ? m_rcToolbar.bottom : 0)*g_dpiScale);
      int id=TrackPopupMenu(menu, TPM_RETURNCMD|(g_ppropGlobal->fTaskbarOnTop() ? 0 : TPM_BOTTOMALIGN), position, *this, nullptr);
      switch(id)
      {
         case 1: g_ppropGlobal->fTaskbarShowTyped(!g_ppropGlobal->fTaskbarShowTyped()); On(Msg::Size{0, 0, 0}); Refresh(); break;
      }
   }

   return msg.Success();
}

LRESULT Wnd_Taskbar::On(const Msg::CaptureChanged &msg)
{
   if(*this!=msg.wndNewCapture())
      Msg::LButtonUp(int2(), 0).Post(*this);

   return msg.Success();
}

void Wnd_Taskbar::Paint()
{
   Color background_color=g_taskbar_theme.background;
   mp_render_target->Clear(ToD2D(background_color));

   // Draw the toolbar
   {
      mp_brush_dynamic->SetColor(ToD2D(g_taskbar_theme.text));
      float top=(m_rcToolbar.size().y-mp_font_emoji->Height()*m_font_size)/2.0f;
      for(unsigned i=0; i<std::size(c_toolbar_items); i++)
      {
         float width=mp_font_emoji->Measure(c_toolbar_items[i].m_icon, m_font_size);
         float x=(CalculateToolbarX(i)+CalculateToolbarX(i+1))/2;
         mp_font_emoji->Draw(c_toolbar_items[i].m_icon, m_font_size, float2(x-width/2.0f, top), *mp_render_target, *mp_brush_dynamic);
         top; width; x;
      }
   }

   if(m_wnd_MDI.GetWindowCount()==0) // This can happen when there are no windows and a message box is asking the user something on exit
      return;

   m_width=float(m_rcTabs.size().x)/m_wnd_MDI.GetWindowCount();
   PinBelow<float>(m_width, Controls::Button::m_tmButtons.tmAveCharWidth*35);

   unsigned window_index=0;
   for(auto &window : m_wnd_MDI.GetRootWindow())
   {
      Color next_background_color=background_color;

      auto rcWindow=CalculateTabRect(window_index);
      if(!m_dragging || window_index!=m_tab_clicked_index) // Only draw if this isn't the tab being dragged
         next_background_color=DrawWindow(rcWindow, window, m_draw_tab_numbers ? window_index+1 : 0);

      float line_indent=background_color==next_background_color ? 5.0f : 0.0f;
      mp_brush_dynamic->SetColor(ToD2D(g_taskbar_theme.tab_divider));
      mp_render_target->DrawLine(ToD2D(float2(rcWindow.left, rcWindow.top+line_indent)), ToD2D(float2(rcWindow.left, rcWindow.bottom-line_indent)), mp_brush_dynamic, 1.0f);
      background_color=next_background_color;

      window_index++;
   }

   {
      auto rcWindow=CalculateTabRect(m_wnd_MDI.GetWindowCount());

      float next_background_color=g_taskbar_theme.background;
      float line_indent=background_color==next_background_color ? 5.0f : 0.0f;
      mp_brush_dynamic->SetColor(ToD2D(g_taskbar_theme.tab_divider));
      mp_render_target->DrawLine(ToD2D(float2(rcWindow.left, rcWindow.top+line_indent)), ToD2D(float2(rcWindow.left, rcWindow.bottom-line_indent)), mp_brush_dynamic, 1.0f);
   }

   if(mp_active_window)
   {
      Wnd_Main &wndMain=*mp_active_window;
      Connection &connection=wndMain.GetConnection();

      FixedStringBuilder<32> strTyped("Typed ");
      FixedStringBuilder<32> strTypedValue(wndMain.GetInputWindow().GetTextLength());
      FixedStringBuilder<32> strOnline("  " STR_Online " ");
      FixedStringBuilder<32> strOnlineTime; Time::SecondsToStringAbbreviated(strOnlineTime, connection.TimeConnectedInSeconds());
      FixedStringBuilder<32> strIdle("  " STR_Idle " ");
      FixedStringBuilder<32> strIdleTime; Time::SecondsToStringAbbreviated(strIdleTime, connection.TimeIdleInSeconds());

      float2 point=m_rcTime.ptRT(); point.x-=4; point.y+=3+mp_font_text->Ascent()*m_font_size;
      auto DrawText=[&](ConstString text, Color color)
      {
         point.x-=mp_font_text->Measure(text, m_font_size);
         mp_brush_dynamic->SetColor(ToD2D(color));
         mp_font_text->DrawBaseline(text, point, m_font_size, *mp_render_target, *mp_brush_dynamic);
      };

      DrawText(strIdleTime, g_taskbar_theme.text);
      DrawText(strIdle, g_taskbar_theme.text_faint);
      DrawText(strOnlineTime, g_taskbar_theme.text);
      DrawText(strOnline, g_taskbar_theme.text_faint);
      if(g_ppropGlobal->fTaskbarShowTyped())
      {
         DrawText(strTypedValue, g_taskbar_theme.text);
         DrawText(strTyped, g_taskbar_theme.text_faint);
      }

      if(m_timer_time && !connection.IsConnected())
         m_timer_time.Reset();
      if(!m_timer_time && connection.IsConnected())
         m_timer_time.Set(1.0f, true);
   }

   if(m_dragging && m_tab_clicked_index<m_wnd_MDI.GetWindowCount())
      DrawWindow(CalculateDraggingTabRect(), m_wnd_MDI.GetWindow(m_tab_clicked_index), 0);
}

struct Shortcut
{
   Shortcut(Prop::Server *ppropServer, Prop::Character *ppropCharacter) noexcept : m_ppropServer(ppropServer), m_ppropCharacter(ppropCharacter) { }

   Prop::Server *m_ppropServer;
   Prop::Character *m_ppropCharacter;
};

void Wnd_Taskbar::PopupCharacterWindow(int2 position)
{
   // Sort the list of servers into stkServers
   Collection<Prop::Server *> stkServers;
   for(auto& ppropServer : g_ppropGlobal->propConnections().propServers())
   {
      Prop::Characters &propCharacters=ppropServer->propCharacters();

      if(propCharacters.Count()==0)
         continue;

      unsigned i=stkServers.LowerBound(ppropServer, [](const Prop::Server *pServer1, const Prop::Server *pServer2) { return pServer1->pclName().ICompare(pServer2->pclName())<0; });
      stkServers.Insert(i, ppropServer);
   }

   PopupMenu menu;
   Collection<Shortcut> stkShortcuts;
   for(auto &ppropServer : stkServers)
   {
      Prop::Characters &propCharacters=ppropServer->propCharacters();

      if(propCharacters.Count()==1)
      {
         Prop::Character *ppropCharacter=propCharacters.First();

         stkShortcuts.Push(ppropServer, ppropCharacter);

         FixedStringBuilder<256> name(ppropServer->pclName(), " - ", ppropCharacter->pclName());
         menu.Append(MF_STRING, stkShortcuts.Count(), name);
      }
      else
      {
         PopupMenu menuSub;

         // Build a sorted list of characters
         Collection<Prop::Character *> stkCharacters;
         for(auto &ppropCharacter : propCharacters)
         {
            unsigned i=stkCharacters.LowerBound(ppropCharacter, [](const Prop::Character *pCharacter1, const Prop::Character *pCharacter2) { return pCharacter1->pclName().ICompare(pCharacter2->pclName())<0; });
            stkCharacters.Insert(i, ppropCharacter);
         }

         // Add them to the sub menu
         for(auto &ppropCharacter : stkCharacters)
         {
            stkShortcuts.Push(ppropServer, ppropCharacter);
            menuSub.Append(MF_STRING, stkShortcuts.Count(), ppropCharacter->pclName());
         }

         menu.Append(std::move(menuSub), ppropServer->pclName());
      }
   }

   int id=TrackPopupMenu(menu, TPM_RETURNCMD | (g_ppropGlobal->fTaskbarOnTop() ? 0 : TPM_BOTTOMALIGN), position, *this, nullptr);
   if(id==0)
      return;

   const Shortcut &shortcut=stkShortcuts[id-1];
   m_wnd_MDI.Connect(shortcut.m_ppropServer, shortcut.m_ppropCharacter, nullptr, true);
}

void Wnd_Taskbar::OnTimerToolTip()
{
   int2 position=ScreenToClient(GetCursorPos())/g_dpiScale;

   TooltipType type{TooltipType::None};
   unsigned index{};

   // See what item we're over
   if(m_rcToolbar.IsInside(position))
   {
      index=(position.x-m_rcToolbar.left)/CalculateToolbarX(1);
      if(index<std::size(c_toolbar_items))
         type=TooltipType::Button;
   }
   else if(m_rcTabs.IsInside(position))
   {
      index=(position.x-m_rcTabs.left)/m_width;
      if(index<m_wnd_MDI.GetWindowCount())
         type=TooltipType::Tab;
   }

   // Nothing has changed, so leave existing tip up
   if(Windows::Controls::HasToolTip(*this))
   {
      // If anything changed, hide the tooltip and reset the timer
      if(m_tooltip_type!=type || m_tooltip_index!=index)
      {
         Windows::Controls::HideToolTip(*this);
         m_timer_tool_tip.Set(0.5f);
      }
      return;
   }

   // Otherwise set the new tooltip
   m_tooltip_type=type;
   m_tooltip_index=index;

   if(m_tooltip_type==TooltipType::Button)
      Windows::Controls::ShowToolTip(*this, c_toolbar_items[m_tooltip_index].m_tooltip);
   else if(m_tooltip_type==TooltipType::Tab)
   {
      Wnd_Main &window=m_wnd_MDI.GetWindow(m_tooltip_index);
      FixedStringBuilder<256> string; GetWindowText(window.GetConnection(), string);

      if(!string)
         string("(Unconnected)");

      Windows::Controls::ShowToolTip(*this, string);
   }
}
