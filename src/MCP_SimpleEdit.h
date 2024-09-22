//
// MCP-simpleedit
//
namespace MCP
{

struct Editor : DLNode<Editor>
{
   Editor(Parser &parser, const Message &msg);
   ~Editor();

   Kernel::Event m_event{true, false};
   Kernel::Thread m_thread;
   ProcessInformation m_piEdit;

   void EditorClosed();

private:
   Editor(const Editor &);

   Parser &m_parser;
   OwnedString m_reference;
   OwnedString m_type;
   OwnedString m_name;

   File::Temp m_filename;
   uint64 m_lastwrite;
   DWORD m_originalsize;

   [[no_unique_address]] UIThreadMessagePoster m_poster;
   void ThreadProc();
};

struct Package_SimpleEdit : Package
{
   Package_SimpleEdit(const PackageInfo &info, Parser &parser);
   ~Package_SimpleEdit();

   OwnedDLNodeList<Editor> m_editors;

   // Package
   virtual void On(const Message &msg);
};

struct PackageFactory_SimpleEdit : PackageFactory
{
   PackageFactory_SimpleEdit();

   virtual UniquePtr<Package> Create(Parser &parser, int iVersion);
   virtual const PackageInfo &info() const { return m_info; }

private:
   PackageInfo m_info{"dns-org-mud-moo-simpleedit", MakeVersion(1,0), MakeVersion(1,0)};
};

}
