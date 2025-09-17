//
// FindString Stuff
//
#include "Main.h"
#include "FindString.h"

FindStringSearch::FindStringSearch(const Prop::FindString &fs, ConstString string)
: m_fs(fs), m_string(string)
{
}

bool FindStringSearch::Next()
{
   m_rangeCount=m_fs.Find(m_string, m_ranges[0].end, m_ranges).Count();
   if(m_rangeCount==0)
      return false;
   
   if(m_ranges[0].end==m_ranges[0].begin)
   {
      if(m_empty_seen)
         return false; // We saw nothing once, and we just saw it again, so exit
      m_empty_seen=true;
   }
   return true;
}

void EscapeHTML(StringBuilder &string, ConstString append)
{
   for(auto &c : append)
   {
      switch(c)
      {
         case '<': string("&lt;"); break;
         case '&': string("&amp;"); break;
         default: string(c); break;
      }
   }
}

FindStringReplacement::FindStringReplacement(ConstString replace)
{
   ConstString::operator=(replace);
}

FindStringReplacement::FindStringReplacement(const FindStringSearch &search, ConstString replace, bool escape_html)
{
   unsigned slash_index=replace.FindFirstOf('\\', replace.Count());

   // There were not any \n strings, so return and do the simple case.
   // We're not a regex string, so pass us straight through (even if search is regex)
   if(slash_index+1>=replace.Count())
   {
      ConstString::operator=(replace);
      return;
   }

   auto ranges=search.ranges();
   while(true)
   {
      m_replacement(replace.First(slash_index));
      replace=replace.WithoutFirst(slash_index);

      if(replace.Count()<2)
         break;

      char c=replace[1];
      if(unsigned value=c-'0'; value<ranges.Count())
      {
         auto sub=search.SearchString().Sub(ranges[value]);
         if(escape_html)
            EscapeHTML(m_replacement, sub);
         else
            m_replacement(sub);
      }
      else
      {
         m_replacement('\\');
         if(c!='\\') // Not an escaped slash?  Then echo the char
            m_replacement(c);
      }

      replace=replace.WithoutFirst(2);
      slash_index=replace.FindFirstOf('\\', replace.Count());
   }

   ConstString::operator=(m_replacement);
}

void FindStringReplacement::ExpandVariables(Prop::Variables &variables)
{
   unsigned first=Find('%');
   if(first==~0U)
      return;

   if(!m_replacement)
      m_replacement(*this);

   while(true)
   {
      unsigned second=m_replacement.FindFirstAt('%', first+1);
      if(second==~0U)
      {
         HybridStringBuilder<> message("Warning: Missing second '%' when processing: ", *this);
         ConsoleText(message);
         break;
      }

      auto name=m_replacement.Sub(first+1, second);

      unsigned index=variables.Find(name);
      if(index==~0U)
         break;

      auto &variable=variables[index];
      m_replacement.Replace(uint2(first, second+1), variable->pclValue());
      first=m_replacement.FindFirstAt('%', first+variable->pclValue().Count());
      if(first==~0U)
         break;
   }

   ConstString::operator=(m_replacement);
}