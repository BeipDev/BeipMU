#include "Main.h"
#include "Speller.h"
#include <SpellCheck.h>

#undef near
#include "..\win_api\hunspelldll.h"

namespace LibHunspell
{
   Library g_library;
   Hunspell *gp_hunspell{};

   //returns pointer to spell object, params are aff file name and dict file name
   static Hunspell *(__cdecl *Initialize)(const char *aff_file, const char *dict_file);
   //frees spell object
   static void(__cdecl *Uninitialize)(Hunspell *pMS);
   //spellcheck word, returns 1 if word ok otherwise 0
   static int(__cdecl *Spell)(Hunspell *pMS, const char *word);
   //suggest words for word, returns number of words in slst
   // YOU NEED TO CALL hunspell_suggest_free after you've done with words
   static int(__cdecl *Suggest)(Hunspell *pMS, const char *word, const char ***slst);
   #if 0
   LIBHUNSPELL_DLL_EXPORTED int hunspell_suggest_auto(Hunspell *pMS, char *word, char ***slst);
   #endif
   //free slst array
   static void(__cdecl *Free_List)(Hunspell *pMS, const char ***slst, int len);
   //add word to dict (word is valid until spell object is not destroyed)
   static int(__cdecl *Add)(Hunspell *pMS, const char *word);
   #if 0
   //make local copy of returned string!!
   LIBHUNSPELL_DLL_EXPORTED char * hunspell_get_dic_encoding(Hunspell *pMS);
   //add word to dict with affixes of the modelword (word is valid until spell object is not destroyed)
   LIBHUNSPELL_DLL_EXPORTED int hunspell_add_with_affix(Hunspell *pMS, char *word, char *modelword);
   #endif

   void Init()
   {
   #ifdef _DEBUG
      g_library.Load("C:\\Programming\\Hunspell-1.3.2\\src\\win_api\\Debug_dll\\libhunspell\\libhunspell.dll");
   //      g_library.Load("C:\\Programming\\Hunspell-1.3.2\\src\\win_api\\Release_dll\\libhunspell\\libhunspell.dll");
   #else
      g_library.Load("Hunspell.dll");
   #endif
      if(!g_library)
         return;

      g_library.GetProcAddress("hunspell_initialize", Initialize);
      g_library.GetProcAddress("hunspell_uninitialize", Uninitialize);
      g_library.GetProcAddress("hunspell_spell", Spell);
      g_library.GetProcAddress("hunspell_suggest", Suggest);
      g_library.GetProcAddress("hunspell_free_list", Free_List);
      g_library.GetProcAddress("hunspell_add", Add);

   #ifdef _DEBUG
      gp_hunspell=Initialize("C:\\Programming\\Hunspell-1.3.2\\src\\win_api\\en_US\\en_US.aff",
                             "C:\\Programming\\Hunspell-1.3.2\\src\\win_api\\en_US\\en_US.dic");
   #else
      gp_hunspell=Initialize("en_US.aff", "en_US.dic");
   #endif

      CallAtShutdown([](){ Uninitialize(gp_hunspell); });
   }
}

namespace Speller
{

struct ISpeller
{
   virtual ~ISpeller() noexcept { }

   virtual bool Check(ConstWString word)=0;
   virtual void Suggest(ConstWString word, Collection<OwnedString> &suggestions)=0;
   virtual void Add(ConstWString word)=0;
};

struct Speller_Hunspell : ISpeller
{
   Speller_Hunspell()
   {
      LibHunspell::Init();
      if(!LibHunspell::gp_hunspell)
         throw std::exception{};
   }

   bool Check(ConstWString word) override
   {
      if(!word) return true;
      return LibHunspell::Spell(LibHunspell::gp_hunspell, FixedStringBuilder<256>(word).Terminate())!=0;
   }

   void Suggest(ConstWString word, Collection<OwnedString> &suggestions) override
   {
      const char **p_words{};
      unsigned count=LibHunspell::Suggest(LibHunspell::gp_hunspell, FixedStringBuilder<256>(word).Terminate(), &p_words);

      for(unsigned i=0;i<count;i++)
         suggestions.Push(SzToString(p_words[i]));

      if(p_words)
         LibHunspell::Free_List(LibHunspell::gp_hunspell, &p_words, count);
   }

   void Add(ConstWString word) override
   {
      LibHunspell::Add(LibHunspell::gp_hunspell, FixedStringBuilder<256>(word).stringz());
   }
};

struct Speller_OS : ISpeller
{
   Speller_OS(CntPtrTo<ISpellChecker> &&p_spell_checker) : mp_spell_checker(std::move(p_spell_checker)) { }

