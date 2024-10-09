#include "Main.h"

static CntPtrTo<Prop::TextWindow> g_propTextWindowClipboard=MakeCounting<Prop::TextWindow>();

void SetTextWindowProperties(Text::Wnd &window, Prop::TextWindow &prop);

struct Dlg_TextWindow : Wnd_Dialog
{
   Dlg_TextWindow(Window wndParent, Text::Wnd &text_window, Prop::TextWindow &propTextWindow);

private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Paint &msg);

   void Update();
   void Load(const Prop::TextWindow &prop);
   void Save(Prop::TextWindow &prop);

   enum
   {
      IDC_FOREGROUND = 100,
      IDC_BACKGROUND,
      IDC_WEBLINK,
      IDC_FANFOLD,
      IDC_FANCOLOR1,
      IDC_FANCOLOR2,
      IDC_FONT,
      IDC_CHECK_TIME,
      IDC_CHECK_FIXEDWIDTH,
      IDC_COPY_SETTINGS,
      IDC_PASTE_SETTINGS,
   };

   AL::CheckBox *m_pcbInvertBrightness;
   AL::CheckBox *m_pcbFanFold;
   AL::Button *m_pbtFanColor1, *m_pbtFanColor2;
   AL::CheckBox *m_pcbSmoothScrolling;
   AL::Edit *m_pedLineWrappedIndent;
   AL::Edit *m_pedParagraphSpacing;
   AL::CheckBox *m_pcbFixedWidth;
   AL::Edit *m_pedFixedWidth;
   AL::Edit *m_pedHistory;
   AL::CheckBox *m_pcbScrollToBottomOnAdd;
   AL::CheckBox *m_pcbSplitOnPageUp;

   AL::Edit *m_pedMLeft, *m_pedMTop, *m_pedMRight, *m_pedMBottom;
   AL::CheckBox *m_pcbTime, *m_pcbDate, *m_pcb24HR;
   AL::CheckBox *m_pcbDateTimeTooltip;
   AL::Static   *m_pstSample, *m_pstFontName;

   AL::CheckBox *m_pcbShowSelectionCopyTip;
   AL::CheckBox *m_pcbNewContentMarker;

   AL::Button *mp_button_copy_settings, *mp_button_paste_settings;

   Text::Wnd &m_text_window;
   CntPtrTo<Prop::TextWindow> m_ppropTextWindow;
   Prop::Font m_propFont;
   Color m_clrFG, m_clrBG, m_clrLink; // Local copies (in case of cancel!)
   Color m_clrFanFold[2];

   Handle<HFONT> m_hfText;
};

LRESULT Dlg_TextWindow::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command, Msg::Paint>(msg);
}

Dlg_TextWindow::Dlg_TextWindow(Window wndParent, Text::Wnd &text_window, Prop::TextWindow &propTextWindow)
 : m_text_window{text_window},
   m_ppropTextWindow{&propTextWindow}
{
   FixedStringBuilder<256> title;
   if(&propTextWindow==&GlobalTextSettings())
      title("Global ");
   title(STR_Title_TextWindow);
   Create(title, WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_VISIBLE | Wnd_Dialog::Style_Modal, 0 /*dwExStyle*/, wndParent);
}

void Dlg_TextWindow::Update()
//
// Updates the Font Sample and Info
{
   // Invalidate the Sample
   Rect rcSample=ScreenToClient(m_pstSample->WindowRect());
   Invalidate(rcSample, false);

   FixedStringBuilder<256> sBuffer(" " STR_Font ": ", m_propFont.pclName(), "  " STR_Size ": ", m_propFont.SizeInPoints());

   m_pstFontName->SetText(sBuffer);

   m_pedFixedWidth->Enable(m_pcbFixedWidth->IsChecked());
   m_pcb24HR->Enable(m_pcbTime->IsChecked());
   m_pbtFanColor1->Enable(m_pcbFanFold->IsChecked());
   m_pbtFanColor2->Enable(m_pcbFanFold->IsChecked());
}

