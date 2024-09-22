#include "Main.h"
#include "MCP.h"

//
namespace MCP
{

struct Package_Client : Package
{
   Package_Client(const PackageInfo &info, Parser &parser)
   :  Package(parser, info)
   {
      Message message(info.name(), "info");
      message.AddParam("name", "BeipMU");
      message.AddParam("text-version", STRINGIZE(BUILD_NUMBER));
      message.AddParam("internal-version", STRINGIZE(BUILD_NUMBER));
      parser.Send(message);
   }

   ~Package_Client()
   {
   }

   void On(const Message &msg);
};

void Package_Client::On(const Message &msg)
{
}

struct PackageFactory_Client : PackageFactory
{
   UniquePtr<Package> Create(Parser &parser, int iVersion) override;
   const PackageInfo &info() const override { return m_info; }

private:
   PackageInfo m_info{"dns-com-vmoo-client", MakeVersion(1,0), MakeVersion(1,0)};
};

UniquePtr<Package> PackageFactory_Client::Create(Parser &parser, int iVersion)
{
   return MakeUnique<Package_Client>(info(), parser);
}

UniquePtr<PackageFactory> CreatePackageFactory_dns_com_vmoo_client()
{
   return MakeUnique<PackageFactory_Client>();
}

}
