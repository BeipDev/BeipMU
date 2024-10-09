//
// Triggers Dialog
//

#include "Main.h"
#include "Wnd_PropTree.h"
#include "Sounds.h"
#include "FindString.h"
#include "Matcharoo.h"

void ApplyParagraphToLine(const Prop::Trigger_Paragraph &p, Text::Line &line, Array<const uint2> ranges);

namespace
{

struct ITriggerDialog : Wnd_ChildDialog
{
   virtual void SetTrigger(Prop::Trigger *ppropTrigger)=0; // Sets a new Trigger
   virtual void Save()=0; // Save the current data displayed in the dialog

   virtual void SetEnabled(bool fEnabled); // If the whole dialog should be disabled
   virtual void UpdateEnabled()=0;

protected:
   bool m_fEnabled{true};
};

void ITriggerDialog::SetEnabled(bool fEnabled)
{
   if(m_fEnabled==fEnabled)
      return;

   m_fEnabled=fEnabled;
   UpdateEnabled();
}

// This define is handy for derived classes
#define Prototypes_ITriggerDialog \
   void SetTrigger(Prop::Trigger *ppropTrigger) override; \
   void UpdateEnabled() override; \
   void Save() override;

//
// Colors Tab -----------------------------------------------------------------
//

struct Dlg_Trigger_Appearance : ITriggerDialog
{
   Dlg_Trigger_Appearance(Window wndParent);

private:
   Prototypes_ITriggerDialog;

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Paint &msg);

   void Update();
   void InvalidateSample();

   Prop::Trigger_Color *m_ptColor{};
   Prop::Trigger_Style *m_ptStyle{};

   enum
   {
      IDC_FONT=100,
      IDC_FOREGROUND,
      IDC_BACKGROUND,
      IDC_STYLE,
      IDC_UPDATEENABLED,
   };

   AL::CheckBox *m_pcbForeground, *m_pcbForegroundDefault, *m_pcbBackground, *m_pcbBackgroundDefault, *m_pcbFont, *m_pcbFontDefault;
   AL::CheckBox *m_pcbForeground_hash, *m_pcbBackground_hash;
   AL::Button *m_pbtForeground, *m_pbtBackground, *m_pbtFont;
   AL::CheckBox *m_pcbWholeLine;
   AL::CheckBox *m_pcbItalic, *m_pcbBold, *m_pcbUnderline, *m_pcbStrikeout, *m_pcbFlash, *m_pcbFlashFast;
   AL::Static *m_pstSample;

   Color m_clrForeground, m_clrBackground;
   Handle<HBRUSH> m_hbBackground;

   Prop::Font m_font;
   Handle<HFONT> m_hfForeground;
};

LRESULT Dlg_Trigger_Appearance::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command, Msg::Paint>(msg);
}

Dlg_Trigger_Appearance::Dlg_Trigger_Appearance(Window wndParent)
{
   Create(STR_Title_Appearance, wndParent);
}

void Dlg_Trigger_Appearance::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
   {
      m_ptColor=&ppropTrigger->propColor();

      m_clrForeground=m_ptColor->clrFore();
      m_clrBackground=m_ptColor->clrBack();

      if(m_ptColor->pclFontFace().Length()!=0 && m_ptColor->FontSize())
      {
         m_font.pclName(m_ptColor->pclFontFace());
         m_font.Size(m_ptColor->FontSize());
      }
      else
         m_font=g_ppropGlobal->propWindows().propMainWindowSettings().propOutput().propFont();

      m_ptStyle=&ppropTrigger->propStyle();
   }
   else
   {
      m_ptColor=nullptr;
      m_ptStyle=nullptr;
   }

   Update();
}

void Dlg_Trigger_Appearance::Save()
{
   if(!m_ptColor) return;

   m_ptColor->fFore(m_pcbForeground->IsChecked());
   m_ptColor->fForeDefault(m_pcbForegroundDefault->IsChecked());
   m_ptColor->fForeHash(m_pcbForeground_hash->IsChecked());
   m_ptColor->fBack(m_pcbBackground->IsChecked());
   m_ptColor->fBackDefault(m_pcbBackgroundDefault->IsChecked());
   m_ptColor->fBackHash(m_pcbBackground_hash->IsChecked());
   m_ptColor->fWholeLine(m_pcbWholeLine->IsChecked());

   m_ptColor->clrFore(m_clrForeground);
   m_ptColor->clrBack(m_clrBackground);

   m_ptColor->pclFontFace(m_pcbFont->IsChecked() ? m_font.pclName() : OwnedString());
   m_ptColor->FontSize(m_pcbFont->IsChecked() ? m_font.Size() : 0);
   m_ptColor->fFontDefault(m_pcbFontDefault->IsChecked());

   Assert(m_ptStyle);

   {
      bool fItalic=m_pcbItalic->IsChecked();
      m_ptStyle->fItalic(fItalic);
      m_ptStyle->fSetItalic(fItalic);
   }

   {
      bool fBold=m_pcbBold->IsChecked();
      m_ptStyle->fBold(fBold);
      m_ptStyle->fSetBold(fBold);
   }

   {
      bool fUnderline=m_pcbUnderline->IsChecked();
      m_ptStyle->fUnderline(fUnderline);
      m_ptStyle->fSetUnderline(fUnderline);
   }

   {
      bool fStrikeout=m_pcbStrikeout->IsChecked();
      m_ptStyle->fStrikeout(fStrikeout);
      m_ptStyle->fSetStrikeout(fStrikeout);
   }

   m_ptStyle->fFlash(m_pcbFlash->IsChecked());
   m_ptStyle->fFlashFast(m_pcbFlashFast->IsChecked());
   m_ptStyle->fWholeLine(m_pcbWholeLine->IsChecked());

}

void Dlg_Trigger_Appearance::Update()
{
   bool fWholeLine=false;

   if(m_ptColor)
   {
      m_pcbFont->Check(m_ptColor->FontSize()!=0);
      m_pcbFontDefault->Check(m_ptColor->fFontDefault());
      m_pcbForeground->Check(m_ptColor->fFore());
      m_pcbForegroundDefault->Check(m_ptColor->fForeDefault());
      m_pcbForeground_hash->Check(m_ptColor->fForeHash());
      m_pcbBackground->Check(m_ptColor->fBack());
      m_pcbBackgroundDefault->Check(m_ptColor->fBackDefault());
      m_pcbBackground_hash->Check(m_ptColor->fBackHash());
      fWholeLine|=m_ptColor->fWholeLine();
   }

   if(m_ptStyle)
   {
      m_pcbItalic->Check(m_ptStyle->fItalic());
      m_pcbBold->Check(m_ptStyle->fBold());
      m_pcbUnderline->Check(m_ptStyle->fUnderline());
      m_pcbStrikeout->Check(m_ptStyle->fStrikeout());
      fWholeLine|=m_ptStyle->fWholeLine();
      m_pcbFlash->Check(m_ptStyle->fFlash());
      m_pcbFlashFast->Check(m_ptStyle->fFlashFast());
   }

   m_pcbWholeLine->Check(fWholeLine);
   InvalidateSample();
   UpdateEnabled();
}

void Dlg_Trigger_Appearance::UpdateEnabled()
{
   bool fEnable=m_ptColor && m_fEnabled;
   EnableWindows(fEnable, *m_pcbFont, *m_pcbForeground, *m_pcbBackground);
   EnableWindows(fEnable && m_pcbFont->IsChecked(), *m_pbtFont, *m_pcbFontDefault);
   EnableWindows(fEnable && m_pcbForeground->IsChecked(), *m_pbtForeground, *m_pcbForegroundDefault, *m_pcbForeground_hash);
   EnableWindows(fEnable && m_pcbBackground->IsChecked(), *m_pbtBackground, *m_pcbBackgroundDefault, *m_pcbBackground_hash);
   EnableWindows(fEnable, *m_pcbItalic, *m_pcbBold, *m_pcbUnderline, *m_pcbStrikeout, *m_pcbFlash, *m_pcbWholeLine);
   EnableWindows(fEnable && m_pcbFlash->IsChecked(), *m_pcbFlashFast);
}

void Dlg_Trigger_Appearance::InvalidateSample()
{
   if(!m_ptColor)
      return;

   auto &propOutput=g_ppropGlobal->propWindows().propMainWindowSettings().propOutput();

   m_font.fItalic(m_pcbItalic->IsChecked());
   m_font.fBold(m_pcbBold->IsChecked());
   m_font.fStrikeout(m_pcbStrikeout->IsChecked());
   m_font.fUnderline(m_pcbUnderline->IsChecked());

   if(m_pcbFont->IsChecked() && !m_pcbFontDefault->IsChecked())
      m_hfForeground=m_font.CreateFont();
   else
   {
      // Use default font face & size, but keep the styles
      Prop::Font font=m_font;
      font.pclName(propOutput.propFont().pclName());
      font.Size(propOutput.propFont().Size());
      m_hfForeground=font.CreateFont();
   }
   m_hbBackground=CreateSolidBrush(m_pcbBackground->IsChecked() && !m_pcbBackgroundDefault->IsChecked() ? m_clrBackground : propOutput.clrBack());

   Invalidate(ScreenToClient(m_pstSample->WindowRect()), true);
}

//
// Window Messages
//
AL::CheckBox *CreateHorizontalCheckbox(AL::LayoutEngine &layout, AL::Group &parent, int id, ConstString label)
{
   AL::Group_Horizontal *pGroup=layout.CreateGroup_Horizontal(); parent << pGroup; pGroup->weight(0);
   pGroup->MatchBaseline(false);
   *pGroup >> AL::Style::Attach_Vert;

   auto line=layout.CreateStatic(" ", SS_ETCHEDHORZ); line->weight(1); line->szMinimum()=int2(2,2);
   auto checkbox=layout.CreateCheckBox(id, label);

   *pGroup << checkbox << line;
   return checkbox;
}

LRESULT Dlg_Trigger_Appearance::On(const Msg::Create &msg)
{
   auto pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV); *pGV >> AL::Style::Attach_Bottom;
   {
      auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      {
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

         {
            m_pcbFont=CreateHorizontalCheckbox(m_layout, *pGV, IDC_STYLE, "Font");
            AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            pGH->MatchBaseline(false); *pGH >> AL::Style::Attach_Right;

            m_pbtFont=m_layout.CreateButton(IDC_FONT, "Choose");
            m_pcbFontDefault=m_layout.CreateCheckBox(IDC_STYLE, "Default");
            *pGH << m_pbtFont << m_pcbFontDefault;
         }
         {
            m_pcbForeground=CreateHorizontalCheckbox(m_layout, *pGV, IDC_STYLE, "Foreground Color");
            AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            pGH->MatchBaseline(false); *pGH >> AL::Style::Attach_Right;

            m_pbtForeground=m_layout.CreateButton(IDC_FOREGROUND, "Choose");
            m_pcbForeground_hash=m_layout.CreateCheckBox(-1, "Hash");
            m_pcbForegroundDefault=m_layout.CreateCheckBox(IDC_STYLE, "Default");
            *pGH << m_pbtForeground << m_pcbForegroundDefault << m_pcbForeground_hash;
         }
         {
            m_pcbBackground=CreateHorizontalCheckbox(m_layout, *pGV, IDC_STYLE, "Background Color");
            AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            pGH->MatchBaseline(false); *pGH >> AL::Style::Attach_Right;

            m_pbtBackground=m_layout.CreateButton(IDC_BACKGROUND, "Choose");
            m_pcbBackground_hash=m_layout.CreateCheckBox(-1, "Hash");
            m_pcbBackgroundDefault=m_layout.CreateCheckBox(IDC_STYLE, "Default");
            *pGH << m_pbtBackground << m_pcbBackgroundDefault << m_pcbBackground_hash;
         }

         SetAllToMax(m_pbtForeground->szMinimum().x, m_pbtBackground->szMinimum().x, m_pbtFont->szMinimum().x);
      }
      {
         m_pcbBold=m_layout.CreateCheckBox(IDC_STYLE, STR_Bold);
         m_pcbItalic=m_layout.CreateCheckBox(IDC_STYLE, STR_Italic);
         m_pcbUnderline=m_layout.CreateCheckBox(IDC_STYLE, STR_Underline);
         m_pcbStrikeout=m_layout.CreateCheckBox(IDC_STYLE, STR_Strikeout);
         m_pcbFlash=m_layout.CreateCheckBox(IDC_UPDATEENABLED, STR_Flashing);
         m_pcbFlashFast=m_layout.CreateCheckBox(-1, "Use Fast Flash");

         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
         *pGV << m_pcbBold << m_pcbItalic << m_pcbUnderline << m_pcbStrikeout << m_pcbFlash << m_pcbFlashFast;
      }
   }
   {
      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->MatchBaseline(false); *pGH >> AL::Style::Attach_Vert;

      AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Sample); *pGH << pGB;
      m_pstSample=m_layout.CreateStatic(" ", SS_BLACKFRAME | SS_SUNKEN); pGB->SetChild(m_pstSample);
      m_pstSample->szMinimum()+=int2(Controls::Button::m_tmButtons.tmAveCharWidth*20, Controls::Button::m_tmButtons.tmHeight*3);

      m_pcbWholeLine=m_layout.CreateCheckBox(-1, STR_WholeLine);
      *pGH << m_pcbWholeLine;
   }

   Update();
   return msg.Success();
}

