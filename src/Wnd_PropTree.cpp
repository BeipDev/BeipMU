//
// Property Tree Window (Triggers, Macros, and Aliases)
//

#include "Main.h"
#include "Wnd_PropTree.h"

struct Wnd_PropTree : ITreeView, Wnd_Dialog
{
   Wnd_PropTree(Window wndParent, UniquePtr<IPropTree> pIPropTree, ConstString title);

   Window GetWindow() override { return *this; }
   IPropTree &GetPropTree() override { return *mp_prop_tree; }
   void SelectItem(Prop::Server *ppropServer, Prop::Character *ppropCharacter, TCallback<bool(IPropTreeItem&)> *callback) override;

private:

   ~Wnd_PropTree() noexcept;

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Notify &msg);
   LRESULT On(const Msg::MouseMove &msg);
   LRESULT On(const Msg::LButtonUp &msg);
   LRESULT On(const Msg::RButtonUp &msg);
   LRESULT On(const Msg::CaptureChanged &msg);

   using Tree=Controls::TreeView<IPropTreeItem*>;

   void InitImageList();
   void InitTree(Tree &tree, UniquePtr<IPropTreeItem> &&root);
   HTREEITEM InsertItem(Tree &tree, HTREEITEM htiParent, HTREEITEM htiInsertAfter, UniquePtr<IPropTreeItem> &&pti);
   void UpdateItem(Tree::ItemAndParam item);
   void UpdateSelection() override; // Only applies to the non sample tree
   void UpdateEnabled();
   void ApplyChanges();

   UniquePtr<IPropTree> mp_prop_tree;
   IPropDlg *mp_prop_dialog;

   enum
   {
      IDC_NEW = 100,
      IDC_COPY,
      IDC_RENAME,
      IDC_DELETE,
      IDC_IMPORT,
      IDC_EXPORT,
      IDC_TREE,
      IDC_TREE_SAMPLES,
      IDC_EDIT_TREEITEM,
      IDC_APPLY,
   };

   Tree m_tree;
   Tree m_tree_samples;
   Tree *mp_active_tree{&m_tree}; // The tree that holds the item currently on display

   AL::Button *m_pbtDelete, *m_pbtApply;

   ImageList m_iml;

   // Dragging
   bool m_drag_from_samples{};
   Tree::ItemAndParam m_drag_from; // Item being dragged (set to nullptr if dragging from samples)
   Tree::ItemAndParam m_drag_to;   // Item it's being dragged to

   enum DragMethod
   {
      DRAG_None,
      DRAG_Before,
      DRAG_After,
      DRAG_Into
   };

   DragMethod m_dragmethod;

   UniquePtr<Controls::DragImage> mp_drag_image;
};

LRESULT Wnd_PropTree::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command, Msg::Notify, Msg::MouseMove, Msg::LButtonUp, Msg::RButtonUp, Msg::CaptureChanged>(msg);
}

Wnd_PropTree::Wnd_PropTree(Window wndParent, UniquePtr<IPropTree> pIPropTree, ConstString title)
:  mp_prop_tree(std::move(pIPropTree))
{
   Create(title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0 /*dwExStyle*/, nullptr);
   CenterInWindow(wndParent);
}

Wnd_PropTree::~Wnd_PropTree()
{
   m_tree.Destroy(); // To handle tree item delete callbacks before we are deleted
   m_tree_samples.Destroy();
}

void Wnd_PropTree::InitImageList()
{
   IconDrawer id{g_ppropGlobal->UIFontSize()*g_dpiScale};

   m_iml=id.CreateImageList(TIB_MAX);
   id.Add("📁", m_iml);
   id.Add("📂", m_iml);
   id.Add("🖥️", m_iml);
   id.Add("👤", m_iml);
   id.Add("⚙️", m_iml);
   id.Add("💱", m_iml);
   id.Add("⌨️", m_iml);
   id.Add("🚫", m_iml);

   Assert(m_iml.GetCount()==TIB_MAX);
}

void Wnd_PropTree::InitTree(Tree &tree, UniquePtr<IPropTreeItem> &&root)
{
   tree.SetImageList(m_iml, TVSIL_NORMAL);

   HTREEITEM htiRoot=InsertItem(tree, TVI_ROOT, TVI_SORT, std::move(root));
   tree.Expand(htiRoot, TVE_EXPAND);
}

