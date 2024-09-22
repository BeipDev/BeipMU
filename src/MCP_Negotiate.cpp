//
// MCP-negotiate
//

#include "Main.h"
#include "Connection.h"
#include "MCP.h"
#include "MCP_Negotiate.h"

namespace MCP
{

Package_Negotiate::Package_Negotiate(const PackageInfo &info, PackageManager &manager, Parser &parser)
:  Package(parser, info),
   m_manager(manager)
{
   for(auto &factory : manager.m_packages)
   {
      Message message(info.name(), "can");
      message.AddParam("package", factory.info().name());

      FixedStringBuilder<256> version;
      VersionToString(version, factory.info().versionMin());
      message.AddParam("min-version", version);
      version.Clear();
      VersionToString(version, factory.info().versionMax());
      message.AddParam("max-version", version);
      parser.Send(message);
   }

   Message message(info.name(), "end");
   parser.Send(message);
}

void Package_Negotiate::On(const Message &message)
{
   if(m_done)
      return;

   if(IEquals(message.message(), "can"))
   {
      ConstString lmin=message["min-version"];
      ConstString lmax=message["max-version"];
      ConstString package=message["package"];

      if(!lmin || !lmax || !package)
         return;

      int iVersionMin=VersionFromString(lmin);
      int iVersionMax=VersionFromString(lmax);

      PackageFactory *pFactory=m_manager.Find(package, iVersionMin, iVersionMax);

      FixedStringBuilder<1024> string("<icon information> <font color='aqua'>mcp-negotiate - <font color='teal'>Package: <font color='#8080FF'>", package, " <font color='teal'>Version-Min: <font color='maroon'>", lmin,
         " <font color='teal'>Max:<font color='maroon'>", lmax, ' ');

      if(pFactory)
      {
         if(IEquals(package, "mcp-negotiate")) // We're special
         {
            string("<font color='lime'>Negotiated");
            m_parser.connection().Text(string);
            return;
         }

         UniquePtr<Package> pPackage=pFactory->Create(m_parser, iVersionMax);
         if(pPackage)
         {
            pPackage->Link(m_parser.m_packages.Prev());
            pPackage.Extract();

            string("<font color='lime'>Negotiated");
         }
         else
         {
            string("<font color='green'>Version Unsupported");
         }
      }
      else
         string("<font color='red'>Unsupported");

      m_parser.connection().Text(string);
      return;
   }

   if(IEquals(message.message(), "end"))
   {
      m_done=true;
      return;
   }

   m_parser.connection().Text("mcp-negotiate - Unknown command");
}

PackageFactory_Negotiate::PackageFactory_Negotiate()
{
}

UniquePtr<Package> PackageFactory_Negotiate::Create(Parser &parser, int iVersion)
{
   Assert(0);
   return nullptr;
}

UniquePtr<PackageFactory> CreatePackageFactory_Negotiate()
{
   return MakeUnique<PackageFactory_Negotiate>();
}

}
