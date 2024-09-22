struct ISpObjectToken;
struct ISpVoice;

struct SAPI
{
   struct Voice
   {
      CntPtrTo<ISpObjectToken> mp_token;
      OwnedString m_name;
      OwnedString m_id;
   };

   SAPI();

   void SetVoice();
   void Say(ConstString text);
   void Stop();

   Array<Voice> GetVoices() const { return m_voices; }


   static bool HasInstance() { return s_p; }
   static SAPI &GetInstance();

private:

   CntPtrTo<ISpVoice> mp_voice;
   Collection<Voice> m_voices;

   static UniquePtr<SAPI> s_p;
};

void StopSpeech();