HTREEITEM Wnd_PropTree::InsertItem(Tree &tree, HTREEITEM htiParent, HTREEITEM htiInsertAfter, UniquePtr<IPropTreeItem> &&pti)
{
   Tree::Insert insert;

   insert.Parent(htiParent);
   insert.InsertAfter(htiInsertAfter);
   insert.Image(pti->eBitmap());
   insert.SelectedImage(pti->eBitmap());
   insert.Param(pti);
   insert.Text(pti->Label() ? pti->Label() : STR_Empty);

   HTREEITEM htiInserted=tree.InsertItem(insert);

   // Add any children of this item
   struct { Wnd_PropTree *pTree; Tree &tree; HTREEITEM htiInserted; } data={ this, tree, htiInserted };
   auto callback=[&data](UniquePtr<IPropTreeItem> &&child)
   {
      data.pTree->InsertItem(data.tree, data.htiInserted, child->fSort() ? TVI_SORT : TVI_FIRST, std::move(child));
   };

   // Recursively add the common items for all trees to the tree (like servers/characters) Typically sorted
   pti->GetChildren(callback);
   // Recursively add the tree specific items to the tree (triggers/macros/aliases)
   mp_prop_tree->GetChildren(callback, *pti);

   auto callback_last=[&data](UniquePtr<IPropTreeItem> &&child)
   {
      Assert(!child->fSort());
      data.pTree->InsertItem(data.tree, data.htiInserted, TVI_LAST, std::move(child));
   };

   // Recursively add the tree specific items to the tree (triggers/macros/aliases)
   mp_prop_tree->GetPostChildren(callback_last, *pti);

   pti.Extract(); // The tree owns it now
   return htiInserted;
}

void Wnd_PropTree::SelectItem(Prop::Server *ppropServer, Prop::Character *ppropCharacter, TCallback<bool(IPropTreeItem&)> *callback)
{
   m_tree.SetFocus();
   auto item=m_tree.GetRoot();

   if(ppropServer)
   {
      for(item=m_tree.GetChild(item);item;item=m_tree.GetNextSibling(item))
      {
         if(item->GetServer()==ppropServer)
            break;
      }

      if(item && ppropCharacter)
      {
         for(item=m_tree.GetChild(item);item;item=m_tree.GetNextSibling(item))
         {
            if(item->GetCharacter()==ppropCharacter)
               break;
         }
      }
   }

   Assert(item);
   if(!item)
      return;

   if(!callback)
   {
      m_tree.SelectItem(item);
      return;
   }

   for(item=m_tree.GetChild(item);item;item=m_tree.GetNextSibling(item))
   {
      if((*callback)(*item))
      {
         m_tree.SelectItem(item);
         return;
      }
   }
   Assert(false);
}

void Wnd_PropTree::UpdateItem(Tree::ItemAndParam p_item)
{
   Tree::Item item;
   item.Handle(p_item);
   item.Text(p_item->Label());
   if(!item.pszText)
      item.Text(STR_Empty);

   eTIB bitmap=p_item->eBitmap();
   if(bitmap==TIB_FOLDER_CLOSED && m_tree.IsExpanded(p_item))
      bitmap=TIB_FOLDER_OPEN;
   item.Image(bitmap);
   item.SelectedImage(bitmap);
   m_tree.SetItem(&item);
}

void Wnd_PropTree::UpdateSelection()
{
   auto selection=m_tree.GetSelection();
   Assert(selection);
   if(!selection)
      return;

   UpdateItem(selection);
}

void Wnd_PropTree::ApplyChanges()
{
   if(mp_active_tree!=&m_tree)
      return;

   mp_prop_dialog->SetProp(nullptr);
   mp_prop_dialog->SetProp(m_tree.GetSelection());
   UpdateSelection();
}

void Wnd_PropTree::UpdateEnabled()
{
   bool fMainTree=mp_active_tree==&m_tree;
   EnableWindows(fMainTree, *m_pbtDelete, *m_pbtApply);
}

