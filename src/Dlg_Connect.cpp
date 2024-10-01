//
// Connection Dialog
//

#include "Main.h"
#include "Connection.h"

#include "Wnd_Main.h"
#include "CDlg_Puppet.h"
#include "CDlg_Character.h"
#include "CDlg_Server.h"

void CreateDialog_Statistics(Window wnd);

//=============================================================================

struct Wnd_Connect : Wnd_Dialog, Singleton<Wnd_Connect>
{
   Wnd_Connect(Window wndParent, Wnd_MDI &m_wnd_MDI);

   enum eTIB // These identify the bitmap and the type of the object
   {
      TIB_SERVER,
      TIB_CHARACTER,
      TIB_PUPPET,
      TIB_MAX
   };

private:

   ~Wnd_Connect() noexcept;

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Notify &msg);
   LRESULT On(const Msg::MouseMove &msg);
   LRESULT On(const Msg::LButtonUp &msg);
   LRESULT On(const Msg::CaptureChanged &msg);

   interface ITreeItem
   {
      virtual ~ITreeItem() noexcept { }

      virtual eTIB Type() const=0;
      virtual ConstString Label() const=0;

      virtual void Delete(ITreeItem *ptiParent)=0;

      virtual Prop::Server    *GetServer() noexcept { Assert(0); return nullptr; }
      virtual Prop::Character *GetCharacter() noexcept { Assert(0); return nullptr; }
      virtual Prop::Puppet    *GetPuppet() noexcept { Assert(0); return nullptr; }
   };

   struct TreeItem_Server : ITreeItem
   {
      TreeItem_Server(Prop::Server &propServer) : m_propServer(propServer) { }

      eTIB Type() const override { return TIB_SERVER; }
      ConstString Label() const override { return m_propServer->pclName(); }
      void Delete(ITreeItem *ptiParent) override { g_ppropGlobal->propConnections().propServers().Delete(*m_propServer); }
      void DeleteChild(Prop::Character *pItem) { m_propServer->propCharacters().Delete(*pItem); }

      Prop::Server *GetServer() noexcept override { return m_propServer; }

   private:

      CntRefTo<Prop::Server> m_propServer;
   };

   struct TreeItem_Character : ITreeItem
   {
      TreeItem_Character(Prop::Character &propCharacter) : m_propCharacter(propCharacter) { }

      eTIB Type() const override { return TIB_CHARACTER; }
      ConstString Label() const override { return m_propCharacter->pclName(); }
      void Delete(ITreeItem *ptiParent) override { static_cast<TreeItem_Server *>(ptiParent)->DeleteChild(m_propCharacter); }
      void DeleteChild(Prop::Puppet *pItem) { m_propCharacter->propPuppets().FindAndDelete(pItem); }

      Prop::Character *GetCharacter() noexcept override { return m_propCharacter; }

   private:

      CntRefTo<Prop::Character> m_propCharacter;
   };

   struct TreeItem_Puppet : ITreeItem
   {
      TreeItem_Puppet(Prop::Puppet &propPuppet) : m_propPuppet(propPuppet) { }

      eTIB Type() const override { return TIB_PUPPET; }
      ConstString Label() const override { return m_propPuppet->pclName(); }
      void Delete(ITreeItem *ptiParent) override { static_cast<TreeItem_Character *>(ptiParent)->DeleteChild(m_propPuppet); }
      Prop::Puppet *GetPuppet() noexcept override { return m_propPuppet; }

   private:

      CntRefTo<Prop::Puppet> m_propPuppet;
   };

   using Tree=Controls::TreeView<ITreeItem*>;

   void InitImageList();
   void InitTree(Tree &tree, Prop::Servers &servers);

   HTREEITEM InsertItem(Tree &tree, HTREEITEM htiParent, UniquePtr<ITreeItem> &&pti);
   void OnSelectionChange(Tree &tree);

   void Save();
   void TreeConnect();
   void Import();
   void Export();

   Wnd_MDI &m_wnd_MDI;

   Tree m_tree;
   Tree m_tree_samples;
   Tree *mp_active_tree{&m_tree}; // The tree that holds the item currently on display

   enum
   {
      IDC_CONNECT = 100,
      IDC_DELETE,
      IDC_COPY,
      IDC_RENAME,
      IDC_NEW,
      IDC_IMPORT,
      IDC_EXPORT,
      ID_NEW_SERVER,
      ID_NEW_CHARACTER,
      ID_NEW_PUPPET,
      IDC_TREE,
      IDC_TREE_SAMPLES,
      IDC_EDIT_TREEITEM,
      IDC_ADD_CURRENT,
      ID_STATISTICS,
      ID_HELP,
   };

   AL::Button *m_pAddCurrentWorld;
   AL::Button *m_pExplore;

   AL::Button *m_pbtNew, *m_pbtConnect, *m_pbtRename, *m_pbtCopy, *m_pbtDelete;
   AL::Button *m_pbtImport, *m_pbtExport;

   AL::Stack *m_pDialogStack;

   ImageList m_iml;

   // Dragging
   bool m_drag_from_samples{};
   Tree::ItemAndParam m_drag_from; // Item being dragged
   Tree::ItemAndParam m_drag_to;   // Item it's being dragged to
   UniquePtr<Controls::DragImage> mp_drag_image;

   CDlg_Server    *m_pDlg_Server{};
   CDlg_Character *m_pDlg_Character{};
   CDlg_Puppet    *m_pDlg_Puppet{};
};

