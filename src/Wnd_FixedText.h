//
// FixedText
//
// Command prompt like text window with no scrollback

struct Wnd_FixedText : TWindowImpl<Wnd_FixedText>, WindowEvents
{
   static ATOM Register();
   Wnd_FixedText(Window wndParent, int2 size);

   void Clear();
   void Write(ConstString string);
   void SetCursorX(int x) { PinBetween(x, 0, m_size.x-1); m_ptCursor.x=x; m_ptDirtyStart.x=x; }
   void SetCursorY(int y) { PinBetween(y, 0, m_size.y-1); m_ptCursor.y=y; m_ptDirtyStart.y=y; }
   void SetCursor(int2 pt) { SetCursorX(pt.x); SetCursorY(pt.y); }
   const int2 &GetCursor() const { return m_ptCursor; }

private:

   void LineFeed();
   void CarriageReturn();
   void ScrollUp();
   void DirtyFlush();

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Paint &msg);

   int2 m_size;
   Handle<HFONT> m_font;
   TEXTMETRIC m_tm;
   int2 m_sizeText;

   int2 m_ptCursor;
   int2 m_ptDirtyStart;

   Prop::Font m_propFont;

   Collection<UniquePtr<char[]>> m_pLines;
};
