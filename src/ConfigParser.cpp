// Configuration File Parser

#include "Main.h"

void Parse(KEY_ID &data, Streams::Input &input)
{
   data.fAlt=false;
   data.fControl=false;
   data.fShift=false;

   while(true)
   {
      if(input.StringSkip("control+")) { data.fControl=true; continue; }
      if(input.StringSkip("alt+")) { data.fAlt=true; continue; }
      if(input.StringSkip("shift+")) { data.fShift=true; continue; }

      if(input.StringSkip("?control+")) { data.fControl=Tristate::Unknown; continue; }
      if(input.StringSkip("?alt+")) { data.fAlt=Tristate::Unknown; continue; }
      if(input.StringSkip("?shift+")) { data.fShift=Tristate::Unknown; continue; }

      break;
   }

   char c=input.CharGet();
   // If the next character is not a letter or a number, and this is a letter or a number, it's a single char
   if( !IsLetter(input.CharSpy()) && !IsNumber(input.CharSpy()) && (IsLetter(c) || IsNumber(c)) )
   {
      // Force the character to uppercase (since that's how VKeys are)
      data.iVKey=Uppercase(c);
      return;
   }
   input.CharBack();

   // Function Key?
   if(input.CharSkip('F'))
   {
      int iFKey;
      if(!input.Parse(iFKey))
         throw std::runtime_error{"Couldn't parse key"};

      data.iVKey=VK_F1-1+iFKey;
      return;
   }

   for(auto &&info : KEY_ID::GetInfos())
   {
      if(input.StringSkip(info.name))
      {
         data.iVKey=info.iVKey;
         return;
      }
   }

   Assert(0); // Unknown key being read in!
   data.iVKey=0; // Set it to 'no key'
   throw std::runtime_error{"Couldn't parse key"};
}

void Write(const KEY_ID &key, Streams::Output &output)
{
   if(key.fControl.IsUnknown())
      output("?Control+");
   else
      if(key.fControl)
         output("Control+");

   if(key.fAlt.IsUnknown())
      output("?Alt+");
   else
      if(key.fAlt)
         output("Alt+");

   if(key.fShift.IsUnknown())
      output("?Shift+");
   else
      if(key.fShift)
         output("Shift+");

   FixedStringBuilder<256> sBuffer; key.KeyName(sBuffer);
   output(sBuffer);
}

void Parse(CKeyMacro &data, Streams::Input &input)
{
   Parse(data.key, input);
   if(input.CharGet()==',')
   {
      HybridStringBuilder<256> string;
      if(input.Parse_String(string))
      {
         if(input.StringSkip(",T"))
            data.fType=true;

         data.pclMacro=string;
         return;
      }
   }

   throw std::runtime_error{"Expecting ',' in key macro"};
}

void Write(const CKeyMacro &macro, Streams::Output &output)
{
   Write(macro.key, output);
   output(',');
   output.Write_String(macro.pclMacro, true);   
   if(macro.fType)
      output(",T");
}

template<typename T>
void TDataInfo<T>::Write(const void *data, Streams::Output &output) const
{
   output(Cast(data));
}

template void TDataInfo<BYTE>::Write(const void *data, Streams::Output &output) const;
template void TDataInfo<int>::Write(const void *data, Streams::Output &output) const;
template void TDataInfo<float>::Write(const void *data, Streams::Output &output) const;
template void TDataInfo<uint64>::Write(const void *data, Streams::Output &output) const;
template void TDataInfo<unsigned>::Write(const void *data, Streams::Output &output) const;

void TDataInfo<bool>::Write(const void *data, Streams::Output &output) const
{
   output(Cast(data) ? ConstString("true") : ConstString("false"));
}

void TDataInfo<bool>::Read(void *data, ConstString input) const
{
   // [True, T] [False, F]
   if(input=="true" || input=="t")
   {
      Cast(data)=true;
      return;
   }

   if(input=="false" || input=="f")
   {
      Cast(data)=false;
      return;
   }

   throw std::runtime_error{"Couldn't parse flag"};
}

void TDataInfo<Color>::Write(const void *data, Streams::Output &output) const
{
   Windows::Color color=Cast(data);
   output("RGB(", (unsigned)color.Red(), ',', (unsigned)color.Green(), ',', (unsigned)color.Blue(), ')');
}