LRESULT Wnd_Connect::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command, Msg::Notify, Msg::MouseMove, Msg::LButtonUp, Msg::CaptureChanged>(msg);
}

Wnd_Connect::Wnd_Connect(Window wndParent, Wnd_MDI &wndMDI)
:  m_wnd_MDI(wndMDI)
{
   m_defID=IDC_CONNECT;
   Create(STR_Title_Worlds,
          WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_VISIBLE | Wnd_Dialog::Style_Modal, 0 /*dwExStyle*/, wndParent);
}

Wnd_Connect::~Wnd_Connect()
{
   m_tree.Destroy();
   m_tree_samples.Destroy();
}

void Wnd_Connect::InitImageList()
{
   IconDrawer id{g_ppropGlobal->iUIFontSize()*g_dpiScale};

   m_iml=id.CreateImageList(3);
   id.Add("🖥", m_iml);
   id.Add("👤", m_iml);
   id.Add("🦖", m_iml);
}

void Wnd_Connect::InitTree(Tree &tree, Prop::Servers &servers)
{
   tree.SetImageList(m_iml, TVSIL_NORMAL);

   // Add the tree items
   for(auto &pPropServer : servers)
      InsertItem(tree, TVI_ROOT, MakeUnique<TreeItem_Server>(*pPropServer));
}

void Wnd_Connect::OnSelectionChange(Tree &tree)
{
   if(mp_active_tree==&m_tree)
   {
      Save();

      if(auto selection=m_tree.GetSelection())
         m_tree.SetItemText(selection, selection->Label());
   }

   if(&tree!=mp_active_tree)
   {
      mp_active_tree=&tree;

      bool is_main_tree=mp_active_tree==&m_tree;
      EnableWindows(is_main_tree, *m_pbtDelete, *m_pbtCopy, *m_pbtNew, *m_pbtImport, *m_pbtExport);
   }

   auto selection=mp_active_tree->GetSelection();
   if(!selection)
      return;

   m_pDialogStack->SetVisible(selection->Type());

   switch(selection->Type())
   {
      case TIB_PUPPET:    m_pDlg_Puppet->SetPuppet(selection->GetPuppet()); break;
      case TIB_CHARACTER:
      {
         auto parent=mp_active_tree->GetParent(selection);
         m_pDlg_Character->SetCharacter(&parent->GetServer()->propCharacters(), selection->GetCharacter());
         break;
      }
      case TIB_SERVER:    m_pDlg_Server->SetServer(selection->GetServer()); break;
      default: Assert(0);
   }
}

HTREEITEM Wnd_Connect::InsertItem(Tree &tree, HTREEITEM htiParent, UniquePtr<ITreeItem> &&pti)
//
// Image is determined by the TreeItem (pti)
{
   Tree::Insert item;
   item.Parent(htiParent);
   item.hInsertAfter=TVI_SORT;
   item.Image(pti->Type());
   item.SelectedImage(item.item.iImage);
   item.Text(pti->Label());
   item.Param(pti);

   Assert(item.item.pszText);
   HTREEITEM htiNew=tree.InsertItem(&item);

   if(pti->Type()==TIB_SERVER)
   {
      for(auto &pPropCharacter : pti->GetServer()->propCharacters())
         InsertItem(tree, htiNew, MakeUnique<TreeItem_Character>(*pPropCharacter));
   }

   if(pti->Type()==TIB_CHARACTER)
   {
      for(auto &pPropPuppet : pti->GetCharacter()->propPuppets())
         InsertItem(tree, htiNew, MakeUnique<TreeItem_Puppet>(*pPropPuppet));
   }

   pti.Extract(); // The tree owns it now
   return htiNew;
}

