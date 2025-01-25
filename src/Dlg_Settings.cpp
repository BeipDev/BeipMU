#include "Main.h"
#include "Speller.h"
#include "Wnd_Main.h"
#include "Dlg_Key.h"
#include "Emoji.h"
#include "Wnd_Taskbar.h"
#include "Speech.h"

namespace OM
{
void DisplayScriptingEngines(Controls::ComboBox &list);
};

static void SetUIFont(Window window, ConstString face, int size)
{
   g_ppropGlobal->pclUIFontName(face);
   g_ppropGlobal->UIFontSize(size);
   MessageBox(window, "Changes will be applied next time the app starts", "Note:", MB_ICONINFORMATION|MB_OK);
}

struct Setting
{
   virtual ~Setting() {}
   virtual void Save()=0;
};

struct Setting_Bool : Setting
{
   void Save() override { m_writer(mp_checkbox->IsChecked()); }

   std::function<void(bool v)> m_writer;
   AL::CheckBox *mp_checkbox;
};

struct SettingsDialog : Wnd_ChildDialog
{
   virtual void Save()=0;
   virtual void OnCreate() { Assert(false); }

   void Create(Window wndParent)
   {
      Wnd_ChildDialog::Create("", wndParent);
   }

   LRESULT On(const Msg::Create &msg)
   {
      mp_root=m_layout.CreateGroup_Vertical();
      *mp_root >> AL::Style::Attach_Bottom;
      m_layout.SetRoot(mp_root);
      OnCreate();
      return msg.Success();
   }

   AL::Group &CreateSection(ConstString label, int weight=1)
   {
      // Only add an extra space if we're not the first item
      if(!m_first)
         *mp_root << m_layout.CreateSpacer({0, Controls::Control::m_tmButtons.tmHeight/2});
      else
         m_first=false;

      AL::Group_Horizontal *p_group=m_layout.CreateGroup_Horizontal(); *mp_root << p_group; p_group->weight(0);
      p_group->MatchBaseline(false);
      *p_group >> AL::Style::Attach_Vert; // Center vertically

      *p_group << m_layout.CreateStatic(label, 0, true);
      auto line=m_layout.CreateStatic(" ", SS_ETCHEDHORZ); line->weight(1); line->szMinimum()=int2(2,2);
      *p_group << line;

      // Add an indent on the left
      auto *pGH=m_layout.CreateGroup_Horizontal(); *mp_root << pGH; pGH->weight(weight);
      *pGH << m_layout.CreateSpacer(int2(5*g_dpiScale, 0));
      auto *p_section=m_layout.CreateGroup_Vertical();
      *pGH << p_section;

      return *p_section;
   }

   AL::Group &GetRoot() { return *mp_root; }

   void SaveSettings()
   {
      for(auto &p : m_settings)
         p->Save();
   }

   AL::CheckBox* AddBool(AL::Group &g, ConstString label, bool value, std::function<void(bool v)> writer)
   {
      auto p_setting=MakeUnique<Setting_Bool>();
      p_setting->m_writer=writer;
      p_setting->mp_checkbox=m_layout.CreateCheckBox(-1, label);
      p_setting->mp_checkbox->Check(value);
      g << p_setting->mp_checkbox;

      auto *p_checkbox=p_setting->mp_checkbox;
      m_settings.Push(std::move(p_setting));
      return p_checkbox;
   }

   AL::CheckBox* AddBool(AL::Group &g, ConstString label, bool &value)
   {
      return AddBool(g, label, value, [&value](bool v) { value=v; });
   }

   Collection<UniquePtr<Setting>> m_settings;

private:
   AL::Group *mp_root{};
   bool m_first{true};
};

struct Dlg_General : SettingsDialog
{
private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   void Save() override;
   void OnCreate() override;
   void UpdateEnabled();

   // Window Messages
   using SettingsDialog::On;

   AL::Edit     *mp_inline_image_height;
   AL::Edit     *mp_avatar_width, *mp_avatar_height;

   Controls::ComboBox m_coVoice;
};

LRESULT Dlg_General::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create>(msg);
}

void Dlg_General::Save()
{
   // Windows
   if(int value;mp_inline_image_height->Get(value))
      g_ppropGlobal->InlineImageHeight(value);
   if(int value;mp_avatar_width->Get(value))
      g_ppropGlobal->AvatarWidth(value);
   if(int value;mp_avatar_height->Get(value))
      g_ppropGlobal->AvatarHeight(value);

   // Speech Voice
   if(m_coVoice.GetCount()>1)
   {
      if(int selection=m_coVoice.GetCurSel(); selection!=-1)
      {
         auto &sapi=SAPI::GetInstance();
         if(unsigned(selection)>=sapi.GetVoices().Count())
            g_ppropGlobal->pclVoiceID(ConstString());
         else
            g_ppropGlobal->pclVoiceID(sapi.GetVoices()[selection].m_id);
         SAPI::GetInstance().SetVoice();
      }
   }
}

void Dlg_General::UpdateEnabled()
{
}

void Dlg_General::OnCreate()
{
   {
      auto &g=CreateSection(STR_Windows);
      g >> AL::Style::Attach_Right;

      AddBool(g, "Changing Layout requires holding down Control", g_ppropGlobal->fLayoutWithCtrl());
      AddBool(g, STR_ActivateDisconnect, g_ppropGlobal->propConnections().fActivateDisconnect());
      AddBool(g, "Show image viewer automatically", g_ppropGlobal->fAutoImageViewer());
      {
         auto *p_gh=m_layout.CreateGroup_Horizontal(); g << p_gh;

         AddBool(*p_gh, "Show images inline, with height", g_ppropGlobal->fInlineImages());

         mp_inline_image_height=m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
         *p_gh << mp_inline_image_height << m_layout.CreateStatic("dips");
      }

      AddBool(g, "Animated images start as paused", g_ppropGlobal->fAnimatedImagesStartPaused());

      {
         auto *p_gh=m_layout.CreateGroup_Horizontal(); g << p_gh;
         *p_gh << m_layout.CreateStatic("Avatar width");
         mp_avatar_width=m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
         mp_avatar_height=m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
         *p_gh << mp_avatar_width << m_layout.CreateStatic("dips, min height") << mp_avatar_height << m_layout.CreateStatic("dips");
      }
   }
   {
      auto &g=CreateSection(STR_Alerts);

      AddBool(g, STR_AskBeforeDisconnecting, g_ppropGlobal->propWindows().fAskBeforeDisconnecting());
      AddBool(g, STR_AskBeforeClosing, g_ppropGlobal->propWindows().fAskBeforeClosing());
   }
   {
      auto &g=CreateSection("Speech");
      {
         auto &gh=*m_layout.CreateGroup_Horizontal(); g << &gh;
         gh << m_layout.CreateStatic("Voice:");
         m_coVoice.Create(*this, -1, int2(20, 16), CBS_DROPDOWNLIST|WS_VSCROLL);
         gh << m_layout.AddComboBox(m_coVoice);
      }
   }

   // Windows
   mp_inline_image_height->Set(g_ppropGlobal->InlineImageHeight());
   mp_avatar_width->Set(g_ppropGlobal->AvatarWidth());
   mp_avatar_height->Set(g_ppropGlobal->AvatarHeight());

   // Speech
   for(auto &voice : SAPI::GetInstance().GetVoices())
   {
      int index=m_coVoice.AddString(voice.m_name);
      m_coVoice.SetItemData(index, nullptr);
      if(voice.m_id==g_ppropGlobal->pclVoiceID())
         m_coVoice.SetCurSel(index);
   }
   m_coVoice.AddString("Default");
   if(m_coVoice.GetCurSel()==-1)
      m_coVoice.SetCurSel(m_coVoice.GetCount()-1);

   UpdateEnabled();
}

struct Dlg_Input : SettingsDialog
{
private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   void Save() override;
   void OnCreate() override;
   void UpdateEnabled();

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);

   AL::CheckBox *m_pcbSpellCheck;
   Controls::TComboBox<OwnedString> m_coSpellLanguage;
};

LRESULT Dlg_Input::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Input::Save()
{
   // SpellCheck
   {
      bool changed=m_pcbSpellCheck->IsChecked()!=g_ppropGlobal->fSpellCheck();

      if(m_coSpellLanguage.GetCount()>1)
      {
         int selection=m_coSpellLanguage.GetCurSel();
         if(selection!=-1)
         {
            const auto &spellLanguage=m_coSpellLanguage.GetItemData(selection);
            if(spellLanguage!=g_ppropGlobal->pclSpellLanguage())
            {
               changed=true;
               g_ppropGlobal->pclSpellLanguage(spellLanguage);
            }
         }
      }
      if(changed && g_ppropGlobal->fSpellCheck())
         Speller::SetLanguage(g_ppropGlobal->pclSpellLanguage());
   }
}

void Dlg_Input::UpdateEnabled()
{
   EnableWindows(m_pcbSpellCheck->IsChecked(), m_coSpellLanguage);
}

void Dlg_Input::OnCreate()
{
   {
      auto &g=CreateSection("Options");
      g >> AL::Style::Attach_Right;

      AddBool(g, "Send unrecognized commands to server", g_ppropGlobal->fSendUnrecognizedCommands());
      AddBool(g, "Prevent smart quote mode (no “” quotes, only \")", g_ppropGlobal->fPreventSmartQuotes());
      AddBool(g, "Automatically show history window while navigating input history", g_ppropGlobal->fAutoShowHistory());
   }

   {
      auto &g=CreateSection("Spell Check");

      m_pcbSpellCheck=AddBool(g, "Enable", g_ppropGlobal->fSpellCheck());
      {
         auto &gh=*m_layout.CreateGroup_Horizontal(); g << &gh;
         gh << m_layout.CreateStatic("Language:");
         m_coSpellLanguage.Create(*this, -1, int2(20, 16), CBS_DROPDOWNLIST|CBS_SORT|WS_VSCROLL);
         gh << m_layout.AddComboBox(m_coSpellLanguage);
      }
   }

   // Spell Check
   {
      if(auto name=GetLocaleInfo(g_ppropGlobal->pclSpellLanguage(), LOCALE_SLOCALIZEDDISPLAYNAME))
      {
         int index=m_coSpellLanguage.AddString(name);
         m_coSpellLanguage.SetItemData(index, nullptr);
         m_coSpellLanguage.SetCurSel(index);
      }
   }

   UpdateEnabled();
}

