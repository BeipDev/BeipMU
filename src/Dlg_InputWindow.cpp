#include "Main.h"
#include "Wnd_Input.h"

static CntPtrTo<Prop::InputWindow> g_propInputWindowClipboard=MakeCounting<Prop::InputWindow>();

struct Dlg_InputWindow : Wnd_Dialog
{
   Dlg_InputWindow(Window wndParent, InputControl &input_window, Prop::InputWindow &propTextWindow);

private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Paint &msg);

   void Update();
   void Load(const Prop::InputWindow &prop);
   void Save(Prop::InputWindow &prop);

   enum
   {
      IDC_FOREGROUND = 100,
      IDC_BACKGROUND,
      IDC_FONT,
      IDC_BROWSE,
      IDC_CHECK_TIME,
      IDC_CHECK_FIXEDWIDTH,
      IDC_CHECK_AUTOSIZEVERTICALLY,
      IDC_CHECK_LOCALECHO,
      IDC_LOCALECHO_COLOR,
      IDC_COPY_SETTINGS,
      IDC_PASTE_SETTINGS,
   };

   AL::Static *m_pstSample, *m_pstFontName;

   AL::CheckBox *m_pcbAutoSizeVertically;
   AL::Edit *m_pedMinimumHeight, *m_pedMaximumHeight;

   AL::Edit *m_pedPrefix;
   AL::Edit *m_pedTitle;

   AL::Edit *m_ped_margins[4];

   AL::CheckBox *m_pcbLocalEcho;
   AL::Button *m_pbtLocalEchoColor;

   AL::CheckBox *m_pcbSticky;

   AL::Button *mp_button_copy_settings, *mp_button_paste_settings;

   InputControl &m_input_window;
   CntPtrTo<Prop::InputWindow> m_ppropInputWindow;
   Prop::Font m_propFont;
   Color m_clrFG, m_clrBG; // Local copies (in case of cancel!)
   Color m_clrLocalEcho;

   Handle<HFONT> m_hfText;
};

LRESULT Dlg_InputWindow::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command, Msg::Paint>(msg);
}

Dlg_InputWindow::Dlg_InputWindow(Window wndParent, InputControl &input_window, Prop::InputWindow &propInputWindow)
 : m_input_window{input_window},
   m_ppropInputWindow{&propInputWindow}
{
   FixedStringBuilder<256> title;
   if(&propInputWindow==&GlobalInputSettings())
      title("Global ");
   title(STR_Title_InputWindow);

   Create(title, WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_VISIBLE | Wnd_Dialog::Style_Modal, 0 /*dwExStyle*/, wndParent);
}

void Dlg_InputWindow::Update()
{
   // Invalidate the Sample
   m_hfText=m_propFont.CreateFont();
   Invalidate(ScreenToClient(m_pstSample->WindowRect()), false);

   FixedStringBuilder<256> sBuffer(" " STR_Font ": ", m_propFont.pclName(), "  " STR_Size ": ", m_propFont.SizeInPoints());

   m_pstFontName->SetText(sBuffer);

   if(m_input_window.IsPrimary())
   {
      m_pedMinimumHeight->Enable(m_pcbAutoSizeVertically->IsChecked());
      m_pedMaximumHeight->Enable(m_pcbAutoSizeVertically->IsChecked());
   }
}

void Dlg_InputWindow::Load(const Prop::InputWindow &prop)
{
   m_propFont=prop.propFont();
   m_clrFG=prop.clrFore();
   m_clrBG=prop.clrBack();

   for(unsigned i=0;i<4;i++)
      m_ped_margins[i]->Set(prop.rcMargins().m[i]);

   // Options
   if(m_input_window.IsPrimary())
   {
      m_pcbAutoSizeVertically->Check(prop.fAutoSizeVertically());
      m_pedMinimumHeight->Set(prop.MinimumHeight());
      m_pedMaximumHeight->Set(prop.MaximumHeight());
   }
   else
   {
      if(m_ppropInputWindow!=&GlobalInputSettings())
      {
         m_pedPrefix->SetText(prop.pclPrefix());
         m_pedTitle->SetText(prop.pclTitle());
      }
      else
      {
         m_pedPrefix->Enable(false);
         m_pedPrefix->SetText("Disabled due to 'Use Global Settings'");
         m_pedTitle->Enable(false);
         m_pedTitle->SetText("Disabled due to 'Use Global Settings'");
      }
   }

   m_pcbLocalEcho->Check(prop.fLocalEcho());
   m_clrLocalEcho=prop.clrLocalEchoColor();
   m_pcbSticky->Check(prop.fSticky());

   Update();
}