LRESULT Dlg_Trigger_Appearance::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_FONT:
         if(m_font.ChooseFont(*this))
         {
            m_hfForeground=m_font.CreateFont();
            InvalidateSample();
         }
         break;

      case IDC_FOREGROUND:
         if(ChooseColorSimple(*this, &m_clrForeground))
         {
            m_pcbForeground->Check(true);
            InvalidateSample();
         }
         break;

      case IDC_BACKGROUND:
         if(ChooseColorSimple(*this, &m_clrBackground))
         {
            m_pcbBackground->Check(true);
            InvalidateSample();
         }
         break;

      case IDC_STYLE:
         InvalidateSample();
         UpdateEnabled();
         break;

      case IDC_UPDATEENABLED:
         UpdateEnabled();
         break;
   }

   return msg.Success();
}

LRESULT Dlg_Trigger_Appearance::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);
   if(!m_ptColor) return msg.Success();

   Assert(m_hbBackground && m_hfForeground);
   Rect rcSample=ScreenToClient(m_pstSample->WindowRect());

   ps.FillRect(rcSample, m_hbBackground);
   ps.SetTextAlign(TA_CENTER);
   ps.SetTextColor(m_pcbForeground->IsChecked() && !m_pcbForegroundDefault->IsChecked() ? m_clrForeground : g_ppropGlobal->propWindows().propMainWindowSettings().propOutput().clrFore());
   ps.SetBackgroundMode(TRANSPARENT);

   DC::FontSelector _(ps, m_hfForeground);
   ps.ExtTextOut(int2(rcSample.left+rcSample.size().x/2, rcSample.top+(rcSample.size().y-abs(m_font.Size()))/2),
                 ETO_CLIPPED, rcSample, STR_AaBbCc, nullptr);

   return msg.Success();
}

//
// Paragraph
//

struct Dlg_Trigger_Paragraph : ITriggerDialog, Text::IHost
{
   Dlg_Trigger_Paragraph(Window wndParent);

private:
   Prototypes_ITriggerDialog;

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void Update();
   void SaveTo(Prop::Trigger_Paragraph &p);
   void UpdateSample();

   Prop::Trigger_Paragraph *m_pParagraph{};

   enum
   {
      IDC_BACKGROUND=100,
      IDC_STROKE,
      IDC_UPDATE,
   };

   AL::CheckBox *m_pcbIndent_Left, *m_pcbIndent_Right;
   AL::Edit *m_edIndent_Left, *m_edIndent_Right;
   AL::CheckBox *m_pcbPadding_Top, *m_pcbPadding_Bottom;
   AL::Edit *m_edPadding_Top, *m_edPadding_Bottom;
   AL::CheckBox *m_pcbBorder;
   AL::Edit *m_edBorder;

   AL::CheckBox *m_pcbBorderStyle;
   AL::Radio *m_pcbSquare, *m_pcbRound;

   AL::CheckBox *m_pcbAlignment;
   AL::Radio *m_pcbAlignLeft, *m_pcbAlignCenter, *m_pcbAlignRight;

   Color m_clrBackground;
   AL::CheckBox *m_pcbBackground, *m_pcbBackground_hash;
   AL::Button *m_pbtBackground;

   AL::CheckBox *m_pcbStroke;
   AL::Edit *m_edStrokeWidth;
   AL::Button *m_pbtStrokeColor;
   Color m_clrStroke;
   AL::CheckBox *m_pcbStrokeHash;

   AL::Static *m_pStrokeStyle;
   AL::ComboBox *m_pStrokeStyleCombo;

   bool m_fIgnoreUpdates{};

   Text::Wnd *m_pText;
};

LRESULT Dlg_Trigger_Paragraph::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command>(msg);
}

Dlg_Trigger_Paragraph::Dlg_Trigger_Paragraph(Window wndParent)
{
   Create(STR_Title_Paragraph, wndParent);
}

void Dlg_Trigger_Paragraph::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
   {
      m_pParagraph=&ppropTrigger->propParagraph();
      m_clrBackground=m_pParagraph->clrBackground();
      m_clrStroke=m_pParagraph->clrStroke();
   }
   else
      m_pParagraph=nullptr;

   Update();
}

void Dlg_Trigger_Paragraph::Save()
{
   if(!m_pParagraph) return;
   SaveTo(*m_pParagraph);
}

void Dlg_Trigger_Paragraph::SaveTo(Prop::Trigger_Paragraph &p)
{
   p.fUseBackgroundColor(m_pcbBackground->IsChecked());
   p.clrBackground(m_clrBackground);
   p.fBackgroundHash(m_pcbBackground_hash->IsChecked());

   p.fUseStroke(m_pcbStroke->IsChecked());
   p.clrStroke(m_clrStroke);
   if(int value;m_edStrokeWidth->Get(value))
      p.StrokeWidth(value);
   p.fStrokeHash(m_pcbStrokeHash->IsChecked());

   p.StrokeStyle(m_pStrokeStyleCombo->GetCurSel());
//   p.iStrokeStyle(m_pcbStrokeTop->IsChecked()+m_pcbStrokeBottom->IsChecked()*2);

   p.fUseIndent_Left(m_pcbIndent_Left->IsChecked());
   if(int value;m_edIndent_Left->Get(value))
      p.Indent_Left(value);
   p.fUseIndent_Right(m_pcbIndent_Right->IsChecked());
   if(int value;m_edIndent_Right->Get(value))
      p.Indent_Right(value);

   p.fUsePadding_Top(m_pcbPadding_Top->IsChecked());
   if(int value;m_edPadding_Top->Get(value))
      p.Padding_Top(value);
   p.fUsePadding_Bottom(m_pcbPadding_Bottom->IsChecked());
   if(int value;m_pcbPadding_Bottom->IsChecked() && m_edPadding_Bottom->Get(value))
      p.Padding_Bottom(value);

   p.fUseBorder(m_pcbBorder->IsChecked());
   if(int value;m_edBorder->Get(value))
      p.Border(value);

   p.fUseBorderStyle(m_pcbBorderStyle->IsChecked());
   p.BorderStyle(m_pcbRound->IsChecked());

   p.fUseAlignment(m_pcbAlignment->IsChecked());
   p.Alignment(m_pcbAlignCenter->IsChecked()+m_pcbAlignRight->IsChecked()*2);
}

void Dlg_Trigger_Paragraph::Update()
{
   if(m_pParagraph)
   {
      RestorerOf _(m_fIgnoreUpdates); m_fIgnoreUpdates=true;

      m_pcbBackground->Check(m_pParagraph->fUseBackgroundColor());
      m_pcbBackground_hash->Check(m_pParagraph->fBackgroundHash());

      m_pcbStroke->Check(m_pParagraph->fUseStroke());
      m_edStrokeWidth->Set(m_pParagraph->StrokeWidth());

      m_pcbStrokeHash->Check(m_pParagraph->fStrokeHash());
      m_pStrokeStyleCombo->SetCurSel(m_pParagraph->StrokeStyle());

      m_pcbIndent_Left->Check(m_pParagraph->fUseIndent_Left());
      m_edIndent_Left->Set(m_pParagraph->Indent_Left());
      m_pcbIndent_Right->Check(m_pParagraph->fUseIndent_Right());
      m_edIndent_Right->Set(m_pParagraph->Indent_Right());

      m_pcbPadding_Top->Check(m_pParagraph->fUsePadding_Top());
      m_edPadding_Top->Set(m_pParagraph->Padding_Top());
      m_pcbPadding_Bottom->Check(m_pParagraph->fUsePadding_Bottom());
      m_edPadding_Bottom->Set(m_pParagraph->Padding_Bottom());

      m_pcbBorder->Check(m_pParagraph->fUseBorder());
      m_edBorder->Set(m_pParagraph->Border());

      m_pcbBorderStyle->Check(m_pParagraph->fUseBorderStyle());
      m_pcbSquare->Check(m_pParagraph->BorderStyle()==0);
      m_pcbRound->Check(m_pParagraph->BorderStyle()==1);

      m_pcbAlignment->Check(m_pParagraph->fUseAlignment());
      m_pcbAlignLeft->Check(m_pParagraph->Alignment()==0);
      m_pcbAlignCenter->Check(m_pParagraph->Alignment()==1);
      m_pcbAlignRight->Check(m_pParagraph->Alignment()==2);
   }

   UpdateEnabled();
   UpdateSample();
}

void Dlg_Trigger_Paragraph::UpdateSample()
{
   m_pText->Clear();
   m_pText->Add(Text::Line::CreateFromText("Not the sample text."));
   auto pLine=Text::Line::CreateFromText("Some sample text.");

   Prop::Trigger_Paragraph p;
   SaveTo(p);
   ApplyParagraphToLine(p, *pLine, {{0, pLine->GetText().Count()}});
   m_pText->Add(std::move(pLine));
   m_pText->Add(Text::Line::CreateFromText("Not the sample text."));
}

void Dlg_Trigger_Paragraph::UpdateEnabled()
{
   bool fEnable=m_pParagraph && m_fEnabled;
   EnableWindows(fEnable, *m_pcbIndent_Left, *m_pcbIndent_Right, *m_pcbPadding_Top, *m_pcbPadding_Bottom, *m_pcbBorder, *m_pcbBorderStyle, *m_pcbAlignment, *m_pcbBackground, *m_pcbStroke);

   EnableWindows(m_pcbIndent_Left->IsChecked() && fEnable, *m_edIndent_Left);
   EnableWindows(m_pcbIndent_Right->IsChecked() && fEnable, *m_edIndent_Right);

   EnableWindows(m_pcbPadding_Top->IsChecked() && fEnable, *m_edPadding_Top);
   EnableWindows(m_pcbPadding_Bottom->IsChecked() && fEnable, *m_edPadding_Bottom);

   EnableWindows(m_pcbBorder->IsChecked() && fEnable, *m_edBorder);
   EnableWindows(m_pcbBorderStyle->IsChecked() && fEnable, *m_pcbSquare, *m_pcbRound);

   EnableWindows(m_pcbAlignment->IsChecked() && fEnable, *m_pcbAlignLeft, *m_pcbAlignCenter, *m_pcbAlignRight);

   EnableWindows(m_pcbBackground->IsChecked() && fEnable, *m_pbtBackground, *m_pcbBackground_hash);
   EnableWindows(m_pcbStroke->IsChecked() && fEnable, *m_edStrokeWidth, *m_pbtStrokeColor, *m_pcbStrokeHash, *m_pStrokeStyle, *m_pStrokeStyleCombo);
}