LRESULT Dlg_Input::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==CBN_DROPDOWN && msg.wndCtl()==m_coSpellLanguage &&
      m_coSpellLanguage.GetCount()<2)
   {
      m_coSpellLanguage.Reset();
      Speller::GetLanguages(m_coSpellLanguage);

      if(auto name=GetLocaleInfo(g_ppropGlobal->pclSpellLanguage(), LOCALE_SLOCALIZEDDISPLAYNAME))
      {
         int iSelection=m_coSpellLanguage.FindStringExact(name);
         m_coSpellLanguage.SetCurSel(iSelection!=-1 ? iSelection : 0);
      }
      return msg.Success();
   }

   return msg.Success();
}

static constexpr const ConstString s_keyNames[] = { STR_Shortcut_Table };

struct Dlg_KeyboardShortcuts : SettingsDialog
{
   ~Dlg_KeyboardShortcuts();

   void SetKey(int iKey, const KEY_ID &key);
   void Save() override; // Applies instantly, nothing to save
   void OnCreate() override;

private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Notify &msg);

   void UpdateItem(int iKey);
   void UpdateItem(int iItem, int iKey);

   Prop::Keys &m_propKeys{g_ppropGlobal->propKeys()};

   Dlg_Key *m_pDlgKey;

   enum
   {
      IDC_LIST=100,
      IDC_DEFAULT,
      IDC_DEFAULTS,
      IDC_REMOVE,
   };

   AL::Button *m_pbtDefault, *m_pbtReset, *m_pbtRemove;

   Controls::ListView m_lv;
};

LRESULT Dlg_KeyboardShortcuts::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command, Msg::Notify>(msg);
}

Dlg_KeyboardShortcuts::~Dlg_KeyboardShortcuts()
{
   m_pDlgKey->Close();
}

void Dlg_KeyboardShortcuts::SetKey(int iKey, const KEY_ID &key)
{
   m_propKeys.Set(iKey, key);
   UpdateItem(iKey);
   m_pDlgKey->SetKey(key);
}

void Dlg_KeyboardShortcuts::UpdateItem(int iKey)
// Simpler version of the below when the location in the list is not known
{
   Controls::ListView::FindInfo fi; fi.Param(iKey);
   UpdateItem(m_lv.FindItem(fi), iKey);
}

void Dlg_KeyboardShortcuts::UpdateItem(int iItem, int iKey)
{
   const KEY_ID key=m_propKeys.Get(iKey);

   FixedStringBuilder<256> sBuffer; key.KeyNameWithModifiers(sBuffer);

   m_lv.SetItemText(iItem, 1, sBuffer);

   if(key.iVKey)
   {
      for(int i=0;i<int(m_propKeys.Count());i++)
      {
         if(i==iKey)
            continue;

         if(m_propKeys.Get(i)==key)
         {
            if(MessageBox(*this, FixedStringBuilder<256>("Duplicate Key Binding Detected:\n\n", s_keyNames[iKey], "\n\nClear shortcut for ", s_keyNames[i], '?'), "Warning", MB_ICONQUESTION|MB_YESNO)==IDYES)
            {
               m_propKeys.Set(i, KEY_ID());
               UpdateItem(i);
            }
         }
      }
   }
}

void Dlg_KeyboardShortcuts::OnCreate()
{
   m_lv.Create(*this, IDC_LIST, LVS_REPORT | LVS_SINGLESEL | LVS_SORTASCENDING | LVS_SHOWSELALWAYS);
   m_lv.SetExtendedStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
   m_pDlgKey=new Dlg_Key(*this);

   m_pbtRemove=m_layout.CreateButton(IDC_REMOVE, STR_Clear);
   m_pbtReset=m_layout.CreateButton(IDC_DEFAULTS, STR_ResetAll);
   m_pbtDefault=m_layout.CreateButton(IDC_DEFAULT, STR_Default);

   GetRoot() << AL::Style::Attach_Bottom;

   auto &g=CreateSection("Keyboard Shortcuts");
   g << m_layout.AddControl(m_lv);

   {
      AL::Group *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
      pGH->weight(0);
      *pGH << m_pDlgKey; m_pDlgKey->weight(0);

      AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
      *pGV >> AL::Style::Attach_Bottom;
      *pGV << m_pbtReset << m_pbtRemove << m_pbtDefault;
   }

   Controls::ListView::Column lvc;
   lvc.Text(STR_Action);
   lvc.Width(50);
   m_lv.InsertColumn(0, lvc);

   lvc.Text(STR_Key);
   lvc.Format(LVCFMT_RIGHT);
   m_lv.InsertColumn(1, lvc);

   m_lv.SetItemCount(m_propKeys.Count());

   static_assert(_countof(s_keyNames)==m_propKeys.Count());
   for(unsigned i=0;i<m_propKeys.Count();i++)
   {
      Controls::ListView::Item lvi;
      lvi.Text(s_keyNames[i]);
      lvi.Param(i);
      int iItem=m_lv.InsertItem(lvi);
      UpdateItem(iItem, i);
   }

   m_lv.SetColumnWidth(0, LVSCW_AUTOSIZE);
   m_lv.SetColumnWidth(1, LVSCW_AUTOSIZE);

   m_lv.szMinimum().x=m_lv.GetColumnWidth(0)+m_lv.GetColumnWidth(1);
   Rect rcItem; m_lv.GetItemRect(0, rcItem, LVIR_LABEL);
   m_lv.szMinimum().y=rcItem.size().y*10;

   m_pDlgKey->Enable(false);
   m_lv.SetFocus();
}

LRESULT Dlg_KeyboardShortcuts::On(const Msg::Notify &msg)
{
   if(Window{msg.pnmh()->hwndFrom}!=m_lv)
      return 0;

   if(msg.pnmh()->code==LVN_ITEMACTIVATE)
   {
      struct Notify : Wnd_Key::INotify
      {
         Notify(Dlg_KeyboardShortcuts &dlg, int iKey) : m_dlg(dlg), m_iKey(iKey) { }

         void Key(const KEY_ID &key) { m_dlg.SetKey(m_iKey, key); }
         void Release() { delete this; }

      private:
         Dlg_KeyboardShortcuts &m_dlg;
         int    m_iKey;
      };

      new Wnd_Key(*this, *new Notify(*this, m_lv.GetItemParam(m_lv.GetNextItem(LVNI_SELECTED))));
      return 0;
   }

   if(msg.pnmh()->code==LVN_ITEMCHANGED)
   {
      auto *pNM=(Controls::ListView::NM *)msg.pnmh();

      auto stateOld=pNM->GetStateOld();
      auto stateNew=pNM->GetStateNew();

      // Unselecting?
      if(stateOld.fSelected() && !stateNew.fSelected())
      {
         m_propKeys.Set(pNM->lParam, m_pDlgKey->GetKey());
         UpdateItem(pNM->iItem, pNM->lParam);
      }
      else if(!stateOld.fSelected() && stateNew.fSelected())
      {
         m_pDlgKey->Enable(true);
         m_pDlgKey->SetKey(m_propKeys.Get(pNM->lParam));
      }
      return 0;
   }

   return 0;
}

void Dlg_KeyboardShortcuts::Save()
{
   // Ensure any just edited key gets saved
   int iSelected=m_lv.GetNextItem(LVNI_SELECTED);
   if(iSelected!=-1)
   {
      int iKey=m_lv.GetItemParam(iSelected);
      m_propKeys.Set(iKey, m_pDlgKey->GetKey());
   }
}

LRESULT Dlg_KeyboardShortcuts::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_REMOVE:
      {
         int iSelected=m_lv.GetNextItem(LVNI_SELECTED);
         if(iSelected==-1) break;

         int iKey=m_lv.GetItemParam(iSelected);
         m_propKeys.Set(iKey, KEY_ID());
         UpdateItem(iKey);
         m_pDlgKey->SetKey(m_propKeys.Get(iKey));
         break;
      }

      case IDC_DEFAULT:
      {
         int iSelected=m_lv.GetNextItem(LVNI_SELECTED);
         if(iSelected==-1) break;

         int iKey=m_lv.GetItemParam(iSelected);

         m_propKeys.GetDataInfos()[iKey]->SetToDefault(&m_propKeys);
         UpdateItem(iKey);
         m_pDlgKey->SetKey(m_propKeys.Get(iKey));
         break;
      }

      case IDC_DEFAULTS:
      {
         for(unsigned iKey=0;iKey<m_propKeys.Count();iKey++)
         {
            m_propKeys.GetDataInfos()[iKey]->SetToDefault(&m_propKeys);
            UpdateItem(iKey);
         }

         int iSelected=m_lv.GetNextItem(LVNI_SELECTED);
         if(iSelected!=-1) // Update the current selection if necessary
            m_pDlgKey->SetKey(m_propKeys.Get(m_lv.GetItemParam(iSelected)));
         break;
      }

   }

   return msg.Success();
}


#include "Sounds.h"

// Black, Red, Green, Yellow, Blue, Magenta, Cyan, Gray, DarkGray, Red, Green, Yellow, Blue, Magenta, Cyan, White
static constexpr const Color c_defaults_CMD[16]=
{
   RGB(  0,  0,  0), RGB(128,  0,  0), RGB(  0,128,  0), RGB(128,128,  0), RGB(  0,  0,128), RGB(128,  0,128), RGB(  0,128,128), RGB(192,192,192), 
   RGB(128,128,128), RGB(255,  0,  0), RGB(  0,255,  0), RGB(255,255,  0), RGB(  0,  0,255), RGB(255,  0,255), RGB(  0,255,255), RGB(255,255,255), 
};