void Dlg_InputWindow::Save(Prop::InputWindow &prop)
{
   // Text
   prop.propFont()=m_propFont;
   prop.clrFore(m_clrFG);
   prop.clrBack(m_clrBG);

   if(m_input_window.IsPrimary())
   {
      prop.fAutoSizeVertically(m_pcbAutoSizeVertically->IsChecked());

      unsigned value;
      if(m_pedMinimumHeight->Get(value))
         prop.MinimumHeight(value);
      if(m_pedMaximumHeight->Get(value))
         prop.MaximumHeight(value);

      // Ensure minimum height is at least 1
      if(prop.MinimumHeight()<1)
         prop.MinimumHeight(1);
      // Ensure maximum height is at least minimum height
      if(prop.MaximumHeight()<prop.MinimumHeight())
         prop.MaximumHeight(prop.MinimumHeight());
   }
   else
   {
      if(m_ppropInputWindow!=&GlobalInputSettings())
      {
         prop.pclPrefix(m_pedPrefix->GetText());
         prop.pclTitle(m_pedTitle->GetText());
      }
   }

   prop.clrLocalEchoColor(m_clrLocalEcho);
   prop.fLocalEcho(m_pcbLocalEcho->IsChecked());
   prop.fSticky(m_pcbSticky->IsChecked());

   for(unsigned i=0;i<4;i++)
   {
      auto rect=prop.rcMargins();
      if(unsigned value;m_ped_margins[i]->Get(value))
         rect.m[i]=value;

      prop.rcMargins(rect);
   }
}

//
// Window Messages
//

LRESULT Dlg_InputWindow::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   {
      AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Text); *pGV << pGB;
      AL::Group *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);
      auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->MatchBaseline(false);
      {
         AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Sample); *pGH << pGB;
         m_pstSample=m_layout.CreateStatic(" ", SS_BLACKFRAME | SS_SUNKEN); pGB->SetChild(m_pstSample);
         m_pstSample->szMinimum().x+=Controls::Button::m_tmButtons.tmAveCharWidth*20;
         m_pstSample->szMinimum().y+=Controls::Button::m_tmButtons.tmHeight*3;

         *pGH << m_layout.CreateButton(IDC_FONT, STR_Font___);
         AL::Group *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
         {
            *pGV << m_layout.CreateButton(IDC_FOREGROUND, STR_Foreground___);
            *pGV << m_layout.CreateButton(IDC_BACKGROUND, STR_Background___);
         }
      }

      m_pstFontName=m_layout.CreateStatic(" ", SS_SUNKEN); *pGV << m_pstFontName;
   }

   {
      AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Options); *pGV << pGB;
      AL::Group *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);

      if(m_input_window.IsPrimary())
      {
         m_pcbAutoSizeVertically=m_layout.CreateCheckBox(IDC_CHECK_AUTOSIZEVERTICALLY, "Resize to fit contents");
         *pGV << m_pcbAutoSizeVertically;

         {
            AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;

            m_pedMinimumHeight=m_layout.CreateEdit(0, int2(4, 1), ES_NUMBER);
            m_pedMinimumHeight->LimitText(4);
            m_pedMaximumHeight=m_layout.CreateEdit(0, int2(4, 1), ES_NUMBER);
            m_pedMaximumHeight->LimitText(4);

            *pGH << m_layout.CreateStatic("Height in lines, Minimum:");
            *pGH << m_pedMinimumHeight;
            *pGH << m_layout.CreateStatic("Maximum:");
            *pGH << m_pedMaximumHeight;
         }
      }
      else
      {
         {
            m_pedPrefix=m_layout.CreateEdit(-1, int2(20, 1), ES_AUTOHSCROLL);

            auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            *pGH << m_layout.CreateStatic("Send Prefix: ") << m_pedPrefix;
         }
         {
            m_pedTitle=m_layout.CreateEdit(-1, int2(20, 1), ES_AUTOHSCROLL);

            auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            *pGH << m_layout.CreateStatic("Title: ") << m_pedTitle;
         }
      }

      {
         auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;

         for(auto &p_margin : m_ped_margins)
            p_margin=m_layout.CreateEdit(0, int2(2, 1), ES_NUMBER);

         *pGH << m_layout.CreateStatic("Margins - Left:") << m_ped_margins[0] << 
                 m_layout.CreateStatic(" Top:") << m_ped_margins[1] << 
                 m_layout.CreateStatic(" Right:") << m_ped_margins[2] << 
                 m_layout.CreateStatic(" Bottom:") << m_ped_margins[3];
      }

      m_pcbSticky=m_layout.CreateCheckBox(-1, "Don't clear input on enter");
      *pGV << m_pcbSticky;

      {
         auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
         *pGH >> AL::Style::Attach_Right;
         m_pcbLocalEcho=m_layout.CreateCheckBox(IDC_CHECK_LOCALECHO, "Local Echo");
         m_pbtLocalEchoColor=m_layout.CreateButton(IDC_LOCALECHO_COLOR, "Color...");

         *pGH << m_pcbLocalEcho << m_pbtLocalEchoColor;
      }
   }

   {
      AL::GroupBox *pGB=m_layout.CreateGroupBox("Settings"); *pGV << pGB;

      AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);

      {
         AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
         pGH->MatchWidth(true);

         mp_button_copy_settings=m_layout.CreateButton(IDC_COPY_SETTINGS, "Copy");
         mp_button_paste_settings=m_layout.CreateButton(IDC_PASTE_SETTINGS, "Paste");

         *pGH << mp_button_copy_settings << mp_button_paste_settings;
      }
   }

   {
      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      *pGH >> AL::Style::Attach_Left >> AL::Style::Attach_Top;
      pGH->MatchWidth(true);

      AL::Button *pOK=m_layout.CreateButton(IDOK, STR_OK); pOK->SizeBigger();
      AL::Button *pCancel=m_layout.CreateButton(IDCANCEL, STR_Cancel); pCancel->SizeBigger();

      *pGH << pOK << pCancel;
   }

   Load(*m_ppropInputWindow);
   return msg.Success();
}

