//
// Keyboard Macros Dialog
//

#include "Main.h"
#include "Wnd_PropTree.h"

namespace
{

struct Dlg : IPropDlg, IEditHost
{
   Dlg(Window wndParent, ITreeView *pITreeView);

   bool SetProp(IPropTreeItem *pti, bool fKeepChanges) override;

private:

   LRESULT WndProc(const Message &msg) override;

   // Window Messages
   LRESULT On(const Msg::Create &msg);

   bool EditKey(const Msg::Key &msg) override;
   bool EditChar(const Msg::Char &msg) override { return true; }

   bool SetKeyMacro(CKeyMacro *pKeyMacro, bool fKeepChanges);

   ITreeView *m_pITreeView;
   CKeyMacro *m_pKeyMacro{};

   int m_iVKey;

   AL::CheckBox *m_pcbActive;
   AL::Edit *m_pedMacro, *m_pedKey;
   AL::CheckBox *m_pcbType, *m_pcbControl, *m_pcbAlt, *m_pcbShift;
};

LRESULT Dlg::WndProc(const Message &msg)
{
   switch(msg.uMessage())
   {
      case Msg::Create::ID: return Call_On(Create);
   }

   return __super::WndProc(msg);
}

Dlg::Dlg(Window wndParent, ITreeView *pITreeView) : m_pITreeView(pITreeView)
{
   Create("", wndParent);
}

bool Dlg::SetProp(IPropTreeItem *pti, bool fKeepChanges)
{
   return SetKeyMacro(pti ? pti->pKeyMacro() : nullptr, fKeepChanges);
}

bool Dlg::SetKeyMacro(CKeyMacro *pNewKeyMacro, bool fKeepChanges)
{
   g_ppropGlobal->propConnections().propKeyboardMacros().fActive(m_pcbActive->IsChecked());

   if(pNewKeyMacro!=nullptr && m_pKeyMacro==pNewKeyMacro)
      return false; // Nothing changed

   HybridStringBuilder sBuffer;
   bool fUpdate=false;

   if(m_pKeyMacro && fKeepChanges)
   {
      sBuffer(m_pedMacro->GetText());
      m_pKeyMacro->pclMacro=sBuffer;

      KEY_ID &key=m_pKeyMacro->key;

      KEY_ID keyNew;
      keyNew.iVKey=m_iVKey;
      keyNew.fControl=m_pcbControl->IsChecked();
      keyNew.fAlt=m_pcbAlt->IsChecked();
      keyNew.fShift=m_pcbShift->IsChecked();

      m_pKeyMacro->fType=m_pcbType->IsChecked();

      if(keyNew!=key)
      {
         fUpdate=true;
         key=keyNew;
      }
   }

   m_pKeyMacro=pNewKeyMacro;

   bool fEnable=m_pKeyMacro!=nullptr;
   m_pedMacro->Enable(fEnable);
   m_pedKey  ->Enable(fEnable);
   m_pcbType   ->Enable(fEnable);
   m_pcbControl->Enable(fEnable);
   m_pcbAlt    ->Enable(fEnable);
   m_pcbShift  ->Enable(fEnable);

   if(!m_pKeyMacro)
   {
      m_pedMacro->SetText("");
      m_pedKey->SetText("");
      return fUpdate;
   }

   m_pedMacro->SetText(m_pKeyMacro->pclMacro);

   const KEY_ID *pKey=&m_pKeyMacro->key;

   sBuffer.Clear();
   pKey->KeyName(sBuffer);

   m_pedKey->SetText(sBuffer);

   m_iVKey=pKey->iVKey;
   m_pcbControl->Check(pKey->fControl);
   m_pcbAlt    ->Check(pKey->fAlt);
   m_pcbShift  ->Check(pKey->fShift);

   m_pcbType->Check(m_pKeyMacro->fType);
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

      m_pcbActive=m_layout.CreateCheckBox(-1, STR_ProcessKeyboardMacros); *pGH << m_pcbActive;
   }

   *pGV << m_layout.CreateStatic(STR_MacroText);
   m_pedMacro=m_layout.CreateEdit(-1, int2(30, 4), ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL); *pGV << m_pedMacro;
   m_pedMacro->LimitText(65536);
   m_pcbType=m_layout.CreateCheckBox(-1, STR_TypeIntoInputWindow); *pGV << m_pcbType;