static constexpr const Color c_defaults_VGA[16]=
{
   RGB(  0,  0,  0), RGB(170,  0,  0), RGB(  0,170,  0), RGB(170, 85,  0), RGB(  0,  0,170), RGB(170,  0,170), RGB(  0,170,170), RGB(170,170,170), 
   RGB( 85, 85, 85), RGB(255, 85, 85), RGB( 85,255, 85), RGB(255,255, 85), RGB( 85, 85,255), RGB(255, 85,255), RGB( 85,255,255), RGB(255,255,255), 
};

#if 0 // These are the regular defaults, so we don't use this version, we just use the config defaults
static constexpr const Color c_defaults_xterm[16]=
{
   RGB(  0,  0,  0), RGB(205,  0,  0), RGB(  0,205,  0), RGB(205,205,  0), RGB(  0,  0,238), RGB(205,  0,205), RGB(  0,205,205), RGB(229,229,229), 
   RGB(127,127,127), RGB(255,  0,  0), RGB(  0,255,  0), RGB(255,255,  0), RGB( 92, 92,255), RGB(255,  0,255), RGB(  0,255,255), RGB(255,255,255), 
};
#endif

static constexpr const Color c_defaults_Old[16]=
{
   RGB(  0,  0,  0), RGB(255,  0,  0), RGB(  0,255,  0), RGB(192,192,  0), RGB(  0,  0,255), RGB(192,  0,192), RGB(  0,192,192), RGB(192,192,192),
   RGB(128,128,128), RGB(255,128,128), RGB(128,255,128), RGB(255,255,  0), RGB(128,128,255), RGB(255,  0,255), RGB(  0,255,255), RGB(255,255,255),
};

static constexpr const Color c_defaults_Bright[16]=
{
   RGB(  0,  0,  0), RGB(255,  0,  0), RGB(  0,255,  0), RGB(255,255,  0), RGB(  0,  0,255), RGB(255,  0,255), RGB(  0,255,255), RGB(255,255,255), 
   RGB(128,128,128), RGB(255,128,128), RGB(128,255,128), RGB(255,255,128), RGB(128,128,255), RGB(255,128,255), RGB(128,255,255), RGB(255,255,255), 
};


struct Dlg_AnsiColors : SettingsDialog
{
private:

   LRESULT WndProc(const Message &msg) override;

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::MeasureItem &msg);
   LRESULT On(const Msg::DrawItem &msg);

   void Save() override;
   void OnCreate() override;

   enum
   {
      IDC_BUTTON_BEEPCHANGE = 100,
      IDC_BUTTON_BEEPPLAY,
      IDC_DEFAULT,
      IDC_XTERMDEFAULTS,
      IDC_CMDDEFAULTS,
      IDC_VGADEFAULTS,
      IDC_OLDDEFAULTS,
      IDC_BRIGHTDEFAULTS,
      IDC_CHANGE,
   };

   // Color List
   Controls::ListBox m_lbColors;

   AL::Radio *m_pcbBeepSystem, *m_pcbBeepCustom;
   AL::Edit *m_pedBeepFileName;

   Prop::Ansi   &m_propAnsi{g_ppropGlobal->propAnsi()};
   Prop::Colors &m_propColors{g_ppropGlobal->propAnsi().propColors()};
};

LRESULT Dlg_AnsiColors::WndProc(const Message &msg)
{
   switch(msg.uMessage())
   {
      case Msg::Create::ID: return Call_On(Create);
      case Msg::Command::ID: return Call_On(Command);
      case Msg::MeasureItem::ID: return Call_On(MeasureItem);
      case Msg::DrawItem::ID: return Call_On(DrawItem);
   }

   return __super::WndProc(msg);
}

void Dlg_AnsiColors::Save()
{
   m_propAnsi.fBeepSystem(m_pcbBeepSystem->IsChecked());
   m_propAnsi.pclBeepFileName(m_pedBeepFileName->GetText());
}

void Dlg_AnsiColors::OnCreate()
{
   GetRoot() << AL::Style::Attach_Bottom;

   {
      auto &g=CreateSection(STR_TextColors);
      {
         AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;

         m_lbColors.Create(*this, -1, int2(16, 8), WS_VSCROLL|LBS_OWNERDRAWFIXED);
         *pGH << m_layout.AddControl(m_lbColors);

         {
            AL::GroupBox *pGB=m_layout.CreateGroupBox("Presets:"); *pGH << pGB;
            pGB->weight(0); 
            {
               AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);
               *pGV << m_layout.CreateButton(IDC_XTERMDEFAULTS, "XTerm");
               *pGV << m_layout.CreateButton(IDC_CMDDEFAULTS, "CMD");
               *pGV << m_layout.CreateButton(IDC_VGADEFAULTS, "VGA");
               *pGV << m_layout.CreateButton(IDC_OLDDEFAULTS, "Old");
               *pGV << m_layout.CreateButton(IDC_BRIGHTDEFAULTS, STR_Bright);
            }
         }
      }
      {
         AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         pGH->MatchWidth(true);
         *pGH >> AL::Style::Attach_Right;

         *pGH << m_layout.CreateButton(IDC_CHANGE, STR_Change);
         *pGH << m_layout.CreateButton(IDC_DEFAULT, STR_Default);
      }
   }
   {
      auto &g=CreateSection(STR_Appearance);
      AddBool(g, STR_PreventInvisible, m_propAnsi.fPreventInvisible());
      AddBool(g, STR_ResetOnNewLine, m_propAnsi.fResetOnNewLine());
      AddBool(g, STR_UseFontBold, m_propAnsi.fFontBold());
      AddBool(g, "Parse Blinking", m_propAnsi.FlashSpeed()!=0, [this](bool v) { m_propAnsi.FlashSpeed(v ? 500 : 0);  });
   }
   {
      auto &g=CreateSection(STR_Beep);
      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;

      {
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

         AddBool(*pGV, STR_EnableBeep, m_propAnsi.fBeep());
         m_pcbBeepSystem=m_layout.CreateRadio(-1, STR_UseSystemBeep, true);
         *pGV << m_pcbBeepSystem;

         AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; *pGH >> AL::Style::Attach_Vert;
         m_pcbBeepCustom=m_layout.CreateRadio(-1, STR_Custom);
         m_pedBeepFileName=m_layout.CreateEdit(-1, int2(15, 1), ES_AUTOHSCROLL);
         m_pedBeepFileName->LimitText(Prop::Ansi::pclBeepFileName_MaxLength());
         m_pedBeepFileName->SetText(m_propAnsi.pclBeepFileName());

         *pGH << m_pcbBeepCustom << m_pedBeepFileName;
         (m_propAnsi.fBeepSystem() ? m_pcbBeepSystem : m_pcbBeepCustom)->Check(true);
      }
      {
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

         *pGV << m_layout.CreateButton(IDC_BUTTON_BEEPPLAY, STR_Play);
         *pGV << m_layout.CreateButton(IDC_BUTTON_BEEPCHANGE, STR_Change___);
      }
   }
   {
      auto &g=CreateSection("Misc");
      AddBool(g, STR_ParseAnsi, m_propAnsi.fParse());
   }

   for(auto *p_color : m_propColors.GetDataInfos())
      m_lbColors.AddData(p_color->GetName().stringz()); // Added as item data, not as a string

   m_lbColors.SetCurSel(0);
}

LRESULT Dlg_AnsiColors::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_CHANGE:
      {
         Color clr=m_propColors.Get(m_lbColors.GetCurSel());
         if(ChooseColorSimple(*this, &clr))
         {
            m_propColors.Set(m_lbColors.GetCurSel(), clr);
            m_lbColors.Invalidate(true);
         }
         break;
      }

      case IDC_DEFAULT:
      case IDC_CMDDEFAULTS:
      case IDC_VGADEFAULTS:
      case IDC_XTERMDEFAULTS:
      case IDC_OLDDEFAULTS:
      case IDC_BRIGHTDEFAULTS:

         if(msg.iID()==IDC_DEFAULT)
            m_propColors.GetDataInfos()[m_lbColors.GetCurSel()]->SetToDefault(&m_propColors);
         else if(msg.iID()==IDC_XTERMDEFAULTS)
            m_propColors.ResetToDefaults();
         else
         {
            const Color *pColors=nullptr;
            switch(msg.iID())
            {
               case IDC_VGADEFAULTS: pColors=c_defaults_VGA; break;
               case IDC_OLDDEFAULTS:    pColors=c_defaults_Old; break;
               case IDC_BRIGHTDEFAULTS: pColors=c_defaults_Bright; break;
               default:
               case IDC_CMDDEFAULTS: pColors=c_defaults_CMD; break;
            }

            for(unsigned i=0;i<16;i++)
               m_propColors.Set(i, pColors[i]);
         }

         m_lbColors.Invalidate(true);
         break;

      case IDC_BUTTON_BEEPCHANGE:
         {
            File::Chooser cf;
            cf.SetTitle(STR_SoundFile);
            cf.SetFilter(STR_SoundFileFilter, 0);

            FixedStringBuilder<256> filename(m_pedBeepFileName->GetText());

            if(cf.Choose(*this, filename, false))
               m_pedBeepFileName->SetText(filename);
         }
         break;

      case IDC_BUTTON_BEEPPLAY:
         if(m_pcbBeepSystem->IsChecked())
            MessageBeep(MB_OK);
         else
            PlaySound(m_pedBeepFileName->GetText());
         break;
   }

   return msg.Success();
}

LRESULT Dlg_AnsiColors::On(const Msg::MeasureItem &msg)
{
   msg->itemHeight=Controls::Control::m_tmButtons.tmHeight;
   msg->itemWidth=Controls::Control::m_tmButtons.tmAveCharWidth*12;
   return true;
}

