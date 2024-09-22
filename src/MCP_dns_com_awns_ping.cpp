#include "Main.h"
#include "MCP.h"

//
namespace MCP
{

struct Package_Ping : Package
{
   Package_Ping(const PackageInfo &info, Parser &parser)
   :  Package(parser, info)
   {
   }

   ~Package_Ping()
   {
   }

   void On(const Message &msg);

};

void Package_Ping::On(const Message &msg)
{
   Message message(info().name(), "reply");
   message.AddParam("id", msg["id"]);
   m_parser.Send(message);
}

struct PackageFactory_Ping : PackageFactory
{
   UniquePtr<Package> Create(Parser &parser, int iVersion) override;
   const PackageInfo &info() const override { return m_info; }

private:
   PackageInfo m_info{"dns-com-awns-ping", MakeVersion(1,0), MakeVersion(1,0)};
};

UniquePtr<Package> PackageFactory_Ping::Create(Parser &parser, int iVersion)
{
   return MakeUnique<Package_Ping>(info(), parser);
}

UniquePtr<PackageFactory> CreatePackageFactory_dns_com_awns_ping()
{
   return MakeUnique<PackageFactory_Ping>();
}

}