   {
      AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Key); *pGV << pGB; pGB->weight(0);
      AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);
      {
         AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
         *pGH << m_layout.CreateStatic(STR_PressKeyHere);
         m_pedKey=m_layout.CreateEdit(-1, int2(10, 1), ES_CENTER);
         *pGH << m_pedKey;
      }
      {
         AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Modifiers); *pGV << pGB;
         AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); pGB->SetChild(pGH);
         m_pcbControl=m_layout.CreateCheckBox(-1, STR_Control); m_pcbControl->weight(1);
         m_pcbAlt=m_layout.CreateCheckBox(-1, STR_Alt); m_pcbAlt->weight(1);
         m_pcbShift=m_layout.CreateCheckBox(-1, STR_Shift); m_pcbShift->weight(1);

         *pGH << m_pcbControl << m_pcbAlt << m_pcbShift;
      }     
   }
   *pGV << m_layout.CreateStatic(STR_MacroLimitations);

   m_pcbActive->Check(g_ppropGlobal->propConnections().propKeyboardMacros().fActive());

   Init_EditSendEnter(*m_pedKey, *this);
   SetProp(nullptr, true);

   return msg.Success();
}

bool Dlg::EditKey(const Msg::Key &msg)
{
   if(msg.direction()==Direction::Up) return true;
   auto iVKey=msg.iVirtKey();

   // Only allow certain keys
   // F-Keys, A-Z, 0-9
   if((iVKey<VK_F1 || iVKey>VK_F24) && (iVKey<'A' || iVKey>'Z') && (iVKey<'0' || iVKey>'9') &&
      (iVKey<VK_NUMPAD0 || iVKey>VK_NUMPAD9) &&
      (iVKey!=VK_TAB && iVKey!=VK_PAUSE && iVKey!=VK_SPACE && iVKey!=VK_HOME &&
       iVKey!=VK_UP && iVKey!=VK_DOWN && iVKey!=VK_LEFT && iVKey!=VK_RIGHT &&
       iVKey!=VK_ADD && iVKey!=VK_SUBTRACT && iVKey!=VK_MULTIPLY && iVKey!=VK_DIVIDE &&
       iVKey!=VK_DECIMAL && iVKey!=VK_OEM_1 /* ; */ && iVKey!=VK_OEM_PLUS))
      return false;

   m_iVKey=iVKey;

   KEY_ID key; // Temporary Key so we can update the display
   key.iVKey=iVKey;
   key.fControl=IsKeyPressed(VK_CONTROL);
   key.fAlt=IsKeyPressed(VK_MENU);
   key.fShift=IsKeyPressed(VK_SHIFT);

   m_pcbControl->Check(key.fControl);
   m_pcbAlt    ->Check(key.fAlt);
   m_pcbShift  ->Check(key.fShift);

   {
      FixedStringBuilder<21> sBuffer; key.KeyName(sBuffer);
      m_pedKey->SetText(sBuffer);
   }

   // Swap in the key so we can display it
   {
      KEY_ID keyOriginal=m_pKeyMacro->key;
      m_pKeyMacro->key=key;
      m_pITreeView->UpdateSelection();
      m_pKeyMacro->key=keyOriginal;
   }

   return true;
}

//
// PropTree Interfaces
//
struct PropTreeItem : IPropTreeItem
{
   PropTreeItem(CKeyMacro &keyMacro) : m_keyMacro(keyMacro) { }

   ConstString Label() const override
   {
      FixedStringBuilder<256> sBuffer;

      const KEY_ID &key=m_keyMacro.key;

      if(key.fAlt) sBuffer(STR_Alt " + ");
      if(key.fControl) sBuffer(STR_Control " + ");
      if(key.fShift) sBuffer(STR_Shift " + ");

      key.KeyName(sBuffer);

      m_pcLabel=sBuffer;
      return m_pcLabel;
   }

   bool fCanDelete() const override { return true; }
   bool fCanMove() const override { return true; }
   bool fSort() const override { return false; }
   eTIB eBitmap() const override { return TIB_KEYMACRO; }

   CKeyMacro *pKeyMacro() override { return &m_keyMacro; }
   Prop::KeyboardMacros *ppropKeyboardMacros() override { return nullptr; }

private:

   mutable OwnedString m_pcLabel;
   CKeyMacro &m_keyMacro;
};

static ITreeView *s_tree{};

//
struct PropTree : IPropTree
{
   ~PropTree() override { s_tree=nullptr; }