void Wnd_Connect::Save()
{
   m_pDlg_Server->Save();
   m_pDlg_Character->Save();
   m_pDlg_Puppet->Save();
}

void Wnd_Connect::TreeConnect() // Connect to the currently selected item in the tree
{
   auto selection=mp_active_tree->GetSelection();
   if(!selection)
      return;

   Prop::Server    *ppropServer=nullptr;
   Prop::Character *ppropCharacter=nullptr;
   Prop::Puppet    *ppropPuppet=nullptr;

   if(selection->Type()==TIB_PUPPET)
   {
      ppropPuppet=selection->GetPuppet();
      selection=mp_active_tree->GetParent(selection);
   }

   if(selection->Type()==TIB_CHARACTER)
   {
      ppropCharacter=selection->GetCharacter();
      selection=mp_active_tree->GetParent(selection);
   }

   if(selection->Type()==TIB_SERVER)
      ppropServer=selection->GetServer();

   if(ppropPuppet && !Connection::FindCharacterConnection(ppropCharacter))
   {
      MessageBox(hWnd(), STR_PuppetNeedsCharacter, STR_Note, MB_OK|MB_ICONEXCLAMATION);
      return;
   }

   if(mp_active_tree==&m_tree)
      Save();

   m_wnd_MDI.Connect(ppropServer, ppropCharacter, ppropPuppet, true);
   m_fSetParentFocusOnClose=false;
   Close();
}

void Wnd_Connect::Import()
{
   File::Chooser cf;
   cf.SetTitle("Import World");
   cf.SetFilter("World Files (*.txt)\0*.txt\0", 0);

   FixedStringBuilder<256> filename;

   if(!cf.Choose(*this, filename, false))
      return;

   Prop::Global props;
   ErrorConsole errors;
   LoadConfig(props, filename, errors);
   if(errors.m_count)
   {
      MessageBox(*this, "Errors while loading file", "Error", MB_ICONEXCLAMATION|MB_OK);
      return;
   }

   if(!props.propConnections().propServers())
   {
      MessageBox(*this, "No worlds in file!", "Error", MB_ICONEXCLAMATION|MB_OK);
      return;
   }

   auto p_prop_server=props.propConnections().propServers().Delete(0);

   for(auto &p_character : p_prop_server->propCharacters())
      p_character->iRestoreLogIndex(-1); // We can't copy this

   auto &servers=g_ppropGlobal->propConnections().propServers();

   // Rename the object so the name is unique
   auto name=p_prop_server->pclName(); p_prop_server->pclName(""); // Reset the name so that the rename doesn't no-op
   p_prop_server->Rename(servers, name);

   m_tree.SelectItem(InsertItem(m_tree, TVI_ROOT, MakeUnique<TreeItem_Server>(*p_prop_server)));
   servers.Push(std::move(p_prop_server));
}

void Wnd_Connect::Export()
{
   auto selection=m_tree.GetSelection();
   if(!selection || selection->Type()!=TIB_SERVER)
   {
      MessageBox(*this, "Please select a world", "Note:", MB_OK|MB_ICONINFORMATION);
      return;
   }

   File::Chooser cf;
   cf.SetTitle("Export World");
   cf.SetFilter("World Files (*.txt)\0*.txt\0", 0);

   FixedStringBuilder<256> filename;

   if(!cf.Choose(*this, filename, true))
      return;

   Prop::Global props;
   props.iVersion(g_ciBuildNumber);

   props.propConnections().propServers().Push(MakeCounting<Prop::Server>(*selection->GetServer()));
   ConfigExport(filename, &props, g_ppropGlobal->fShowDefaults(), true);
}

//=============================================================================
// Window Messages

