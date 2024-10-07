struct Connection;

struct TaskbarTheme
{
   Color background{245, 245, 245}; // {31, 31, 31}
   Color highlight{51, 183, 255}; // , {33, 122, 170}};
   Color flash{0x00, 0x70, 0xBB};
   Color text{Colors::Black};
   Color text_faint{Colors::Gray};
   Color text_disconnected{Colors::Gray};
   Color tab_divider{Colors::DkGray};
};

extern TaskbarTheme g_taskbar_theme;

struct Wnd_Taskbar : Wnd_D2D
{
   Wnd_Taskbar(Wnd_MDI &wndMDI);
   ~Wnd_Taskbar() noexcept;

   void SetActiveWindow(Wnd_Main &wndMain, bool fActive);
   void Refresh(Wnd_Main &wndMain) { Refresh(); } // Redraw only a particular window button
   void Refresh() { Invalidate(); }
   void DrawTabNumbers(bool draw) { if(m_draw_tab_numbers!=draw) { m_draw_tab_numbers=draw; Refresh(); } }

   void OnSize(Rect &rcClient);
   void OpenMenu();

private:

   Color DrawWindow(RectF rcWindow, Wnd_Main &wnd, unsigned tabNumber);
   void GetWindowText(Connection &connection, StringBuilder &string);

   friend TWindowImpl;
   using Wnd_D2D::On;
   LRESULT WndProc(const Message &msg) override;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Size &msg);
   LRESULT On(const Msg::LButtonDown &msg);
   LRESULT On(const Msg::LButtonUp &msg);
   LRESULT On(const Msg::MouseMove &msg);
   LRESULT On(const Msg::RButtonUp &msg);
   LRESULT On(const Msg::CaptureChanged &msg);
   LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
   LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }

   void CreateD2DResources() override;
   void Paint() override;

   RectF CalculateButtonRect(unsigned iButton) const;
   RectF CalculateTabRect(unsigned iWindow) const;
   RectF CalculateDraggingTabRect() const { return RectF{float2(m_width, m_rcTabs.size().y)}+ScreenToClient(Windows::GetMessagePos())/g_dpiScale-m_pt_dragged; }
   int CalculateToolbarX(unsigned iButton) const noexcept { return m_rcTabs.size().y*iButton; }
   void PopupCharacterWindow(int2 position);
   void OnTimerToolTip();
   void OnButton();

   Wnd_MDI &m_wnd_MDI;
   const Font *mp_font_emoji, *mp_font_text, *mp_font_text_bold;
   float m_font_size, m_ellipsis_width;

   Wnd_Main *mp_active_window{};

   float m_width{1}; // Width of a single tab
   RectF m_rcToolbar;
   RectF m_rcTabs;
   RectF m_rcTime;

   float2 m_pt_clicked;
   bool m_dragging{};
   float2 m_pt_dragged; // Location relative to top left of tab that we started dragging
   unsigned m_toolbar_clicked_index{~0U}; // Toolbar button being clicked
   bool m_over_toolbar{}; // Mouse is over the clicked toolbar button
   bool m_draw_tab_numbers{};

   unsigned m_tab_clicked_index{~0U}; // Window being clicked
   Time::Timer m_timer_time{[this]() { Refresh(); }};

   Time::Timer m_timer_tool_tip{[this]() { OnTimerToolTip(); }};
   unsigned m_tooltip_index{};
   enum struct TooltipType { None, Button, Tab };
   TooltipType m_tooltip_type{TooltipType::None};
};