LRESULT Wnd_PropTree::On(const Msg::Create &msg)
{
   mp_prop_dialog=mp_prop_tree->CreateDialog(*this, this);

   InitImageList();
   m_tree.Create(*this, IDC_TREE, TVS_EDITLABELS | TVS_SHOWSELALWAYS | TVS_HASLINES | TVS_HASBUTTONS);
   InitTree(m_tree, MakeUnique<PropTreeItem_Global>(*g_ppropGlobal, STR_Global));
   m_tree_samples.Create(*this, IDC_TREE_SAMPLES, TVS_SHOWSELALWAYS | TVS_HASLINES | TVS_HASBUTTONS);
   InitTree(m_tree_samples, MakeUnique<PropTreeItem_Global>(GetSampleConfig(), "Samples"));

   AL::Splitter *p_hsplitter=m_layout.CreateSplitter(Direction::Horizontal); m_layout.SetRoot(p_hsplitter);
   {
      AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *p_hsplitter << pGV;

      AL::Splitter *p_splitter=m_layout.CreateSplitter(Direction::Vertical); *pGV << p_splitter;
      AL::Control *p_tree=m_layout.AddControl(m_tree); *p_splitter << p_tree;
      AL::Control *p_tree_samples=m_layout.AddControl(m_tree_samples); *p_splitter << p_tree_samples;
      p_splitter->SetGrowObject(0);
      p_splitter->SetSize(1, Controls::Control::m_tmButtons.tmHeight*7);

      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->MatchWidth(true); pGH->weight(0);
      *pGH << m_layout.CreateButton(IDC_NEW, STR_New);
      *pGH << m_layout.CreateButton(IDC_COPY, STR_Copy);
      *pGH << (m_pbtDelete=m_layout.CreateButton(IDC_DELETE, STR_Delete));
      *pGH << m_layout.CreateButton(IDC_IMPORT, "Import…");
      *pGH << m_layout.CreateButton(IDC_EXPORT, "Export…");
   }
   {
      auto *pGHPad=m_layout.CreateGroup_Horizontal(); *p_hsplitter << pGHPad;
      auto *p_spacer=m_layout.CreateSpacer(); *pGHPad << p_spacer; p_spacer->weight(0);
      AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGHPad << pGV;
      *pGV << mp_prop_dialog;

      AL::Group_Horizontal *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);
      pGH->MatchWidth(true);

      *pGH << (m_pbtApply=m_layout.CreateButton(IDC_APPLY, STR_Apply));
      *pGH << m_layout.CreateButton(IDOK, STR_OK);
      *pGH << m_layout.CreateButton(IDCANCEL, STR_Cancel);
      *pGH << m_layout.CreateButton(IDHELP, "Help");
   }
   p_hsplitter->SetGrowObject(1);
//   p_hsplitter->SetSize(1, Controls::Control::m_tmButtons.tmHeight*7);

   PinAbove<int>(m_tree.szMinimum().x, Windows::Controls::Control::m_tmButtons.tmAveCharWidth*45);
   m_tree.SetFocus();
   return msg.Success();
}