LRESULT Wnd_Connect::On(const Msg::Create &msg)
{
   // Look to see if the current world is already in the list. If not, show the 'Add current world' button.
   auto *pCurrent=m_wnd_MDI.GetActiveWindow().GetConnection().GetServer();
   bool fExistingWorld=false;
   if(pCurrent)
   {
      for(auto &&pServer : g_ppropGlobal->propConnections().propServers())
         if(pCurrent==pServer)
         {
            fExistingWorld=true;
            break;
         }
   }
   else
      fExistingWorld=true;

   m_pDlg_Server=new CDlg_Server(*this);
   m_pDlg_Character=new CDlg_Character(*this);
   m_pDlg_Puppet=new CDlg_Puppet(*this);

   InitImageList();
   m_tree.Create(*this, IDC_TREE, TVS_LINESATROOT | TVS_EDITLABELS | TVS_SHOWSELALWAYS | TVS_HASLINES | TVS_HASBUTTONS);
   InitTree(m_tree, g_ppropGlobal->propConnections().propServers());
   m_tree_samples.Create(*this, IDC_TREE_SAMPLES, TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_HASLINES | TVS_HASBUTTONS);
   InitTree(m_tree_samples, GetSampleConfig().propConnections().propServers());

   AL::Group *pG1=m_layout.CreateGroup(Direction::Horizontal); m_layout.SetRoot(pG1);
   {
      AL::Group *pG2=m_layout.CreateGroup(Direction::Vertical); *pG1 << pG2;

      AL::Splitter *p_splitter=m_layout.CreateSplitter(Direction::Vertical); *pG2 << p_splitter;
      AL::Control *p_tree=m_layout.AddControl(m_tree); *p_splitter << p_tree;
      {
         AL::Group *pGV=m_layout.CreateGroup_Vertical(); *p_splitter << pGV;
         *pGV << m_layout.CreateStatic("Sample Worlds");
         *pGV << m_layout.AddControl(m_tree_samples);
      }
      p_splitter->SetGrowObject(0);
      p_splitter->SetSize(1, Controls::Control::m_tmButtons.tmHeight*5);

      if(!fExistingWorld)
      {
         m_pAddCurrentWorld=m_layout.CreateButton(IDC_ADD_CURRENT, "Add current world");
         m_pAddCurrentWorld->weight(0);
         *pG2 << m_pAddCurrentWorld;
      }
      {
         AL::Group_Horizontal *pG1=m_layout.CreateGroup_Horizontal(); *pG2 << pG1;
         pG1->weight(0); pG1->MatchWidth(true);
         *pG1 << (m_pbtNew=m_layout.CreateButton(IDC_NEW, STR_NewEllipsis))
              << (m_pbtImport=m_layout.CreateButton(IDC_IMPORT, "Import..."))
              << (m_pbtConnect=m_layout.CreateButton(IDC_CONNECT, STR_Connect));
      }
      {
         AL::Group_Horizontal *pG1=m_layout.CreateGroup_Horizontal(); *pG2 << pG1;
         pG1->weight(0); pG1->MatchWidth(true);
         *pG1 //<< (m_pbtRename=m_layout.CreateButton(IDC_RENAME, STR_Rename))
              << (m_pbtCopy  =m_layout.CreateButton(IDC_COPY, STR_Copy))
              << (m_pbtExport=m_layout.CreateButton(IDC_EXPORT, "Export..."))
              << (m_pbtDelete=m_layout.CreateButton(IDC_DELETE, STR_Delete));
      }
   }
   {
      AL::Group *pG2=m_layout.CreateGroup(Direction::Vertical); *pG1 << pG2;
      pG2->weight(0);

      m_pDialogStack=m_layout.CreateStack();
      *m_pDialogStack << m_layout.AddObjectWindow(*m_pDlg_Server, *m_pDlg_Server)
                      << m_layout.AddObjectWindow(*m_pDlg_Character, *m_pDlg_Character)
                      << m_layout.AddObjectWindow(*m_pDlg_Puppet, *m_pDlg_Puppet);

      m_pDialogStack->weight(0); *pG2 << m_pDialogStack;

      {
         auto &gv=*m_layout.CreateGroup_Vertical(); *pG2 << &gv;
         gv >> AL::Style::Attach_Top;

         {
            AL::Group_Horizontal *pG1=m_layout.CreateGroup_Horizontal(); *pG2 << pG1;
            pG1->weight(0); pG1->MatchWidth(true);
            *pG1 << m_layout.CreateButton(ID_STATISTICS, "Statistics")
                 << m_layout.CreateButton(ID_HELP, "Help");
         }
         {
            AL::Group_Horizontal *pG1=m_layout.CreateGroup_Horizontal(); *pG2 << pG1;
            pG1->weight(0); pG1->MatchWidth(true);
            *pG1 << m_layout.CreateButton(IDOK, STR_OK)
                 << m_layout.CreateButton(IDCANCEL, STR_Cancel);
         }
      }
   }

   int2 szCurrent=m_layout.CalcMinSize()*6/5;
   SetSize(szCurrent);

   m_tree.SetFocus();
   m_pbtConnect->SetStyle(BS_DEFPUSHBUTTON, true);

   auto &connection=m_wnd_MDI.GetActiveWindow().GetConnection();
   if(auto *ppropServer=connection.GetServer())
   {
      for(auto item_server=m_tree.GetRoot();item_server;item_server=m_tree.GetNextSibling(item_server))
      {
         if(item_server->GetServer()!=ppropServer)
            continue;

         // If there is a current character, try to select that
         if(auto *ppropCharacter=connection.GetCharacter())
         {
            for(auto item_character=m_tree.GetChild(item_server);item_character;item_character=m_tree.GetNextSibling(item_character))
            {
               if(item_character->GetCharacter()!=ppropCharacter)
                  continue;

               // If there is a current character, try to select that
               if(auto *ppropPuppet=connection.GetPuppet())
               {
                  for(auto item_puppet=m_tree.GetChild(item_character); item_puppet; item_puppet=m_tree.GetNextSibling(item_puppet))
                  {
                     if(item_puppet->GetPuppet()!=ppropPuppet)
                        continue;

                     m_tree.SelectItem(item_puppet);
                     break;
                  }
               }
               else
                  m_tree.SelectItem(item_character);
               break;
            }
         }
         else // Just the server
            m_tree.SelectItem(item_server);
         break;
      }
   }

   return msg.Success();
}