LRESULT Dlg_AnsiColors::On(const Msg::DrawItem &msg)
{
   // Draw the standard listbox text
   DC dc(msg->hDC);
   Rect &rcItem=(Rect&)(msg->rcItem); rcItem.left+=rcItem.size().y;
   bool fSelected=msg->itemState&ODS_SELECTED;
   dc.FillRect(rcItem, GetSysColorBrush(fSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
   dc.SetTextColor(GetSysColor(fSelected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
   dc.SetBackgroundColor(GetSysColor(fSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
   dc.TextOut(int2(rcItem.left+2, rcItem.bottom-Controls::Control::m_tmButtons.tmHeight), SzToString((const char *)msg->itemData));

   if(msg->itemState&ODS_FOCUS)
   {
      dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
      dc.SetBackgroundColor(GetSysColor(COLOR_WINDOW));
      dc.DrawFocusRect(rcItem);
   }

   // Draw the color sample
   Rect rcSample(rcItem.left-rcItem.size().y, rcItem.top, rcItem.left, rcItem.bottom);
   dc.SetDCBrushColor(m_propColors.Get(msg->itemID));
   dc.FillRect(rcSample, (HBRUSH)GetStockObject(DC_BRUSH));
   return true;
}

struct ThemeEntry
{
   static inline Windows::Color s_divider;

   bool IsEditable() const { return &color!=&s_divider; }

   ConstString label;
   Windows::Color &color;
};

ThemeEntry g_theme_entries[]=
{
   { "Docked Window Captions", ThemeEntry::s_divider },
   { "Background", Docking::g_theme_colors.caption_background },
   { "Active Background", Docking::g_theme_colors.active_caption_background },
   { "Inactive Outline", Docking::g_theme_colors.inactive_caption_outline },
   { "Active Text", Docking::g_theme_colors.active_caption_text },
   { "Inactive Text", Docking::g_theme_colors.inactive_caption_text },

   { "Taskbar", ThemeEntry::s_divider },
   { "Background", g_taskbar_theme.background },
   { "Flash Color", g_taskbar_theme.flash },
   { "Activity Highlight", g_taskbar_theme.highlight },
   { "Text", g_taskbar_theme.text },
   { "Faint Text", g_taskbar_theme.text_faint },
   { "Disconnected Text", g_taskbar_theme.text_disconnected },
   { "Tab Divider", g_taskbar_theme.tab_divider },

   { "Controls (Spawn Tabs)", ThemeEntry::s_divider },
   { "3D Face", Windows::g_uitheme.color3DFace },
   { "3D Light", Windows::g_uitheme.color3DLight },
   { "3D Shadow", Windows::g_uitheme.color3DShadow },
   { "Active", Windows::g_uitheme.colorActive },
   { "Flash", Windows::g_uitheme.colorFlash },
   { "Flash Text", Windows::g_uitheme.colorFlashText },
   { "Text", Windows::g_uitheme.colorText },

   { "Misc", ThemeEntry::s_divider },
   { "Splitter", Windows::Controls::Wnd_Splitter::s_color },
};

struct Theme
{
   ConstString m_name;
   Windows::UITheme m_ui;
   Docking::ThemeColors m_docking;
   TaskbarTheme m_taskbar;
   Color m_splitter;
};

static constexpr Theme g_themes[]=
{
   {
      "Light",
      {
         .color3DFace{240,240,240},
         .color3DLight{255,255,255},
         .color3DShadow{160,160,160},
         .colorActive{153,180,209},
         .colorFlash{0,120,215},
         .colorFlashText{255, 255, 255},
         .colorText{0, 0, 0},
      },
      {
         .caption_background{240,240,240},
         .active_caption_background{153,180,209},
         .inactive_caption_outline{191,205,219},
         .active_caption_text{20, 20, 20},
         .inactive_caption_text{20, 20, 20},
      },
      {
         .background{245, 245, 245},
         .highlight{51, 183, 255},
         .flash{0x00, 0x70, 0xBB},
         .text{Colors::Black},
         .text_faint{Colors::Gray},
         .text_disconnected{Colors::Gray},
         .tab_divider{Colors::DkGray},
      },
      {205, 205, 205}, // Splitter
   },
   {
      "Dark",
      {
         .color3DFace{14,14,14},
         .color3DLight{0,0,0},
         .color3DShadow{94,94,94},
         .colorActive{68,80,93},
         .colorFlash{40,141,221},
         .colorFlashText{0, 0, 0},
         .colorText{255, 255, 255},
      },
      {
         .caption_background{14,14,14},
         .active_caption_background{68,80,93},
         .inactive_caption_outline{49,53,56},
         .active_caption_text{235,235,235},
         .inactive_caption_text{235,235,235},
      },
      {
         .background{31, 31, 31},
         .highlight{33, 122, 170},
         .flash{0x00, 0x70, 0xBB},
         .text{Colors::White},
         .text_faint{Colors::Gray},
         .text_disconnected{Colors::Gray},
         .tab_divider{Colors::LtGray},
      },
      {55, 55, 55}, // Splitter
   },
};

void ApplyCustomTheme()
{
   auto &theme=g_ppropGlobal->propCustomTheme();
   Windows::g_uitheme.color3DFace=theme.clrUI_3D_face();
   Windows::g_uitheme.color3DLight=theme.clrUI_3D_light();
   Windows::g_uitheme.color3DShadow=theme.clrUI_3D_shadow();
   Windows::g_uitheme.colorActive=theme.clrUI_active();
   Windows::g_uitheme.colorFlash=theme.clrUI_flash();
   Windows::g_uitheme.colorFlashText=theme.clrUI_flash_text();
   Windows::g_uitheme.colorText=theme.clrUI_text();

   Docking::g_theme_colors.caption_background=theme.clrDocking_caption_background();
   Docking::g_theme_colors.active_caption_background=theme.clrDocking_active_caption_background();
   Docking::g_theme_colors.inactive_caption_outline=theme.clrDocking_inactive_caption_outline();
   Docking::g_theme_colors.active_caption_text=theme.clrDocking_active_caption_text();
   Docking::g_theme_colors.inactive_caption_text=theme.clrDocking_inactive_caption_text();

   g_taskbar_theme.background=theme.clrTaskbar_background();
   g_taskbar_theme.highlight=theme.clrTaskbar_highlight();
   g_taskbar_theme.flash=theme.clrTaskbar_flash();
   g_taskbar_theme.text=theme.clrTaskbar_text();
   g_taskbar_theme.text_faint=theme.clrTaskbar_text_faint();
   g_taskbar_theme.text_disconnected=theme.clrTaskbar_text_disconnected();
   g_taskbar_theme.tab_divider=theme.clrTaskbar_tab_divider();

   Windows::Controls::Wnd_Splitter::s_color=theme.clrSplitter();
}

void SaveCustomTheme()
{
   auto &theme=g_ppropGlobal->propCustomTheme();

   theme.clrUI_3D_face(Windows::g_uitheme.color3DFace);
   theme.clrUI_3D_light(Windows::g_uitheme.color3DLight);
   theme.clrUI_3D_shadow(Windows::g_uitheme.color3DShadow);
   theme.clrUI_active(Windows::g_uitheme.colorActive);
   theme.clrUI_flash(Windows::g_uitheme.colorFlash);
   theme.clrUI_flash_text(Windows::g_uitheme.colorFlashText);
   theme.clrUI_text(Windows::g_uitheme.colorText);

   theme.clrDocking_caption_background(Docking::g_theme_colors.caption_background);
   theme.clrDocking_active_caption_background(Docking::g_theme_colors.active_caption_background);
   theme.clrDocking_inactive_caption_outline(Docking::g_theme_colors.inactive_caption_outline);
   theme.clrDocking_active_caption_text(Docking::g_theme_colors.active_caption_text);
   theme.clrDocking_inactive_caption_text(Docking::g_theme_colors.inactive_caption_text);

   theme.clrTaskbar_background(g_taskbar_theme.background);
   theme.clrTaskbar_highlight(g_taskbar_theme.highlight);
   theme.clrTaskbar_flash(g_taskbar_theme.flash);
   theme.clrTaskbar_text(g_taskbar_theme.text);
   theme.clrTaskbar_text_faint(g_taskbar_theme.text_faint);
   theme.clrTaskbar_text_disconnected(g_taskbar_theme.text_disconnected);
   theme.clrTaskbar_tab_divider(g_taskbar_theme.tab_divider);

   theme.clrSplitter(Windows::Controls::Wnd_Splitter::s_color);
}

void ApplyTheme()
{
   if(g_ppropGlobal->fPropCustomTheme())
      ApplyCustomTheme();
   else
   {
      unsigned index=g_ppropGlobal->Theme();
      if(index>=std::size(g_themes))
         index=0;

      auto &theme=g_themes[index];
      Windows::g_uitheme=theme.m_ui;
      Docking::g_theme_colors=theme.m_docking;
      g_taskbar_theme=theme.m_taskbar;
      Windows::Controls::Wnd_Splitter::s_color=theme.m_splitter;
   }
}

struct Dlg_UITheme : SettingsDialog
{
private:

   LRESULT WndProc(const Message &msg) override;

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::MeasureItem &msg);
   LRESULT On(const Msg::DrawItem &msg);

   void Save() override;
   void OnCreate() override;
   void UpdateEnabled();

   enum
   {
      IDC_CHANGE = 100,
      IDC_DEFAULT,
      IDC_APPLY_PRESET,
      IDC_CUSTOM,
      IDC_UIFONT,
      IDC_UIFONT_DEFAULT,
   };

   // Color List
   Controls::ListBox m_lbPresets;
   AL::CheckBox *m_pcbColors;
   AL::ListBox *m_plbColors;
   AL::Button *m_pbtChange, *m_pbtDefault;
};

LRESULT Dlg_UITheme::WndProc(const Message &msg)
{
   switch(msg.uMessage())
   {
      case Msg::Create::ID: return Call_On(Create);
      case Msg::Command::ID: return Call_On(Command);
      case Msg::MeasureItem::ID: return Call_On(MeasureItem);
      case Msg::DrawItem::ID: return Call_On(DrawItem);
   }

   return __super::WndProc(msg);
}

void Dlg_UITheme::Save()
{
   if(g_ppropGlobal->fPropCustomTheme())
      SaveCustomTheme();
}

void Dlg_UITheme::OnCreate()
{
   GetRoot() << AL::Style::Attach_Bottom;

   {
      auto &g=CreateSection("General", 0);

      AddBool(g, "Dark title bar/scrollbars (Requires app restart)", g_ppropGlobal->fDarkMode());

      {
         auto *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         *pGH << m_layout.CreateButton(IDC_UIFONT, "Font...") << m_layout.CreateButton(IDC_UIFONT_DEFAULT, "Use Default font");
      }
   }
   {
      auto &g=CreateSection("Presets", 0);

      m_lbPresets.Create(*this, -1, int2(16, 2), WS_VSCROLL);
      for(auto &theme : g_themes)
         m_lbPresets.AddString(theme.m_name);
      g << m_layout.AddControl(m_lbPresets);
      {
         AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         pGH->weight(0);
         pGH->MatchWidth(true);
         *pGH >> AL::Style::Attach_Right;

         *pGH << m_layout.CreateButton(IDC_APPLY_PRESET, "Apply");
      }
   }
   {
      auto &g=CreateSection("Custom Colors");
      m_pcbColors=m_layout.CreateCheckBox(IDC_CUSTOM, "Use Custom Colors");
      g << m_pcbColors;
      m_plbColors=m_layout.CreateListBox(-1, int2(16, 8), WS_VSCROLL|LBS_OWNERDRAWFIXED);
      g << m_plbColors;
      {
         AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         pGH->weight(0);
         pGH->MatchWidth(true);
         *pGH >> AL::Style::Attach_Right;

         m_pbtChange=m_layout.CreateButton(IDC_CHANGE, STR_Change);
         m_pbtDefault=m_layout.CreateButton(IDC_DEFAULT, STR_Default);

         *pGH << m_pbtChange << m_pbtDefault;
      }
   }

   // UI
   m_pcbColors->Check(g_ppropGlobal->fPropCustomTheme());
   m_lbPresets.SetCurSel(g_ppropGlobal->Theme());

   for(unsigned i=0;i<std::size(g_theme_entries);i++)
      m_plbColors->AddData(nullptr);

   UpdateEnabled();
}

void Dlg_UITheme::UpdateEnabled()
{
   EnableWindows(m_pcbColors->IsChecked(), *m_plbColors, *m_pbtChange, *m_pbtDefault);
}

LRESULT Dlg_UITheme::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_UIFONT_DEFAULT:
         SetUIFont(*this, "Calibri", 13);
         break;

      case IDC_UIFONT:
      {
         Prop::Font font;
         font.pclName(g_ppropGlobal->pclUIFontName());
         font.Size(g_ppropGlobal->UIFontSize());

         if(font.ChooseFont(*this))
            SetUIFont(*this, font.pclName(), font.Size());
         break;
      }

      case IDC_CUSTOM:
      {
         if(m_pcbColors->IsChecked())
            g_ppropGlobal->propCustomTheme(); // Create a custom theme (leaving current settings intact)
         else
         {
            g_ppropGlobal->ResetCustomTheme();
            ApplyTheme();
         }
         UpdateEnabled();
         Global_PropChange();
         break;
      }

      case IDC_APPLY_PRESET:
      {
         auto sel=m_lbPresets.GetCurSel();
         if(sel>=0 && size_t(sel)<std::size(g_themes))
         {
            g_ppropGlobal->Theme(sel);
            g_ppropGlobal->ResetCustomTheme();
            m_pcbColors->SetCheck(false);
            m_plbColors->Invalidate(true);
            ApplyTheme();
            UpdateEnabled();
            Global_PropChange();
         }
         break;
      }

      case IDC_CHANGE:
      {
         auto &entry=g_theme_entries[m_plbColors->GetCurSel()];
         if(!entry.IsEditable())
            break;

         Color &color=entry.color;
         if(ChooseColorSimple(*this, &color))
         {
            m_plbColors->Invalidate(true);
            Global_PropChange();
         }
         break;
      }

      case IDC_DEFAULT:
      {
         auto &entry=g_theme_entries[m_plbColors->GetCurSel()];
         if(!entry.IsEditable())
            break;

         // Save our current theme, then apply the original theme to get the default color
         SaveCustomTheme();
         Prop::CustomTheme theme=g_ppropGlobal->propCustomTheme();
         g_ppropGlobal->ResetCustomTheme();
         ApplyTheme();
         Color default_color=entry.color; // Copy out the defualt color

         // Now restore the custom theme back again
         g_ppropGlobal->propCustomTheme()=theme;
         ApplyTheme();
         entry.color=default_color;
         m_plbColors->Invalidate(true);
         Global_PropChange();
         break;
      }
   }

   return msg.Success();
}

LRESULT Dlg_UITheme::On(const Msg::MeasureItem &msg)
{
   msg->itemHeight=Controls::Control::m_tmButtons.tmHeight;
   msg->itemWidth=Controls::Control::m_tmButtons.tmAveCharWidth*12;
   return true;
}

LRESULT Dlg_UITheme::On(const Msg::DrawItem &msg)
{
   auto &entry=g_theme_entries[msg->itemID];
   bool is_header=&entry.color==&ThemeEntry::s_divider;

   // Draw the standard listbox text
   DC dc(msg->hDC);
   Rect &rcItem=(Rect &)(msg->rcItem);
   if(!is_header)
      rcItem.left+=rcItem.size().y*2;
   bool fSelected=msg->itemState&ODS_SELECTED;
   dc.FillRect(rcItem, GetSysColorBrush(fSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
   dc.SetTextColor(GetSysColor(fSelected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
   dc.SetBackgroundColor(GetSysColor(fSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
   if(msg->itemState&ODS_DISABLED)
      dc.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
   dc.TextOut(int2(rcItem.left+2, rcItem.bottom-Controls::Control::m_tmButtons.tmHeight), entry.label);

   if(msg->itemState&ODS_FOCUS)
   {
      dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
      dc.SetBackgroundColor(GetSysColor(COLOR_WINDOW));
      dc.DrawFocusRect(rcItem);
   }

   if(!is_header)
   {
      // Draw the color sample
      Rect rcSample(rcItem.left-rcItem.size().y, rcItem.top, rcItem.left, rcItem.bottom);
      dc.SetDCBrushColor(entry.color);
      dc.FillRect(rcSample, (HBRUSH)GetStockObject(DC_BRUSH));
   }
   return true;
}

struct Dlg_Network : SettingsDialog
{
private:

   enum
   {
      IDC_UIFONT=100,
      IDC_UIFONT_DEFAULT,
   };

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   void OnCreate() override;
   void Save() override;
   void UpdateEnabled();

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);

   AL::Edit *m_pedConnectTimeout, *m_pedConnectRetry;
   AL::CheckBox *m_pcbRetryForever;
};

LRESULT Dlg_Network::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Network::Save()
{
   // Connections
   Prop::Connections *ppropConnections=&g_ppropGlobal->propConnections();

   if(unsigned value;m_pedConnectTimeout->Get(value))
      ppropConnections->ConnectTimeout(value*1000);

   if(unsigned value;m_pedConnectRetry->Get(value))
      ppropConnections->ConnectRetry(value);

   ppropConnections->fRetryForever(m_pcbRetryForever->IsChecked());
}

void Dlg_Network::UpdateEnabled()
{
   EnableWindows(!m_pcbRetryForever->IsChecked(), *m_pedConnectRetry);
}

void Dlg_Network::OnCreate()
{
   {
      auto &g=CreateSection("Network Messages");
      AddBool(g, "Show connect/disconnect in spawn windows", g_ppropGlobal->fNetworkMessagesInSpawns());
   }
   {
      auto &g=CreateSection("Connections"); g >> AL::Style::Attach_Right;

      auto *pConnectTimeout=m_layout.CreateStatic(STR_ConnectTimeout);
      auto *pConnectRetry=m_layout.CreateStatic(STR_ConnectRetry);

      SetAllToMax(pConnectTimeout->szMinimum().x, pConnectRetry->szMinimum().x);

      {
         auto *pG1=m_layout.CreateGroup(Direction::Horizontal); g << pG1;

         m_pedConnectTimeout=m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
         *pG1 << pConnectTimeout << m_pedConnectTimeout << m_layout.CreateStatic(STR_Seconds);
      }
      {
         auto *pG1=m_layout.CreateGroup(Direction::Horizontal); g << pG1;

         m_pedConnectRetry=m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
         m_pcbRetryForever=m_layout.CreateCheckBox(-1, STR_RetryForever);

         *pG1 << pConnectRetry << m_pedConnectRetry << m_layout.CreateStatic(STR_Times)
               << m_pcbRetryForever;
      }
      AddBool(g, "Ignore errors when retrying", g_ppropGlobal->propConnections().fIgnoreErrors());
   }
   {
      auto &g=CreateSection("Advanced Settings");

      AddBool(g, STR_TCP_KeepAlive, g_ppropGlobal->fTCP_KeepAlive());
      AddBool(g, STR_TCP_NoDelay, g_ppropGlobal->fTCP_NoDelay());
   }

   // Connections
   const Prop::Connections *ppropConnections=&g_ppropGlobal->propConnections();

   m_pedConnectTimeout->LimitText(10);
   m_pedConnectTimeout->Set(ppropConnections->ConnectTimeout()/1000);

   m_pedConnectRetry->LimitText(10);
   m_pedConnectRetry->Set(ppropConnections->ConnectRetry());

   m_pcbRetryForever->Check(ppropConnections->fRetryForever());

   UpdateEnabled();
}

LRESULT Dlg_Network::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==BN_CLICKED && msg.wndCtl()==*m_pcbRetryForever)
      UpdateEnabled();

   return msg.Success();
}

struct Dlg_Taskbar : SettingsDialog
{
private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   void OnCreate() override;
   void Save() override;
   void UpdateEnabled();

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);

   AL::Radio *m_pcbActivityNotify_None, *m_pcbActivityNotify_Blink;
   AL::Radio *m_pcbActivityNotify_Solid;
   AL::CheckBox *m_pcbTaskbarBadge{};
};

LRESULT Dlg_Taskbar::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Taskbar::Save()
{
   // Activity Notify
   Prop::Connections::ActivityNotify an(Prop::Connections::Blink);

   if(m_pcbActivityNotify_None->IsChecked())  an=Prop::Connections::None; else
   if(m_pcbActivityNotify_Blink->IsChecked()) an=Prop::Connections::Blink; else
   if(m_pcbActivityNotify_Solid->IsChecked()) an=Prop::Connections::Solid;

   g_ppropGlobal->propConnections().eActivityNotify(an);
   if(m_pcbTaskbarBadge)
   {
      g_ppropGlobal->fTaskbarBadge(m_pcbTaskbarBadge->IsChecked());
      Wnd_MDI::RefreshBadgeCount();
   }
}

void Dlg_Taskbar::UpdateEnabled()
{
}

void Dlg_Taskbar::OnCreate()
{
   {
      auto &g=CreateSection("Tab Activity Display");

      auto *pGH=m_layout.CreateGroup(Direction::Horizontal); g << pGH;
      *pGH >> AL::Style::Attach_Right;

      m_pcbActivityNotify_None =m_layout.CreateRadio(-1, STR_None, true);
      m_pcbActivityNotify_Blink=m_layout.CreateRadio(-1, STR_Blink);
      m_pcbActivityNotify_Solid=m_layout.CreateRadio(-1, STR_Solid);

      *pGH << m_pcbActivityNotify_None << m_pcbActivityNotify_Blink << m_pcbActivityNotify_Solid;
   }

   {
      auto &g=CreateSection("Options");
      AddBool(g, "Taskbar at top", g_ppropGlobal->fTaskbarOnTop());
      AddBool(g, "Show Typed field", g_ppropGlobal->fTaskbarShowTyped());
      AddBool(g, "Show red dot on tab when logging", g_ppropGlobal->fTaskbarShowLogging());

      if(IsStoreApp())
         m_pcbTaskbarBadge=m_layout.CreateCheckBox(-1, "Show Taskbar Badge for activity");
      else
      {
         m_pcbTaskbarBadge=m_layout.CreateCheckBox(-1, "Show Taskbar Badge for activity (only works in store version)");
         m_pcbTaskbarBadge->Enable(false);
      }
      g << m_pcbTaskbarBadge;
   }

   // Activity Notify
   switch(g_ppropGlobal->propConnections().eActivityNotify())
   {
      case Prop::Connections::None:  m_pcbActivityNotify_None->Check(true);  break;
      case Prop::Connections::Blink: m_pcbActivityNotify_Blink->Check(true); break;
      case Prop::Connections::Solid: m_pcbActivityNotify_Solid->Check(true); break;
   }

   if(m_pcbTaskbarBadge)
      m_pcbTaskbarBadge->Check(g_ppropGlobal->fTaskbarBadge());

   UpdateEnabled();
}

LRESULT Dlg_Taskbar::On(const Msg::Command &msg)
{
   return msg.Success();
}


struct Dlg_Scripting : SettingsDialog
{
   void OnCreate() override;
   void Save() override;

private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);

   AL::ComboBox *m_pcoScriptLanguage;
   AL::Edit *m_pedScriptStartup;
   AL::Button *m_pbtScriptStartup;
   AL::CheckBox *m_pcbScriptDebug;
};

LRESULT Dlg_Scripting::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Scripting::Save()
{
   // Scripting
   if(m_pcoScriptLanguage->GetCount()!=1)
   {
      if(m_pcoScriptLanguage->GetCurSel()==0)
         g_ppropGlobal->pclScriptLanguage(ConstString());
      else
         g_ppropGlobal->pclScriptLanguage(m_pcoScriptLanguage->GetItemText(m_pcoScriptLanguage->GetCurSel()));
   }

   g_ppropGlobal->pclScriptStartup(m_pedScriptStartup->GetText());
   g_ppropGlobal->fScriptDebug(m_pcbScriptDebug->IsChecked());
}

void Dlg_Scripting::OnCreate()
{
   auto &g=CreateSection("Scripting");
   {
      auto *pG1=m_layout.CreateGroup(Direction::Horizontal); g << pG1;
      m_pcoScriptLanguage=m_layout.CreateComboBox(-1, int2(20, 8), CBS_DROPDOWNLIST|CBS_SORT|WS_VSCROLL);

      *pG1 << m_layout.CreateStatic(STR_Language) << m_pcoScriptLanguage;
   }
   {
      auto *pG1=m_layout.CreateGroup(Direction::Horizontal); g << pG1;
      m_pedScriptStartup=m_layout.CreateEdit(-1, int2(20, 1), ES_AUTOHSCROLL);
      m_pedScriptStartup->LimitText(Prop::Global::pclScriptStartup_MaxLength());
      m_pbtScriptStartup=m_layout.CreateButton(-1, "Choose...");
      *pG1 << m_layout.CreateStatic(STR_StartupScript) << m_pedScriptStartup << m_pbtScriptStartup;
   }
   {
      auto *pG1=m_layout.CreateGroup(Direction::Horizontal); g << pG1;
      m_pcbScriptDebug=m_layout.CreateCheckBox(-1, STR_DebuggingMode);

      *pG1 << m_pcbScriptDebug;
   }

   // Scripting
   if(!g_ppropGlobal->pclScriptLanguage())
      m_pcoScriptLanguage->AddString("(" STR_None ")");
   else
      m_pcoScriptLanguage->AddString(g_ppropGlobal->pclScriptLanguage());
   m_pcoScriptLanguage->SetCurSel(0);

   m_pedScriptStartup->SetText(g_ppropGlobal->pclScriptStartup());
   m_pcbScriptDebug->Check(g_ppropGlobal->fScriptDebug());
}

LRESULT Dlg_Scripting::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==BN_CLICKED && msg.wndCtl()==*m_pbtScriptStartup)
   {
      FixedStringBuilder<MAX_PATH> filename{m_pedScriptStartup->GetText()};
      File::ChooseFilename("Select startup script", "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0", filename);
      m_pedScriptStartup->SetText(filename);
      return msg.Success();
   }

   if(msg.uCodeNotify()==CBN_DROPDOWN && msg.wndCtl()==*m_pcoScriptLanguage &&
      m_pcoScriptLanguage->GetCount()==1)
   {
      m_pcoScriptLanguage->Reset();
      m_pcoScriptLanguage->AddString(STR_BuildingList);
      m_pcoScriptLanguage->SetCurSel(0);
      m_pcoScriptLanguage->AddString("(" STR_None ")");

      OM::DisplayScriptingEngines(*m_pcoScriptLanguage);
      m_pcoScriptLanguage->DeleteItem(0); // Remove the building list item

      int iLanguage=m_pcoScriptLanguage->FindStringExact(g_ppropGlobal->pclScriptLanguage());
      m_pcoScriptLanguage->SetCurSel(iLanguage!=-1 ? iLanguage : 0);

      return msg.Success();
   }

   return msg.Success();
}

struct Dlg_Logging : SettingsDialog
{
private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);

   void Save() override;
   void OnCreate() override;
   void Update();

   enum
   {
      IDC_CHECK_WRAP                   =100,
      IDC_CHECK_HANGINGINDENT          =101,
      IDC_CHECK_TIME                   =102,
      IDC_BUTTON_FILEDATEFORMATDEFAULT =103
   };

   AL::Edit *m_pedDefaultLogFile;

   AL::CheckBox *m_pcbLogSent, *m_pcbLogTyped;
   AL::Edit *m_pedSentPrefix, *m_pedTypedPrefix;

   AL::CheckBox *m_pcbTime, *m_pcbDate, *m_pcb24HR;
   AL::CheckBox *m_pcbWrap, *m_pcbHangingIndent, *m_pcbWrapNearestWord, *m_pcbDoubleSpace;
   AL::Edit *m_pedWrap, *m_pedHangingIndent;

   AL::Edit *m_pedFileDateFormat;
   AL::Button *m_pbtFileDateFormatDefault;

   Prop::Logging *m_ppropLogging{&g_ppropGlobal->propConnections().propLogging()};
};

LRESULT Dlg_Logging::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Logging::Save()
{
   m_ppropLogging->pclDefaultLogFileName(m_pedDefaultLogFile->GetText());

   // What gets logged
   m_ppropLogging->fLogSent(m_pcbLogSent->IsChecked());
   m_ppropLogging->pclSentPrefix(m_pedSentPrefix->GetText());
   m_ppropLogging->fLogTyped(m_pcbLogTyped->IsChecked());
   m_ppropLogging->pclTypedPrefix(m_pedTypedPrefix->GetText());

   // Date Format
   m_ppropLogging->pclFileDateFormat(m_pedFileDateFormat->GetText());

   // Time Format
   int iTimeFormat=0;

   if(m_pcbTime->IsChecked()) iTimeFormat|=Text::Time32::F_Time;
   if(m_pcbDate->IsChecked()) iTimeFormat|=Text::Time32::F_Date;
   if(m_pcb24HR->IsChecked()) iTimeFormat|=Text::Time32::F_24HR;

   m_ppropLogging->TimeFormat(iTimeFormat);

   unsigned value;

   // General
   m_ppropLogging->fWrap(m_pcbWrap->IsChecked());
   m_ppropLogging->fHangingIndent(m_pcbHangingIndent->IsChecked());

   if(m_pedWrap->Get(value))
      m_ppropLogging->Wrap(value);

   if(m_pedHangingIndent->Get(value))
      m_ppropLogging->HangingIndent(value);

   m_ppropLogging->fWrapNearestWord(m_pcbWrapNearestWord->IsChecked());
   m_ppropLogging->fDoubleSpace(m_pcbDoubleSpace->IsChecked());
}

void Dlg_Logging::Update()
{
   bool fEnable=m_pcbWrap->IsChecked();

   m_pedWrap->Enable(fEnable);
   m_pcbHangingIndent->Enable(fEnable);
   m_pcbWrapNearestWord->Enable(fEnable);
   m_pcbDoubleSpace->Enable(fEnable);

   fEnable &= m_pcbHangingIndent->IsChecked();
   m_pedHangingIndent->Enable(fEnable);

   m_pcb24HR->Enable(m_pcbTime->IsChecked());
}

void Dlg_Logging::OnCreate()
{
   {
      auto &g=CreateSection("Default Filenames");
      {
         auto *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         m_pedDefaultLogFile=m_layout.CreateEdit(-1, int2(40, 1), ES_AUTOHSCROLL);
         m_pedDefaultLogFile->SetCueBanner("%userprofile%\\Logs\\%server%\\%character%\\%Date%.txt");
         m_pedDefaultLogFile->LimitText(Prop::Logging::pclDefaultLogFileName_MaxLength());
         m_pedDefaultLogFile->SetText(m_ppropLogging->pclDefaultLogFileName());

         *pGH << m_layout.CreateStatic("Per Character:") << m_pedDefaultLogFile;
      }
   }
   {
      auto &g=CreateSection(STR_LogYourInput);
      m_pcbLogSent=m_layout.CreateCheckBox(-1, STR_Sent);
      m_pedSentPrefix=m_layout.CreateEdit(-1, int2(40, 1), ES_AUTOHSCROLL);

      m_pcbLogSent->Check(m_ppropLogging->fLogSent());
      m_pedSentPrefix->SetText(m_ppropLogging->pclSentPrefix());

      m_pcbLogTyped=m_layout.CreateCheckBox(-1, STR_Typed);
      m_pedTypedPrefix=m_layout.CreateEdit(-1, int2(40, 1), ES_AUTOHSCROLL);

      m_pcbLogTyped->Check(m_ppropLogging->fLogTyped());
      m_pedTypedPrefix->SetText(m_ppropLogging->pclTypedPrefix());

      SetAllToMax(m_pcbLogSent->szMinimum().x, m_pcbLogTyped->szMinimum().x);

      {
         AL::Group *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         *pGH << m_pcbLogSent << m_pedSentPrefix;
      }
      {
         AL::Group *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         *pGH << m_pcbLogTyped << m_pedTypedPrefix;
      }
   }
   {
      auto &g=CreateSection(STR_FileDateFormat);
      m_pedFileDateFormat=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
      m_pedFileDateFormat->SetText(m_ppropLogging->pclFileDateFormat());

      m_pbtFileDateFormatDefault=m_layout.CreateButton(IDC_BUTTON_FILEDATEFORMATDEFAULT, STR_Default);

      {
         AL::Group *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         *pGH << m_layout.CreateStatic(STR_FormatString) << m_pedFileDateFormat << m_pbtFileDateFormatDefault;
      }
      g << m_layout.CreateStatic("Note: You can also use %date% to insert this in log filenames\n"
                                    "You can also use any OS environment variables plus:\n"
                                    "%server% - Current server name, %character% - Current character name");
   }
   {
      auto &g=CreateSection(STR_DateAndTimePerLine);
      AL::Group *pG=m_layout.CreateGroup_Horizontal(); g << pG;

      m_pcbTime=m_layout.CreateCheckBox(IDC_CHECK_TIME, STR_Time);
      m_pcbDate=m_layout.CreateCheckBox(-1, STR_Date);
      m_pcb24HR=m_layout.CreateCheckBox(-1, STR_24Hour);

      int iTimeFormat=m_ppropLogging->TimeFormat();

      if(iTimeFormat&Text::Time32::F_Time) m_pcbTime->Check(true);
      if(iTimeFormat&Text::Time32::F_Date) m_pcbDate->Check(true);
      if(iTimeFormat&Text::Time32::F_24HR) m_pcb24HR->Check(true);

      *pG << m_pcbTime << m_pcb24HR << m_pcbDate;
   }
   {
      auto &g=CreateSection(STR_Wrapping);
      {
         auto *pGH=m_layout.CreateGroup_Horizontal(); g << pGH; *pGH >> AL::Style::Attach_Right;

         m_pcbWrap=m_layout.CreateCheckBox(IDC_CHECK_WRAP, STR_WrapText);
         m_pcbWrap->Check(m_ppropLogging->fWrap());

         m_pedWrap=m_layout.CreateEdit(-1, int2(4, 1), ES_NUMBER);
         m_pedWrap->Set(m_ppropLogging->Wrap());

         *pGH << m_pcbWrap << m_pedWrap << m_layout.CreateStatic(STR_TextCharacters);
      }
      {
         auto *pGH=m_layout.CreateGroup_Horizontal(); g << pGH; *pGH >> AL::Style::Attach_Right;
         m_pcbHangingIndent=m_layout.CreateCheckBox(IDC_CHECK_HANGINGINDENT, STR_HangingIndent);
         m_pcbHangingIndent->Check(m_ppropLogging->fHangingIndent());

         m_pedHangingIndent=m_layout.CreateEdit(-1, int2(4, 1), ES_NUMBER);
         m_pedHangingIndent->Set(m_ppropLogging->HangingIndent());

         *pGH << m_pcbHangingIndent << m_pedHangingIndent << m_layout.CreateStatic(STR_TextCharacters);
      }

      m_pcbWrapNearestWord=m_layout.CreateCheckBox(-1, STR_WrapNearestWord);
      m_pcbWrapNearestWord->Check(m_ppropLogging->fWrapNearestWord());
      g << m_pcbWrapNearestWord;

      m_pcbDoubleSpace=m_layout.CreateCheckBox(-1, "Double space lines");
      m_pcbDoubleSpace->Check(m_ppropLogging->fDoubleSpace());
      g << m_pcbDoubleSpace;
   }

   Update();
}

LRESULT Dlg_Logging::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_BUTTON_FILEDATEFORMATDEFAULT:
         m_pedFileDateFormat->SetText(STR_FileDateFormatString);
         break;

      case IDC_CHECK_WRAP:
      case IDC_CHECK_HANGINGINDENT:
      case IDC_CHECK_TIME:
         if(msg.uCodeNotify()==BN_CLICKED)
            Update();
         break;
   }

   return msg.Success();
}

