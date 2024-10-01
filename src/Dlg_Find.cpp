//
// Connection Dialog
//

#include "Main.h"

static bool s_regular_expression=false;

struct Dlg_Find : Wnd_Dialog
{
   ~Dlg_Find();

protected:

   void Create(Window wndParent);
   Prop::FindString m_propfs;
   bool m_fShowReplace{};

private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   void UpdateFindString();

   virtual bool FindNext()=0;
   virtual void Replace(ConstString replacement, bool fAll) { }

   enum
   {
      IDC_CHECK_REGULAREXPRESSION = 100,
      IDC_EDIT_FIND,
      IDC_EDIT_REPLACE,
      IDC_FINDNEXT,
      IDC_REPLACE,
      IDC_REPLACE_ALL,
      IDC_CHECK_WHOLEWORD,
      IDC_CHECK_MATCHCASE,
      IDC_DIRECTION_UP,
      IDC_DIRECTION_DOWN,
   };

   AL::Edit *m_pedFind;
   AL::Edit *m_pedReplace;
   AL::CheckBox *m_pcbRegularExpression;
   AL::Radio *m_pcbDirection_Down, *m_pcbDirection_Up;
   AL::CheckBox *m_pcbMatchCase, *m_pcbWholeWord;
   AL::Button *m_pbtFindNext, *m_pbtReplace, *m_pbtReplaceAll, *m_pbtCancel;
};

LRESULT Dlg_Find::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command>(msg);
}

void Dlg_Find::Create(Window wndParent)
{
   Wnd_Dialog::Create(STR_Title_Find, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | Wnd_Dialog::Style_Modal, 0 /*dwExStyle*/, wndParent);

   // Have to do this after the dialog is created or else the text won't show up
   m_pedFind->SetSelAll();
   m_pedFind->SetFocus();
}

Dlg_Find::~Dlg_Find()
{
   g_ppropGlobal->pclLastFind(m_pedFind->GetText());
   s_regular_expression=m_pcbRegularExpression->IsChecked();
}

void Dlg_Find::UpdateFindString()
{
   m_propfs.fRegularExpression(m_pcbRegularExpression->IsChecked());
   m_propfs.fForward(m_pcbDirection_Down->IsChecked());
   m_propfs.fMatchCase(m_pcbMatchCase->IsChecked());
   m_propfs.fWholeWord(m_pcbWholeWord->IsChecked());
   m_propfs.pclMatchText(m_pedFind->GetText());
}