//
// Window Messages
//
LRESULT Dlg_Trigger_Paragraph::On(const Msg::Create &msg)
{
   auto pGVRoot=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGVRoot);
   {
      auto pGV=m_layout.CreateGroup_Vertical(); *pGVRoot << pGV;
      pGV->weight(0);

      {
         auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;

         m_pcbBackground=m_layout.CreateCheckBox(IDC_UPDATE, "Background");
         m_pbtBackground=m_layout.CreateButton(IDC_BACKGROUND, "Color");
         m_pbtBackground->weight(0);
         m_pcbBackground_hash=m_layout.CreateCheckBox(IDC_UPDATE, "Hash");
         *pGH << m_pcbBackground << m_pbtBackground << m_pcbBackground_hash;
      }
      {
         auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;

         m_pcbStroke=m_layout.CreateCheckBox(IDC_UPDATE, "Stroke (px)");
         m_edStrokeWidth=m_layout.CreateEdit(IDC_UPDATE, int2(5, 1), ES_NUMBER);
         m_edStrokeWidth->weight(0);
         m_pbtStrokeColor=m_layout.CreateButton(IDC_STROKE, "Color");
         m_pbtStrokeColor->weight(0);
         m_pcbStrokeHash=m_layout.CreateCheckBox(IDC_UPDATE, "Hash");

         m_pStrokeStyle=m_layout.CreateStatic("  Style:");
         m_pStrokeStyleCombo=m_layout.CreateComboBox(IDC_UPDATE, int2(7, 3), CBS_DROPDOWNLIST);
         m_pStrokeStyleCombo->weight(0);
         for(auto string : { ConstString("Outline"), ConstString("Top"), ConstString("Bottom")})
            m_pStrokeStyleCombo->AddString(string);

         *pGH << m_pcbStroke << m_edStrokeWidth << m_pbtStrokeColor << m_pcbStrokeHash << m_pStrokeStyle << m_pStrokeStyleCombo;
      }

      m_pcbBorder=m_layout.CreateCheckBox(IDC_UPDATE, "Border (px):");
      m_edBorder=m_layout.CreateEdit(IDC_UPDATE, int2(5,1), ES_NUMBER);
      m_edBorder->LimitText(2);
      m_edBorder->weight(0);
      *pGV << m_layout.CreateGroup_Horizontal(m_pcbBorder, m_edBorder);

      m_pcbBorderStyle=m_layout.CreateCheckBox(IDC_UPDATE, "Border Style:");
      {
         auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
         m_pcbSquare=m_layout.CreateRadio(IDC_UPDATE, "Square", true);
         m_pcbRound=m_layout.CreateRadio(IDC_UPDATE, "Round");
         *pGH << m_pcbBorderStyle << m_pcbSquare << m_pcbRound;
      }

      m_pcbAlignment=m_layout.CreateCheckBox(IDC_UPDATE, "Alignment:");
      {
         auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
         m_pcbAlignLeft=m_layout.CreateRadio(IDC_UPDATE, "Left", true);
         m_pcbAlignCenter=m_layout.CreateRadio(IDC_UPDATE, "Center");
         m_pcbAlignRight=m_layout.CreateRadio(IDC_UPDATE, "Right");
         *pGH << m_pcbAlignment << m_pcbAlignLeft << m_pcbAlignCenter << m_pcbAlignRight;
      }

      {
         auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
         {
            auto pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
            m_pcbIndent_Left=m_layout.CreateCheckBox(IDC_UPDATE, "Indent Left %:");
            m_edIndent_Left=m_layout.CreateEdit(IDC_UPDATE, int2(5,1), ES_NUMBER);
            m_edIndent_Left->LimitText(2);
            m_edIndent_Left->weight(0);
            m_pcbIndent_Right=m_layout.CreateCheckBox(IDC_UPDATE, "Indent Right %:");
            m_edIndent_Right=m_layout.CreateEdit(IDC_UPDATE, int2(5,1), ES_NUMBER);
            m_edIndent_Right->LimitText(2);
            m_edIndent_Right->weight(0);
            SetAllToMax(m_pcbIndent_Left->szMinimum().x, m_pcbIndent_Right->szMinimum().x);
            *pGV << m_layout.CreateGroup_Horizontal(m_pcbIndent_Left, m_edIndent_Left);
            *pGV << m_layout.CreateGroup_Horizontal(m_pcbIndent_Right, m_edIndent_Right);
         }

         {
            auto pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
            m_pcbPadding_Top=m_layout.CreateCheckBox(IDC_UPDATE, "Padding Top (px):");
            m_edPadding_Top=m_layout.CreateEdit(IDC_UPDATE, int2(5,1), ES_NUMBER);
            m_edPadding_Top->weight(0);
            m_edPadding_Top->LimitText(2);
            m_pcbPadding_Bottom=m_layout.CreateCheckBox(IDC_UPDATE, "Padding Bottom (px):");
            m_edPadding_Bottom=m_layout.CreateEdit(IDC_UPDATE, int2(5,1), ES_NUMBER);
            m_edPadding_Bottom->LimitText(2);
            m_edPadding_Bottom->weight(0);
            SetAllToMax(m_pcbPadding_Top->szMinimum().x, m_pcbPadding_Bottom->szMinimum().x);
            *pGV << m_layout.CreateGroup_Horizontal(m_pcbPadding_Top, m_edPadding_Top);
            *pGV << m_layout.CreateGroup_Horizontal(m_pcbPadding_Bottom, m_edPadding_Bottom);
         }
      }
   }

   {
      m_pText=new Text::Wnd(*this, *this);
      m_pText->SetFont(g_ppropGlobal->pclUIFontName(), g_ppropGlobal->UIFontSize(), DEFAULT_CHARSET);
      m_pText->SetSmoothScrolling(false);
      m_pText->SetMargins(Rect(1,1,1,1));
      auto *pTextLayout=m_layout.AddChildWindow(*m_pText);
      pTextLayout->szMinimum()=int2(160, 100);
      *pGVRoot << pTextLayout;
   }

   Update();
   return msg.Success();
}

LRESULT Dlg_Trigger_Paragraph::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_BACKGROUND:
         if(ChooseColorSimple(*this, &m_clrBackground))
         {
            m_pcbBackground->Check(true);
            UpdateSample();
         }
         break;
      case IDC_STROKE:
         if(ChooseColorSimple(*this, &m_clrStroke))
         {
            m_pcbStroke->Check(true);
            UpdateSample();
         }
         break;

      case IDC_UPDATE:
         if(m_fIgnoreUpdates)
            break;
         if(msg.uCodeNotify()==BN_CLICKED)
         {
            UpdateEnabled();
            UpdateSample();
         }
         if(msg.uCodeNotify()==EN_CHANGE)
            UpdateSample();
         break;
   }

   return msg.Success();
}

//
// Spawn Tab --------------------------------------------------------------------
//

struct Dlg_Trigger_Spawn : ITriggerDialog
{
   Dlg_Trigger_Spawn(Window wndParent) { Create("Spawn", wndParent); }

private:
   Prototypes_ITriggerDialog;
   ~Dlg_Trigger_Spawn() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void Update();

   Prop::Trigger_Spawn *m_ptSpawn{};

   AL::CheckBox *m_pcbActive;
   AL::Edit *m_pedTitle, *m_pedTabGroup, *m_pedCaptureUntil;
   AL::CheckBox *m_pcbOnlyChildrenDuringCapture;
   AL::CheckBox *m_pcbClear, *m_pcbShowTab, *m_pcbGagLog, *m_pcbCopy;
};

LRESULT Dlg_Trigger_Spawn::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Trigger_Spawn::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
      m_ptSpawn=&ppropTrigger->propSpawn();
   else
      m_ptSpawn=nullptr;

   Update();
}

void Dlg_Trigger_Spawn::Save()
{
   if(!m_ptSpawn) return;

   m_ptSpawn->fActive(m_pcbActive->IsChecked());
   m_ptSpawn->pclTitle(m_pedTitle->GetText());
   m_ptSpawn->pclTabGroup(m_pedTabGroup->GetText());
   m_ptSpawn->pclCaptureUntil(m_pedCaptureUntil->GetText()); m_ptSpawn->mp_regex_cache=nullptr;
   m_ptSpawn->fOnlyChildrenDuringCapture(m_pcbOnlyChildrenDuringCapture->IsChecked());
   m_ptSpawn->fClear(m_pcbClear->IsChecked());
   m_ptSpawn->fShowTab(m_pcbShowTab->IsChecked());
   m_ptSpawn->fGagLog(m_pcbGagLog->IsChecked());
   m_ptSpawn->fCopy(m_pcbCopy->IsChecked());
}

void Dlg_Trigger_Spawn::UpdateEnabled()
{
   bool fEnabled=m_ptSpawn && m_fEnabled;
   EnableWindows(fEnabled, *m_pcbActive);
   fEnabled&=m_pcbActive->IsChecked();
   EnableWindows(fEnabled, *m_pedTitle, *m_pedTabGroup, *m_pedCaptureUntil, *m_pcbOnlyChildrenDuringCapture, *m_pcbClear, *m_pcbShowTab, *m_pcbGagLog, *m_pcbCopy);
}

void Dlg_Trigger_Spawn::Update()
{
   if(m_ptSpawn)
   {
      m_pcbActive->Check(m_ptSpawn->fActive());
      m_pedTitle->SetText(m_ptSpawn->pclTitle());
      m_pedTabGroup->SetText(m_ptSpawn->pclTabGroup());
      m_pedCaptureUntil->SetText(m_ptSpawn->pclCaptureUntil());
      m_pcbOnlyChildrenDuringCapture->Check(m_ptSpawn->fOnlyChildrenDuringCapture());
      m_pcbClear->Check(m_ptSpawn->fClear());
      m_pcbShowTab->Check(m_ptSpawn->fShowTab());
      m_pcbGagLog->Check(m_ptSpawn->fGagLog());
      m_pcbCopy->Check(m_ptSpawn->fCopy());
   }
   else
   {
      m_pcbActive->Check(false);
      m_pedTitle->SetText("");
      m_pedTabGroup->SetText("");
      m_pedCaptureUntil->SetText("");
      m_pcbOnlyChildrenDuringCapture->Check(false);
      m_pcbClear->Check(false);
      m_pcbShowTab->Check(false);
      m_pcbGagLog->Check(false);
      m_pcbCopy->Check(false);
   }
   UpdateEnabled();
}

LRESULT Dlg_Trigger_Spawn::On(const Msg::Create &msg)
{
   m_pcbActive=m_layout.CreateCheckBox(-1, "Active");
   m_pedTitle=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
   m_pedTitle->SetCueBanner("Leave blank to capture to main output");
   m_pedTabGroup=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
   m_pedCaptureUntil=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
   m_pcbOnlyChildrenDuringCapture=m_layout.CreateCheckBox(-1, "During capture, process only children of this trigger");
   m_pcbClear=m_layout.CreateCheckBox(-1, "Clear spawn before displaying");
   m_pcbShowTab=m_layout.CreateCheckBox(-1, "If in tab group, switch to this tab");
   m_pcbGagLog=m_layout.CreateCheckBox(-1, "Gag from log");
   m_pcbCopy=m_layout.CreateCheckBox(-1, "Copy line instead of move (Original line is still in main output)");

   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);
   *pGV >> AL::Style::Attach_Bottom;
   *pGV << m_pcbActive;

   *pGV << m_layout.CreateGroup_Horizontal(m_layout.CreateStatic("Window Title:"), m_pedTitle);
   *pGV << m_layout.CreateGroup_Horizontal(m_layout.CreateStatic("Add to tab group (optional):"), m_pedTabGroup);
   *pGV << m_layout.CreateGroup_Horizontal(m_layout.CreateStatic("Capture Until (optional):"), m_pedCaptureUntil);
   *pGV << m_pcbOnlyChildrenDuringCapture << m_pcbClear << m_pcbShowTab << m_pcbGagLog << m_pcbCopy;
   *pGV << m_layout.CreateStatic("\nTo redirect only this trigger's line, leave 'Capture Until' blank."
                                 "\nCapture Until is always a case-sensitive regular expression \n"
                                 "Use the /capturecancel command to stop if things go wrong.");
   Update();
   return msg.Success();
}