LRESULT Dlg_InputWindow::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDOK:
         Save(*m_ppropInputWindow);
         OnGlobalInputSettingsModified(*m_ppropInputWindow);
      case IDCANCEL:
         Close();
         break;

      case IDC_COPY_SETTINGS: Save(*g_propInputWindowClipboard); break;
      case IDC_PASTE_SETTINGS: Load(*g_propInputWindowClipboard); break;

      case IDC_CHECK_AUTOSIZEVERTICALLY:
         if(msg.uCodeNotify()==BN_CLICKED)
            Update();
         break;

      case IDC_FONT:
         if(m_propFont.ChooseFont(*this))
         {
            m_hfText=m_propFont.CreateFont();
            Update();
         }
         break;

      case IDC_FOREGROUND:
         if(ChooseColorSimple(*this, &m_clrFG))
            Update();
         break;

      case IDC_BACKGROUND:
         if(ChooseColorSimple(*this, &m_clrBG))
            Update();
         break;

      case IDC_LOCALECHO_COLOR:
         ChooseColorSimple(*this, &m_clrLocalEcho);
         break;

   }

   return msg.Success();
}

LRESULT Dlg_InputWindow::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);

   Rect rcSample=ScreenToClient(m_pstSample->WindowRect());

   Handle<HBRUSH> hbBG(CreateSolidBrush(m_clrBG));
   ps.FillRect(rcSample, hbBG);

   ps.SetTextAlign(TA_CENTER);
   ps.SetTextColor(m_clrFG);
   ps.SetBackgroundMode(TRANSPARENT);

   DC::FontSelector _(ps, m_hfText);
   ps.ExtTextOut(int2(rcSample.left+rcSample.size().x/2, rcSample.top+(rcSample.size().y-m_propFont.Size())/2),
                 ETO_CLIPPED, rcSample, STR_AaBbCc, nullptr);

   return msg.Success();
}

void CreateDialog_InputWindow(Window wndParent, InputControl &input_window, Prop::InputWindow &propInputWindow)
{
   new Dlg_InputWindow(wndParent, input_window, propInputWindow);
}
