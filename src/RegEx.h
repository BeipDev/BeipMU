namespace RegEx
{
#define PCRE2_STATIC 1
#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2-10.42\src\pcre2.h"

struct Expression
{
   Expression(ConstString expression, int options);
   ~Expression() noexcept;

   bool HasError() const { return !m_code; }
   void GetError(StringBuilder &string) const;
   unsigned GetErrorOffset() const { return unsigned(m_error_offset); }

   bool Find(ConstString string, unsigned start, uint2 &found) const;
   Array<uint2> Find(ConstString string, unsigned start, Array<uint2> ranges) const;

private:
   Expression(const Expression&)=delete; // No way to copy the cached values
   void operator=(const Expression&)=delete;

   pcre2_code *m_code{};
   pcre2_match_data *m_data{};

   int m_error_code{};
   size_t m_error_offset{};
};

};