LRESULT Dlg_Trigger_Spawn::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==BN_CLICKED && msg.wndCtl()==*m_pcbActive)
      UpdateEnabled();

   return msg.Success();
}

//
// Stat Tab -----------------------------------------------------------------
//

struct Dlg_Trigger_Stat : ITriggerDialog
{
   Dlg_Trigger_Stat(Window wndParent) { Create("Stat", wndParent); }
   Prototypes_ITriggerDialog;

private:
   ~Dlg_Trigger_Stat() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void Update();

   Prop::Trigger_Stat *m_pTrigger{};

   AL::Edit  *m_pedName, *m_pedPrefix, *m_pedValue;
   AL::Radio *m_pNameLeftAlign, *m_pNameCenterAlign, *m_pNameRightAlign;
   AL::CheckBox *m_pcbColor, *m_pcbFont;
   AL::Button *m_pbtColor, *m_pbtFont;
   AL::Radio *m_pRadioInt, *m_pRadioString, *m_pRadioRange;

   Color m_color;
   Prop::Font m_propFont;

   AL::Edit *m_pedTitle;

   // Int fields
   AL::CheckBox *m_pcbIntAdd;

   // Range fields
   AL::Edit *m_pRangeLower, *m_pRangeUpper;
   Color m_rangeColor;
   AL::Button *m_pbtRangeColor;
};

LRESULT Dlg_Trigger_Stat::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Trigger_Stat::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
      m_pTrigger=&ppropTrigger->propStat();
   else
      m_pTrigger=nullptr;

   Update();
}

void Dlg_Trigger_Stat::Save()
{
   if(!m_pTrigger) return;

   m_pTrigger->pclTitle(m_pedTitle->GetText());
   m_pTrigger->pclName(m_pedName->GetText());
   m_pTrigger->pclPrefix(m_pedPrefix->GetText());
   m_pTrigger->pclValue(m_pedValue->GetText());
   m_pTrigger->NameAlignment(m_pNameCenterAlign->IsChecked()*1 + m_pNameRightAlign->IsChecked()*2);
   m_pTrigger->fUseColor(m_pcbColor->IsChecked());
   m_pTrigger->clrColor(m_color);
   m_pTrigger->fUseFont(m_pcbFont->IsChecked());
   m_pTrigger->propFont()=m_propFont;
   m_pTrigger->Type(m_pRadioString->IsChecked()*1 + m_pRadioRange->IsChecked()*2);
   m_pTrigger->propRange().pclLower(m_pRangeLower->GetText());
   m_pTrigger->propRange().pclUpper(m_pRangeUpper->GetText());
   m_pTrigger->propRange().clrColor(m_rangeColor);
   m_pTrigger->propInt().fAdd(m_pcbIntAdd->IsChecked());
}

void Dlg_Trigger_Stat::Update()
{
   if(m_pTrigger)
   {
      m_pedTitle->SetText(m_pTrigger->pclTitle());
      m_pedName->SetText(m_pTrigger->pclName());
      m_pedPrefix->SetText(m_pTrigger->pclPrefix());
      m_pedValue->SetText(m_pTrigger->pclValue());
      m_pNameLeftAlign->SetCheck(m_pTrigger->NameAlignment()==0);
      m_pNameCenterAlign->SetCheck(m_pTrigger->NameAlignment()==1);
      m_pNameRightAlign->SetCheck(m_pTrigger->NameAlignment()==2);
      m_pcbColor->SetCheck(m_pTrigger->fUseColor());
      m_color=m_pTrigger->clrColor();
      m_pcbFont->SetCheck(m_pTrigger->fUseFont());
      m_propFont=m_pTrigger->propFont();

      m_pRadioInt->SetCheck(m_pTrigger->Type()==0);
      m_pRadioString->SetCheck(m_pTrigger->Type()==1);
      m_pRadioRange->SetCheck(m_pTrigger->Type()==2);

      m_pRangeLower->SetText(m_pTrigger->propRange().pclLower());
      m_pRangeUpper->SetText(m_pTrigger->propRange().pclUpper());
      m_rangeColor=m_pTrigger->propRange().clrColor();

      m_pcbIntAdd->SetCheck(m_pTrigger->propInt().fAdd());
   }
   else
      m_pedName->SetText("");
   UpdateEnabled();
}

void Dlg_Trigger_Stat::UpdateEnabled()
{
   bool fEnable=m_pTrigger && m_fEnabled;
   EnableWindows(fEnable, *m_pedName, *m_pNameLeftAlign, *m_pNameCenterAlign, *m_pNameRightAlign, *m_pedPrefix, *m_pedValue, *m_pcbColor, *m_pcbFont, *m_pedTitle, *m_pRadioInt, *m_pRadioString, *m_pRadioRange);
   EnableWindows(fEnable & m_pcbColor->IsChecked(), *m_pbtColor);
   EnableWindows(fEnable & m_pcbFont->IsChecked(), *m_pbtFont);
   bool fRadioInt=fEnable&m_pRadioInt->IsChecked();
   EnableWindows(fRadioInt, *m_pcbIntAdd);
   bool fRadioRange=fEnable&m_pRadioRange->IsChecked();
   EnableWindows(fRadioRange, *m_pRangeLower, *m_pRangeUpper, *m_pbtRangeColor);
}

LRESULT Dlg_Trigger_Stat::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   {
      auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      m_pedPrefix=m_layout.CreateEdit(-1, int2(5, 1), ES_AUTOHSCROLL); m_pedPrefix->weight(1);
      m_pedName=m_layout.CreateEdit(-1, int2(10, 1), ES_AUTOHSCROLL); m_pedName->weight(2);

      m_pedValue=m_layout.CreateEdit(-1, int2(10, 1), ES_AUTOHSCROLL); m_pedValue->weight(2);
      *pGH << m_layout.CreateStatic("Invisible Prefix:") << m_pedPrefix << m_layout.CreateStatic("Name:") << m_pedName << m_layout.CreateStatic("Value:") << m_pedValue;
   }

   {
      auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      m_pNameLeftAlign=m_layout.CreateRadio(-1, "Left", true);
      m_pNameCenterAlign=m_layout.CreateRadio(-1, "Center");
      m_pNameRightAlign=m_layout.CreateRadio(-1, "Right");
      *pGH << m_layout.CreateStatic("Name alignment:") << m_pNameLeftAlign << m_pNameCenterAlign << m_pNameRightAlign;
   }

   {
      auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      m_pcbColor=m_layout.CreateCheckBox(-1, "Custom Color");
      m_pbtColor=m_layout.CreateButton(-1, "Choose"); m_pbtColor->weight(0);
      *pGH << m_pcbColor << m_pbtColor;
   }

   {
      auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      m_pcbFont=m_layout.CreateCheckBox(-1, "Custom Font");
      m_pbtFont=m_layout.CreateButton(-1, "Choose"); m_pbtFont->weight(0);
      *pGH << m_pcbFont << m_pbtFont;
   }

   {
      auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      m_pedTitle=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
      *pGH << m_layout.CreateStatic("Window Title (optional):") << m_pedTitle;
   }

   {
      auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      m_pRadioInt=m_layout.CreateRadio(-1, "Integer", true);
      m_pcbIntAdd=m_layout.CreateCheckBox(-1, "Add Value");
      *pGH << m_pRadioInt << m_pcbIntAdd;
   }

   m_pRadioString=m_layout.CreateRadio(-1, "String");
   *pGV << m_pRadioString;

   {
      auto pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      m_pRadioRange=m_layout.CreateRadio(-1, "Range");
      *pGH << m_pRadioRange;

      m_pRangeLower=m_layout.CreateEdit(-1, int2(10, 1), ES_AUTOHSCROLL);
      m_pRangeUpper=m_layout.CreateEdit(-1, int2(10, 1), ES_AUTOHSCROLL);
      *pGH << m_layout.CreateStatic("Min:") << m_pRangeLower << m_layout.CreateStatic("Max:") << m_pRangeUpper;

      m_pbtRangeColor=m_layout.CreateButton(-1, "Range Bar Color"); m_pbtRangeColor->weight(0);
      *pGH << m_pbtRangeColor;
   }

   UpdateEnabled();
   return msg.Success();
}

LRESULT Dlg_Trigger_Stat::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==BN_CLICKED && (msg.wndCtl()==*m_pRadioInt || msg.wndCtl()==*m_pRadioString || msg.wndCtl()==*m_pRadioRange || msg.wndCtl()==*m_pcbColor || msg.wndCtl()==*m_pcbFont))
      UpdateEnabled();

   if(msg.uCodeNotify()==BN_CLICKED && msg.wndCtl()==*m_pbtColor)
      ChooseColorSimple(*this, &m_color);

   if(msg.uCodeNotify()==BN_CLICKED && msg.wndCtl()==*m_pbtFont)
      m_propFont.ChooseFont(*this);

   if(msg.uCodeNotify()==BN_CLICKED && msg.wndCtl()==*m_pbtRangeColor)
      ChooseColorSimple(*this, &m_rangeColor);

   return msg.Success();
}


//
// Toast Tab ---------------------------------------------------------------
//

struct Dlg_Trigger_Toast : ITriggerDialog
{
   Dlg_Trigger_Toast(Window wndParent) { Create(STR_Title_Toast, wndParent); }
   Prototypes_ITriggerDialog;

private:
   ~Dlg_Trigger_Toast() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;
   LRESULT On(const Msg::Create &msg);

   void Update();

   Prop::Trigger_Toast *m_ptToast{};

   AL::CheckBox *m_pcbActive;
};

LRESULT Dlg_Trigger_Toast::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create>(msg);
}

void Dlg_Trigger_Toast::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
      m_ptToast=&ppropTrigger->propToast();
   else
      m_ptToast=nullptr;

   Update();
}

void Dlg_Trigger_Toast::Save()
{
   if(!m_ptToast) return;

   m_ptToast->fActive(m_pcbActive->IsChecked());
}

void Dlg_Trigger_Toast::UpdateEnabled()
{
   EnableWindows(m_ptToast && m_fEnabled, *m_pcbActive);
}

void Dlg_Trigger_Toast::Update()
{
   if(m_ptToast)
      m_pcbActive->Check(m_ptToast->fActive());
   UpdateEnabled();
}

LRESULT Dlg_Trigger_Toast::On(const Msg::Create &msg)
{
   auto *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   m_pcbActive=m_layout.CreateCheckBox(-1, STR_ToastOnTrigger); *pGV << m_pcbActive;

   Update();
   return msg.Success();
}

//
// Notify Tab ---------------------------------------------------------------
//

struct Dlg_Trigger_Activity : ITriggerDialog
{
   Dlg_Trigger_Activity(Window wndParent) { Create(STR_Title_Activity, wndParent); }

private:
   Prototypes_ITriggerDialog;
   ~Dlg_Trigger_Activity() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;
   LRESULT On(const Msg::Create &msg);

   void SetEnabled(bool fEnabled) override;
   void Update();

   Prop::Trigger_Activate *m_ptActivate{};

   AL::CheckBox *m_pcbActive;
   AL::CheckBox *m_pcbImportantActivity;
   AL::CheckBox *m_pcbActivity, *m_pcbNoActivity;

   Dlg_Trigger_Toast *m_pToast;
};

LRESULT Dlg_Trigger_Activity::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create>(msg);
}

void Dlg_Trigger_Activity::SetTrigger(Prop::Trigger *ppropTrigger)
{
   m_pToast->SetTrigger(ppropTrigger);

   if(ppropTrigger)
      m_ptActivate=&ppropTrigger->propActivate();
   else
      m_ptActivate=nullptr;

   Update();
}

void Dlg_Trigger_Activity::Save()
{
   m_pToast->Save();

   if(!m_ptActivate) return;

   m_ptActivate->fActive(m_pcbActive->IsChecked());
   m_ptActivate->fImportantActivity(m_pcbImportantActivity->IsChecked());
   m_ptActivate->fActivity(m_pcbActivity->IsChecked());
   m_ptActivate->fNoActivity(m_pcbNoActivity->IsChecked());
}