//
// Window Messages
//
LRESULT Dlg_Find::On(const Msg::Create &msg)
{
   auto stFind=m_layout.CreateStatic(STR_Find_MatchText);
   m_pedFind=m_layout.CreateEdit(IDC_EDIT_FIND, int2(20, 1), ES_AUTOHSCROLL);
   if(m_fShowReplace)
      m_pedReplace=m_layout.CreateEdit(IDC_EDIT_REPLACE, int2(20, 1), ES_AUTOHSCROLL);
   m_pbtFindNext=m_layout.CreateButton(IDC_FINDNEXT, STR_Find_FindNext);
   if(m_fShowReplace)
   {
      m_pbtReplace=m_layout.CreateButton(IDC_REPLACE, "Replace");
      m_pbtReplace->Enable(false);
      m_pbtReplaceAll=m_layout.CreateButton(IDC_REPLACE_ALL, "Replace All");
   }

   m_pbtCancel=m_layout.CreateButton(IDCANCEL, STR_Cancel);

   m_pcbRegularExpression=m_layout.CreateCheckBox(IDC_CHECK_REGULAREXPRESSION, STR_Find_RegularExpression);
   m_pcbWholeWord=m_layout.CreateCheckBox(IDC_CHECK_WHOLEWORD, STR_Find_WholeWord);
   m_pcbMatchCase=m_layout.CreateCheckBox(IDC_CHECK_MATCHCASE, STR_Find_MatchCase);

   m_pcbDirection_Up=m_layout.CreateRadio(IDC_DIRECTION_UP, STR_Find_Up, true);
   m_pcbDirection_Down=m_layout.CreateRadio(IDC_DIRECTION_DOWN, STR_Find_Down);

   {
      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); m_layout.SetRoot(pGH);

      
      {
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

         {
            AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            *pGH << stFind << m_pedFind;
         }

         if(m_fShowReplace)
         {
            AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
            auto stReplace=m_layout.CreateStatic("Replace with:");
            *pGH << stReplace << m_pedReplace;
            SetAllToMax(stFind->szMinimum().x, stReplace->szMinimum().x);
         }

         {
            AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;

            {
               AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
               *pGV << m_pcbRegularExpression << m_pcbWholeWord << m_pcbMatchCase;
            }

            {
               AL::GroupBox *pG=m_layout.CreateGroupBox(STR_Find_Direction); *pGH << pG;
               AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); pG->SetChild(pGH);

               *pGH << m_pcbDirection_Up << m_pcbDirection_Down;
            }
         }
      }

      {
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;

         *pGV << m_pbtFindNext;
         if(m_fShowReplace)
            *pGV << m_pbtReplace << m_pbtReplaceAll;
         *pGV << m_pbtCancel;
      }
   }

   m_pedFind->LimitText(256);
   m_pedFind->SetText(g_ppropGlobal->pclLastFind());
   m_pcbRegularExpression->Check(s_regular_expression);
   m_pcbDirection_Up->Check(!m_propfs.fForward());
   m_pcbDirection_Down->Check(m_propfs.fForward());
   m_pbtFindNext->Enable(!!g_ppropGlobal->pclLastFind());
   m_pbtFindNext->SetStyle(BS_DEFPUSHBUTTON, true);

   m_defID = IDC_FINDNEXT;
   return msg.Success();
}

LRESULT Dlg_Find::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDCANCEL:
         Close();
         break;

      case IDC_FINDNEXT:
         UpdateFindString();
         if(!FindNext())
            MessageBox(*this, STR_SearchTextNotFound, STR_Note, MB_ICONEXCLAMATION|MB_OK);
         else
         {
            if(m_fShowReplace)
               EnableWindow(*m_pbtReplace, true);
         }
         break;

      case IDC_EDIT_FIND:
         if(msg.uCodeNotify()==EN_CHANGE)
         {
            m_pbtFindNext->Enable(m_pedFind->GetTextLength()!=0);
            m_propfs.mp_regex_cache=nullptr;
         }
         break;

      case IDC_REPLACE:
         EnableWindow(*m_pbtReplace, false);
         Replace(m_pedReplace->GetText(), false);
         break;

      case IDC_REPLACE_ALL:
         UpdateFindString();
         Replace(m_pedReplace->GetText(), true);
         break;
   }

   return msg.Success();
}

struct Dlg_Find_TextWnd : Dlg_Find
{
   Dlg_Find_TextWnd(Window wndParent, Text::Wnd &wndText)
    : m_wnd_text(wndText)
   {
      m_propfs.fForward(false);
      Create(wndParent);
   }

   ~Dlg_Find_TextWnd()
   {
   }

   bool FindNext() override;
   bool Find(Text::List::Selection &selection);
   bool Find(Text::List::Selection &selection, uint2 &rangeFound);

private:

   Text::Wnd &m_wnd_text;
   Text::Pauser m_pauser{m_wnd_text};
};

bool Dlg_Find_TextWnd::FindNext()
{
   Text::List &list=m_wnd_text.GetTextList();
   Text::List::Selection selection(list.SelectionGet());

   bool fFound=Find(selection);

   // Not found?  Then reset and try again
   if(!fFound && list.SelectionGet())
   {
      selection.Clear();
      fFound=Find(selection);
   }

   if(fFound)
      m_wnd_text.SelectionSet(selection);
   return fFound;
}

bool Dlg_Find_TextWnd::Find(Text::List::Selection &selection)
{
   uint2 rangeFound;
   if(!Find(selection, rangeFound))
      return false;

   selection.m_end.m_line=selection.m_start.m_line;
   selection.m_start.m_char_index=rangeFound.begin;
   selection.m_end.m_char_index=rangeFound.end;
   return true;
}