LRESULT Wnd_Connect::On(const Msg::Notify &msg)
{
   Tree *p_tree{};

   switch(msg.pnmh()->idFrom)
   {
      case IDC_TREE: p_tree=&m_tree; break;
      case IDC_TREE_SAMPLES: p_tree=&m_tree_samples; break;
   }

   if(!p_tree)
      return 0;

   LPNMTREEVIEW pnmtv=(LPNMTREEVIEW)msg.pnmh();

   switch(msg.pnmh()->code)
   {
      case TVN_BEGINDRAG:
      {
         ITreeItem *pti=reinterpret_cast<ITreeItem *>(pnmtv->itemNew.lParam);
         if(pti->Type()==TIB_SERVER)
         {
            if(p_tree==&m_tree_samples)
               m_drag_from_samples=true;
            else
               return 0;
         }

         m_drag_from=Tree::ItemAndParam{pnmtv->itemNew.hItem, pti};

         Handle<HBITMAP> bitmap;
         {
            CompatibleDC dc{nullptr};
            dc.SelectFont(Controls::Control::m_font_buttons);

            int2 pad=int2(5,3)*g_dpiScale;

            int2 size=dc.TextExtent(pti->Label())+pad*2+int2(20,0);
            bitmap=CreateDIBSection(size, 32);
            dc.SelectBitmap(bitmap);

            Handle<HPEN> pen=CreatePen(PS_SOLID, 2*g_dpiScale, Colors::Black);
            dc.SelectPen(pen);

            dc.Rectangle(Rect{int2(0,0), size});
            ImageList_Draw(m_iml, pti->Type(), dc.hDC(), pad.x, size.y-pad.y-Controls::Control::m_tmButtons.tmDescent-15, ILD_NORMAL);
            dc.TextOut(pad+int2(20,0), pti->Label());
         }

         mp_drag_image=MakeUnique<Controls::DragImage>(*this, std::move(bitmap), GetCursorPos());
         SetCapture();
         return 0;
      }      

      case NM_SETFOCUS:
      case TVN_SELCHANGED:
         OnSelectionChange(*p_tree);
         return 0;

      case TVN_DELETEITEM:
         delete reinterpret_cast<ITreeItem *>(pnmtv->itemOld.lParam);
         return 0;

      case TVN_BEGINLABELEDIT:
      {
         m_tree.GetEditControl().SetCtrlId(IDC_EDIT_TREEITEM);
         // So far all items are renameable
         Wnd_Dialog::s_wndActiveDialog=nullptr;
         return 0;
      }

      case TVN_ENDLABELEDIT:
      {
         Wnd_Dialog::s_wndActiveDialog=*this;

         LPNMTVDISPINFO ptvdi=(LPNMTVDISPINFO)msg.pnmh();
         if(ptvdi->item.pszText==nullptr) // Cancelled?
            return 0;

         ITreeItem *pti=reinterpret_cast<ITreeItem *>(ptvdi->item.lParam);

         OwnedString strName=WzToString(ptvdi->item.pszText);
         auto parent=m_tree.GetParent(ptvdi->item.hItem);

         switch(pti->Type())
         {
            case TIB_PUPPET:
               pti->GetPuppet()->pclName(strName);
               m_pDlg_Puppet->RefreshName();
               break;

            case TIB_CHARACTER:
               pti->GetCharacter()->Rename(parent->GetServer()->propCharacters(), strName);
               m_pDlg_Character->RefreshName();
               break;

            case TIB_SERVER:
               pti->GetServer()->Rename(g_ppropGlobal->propConnections().propServers(), strName);
               m_pDlg_Server->RefreshName();
               break;
         }

         // In case the name had to be changed to be unique
         m_tree.SetItemText(ptvdi->item.hItem, pti->Label());
         return 0;
      }

      case TVN_KEYDOWN:
      {
         LPNMTVKEYDOWN ptvkd=(LPNMTVKEYDOWN)msg.pnmh();

         if(ptvkd->wVKey==VK_F2)
         {
            Msg::Command(IDC_RENAME, nullptr, 0).Post(*this);
            return 1;
         }
         if(ptvkd->wVKey==VK_DELETE)
         {
            Msg::Command(IDC_DELETE, nullptr, 0).Post(*this);
            return 1;
         }
         return 0;
      }     

      case NM_DBLCLK:
         Msg::Command(IDC_CONNECT, nullptr, 0).Post(*this);
         return 0;
   }

   return 0;
}