void Dlg_Trigger_Activity::SetEnabled(bool fEnabled)
{
   __super::SetEnabled(fEnabled);
   m_pToast->SetEnabled(fEnabled);
}

void Dlg_Trigger_Activity::UpdateEnabled()
{
   EnableWindows(m_ptActivate && m_fEnabled, *m_pcbActive, *m_pcbImportantActivity, *m_pcbActivity, *m_pcbNoActivity);
}

void Dlg_Trigger_Activity::Update()
{
   if(m_ptActivate)
   {
      m_pcbActive->Check(m_ptActivate->fActive());
      m_pcbImportantActivity->Check(m_ptActivate->fImportantActivity());
      m_pcbActivity->Check(m_ptActivate->fActivity());
      m_pcbNoActivity->Check(m_ptActivate->fNoActivity());
   }
   UpdateEnabled();
}

LRESULT Dlg_Trigger_Activity::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   m_pcbImportantActivity=m_layout.CreateCheckBox(-1, "Show as important activity on window tab (the red box)"); *pGV << m_pcbImportantActivity;
   m_pcbActive=m_layout.CreateCheckBox(-1, STR_ActivateOnTrigger); *pGV << m_pcbActive;
   m_pcbActivity=m_layout.CreateCheckBox(-1, "Show as activity (window will blink)"); *pGV << m_pcbActivity;
   m_pcbNoActivity=m_layout.CreateCheckBox(-1, "Don't show as activity (window won't blink)"); *pGV << m_pcbNoActivity;
   Update();

   m_pToast=new Dlg_Trigger_Toast(*this);
   *pGV << m_pToast;
   return msg.Success();
}

//
// Speech Tab -----------------------------------------------------------------
//

struct Dlg_Trigger_Speech : ITriggerDialog
{
   Dlg_Trigger_Speech(Window wndParent) { Create("Speech", wndParent); }
   Prototypes_ITriggerDialog;

private:
   ~Dlg_Trigger_Speech() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void Update();

   Prop::Trigger_Speech *m_pTrigger{};

   AL::CheckBox *m_pcbSpeech;
   AL::CheckBox *m_pcbWholeLine;
   AL::Edit     *m_pedText;
};

LRESULT Dlg_Trigger_Speech::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Trigger_Speech::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
      m_pTrigger=&ppropTrigger->propSpeech();
   else
      m_pTrigger=nullptr;

   Update();
}

void Dlg_Trigger_Speech::Save()
{
   if(!m_pTrigger) return;

   m_pTrigger->fActive(m_pcbSpeech->IsChecked());
   m_pTrigger->fWholeLine(m_pcbWholeLine->IsChecked());
   m_pTrigger->pclSay(m_pedText->GetText());
}

void Dlg_Trigger_Speech::UpdateEnabled()
{
   bool fEnable=m_pTrigger && m_fEnabled;
   EnableWindows(fEnable, *m_pcbSpeech);
   fEnable&=m_pcbSpeech->IsChecked();
   EnableWindows(fEnable, *m_pcbWholeLine);
   EnableWindows(fEnable && !m_pcbWholeLine->IsChecked(), *m_pedText);
}

void Dlg_Trigger_Speech::Update()
{
   if(m_pTrigger)
   {
      m_pedText->SetText(m_pTrigger->pclSay());
      m_pcbSpeech->Check(m_pTrigger->fActive());
      m_pcbWholeLine->Check(m_pTrigger->fWholeLine());
   }
   else
      m_pedText->SetText("");
   UpdateEnabled();
}

LRESULT Dlg_Trigger_Speech::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   m_pcbSpeech=m_layout.CreateCheckBox(-1, "Speak");
   m_pcbWholeLine=m_layout.CreateCheckBox(-1, "Say whole line");

   AL::Static *pText=m_layout.CreateStatic("Say only this:");
   *pGV << m_pcbSpeech << m_pcbWholeLine << pText;

   m_pedText=m_layout.CreateEdit(-1, int2(30, 1), ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL); *pGV << m_pedText;
   m_pedText->LimitText(Prop::Trigger_Filter::pclReplace_MaxLength());

   *pGV << m_layout.CreateStatic("You can use SAPI XML tags in the text, like\n<emph> <spell> <volume level='30'> <rate speed='5'> and more!\nTo use tags, enclose them in a <sapi> </sapi> block\nSearch online for 'SAPI XML'.");

   UpdateEnabled();
   return msg.Success();
}

LRESULT Dlg_Trigger_Speech::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==BN_CLICKED && (msg.wndCtl()==*m_pcbSpeech || msg.wndCtl()==*m_pcbWholeLine))
      UpdateEnabled();

   return msg.Success();
}

//
// Sound Tab -----------------------------------------------------------------
//

struct Dlg_Trigger_Sound : ITriggerDialog
{
   Dlg_Trigger_Sound(Window wndParent) { Create(STR_Title_Sound, wndParent); }
   Prototypes_ITriggerDialog;

private:
   ~Dlg_Trigger_Sound() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void SetEnabled(bool fEnabled) override;
   void Update();

   Prop::Trigger_Sound *m_ptSound{};

   AL::Edit *m_pedFilename;
   AL::CheckBox *m_pcbActive;
   AL::Button *m_pbtBrowse, *m_pbtPlay;

   Dlg_Trigger_Speech *m_pSpeech;
};

LRESULT Dlg_Trigger_Sound::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Trigger_Sound::SetTrigger(Prop::Trigger *ppropTrigger)
{
   m_pSpeech->SetTrigger(ppropTrigger);

   if(ppropTrigger)
      m_ptSound=&ppropTrigger->propSound();
   else
      m_ptSound=nullptr;

   Update();
}

void Dlg_Trigger_Sound::Save()
{
   m_pSpeech->Save();

   if(!m_ptSound) return;

   m_ptSound->fActive(m_pcbActive->IsChecked());
   m_ptSound->pclSound(m_pedFilename->GetText());
}

void Dlg_Trigger_Sound::SetEnabled(bool fEnabled)
{
   __super::SetEnabled(fEnabled);
   m_pSpeech->SetEnabled(fEnabled);
}

void Dlg_Trigger_Sound::UpdateEnabled()
{
   bool fEnabled=m_ptSound && m_fEnabled;
   EnableWindows(fEnabled, *m_pcbActive);
   fEnabled&=m_pcbActive->IsChecked();
   EnableWindows(fEnabled, *m_pbtBrowse, *m_pbtPlay, *m_pedFilename);
}

void Dlg_Trigger_Sound::Update()
{
   if(m_ptSound)
   {
      m_pedFilename->SetText(m_ptSound->pclSound());
      m_pcbActive->Check(m_ptSound->fActive());
   }
   else
      m_pedFilename->SetText("");
   UpdateEnabled();
}

//
// Window Messages
//
LRESULT Dlg_Trigger_Sound::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   m_pcbActive=m_layout.CreateCheckBox(-1, STR_PlaySound);
   *pGV << m_pcbActive;

   {
      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);
      pGH->MatchBaseline(false); *pGH >> AL::Style::Attach_Vert;
      AL::Static *pStatic=m_layout.CreateStatic(STR_SoundFile); pStatic->weight(100);
      m_pbtBrowse=m_layout.CreateButton(-1, STR_Browse___);
      m_pbtPlay=m_layout.CreateButton(-1, STR_Play);

      *pGH << pStatic << m_pbtBrowse << m_pbtPlay;
   }

   m_pedFilename=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL); *pGV << m_pedFilename;
   m_pedFilename->weight(0);
   m_pedFilename->LimitText(Prop::Trigger_Sound::pclSound_MaxLength());
   UpdateEnabled();

   m_pSpeech=new Dlg_Trigger_Speech(*this);
   *pGV << m_pSpeech;
   return msg.Success();
}

LRESULT Dlg_Trigger_Sound::On(const Msg::Command &msg)
{
   FixedStringBuilder<256> fileName;

   if(msg.uCodeNotify()==BN_CLICKED)
   {
      if(msg.wndCtl()==*m_pbtBrowse)
      {
         File::Chooser cf;
         cf.SetTitle(STR_SoundFile);
         cf.SetFilter(STR_SoundFileFilter, 0);

         fileName(m_pedFilename->GetText());

         if(cf.Choose(*this, fileName, false))
            m_pedFilename->SetText(fileName);
      }
      else if(msg.wndCtl()==*m_pbtPlay)
      {
         PlaySound(m_pedFilename->GetText());
      }
      else if(msg.wndCtl()==*m_pcbActive)
      {
         UpdateEnabled();
      }
   }

   return msg.Success();
}

//
// Script Tab ---------------------------------------------------------------
//

struct Dlg_Trigger_Script : ITriggerDialog
{
   Dlg_Trigger_Script(Window wndParent) { Create(STR_Title_Script, wndParent); }

private:
   Prototypes_ITriggerDialog;
   ~Dlg_Trigger_Script() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void Update();

   Prop::Trigger_Script *m_ptScript{};

   AL::CheckBox *m_pcbActive;
   AL::Edit *m_pedText;
};

LRESULT Dlg_Trigger_Script::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Trigger_Script::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
      m_ptScript=&ppropTrigger->propScript();
   else
      m_ptScript=nullptr;

   Update();
}

void Dlg_Trigger_Script::Save()
{
   if(!m_ptScript) return;

   m_ptScript->fActive(m_pcbActive->IsChecked());
   m_ptScript->pclFunction(m_pedText->GetText());
}

void Dlg_Trigger_Script::UpdateEnabled()
{
   bool fEnable=m_ptScript && m_fEnabled;
   EnableWindows(fEnable, *m_pcbActive);
   fEnable&=m_pcbActive->IsChecked();
   EnableWindows(fEnable, *m_pedText);
}

void Dlg_Trigger_Script::Update()
{
   if(m_ptScript)
   {
      m_pcbActive->Check(m_ptScript->fActive());
      m_pedText->SetText(m_ptScript->pclFunction());
   }
   else
      m_pedText->SetText("");

   UpdateEnabled();
}

LRESULT Dlg_Trigger_Script::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   m_pcbActive=m_layout.CreateCheckBox(-1, STR_Enabled);
   *pGV << m_pcbActive;

   *pGV << m_layout.CreateStatic(STR_FunctionToCall);

   m_pedText=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
   m_pedText->weight(0);
   m_pedText->LimitText(Prop::Trigger_Script::pclFunction_MaxLength());
   *pGV << m_pedText;

   *pGV << m_layout.CreateStatic(STR_FunctionExample);

   Update();
   return msg.Success();
}

LRESULT Dlg_Trigger_Script::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==BN_CLICKED && (msg.wndCtl()==*m_pcbActive))
      UpdateEnabled();

   return msg.Success();
}

//
// Send Tab -----------------------------------------------------------------
//

struct Dlg_Trigger_Send : ITriggerDialog
{
   Dlg_Trigger_Send(Window wndParent) { Create(STR_Title_Send, wndParent); }

private:
   Prototypes_ITriggerDialog;
   ~Dlg_Trigger_Send() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void Update();

   Prop::Trigger_Send *m_ptSend{};

   AL::CheckBox *m_pcbActive;
   AL::CheckBox *m_pcbSendOnClick;
   AL::Edit *m_pedCaptureIndex;
   AL::CheckBox *m_pcbExpandVariables;
   AL::Edit *m_pedText;
};

LRESULT Dlg_Trigger_Send::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Trigger_Send::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
      m_ptSend=&ppropTrigger->propSend();
   else
      m_ptSend=nullptr;

   Update();
}

void Dlg_Trigger_Send::Save()
{
   if(!m_ptSend) return;

   m_ptSend->fActive(m_pcbActive->IsChecked());
   m_ptSend->pclSend(m_pedText->GetText());
   m_ptSend->fSendOnClick(m_pcbSendOnClick->IsChecked());
   if(int value;m_pedCaptureIndex->Get(value))
      m_ptSend->CaptureIndex(value);
   m_ptSend->fExpandVariables(m_pcbExpandVariables->IsChecked());
}

