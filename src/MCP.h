//
// MCP Support
//
struct Connection;

namespace MCP
{
bool HasMCP(ConstString string);

struct Package;
struct Parser;
struct Message;

constexpr int MakeVersion(int iMajor, int iMinor)
{
   Assert(iMinor<65536);
   return (iMajor<<16)|iMinor;
}

int VersionFromString(ConstString str); // Version is in the form of x.x (comes back as a 16:16 fraction)
void VersionToString(StringBuilder &string, int version);

struct PackageInfo
{
   PackageInfo(ConstString name, int version_min, int version_max)
   : m_name(name), m_version_min(version_min), m_version_max(version_max)
   {
   }

   ConstString name() const { return m_name; }
   int versionMin() const { return m_version_min; }
   int versionMax() const { return m_version_max; }

private:

   OwnedString m_name;
   int m_version_min, m_version_max;
};

struct Package : DLNode<Package>
{
   Package(Parser &parser, const PackageInfo &info);
   virtual ~Package() noexcept { }

   virtual void On(const Message &msg)=0;

   const PackageInfo &info() const { return m_info; }

protected:
   Parser &m_parser;

private:
   const PackageInfo &m_info;
};

struct PackageFactory : DLNode<PackageFactory>
{
   PackageFactory();
   virtual ~PackageFactory() noexcept { }

   virtual UniquePtr<Package> Create(Parser &parser, int iVersion)=0;
   virtual const PackageInfo &info() const=0;

private:
   PackageFactory(const PackageFactory &);
};

struct PackageManager : Singleton<PackageManager>
// Only one of these PackageManagers exist globally
{
   PackageManager();
   ~PackageManager() noexcept;

   PackageFactory *Find(ConstString name, int iVersionMin, int iVersionMax);
   void Add(UniquePtr<PackageFactory> &&pFactory);

private:
   OwnedDLNodeList<PackageFactory> m_packages;
   friend struct Package_Negotiate;
};

struct Keyval // A single value in an MCP message
{
   Keyval() { }
   Keyval(ConstString key, ConstString value);

   bool operator==(const Keyval &t) const { return IEquals(m_key, t.m_key) && IEquals(m_value, t.m_value); }

   ConstString key() const { return m_key; }
   ConstString value() const { return m_value; }
         OwnedString &value() { return m_value; }

private:
   OwnedString m_key;
   OwnedString m_value;
   friend struct Parser;
};

struct Keylist // A list of values in an MCP message (think of the parameter being passed as an array)
{
   Keylist() = default;

   ConstString key() const { return m_key; }
   void Add(ConstString value);

   struct Item : DLNode<Item>
   {
      OwnedString value;
   };

   bool operator==(const Keylist &t) const;
   bool operator!=(const Keylist &t) const { return !operator==(t); }

   OwnedDLNodeList<Item> m_root;
   OwnedString m_key;
};

struct Message
{
   Message() = default;
   Message(ConstString package, ConstString message);

   bool operator==(const Message& message) const;

   ConstString package() const { return m_package; }
   ConstString message() const { return m_message; }

   const Keyval *GetKeyval(ConstString key) const; // Returns nullptr if not found
   ConstString operator[](ConstString key) const; // Throws Missing_Parameter if not found
   const Keyval &operator[](unsigned int iParam) const;

   unsigned int Params() const { return m_keyvals.Count(); }

   const Keylist *GetKeylist(ConstString key) const; // Returns nullptr if not found
   const Keylist &keylist(ConstString key) const; // Throws Missing_Parameter if not found
   unsigned int Keylists() const { return m_keylists.Count(); }

   void AddParam(ConstString key, ConstString value);
   void SetParam(ConstString key, ConstString value); // Throws Missing_Parameter if not found
   void AddKeylist(UniquePtr<Keylist> &keylist) { m_keylists.Push(keylist.Extract()); }

   struct Missing_Parameter : std::exception { };

   DebugOnly(void DebugDump() const;) // Dump the message to the debug output window

private:
   Collection<UniquePtr<Keyval>> m_keyvals{8};
   Collection<UniquePtr<Keylist>> m_keylists;

   OwnedString m_message_name; // Full name (split into package-message when dispatching)
   ConstString m_package; // Points into m_message_name
   ConstString m_message; // Points into m_message_name

   friend struct Parser;
};

struct Multiline
{
   UniquePtr<Message> pMessage;
   OwnedString dataTag;
};

struct Parser : Streams::Input
{
   Parser(Connection &connection, ConstString string);
   void AddPackage(Package &package) { package.Link(m_packages.Prev()); }

   void OnLine(ConstString string);

   Connection &connection() { return m_connection; }
   void Send(const Message &message);

   Package *GetPackage(ConstString name);

private:

   void Dispatch(Message &message); // Modifies the message!

   static bool IsIdentChar(char c);
   static bool IsQuoteChar(char c);
   static bool IsSimpleChar(char c);
   static bool IsOtherSimple(char c);
   static bool IsDigit(char c);
   static bool IsAlpha(char c);

   bool ParseSpace();
   bool ParseKey(ConstString &key);
   bool ParseValue(ConstString &value);
   void ParseMultilineContinue();
   void ParseMultilineEnd();
   void ParseMessage();

   Connection &m_connection;

   OwnedString m_auth_key; // 8 character random key

   OwnedDLNodeList<Package> m_packages;
   Collection<UniquePtr<Multiline>> m_multilines; // Current multiline messages being received

   PackageInfo m_infoNegotiate;
   friend struct Package_Negotiate;
};

PackageManager &GetPackageManager();
};