void TDataInfo<KEY_ID>::Write(const void *data, Streams::Output &output) const
{
   ::Write(Cast(data), output);
}

template<typename T>
void TDataInfo<T>::Read(void *data, ConstString input) const
{
   if(!input.To(Cast(data)))
      throw std::runtime_error{"Couldn't parse value"};
}

template void TDataInfo<int>::Read(void *, ConstString) const;
template void TDataInfo<float>::Read(void *, ConstString) const;
template void TDataInfo<uint64>::Read(void *, ConstString) const;
template void TDataInfo<unsigned>::Read(void *, ConstString) const;

void TDataInfo<Color>::Read(void *data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);
   if(!input.Parse(Cast(data)))
      throw std::runtime_error{"Couldn't parse color"};
}

void TDataInfo<BYTE>::Read(void *data, ConstString input) const
{
   int iValue;

   if(!input.To(iValue))
      throw std::runtime_error{"Couldn't parse value"};

   if(iValue<0 || iValue>255)
      throw std::runtime_error{"BYTE Value out of range"};
}

void TDataInfo<Rect>::Write(const void *data, Streams::Output &output) const
{
   const Rect &rect=Cast(data);
   output('(', (int)rect.left, ',', (int)rect.top, ',', (int)rect.right, ',', (int)rect.bottom, ')');
}

void TDataInfo<Rect>::Read(void *data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);

   int iLeft, iTop, iRight, iBottom;

   // Expecting input is "(left,top,right,bottom)"
   if(input.CharGet()!='(' ||
      !input.Parse(iLeft) || input.CharGet()!=',' || !input.Parse(iTop) || input.CharGet()!=',' || !input.Parse(iRight) || input.CharGet()!=',' || !input.Parse(iBottom) || 
      input.CharGet()!=')')
      throw std::runtime_error{"Can't parse rectangle"};

   Cast(data)=Rect(iLeft, iTop, iRight, iBottom);
}


void DataInfo_int::Read(void *data, ConstString input) const
{
   int iValue;

   if(!input.To(iValue))
      throw std::runtime_error{"Can't parse integer"};


   if(!IsBetween(iValue, m_range))
      throw std::runtime_error{"Int Value out of range"};

   Cast(data)=iValue;
}

void DataInfo_String::Read(void *data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);

   Strings::HeapStringBuilder string(m_maxLength);
   if(!input.Parse_String(string))
      throw std::runtime_error{FixedStringBuilder<256>("String too long (exceeds ", m_maxLength, " characters)").Terminate()};

   Cast(data)=string;
}

void TDataInfo<int2>::Write(const void *data, Streams::Output &output) const
{
   int2 value=Cast(data);
   output('{', value.x, ',', value.y, '}');
}

void TDataInfo<int2>::Read(void* data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);

   int2 &value=Cast(data);
   if(input.CharGet()!='{' || !input.Parse(value.x) || input.CharGet()!=',' || !input.Parse(value.y) || input.CharGet()!='}')
      throw std::runtime_error{"Can't parse int2"};
}

void TDataInfo<float2>::Write(const void *data, Streams::Output &output) const
{
   float2 value=Cast(data);
   output('{', value.x, ',', value.y, '}');
}

void TDataInfo<float2>::Read(void* data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);

   float2 &value=Cast(data);
   if(input.CharGet()!='{' || !input.Parse(value.x) || input.CharGet()!=',' || !input.Parse(value.y) || input.CharGet()!='}')
      throw std::runtime_error{"Can't parse float2"};
}

void DataInfo_KeyMacro::Read(void *data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);
   return ::Parse(Cast(data), input);
}

void DataInfo_KeyMacro::Write(const void *data, Streams::Output &output) const
{
   ::Write(Cast(data), output);
}

void DataInfo_Enum::Read(void *data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);
   for(unsigned i=0;i<m_enums.Count();i++)
   {
      if(input.StringSkip(m_enums[i]))
      {
         Cast(data)=i;
         return;
      }
   }

   throw std::runtime_error{"Unknown enum value"};
}

void DataInfo_Enum::Write(const void *data, Streams::Output &output) const
{
   output.Write_Bytes(m_enums[Cast(data)]);
}