void Dlg_Trigger_Send::UpdateEnabled()
{
   bool fEnabled=m_ptSend && m_fEnabled;
   EnableWindows(fEnabled, *m_pcbActive);
   fEnabled&=m_pcbActive->IsChecked();
   EnableWindows(fEnabled, *m_pcbSendOnClick, *m_pcbExpandVariables, *m_pedText);
   EnableWindows(fEnabled && m_pcbSendOnClick->IsChecked(), *m_pedCaptureIndex);
}

void Dlg_Trigger_Send::Update()
{
   if(m_ptSend)
   {
      m_pcbActive->Check(m_ptSend->fActive());
      m_pedText->SetText(m_ptSend->pclSend());
      m_pcbSendOnClick->Check(m_ptSend->fSendOnClick());
      m_pedCaptureIndex->Set(m_ptSend->CaptureIndex());
      m_pcbExpandVariables->Check(m_ptSend->fExpandVariables());
   }
   else
      m_pedText->SetText("");
   UpdateEnabled();
}

LRESULT Dlg_Trigger_Send::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   m_pcbActive=m_layout.CreateCheckBox(-1, STR_SendText);
   m_pcbSendOnClick=m_layout.CreateCheckBox(-1, "Send on click of matched text");

   AL::Static *pText=m_layout.CreateStatic(STR_TextToSend);
   *pGV << m_pcbActive << m_pcbSendOnClick;

   {
      auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);
      m_pedCaptureIndex=m_layout.CreateEdit(-1, int2(5, 1), ES_NUMBER);
      m_pedCaptureIndex->weight(0);
      *pGH << m_layout.CreateStatic("RegEx Capture Index To Apply To: ") << m_pedCaptureIndex;
   }

   m_pcbExpandVariables=m_layout.CreateCheckBox(-1, "Expand %variables%");
   *pGV << m_pcbExpandVariables;

   *pGV << pText;

   m_pedText=m_layout.CreateEdit(-1, int2(30, 1), ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL); *pGV << m_pedText;
   m_pedText->LimitText(Prop::Trigger_Send::pclSend_MaxLength());

   Update();
   return msg.Success();
}

LRESULT Dlg_Trigger_Send::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==BN_CLICKED && (msg.wndCtl()==*m_pcbActive || msg.wndCtl()==*m_pcbSendOnClick))
      UpdateEnabled();

   return msg.Success();
}

//
// Gag Tab --------------------------------------------------------------------
//

struct Dlg_Trigger_Gag : ITriggerDialog
{
   Dlg_Trigger_Gag(Window wndParent) { Create(STR_Title_Gag, wndParent); }
   Prototypes_ITriggerDialog;

private:
   ~Dlg_Trigger_Gag() { }

   friend TWindowImpl;
   LRESULT WndProc(const Message &msg) override;

   // Window Messages
   LRESULT On(const Msg::Create &msg);

   void Update();

   Prop::Trigger_Gag *m_ptGag{};

   AL::CheckBox *m_pcbGag, *m_pcbLog;
};

LRESULT Dlg_Trigger_Gag::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create>(msg);
}

void Dlg_Trigger_Gag::SetTrigger(Prop::Trigger *ppropTrigger)
{
   if(ppropTrigger)
      m_ptGag=&ppropTrigger->propGag();
   else
      m_ptGag=nullptr;

   Update();
}

void Dlg_Trigger_Gag::Save()
{
   if(!m_ptGag) return;

   m_ptGag->fActive(m_pcbGag->IsChecked());
   m_ptGag->fLog(m_pcbLog->IsChecked());
}

void Dlg_Trigger_Gag::UpdateEnabled()
{
   EnableWindows(m_ptGag && m_fEnabled, *m_pcbGag, *m_pcbLog);
}

void Dlg_Trigger_Gag::Update()
{
   if(m_ptGag)
   {
      m_pcbGag->Check(m_ptGag->fActive());
      m_pcbLog->Check(m_ptGag->fLog());
   }

   UpdateEnabled();
}

LRESULT Dlg_Trigger_Gag::On(const Msg::Create &msg)
{
   m_pcbGag=m_layout.CreateCheckBox(-1, STR_GagThisLine);
   m_pcbLog=m_layout.CreateCheckBox(-1, STR_GagLineInLog);

   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   *pGV >> AL::Style::Attach_Bottom;
   *pGV << m_pcbGag << m_pcbLog;

   Update();
   return msg.Success();
}

//
// Filter Tab -----------------------------------------------------------------
//

struct Dlg_Trigger_Filter : ITriggerDialog
{
   Dlg_Trigger_Filter(Window wndParent) { Create(STR_Title_Filter, wndParent); }

private:
   Prototypes_ITriggerDialog;
   ~Dlg_Trigger_Filter() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void SetEnabled(bool fEnabled) override;
   void Update();

   Prop::Trigger_Filter *m_ptFilter{};
   Prop::Trigger_Avatar *m_ptAvatar{};

   AL::CheckBox *m_pcbActive;
   AL::CheckBox *m_pcbHTML;
   AL::Edit     *m_pedText;

   AL::Edit     *m_pedAvatarURL;

   Dlg_Trigger_Gag *m_pGag;
};

LRESULT Dlg_Trigger_Filter::WndProc(const Message &msg)
{
   return Dispatch<ITriggerDialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Trigger_Filter::SetTrigger(Prop::Trigger *ppropTrigger)
{
   m_pGag->SetTrigger(ppropTrigger);

   if(ppropTrigger)
   {
      m_ptFilter=&ppropTrigger->propFilter();
      m_ptAvatar=&ppropTrigger->propAvatar();
   }
   else
   {
      m_ptFilter=nullptr;
      m_ptAvatar=nullptr;
   }

   Update();
}

void Dlg_Trigger_Filter::Save()
{
   m_pGag->Save();

   if(m_ptFilter)
   {
      m_ptFilter->fActive(m_pcbActive->IsChecked());
      m_ptFilter->fHTML(m_pcbHTML->IsChecked());
      m_ptFilter->pclReplace(m_pedText->GetText());
   }

   if(m_ptAvatar)
      m_ptAvatar->pclURL(m_pedAvatarURL->GetText());
}

void Dlg_Trigger_Filter::SetEnabled(bool fEnabled)
{
   __super::SetEnabled(fEnabled);
   m_pGag->SetEnabled(fEnabled);
}

void Dlg_Trigger_Filter::UpdateEnabled()
{
   bool fEnabled=m_ptFilter && m_fEnabled;
   EnableWindows(fEnabled, *m_pcbActive);
   fEnabled&=m_pcbActive->IsChecked();
   EnableWindows(fEnabled, *m_pedText, *m_pcbHTML);
}

void Dlg_Trigger_Filter::Update()
{
   if(m_ptFilter)
   {
      m_pedText->SetText(m_ptFilter->pclReplace());
      m_pcbActive->Check(m_ptFilter->fActive());
      m_pcbHTML->Check(m_ptFilter->fHTML());
   }
   else
      m_pedText->SetText("");

   if(m_ptAvatar)
   {
      m_pedAvatarURL->SetText(m_ptAvatar->pclURL());
   }
   else
      m_pedAvatarURL->SetText("");
   UpdateEnabled();
}

//
// Window Messages
//
LRESULT Dlg_Trigger_Filter::On(const Msg::Create &msg)
{
   auto *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV); //*pGV >> AL::Style::Attach_Bottom;

   m_pcbActive=CreateHorizontalCheckbox(m_layout, *pGV, -1, STR_FilterText);
   m_pcbHTML=m_layout.CreateCheckBox(-1, STR_FilterHTML);

   AL::Static *pText=m_layout.CreateStatic(STR_TextFilteredWith);
   *pGV << m_pcbHTML << pText;

   m_pedText=m_layout.CreateEdit(-1, int2(30, 1), ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL); *pGV << m_pedText;
   m_pedText->LimitText(Prop::Trigger_Filter::pclReplace_MaxLength());

   auto *p_avatar_group=m_layout.CreateGroupBox("Avatars (image left of text)"); *pGV << p_avatar_group; p_avatar_group->weight(0);

   {
      auto *pGV=m_layout.CreateGroup_Vertical(); p_avatar_group->SetChild(pGV);
      {
         auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
         m_pedAvatarURL=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
         *pGH << m_layout.CreateStatic("URL ") << m_pedAvatarURL;
      }
      *pGV << m_layout.CreateStatic("NOTE: URL can also be a local path");
   }

   Update();

   auto *p_gag_group=m_layout.CreateGroupBox("Gagging"); *pGV << p_gag_group; p_gag_group->weight(0);
   m_pGag=new Dlg_Trigger_Gag(*this); p_gag_group->SetChild(m_pGag);
   return msg.Success();
}

LRESULT Dlg_Trigger_Filter::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()==BN_CLICKED && (msg.wndCtl()==*m_pcbActive))
      UpdateEnabled();

   return msg.Success();
}

//
// Triggers Dialog ------------------------------------------------------------
//

struct Dlg_Trigger : IPropDlg, AL::Tab::INotify
{
   Dlg_Trigger(Window wndParent, ITreeView *pITreeView);

   bool SetProp(IPropTreeItem *pti, bool fKeepChanges) override;
   bool SetTrigger(Prop::Trigger *ppropTrigger, bool fKeepChanges);
   // Sets a new trigger and saves the previous
   // does nothing when the same trigger is set twice
   bool SetTriggers(Prop::Triggers *ppropNewTriggers, bool fKeepChanges);

private:
   ~Dlg_Trigger() { }

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   // AL::Tab::INotify
   void TabChange(unsigned int iTabOld);

   bool GetFindString(Prop::FindString &fs);

   void UpdateEnabled();
   void UpdateExample();

   enum TAB
   {
      TAB_APPEARANCE,
      TAB_PARAGRAPH,
      TAB_SOUND,
      TAB_SPAWN,
      TAB_STAT,
      TAB_ACTIVITY,
      TAB_SEND,
      TAB_FILTER,
      TAB_SCRIPT,
      TAB_MAX
   };

   enum
   {
      IDC_CHECK_AWAYPRESENT = 100,
      IDC_EDIT_MATCHAROO,
      IDC_EDIT_EXAMPLE,
      IDC_BUTTON_REGEX101,
      IDC_RADIO_AWAY,
      IDC_RADIO_PRESENT,
      IDC_CHECK_DISABLED,
      IDC_CHECK_REGULAREXPRESSION,
      IDC_CHECK_MATCHCASE,
      IDC_CHECK_WHOLEWORD,
      IDC_CHECK_LINESTARTSWITH,
      IDC_CHECK_LINEENDSWITH,
      IDC_CHECK_ONCEPERLINE,
      IDC_CHECK_COOLDOWN,
      IDC_CHECK_MULTILINE,
   };

   AL::CheckBox *m_pcbActive;
   AL::Edit *m_pedDescription;

   MatcharooControls m_matcharoo;
   Time::Timer m_timerExample{[this]() { UpdateExample(); }};

   AL::CheckBox *m_pcbRegularExpression;

   AL::CheckBox *m_pcbMatchCase, *m_pcbWholeWord;
   AL::CheckBox *m_pcbStartsWith, *m_pcbEndsWith;
   AL::CheckBox *m_pcbStopProcessing, *m_pcbOncePerLine;
   AL::Radio *m_pcbEnabled, *m_pcbFolder;
   AL::CheckBox *m_pcbAwayPresent, *m_pcbAwayPresentOnce;
   AL::Radio *m_pcbAway, *m_pcbPresent;
   AL::CheckBox *m_pcbCooldown;
   AL::Edit *m_pedCooldownTime;
   AL::CheckBox *m_pcbMultiline;
   AL::Edit *m_pedMultilineLines, *m_pedMultilineTime;
   AL::Tab *m_ptbTypes;