LRESULT Wnd_PropTree::On(const Msg::Command &msg)
{
   if(msg.uCodeNotify()!=BN_CLICKED)
      return false;

   switch(msg.iID())
   {
      case IDC_APPLY:
         ApplyChanges();
         break;

      case IDHELP:
         mp_prop_tree->OnHelp();
         break;

      case IDOK:
         mp_prop_dialog->SetProp(nullptr, mp_active_tree==&m_tree);
         SaveConfig(nullptr);
      case IDCANCEL:
         Close();
         break;

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
         auto selection=m_tree.GetSelection();
         if(!selection)
            break; // No selection

         auto ptiNew=mp_prop_tree->NewChild(*selection);

         if(!ptiNew) // Can't create a child?  Try using the parent
         {
            selection=m_tree.GetParent(selection);
            ptiNew=mp_prop_tree->NewChild(*selection);
         }

         // Add the item before any servers or characters in this group
         Tree::ItemAndParam insert_after;
         if(!ptiNew->ppropTrigger() && !ptiNew->ppropAlias())
         {
            for(insert_after=m_tree.GetChild(selection);insert_after;insert_after=m_tree.GetNextSibling(insert_after))
            {
               if(insert_after->eBitmap()==TIB_SERVER || insert_after->eBitmap()==TIB_CHARACTER)
               {
                  // Backup to the last item we were the same as
                  insert_after=m_tree.GetPrevSibling(insert_after);
                  break;
               }
            }
         }
         else
            insert_after.m_hti=TVI_LAST;

         HTREEITEM htiNew=InsertItem(m_tree, selection, insert_after ? insert_after : TVI_FIRST, std::move(ptiNew));
         m_tree.SelectItem(htiNew);
         UpdateItem(selection);
         break;
      }

      case IDC_COPY:
      {
         auto child=m_tree.GetSelection();
         if(!child)
            break; // No selection to copy to
         auto parent=m_tree.GetParent(child);
         if(!parent) // Root is selected, so make the parent be the root, and the child be the top of the root
         {
            parent=child;
            child={TVI_FIRST, nullptr};
         }

         auto copy=mp_active_tree->GetSelection();
         if(!copy)
            break; // No selection to copy from
         auto copy_parent=mp_active_tree->GetParent(copy);
         if(!copy_parent)
            break; // All copyable items have a parent

         ApplyChanges();

         auto ptiNew=mp_prop_tree->CopyChild(*copy_parent, *copy);
         if(!ptiNew) // Can't create a child
            break;

         mp_prop_tree->InsertChild(*parent, ptiNew, mp_active_tree==&m_tree ? child.m_param : nullptr, true);
         HTREEITEM htiNew=InsertItem(m_tree, parent, child, std::move(ptiNew));
         m_tree.SelectItem(htiNew);
         m_tree.SetFocus();
         break;
      }

      case IDC_DELETE:
      {
         if(mp_active_tree!=&m_tree)
            break;

         auto child=m_tree.GetSelection();
         if(!child)
            break; // No selection

         mp_prop_dialog->SetProp(nullptr);

         if(!child->fCanDelete())
            break; // Can't delete

         if(mp_prop_tree->DeleteChild(*m_tree.GetParent(child), child, *this))
            m_tree.DeleteItem(child);
         break;
      }

      case IDC_IMPORT:
      {
         auto selection=m_tree.GetSelection();
         if(!selection)
            break; // No selection

         ApplyChanges();

         File::Chooser cf;
         cf.SetTitle("Import");
         cf.SetFilter("Config Files (*.txt)\0*.txt\0", 0);

         FixedStringBuilder<256> filename;

         if(!cf.Choose(*this, filename, false))
            return {};

         Prop::Global props;
         ErrorConsole errors;
         LoadConfig(props, filename, errors);
         if(errors.m_count)
         {
            MessageBox(*this, "Errors while loading file", "Error", MB_ICONEXCLAMATION|MB_OK);
            return {};
         }

         auto ptiNew=mp_prop_tree->Import(*selection, props, *this);
         if(!ptiNew)
            break;

         // Add the item before any servers or characters in this group
         Tree::ItemAndParam insert_after;
         for(insert_after=m_tree.GetChild(selection);insert_after;insert_after=m_tree.GetNextSibling(insert_after))
         {
            if(insert_after->eBitmap()==TIB_SERVER || insert_after->eBitmap()==TIB_CHARACTER)
            {
               // Backup to the last item we were the same as
               insert_after=m_tree.GetPrevSibling(insert_after);
               break;
            }
         }

         HTREEITEM htiNew=InsertItem(m_tree, selection, insert_after ? insert_after: TVI_LAST, std::move(ptiNew));
         m_tree.SelectItem(htiNew);
         UpdateItem(selection);
         break;
      }

      case IDC_EXPORT:
      {
         auto selection=m_tree.GetSelection();
         if(!selection)
            break; // No selection

         File::Chooser cf;
         cf.SetTitle("Export");
         cf.SetFilter("Config Files (*.txt)\0*.txt\0", 0);

         FixedStringBuilder<256> filename;

         if(!cf.Choose(*this, filename, true))
            break;

         Prop::Global props;
         props.Version(g_build_number);

         mp_prop_tree->Export(*selection, props);

         ConfigExport(filename, &props, g_ppropGlobal->fShowDefaults(), true);
         break;
      }
   }
   return true;
}

