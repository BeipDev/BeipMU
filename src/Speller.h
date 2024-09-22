namespace Speller
{
   // Until SetLanguage is called, speller does nothing
   void SetLanguage(ConstString language);
   void GetLanguages(Controls::TComboBox<OwnedString> &list);

   bool Check(ConstWString word);
   void Suggest(ConstWString word, Collection<OwnedString> &suggestions);
   void Add(ConstWString word);
};