struct Dlg_RestoreLogs : SettingsDialog
{
private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);

   void Save() override;
   void OnCreate() override;
   void Update();

   AL::Edit *m_pedRestoreBufferSize;

   Prop::Logging *m_ppropLogging{&g_ppropGlobal->propConnections().propLogging()};
};

LRESULT Dlg_RestoreLogs::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_RestoreLogs::Save()
{
   if(unsigned value;m_pedRestoreBufferSize->Get(value))
      m_ppropLogging->RestoreBufferSize(value);

   gp_restore_logs=nullptr;
   gp_restore_logs=RestoreLogs::Create();
}

void Dlg_RestoreLogs::Update()
{
}

void Dlg_RestoreLogs::OnCreate()
{
   auto &g=CreateSection("Restore Logs ");
   g << m_layout.CreateStatic("The restore log is what refills the windows with the contents they\nheld when the app was last closed.\nThe data is stored in the file Restore.dat\n");

   {
      AddBool(g, "Enabled", m_ppropLogging->fRestoreLogs());

      {
         AL::Group *pGH=m_layout.CreateGroup_Horizontal(); g << pGH;
         *pGH >> AL::Style::Attach_Right;

         m_pedRestoreBufferSize=m_layout.CreateEdit(-1, int2(8, 1), ES_NUMBER);
         m_pedRestoreBufferSize->Set(m_ppropLogging->RestoreBufferSize());

         *pGH << m_layout.CreateStatic("Per character buffer Size in KB:") << m_pedRestoreBufferSize << m_layout.CreateStatic("(rounds up to nearest 64KB)");
      }

      FixedStringBuilder<256> currentSize("Currently using ", gp_restore_logs->Count(), " buffers for a file size of ");
      ByteCountToStringAbbreviated(currentSize, gp_restore_logs->Count()*gp_restore_logs->GetBufferSize());
      g << m_layout.CreateStatic(currentSize);
   }

   Update();
}

