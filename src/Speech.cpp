#include "Main.h"
#include <sapi.h>
#include "Speech.h"

SAPI::SAPI()
{
   mp_voice.CoCreateInstance(CLSID_SpVoice);

   CntPtrTo<IEnumSpObjectTokens> p_enum;
   CntPtrTo<ISpObjectTokenCategory> p_category;
   p_category.CoCreateInstance(CLSID_SpObjectTokenCategory);
   //      p_category->SetId(SPCAT_VOICES, FALSE); // L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Voices"
   p_category->SetId(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices", FALSE);
   p_category->EnumTokens(nullptr, nullptr, p_enum.Address());
   if(p_enum)
   {
      ULONG count{};
      p_enum->GetCount(&count);
      m_voices.AllocateForSize(count);

      for(unsigned i=0; i<count; i++)
      {
         auto &voice=m_voices.Push();
         p_enum->Next(1, voice.mp_token.Address(), nullptr);

         CntPtrTo<ISpDataKey> p_attributes;
         voice.mp_token->OpenKey(L"Attributes", p_attributes.Address());

         CoTaskHolder<wchar_t> name, gender, lcid_string;
         p_attributes->GetStringValue(L"name", name.Address());
         p_attributes->GetStringValue(L"gender", gender.Address());
         p_attributes->GetStringValue(L"language", lcid_string.Address());

         DWORD lcid{};
         FixedStringBuilder<256> lcid_string8(WzToString(lcid_string));
         lcid_string8.HexTo(lcid);
         auto language=GetLocaleInfo(LCIDToLocale(lcid));

         voice.m_name=FixedStringBuilder<256>(WzToString(name), " - ", WzToString(gender), " - ", language);

         CoTaskHolder<wchar_t> id;
         voice.mp_token->GetId(id.Address());
         voice.m_id=FixedStringBuilder<256>(WzToString(id));
      }
   }
   SetVoice();

   CallAtShutdown([]() { s_p=nullptr; });
}

void SAPI::SetVoice()
{
   if(!mp_voice)
      return;

   if(g_ppropGlobal->pclVoiceID())
   {
      CntPtrTo<ISpObjectToken> p_token;
      p_token.CoCreateInstance(CLSID_SpObjectToken);
      if(p_token)
      {
         p_token->SetId(nullptr, UTF16(g_ppropGlobal->pclVoiceID()).stringz(), FALSE);
         mp_voice->SetVoice(p_token);
      }
   }
   else
      mp_voice->SetVoice(nullptr);
}

void SAPI::Say(ConstString text)
{
   if(mp_voice)
      mp_voice->Speak(UTF16(text).stringz(), SPF_ASYNC, NULL);
}

void SAPI::Stop()
{
   if(mp_voice)
      mp_voice->Speak(L"", SPF_PURGEBEFORESPEAK|SPF_ASYNC, NULL);
}

SAPI &SAPI::GetInstance()
{
   if(!s_p)
      s_p=MakeUnique<SAPI>();
   return *s_p;
}

UniquePtr<SAPI> SAPI::s_p;

void StopSpeech()
{
   if(SAPI::HasInstance())
      SAPI::GetInstance().Stop();
}

#ifdef TOLK

#include "tolk/src/Tolk.h"
#pragma comment(lib, "tolk/bin/x86/Tolk.lib")

struct Tolk
{
   Tolk()
   {
      Tolk_Load();
      //      Say("BeipMU is using Tolk!");
   }

   ~Tolk()
   {
      Tolk_Unload();
   }


   void Say(ConstString text)
   {
      Tolk_Output(UTF16(text).stringz());
   }

   static Tolk &GetInstance()
   {
      if(!sp_tolk)
         sp_tolk=MakeUnique<Tolk>();
      return *sp_tolk;
   }

private:
   static UniquePtr<Tolk> sp_tolk;
};

UniquePtr<Tolk> Tolk::sp_tolk;
#endif