   bool Check(ConstWString word) override;
   void Suggest(ConstWString word, Collection<OwnedString> &suggestions) override;
   void Add(ConstWString word) override;

private:
   CntPtrTo<ISpellChecker> mp_spell_checker;
};

bool Speller_OS::Check(ConstWString word)
{
   CntPtrTo<IEnumSpellingError> p_errors;
   if(mp_spell_checker->Check(FixedWStringBuilder<256>(word).Terminate(), p_errors.Address())!=S_OK)
      return true;

   Collection<ULONG> errors;
   while(true)
   {
      CntPtrTo<ISpellingError> p_error;
      if(p_errors->Next(p_error.Address())!=S_OK)
         break;
      p_error->get_StartIndex(&errors.Push());
   }
   return errors.Count()==0;
}

void Speller_OS::Add(ConstWString word)
{
   mp_spell_checker->Ignore(FixedWStringBuilder<256>(word).Terminate());
}

void Speller_OS::Suggest(ConstWString word, Collection<OwnedString> &suggestions)
{
   CntPtrTo<IEnumString> p_suggestions;
   mp_spell_checker->Suggest(FixedWStringBuilder<256>(word).Terminate(), p_suggestions.Address());
   while(true)
   {
      CoTaskHolder<OLECHAR> wzString;
      ULONG count;
      if(p_suggestions->Next(1, wzString.Address(), &count)!=S_OK)
         break;

      suggestions.Push(UTF8(WzToString(wzString)));
   }
}

struct Spellers
{
   Spellers()
   {
      mp_spell_checker_factory.CoCreateInstance(__uuidof(SpellCheckerFactory), CLSCTX_INPROC_SERVER);
      if(!mp_spell_checker_factory)
         throw std::exception{};
   }

   void GetLanguages(Controls::TComboBox<OwnedString> &list);
   UniquePtr<ISpeller> CreateForLanguage(ConstString language);

private:
   CntPtrTo<ISpellCheckerFactory> mp_spell_checker_factory;
};

void Spellers::GetLanguages(Controls::TComboBox<OwnedString> &list)
{
   CntPtrTo<IEnumString> p_languages;
   mp_spell_checker_factory->get_SupportedLanguages(p_languages.Address());

   while(true)
   {
      CoTaskHolder<OLECHAR> wzString;
      ULONG count;
      if(p_languages->Next(1, wzString.Address(), &count)!=S_OK)
         break;
      UTF8 locale(WzToString(wzString));
      auto display_name=GetLocaleInfo(locale, LOCALE_SLOCALIZEDDISPLAYNAME);
      if(!display_name)
         display_name=locale;
      int index=list.AddString(display_name);
      list.SetItemData(index, MakeUnique<OwnedString>(locale));
   }
}

UniquePtr<ISpeller> Spellers::CreateForLanguage(ConstString language)
{
   CntPtrTo<ISpellChecker> p_spell_checker;
   mp_spell_checker_factory->CreateSpellChecker(UTF16(language).stringz(), p_spell_checker.Address());
   if(p_spell_checker)
      return MakeUnique<Speller_OS>(std::move(p_spell_checker));
   return nullptr;
}

Spellers *GetSpellers()
{
   static bool s_inited{};
   static UniquePtr<Spellers> sp_spellers{};

   if(!s_inited)
   {
      s_inited=true;
      try
      {
         sp_spellers=MakeUnique<Spellers>();
         CallAtShutdown([](){ sp_spellers=nullptr; });
      }
      catch(const std::exception &)
      {
      }
   }
   return sp_spellers;
}

static UniquePtr<ISpeller> gp_speller;

void SetLanguage(ConstString language)
{
   if(auto *p_spellers=GetSpellers())
   {
      if(gp_speller==nullptr)
         CallAtShutdown([]() { gp_speller=nullptr; });
      gp_speller=p_spellers->CreateForLanguage(language);
   }
   else
   {
      static bool s_inited{};
      if(!s_inited)
      {
         s_inited=true;
         try
         {
            gp_speller=MakeUnique<Speller_Hunspell>();
            CallAtShutdown([](){ gp_speller=nullptr; });
         }
         catch(const std::exception &)
         {
         }
      }
   }
}

void GetLanguages(Controls::TComboBox<OwnedString> &list)
{
   if(auto *p_spellers=GetSpellers())
      p_spellers->GetLanguages(list);
}

bool Check(ConstWString word)
{
   if(!gp_speller) return true;
   return gp_speller->Check(word);
}

void Suggest(ConstWString word, Collection<OwnedString> &suggestions)
{
   if(!gp_speller) return;
   gp_speller->Suggest(word, suggestions);
}

void Add(ConstWString word)
{
   if(!gp_speller) return;
   gp_speller->Add(word);
}

}