LRESULT Dlg_RestoreLogs::On(const Msg::Command &msg)
{
   return msg.Success();
}

struct Dlg_Emoji : SettingsDialog
{
private:

   void OnCreate() override;
   void Update();
   void Save() override;

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::CtlColor &msg);

   enum
   {
      IDC_MAKE_COPY = 100,
      IDC_REVERT,
      IDC_EDIT,
      IDC_RELOAD,
      IDC_STATIC_FONT,
   };

   File::Path m_pathOriginal{GetResourcePath(), "Emoji.txt"};
   File::Path m_pathEditable{GetConfigPath(), "Emoji.txt"};
   bool m_fHasEditable{GetFileAttributes(m_pathEditable)!=INVALID_FILE_ATTRIBUTES};
   bool m_available{m_fHasEditable || GetFileAttributes(m_pathOriginal)!=INVALID_FILE_ATTRIBUTES};

   AL::CheckBox *m_pcbEnabled;
   AL::Static *m_pInfo;
   AL::Static m_stLink;
   
   AL::Button *m_pbtMakeCopy, *m_pbtRevert, *m_pbtReload, *m_pbtEdit;
};

LRESULT Dlg_Emoji::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command, Msg::CtlColorStatic>(msg);
}

void Dlg_Emoji::Save()
{
}