void DataInfo_Time::Write(const void *data, Streams::Output &output) const
{
   Time::Time time=Cast(data); time.Stop();

   output(time.wYear, '-', time.wMonth, '-', time.wDay, '-',
          time.wHour, '-', time.wMinute, '-', time.wSecond, '-', time.wMilliseconds);
}

void DataInfo_Time::Read(void *data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);

   unsigned iYear, iMonth, iDay, iHour, iMinute, iSecond, iMilliseconds;

   if(!input.Parse(iYear) || input.CharGet()!='-' ||
      !input.Parse(iMonth) || input.CharGet()!='-' ||
      !input.Parse(iDay) || input.CharGet()!='-' ||
      !input.Parse(iHour) || input.CharGet()!='-' ||
      !input.Parse(iMinute) || input.CharGet()!='-' ||
      !input.Parse(iSecond) || input.CharGet()!='-' ||
      !input.Parse(iMilliseconds))
      throw std::runtime_error{"Time parse error"};

   // Sanity Check
   if(iMonth==0 || iMonth>12 ||
      iDay==0 || iDay>34 ||
      iHour>24 || iMinute>60 || iSecond>60)
      throw std::runtime_error{"Time value out of range"};

   Time::Time time;

   time.wYear=iYear;
   time.wMonth=iMonth;
   time.wDay=iDay;
   time.wDayOfWeek=0;
   time.wHour=iHour;
   time.wMinute=iMinute;
   time.wSecond=iSecond;
   time.wMilliseconds=iMilliseconds;

   Cast(data)=time;
}

void DataInfo_GUID::Read(void *data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);

   FixedStringBuilder<38> guid; input.Parse_String(guid);
   if(guid.Length()!=38)
      throw std::runtime_error{"Guid has an invalid length"};

   Cast(data)=StringToGUID(guid);
}

void DataInfo_GUID::Write(const void *data, Streams::Output &output) const
{
   output(GUIDToString(Cast(data)));
}

void DataInfo_KEY_ID::Read(void *data, ConstString input_string) const
{
   auto input=Streams::Input(input_string);
   return Parse(Cast(data), input);
}

void IParserIO::ResetToDefaults()
{
   for(auto *p_info : GetDataInfos())
      p_info->SetToDefault(this);
}

//
// ConfigImport
//

ConfigImport::ConfigImport(Array<const BYTE> data, IParserIO *pIO, IError &error)
 : Streams::Input(ConstString(reinterpret_cast<const char*>(data.begin()), data.Count())),
   m_error(error)
{
   try
   {
      while(true)
      {
         Parse_Whitespace();
         if(!Parse_Key(pIO))
            break;
      }
   }
   catch(const Abort &)
   {
      m_error.Error("Fatal Error, parsing aborted");
   }
}

//
void ConfigImport::ParseError(ConstString error)
{
   if(m_errorCount==10)
      m_error.Error("More than 10 errors seen, suppressing further error messages.");

   ++m_errorCount;
   m_error.Error(FixedStringBuilder<256>("Error on line ", GetLineNumber(), " column ", GetColumnNumber(), " : ", error));

   // Go to the end of the line
   char c;
   while((c=CharGet()) && c!=CHAR_CR && c!=CHAR_LF);
}

//
bool ConfigImport::Parse_KeyString()
{
   m_strKey.Clear();
   while(char c=CharGet())
   {
      // Any non letter character ends the string
      if(!IsLetter(c))
      {
         CharBack();
         if(m_strKey)
            return true;
         return false;
      }
      m_strKey(c);
   }

   Assert(IsEnd());
   if(m_strKey)
      m_error.Error("Unexpected End of File while parsing Key name");
   return false;
}

//
void ConfigImport::Parse_Whitespace()
{
   while(true)
   {
      __super::Parse_Whitespace();

      // Comment?
      if(CharSpy()==';')
      {
         while(char c=CharGet())
            if(c==CHAR_CR || c==CHAR_LF)
               break;
      }
      else
         return;
   }
}

