//
// MCP Support
//

#include "Main.h"
#include "MCP.h"
#include "MCP_Negotiate.h"
#include "Connection.h"

namespace MCP
{

// Included Packages
UniquePtr<PackageFactory> CreatePackageFactory_Negotiate();
UniquePtr<PackageFactory> CreatePackageFactory_SimpleEdit();
UniquePtr<PackageFactory> CreatePackageFactory_dns_com_awns_status();
UniquePtr<PackageFactory> CreatePackageFactory_dns_com_vmoo_client();
UniquePtr<PackageFactory> CreatePackageFactory_dns_com_awns_ping();

bool HasMCP(ConstString string)
{
   return string.StartsWith("#$#mcp");
}

int VersionFromString(ConstString str)
{
   ConstString major_string, minor_string;
   if(!str.Split('.', major_string, minor_string))
      return 0;

   unsigned int major, minor;

   if(major_string.To(major) && minor_string.To(minor))
      return MakeVersion(major, minor);

   return 0;
}

void VersionToString(StringBuilder &string, int version)
{
   string << (version>>16) << '.' << (version&0xFFFF);
}

PackageFactory::PackageFactory()
{
}

PackageManager &GetPackageManager()
{
   if(!PackageManager::HasInstance())
      new PackageManager();

   return PackageManager::GetInstance();
}

PackageManager::PackageManager()
{
   Add(CreatePackageFactory_Negotiate());
   Add(CreatePackageFactory_SimpleEdit());
   Add(CreatePackageFactory_dns_com_awns_status());
   Add(CreatePackageFactory_dns_com_vmoo_client());
   Add(CreatePackageFactory_dns_com_awns_ping());

   CallAtShutdown([]() { delete &GetInstance(); });
}

PackageManager::~PackageManager()
{
}

PackageFactory *PackageManager::Find(ConstString name, int version_min, int version_max)
{
   for(auto &pf : m_packages)
   {
      // Name must match and version must intersect
      const PackageInfo &info=pf.info();

      if(info.name().ICompare(name)==0 &&
         ( (info.versionMin()>=version_min && info.versionMin()<=version_max) ||
           (info.versionMax()>=version_min && info.versionMax()<=version_max) )
        )
      {
         return &pf;
      }
   }

   return nullptr;
}

void PackageManager::Add(UniquePtr<PackageFactory> &&pFactory)
{
   Assert(!pFactory->Linked());
   pFactory.Extract()->Link(m_packages.Prev());
}

void Keylist::Add(ConstString value)
{
   auto p_item=MakeUnique<Item>();
   p_item->value=value;

   p_item->Link(m_root.Prev());
   p_item.Extract();
}

bool Keylist::operator==(const Keylist &t) const
{
   if(!IEquals(m_key, t.m_key))
      return false;

   auto iter1=m_root.begin(), iter1end=m_root.end();
   auto iter2=t.m_root.begin(), iter2end=t.m_root.end();
   while(true)
   {
      if(iter1==iter1end && iter2==iter2end)
         return true;

      if(iter1==iter1end || iter2==iter2end || !IEquals((*iter1).value, (*iter2).value))
         return false;
   }
}

Message::Message(ConstString package, ConstString message)
{
   m_message_name.Allocate(package.Length()+1+message.Length());
   StringBuilder string(m_message_name);
   string << package << '-' << message;

   m_package=m_message_name.First(package.Length());
   m_message=m_message_name.Last(message.Length());
}

bool Message::operator==(const Message &message) const
{
   if(!IEquals(m_message_name, message.m_message_name) ||
      m_keyvals.Count()!=message.m_keyvals.Count() ||
      m_keylists.Count()!=message.m_keylists.Count())
      return false;

   for(unsigned int i=0;i<m_keyvals.Count();i++)
      if(m_keyvals[i]!=message.m_keyvals[i])
         return false;

   for(unsigned int i=0;i<m_keylists.Count();i++)
      if(m_keylists[i]!=message.m_keylists[i])
         return false;

   return true;
}

const Keyval *Message::GetKeyval(ConstString key) const
{
   for(auto &keyval : m_keyvals)
      if(keyval->key().ICompare(key)==0)
         return keyval;

   return nullptr;
}

ConstString Message::operator[](ConstString key) const
{
   const Keyval *pKeyval=GetKeyval(key);
   if(!pKeyval)
      throw Missing_Parameter();

   return pKeyval->value();
}

const Keyval &Message::operator[](unsigned int iParam) const
{
   return *m_keyvals[iParam];
}

const Keylist *Message::GetKeylist(ConstString key) const
{
   for(auto &keylist : m_keylists)
      if(keylist->key().ICompare(key)==0)
         return keylist;

   return nullptr;
}

const Keylist &Message::keylist(ConstString key) const
{
   const Keylist *p_list=GetKeylist(key);
   if(!p_list)
      throw Missing_Parameter();

   return *p_list;
}

#ifdef _DEBUG
void Message::DebugDump() const
{
   FixedStringBuilder<1024> string("Package: ", m_package, CRLF "Message: ", m_message, CRLF);

   if(m_keyvals.Count())
   {
      string("Parameters: ");
      for(auto &keyval : m_keyvals)
         string(keyval->key(), '=', keyval->value(), ' ');
      string(CRLF);
   }

   for(auto &keylist : m_keylists)
   {
      string("List:", keylist->m_key, " = ");
      for(auto &item : keylist->m_root)
         string('"', item.value, "\" ");
      string(CRLF);
   }

   string(CRLF);
   OutputDebugString(string);
}
#endif

Keyval::Keyval(ConstString key, ConstString value)
: m_key(key), m_value(value)
{
}

void Message::AddParam(ConstString key, ConstString value)
{
   m_keyvals.Push(new Keyval(key, value));
}

void Message::SetParam(ConstString key, ConstString value)
{
   Keyval *pKeyval=UnconstPtr(GetKeyval(key));
   if(!pKeyval)
      throw Missing_Parameter();

   pKeyval->value()=value;
}

bool Parser::IsIdentChar(char c)
{
   return IsAlpha(c) || IsDigit(c) || c=='-';
}

/*
bool Parser::IsStringChar(Char c)
{
   return IsSimpleChar(c) || c==' ' || c==':' || c=='*';
   // / needs to be handled special
}

bool Parser::IsLineChar(Char c)
{
   return IsSimpleChar(c) || IsQuoteChar(c) || c==' ' || c==':' || c=='*';
}
*/

bool Parser::IsQuoteChar(char c)
{
   return c=='"' || c=='\\';
}

bool Parser::IsSimpleChar(char c)
{
   return IsAlpha(c) || IsDigit(c) || IsOtherSimple(c);
}

bool Parser::IsOtherSimple(char c)
{
   return c=='!' || (c>='#' && c<=')') || c=='+' || c==',' || (c>='-' && c<='/') 
      || (c>=';' && c<='@') || c=='[' || c==']' || c=='^' || c=='`' || (c>='{' && c<='~');

   // ! 33 
   // # 35 $ 36 % 37 & 38 ' 39 ( 40 ) 41
   // + 43 , 44 
   // - 45 . 46 / 47
   // ; 59 < 60 = 61 > 62 ? 63 @ 64
   // [ 91
   // ] 93 ^ 94
   // ` 96
   // { 123 | 124 } 125 ~ 126
}

bool Parser::IsDigit(char c)
{
   return c>='0' && c<='9';
}

bool Parser::IsAlpha(char c)
{
   return c>='a' && c<='z' || c>='A' && c<='Z' || c=='_';
}

Parser::Parser(Connection &connection, ConstString string)
:  m_connection(connection),
   m_infoNegotiate("mcp-negotiate", MakeVersion(2,0), MakeVersion(2,0))
{
   // Set the Authentication Key
   {
      CryptProvider hCryptProv;
      bool fSuccess=!!CryptAcquireContext(hCryptProv.Address(), nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
      DWORD iData;
      if(fSuccess)
         CryptGenRandom(hCryptProv, sizeof(DWORD), (BYTE *)&iData);
      else
      {
         m_connection.Text("MCP - Using GetTickCount() instead of CryptGenRandom.  No big deal\n");
         iData=GetTickCountQPC(); // Anything better?
      }

      m_auth_key.Allocate(8);
      StringBuilder{m_auth_key}(Strings::Hex32(iData, sizeof(iData)*2));
   }

   // Send the key to the server
   FixedStringBuilder<2048> strMessage;
   strMessage << "#$#mcp authentication-key: " << m_auth_key << " version: 2.1 to: 2.1\n";
   m_connection.Send(strMessage, false);

   AddPackage(*new Package_Negotiate(m_infoNegotiate, GetPackageManager(), *this));
}

void Parser::OnLine(ConstString string)
{
   if(string.StartsWith("#$#"))
   {
      FromString(string.WithoutFirst(3));
      ParseMessage();
// Uncomment to see incoming MCP messages
//      m_connection.OnLine(string.string(), string.Length());
      return;
   }

   if(string.StartsWith("#$\""))
   {
      // Strip out 
      m_connection.OnLine(string.WithoutFirst(3));
      return;
   }

   // Not MCP, send straight through
   m_connection.OnLine(string);
}

bool Parser::ParseSpace()
{
   if(CharGet()!=' ')
      return false;

   SkipChars(' ');
   return true;
}

bool Parser::ParseKey(ConstString &string)
{
   int iPos=PosGet();
   if(!IsAlpha(CharGet()))
      return false; // Bad keyval

   while(IsIdentChar(CharGet()))
   {
   }
   CharBack();

   string=GetConstStringAt(iPos);
   return true;
}

bool Parser::ParseValue(ConstString &string)
{
   int iPos=PosGet();
   if(!IsSimpleChar(CharGet()))
      return false; // Must have at least one simplechar

   while(IsSimpleChar(CharGet()))
   {
   }
   CharBack();

   string=GetConstStringAt(iPos);
   return true;
}

void Parser::ParseMultilineContinue()
{
   if(CharGet()!=' ')
      return;

   ConstString dataTag;
   if(!ParseValue(dataTag))
      return;

   if(!ParseSpace())
      return; // Missing a space

   ConstString key;
   if(!ParseKey(key))
      return;
   if(CharGet()!=':')
      return;
   if(CharGet()!=' ')
      return;

   int iPos=PosGet();
   while(!IsEnd())
      CharGet();

   ConstString value=GetConstStringAt(iPos);

   for(unsigned int i=0;i<m_multilines.Count();i++)
   {
      if(IEquals(m_multilines[i]->dataTag, dataTag))
      {
         for(auto &keylist : m_multilines[i]->pMessage->m_keylists)
         {
            if(IEquals(keylist->key(), key))
            {
               auto pItem=MakeUnique<Keylist::Item>();
               pItem->value=value;

               pItem->Link(keylist->m_root.Prev());
               pItem.Extract();
               return;
            }
         }

         Assert(0);
         m_multilines.UnsortedDelete(i);
         return;
      }
   }
   Assert(0);
}

void Parser::ParseMultilineEnd()
{
   if(CharGet()!=' ')
      return;

   ConstString dataTag;
   if(!ParseValue(dataTag))
      return;

   for(unsigned int i=0;i<m_multilines.Count();i++)
   {
      if(IEquals(m_multilines[i]->dataTag, dataTag))
      {
         Message &message=*m_multilines[i]->pMessage;
         Dispatch(message);
         m_multilines.UnsortedDelete(i);
         return;
      }
   }

   Assert(0);
}


void Parser::ParseMessage()
{
   if(CharSkip('*')) // Multiline continuation?
   {
      ParseMultilineContinue();
      return;
   }

   if(CharSkip(':')) // Multiline ending?
   {
      ParseMultilineEnd();
      return;
   }

   auto pMessage=MakeUnique<Message>();
   ConstString message_name;
   // <message-name>
   if(!ParseKey(message_name))
      return;
   if(!ParseSpace())
      return;

   // Check AuthKey
   if(!StringSkip(m_auth_key))
      return; // Bad Authkey!

   while(true)
   {
      if(!ParseSpace())
         break;

      int iPos=PosGet();
      if(!IsAlpha(CharGet()))
         return; // Bad keyval

      while(IsIdentChar(CharGet()))
      {
      }
      CharBack();

      auto pKeyval=MakeUnique<Keyval>();
      pKeyval->m_key=GetConstStringAt(iPos);

      bool fMultiline=false;

      if(CharSkip('*')) // Multiline?
         fMultiline=true;

      if(CharGet()!=':')
         return; // Bad keyval

      if(!ParseSpace())
         return; // Missing a space

      if(CharSkip('"')) // Quoted string?
      {
         int iEsc=0;
         iPos=PosGet();
         while(true)
         {
            char c=CharGet();

            if(IsSimpleChar(c) || c==' ' || c==':' || c=='*')
               continue;

            if(c=='\\')
            {
               iEsc++;
               if(IsQuoteChar(CharGet()))
                  continue;

               return; // Bad quote char
            }

            break;
         }
         CharBack();

         if(iEsc)
         {
            pKeyval->m_value.Allocate(PosGet()-iPos-iEsc);
            ConstString string=GetConstStringAt(iPos);
            for(unsigned int j=0, i=0;i<string.Length();i++,j++)
            {
               if(string[i]=='\\') // Strip out
                  i++;
               pKeyval->m_value[j]=string[i];
            }
         }
         else
            pKeyval->m_value=GetConstStringAt(iPos);

         if(CharGet()!='"')
            return; // Missing end quote
      }
      else
      {
         ConstString value;
         if(!ParseValue(value))
            return;

         pKeyval->m_value=value;
      }

      if(fMultiline)
      {
         auto pKeylist=MakeUnique<Keylist>();
         pKeylist->m_key=std::move(pKeyval->m_key);

         pMessage->m_keylists.Push(pKeylist.Extract());
      }
      else
         pMessage->m_keyvals.Push(pKeyval.Extract());
   }

   pMessage->m_message_name=message_name;

   // Look for _data_tag
   if(pMessage->m_keylists.Count())
   {
      try
      {
         ConstString data_tag=(*pMessage)["_data-tag"];

         auto pMultiline=MakeUnique<Multiline>();
         pMultiline->pMessage=std::move(pMessage);
         pMultiline->dataTag=data_tag;

         m_multilines.Push(pMultiline.Extract());
      }
      catch(const Message::Missing_Parameter &)
      {
         m_connection.Text("<icon error><font color='red'>MCP - Multiline message missing _data-tag");
      }
      return;
   }

   Dispatch(*pMessage);
}

void Parser::Dispatch(Message &message)
{
   // Start with the package as the whole 
   message.m_package=message.m_message_name;

   while(true)
   {
      for(auto &package : m_packages)
      {
         if(IEquals(package.info().name(), message.package()))
         {
            try
            {
               package.On(message);
            }
            catch(const Message::Missing_Parameter &)
            {
               m_connection.Text("<icon error><font color='red'>MCP - Message was missing a required parameter");
            }
            return;
         }
      }

      unsigned iDash=message.m_package.FindLastOf('-');
      if(iDash==Strings::Result::Not_Found)
         break;

      // Backup a dash (since we can't tell the difference between the package name and the message names!  Is it 1-2-3 where 3 is the message or 1-2-3 where 2-3 is?! Bad Revar!)
      message.m_package=message.m_message_name.First(iDash);
      message.m_message=message.m_message_name.WithoutFirst(iDash+1);
   }

   m_connection.Text(FixedStringBuilder<256>("<icon error><font color='red'>MCP protocol error - Unsupported package message - ", message.m_message_name));
}

void Parser::Send(const Message &message)
{
   FixedStringBuilder<2048> strMessage("#$#", message.package());
   if(message.message())
      strMessage('-', message.message());
   strMessage(' ', m_auth_key);

   for(auto &keyval : message.m_keyvals)
   {
      strMessage(' ', keyval->key(), ": \"");

      ConstString value=keyval->value();

      for(unsigned int j=0;j<value.Length();j++)
      {
         if(value[j]=='"')
            strMessage('\\');
         strMessage(value[j]);
      }

      strMessage('\"');
   }

   for(auto &keylist : message.m_keylists)
      strMessage(' ', keylist->key(), "*: \"\" _data-tag: ", Strings::HexAddress(&keylist));

   strMessage('\n');
//   m_connection.Text(strMessage, Colors::Green);
   m_connection.Send(strMessage, false);

   for(auto &keylist : message.m_keylists)
   {
      for(auto &item : keylist->m_root)
      {
         strMessage.Clear();
         strMessage("#$#* ", Strings::HexAddress(&keylist), ' ', keylist->key(), ": ", item.value, '\n');
//         m_connection.Text(strMessage, Colors::Green);
         m_connection.Send(strMessage, false);
      }

      strMessage.Clear();
      strMessage("#$#: ", Strings::HexAddress(&keylist), '\n');
//      m_connection.Text(strMessage, Colors::Green);
      m_connection.Send(strMessage);
   }
}

Package *Parser::GetPackage(ConstString name)
{
   for(auto &package : m_packages)
   {
      if(IEquals(package.info().name(), name))
         return &package;
   }

   return nullptr;
}

Package::Package(Parser &parser, const PackageInfo &info)
: m_parser(parser), m_info(info)
{
}


};
