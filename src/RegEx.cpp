//
// Regular Expression Wrapper Class
//

#include "Main.h"

#if 0
// This code was an attempt to figure out if the C++ regex library was any good, but
// it allocates a lot more memory to do the same expression. Need to wait to see if they
// improve it later.

   LString test("This is a test");

   std::regex regex("[a-e]");
   std::regex_iterator<const char *> it(test.begin(), test.end(), regex);
   std::regex_iterator<const char*> end;

   {
      RegEx::Expression expression("[a-e]");
      FixedArray<uint2, 15> storage;
      auto results=expression.Find(test, 0, storage);
   }

   TString<256> results;

   while(it!=end)
   {
      auto &match=*it;
      results(test.Sub(match.position()).First(match.length()));
      ++it;
   }
#endif

namespace RegEx
{

Expression::Expression(ConstString expression, int options)
{
   Assert(expression);
   m_code=pcre2_compile((PCRE2_UCHAR8*)expression.begin(), expression.Length(), options, &m_error_code, &m_error_offset, nullptr);
   if(!m_code)
      return;

   m_data=pcre2_match_data_create_from_pattern(m_code, nullptr);
}

Expression::~Expression()
{
   pcre2_code_free(m_code);
   pcre2_match_data_free(m_data);
}

void Expression::GetError(StringBuilder &string) const
{
   auto chars=string.Allocate(200);
   int count=pcre2_get_error_message(m_error_code, (PCRE2_UCHAR8*)chars.begin(), chars.Count());
   string.AddLength(count-chars.Count()); // Add back unused space
}

#if 0
std::optional<uint2> Expression::Find(ConstString string, unsigned start) const
{
   int result_count=pcre2_match(m_code, (PCRE2_UCHAR8*)string.begin(), string.Length(), start, 0, m_data, nullptr);
   if(result_count<=0)
      return {};

   auto ovector=pcre2_get_ovector_pointer(m_data);
   if(ovector[1]==start)
      return {}; // Ignore empty matches

   return uint2{unsigned(ovector[0]), unsigned(ovector[1])};
}
#endif

Array<size_t_2> Expression::Find(ConstString string, unsigned start) const
{
   int result_count=pcre2_match(m_code, (PCRE2_SPTR8)string.begin(), string.Length(), start, PCRE2_NO_UTF_CHECK, m_data, nullptr);
   if(result_count<=0)
      return {};

   auto *ovector=pcre2_get_ovector_pointer(m_data);
   // static_assert that ovector is of type size_t
   static_assert(std::is_same_v<decltype(ovector), size_t *>);

   return {reinterpret_cast<size_t_2*>(pcre2_get_ovector_pointer(m_data)), static_cast<unsigned>(result_count)};
}

};