LRESULT Wnd_PropTree::On(const Msg::Notify &msg)
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
         IPropTreeItem *pti=m_tree.GetItemParam(pnmtv->itemNew.hItem);

         if(!pti->fCanMove())
            return 0;

         m_drag_from=Tree::ItemAndParam{pnmtv->itemNew.hItem, pti};
         m_drag_from_samples=p_tree==&m_tree_samples;

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
            ImageList_Draw(m_iml, pti->eBitmap(), dc.hDC(), pad.x, size.y-pad.y-Controls::Control::m_tmButtons.tmDescent-15, ILD_NORMAL);
            dc.TextOut(pad+int2(20,0), pti->Label());
         }

         mp_drag_image=MakeUnique<Controls::DragImage>(*this, std::move(bitmap), GetCursorPos());
         SetCapture();
         return 0;
      }

      case NM_RCLICK:
      {
         TVHitTestInfo tvht; // hit test information 

                             // Find out if the cursor is on the item. If it is, highlight 
                             // the item as a drop target. 
         tvht.point()=m_tree.ScreenToClient(Windows::GetMessagePos());
         auto target=m_tree.HitTest(&tvht);
         if(!target)
            return 0;

         m_tree.SelectItem(target);
         if(!target->ppropTrigger() && !target->ppropAlias())
            return 0;

         enum struct Commands : UINT_PTR
         {
            MoveToTop=1,
            MoveToBottom,
         };

         PopupMenu menu;
         menu.Append(!m_tree.GetPrevSibling(target) ? MF_GRAYED : 0, (UINT_PTR)Commands::MoveToTop, "Move to top");
         menu.Append(!m_tree.GetNextSibling(target) ? MF_GRAYED : 0, (UINT_PTR)Commands::MoveToBottom, "Move to bottom");

         switch(auto command=Commands(TrackPopupMenu(menu, TPM_RETURNCMD, GetCursorPos(), *this, nullptr));command)
         {
            case Commands::MoveToTop:
            case Commands::MoveToBottom:
            {
               auto parent=m_tree.GetParent(target);
               auto item=UniquePtr<IPropTreeItem>(m_tree.ExtractItemParam(target));
               mp_prop_tree->ExtractChild(*parent, item);

               Tree::ItemAndParam insert_after{};
               if(command==Commands::MoveToBottom)
               {
                  insert_after=m_tree.GetChild(parent); Assert(insert_after);
                  while(auto next=m_tree.GetNextSibling(insert_after))
                     insert_after=next;
               }

               mp_prop_tree->InsertChild(*parent, item, insert_after, true);
               auto moved=InsertItem(m_tree, parent, insert_after ? insert_after : TVI_FIRST, std::move(item));
               m_tree.SelectItem(moved);
               break;
            }
         }
         return 0;
      }

      case NM_SETFOCUS:
         if(mp_active_tree!=p_tree)
         {
            Tree *pInactive=mp_active_tree;
            mp_active_tree=p_tree;
            UpdateEnabled();

            if(mp_prop_dialog->SetProp(p_tree->GetSelection(), mp_active_tree==&m_tree))
            {
               Assert(pInactive==&m_tree); // The samples tree should never be changed
               UpdateItem(pInactive->GetSelection());
            }
         }
         return 0;

      case TVN_SELCHANGED:
         if(mp_prop_dialog->SetProp(reinterpret_cast<IPropTreeItem *>(pnmtv->itemNew.lParam), mp_active_tree==&m_tree) && pnmtv->itemOld.lParam)
         {
            Assert(p_tree==&m_tree); // The samples tree should never be changed
            UpdateItem({pnmtv->itemOld.hItem, m_tree.GetItemParam(pnmtv->itemOld.hItem)});
         }

         if(mp_active_tree!=p_tree)
         {
            mp_active_tree=p_tree;
            UpdateEnabled();
         }
         return 0;

      case TVN_DELETEITEM:
         delete reinterpret_cast<IPropTreeItem *>(pnmtv->itemOld.lParam);
         return 0;

      case TVN_BEGINLABELEDIT:
      {
         LPNMTVDISPINFO ptvdi=(LPNMTVDISPINFO)msg.pnmh();

         const IPropTreeItem *pti=reinterpret_cast<IPropTreeItem *>(ptvdi->item.lParam);
         if(!pti->fCanRename()) return TRUE;

         m_tree.GetEditControl().SetCtrlId(IDC_EDIT_TREEITEM);
         // We don't want to edit the (Empty) string, so set it to nothing if we're that
         if(!pti->Label())
            m_tree.GetEditControl().SetText("");
         
         Wnd_Dialog::s_wndActiveDialog=nullptr;
         return 0;
      }

      case TVN_ENDLABELEDIT:
      {
         Wnd_Dialog::s_wndActiveDialog=*this;
         LPNMTVDISPINFO ptvdi=(LPNMTVDISPINFO)msg.pnmh();

         if(ptvdi->item.pszText==nullptr) // Cancelled?
            return 0;

         IPropTreeItem *pti=reinterpret_cast<IPropTreeItem *>(ptvdi->item.lParam);
         Assert(pti->fCanRename());

         UTF8 string(WzToString(ptvdi->item.pszText));
         mp_prop_dialog->SetProp(nullptr);
         pti->Rename(string);
         mp_prop_dialog->SetProp(pti);

         Tree::Item item;
         item.Handle(ptvdi->item.hItem);
         item.Text(ptvdi->item.pszText);
         if(!string)
            item.pszText=UnconstRef(WIDEN(STR_Empty));

         m_tree.SetItem(&item);
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

      case TVN_ITEMEXPANDED:
      {
         const IPropTreeItem *pti=reinterpret_cast<IPropTreeItem *>(pnmtv->itemNew.lParam);

         // Non Folder Item?
         if(pti->eBitmap()!=TIB_FOLDER_CLOSED) return 0;

         Tree::Item item;
         item.Handle(pnmtv->itemNew.hItem);
         item.Image(pnmtv->action==TVE_COLLAPSE ? TIB_FOLDER_CLOSED : TIB_FOLDER_OPEN);
         item.SelectedImage(item.iImage);
         m_tree.SetItem(&item);
         return 0;
      }
   }

   return 0;
}