//
bool ConfigImport::Parse_Key(IParserIO *pIO)
//
// (Whitespace) Key [.Key]* (Whitespace) [= {]
//
{
   // String?
   if(CharSpy()=='"')
   {
      m_strKey.Clear(); Parse_String(m_strKey);

      if(m_strKey.Remaining()==0)
      {
         ParseError("Parse Error: Key name exceeds 256 characters");
         return true;
      }
   }
   else  // Parse the string ourselves
   {
      // We use this routine instead of Parse_String because we stop at the '.' character
      if(!Parse_KeyString())
      {
         if(CharSpy()!='{') // Is this anything but an unnamed scope?
            return false;
      }
   }

   if(CharSkip('.'))   // New Scope?
   {
      IParserIO *pNewScope{};
      if(pIO)
         pNewScope=pIO->BeginScope(m_strKey);

      if(pIO && !pNewScope)
         ParseError(FixedStringBuilder<256>("Ignoring unknown scope '", m_strKey, '\''));

      Parse_Key(pNewScope);
      return true;
   }

   // Otherwise 
   Parse_Whitespace();

   switch(CharGet())
   {
      case '{':
         Parse_Scope(pIO);
         return true;

      case '}':   // End of scope?
         CharBack();
         return false;

      case '=':
      {
         Parse_Whitespace();
         ConstString value;
         if(!StringUntilChar(CHAR_CR, value))
            ParseError("Missing CR at end of value");
         if(pIO)
            Parse_Data(pIO, value);
         return true;
      }
   }

   if(IsEnd()) return false;

   ParseError(FixedStringBuilder<256>("Unrecognized Token/Data '", m_strKey, '\''));
   return true;
}

bool ConfigImport::Parse_Scope(IParserIO *pIO)
{
   IParserIO *pPIScope{};
   if(pIO)
      pPIScope=pIO->BeginScope(m_strKey);

   if(pIO && !pPIScope)
      ParseError(FixedStringBuilder<256>("Ignoring unknown scope '", m_strKey, '\''));

   while(true)
   {
      Parse_Whitespace();
      if(!Parse_Key(pPIScope))
         break;
   }

   if(CharGet()=='}')
      return true;

   ParseError("Missing end of scope");
   CharBack();
   return true;
}

bool ConfigImport::Parse_Data(IParserIO *pIO, ConstString value)
{
   for(auto *p_info : pIO->GetDataInfos())
   {
      if(p_info->GetName()==m_strKey)
      {
         try
         {
            p_info->Read(pIO, value);
         }
         catch(const std::exception &message)
         {
            if(message.what())
               ParseError(SzToString(message.what()));
            else
               ParseError(FixedStringBuilder<256>("Could not parse data value of '", m_strKey, '\''));
         }
         return true;
      }
   }

   ParseError(FixedStringBuilder<256>("Unrecognized data item '", m_strKey, '\''));
   return true;
}

//
// ConfigExport
//

ConfigExport::ConfigExport(ConstString filename, IParserIO *pIO, bool fShowDefaults, bool allow_gui)
 : m_fShowDefaults{fShowDefaults}
{
   if(!NewFile(filename))
   {
      if(allow_gui)
         MessageBox(nullptr, "Could not write out configuration file", "Error:", MB_ICONWARNING|MB_OK);
      return;
   }

   Write_Scope(pIO);
}

ConfigExport::~ConfigExport()
{
}

void ConfigExport::Write_Scope(IParserIO *pIO)
{
   for(auto *p_info : pIO->GetDataInfos())
   {
      if(!p_info->GetName()) // Collection?
      {
         const DataInfo_Collection &collection=static_cast<const DataInfo_Collection &>(*p_info);

         for(unsigned i=0;i<collection.ItemCount(pIO);i++)
         {
            CharPut(' ', m_iScope*2);
            Write_String(p_info->GetName(), false);
            CharPut('=');
            collection.Write(pIO, *this, i);
            NewLine();
         }
      }
      else if(ShouldWrite(pIO, *p_info))
      {
         CharPut(' ', m_iScope*2);
         Write_String(p_info->GetName(), false);
         CharPut('=');
         p_info->Write(pIO, *this);
         NewLine();
      }
   }

   unsigned iScopes=pIO->ScopeCount();

   for(unsigned iScope=0;iScope<iScopes;iScope++)
   {
      if(!IsScopeEmpty(pIO->ScopeGet(iScope)))
      {
         // If the Scope name has zero length, it's an Unnamed Scope.
         // Never treat it as a small scope, because you can't tell them apart!
         ConstString scopeName=pIO->ScopeName(iScope);

         if(scopeName.Length()>0 && DataInScope(pIO->ScopeGet(iScope))==1 && ScopesInScope(pIO->ScopeGet(iScope))==0)
         {
            Write_SmallDataScope(iScope, pIO);
            continue;
         }

         if(scopeName.Length()>0) // Write out the Scope name of a Named Scope
         {
            CharPut(' ', m_iScope*2);
            Write_String(scopeName, false);
            NewLine();
         }

         CharPut(' ', m_iScope*2);
         CharPut('{');

         NewLine();
         m_iScope++;
         Write_Scope(pIO->ScopeGet(iScope));
         m_iScope--;

         CharPut(' ', m_iScope*2);
         CharPut('}');
         NewLine();
      }
   }
}