LRESULT Wnd_Connect::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()!=BN_CLICKED)
      return false;

   switch(msg.iID())
   {
      case IDC_RENAME:
      {
         if(mp_active_tree!=&m_tree)
            break;

         if(HTREEITEM hti=m_tree.GetSelection())
            m_tree.EditLabel(hti);
         break;
      }

      case IDC_NEW:
      {
         PopupMenu menu;
         menu.Append(0,         ID_NEW_SERVER, STR_Menu_Server);
         menu.Append(MF_GRAYED, ID_NEW_CHARACTER, STR_Menu_Character);
         menu.Append(MF_GRAYED, ID_NEW_PUPPET, STR_Menu_Puppet);

         if(auto selection=m_tree.GetSelection())
         {
            EnableMenuItem(menu, ID_NEW_CHARACTER, MF_BYCOMMAND|MF_ENABLED);

            if(selection->Type()==TIB_CHARACTER || selection->Type()==TIB_PUPPET)
               EnableMenuItem(menu, ID_NEW_PUPPET, MF_BYCOMMAND|MF_ENABLED);
         }

         TrackPopupMenu(menu, 0, GetCursorPos(), *this, nullptr);
         break;
      }

      case IDC_ADD_CURRENT:
      {
         Prop::Server &propServer=*m_wnd_MDI.GetActiveWindow().GetConnection().GetServer();

         // To ensure a unique name, pull out the name, set it to null, then rename it
         OwnedString name=propServer.pclName();
         propServer.pclName("");
         propServer.Rename(g_ppropGlobal->propConnections().propServers(), name);

         g_ppropGlobal->propConnections().propServers().Push(&propServer);
         m_pAddCurrentWorld->Enable(false);

         HTREEITEM hti=InsertItem(m_tree, TVI_ROOT, MakeUnique<TreeItem_Server>(propServer));
         m_tree.SelectItem(hti);
         m_tree.EditLabel(hti);
         break;
      }

      case ID_NEW_SERVER:
      {
         Prop::Servers &propServers=g_ppropGlobal->propConnections().propServers();

         HTREEITEM hti=InsertItem(m_tree, TVI_ROOT, MakeUnique<TreeItem_Server>(propServers.New()));
         m_tree.SelectItem(hti);
         m_tree.EditLabel(hti);
         break;
      }

      case ID_NEW_CHARACTER:
      {
         auto selection=m_tree.GetSelection();
         if(!selection)
            break; // Can't add a character in the root

         // Now back up to the server
         while(selection && selection->Type()!=TIB_SERVER)
            selection=m_tree.GetParent(selection);

         if(!selection) { Assert(0); break; }

         HTREEITEM hti=InsertItem(m_tree, selection, MakeUnique<TreeItem_Character>(selection->GetServer()->propCharacters().New()));
         m_tree.SelectItem(hti);
         m_tree.EditLabel(hti);
         break;
      }

      case ID_NEW_PUPPET:
      {
         auto selection=m_tree.GetSelection();
         if(!selection || selection->Type()==TIB_SERVER) break; // Can't add a puppet under a server

         while(selection && selection->Type()!=TIB_CHARACTER)
            selection=m_tree.GetParent(selection);

         if(!selection) { Assert(0); break; }

         HTREEITEM hti=InsertItem(m_tree, selection, MakeUnique<TreeItem_Puppet>(selection->GetCharacter()->propPuppets().New()));
         m_tree.SelectItem(hti);
         m_tree.EditLabel(hti);
         break;
      }

      case IDC_COPY:
         if(mp_active_tree!=&m_tree)
            break;

         if(auto selection=m_tree.GetSelection())
         {
            HTREEITEM hti{};
            switch(selection->Type())
            {
               case TIB_SERVER:
               {
                  Prop::Servers &propServers=g_ppropGlobal->propConnections().propServers();
                  hti=InsertItem(m_tree, TVI_ROOT, MakeUnique<TreeItem_Server>(propServers.Copy(*selection->GetServer())));
                  break;
               }

               case TIB_CHARACTER:
               {
                  auto selection_insert=m_tree.GetParent(selection); // We insert inside of the parent of the selection

                  Prop::Characters &propCharacters=selection_insert->GetServer()->propCharacters();
                  hti=InsertItem(m_tree, selection_insert, MakeUnique<TreeItem_Character>(propCharacters.Copy(*selection->GetCharacter())));
                  break;
               }

               case TIB_PUPPET:
               {
                  auto selection_insert=m_tree.GetParent(selection); // We insert inside of the parent of the selection

                  Prop::Puppets &propPuppets=selection_insert->GetCharacter()->propPuppets();
                  hti=InsertItem(m_tree, selection_insert, MakeUnique<TreeItem_Puppet>(propPuppets.Copy(*selection->GetPuppet())));
                  break;
               }
            }

            if(!hti)
               break;
            m_tree.SelectItem(hti);
            m_tree.EditLabel(hti);
         }
         break;

      case IDC_DELETE:
      {
         if(mp_active_tree!=&m_tree)
            break;

         if(auto selection=m_tree.GetSelection())
         {
            if(MessageBox(hWnd(), STR_AreYouSure, STR_Delete, MB_YESNO|MB_ICONQUESTION)==IDNO)
            {
               m_tree.SetFocus();
               break;
            }

            m_pDlg_Server->SetServer(nullptr);
            m_pDlg_Character->SetCharacter(nullptr, nullptr);
            m_pDlg_Puppet->SetPuppet(nullptr);

            auto parent=m_tree.GetParent(selection);
            selection->Delete(parent);
            m_tree.DeleteItem(selection);
            m_tree.SetFocus();
         }
         break;
      }

      case IDC_CONNECT: TreeConnect(); break;
      case IDC_IMPORT: Import(); break;
      case IDC_EXPORT: Export(); break;

      case IDOK:
         if(mp_active_tree==&m_tree)
            Save();
         SaveConfig(nullptr);

      case IDCANCEL:
         Close();
         break;

      case ID_STATISTICS:
         CreateDialog_Statistics(Owner());
         break;

      case ID_HELP:
         OpenHelpURL("GettingStarted.md#using-the-world-list");
         break;
   }

   return msg.Success();
}