LRESULT Wnd_PropTree::On(const Msg::MouseMove &msg)
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

      TVHitTestInfo tvht; // hit test information 

      // Find out if the cursor is on the item. If it is, highlight 
      // the item as a drop target. 
      tvht.point()=m_tree.ScreenToClient(ClientToScreen(msg.position()));

      auto target=m_tree.HitTest(&tvht);

      if(target!=m_drag_from) // It isn't the item we're dragging from
      {
         DragMethod dragmethod=DRAG_None;

         if(target)
         {
            Rect rcItem;
            m_tree.GetItemRect(target, &rcItem, false);

            auto target_parent=m_tree.GetParent(target);

            // We can Drag_Into if the target can hold us
            bool can_hold=mp_prop_tree->fCanHold(*target, m_drag_from);
            // We can Drag_Before/After if we are not sorted and the parent of the target can hold us
            // And, that the target can also be moved.
            bool can_move=!m_drag_from->fSort() && (target_parent ? mp_prop_tree->fCanHold(*target_parent, m_drag_from) : false)
                          && mp_prop_tree->fCanHold(*target_parent, target);

            bool can_move_after=can_move;
            // If we can't move, are we trying to drag past the end of the world/character/puppet list?
            if(!can_move && target_parent && (m_drag_from->ppropTrigger() || m_drag_from->ppropAlias()))
            {
               if(!m_tree.GetNextSibling(target))
                  can_move_after=true;
            }

            // Make sure the source is not the parent of the target (can't pull the rug out from ourselves)
            {
               auto item=target;
               while(item=m_tree.GetParent(item))
               {
                  if(item==m_drag_from)
                  {
                     can_hold=can_move=false;
                     break;
                  }
               }
            }

            bool before=tvht.point().y < rcItem.top+rcItem.size().y/3;
            bool after=tvht.point().y > rcItem.top+(rcItem.size().y*2)/3;

            if(can_hold)
               dragmethod=DRAG_Into;
            if(can_move)
            {
               if(before)
                  dragmethod=DRAG_Before;
               if(after)
                  dragmethod=DRAG_After;
            }
            if(can_move_after && after)
               dragmethod=DRAG_After;
         }

         // Only redraw if we're a different item or if the selection after changed
         if(m_drag_to!=target || m_dragmethod!=dragmethod)
         {
            if(m_dragmethod==DRAG_Into)
               m_tree.SelectDropTarget(nullptr);
            else if(m_dragmethod==DRAG_Before || m_dragmethod==DRAG_After)
               m_tree.SetInsertMark(nullptr, 0);

            m_drag_to=target;
            m_dragmethod=dragmethod;

            if(m_dragmethod==DRAG_Into)
               m_tree.SelectDropTarget(m_drag_to);
            else if(m_dragmethod==DRAG_Before || m_dragmethod==DRAG_After)
               m_tree.SetInsertMark(m_drag_to, !(m_dragmethod==DRAG_Before));

            m_tree.Update();
         }
      } 
   }
   return msg.Success();
}

