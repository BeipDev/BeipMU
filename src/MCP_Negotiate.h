//
// MCP-negotiate
//
namespace MCP
{

struct Package_Negotiate : Package
{
   Package_Negotiate(const PackageInfo &info, PackageManager &manager, Parser &parser);

   void On(const Message &msg);

   bool done() { return m_done; } // Received the mcp-negotiation-end

private:
   PackageManager &m_manager;
   bool m_done{};
};

struct PackageFactory_Negotiate : PackageFactory
{
   PackageFactory_Negotiate();

   virtual UniquePtr<Package> Create(Parser &parser, int iVersion);
   virtual const PackageInfo &info() const { return m_info; }

private:
   PackageInfo m_info{"mcp-negotiate", MakeVersion(1,0), MakeVersion(2,0)};
};

}
