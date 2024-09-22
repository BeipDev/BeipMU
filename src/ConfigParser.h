// Configuration File Parser

// Used by the properties code to handle the optional types
template<typename T>
T &AllocateIfEmpty(UniquePtr<T> &pT)
{
   if(!pT)
      pT=MakeUnique<T>();
   return *pT;
}

template<typename T>
T &AllocateIfEmpty(CntPtrTo<T> &pT)
{
   if(!pT)
      pT=MakeCounting<T>();
   return *pT;
}

struct CKeyMacro
{
   OwnedString pclMacro; // The Macro text
   KEY_ID key;
   bool   fType{}; // True to type the text rather than to send it directly
};

void Parse(CKeyMacro &data, Streams::Input &input);
void Write(const CKeyMacro &macro, Streams::Output &output);

struct DataInfo
{
   DataInfo(DataInfo&&)=default;

   ConstString GetName() const { return m_name; }

   virtual void Read(void *data, ConstString input) const=0;
   virtual void Write(const void *data, Streams::Output &output) const=0;

   virtual bool IsDefault(const void *data) const=0;
   virtual void SetToDefault(void *data) const=0;

   static const BYTE Flag_None        = 0x00;
   static const BYTE Flag_NoWrite     = 0x01;
   static const BYTE Flag_AlwaysWrite = 0x02;

   bool IsFlagSet(BYTE flag) const { return (m_flags&flag)!=0; }

protected:

   constexpr DataInfo(ptrdiff_t offset, ConstString name, BYTE flags) : m_offset{offset}, m_name{name}, m_flags{flags} { }

   template<typename T> const T &TCast(const void *p) const { return TCast<T>(UnconstPtr(p)); }
   template<typename T>       T &TCast(      void *p) const { return *reinterpret_cast<T *>( reinterpret_cast<BYTE *>(p)+m_offset ); }
   ptrdiff_t m_offset; // Offset of our data into the structure

private:
   DataInfo(const DataInfo&)=delete;

   ConstString m_name;
   BYTE m_flags;
};

template<typename... Types>
struct DataInfos
{
   using TArray=std::array<const DataInfo*, sizeof...(Types)>;

   consteval DataInfos(Types&&... params)
    : m_tuple{std::forward<Types>(params)...},
      m_array{std::apply([](auto &&... t) { return TArray{ &t... }; }, m_tuple)}
   {
   }

   operator Array<const DataInfo*>() const { return UnconstRef(m_array); }

private:
   std::tuple<Types...> m_tuple;
   TArray m_array;
};

struct DataInfo_Collection : DataInfo
{
   constexpr DataInfo_Collection(unsigned offset) : DataInfo(offset, "", 0) { }

   void Write(const void *data, Streams::Output &output) const override { Assert(0); __assume(0); } // Can't write out one item with a collection
   // The collection interface, gives a count of items and ability to write out multiple items
   virtual unsigned ItemCount(const void *data) const=0;
   virtual void Write(const void *data, Streams::Output &output, unsigned index) const=0; // Have to use this instead

private:
   bool IsDefault(const void *data) const override { Assert(0); __assume(0); }
   void SetToDefault(void *data) const override { }
};

template<typename T> Collection<T> &CastToCollection(void *data);

template<typename T>
struct TDataInfo_Collection : DataInfo_Collection
{
   constexpr TDataInfo_Collection() : DataInfo_Collection{0} { }

   void Read(void *data, ConstString input_string) const override
   {
      auto pNew=MakeUnique<T>();
      auto input=Streams::Input(input_string);
      Parse(*pNew, input);
      CastToCollection<UniquePtr<T>>(data).Push(std::move(pNew));
   }

   void Write(const void *data, Streams::Output &output, unsigned index) const override
   {
      ::Write(*CastToCollection<UniquePtr<T>>(UnconstPtr(data))[index], output);
   }

   unsigned ItemCount(const void *data) const { return CastToCollection<UniquePtr<T>>(UnconstPtr(data)).Count(); }
};