void Dlg_TextWindow::Load(const Prop::TextWindow &prop)
{
   m_propFont=prop.propFont();
   m_clrFG=prop.clrFore();
   m_clrBG=prop.clrBack();
   m_clrLink=prop.clrLink();
   m_clrFanFold[0]=prop.clrFanFold1();
   m_clrFanFold[1]=prop.clrFanFold2();

   m_hfText=m_propFont.CreateFont();

   // General
   m_pcbInvertBrightness->Check(prop.fInvertBrightness());
   m_pcbFanFold->Check(prop.fFanFold());

   m_pedHistory->Set(prop.History());

   m_pedLineWrappedIndent->Set(prop.LineWrappedIndent());
   m_pedParagraphSpacing->Set(prop.ParagraphSpacing());

   m_pcbFixedWidth->Check(prop.fFixedWidth());
   m_pedFixedWidth->Set(prop.FixedWidthChars());

   m_pcbSmoothScrolling->Check(prop.fSmoothScrolling());
   m_pcbScrollToBottomOnAdd->Check(prop.fScrollToBottomOnAdd());
   m_pcbSplitOnPageUp->Check(prop.fSplitOnPageUp());

   // Margins
   m_pedMLeft->Set(prop.rcMargins().left);
   m_pedMTop->Set(prop.rcMargins().top);
   m_pedMRight->Set(prop.rcMargins().right);
   m_pedMBottom->Set(prop.rcMargins().bottom);

   // Time & Date
   int iTimeFormat=prop.TimeFormat();

   m_pcbTime->Check((iTimeFormat&Text::Time32::F_Time)!=0);
   m_pcbDate->Check((iTimeFormat&Text::Time32::F_Date)!=0);
   m_pcb24HR->Check((iTimeFormat&Text::Time32::F_24HR)!=0);

   m_pcbDateTimeTooltip->Check(prop.TimeFormatToolTip()!=0);

   Update();
}

void Dlg_TextWindow::Save(Prop::TextWindow &prop)
{
   unsigned value;

   // Text
   prop.propFont()=m_propFont;
   prop.clrFore(m_clrFG);
   prop.clrBack(m_clrBG);
   prop.clrLink(m_clrLink);
   prop.fInvertBrightness(m_pcbInvertBrightness->IsChecked());
   prop.fFanFold(m_pcbFanFold->IsChecked());
   prop.clrFanFold1(m_clrFanFold[0]);
   prop.clrFanFold2(m_clrFanFold[1]);

   // General
   if(m_pedHistory->Get(value))
   {
      PinAbove(value, 100U);
      prop.History(value);
   }

   if(m_pedLineWrappedIndent->Get(value))
      prop.LineWrappedIndent(value);

   if(m_pedParagraphSpacing->Get(value))
      prop.ParagraphSpacing(value);

   prop.fFixedWidth(m_pcbFixedWidth->IsChecked());
   if(m_pedFixedWidth->Get(value))
      prop.FixedWidthChars(value);

   prop.fSmoothScrolling(m_pcbSmoothScrolling->IsChecked());
   prop.fScrollToBottomOnAdd(m_pcbScrollToBottomOnAdd->IsChecked());
   prop.fSplitOnPageUp(m_pcbSplitOnPageUp->IsChecked());

   // Margins
   Rect rcMargins=prop.rcMargins();

   if(m_pedMLeft->Get(value))
      rcMargins.left=value;
   if(m_pedMTop->Get(value))
      rcMargins.top=value;
   if(m_pedMRight->Get(value))
      rcMargins.right=value;
   if(m_pedMBottom->Get(value))
      rcMargins.bottom=value;

   prop.rcMargins(rcMargins);

   // Time & Date
   int iTimeFormat=0;

   if(m_pcbTime->IsChecked()) iTimeFormat|=Text::Time32::F_Time;
   if(m_pcbDate->IsChecked()) iTimeFormat|=Text::Time32::F_Date;
   if(m_pcb24HR->IsChecked()) iTimeFormat|=Text::Time32::F_24HR;

   prop.TimeFormat(iTimeFormat);

   if(m_pcbDateTimeTooltip->IsChecked())
      prop.TimeFormatToolTip(Text::Time32::F_Time|Text::Time32::F_Date);
   else
      prop.TimeFormatToolTip(0);

   // Help
   g_ppropGlobal->fShowTip_SelectionCopy(m_pcbShowSelectionCopyTip->IsChecked());
   g_ppropGlobal->fNewContentMarker(m_pcbNewContentMarker->IsChecked());
}

//
// Window Messages
//