   ITreeView      *m_pITreeView;
   std::array<ITriggerDialog*, TAB_MAX> m_pIDialogs;

   Prop::Trigger  *m_ppropTrigger{};  // Currently Selected Trigger
   Prop::Triggers *m_ppropTriggers{}; // Currently Selected Scope
};

LRESULT Dlg_Trigger::WndProc(const Message &msg)
{
   return Dispatch<IPropDlg, Msg::Create, Msg::Command>(msg);
}

Dlg_Trigger::Dlg_Trigger(Window wndParent, ITreeView *pITreeView)
:  m_pITreeView(pITreeView)
{
   Create("", wndParent);
}

bool Dlg_Trigger::GetFindString(Prop::FindString &fs)
{
   bool fChanged=false;

   auto strMatch=m_matcharoo.m_pedMatchText->GetText();

   fChanged=fs.pclMatchText()!=strMatch;

   fs.pclMatchText(std::move(strMatch));
   fs.fRegularExpression(m_pcbRegularExpression->IsChecked()); fs.mp_regex_cache=nullptr;
   fs.fMatchCase(m_pcbMatchCase->IsChecked());
   fs.fWholeWord(m_pcbWholeWord->IsChecked());
   fs.fStartsWith(m_pcbStartsWith->IsChecked());
   fs.fEndsWith(m_pcbEndsWith->IsChecked());

   return fChanged;
}

bool Dlg_Trigger::SetTrigger(Prop::Trigger *ppropNewTrigger, bool fKeepChanges)
{
   if(ppropNewTrigger && m_ppropTrigger==ppropNewTrigger)
      return false; // Nothing to do

   bool fUpdate=false;

   if(m_ppropTrigger && fKeepChanges)
   {
      auto strDescription=m_pedDescription->GetText();

      fUpdate|=GetFindString(m_ppropTrigger->propFindString());
      fUpdate|=m_ppropTrigger->pclDescription()!=strDescription || m_ppropTrigger->fDisabled()!=m_pcbFolder->IsChecked();

      m_ppropTrigger->fDisabled(m_pcbFolder->IsChecked());
      m_ppropTrigger->pclDescription(std::move(strDescription));

      auto strExample=m_matcharoo.m_pedExample->GetText();
      m_ppropTrigger->pclExample(strExample!=m_matcharoo.c_strExample ? strExample : ConstString{});

      m_ppropTrigger->fOncePerLine(m_pcbOncePerLine->IsChecked());
      m_ppropTrigger->fStopProcessing(m_pcbStopProcessing->IsChecked());

      m_ppropTrigger->fAwayPresent(m_pcbAwayPresent->IsChecked());
      m_ppropTrigger->fAwayPresentOnce(m_pcbAwayPresentOnce->IsChecked());
      m_ppropTrigger->fAway(m_pcbAway->IsChecked());

      m_ppropTrigger->fCooldown(m_pcbCooldown->IsChecked());
      if(float value;ParseTimeInSeconds(m_pedCooldownTime->GetText(), value))
         m_ppropTrigger->CooldownTime(value);

      m_ppropTrigger->fMultiline(m_pcbMultiline->IsChecked());
      if(unsigned value; m_pedMultilineLines->Get(value))
         m_ppropTrigger->Multiline_Limit(value);
      if(float value; ParseTimeInSeconds(m_pedMultilineTime->GetText(), value))
         m_ppropTrigger->Multiline_Time(value);
      else
         m_ppropTrigger->Multiline_Time(0.0f);

      for(auto *pIDialog : m_pIDialogs)
         pIDialog->Save();
   }

   m_ppropTrigger=ppropNewTrigger;

   for(auto *pIDialog : m_pIDialogs)
      pIDialog->SetTrigger(m_ppropTrigger);

   if(!m_ppropTrigger)
   {
      m_pedDescription->SetText("");
      m_matcharoo.m_pedMatchText->SetText("");
      m_matcharoo.m_pedExample->SetText("");
      UpdateEnabled();
      return fUpdate;
   }

   const Prop::FindString *pfs=&m_ppropTrigger->propFindString();

   m_pedDescription->SetText(m_ppropTrigger->pclDescription());
   m_matcharoo.m_pedMatchText->SetText(pfs->pclMatchText());
   m_matcharoo.m_pedExample->SetText(m_ppropTrigger->pclExample() ? ConstString{m_ppropTrigger->pclExample()} : m_matcharoo.c_strExample);

   m_pcbRegularExpression->Check(pfs->fRegularExpression());
   m_pcbMatchCase->Check(pfs->fMatchCase());
   m_pcbWholeWord->Check(pfs->fWholeWord());
   m_pcbStartsWith->Check(pfs->fStartsWith());
   m_pcbEndsWith->Check(pfs->fEndsWith());
   m_pcbOncePerLine->Check(m_ppropTrigger->fOncePerLine());
   m_pcbEnabled->Check(!m_ppropTrigger->fDisabled());
   m_pcbFolder->Check(m_ppropTrigger->fDisabled());
   m_pcbAwayPresent->Check(m_ppropTrigger->fAwayPresent());
   m_pcbAwayPresentOnce->Check(m_ppropTrigger->fAwayPresentOnce());
   m_pcbAway->Check(m_ppropTrigger->fAway());
   m_pcbPresent->Check(!m_ppropTrigger->fAway());
   m_pcbCooldown->Check(m_ppropTrigger->fCooldown());
   m_pedCooldownTime->SetText(Strings::TValue<float>(m_ppropTrigger->CooldownTime()));
   m_pcbMultiline->Check(m_ppropTrigger->fMultiline());
   m_pedMultilineLines->Set(m_ppropTrigger->Multiline_Limit());
   if(m_ppropTrigger->Multiline_Time()!=0.0f)
      m_pedMultilineTime->SetText(Strings::TValue<float>(m_ppropTrigger->Multiline_Time()));
   else
      m_pedMultilineTime->SetText({});

   m_pcbStopProcessing->Check(m_ppropTrigger->fStopProcessing());
   UpdateEnabled();

   return fUpdate;
}

void Dlg_Trigger::UpdateEnabled()
{
   bool fEnable=m_ppropTrigger!=nullptr;
   EnableWindows(fEnable, *m_pcbEnabled, *m_pcbFolder);

   EnableWindows(fEnable, *m_pedDescription, *m_matcharoo.m_pedMatchText);
   fEnable&=m_pcbEnabled->IsChecked();

   for(auto *pIDialog : m_pIDialogs)
     pIDialog->SetEnabled(fEnable);

   EnableWindows(fEnable, *m_matcharoo.m_pedExample, *m_pcbRegularExpression, *m_pcbMatchCase);
   EnableWindows(fEnable && m_pcbRegularExpression->IsChecked(), *m_matcharoo.m_pRegex101);
   EnableWindows(fEnable && !m_pcbRegularExpression->IsChecked(), *m_pcbWholeWord, *m_pcbStartsWith, *m_pcbEndsWith);
   EnableWindows(fEnable, *m_pcbOncePerLine, *m_pcbStopProcessing, *m_pcbAwayPresent, *m_pcbCooldown);
   EnableWindows(fEnable && m_pcbCooldown->IsChecked(), *m_pedCooldownTime);
   EnableWindows(fEnable, *m_pcbMultiline);
   EnableWindows(fEnable && m_pcbMultiline->IsChecked(), *m_pedMultilineLines, *m_pedMultilineTime);

   fEnable&= m_pcbAwayPresent->IsChecked();
   m_pcbAwayPresentOnce->Enable(fEnable && m_pcbAway->IsChecked());
   EnableWindows(fEnable, *m_pcbAway, *m_pcbPresent);
}

bool Dlg_Trigger::SetTriggers(Prop::Triggers *ppropNewTriggers, bool fKeepChanges)
{
   if(m_ppropTriggers && m_ppropTriggers==ppropNewTriggers)
      return false; // Nothing to do

   bool fUpdate=false;

   if(m_ppropTriggers && fKeepChanges)
   {
      if(m_ppropTriggers->fActive()!=m_pcbActive->IsChecked())
         fUpdate=true;
      m_ppropTriggers->fActive(m_pcbActive->IsChecked());
   }

   m_ppropTriggers=ppropNewTriggers;
   m_pcbActive->Enable(m_ppropTriggers!=nullptr);

   if(m_ppropTriggers)
      m_pcbActive->Check(m_ppropTriggers->fActive());

   return fUpdate;
}

bool Dlg_Trigger::SetProp(IPropTreeItem *pti, bool fKeepChanges)
{
   return SetTriggers(pti ? pti->ppropTriggers() : nullptr, fKeepChanges) ||
          SetTrigger(pti ? pti->ppropTrigger() : nullptr, fKeepChanges);
}

void Dlg_Trigger::TabChange(unsigned int iTabOld)
{
}

//
// Window Messages
//
LRESULT Dlg_Trigger::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   {
      m_pcbEnabled=m_layout.CreateRadio(IDC_CHECK_DISABLED, "Enabled", true);
      m_pcbFolder=m_layout.CreateRadio(IDC_CHECK_DISABLED, "Folder");

      AL::Group *pGroup=m_layout.CreateGroup_Horizontal(m_pcbEnabled, m_pcbFolder); pGroup->weight(0);
      *pGV << pGroup;
   }

   m_pcbActive=m_layout.CreateCheckBox(-1, STR_ProcessTriggers);
   *pGV << m_pcbActive;

   auto *pstDescription=m_layout.CreateStatic("Description:");
   {
      m_pedDescription=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
      m_pedDescription->LimitText(Prop::Trigger::pclDescription_MaxLength());
      AL::Group_Horizontal *pGroup=m_layout.CreateGroup_Horizontal(); pGroup->weight(0);
      *pGroup << pstDescription << m_pedDescription;
      *pGV << pGroup;
   }

   m_matcharoo.Create(m_layout, *pGV, IDC_EDIT_MATCHAROO, IDC_EDIT_EXAMPLE, IDC_BUTTON_REGEX101);
   SetAllToMax(pstDescription->szMinimum().x, m_matcharoo.m_pstMatchText->szMinimum().x, m_matcharoo.m_pstExample->szMinimum().x);

   {
      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);
      {
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

         m_pcbRegularExpression=m_layout.CreateCheckBox(IDC_CHECK_REGULAREXPRESSION, STR_RegularExpression);
         m_pcbMatchCase=m_layout.CreateCheckBox(IDC_CHECK_MATCHCASE, STR_MatchCase);
         m_pcbWholeWord=m_layout.CreateCheckBox(IDC_CHECK_WHOLEWORD, STR_WholeWord);
         m_pcbStartsWith=m_layout.CreateCheckBox(IDC_CHECK_LINESTARTSWITH, STR_LineStartsWith);
         m_pcbEndsWith=m_layout.CreateCheckBox(IDC_CHECK_LINEENDSWITH, STR_LineEndsWith);

         *pGV << m_pcbRegularExpression << m_pcbMatchCase << m_pcbWholeWord << m_pcbStartsWith << m_pcbEndsWith;
      }
      {
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

         m_pcbStopProcessing=m_layout.CreateCheckBox(-1, STR_StopProcessing);
         m_pcbOncePerLine=m_layout.CreateCheckBox(IDC_CHECK_ONCEPERLINE, STR_OncePerLine);
         m_pcbAwayPresent=m_layout.CreateCheckBox(IDC_CHECK_AWAYPRESENT, STR_OnlyWhen);

         *pGV << m_pcbStopProcessing << m_pcbOncePerLine << m_pcbAwayPresent;

         {
            AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            AL::Spacer *pSpace=m_layout.CreateSpacer(); *pGH << pSpace; pSpace->GetSize().x=10;
            {
               AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
               m_pcbAway=m_layout.CreateRadio(IDC_RADIO_AWAY, STR_Away, true);
               m_pcbPresent=m_layout.CreateRadio(IDC_RADIO_PRESENT, STR_Present);
               *pGV << m_pcbAway << m_pcbPresent;
            }
            {
               AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
               m_pcbAwayPresentOnce=m_layout.CreateCheckBox(-1, STR_Once);
               *pGV << m_pcbAwayPresentOnce;
            }
         }
      }
   }
   {
      auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);

      m_pcbCooldown=m_layout.CreateCheckBox(IDC_CHECK_COOLDOWN, "Limit to once every ");
      m_pedCooldownTime=m_layout.CreateEdit(-1, int2(10, 1));
      m_pedCooldownTime->LimitText(16);
      m_pedCooldownTime->weight(0);
      *pGH << m_pcbCooldown << m_pedCooldownTime << m_layout.CreateStatic(" seconds");
   }
   {
      auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);

      m_pcbMultiline=m_layout.CreateCheckBox(IDC_CHECK_MULTILINE, "Run child triggers for next ");
      m_pedMultilineLines=m_layout.CreateEdit(-1, int2(10, 1), ES_NUMBER);
      m_pedMultilineLines->LimitText(4);
      m_pedMultilineLines->weight(0);
      m_pedMultilineLines->SetCueBanner("infinite");
      m_pedMultilineTime=m_layout.CreateEdit(-1, int2(10, 1));
      m_pedMultilineTime->LimitText(6);
      m_pedMultilineTime->weight(0);
      m_pedMultilineTime->SetCueBanner("infinite");
      *pGH << m_pcbMultiline << m_pedMultilineLines << m_layout.CreateStatic(" lines or after ") << m_pedMultilineTime << m_layout.CreateStatic(" seconds");
   }

   m_ptbTypes=m_layout.CreateTab(); *pGV << m_ptbTypes; m_ptbTypes->weight(1);

   m_pIDialogs={ new Dlg_Trigger_Appearance(*this),
                 new Dlg_Trigger_Paragraph(*this),
                 new Dlg_Trigger_Sound(*this),
                 new Dlg_Trigger_Spawn(*this),
                 new Dlg_Trigger_Stat(*this),
                 new Dlg_Trigger_Send(*this),
                 new Dlg_Trigger_Filter(*this),
                 new Dlg_Trigger_Activity(*this),
                 new Dlg_Trigger_Script(*this),
   };

   for(auto &pDialog : m_pIDialogs)
      m_ptbTypes->AddWindow(m_layout.AddObjectWindow(*pDialog, *pDialog));

   m_ptbTypes->SetVisible(TAB_APPEARANCE);
   m_ptbTypes->SetNotify(this);

   TabChange(0);

   if(g_ppropGlobal->propConnections().propTriggers().fActive())
      m_pcbActive->Check(true);

   SetTrigger(nullptr, true);
   SetTriggers(nullptr, true);
   return msg.Success();
}

