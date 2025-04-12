//
// Aliases Dialog
//

#include "Main.h"
#include "Wnd_PropTree.h"
#include "Matcharoo.h"
#include "FindString.h"

namespace
{

struct Dlg : IPropDlg
{
   Dlg(Window wndParent, ITreeView *pITreeView);

   bool SetProp(IPropTreeItem *pti, bool fKeepChanges) override;

private:

   LRESULT WndProc(const Message &msg) override;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);

   bool GetFindString(Prop::FindString &fs);

   bool SetAlias(Prop::Alias *ppropAlias, bool fKeepChanges);
   void UpdateEnabled();
   void UpdateExample();

   enum
   {
      IDC_CHECK_TREATASFOLDER = 100,
      IDC_EDIT_MATCHAROO,
      IDC_EDIT_EXAMPLE,
      IDC_EDIT_ALIAS,
      IDC_BUTTON_REGEX101,
      IDC_CHECK_REGULAREXPRESSION,
      IDC_CHECK_MATCHCASE,
      IDC_CHECK_WHOLEWORD,
      IDC_CHECK_LINESTARTSWITH,
      IDC_CHECK_LINEENDSWITH,
   };

   AL::CheckBox *m_pcbActive;
   AL::CheckBox *m_pcbEcho, *m_pcbProcessCommands;
   AL::CheckBox *m_pcbTreatAsFolder;
   AL::Edit *m_pedDescription, *m_pedAlias;
   MatcharooControls m_matcharoo;
   AL::Edit *m_pedResult;
   AL::CheckBox *m_pcbRegularExpression;
   AL::CheckBox *m_pcbMatchCase, *m_pcbWholeWord, *m_pcbStartsWith, *m_pcbEndsWith;
   AL::CheckBox *m_pcbStopProcessing;
   AL::CheckBox *m_pcbExpandVariables;

   ITreeView *m_pITreeView;
   Prop::Alias *m_ppropAlias{};

   Time::Timer m_timerExample{[this]() { UpdateExample(); }};
};

LRESULT Dlg::WndProc(const Message &msg)
{
   switch(msg.uMessage())
   {
      case WM_CREATE: return Call_On(Create);
      case WM_COMMAND: return Call_On(Command);
   }

   return __super::WndProc(msg);
}

Dlg::Dlg(Window wndParent, ITreeView *pITreeView) : m_pITreeView(pITreeView)
{
   Create("", wndParent);
}

bool Dlg::SetProp(IPropTreeItem *pti, bool fKeepChanges)
{
   return SetAlias(pti ? pti->ppropAlias() : nullptr, fKeepChanges);
}

void Dlg::UpdateEnabled()
{
   bool fEnable=m_ppropAlias!=nullptr;

   m_pcbTreatAsFolder->Enable(fEnable);
   EnableWindows(fEnable, *m_pedDescription, *m_matcharoo.m_pedMatchText);
   fEnable&=!m_pcbTreatAsFolder->IsChecked();
   EnableWindows(fEnable, *m_matcharoo.m_pedExample, *m_pcbRegularExpression, *m_pcbMatchCase, *m_pedAlias, *m_pcbStopProcessing, *m_pcbExpandVariables);
   EnableWindows(fEnable && m_pcbRegularExpression->IsChecked(), *m_matcharoo.m_pRegex101);
   EnableWindows(fEnable && !m_pcbRegularExpression->IsChecked(), *m_pcbWholeWord, *m_pcbStartsWith, *m_pcbEndsWith);
}

bool Dlg::GetFindString(Prop::FindString &fs)
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