//
void ConfigExport::Write_SmallDataScope(unsigned iScope, IParserIO *pIO)
//
// Writes out all data items for scope number iScope in pIO.  pIO must NOT
// have other sub-scopes!
{
   IParserIO *pIOData=pIO->ScopeGet(iScope);
   Assert(ScopesInScope(pIOData)==0);
   Assert(DataInScope(pIOData));

   ConstString scopeName=pIO->ScopeName(iScope);

   for(auto *p_info : pIOData->GetDataInfos())
   {
      if(!p_info->GetName()) // Collection?
      {
         const DataInfo_Collection &collection=static_cast<const DataInfo_Collection &>(*p_info);

         for(unsigned i=0;i<collection.ItemCount(pIOData);i++)
         {
            CharPut(' ', m_iScope*2);
            Write_String(scopeName, false);
            CharPut('.');
            Write_String(p_info->GetName(), false);
            CharPut('=');
            collection.Write(pIOData, *this, i);
            NewLine();
         }
      }
      else if(ShouldWrite(pIOData, *p_info))
      {
         CharPut(' ', m_iScope*2);
         Write_String(scopeName, false);
         CharPut('.');
         Write_String(p_info->GetName(), false);
         CharPut('=');
         p_info->Write(pIOData, *this);
         NewLine();
      }
   }
}

bool ConfigExport::IsScopeEmpty(IParserIO *pIO)
{
   // nullptr scopes are always empty
   if(pIO==nullptr) return true;

   for(auto *p_info : pIO->GetDataInfos())
   {
      if(!p_info->GetName()) // Collection?
      {
         auto &collection=static_cast<const DataInfo_Collection &>(*p_info);
         if(collection.ItemCount(pIO))
            return false;
      }
      else if(ShouldWrite(pIO, *p_info))
         return false;
   }

   unsigned iScopes=pIO->ScopeCount();
   for(unsigned iScope=0;iScope<iScopes;iScope++)
   {
      if(!IsScopeEmpty(pIO->ScopeGet(iScope)))
         return false;
   }
   return true;
}

bool ConfigExport::ShouldWrite(IParserIO *pIO, const DataInfo &info)
{
   return info.IsFlagSet(DataInfo::Flag_AlwaysWrite) || !info.IsFlagSet(DataInfo::Flag_NoWrite) && (!info.IsDefault(pIO) || m_fShowDefaults);
}

unsigned ConfigExport::DataInScope(IParserIO *pIO)
{
   unsigned iItems=0;

   for(auto *p_info : pIO->GetDataInfos())
   {
      if(!p_info->GetName()) // Collection?
      {
         auto &collection=static_cast<const DataInfo_Collection &>(*p_info);
         iItems+=collection.ItemCount(pIO);
      }
      else if(ShouldWrite(pIO, *p_info))
         iItems++;
   }

   return iItems;
}

unsigned ConfigExport::ScopesInScope(IParserIO *pIO)
{
   unsigned itemCount=0;
   unsigned iScopes=pIO->ScopeCount();

   for(unsigned iScope=0;iScope<iScopes;iScope++)
   {
      if(!IsScopeEmpty(pIO->ScopeGet(iScope)))
         itemCount++;
   }
   return itemCount;
}

/*

Config File Format:

; = Comment

Data=Value
Scope.Data=Value

Scope
{
  Data=Value
  Scope.Data=Value

  Scope
  {
    Data=Value
    ; Scopes can be nested as deep as necessary
  }
}

Scope.Scope.Scope.Data=Value

; Unnamed Scopes:

{
  Data=Value
}

*/