   void OnHelp() override { OpenHelpURL("Macros.md"); }

   UniquePtr<IPropTreeItem> Import(IPropTreeItem &item, Prop::Global &props, Window window) override;
   void Export(IPropTreeItem &item, Prop::Global &props) override;

   IPropDlg *CreateDialog(Window wndParent, ITreeView *pITreeView) override
   {
      return new Dlg(wndParent, pITreeView);
   }

   void GetChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback, IPropTreeItem &item) override
   {
      if(!item.ppropKeyboardMacros())
         return;

      for(auto &pMacro : make_reverse_container(*item.ppropKeyboardMacros()))
         callback(MakeUnique<PropTreeItem>(*pMacro));
   }

   UniquePtr<IPropTreeItem> NewChild(IPropTreeItem &item) override
   {
      if(!item.ppropKeyboardMacros())
         return nullptr;

      return MakeUnique<PropTreeItem>(*item.ppropKeyboardMacros()->Push(MakeUnique<CKeyMacro>()));
   }

   UniquePtr<IPropTreeItem> CopyChild(IPropTreeItem &item, IPropTreeItem &child) override
   {
      if(!child.pKeyMacro())
         return nullptr;

      auto ppropKeyMacro=MakeUnique<CKeyMacro>(*child.pKeyMacro());
      auto pItem=MakeUnique<PropTreeItem>(*ppropKeyMacro);

      ppropKeyMacro.Extract(); // TODO: The alias isn't owned by the IPropTreeItem, so if the InsertChild doesn't happen this object can leak
      return pItem;
   }

   bool DeleteChild(IPropTreeItem &item, IPropTreeItem *pti, Window wnd) override
   {
      item.ppropKeyboardMacros()->FindAndDelete(pti->pKeyMacro());
      return true;
   }

   void ExtractChild(IPropTreeItem &item, IPropTreeItem *pti) override
   {
      item.ppropKeyboardMacros()->FindAndDelete(pti->pKeyMacro()).Extract();
   }

   void InsertChild(IPropTreeItem &item, IPropTreeItem *pti, IPropTreeItem *ptiNextTo, bool fAfter) override
   {
      CKeyMacro *pKeyMacro=pti->pKeyMacro();

      unsigned iPosition=0;
      if(ptiNextTo)
      {
         CKeyMacro *ppropKeyboardMacroNextTo=ptiNextTo->pKeyMacro();
         Assert(ppropKeyboardMacroNextTo);
         iPosition=item.ppropKeyboardMacros()->Find(ppropKeyboardMacroNextTo);
         if(fAfter) iPosition++;
      }

      item.ppropKeyboardMacros()->Insert(iPosition, pKeyMacro);
   }

   bool fCanHold(IPropTreeItem &item, IPropTreeItem *pti) override
   {
      // Return true if the item passed in is a keyboard macro and we can hold one
      return pti->pKeyMacro() && item.ppropKeyboardMacros();
   }
};

UniquePtr<IPropTreeItem> PropTree::Import(IPropTreeItem &item, Prop::Global &props, Window window)
{
   if(!props.propConnections().propKeyboardMacros())
   {
      MessageBox(window, "No macros in file!", "Error", MB_ICONEXCLAMATION|MB_OK);
      return nullptr;
   }

   auto p_prop_macro=props.propConnections().propKeyboardMacros().Delete(0);
   auto new_item=MakeUnique<PropTreeItem>(*p_prop_macro);
   item.ppropKeyboardMacros()->Push(std::move(p_prop_macro));
   return new_item;
}

void PropTree::Export(IPropTreeItem &item, Prop::Global &props)
{
   auto &macros=props.propConnections().propKeyboardMacros();

   if(auto *p_macro=item.pKeyMacro())
      macros.Push(MakeUnique<CKeyMacro>(*p_macro));
}

};

void CreateDialog_KeyboardMacros(Window wnd, Prop::Server *ppropServer, Prop::Character *ppropCharacter)
{
   if(s_tree)
      s_tree->GetWindow().Show(SW_SHOWNORMAL);
   else
      s_tree=CreateWindow_PropTree(wnd, MakeUnique<PropTree>(), STR_Title_KeyboardMacros); 

   s_tree->SelectItem(ppropServer, ppropCharacter, nullptr);
}

void CloseDialog_KeyboardMacros()
{
   if(s_tree)
      s_tree->GetWindow().Destroy();
}