bool Dlg_Find_TextWnd::Find(Text::List::Selection &selection, uint2 &rangeFound)
{
   auto &lines=m_wnd_text.GetTextList().GetLines();
   auto &iter=selection.m_start.m_line;

   if(m_propfs.fForward())
   {
      // If already existing, search from previous position first
      // Move the selection a notch forwards (so we don't find the same thing twice)
      if(selection && m_propfs.Find(selection.m_start.m_line->GetText(), selection.m_start.m_char_index+1, rangeFound))
         return true;
      if(!selection)
         iter=lines.begin();
      else
         ++iter;

      auto end=lines.end();
      while(iter!=end)
      {
         if(m_propfs.Find(iter->GetText(), 0, rangeFound))
            return true;
         ++iter;
      }
   }
   else
   {
      // If already existing, search from previous position first
      // Move the selection a notch backwards (so we don't find the same thing twice)
      if(selection && m_propfs.Find(selection.m_start.m_line->GetText(), selection.m_end.m_char_index-1, rangeFound))
         return true;
      if(!selection)
         iter=lines.end();

      auto begin=lines.begin();
      while(iter!=begin)
      {
         --iter;
         if(m_propfs.Find(iter->GetText(), iter->GetText().Count(), rangeFound))
            return true;
      }
   }

   return false;
}

void CreateDialog_Find(Window wndParent, Text::Wnd &wndText)
{
   new Dlg_Find_TextWnd(wndParent, wndText);
}

struct Dlg_Find_RichEdit : Dlg_Find
{
   Dlg_Find_RichEdit(Window wndParent, Controls::RichEdit &wndText)
    : m_text(wndText)
   {
      m_propfs.fForward(true);
      m_fShowReplace=true;
      Create(wndParent);
   }

   bool FindNext() override;
   void Replace(ConstString replacement, bool fAll) override;

private:

   Controls::RichEdit &m_text;
};

bool Dlg_Find_RichEdit::FindNext()
{
   Controls::RichEdit::FindTextEx find(m_propfs.pclMatchText());
   
   WPARAM flags=0;
   if(m_propfs.fMatchCase())
      flags|=FR_MATCHCASE;
   if(m_propfs.fWholeWord())
      flags|=FR_WHOLEWORD;
   if(m_propfs.fForward())
   {
      flags|=FR_DOWN;
      find.chrg.cpMin=m_text.GetSel().cpMax;
   }
   else
      find.chrg.cpMin=m_text.GetSel().cpMin;

   if(SendMessage(m_text, EM_FINDTEXTEX, flags, reinterpret_cast<LPARAM>(&find))!=-1)
   {
      m_text.SetSel(find.chrgText);
      m_text.HideSelection(false); // Just in case
      return true;
   }

   m_text.SetSel(0, 0); // Loop to start so next find might succeed
   return false;
}

void Dlg_Find_RichEdit::Replace(ConstString replacement, bool fAll)
{
   if(fAll)
   {
      Controls::RichEdit::EventMaskRestorer _(m_text, 0);
      m_text.HideSelection();
      Controls::RichEdit::FindTextEx find(m_propfs.pclMatchText());
   
      WPARAM flags=FR_DOWN;
      if(m_propfs.fMatchCase())
         flags|=FR_MATCHCASE;
      if(m_propfs.fWholeWord())
         flags|=FR_WHOLEWORD;

      while(SendMessage(m_text, EM_FINDTEXTEX, flags, reinterpret_cast<LPARAM>(&find))!=-1)
      {
         m_text.SetSel(find.chrgText);
         m_text.ReplaceSel(replacement);
         find.chrg.cpMin=m_text.GetSel().cpMax;
      }

      m_text.HideSelection(false);
   }
   else
      m_text.ReplaceSel(replacement);
}

void CreateDialog_Find(Window wndParent, Controls::RichEdit &wndText)
{
   new Dlg_Find_RichEdit(wndParent, wndText);
}