template<typename T>
struct TDataInfo : DataInfo
{
   constexpr TDataInfo(ptrdiff_t offset, ConstString name, BYTE flags, T defaultValue)
    : DataInfo{offset, name, flags}, m_default{defaultValue} { }

   void Read(void *data, ConstString input) const override;
   void Write(const void *data, Streams::Output &output) const override;

   bool IsDefault(const void *data) const override { return Cast(data)==m_default; }
   void SetToDefault(void *data) const override { Cast(data)=m_default; }

protected:

   const T &Cast(const void *p) const { return TCast<T>(p); }
   T &Cast(void *p) const { return TCast<T>(p); }

   T m_default;
};

using DataInfo_Color=TDataInfo<Color>;
using DataInfo_bool=TDataInfo<bool>;
using DataInfo_float=TDataInfo<float>;
using DataInfo_uint32=TDataInfo<uint32>;
using DataInfo_uint64=TDataInfo<uint64>;
using DataInfo_int2=TDataInfo<int2>;
using DataInfo_float2=TDataInfo<float2>;
using DataInfo_Rect=TDataInfo<Rect>;
using DataInfo_KEY_ID=TDataInfo<KEY_ID>;
using DataInfo_BYTE=TDataInfo<BYTE>;

struct DataInfo_int : TDataInfo<int>
{
   constexpr DataInfo_int(ptrdiff_t offset, ConstString name, BYTE flags, int defaultValue, int2 range)
    : TDataInfo<int>(offset, name, flags, defaultValue), m_range(range) { }

   void Read(void *data, ConstString input) const override;

private:
   const int2 m_range;
};

struct DataInfo_String : DataInfo
{
   constexpr DataInfo_String(ptrdiff_t offset, ConstString name, BYTE flags, ConstString defaultValue, unsigned maxLength)
   : DataInfo(offset, name, flags), m_default(defaultValue), m_maxLength(maxLength)
   { }

   void Read(void *data, ConstString input) const override;
   void Write(const void *data, Streams::Output &output) const override { output.Write_String(Cast(data), true); }

   bool IsDefault(const void *data) const { return m_default.Compare(Cast(data))==0; }
   void SetToDefault(void *data) const { Cast(data)=m_default; }
   unsigned GetMaxLength() const { return m_maxLength; }

private:

   const OwnedString &Cast(const void *p) const { return TCast<OwnedString>(p); }
         OwnedString &Cast(void *p)       const { return TCast<OwnedString>(p); }

   ConstString m_default;
   const unsigned m_maxLength;
};

struct DataInfo_KeyMacro : DataInfo
{
   DataInfo_KeyMacro(ptrdiff_t offset, ConstString name, BYTE flags, const KEY_ID keyDefault)
   : DataInfo(offset, name, flags), m_keyDefault(keyDefault)
   { }

   void Read(void *data, ConstString input) const override;
   void Write(const void *data, Streams::Output &output) const override;

   bool IsDefault(const void *data) const override { return false; }
   void SetToDefault(void *data) const override { Assert(0); __assume(0); /* Well? */ }

private:

   const CKeyMacro &Cast(const void *p) const { return TCast<CKeyMacro>(p); }
   CKeyMacro &Cast(void *p) const { return TCast<CKeyMacro>(p); }

   const KEY_ID m_keyDefault;
};

struct DataInfo_Enum : DataInfo
{
   template<unsigned enumCount>
   constexpr DataInfo_Enum(ptrdiff_t offset, ConstString name, BYTE flags, const ConstString (&enums)[enumCount], unsigned iDefault)
   : DataInfo(offset, name, flags), m_enums(enums), m_iDefault(iDefault)
   { }

   void Read(void *data, ConstString input) const override;
   void Write(const void *data, Streams::Output &output) const override;

   bool IsDefault(const void *data) const override { return Cast(data)==m_iDefault; }
   void SetToDefault(void *data) const override { Cast(data)=m_iDefault; }

private:

   const unsigned &Cast(const void *p) const { return TCast<unsigned>(p); }
   unsigned &Cast(void *p) const { return TCast<unsigned>(p); }

   Array<const ConstString> m_enums;
   unsigned m_iDefault;
};