bool Dlg::SetAlias(Prop::Alias *ppropNewAlias, bool fKeepChanges)
{
   g_ppropGlobal->propConnections().propAliases().fActive(m_pcbActive->IsChecked());
   g_ppropGlobal->propConnections().propAliases().fEcho(m_pcbEcho->IsChecked());
   g_ppropGlobal->propConnections().propAliases().fProcessCommands(m_pcbProcessCommands->IsChecked());

   if(ppropNewAlias!=nullptr && m_ppropAlias==ppropNewAlias)
      return false;

   bool fUpdate=false;

   if(m_ppropAlias && fKeepChanges)
   {
      fUpdate|=GetFindString(m_ppropAlias->propFindString());

      auto strDescription=m_pedDescription->GetText();

      fUpdate=m_ppropAlias->pclDescription()!=strDescription || m_ppropAlias->fFolder()!=m_pcbTreatAsFolder->IsChecked();

      auto strExample=m_matcharoo.m_pedExample->GetText();
      m_ppropAlias->pclExample(strExample!=m_matcharoo.c_strExample ? strExample : ConstString{});

      m_ppropAlias->fFolder(m_pcbTreatAsFolder->IsChecked());
      m_ppropAlias->pclDescription(strDescription);

      m_ppropAlias->pclReplace(m_pedAlias->GetText());
      m_ppropAlias->fStopProcessing(m_pcbStopProcessing->IsChecked());
      m_ppropAlias->fExpandVariables(m_pcbExpandVariables->IsChecked());
   }

   m_ppropAlias=ppropNewAlias;

   if(!m_ppropAlias)
   {
      m_pedDescription->SetText("");
      m_matcharoo.m_pedMatchText->SetText("");
      m_matcharoo.m_pedExample->SetText("");
      m_pedAlias->SetText("");
      UpdateEnabled();
      return fUpdate;
   }

   const Prop::FindString *pfs=&m_ppropAlias->propFindString();

   m_pcbTreatAsFolder->Check(m_ppropAlias->fFolder());
   m_pedDescription->SetText(m_ppropAlias->pclDescription());
   m_matcharoo.m_pedMatchText->SetText(pfs->pclMatchText());
   m_matcharoo.m_pedExample->SetText(m_ppropAlias->pclExample() ? ConstString{m_ppropAlias->pclExample()} : m_matcharoo.c_strExample);

   m_pcbRegularExpression->Check(pfs->fRegularExpression());
   m_pcbMatchCase->Check(pfs->fMatchCase());
   m_pcbWholeWord->Check(pfs->fWholeWord());
   m_pcbStartsWith->Check(pfs->fStartsWith());
   m_pcbEndsWith->Check(pfs->fEndsWith());

   m_pcbStopProcessing->Check(ppropNewAlias->fStopProcessing());
   m_pcbExpandVariables->Check(ppropNewAlias->fExpandVariables());

   m_pedAlias->SetText(m_ppropAlias->pclReplace());
   UpdateEnabled();

   return fUpdate;
}

//
// Window Messages
//
LRESULT Dlg::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   {
      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);
      *pGH >> AL::Style::Attach_Left;

      {
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
         m_pcbActive=m_layout.CreateCheckBox(-1, STR_ProcessAliases); *pGV << m_pcbActive;
         m_pcbEcho=m_layout.CreateCheckBox(-1, STR_EchoProcessedAliases); *pGV << m_pcbEcho;
         m_pcbProcessCommands=m_layout.CreateCheckBox(-1, "Process commands in result"); *pGV << m_pcbProcessCommands;
      }
   }
   m_pcbTreatAsFolder=m_layout.CreateCheckBox(IDC_CHECK_TREATASFOLDER, STR_AliasTreatAsFolder);
   *pGV << m_pcbTreatAsFolder;

   auto *pstDescription=m_layout.CreateStatic("Description:");
   {
      m_pedDescription=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
      m_pedDescription->LimitText(Prop::Trigger::pclDescription_MaxLength());
      AL::Group_Horizontal *pGroup=m_layout.CreateGroup_Horizontal(); pGroup->weight(0);
      *pGroup << pstDescription << m_pedDescription;
      *pGV << pGroup;
   }

   m_matcharoo.Create(m_layout, *pGV, IDC_EDIT_MATCHAROO, IDC_EDIT_EXAMPLE, IDC_BUTTON_REGEX101);

   auto *pstResult=m_layout.CreateStatic("Test Result:");
   {
      m_pedResult=m_layout.CreateEdit(-1, int2(30, 3), ES_MULTILINE|ES_SUNKEN|ES_READONLY|WS_VSCROLL);
      AL::Group_Horizontal *pGroup=m_layout.CreateGroup_Horizontal(); pGroup->weight(0);
      *pGroup << pstResult << m_pedResult;
      *pGV << pGroup;
   }

   SetAllToMax(pstDescription->szMinimum().x, m_matcharoo.m_pstMatchText->szMinimum().x, m_matcharoo.m_pstExample->szMinimum().x, pstResult->szMinimum().x);

   {
      auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);
      {
         auto *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
         m_pcbRegularExpression=m_layout.CreateCheckBox(IDC_CHECK_REGULAREXPRESSION, STR_RegularExpression);
         m_pcbMatchCase=m_layout.CreateCheckBox(IDC_CHECK_MATCHCASE, STR_MatchCase);
         m_pcbWholeWord=m_layout.CreateCheckBox(IDC_CHECK_WHOLEWORD, STR_WholeWord);

         *pGV << m_pcbRegularExpression << m_pcbMatchCase << m_pcbWholeWord;
      }
      {
         auto *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
         m_pcbStopProcessing=m_layout.CreateCheckBox(-1, "Stop Processing if hit");
         m_pcbStartsWith=m_layout.CreateCheckBox(IDC_CHECK_LINESTARTSWITH, STR_LineStartsWith);
         m_pcbEndsWith=m_layout.CreateCheckBox(IDC_CHECK_LINEENDSWITH, STR_LineEndsWith);
         *pGV << m_pcbStopProcessing << m_pcbStartsWith << m_pcbEndsWith;
      }
   }

   m_pcbExpandVariables=m_layout.CreateCheckBox(-1, "Expand %variables%");
   *pGV << m_pcbExpandVariables;

   *pGV << m_layout.CreateStatic(STR_AliasFor);

   m_pedAlias=m_layout.CreateEdit(IDC_EDIT_ALIAS, int2(30, 3), ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL); *pGV << m_pedAlias;
   m_pedAlias->LimitText(Prop::Alias::pclReplace_MaxLength());

   m_pcbActive->Check(g_ppropGlobal->propConnections().propAliases().fActive());
   m_pcbEcho->Check(g_ppropGlobal->propConnections().propAliases().fEcho());
   m_pcbProcessCommands->Check(g_ppropGlobal->propConnections().propAliases().fProcessCommands());

   SetProp(nullptr, true);
   return msg.Success();
}

