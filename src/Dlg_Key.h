struct Wnd_Key : TWindowImpl<Wnd_Key>, AutoLayout::LayoutEngine
// Brings up a modal window that demands a keypress and sends a notification
// if the key press is made (and no notification if the user cancels)
{
   interface INotify
   {
      virtual void Key(const KEY_ID &key)=0;
      virtual void Release()=0;
   };

   Wnd_Key(Window wndParent, INotify &notify);
   ~Wnd_Key() noexcept;

   static ATOM Register();

private:

   friend TWindowImpl;
   LRESULT WndProc(const Message &msg) override;

   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Close &msg);
   LRESULT On(const Msg::Key &msg);
   LRESULT On(const Msg::LButtonDown &msg);
   using AutoLayout::LayoutEngine::On;

   INotify &m_notify;
};

struct Dlg_Key : Wnd_ChildDialog, IEditHost
// The key dialog, that shows the name of the current key and the modifiers applied
// to it.
{
   Dlg_Key(Window wndParent);

   void SetKey(KEY_ID key);
   const KEY_ID &GetKey();

private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Enable &msg);
   LRESULT On(const Msg::Command &msg);

   bool IEditHost::EditKey(const Msg::Key &msg) override;

   KEY_ID m_key;

   AL::Edit *m_pedKey;
   AL::Button *m_pbtKey;
   AL::CheckBox *m_pcbControl, *m_pcbAlt, *m_pcbShift;
};