struct DataInfo_Time : DataInfo
{
   constexpr DataInfo_Time(ptrdiff_t offset, ConstString name, BYTE flags)
   : DataInfo(offset, name, flags)
   { }

   void Read(void *data, ConstString input) const override;
   void Write(const void *data, Streams::Output &output) const override;

   bool IsDefault(const void *data) const override { return Time::Time()==Cast(data); }
   void SetToDefault(void *data) const override { Cast(data)=Time::Time(); }

private:

   const Time::Time &Cast(const void *p) const { return TCast<Time::Time>(p); }
   Time::Time &Cast(void *p) const { return TCast<Time::Time>(p); }
};

struct DataInfo_GUID : DataInfo
{
   constexpr DataInfo_GUID(ptrdiff_t offset, ConstString name, BYTE flags, REFIID defaultValue)
   : DataInfo(offset, name, flags), m_default(defaultValue)
   { }

   void Read(void *data, ConstString input) const override;
   void Write(const void *data, Streams::Output &output) const override;

   bool IsDefault(const void *data) const override { return (Cast(data)==m_default)!=0; }
   void SetToDefault(void *data) const override { Cast(data)=m_default; }

private:

   const GUID &Cast(const void *p) const { return TCast<GUID>(p); }
   GUID &Cast(void *p) const { return TCast<GUID>(p); }

   REFIID m_default;
};

// Import Interface that is passed to the parser
interface IParserIO
{
   // Opens up a new scope with the given name
   virtual IParserIO *BeginScope(ConstString key) { return nullptr; }

   //
   // Methods for writing data back to the file
   //

   // The number of contained scopes
   virtual unsigned ScopeCount() const { return 0; }
   // Retrieve the name of the scope
   virtual ConstString ScopeName(unsigned scopeIndex) const { return ConstString(); }
   // Retrieve the interface of the scope
   virtual IParserIO *ScopeGet(unsigned scopeIndex) { Assert(0); return nullptr; }

   virtual Array<const DataInfo*> GetDataInfos() const { return {}; }

   void ResetToDefaults();

   bool operator==(const IParserIO &) const { return true; } // All IParserIO are equal, so classes that derive from them can still default compare
};

// This define is handy for derived classes
#define Prototypes_IParserIO_Scope \
   IParserIO *BeginScope(ConstString key) override; \
   unsigned ScopeCount() const override; \
   ConstString ScopeName(unsigned scopeIndex) const override; \
   IParserIO *ScopeGet(unsigned scopeIndex) override; \

template<typename T> Collection<T>& CastToCollection(void *data)
{
   struct Fake : IParserIO, Collection<T> { };
   return *static_cast<Collection<T>*>(reinterpret_cast<Fake*>(data));
}


struct ConfigImport : private Streams::Input
{
   ConfigImport(Array<const BYTE> data, IParserIO *pIO, IError &error);
   operator bool() const noexcept { return m_errorCount==0; }

private:

   void ParseError(ConstString error);

   bool Parse_KeyString();
   void Parse_Whitespace();

   bool Parse_Key(IParserIO *pIO);
   bool Parse_Scope(IParserIO *pIO);
   bool Parse_Data(IParserIO *pIO, ConstString value);

   struct Abort { };
   unsigned m_errorCount{};

   FixedStringBuilder<256> m_strKey;
   IError &m_error;
};

struct ConfigExport : private Streams::Output
{
   ConfigExport(ConstString fileName, IParserIO *pIO, bool fShowDefaults, bool allow_gui);
   ~ConfigExport() noexcept;

private:

   void NewLine() { CharPut(CHAR_CR); CharPut(CHAR_LF); }
   void Write_Scope(IParserIO *pIO);
   void Write_SmallDataScope(unsigned iScope, IParserIO *pIO);

   bool IsScopeEmpty(IParserIO *pIO);
   bool ShouldWrite(IParserIO *pIO, const DataInfo &info);

   unsigned DataInScope(IParserIO *pIO);
   unsigned ScopesInScope(IParserIO *pIO);

   bool m_fShowDefaults; // Don't write out properties that have their default value
   unsigned m_iScope{}; // How many scopes deep we are
};