LRESULT Dlg_TextWindow::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   {
      AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Text); *pGV << pGB;
      AL::Group *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);
      AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      {
         AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Sample); *pGH << pGB;
         m_pstSample=m_layout.CreateStatic(" ", SS_BLACKFRAME | SS_SUNKEN); pGB->SetChild(m_pstSample);
         m_pstSample->szMinimum().x+=Controls::Button::m_tmButtons.tmAveCharWidth*20;
         m_pstSample->szMinimum().y+=Controls::Button::m_tmButtons.tmHeight*3;

         {
            AL::Group *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
            *pGV << m_layout.CreateButton(IDC_FONT, STR_Font___);
            *pGV << (m_pcbInvertBrightness=m_layout.CreateCheckBox(-1, "Invert Brightness"));
            *pGV << (m_pcbFanFold=m_layout.CreateCheckBox(IDC_FANFOLD, "Fan Fold"));
         }
         {
            AL::Group *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
            *pGV << m_layout.CreateButton(IDC_FOREGROUND, STR_Foreground___);
            *pGV << m_layout.CreateButton(IDC_BACKGROUND, STR_Background___);
            *pGV << m_layout.CreateButton(IDC_WEBLINK, "Web Link...");
            {
               auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
               *pGH << (m_pbtFanColor1=m_layout.CreateButton(IDC_FANCOLOR1, "Fan 1"));
               *pGH << (m_pbtFanColor2=m_layout.CreateButton(IDC_FANCOLOR2, "Fan 2"));
            }
         }
      }

      m_pstFontName=m_layout.CreateStatic(" ", SS_SUNKEN); *pGV << m_pstFontName;
   }
   AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);
   {
      AL::Group *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

      {
         AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_General); *pGV << pGB;
         AL::Group *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);

         {
            AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            m_pedHistory=m_layout.CreateEdit(-1, int2(6, 1), ES_NUMBER);
            m_pedHistory->LimitText(6);
            *pGH << m_layout.CreateStatic(STR_HistoryLines) << m_pedHistory;
         }
         {
            AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            m_pedLineWrappedIndent=m_layout.CreateEdit(-1, int2(4, 1), ES_NUMBER);
            m_pedLineWrappedIndent->LimitText(20);
            *pGH << m_layout.CreateStatic(STR_WrappedLineIndent) << m_pedLineWrappedIndent;
         }
         {
            AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            m_pedParagraphSpacing=m_layout.CreateEdit(-1, int2(4, 1), ES_NUMBER);
            m_pedParagraphSpacing->LimitText(20);
            *pGH << m_layout.CreateStatic(STR_ParagraphSpacing) << m_pedParagraphSpacing;
         }
         {
            AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            m_pcbFixedWidth=m_layout.CreateCheckBox(IDC_CHECK_FIXEDWIDTH, STR_FixedWidth);
            m_pedFixedWidth=m_layout.CreateEdit(-1, int2(4, 1), ES_NUMBER);
            m_pedFixedWidth->LimitText(20);
            *pGH << m_pcbFixedWidth << m_pedFixedWidth << m_layout.CreateStatic(STR_Characters);
         }
         {
            AL::Group *pGH=m_layout.CreateGroup(Direction::Horizontal); *pGV << pGH;

            m_pcbSmoothScrolling=m_layout.CreateCheckBox(-1, STR_SmoothScrolling);
            *pGH << m_pcbSmoothScrolling;
         }
         {
            m_pcbScrollToBottomOnAdd=m_layout.CreateCheckBox(-1, STR_ScrollToBottomOnAdd);
            m_pcbSplitOnPageUp=m_layout.CreateCheckBox(-1, "Split on page up");

            *pGV << m_pcbScrollToBottomOnAdd << m_pcbSplitOnPageUp;
         }
      }

      {
         AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_DateTimeDisplay); *pGV << pGB;
         AL::Group *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);

         {
            AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            m_pcbTime=m_layout.CreateCheckBox(IDC_CHECK_TIME, STR_Time);
            m_pcb24HR=m_layout.CreateCheckBox(-1, STR_24Hour);
            *pGH << m_pcbTime << m_pcb24HR;
         }

         m_pcbDate=m_layout.CreateCheckBox(-1, STR_Date);
         *pGV << m_pcbDate;

         AL::Spacer *pSpacer=m_layout.CreateSpacer(); *pGV << pSpacer;
         pSpacer->SetCharSize(0, 1);

         m_pcbDateTimeTooltip=m_layout.CreateCheckBox(-1, STR_DateTimeTooltip);
         *pGV << m_pcbDateTimeTooltip;
      }
   }
   {
      AL::Group *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
      {
         AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Margins); *pGV << pGB;
         AL::Group *pGH=m_layout.CreateGroup_Horizontal(); pGB->SetChild(pGH);
         {
            m_pedMLeft  =m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
            m_pedMRight =m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
            m_pedMTop   =m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
            m_pedMBottom=m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
            m_pedMLeft->LimitText(4);
            m_pedMTop->LimitText(4);
            m_pedMRight->LimitText(4);
            m_pedMBottom->LimitText(4);

            AL::Static *pLeft, *pRight, *pTop, *pBottom;
            pLeft  =m_layout.CreateStatic(STR_Left);
            pRight =m_layout.CreateStatic(STR_Right);
            pTop   =m_layout.CreateStatic(STR_Top);
            pBottom=m_layout.CreateStatic(STR_Bottom);
            SetAllToMax(pLeft->szMinimum().x, pRight->szMinimum().x, pTop->szMinimum().x, pBottom->szMinimum().x);

            AL::Group *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
            {
               AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
               *pGH << pLeft << m_pedMLeft;
            }
            {
               AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
               *pGH << pRight << m_pedMRight;
            }
            {
               AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
               *pGH << pTop << m_pedMTop;
            }
            {
               AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
               *pGH << pBottom << m_pedMBottom;
            }
         }
         {
            AL::Group *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
            *pGV >> AL::Style::Attach_All;
            *pGV << m_layout.CreateStatic(STR_Pixels);
         }

         {
            AL::GroupBox *pGB=m_layout.CreateGroupBox("Help (Applies globally)"); *pGV << pGB;
            
            auto &gv=*m_layout.CreateGroup_Vertical(); pGB->SetChild(&gv);

            m_pcbShowSelectionCopyTip=m_layout.CreateCheckBox(-1, "Show Selection Copied Popup");
            gv << m_pcbShowSelectionCopyTip;

            m_pcbNewContentMarker=m_layout.CreateCheckBox(-1, "New content markers");
            gv << m_pcbNewContentMarker;
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
   }

   // Help
   m_pcbShowSelectionCopyTip->Check(g_ppropGlobal->fShowTip_SelectionCopy());
   m_pcbNewContentMarker->Check(g_ppropGlobal->fNewContentMarker());

   Load(*m_ppropTextWindow);
   return msg.Success();
}

