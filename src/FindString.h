//
// FindString Tools
//

struct FindStringSearch
{
   FindStringSearch(const Prop::FindString &fs, ConstString string);

   bool Next();

   Array<const uint2> ranges() const { return m_ranges.First(m_rangeCount); }

   const uint2 RangeFound() const { return m_ranges[0]; }
   unsigned Start() const { return m_ranges[0].begin; }
   unsigned End() const { return m_ranges[0].end; }

   // After a replace operation, we need to adjust the search end to after the replacement
   // We pass in the string again because typically after advancing, the original string passed in
   // was realloc'd such that m_string is no longer valid anyways.
   void HandleReplacement(unsigned replacementLength, ConstString string)
   {
      m_string=string;
      m_ranges[0].end=m_ranges[0].begin+replacementLength;
   }

   ConstString SearchString() const { return m_string; }

private:
   const Prop::FindString &m_fs;
   ConstString m_string;
   bool m_empty_seen{}; // If we ever match on nothing, we need to find nothing the second time through, or we'll loop forever

   FixedArray<uint2, 15> m_ranges;
   unsigned m_rangeCount{}; // Number of ranges filled in by the RegExFind() function
};

struct FindStringReplacement : ConstString
// Given an ConstString, can generate regex \1 \2 \3 type replacement strings when you call 'Get' during a RegEx search
{
   FindStringReplacement(const FindStringSearch &search, ConstString replace, bool escape_html=false);

   void ExpandVariables(Collection<Variable> &variables);

private:
   HybridStringBuilder<> m_replacement;
};