void Dlg_Emoji::Update()
{
   EnableWindows(m_fHasEditable, *m_pbtEdit, *m_pbtReload, *m_pbtRevert);
   EnableWindows(m_available, *m_pbtMakeCopy, *m_pcbEnabled);

   m_pInfo->SetText(m_fHasEditable ? ConstString("Using custom Emoji.txt") : ConstString("Using default Emoji.txt"));
}

void Dlg_Emoji::OnCreate()
{
   m_pInfo=m_layout.CreateStatic("(Placeholder)");
   m_pbtMakeCopy=m_layout.CreateButton(IDC_MAKE_COPY, "Make an editable copy in config location");
   m_pbtEdit=m_layout.CreateButton(IDC_EDIT, "Edit editable copy in notepad");
   m_pbtReload=m_layout.CreateButton(IDC_RELOAD, "Reload Emoji.txt");
   m_pbtRevert=m_layout.CreateButton(IDC_REVERT, "Delete editable copy and use defaults");

   auto &g=CreateSection("Auto Emoji");

   g << m_layout.CreateStatic(
      "This will turn smileys into their corresponding emojis\n"
      "and add emojis after any words. You can also customize it.\n"
      "For example:\n"
      "  :) → 🙂\n"
      "  dog → dog 🐕\n"
   );

   if(!m_available)
      g << m_layout.CreateStatic("NOTE: No emoji translation files present, requires Assets\\Emoji.txt\nor a copy in the same location as Config.txt to function\n");

   m_pcbEnabled=AddBool(g, "Enabled", g_ppropGlobal->fParseEmoticons());
   g.Add(m_pInfo, m_pbtMakeCopy, m_pbtRevert, m_pbtReload, m_pbtEdit);
   Update();
}

