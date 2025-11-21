bool IsWordBreak(char c);
struct Wnd_InputPane;
struct ITextDocument;

struct InputControl : IEditHost, Controls::RichEdit, Events::Sends_Deleted
{
   InputControl(Wnd_Main &wnd_main, Prop::InputWindow &props, Wnd_InputPane *p_input_pane=nullptr, IEditHost *pIEditHost=nullptr);
   InputControl(const InputControl &)=delete;
   ~InputControl();

   void Create(Window wndParent);

   Prop::InputWindow &GetProps() { return *mp_props; }
   void SetProps(Prop::InputWindow &props) { mp_props=&props; ApplyProps(); }
   void ApplyProps(); // Used if props change but pointer is the same

   void SetText(ConstString text);
   void ReplaceSelection(ConstString text);
   bool IsPrimary() const;

   void ConvertReturns();
   void ConvertTabs();
   void ConvertSpaces();

   bool IsSpellChecking() const { return m_fSpellCheck; }
   void EnableSpellChecking(bool fEnable);
   void SpellCheckRange(CHARRANGE range);
   void RichEditPopup(unsigned index, int2 position);

   void Autocomplete(Collection<const Text::Lines*> &linesCollection, bool whole_line);
   Wnd_Main &GetWndMain() { return m_wnd_main; }

   // If the window is our m_edInput window, call these to handle things
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Notify &msg);

private:

   void OnChange();
   LRESULT On(SELCHANGE &selchange);

   bool EditChar(const Msg::Char &msg) override;
   bool EditKey(const Msg::Key &msg) override;

   Wnd_Main &m_wnd_main;
   CntPtrTo<Prop::InputWindow> mp_props;
   CntPtrTo<ITextDocument> p_text_document;
   Wnd_InputPane *mp_input_pane;
   IEditHost *m_pIEditHost; // Set if we're an edit window vs an input window.
   unsigned m_uncheckedSelection{~0U}; // If ~0U there is no unchecked selection position
   unsigned m_inputLastSize{};
   bool m_fSpellCheck{true};
};

struct Wnd_InputPane
 : TWindowImpl<Wnd_InputPane>, 
   Events::ReceiversOf<Wnd_InputPane, GlobalInputSettingsModified>
{
   static ATOM Register();

   Wnd_InputPane(Wnd_Main &wndMain, Prop::InputWindow &props);
   Wnd_InputPane(const Wnd_InputPane &)=delete;
   ~Wnd_InputPane();

   void On(const GlobalInputSettingsModified &event);

   LRESULT WndProc(const Windows::Message &msg) override;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Size &msg);
   LRESULT On(const Msg::Close &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Notify &msg);
   LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
   LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }

   InputControl m_input;
   Wnd_Docking &GetDocking() const { return *mp_docking; }
   void UpdateTitle();

private:
   Wnd_Docking *mp_docking;
};

struct Wnd_EditPane : Wnd_Dialog, IEditHost
{
   Wnd_EditPane(Wnd_Main &wndMain, ConstString title, bool fDockable, bool fSpellCheck);
   Wnd_EditPane(const Wnd_EditPane &)=delete;
   ~Wnd_EditPane();

   // IEditHost
   bool EditKey(const Msg::Key &msg) override;
   void EditSelChanged() override;

   LRESULT WndProc(const Windows::Message &msg) override;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Close &msg);
   LRESULT On(const Msg::WindowPosChanged &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Notify &msg);
   LRESULT On(const Msg::SetFocus &msg);
   LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
   LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }

   void Find();

   Wnd_Docking &GetDocking() const { return *mp_docking; }
   InputControl &GetInput() { return m_input; }
   ConstString GetTitle() const { return m_title; }

private:

   enum
   {
      IDC_SEND  = 100,
      IDC_EDIT,
      IDC_OPTIONS,
   };

   Wnd_Main &m_wnd_main;
   OwnedString m_title;
   Wnd_Docking *mp_docking;
   InputControl m_input;

   AL::Button *m_pButton_send, *m_pButton_edit, *m_pButton_options;
   AL::Static *mp_info;
};

struct Wnd_EditPropertyPane : Wnd_Dialog, IEditHost, OwnerPtrData
{
   Wnd_EditPropertyPane(Wnd_Main &wndMain, Prop::Character &propCharacter);
   Wnd_EditPropertyPane(const Wnd_EditPropertyPane &)=delete;
   ~Wnd_EditPropertyPane();

   void Save();

   // IEditHost
   bool EditKey(const Msg::Key &msg) override;

   LRESULT WndProc(const Windows::Message &msg) override;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Close &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Notify &msg);
   LRESULT On(const Msg::SetFocus &msg);
   LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
   LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }

   void Find();

   Wnd_Docking &GetDocking() const { return *mp_docking; }
   InputControl &GetInput() { return m_input; }

private:

   enum
   {
      IDC_EDIT = 100,
      IDC_OPTIONS,
   };

   Wnd_Main &m_wnd_main;
   Wnd_Docking *mp_docking;
   InputControl m_input;
   Prop::Character &m_propCharacter;
};