LRESULT Dlg_Trigger::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_EDIT_MATCHAROO:
      case IDC_EDIT_EXAMPLE:
         if(msg.uCodeNotify()==EN_CHANGE)
            m_timerExample.Set(0.1f);
         break;

      case IDC_BUTTON_REGEX101:
         if(msg.uCodeNotify()==BN_CLICKED)
            m_matcharoo.OpenRegex101(m_pcbOncePerLine->IsChecked(), m_pcbMatchCase->IsChecked());
         break;

      case IDC_CHECK_MATCHCASE:
      case IDC_CHECK_WHOLEWORD:
      case IDC_CHECK_LINESTARTSWITH:
      case IDC_CHECK_LINEENDSWITH:
      case IDC_CHECK_REGULAREXPRESSION:
      case IDC_CHECK_ONCEPERLINE:
         if(msg.uCodeNotify()==BN_CLICKED)
            m_timerExample.Set(0.1f);

      case IDC_CHECK_AWAYPRESENT:
      case IDC_RADIO_AWAY:
      case IDC_RADIO_PRESENT:
      case IDC_CHECK_DISABLED:
      case IDC_CHECK_COOLDOWN:
      case IDC_CHECK_MULTILINE:
         if(msg.uCodeNotify()==BN_CLICKED)
            UpdateEnabled();
         break;
   }

   return msg.Success();
}

void Dlg_Trigger::UpdateExample()
{
   Prop::FindString fs;
   GetFindString(fs);
   m_matcharoo.UpdateExample(fs, m_pcbOncePerLine->IsChecked());
}

//
// PropTree Interfaces
//
struct PropTreeItem_Trigger : IPropTreeItem
{
   PropTreeItem_Trigger(Prop::Trigger *ppropTrigger) : m_ppropTrigger(ppropTrigger) { }

   ConstString Label() const override { return m_ppropTrigger->pclDescription() ? m_ppropTrigger->pclDescription() : m_ppropTrigger->propFindString().pclMatchText(); }
   bool fCanRename() const override { return true; }

   void Rename(ConstString string) override
   {
      m_ppropTrigger->pclDescription() ? m_ppropTrigger->pclDescription(string) : m_ppropTrigger->propFindString().pclMatchText(string);
   }

   bool fCanDelete() const override { return true; }
   bool fCanMove() const override { return true; }
   bool fSort() const override { return false; }
   eTIB eBitmap() const override
   {
      // If we're not disabled, show as a trigger (even if we're not processing sub items)
      if(!m_ppropTrigger->fDisabled())
         return TIB_TRIGGER;

      // If we don't have child triggers or we do and they're active, we're a folder
      if(!m_ppropTrigger->fPropTriggers() || UnconstRef(m_ppropTrigger)->propTriggers().fActive())
         return TIB_FOLDER_CLOSED;

      // We're disabled, nothing will run
      return TIB_DISABLED;
   }

   Prop::Trigger *ppropTrigger() override { return m_ppropTrigger; }
   Prop::Triggers *ppropTriggers() override { return &m_ppropTrigger->propTriggers(); }

private:

   CntPtrTo<Prop::Trigger> m_ppropTrigger;
};

static ITreeView *s_tree{};

struct PropTree : IPropTree
{
   ~PropTree() override { s_tree=nullptr; }

   void OnHelp() override { OpenHelpURL("Triggers.md"); }

   UniquePtr<IPropTreeItem> Import(IPropTreeItem &item, Prop::Global &props, Window window) override;
   void Export(IPropTreeItem &item, Prop::Global &props) override;

   IPropDlg *CreateDialog(Window wndParent, ITreeView *pITreeView) override
   {
      return new Dlg_Trigger(wndParent, pITreeView);
   }

   void GetChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback, IPropTreeItem &item) override
   {
      for(auto &trigger : make_reverse_container(item.ppropTriggers()->WithoutLast(item.ppropTriggers()->AfterCount())))
         callback(MakeUnique<PropTreeItem_Trigger>(trigger));
   }

   void GetPostChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback, IPropTreeItem &item) override
   {
      for(auto &trigger : item.ppropTriggers()->Last(item.ppropTriggers()->AfterCount()))
         callback(MakeUnique<PropTreeItem_Trigger>(trigger));
   }

   UniquePtr<IPropTreeItem> NewChild(IPropTreeItem &item)
   {
      auto ppropTrigger=MakeCounting<Prop::Trigger>();
      auto &triggers=*item.ppropTriggers();
      triggers.Push(ppropTrigger);
      triggers.AfterCount(triggers.AfterCount()+1);
      return MakeUnique<PropTreeItem_Trigger>(ppropTrigger);
   }

   UniquePtr<IPropTreeItem> CopyChild(IPropTreeItem &item, IPropTreeItem &child) override
   {
      if(!child.ppropTrigger())
         return nullptr;

      auto ppropTrigger=MakeCounting<Prop::Trigger>(*child.ppropTrigger());
      return MakeUnique<PropTreeItem_Trigger>(ppropTrigger);
   }

   bool DeleteChild(IPropTreeItem &item, IPropTreeItem *pti, Window wnd) override
   {
      Prop::Trigger *ppropTrigger=pti->ppropTrigger();
      AssumeAssert(ppropTrigger); // Nothing to delete?

      if(ppropTrigger->fPropTriggers() && ppropTrigger->propTriggers().Count()
         && MessageBox(wnd, STR_ChildTriggersDelete, STR_Warning, MB_YESNO)!=IDYES)
         return false;

      ExtractChild(item, pti);
      return true;
   }

   void ExtractChild(IPropTreeItem &item, IPropTreeItem *pti) override
   {
      auto &triggers=*item.ppropTriggers();
      auto position=triggers.Find(pti->ppropTrigger());
      if(position>=triggers.Count()-triggers.AfterCount())
         triggers.AfterCount(triggers.AfterCount()-1);
      triggers.Delete(position);
   }

   void InsertChild(IPropTreeItem &item, IPropTreeItem *ptiChild, IPropTreeItem *ptiNextTo, bool after) override
   {
      CntPtrTo<Prop::Trigger> ppropTrigger=ptiChild->ppropTrigger();
      Prop::Triggers &triggers=*item.ppropTriggers();

      unsigned position=0;

      if(ptiNextTo) // This is nullptr when we're dragged into a container
      {
         if(Prop::Trigger *ppropTriggerNextTo=ptiNextTo->ppropTrigger())
         {
            position=item.ppropTriggers()->Find(ppropTriggerNextTo);
            if(after) position++;
         }
         else // After the bottom of the list, means we're adding to the top of the after count
         {
            if(after)
            {
               position=triggers.Count()-triggers.AfterCount();
               triggers.Insert(position, ppropTrigger);
               triggers.AfterCount(triggers.AfterCount()+1);
               return;
            }
         }

         if(position-after>=triggers.Count()-triggers.AfterCount())
            triggers.AfterCount(triggers.AfterCount()+1);
      }

      triggers.Insert(position, ppropTrigger);
   }

   bool fCanHold(IPropTreeItem &item, IPropTreeItem *pti) override
   {
      // We can hold an item if it's a trigger
      return pti->ppropTrigger();
   }
};

UniquePtr<IPropTreeItem> PropTree::Import(IPropTreeItem &item, Prop::Global &props, Window window)
{
   if(!props.propConnections().propTriggers())
   {
      MessageBox(window, "No triggers in file!", "Error", MB_ICONEXCLAMATION|MB_OK);
      return nullptr;
   }

   auto p_prop_trigger=props.propConnections().propTriggers().Delete(0);
   auto new_item=MakeUnique<PropTreeItem_Trigger>(p_prop_trigger);
   item.ppropTriggers()->Push(std::move(p_prop_trigger));
   item.ppropTriggers()->AfterCount(item.ppropTriggers()->AfterCount()+1);
   return new_item;
}

void PropTree::Export(IPropTreeItem &item, Prop::Global &props)
{
   auto &triggers=props.propConnections().propTriggers();

   if(auto *pTrigger=item.ppropTrigger())
      triggers.Push(MakeCounting<Prop::Trigger>(*pTrigger));
   else if(auto *pTriggers=item.ppropTriggers())
   {
      // If we selected a container only (global/server/char), copy it's contents
      auto &pFolder=triggers.Push(MakeCounting<Prop::Trigger>());
      pFolder->fDisabled(true);
      pFolder->pclDescription(item.Label());

      for(auto &pTrigger : *pTriggers)
         pFolder->propTriggers().Push(MakeCounting<Prop::Trigger>(*pTrigger));
   }
}

};

void CreateDialog_Triggers(Window wnd, Prop::Server *ppropServer, Prop::Character *ppropCharacter, Prop::Trigger *ppropTrigger)
{
   if(s_tree)
      s_tree->GetWindow().Show(SW_SHOWNORMAL);
   else
      s_tree=CreateWindow_PropTree(wnd, MakeUnique<PropTree>(), STR_Title_Triggers);

   if(ppropTrigger)
   {
      auto callback=TCallback<bool(IPropTreeItem&)>([ppropTrigger](IPropTreeItem &item) { return item.ppropTrigger()==ppropTrigger; });
      s_tree->SelectItem(ppropServer, ppropCharacter, &callback);
   }
   else
      s_tree->SelectItem(ppropServer, ppropCharacter, nullptr);
}

void CloseDialog_Triggers()
{
   if(s_tree)
      s_tree->GetWindow().Destroy();
}