LRESULT Dlg::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_EDIT_MATCHAROO:
      case IDC_EDIT_EXAMPLE:
      case IDC_EDIT_ALIAS:
         if(msg.uCodeNotify()==EN_CHANGE)
            m_timerExample.Set(0.1f);
         break;

      case IDC_BUTTON_REGEX101:
         if(msg.uCodeNotify()==BN_CLICKED)
            m_matcharoo.OpenRegex101(false, m_pcbMatchCase->IsChecked());
         break;

      case IDC_CHECK_MATCHCASE:
      case IDC_CHECK_WHOLEWORD:
      case IDC_CHECK_LINESTARTSWITH:
      case IDC_CHECK_LINEENDSWITH:
      case IDC_CHECK_REGULAREXPRESSION:
         if(msg.uCodeNotify()==BN_CLICKED)
            m_timerExample.Set(0.1f);

      case IDC_CHECK_TREATASFOLDER:
         if(msg.uCodeNotify()==BN_CLICKED)
            UpdateEnabled();
         break;
   }

   return msg.Success();
}

void Dlg::UpdateExample()
{
   Prop::FindString fs;
   GetFindString(fs);
   m_matcharoo.UpdateExample(fs);

   if(m_ppropAlias && fs.pclMatchText())
   {
      HybridStringBuilder result{m_matcharoo.m_pedExample->GetText() ? m_matcharoo.m_pedExample->GetText() : m_matcharoo.c_strExample};
      auto replace=m_pedAlias->GetText();

      FindStringSearch search(fs, result);
      while(search.Next())
      {
         FindStringReplacement replacement(search, replace);
         result.Replace(search.RangeFound(), replacement);
         search.HandleReplacement(replacement.Length(), result);
      }

      m_pedResult->SetText(result);
   }
   else
      m_pedResult->SetText("");
}

//
// PropTree Interfaces
//
struct PropTreeItem_Alias : IPropTreeItem
{
   PropTreeItem_Alias(Prop::Alias *p_prop_alias) : mp_prop_alias(p_prop_alias) { }

   ConstString Label() const override { return mp_prop_alias->pclDescription() ? mp_prop_alias->pclDescription() : mp_prop_alias->propFindString().pclMatchText(); }

   bool fCanRename() const override { return true; }
   void Rename(ConstString string) override
   {
      mp_prop_alias->pclDescription() ? mp_prop_alias->pclDescription(string) : mp_prop_alias->propFindString().pclMatchText(string);
   }

   bool fCanDelete() const override { return true; }
   bool fCanMove() const override { return true; }
   bool fSort() const override { return false; }
   eTIB eBitmap() const override { return mp_prop_alias->fFolder() ? TIB_FOLDER_CLOSED : TIB_ALIAS; }

   Prop::Alias *ppropAlias() override { return mp_prop_alias; }
   Prop::Aliases *ppropAliases() override { return &mp_prop_alias->propAliases(); }

private:

   CntPtrTo<Prop::Alias> mp_prop_alias;
};

//
// PropTree
//

static ITreeView *s_tree{};

struct PropTree : IPropTree
{
   ~PropTree() override { s_tree=nullptr; }

   void OnHelp() override { OpenHelpURL("Aliases.md"); }

   UniquePtr<IPropTreeItem> Import(IPropTreeItem &item, Prop::Global &props, Window window) override;
   void Export(IPropTreeItem &item, Prop::Global &props) override;

   IPropDlg *CreateDialog(Window wndParent, ITreeView *pITreeView) override
   {
      return new Dlg(wndParent, pITreeView);
   }