LRESULT Wnd_Connect::On(const Msg::CaptureChanged &msg)
{
   if(msg.wndNewCapture()==*this) // Gaining the capture?
      return msg.Success();

   // We're losing the capture, act like the left mouse button was released
   if(mp_drag_image)
      Msg::LButtonUp(int2(), 0).Post(*this);

   return msg.Success();
}

LRESULT Wnd_Connect::On(const Msg::MouseMove &msg)
{
   if(mp_drag_image)
   {
      int2 screen_pos=ClientToScreen(msg);
      mp_drag_image->SetPosition(screen_pos-16);

      // Should we scroll the tree if the cursor is above/below it?
      if(!m_drag_from_samples)
      {
         Rect rcTree=m_tree.WindowRect();
      
         if(rcTree.top>screen_pos.y)
            Msg::VScroll(SB_LINEUP).Post(m_tree);
         if(rcTree.bottom<screen_pos.y)
            Msg::VScroll(SB_LINEDOWN).Post(m_tree);
      }

      // Find out if the cursor is on the item. If it is, highlight 
      // the item as a drop target. 
      TVHitTestInfo tvht;  // hit test information 
      tvht.point() = m_tree.ScreenToClient(ClientToScreen(msg));

      auto target=m_tree.HitTest(&tvht);

      if(target!=m_drag_from) // It isn't the item we're dragging from
      {
         if(target)
         {
            // No dragging a puppet to anywhere but another character
            if(m_drag_from->Type()==TIB_PUPPET && target->Type()!=TIB_CHARACTER)
               target=nullptr;

            // No dragging a character into a puppet
            if(m_drag_from->Type()==TIB_CHARACTER && target->Type()!=TIB_SERVER)
               target=nullptr;
         }

         // Only redraw if we're a different item or if the selection after changed
         if(m_drag_to!=target)
         {
            m_drag_to=target;
            m_tree.SelectDropTarget(m_drag_to);
            m_tree.Update();
         }
      } 
   }
   return msg.Success();
}