LRESULT Dlg_Emoji::On(const Msg::CtlColor &msg)
{
   if(msg.iType()==CTLCOLOR_STATIC && msg.wndChild()==m_stLink)
   {
      msg.dc().SetTextColor(Colors::Blue);
      msg.dc().SetBackgroundMode(TRANSPARENT);
      return msg.Success(GetSysColorBrush(COLOR_BTNFACE));
   }

   return __super::WndProc(msg);
}

LRESULT Dlg_Emoji::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_STATIC_FONT:
         if(msg.uCodeNotify()==STN_CLICKED)
            OpenURLAsync("http://www.beipmu.com/seguiemj.zip");
         break;

      case IDC_MAKE_COPY:
      {
         if(CopyFile(m_pathOriginal, m_pathEditable, false))
         {
            m_fHasEditable=true;
            Update();
         }
         break;
      }

      case IDC_REVERT:
      {
         if(DeleteFile(m_pathEditable))
         {
            m_fHasEditable=false;
            Update();
            Emoji::Reset();
         }
         break;
      }

      case IDC_EDIT:
      {
         ProcessInformation pi;
         CreateProcess(HybridStringBuilder<>("notepad.exe ", m_pathEditable), pi);
         break;
      }

      case IDC_RELOAD: Emoji::Reset(); break;
   }

   return msg.Success();
}


struct Dlg_UserData : SettingsDialog
{
private:

   void OnCreate() override;
   void Update();
   void Save() override;

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   using SettingsDialog::On;
   LRESULT On(const Msg::Command &msg);

   enum
   {
      IDC_EXPORT = 100,
      IDC_IMPORT,
      IDC_SAVE,
      IDC_LOAD_BACKUP,
   };

   AL::Button *mp_export, *mp_import, *mp_save, *mp_load_backup;
};

LRESULT Dlg_UserData::WndProc(const Message &msg)
{
   return Dispatch<SettingsDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_UserData::Save()
{
}

void Dlg_UserData::OnCreate()
{
   mp_export=m_layout.CreateButton(IDC_EXPORT, "Export");
   mp_import=m_layout.CreateButton(IDC_IMPORT, "Import");
   mp_save=m_layout.CreateButton(IDC_SAVE, "Save Right Now");
   mp_load_backup=m_layout.CreateButton(IDC_LOAD_BACKUP, "Load");

   {
      auto &g=CreateSection("User Data");
      g << m_layout.CreateStatic(
         "Your data is automatically saved on exit and after modifying\n"
         "triggers/worlds. This is to make custom backups and restore them.\n");

      g << mp_export << mp_import << mp_save;
   }
   {
      auto &g=CreateSection("Automatic Backup");

      g << m_layout.CreateStatic(
         "This loads the backup made automatically every\n"
         "time BeipMU upgrades to a new version\n");

      {
         FixedStringBuilder<256> string("Backup is from ");
         {
            File::Read_Only file(File::Path(GetConfigPath(), "Config.bak"));
            if(!file)
            {
               string("No backup");
               mp_load_backup->Enable(false);
            }
            else
            {
               auto time=Time::File(Time::System().GetFileTime()-file.GetLastWrite()).ToSeconds();
               Time::SecondsToStringAbbreviated(string, time);
               string(" ago (");
               Time::Time(file.GetLastWrite()).FormatDate(string);
               string(")");
            }
         }
         g << m_layout.CreateStatic(string);
      }

      g << mp_load_backup;
   }
}

void ImportConfig(ConstString filename);
void SaveWindowInformationInConfig();

LRESULT Dlg_UserData::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_SAVE:
      {
         if(!SaveConfig(*this))
            return msg.Success();
         if(IsStoreApp())
            MessageBox(*this, "Configuration Saved", "Note:", MB_ICONINFORMATION|MB_OK);
         else
         {
            FixedStringBuilder<1024> path("Configuration saved to ", GetConfigPath(), "Config.txt");
            MessageBox(*this, path, "Note:", MB_ICONINFORMATION|MB_OK);
         }
         return msg.Success();
      }

      case IDC_EXPORT:
      {
         File::Chooser cf;
         cf.SetTitle("Export Configuration");
         cf.SetFilter("*.txt\0\0", 0);

         File::Path filename{"BeipMU Config "};
         Time::Local().FormatDate(filename, "yyyy'-'MM'-'dd");
         filename(".txt");
            
         if(cf.Choose(*this, filename, true))
         {
            if(filename.Exists() && MessageBox(*this, "File already exists, overwrite?", "Warning", MB_ICONWARNING|MB_YESNO)!=IDYES)
               return msg.Success();

            SaveWindowInformationInConfig();
            ConfigExport(filename, g_ppropGlobal, g_ppropGlobal->fShowDefaults(), true);
         }

         return msg.Success();
      }

      case IDC_LOAD_BACKUP:
      {
         File::Path filename(GetConfigPath(), "Config.bak");
         if(!filename.Exists())
         {
            MessageBox(*this, "No backup config exists", "Note:", MB_ICONERROR|MB_OK);
            return msg.Success();
         }

         if(MessageBox(*this, "This will replace all current settings and restart BeipMU.\rAre you sure?", "Warning", MB_ICONQUESTION|MB_YESNO)==IDNO)
            return msg.Success();

         ImportConfig(filename);
         return msg.Success();
      }

      case IDC_IMPORT:
      {
         if(MessageBox(*this, "This will replace all current settings and restart BeipMU.\rAre you sure?", "Warning", MB_ICONWARNING|MB_YESNO)!=IDYES)
            return msg.Success();

         File::Chooser cf;
         cf.SetTitle("Import Configuration");
         cf.SetFilter("*.txt\0\0", 0);

         FixedStringBuilder<256> filename;

         if(cf.Choose(*this, filename, false))
            ImportConfig(filename);

         return msg.Success();
      }
   }

   return msg.Success();
}

struct Dlg_Settings : Wnd_Dialog, Singleton<Dlg_Settings>
{
   Dlg_Settings(Window wndParent);

private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   void Save();
   void UpdateEnabled();

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   AL::ListBox *mp_list_categories;
   AL::Stack *mp_stack;
   Collection<SettingsDialog*> m_dialogs;

   void AddCategory(SettingsDialog *p, ConstString label);
};

Dlg_Settings::Dlg_Settings(Window wndParent)
{
   Create("Settings", WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_VISIBLE, 0 /*dwExStyle*/, wndParent);
}

LRESULT Dlg_Settings::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Settings::AddCategory(SettingsDialog *p, ConstString label)
{
   p->Create(*this);
   *mp_stack << m_layout.AddObjectWindow(*p, *p);
   mp_list_categories->AddString(label);
   m_dialogs.Push(p);
}

LRESULT Dlg_Settings::On(const Msg::Create &msg)
{
   auto *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);
   {
      auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;

      mp_list_categories=m_layout.CreateListBox(-1, int2(12, 10));
      mp_list_categories->weight(0);
      *pGH << mp_list_categories;

      auto *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

      mp_stack=m_layout.CreateStack();
      *pGV << mp_stack;
      {
         auto *pG=m_layout.CreateGroup_Horizontal(); *pGV << pG;
         *pG >> (AL::Style::Attach_Left | AL::Style::Attach_Top);

         auto *pbtOk=m_layout.CreateButton(IDOK, STR_OK); pbtOk->SizeBigger();
         auto *pbtCancel=m_layout.CreateButton(IDCANCEL, STR_Cancel); pbtCancel->SizeBigger();

         SetAllToMax(pbtOk->szMinimum().x, pbtCancel->szMinimum().x);
         pG->Add(pbtOk, pbtCancel);
      }
   }

   AddCategory(new Dlg_General(), "General");
   AddCategory(new Dlg_UITheme(), "UI Theme");
   AddCategory(new Dlg_Input(), "Input Windows");
   AddCategory(new Dlg_Logging(), "Logging");
   AddCategory(new Dlg_Network(), "Network");
   AddCategory(new Dlg_Taskbar(), "Taskbar");
   AddCategory(new Dlg_KeyboardShortcuts(), "Keyboard Shortcuts");
   AddCategory(new Dlg_Emoji(), "Auto Emoji");
   AddCategory(new Dlg_Scripting(), "Scripting");
   AddCategory(new Dlg_RestoreLogs(), "Restore Logs");
   AddCategory(new Dlg_AnsiColors(), "Ansi Colors");
   AddCategory(new Dlg_UserData(), "User Data");
   mp_list_categories->SetCurSel(0);

   return msg.Success();
}

LRESULT Dlg_Settings::On(const Msg::Command &msg)
{
   if(msg.wndCtl()==*mp_list_categories)
   {
      if(msg.uCodeNotify()==LBN_SELCHANGE)
      {
         int sel=mp_list_categories->GetCurSel();
         mp_stack->SetVisible(sel);
      }

      return msg.Success();
   }

   switch(msg.iID())
   {
      case IDOK:
         for(auto &d : m_dialogs)
         {
            d->Save();
            d->SaveSettings();
         }
         Global_PropChange();
         __fallthrough;
      case IDCANCEL:
         Close();
         break;
   }

   return msg.Success();
}

void CreateDialog_Settings(Window wndParent)
{
   if(Dlg_Settings::HasInstance())
      Dlg_Settings::GetInstance().SetFocus();
   else
      new Dlg_Settings(wndParent);
}