LRESULT Dlg_TextWindow::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDOK:
         Save(*m_ppropTextWindow);
         Global_PropChange(); // To handle the selection message/new content global settings
         if(m_ppropTextWindow==&GlobalTextSettings())
            OnGlobalTextSettingsModified(); // Update them all through an event
         else
            ::SetTextWindowProperties(m_text_window, *m_ppropTextWindow);
      case IDCANCEL:
         Close();
         break;

      case IDC_COPY_SETTINGS: Save(*g_propTextWindowClipboard); break;
      case IDC_PASTE_SETTINGS: Load(*g_propTextWindowClipboard); break;

      case IDC_FONT:
         if(m_propFont.ChooseFont(*this))
         {
            m_propFont.fBold(false);
            m_propFont.fItalic(false);
            m_propFont.fUnderline(false);
            m_propFont.fStrikeout(false);
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

      case IDC_WEBLINK:
         ChooseColorSimple(*this, &m_clrLink);
         break;

      case IDC_FANFOLD: Update(); break;
      case IDC_FANCOLOR1: ChooseColorSimple(*this, &m_clrFanFold[0]); break;
      case IDC_FANCOLOR2: ChooseColorSimple(*this, &m_clrFanFold[1]); break;

      case IDC_CHECK_TIME:
      case IDC_CHECK_FIXEDWIDTH:
         if(msg.uCodeNotify()==BN_CLICKED)
            Update();
         break;
   }

   return msg.Success();
}

LRESULT Dlg_TextWindow::On(const Msg::Paint &msg)
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

void CreateDialog_TextWindow(Window wndParent, Text::Wnd &text_window, Prop::TextWindow &propTextWindow)
{
   new Dlg_TextWindow(wndParent, text_window, propTextWindow);
}