LRESULT Wnd_PropTree::On(const Msg::LButtonUp &msg)
{
   if(!mp_drag_image)
      return msg.Success();

   mp_drag_image=nullptr;
   ReleaseCapture(); 
   ShowCursor(true);

   SetOnDestroy _(m_drag_from_samples, false);
   SetOnDestroy<HTREEITEM> _(m_drag_from.m_hti, nullptr);
   SetOnDestroy<HTREEITEM> _(m_drag_to.m_hti, nullptr);

   if(m_dragmethod==DRAG_None)
      return msg.Success();
   else if(m_dragmethod==DRAG_Into)
      m_tree.SelectDropTarget(nullptr);
   else if(m_dragmethod==DRAG_Before || m_dragmethod==DRAG_After)
      m_tree.SetInsertMark(nullptr, 0);

   // Ensure it's valid
   if(!m_drag_to)
      return msg.Success();

   UniquePtr<IPropTreeItem> pti;
   if(!m_drag_from_samples)
   {
      // Ensure we're not dragging into a child of ourselves
      {
         auto parent=m_drag_to;
         while(parent=m_tree.GetParent(parent))
         {
            if(parent==m_drag_from)
               return msg.Success(); // Can't insert this item deeper into itself
         }
      }

      // Pull the old item out from where it was
      auto drag_from_parent=m_tree.GetParent(m_drag_from);
      pti=UniquePtr<IPropTreeItem>(m_tree.ExtractItemParam(m_drag_from));
      Assert(pti->fCanMove());

      // Extract
      mp_prop_tree->ExtractChild(*drag_from_parent, pti);
   }
   else
   {
      auto drag_from_parent=m_tree_samples.GetParent(m_drag_from);
      pti=mp_prop_tree->CopyChild(*drag_from_parent, *m_drag_from);
   }

   // Insert the item in the new place
   HTREEITEM htiInserted=nullptr;

   if(m_dragmethod==DRAG_Into)
   {
      mp_prop_tree->InsertChild(*m_drag_to, pti, nullptr, false);
      htiInserted=InsertItem(m_tree, m_drag_to, TVI_FIRST, std::move(pti));
      UpdateItem(m_drag_to);
   }
   else
   {
      auto drag_to_parent=m_tree.GetParent(m_drag_to);

      mp_prop_tree->InsertChild(*drag_to_parent, pti, m_drag_to, m_dragmethod==DRAG_After ? true : false);

      HTREEITEM htiInsertAfter=m_drag_to;
      if(m_dragmethod==DRAG_Before)
      {
         htiInsertAfter=m_tree.GetPrevSibling(htiInsertAfter);
         if(htiInsertAfter==nullptr)
            htiInsertAfter=TVI_FIRST;
      }

      htiInserted=InsertItem(m_tree, drag_to_parent, htiInsertAfter, std::move(pti));
   }
   m_tree.SelectItem(htiInserted);
   m_tree.SetFocus();

   return msg.Success();
}

LRESULT Wnd_PropTree::On(const Msg::RButtonUp &msg)
{
   ReleaseCapture(); 
   return msg.Success();
}

LRESULT Wnd_PropTree::On(const Msg::CaptureChanged &msg)
{
   if(msg.wndNewCapture()==*this) // Gaining the capture?
      return msg.Success();

   // We're losing the capture, act like the left mouse button was released
   if(mp_drag_image)
      Msg::LButtonUp(int2(), 0).Post(*this);

   return msg.Success();
}

ITreeView *CreateWindow_PropTree(Window wnd, UniquePtr<IPropTree> pIPropTree, ConstString title)
{
   return new Wnd_PropTree(wnd, std::move(pIPropTree), title);
}
