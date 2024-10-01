//
// Property Tree Window (Triggers, Macros, and Aliases)
//

enum eTIB // Tree Item Bitmaps
{
   TIB_FOLDER_CLOSED, // Use this to get animated folders!
   TIB_FOLDER_OPEN,
   TIB_SERVER,
   TIB_CHARACTER,
   TIB_TRIGGER,
   TIB_ALIAS,
   TIB_KEYMACRO,
   TIB_DISABLED,
   TIB_MAX
};

interface IPropDlg;
interface IPropTree;

interface IPropTreeItem
{
   virtual ~IPropTreeItem() noexcept { }

   virtual bool fCanDelete() const { return false; }
   virtual bool fCanRename() const { return false; }
   virtual bool fCanMove() const { return false; } // Item can be rearranged in the list (and only into an item that can hold it!)

   virtual ConstString Label() const=0;
   virtual void Rename(ConstString string) { Assert(0); }

   virtual void GetChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback) { }

   virtual bool fSort() const=0;
   virtual eTIB eBitmap() const=0;

   virtual Prop::Server *GetServer() { return nullptr; }
   virtual Prop::Character *GetCharacter() { return nullptr; }

   virtual Prop::Alias *ppropAlias() { return nullptr; }
   virtual CKeyMacro *pKeyMacro() { return nullptr; }
   virtual Prop::Trigger *ppropTrigger() { return nullptr; }

   virtual Prop::Aliases *ppropAliases() { return nullptr; }
   virtual Prop::KeyboardMacros *ppropKeyboardMacros() { return nullptr; }
   virtual Prop::Triggers *ppropTriggers() { return nullptr; }
};

// Passed in by the Wnd_PropTree so that updates in the dialog
// can be reflected in the tree
//
interface ITreeView
{
   virtual Window GetWindow()=0;
   virtual void UpdateSelection()=0;
   virtual IPropTree &GetPropTree()=0;

   virtual void SelectItem(Prop::Server *ppropServer, Prop::Character *ppropCharacter, TCallback<bool(IPropTreeItem&)> *callback)=0;
};

interface IPropTree
{
   virtual ~IPropTree()=default;

   virtual void OnHelp() { OpenHelpURL("README.md"); }

   virtual UniquePtr<IPropTreeItem> Import(IPropTreeItem &item, Prop::Global &props, Window window)=0;
   virtual void Export(IPropTreeItem &item, Prop::Global &props)=0;

   virtual IPropDlg *CreateDialog(Window wndParent, ITreeView *pITreeView)=0;
   virtual void GetChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback, IPropTreeItem &item)=0;
   virtual void GetPostChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback, IPropTreeItem &item) { }
   virtual UniquePtr<IPropTreeItem> NewChild(IPropTreeItem &item)=0;
   virtual UniquePtr<IPropTreeItem> CopyChild(IPropTreeItem &item, IPropTreeItem &child) { return nullptr; }

   virtual bool DeleteChild(IPropTreeItem &item, IPropTreeItem *pti, Window wnd)=0; // wnd is provided as a parent window in case a message box needs to be shown
   virtual void ExtractChild(IPropTreeItem &item, IPropTreeItem *pti)=0;
   virtual void InsertChild(IPropTreeItem &item, IPropTreeItem *ptiChild, IPropTreeItem *ptiInsertNextTo, bool fAfter)=0;
   virtual bool fCanHold(IPropTreeItem &item, IPropTreeItem *pti) { return false; } // This item can hold this other item
};

interface IPropDlg : Wnd_ChildDialog
{
   virtual bool SetProp(IPropTreeItem *pti, bool fKeepChanges=true)=0; // Return true to indicate the selection display needs to be updated
};

struct PropTreeItem_Character : IPropTreeItem
{
   PropTreeItem_Character(Prop::Character &propCharacter) : m_ppropCharacter(&propCharacter) { }

   ConstString Label() const { return m_ppropCharacter->pclName(); }

   Prop::Character *GetCharacter() override { return m_ppropCharacter; }

   Prop::KeyboardMacros *ppropKeyboardMacros() override { return &m_ppropCharacter->propKeyboardMacros(); }
   Prop::Aliases *ppropAliases() override { return &m_ppropCharacter->propAliases(); }
   Prop::Triggers *ppropTriggers() override { return &m_ppropCharacter->propTriggers(); }

   bool fSort() const override { return true; }
   eTIB eBitmap() const override { return TIB_CHARACTER; }

private:

   CntPtrTo<Prop::Character> m_ppropCharacter;
};

struct PropTreeItem_Server : IPropTreeItem
{
   PropTreeItem_Server(Prop::Server &propServer) : m_ppropServer(&propServer) { }

   ConstString Label() const override { return m_ppropServer->pclName(); }

   void GetChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback) override
   {
      for(auto &pCharacter : m_ppropServer->propCharacters())
         callback(MakeUnique<PropTreeItem_Character>(*pCharacter));
   }

   Prop::Server *GetServer() override { return m_ppropServer; }

   Prop::KeyboardMacros *ppropKeyboardMacros() override { return &m_ppropServer->propKeyboardMacros(); }
   Prop::Aliases *ppropAliases() override { return &m_ppropServer->propAliases(); }
   Prop::Triggers *ppropTriggers() override { return &m_ppropServer->propTriggers(); }

   bool fSort() const override { return true; }
   eTIB eBitmap() const override { return TIB_SERVER; }

private:
   CntPtrTo<Prop::Server> m_ppropServer;
};

struct PropTreeItem_Global : IPropTreeItem
{
   PropTreeItem_Global(Prop::Global &propsGlobal, ConstString label) : m_propsGlobal(propsGlobal), m_label(label) { }

   ConstString Label() const override { return m_label; }

   void GetChildren(TCallback<void(UniquePtr<IPropTreeItem>&&)> callback) override
   {
      for(auto &pServer : m_propsGlobal.propConnections().propServers())
         callback(MakeUnique<PropTreeItem_Server>(*pServer));
   }

   Prop::KeyboardMacros *ppropKeyboardMacros() override { return &m_propsGlobal.propConnections().propKeyboardMacros(); }
   Prop::Triggers *ppropTriggers() override { return &m_propsGlobal.propConnections().propTriggers(); }
   Prop::Aliases *ppropAliases() override { return &m_propsGlobal.propConnections().propAliases(); }

   bool fSort() const override { return false; }
   eTIB eBitmap() const override { return TIB_FOLDER_CLOSED; }

   Prop::Global &m_propsGlobal;
   ConstString m_label;
};

ITreeView *CreateWindow_PropTree(Window wndOwner, UniquePtr<IPropTree> pIPropTree, ConstString label);