LRESULT Wnd_Connect::On(const Msg::LButtonUp &msg)
{
   if(!mp_drag_image)
      return msg.Success();

   mp_drag_image=nullptr;
   ReleaseCapture(); 
   ShowCursor(true); 

   SetOnDestroy _(m_drag_from_samples, false);
   SetOnDestroy<HTREEITEM> _(m_drag_from.m_hti, nullptr);
   SetOnDestroy<HTREEITEM> _(m_drag_to.m_hti, nullptr);

   m_tree.SelectDropTarget(nullptr);

   // Ensure it's valid
   if(!m_drag_to)
      return msg.Success();

   if(!m_drag_from_samples)
   {
      Assert(m_drag_from->Type()==TIB_CHARACTER || m_drag_from->Type()==TIB_PUPPET);
      auto drag_from_parent=m_tree.GetParent(m_drag_from);

      if(m_drag_from->Type()==TIB_PUPPET)
      {
         Assert(m_drag_to->Type()==TIB_CHARACTER);
         CntPtrTo<Prop::Puppet> ppropPuppet=m_drag_from->GetPuppet();

         // Extract
         static_cast<TreeItem_Character *>(drag_from_parent.m_param)->DeleteChild(ppropPuppet);

         // Inject
         m_drag_to->GetCharacter()->propPuppets().Push(ppropPuppet);
      }

      if(m_drag_from->Type()==TIB_CHARACTER)
      {
         Assert(m_drag_to->Type()==TIB_SERVER);
         CntPtrTo<Prop::Character> ppropCharacter=m_drag_from->GetCharacter();

         bool exists=m_drag_to->GetServer()->propCharacters().FindByName(m_drag_from->Label());
         if(exists)
         {
            MessageBox(hWnd(), STR_CharacterNameAlreadyExists, STR_Note, MB_OK|MB_ICONEXCLAMATION);
            return msg.Success();
         }

         // Extract
         static_cast<TreeItem_Server *>(drag_from_parent.m_param)->DeleteChild(ppropCharacter);

         // Inject
         m_drag_to->GetServer()->propCharacters().Push(ppropCharacter);
      }

      // Ensure we don't delete the IPropTreeItem (lParam) by nulling out the Param then deleting it
      m_tree.ExtractItemParam(m_drag_from);

      // Insert the item in the new place
      HTREEITEM htiInserted=InsertItem(m_tree, m_drag_to, UniquePtr<ITreeItem>(m_drag_from));
      m_tree.SelectItem(htiInserted);
   }
   else
   {
      Assert(m_drag_from->Type()==TIB_SERVER);
      auto *ppropServer=m_drag_from->GetServer();

      bool exists=g_ppropGlobal->propConnections().propServers().FindByName(m_drag_from->Label());
      if(exists)
      {
         MessageBox(hWnd(), STR_ServerNameAlreadyExists, STR_Note, MB_OK|MB_ICONEXCLAMATION);
         return msg.Success();
      }

      // Add a copy
      Prop::Server *p_new_server=g_ppropGlobal->propConnections().propServers().Push(MakeCounting<Prop::Server>(*ppropServer));

      HTREEITEM htiInserted=InsertItem(m_tree, TVI_ROOT, MakeUnique<TreeItem_Server>(*p_new_server));
      m_tree.SelectItem(htiInserted);
   }

   return msg.Success();
}

void CreateDialog_Connect(Window wnd, Wnd_MDI &wndMDI)
{
   if(Wnd_Connect::HasInstance())
      Wnd_Connect::GetInstance().SetFocus();
   else
      new Wnd_Connect(wnd, wndMDI);
}