   void GetChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback, IPropTreeItem &item) override
   {
      for(auto &alias : make_reverse_container(item.ppropAliases()->Pre()))
         callback(MakeUnique<PropTreeItem_Alias>(alias));
   }

   void GetPostChildren(TCallback<void(UniquePtr<IPropTreeItem> &&)> callback, IPropTreeItem &item) override
   {
      for(auto &alias : item.ppropAliases()->Post())
         callback(MakeUnique<PropTreeItem_Alias>(alias));
   }

   UniquePtr<IPropTreeItem> NewChild(IPropTreeItem &item) override
   {
      if(!item.ppropAliases())
         return nullptr;

      return MakeUnique<PropTreeItem_Alias>(item.ppropAliases()->Push(MakeCounting<Prop::Alias>()));
   }

   UniquePtr<IPropTreeItem> CopyChild(IPropTreeItem &item, IPropTreeItem &child) override
   {
      if(!child.ppropAlias())
         return nullptr;

      auto p_prop_alias=MakeCounting<Prop::Alias>(*child.ppropAlias());
      auto pItem=MakeUnique<PropTreeItem_Alias>(p_prop_alias);

      return pItem;
   }

   bool DeleteChild(IPropTreeItem &item, IPropTreeItem *pti, Window wnd) override
   {
      item.ppropAliases()->FindAndDelete(pti->ppropAlias());
      return true;
   }

   void ExtractChild(IPropTreeItem &item, IPropTreeItem *pti) override
   {
      auto &aliases=*item.ppropAliases();
      auto position=aliases.Find(pti->ppropAlias());
      if(position>=aliases.Count()-aliases.AfterCount())
         aliases.AfterCount(aliases.AfterCount()-1);
      aliases.Delete(position);
   }

   void InsertChild(IPropTreeItem &item, IPropTreeItem *ptiChild, IPropTreeItem *ptiNextTo, bool after) override
   {
      CntPtrTo<Prop::Alias> ppropAlias=ptiChild->ppropAlias();
      Prop::Aliases &aliases=*item.ppropAliases();

      unsigned position=0;

      if(ptiNextTo) // This is nullptr when we're dragged into a container
      {
         if(Prop::Alias *ppropAliasNextTo=ptiNextTo->ppropAlias())
         {
            position=item.ppropAliases()->Find(ppropAliasNextTo);
            if(after) position++;
         }
         else // After the bottom of the list, means we're adding to the top of the after count
         {
            if(after)
            {
               position=aliases.Count()-aliases.AfterCount();
               aliases.Insert(position, ppropAlias);
               aliases.AfterCount(aliases.AfterCount()+1);
               return;
            }
         }

         if(position-after>=aliases.Count()-aliases.AfterCount())
            aliases.AfterCount(aliases.AfterCount()+1);
      }

      aliases.Insert(position, ppropAlias);
   }

   bool fCanHold(IPropTreeItem &target, IPropTreeItem *source) override
   {
      // Return true if we can hold aliases and the target is an alias
      return target.ppropAliases() && source->ppropAlias();
   }

};

UniquePtr<IPropTreeItem> PropTree::Import(IPropTreeItem &item, Prop::Global &props, Window window)
{
   if(!props.propConnections().propAliases())
   {
      MessageBox(window, "No aliases in file!", "Error", MB_ICONEXCLAMATION|MB_OK);
      return nullptr;
   }

   auto p_prop_alias=props.propConnections().propAliases().Delete(0);
   auto new_item=MakeUnique<PropTreeItem_Alias>(p_prop_alias);
   item.ppropAliases()->Push(std::move(p_prop_alias));
   item.ppropAliases()->AfterCount(item.ppropAliases()->AfterCount()+1);
   return new_item;
}

void PropTree::Export(IPropTreeItem &item, Prop::Global &props)
{
   auto &aliases=props.propConnections().propAliases();

   if(auto *p_alias=item.ppropAlias())
      aliases.Push(MakeCounting<Prop::Alias>(*p_alias));
   else if(auto *pAliases=item.ppropAliases())
   {
      // If we selected a container only (global/server/char), copy it's contents
      auto &pFolder=aliases.Push(MakeCounting<Prop::Alias>());
      pFolder->pclDescription(item.Label());

      for(auto &pAlias : *pAliases)
         pFolder->propAliases().Push(MakeCounting<Prop::Alias>(*pAlias));
   }
}

};

void CreateDialog_Aliases(Window wnd, Prop::Server *ppropServer, Prop::Character *ppropCharacter)
{
   if(s_tree)
      s_tree->GetWindow().Show(SW_SHOWNORMAL);
   else
      s_tree=CreateWindow_PropTree(wnd, MakeUnique<PropTree>(), STR_Aliases); 

   s_tree->SelectItem(ppropServer, ppropCharacter, nullptr);
}

void CloseDialog_Aliases()
{
   if(s_tree)
      s_tree->GetWindow().Destroy();
}
