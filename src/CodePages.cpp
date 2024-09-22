#include "Main.h"
#include "CodePages.h"

// This is used when sending text to a server to translate local text into the target server's codepage
UTF8ToCodePage::UTF8ToCodePage(ConstString string, UINT code_page) : TString<char>(mp_buffer, _countof(mp_buffer)-1)
{
   // The idea is to convert to UTF16 first, then back to the codePage
   UTF16 string16(string);
   m_count=UTF16ToCodePage(code_page, 0, string16, *this);
   if(!Count())
   {
      m_count=UTF16ToCodePage(code_page, 0, string16, String());
      m_p=mp_buffer_huge=MakeUnique<char[]>(m_count+1);
      UTF16ToCodePage(code_page, 0, string16, *this);
   }
   m_p[m_count]=0; // Null terminate
}

// This translates the characters 128-255 from a given codepage into their UTF8 counterparts
// It's used when receiving text from a server
struct CodePageTable
{
   CodePageTable(unsigned code_page)
   {
      // Fill an array with the char values 128-255
      char code_page_chars[128];
      for(unsigned i=0;i<128;i++)
         code_page_chars[i]=i+128;

      ConstString code_page_string(code_page_chars, 128);

      OwnedWString wstring; wstring.Allocate(CodePageToUTF16(code_page, 0, code_page_string, WString()));
      CodePageToUTF16(code_page, 0, code_page_string, wstring);

      m_translation=UTF8(wstring);

      unsigned index=0;
      m_indices[0]=0;
      for(unsigned i=1;i<129;i++)
      {
         index+=UTF8ByteCount(m_translation[index]);
         m_indices[i]=index;
      }
      Assert(index==m_translation.Count());
   }

   ConstString operator[](unsigned char c) { Assert(c>=128); c-=128; return m_translation.Sub(m_indices[c], m_indices[c+1]); }

private:

   OwnedString m_translation;
   unsigned m_indices[129];
};

ConstString CP1252toUTF8(unsigned char c)
{
   static CodePageTable s_table(1252);
   return s_table[c];
}

ConstString CP437toUTF8(unsigned char c)
{
   static CodePageTable s_table(437);
   return s_table[c];
}
