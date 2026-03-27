#include "Main.h"
#include "Sounds.h"

#include <mfapi.h>
#include <mfmediaengine.h>
#pragma comment(lib, "Mfplat.lib")

struct SoundPlayer;
UniquePtr<SoundPlayer> gp_sound_player;

struct Engine;
struct MediaNotify : General::Unknown<IMFMediaEngineNotify>
{
   MediaNotify(Engine &engine) : m_engine(engine) { }

   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override
   {
      return TQueryInterface(riid, ppvObj);
   }

   STDMETHODIMP EventNotify(DWORD event, DWORD_PTR param1, DWORD param2) override;

private:
   Engine &m_engine;
};

struct Engine final : ISound
{
   Engine(IMFMediaEngineClassFactory &factory)
   {
      if(FAILED(MFCreateAttributes(mp_attributes.Address(), 1)))
         throw std::exception{};
      mp_attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, mp_notify);

      if(FAILED(factory.CreateInstance(MF_MEDIA_ENGINE_AUDIOONLY, mp_attributes, mp_engine.Address())))
         throw std::exception{};
   }

   ~Engine()
   {
      mp_engine->Pause();
      mp_engine->SetSource(nullptr);
      mp_engine->Shutdown(); // Without this, the sounds keep playing. Releasing isn't enough
   }

   bool IsPlaying() override
   {
      return !mp_engine->IsEnded() && !mp_engine->IsPaused();
   }

   double GetCurrentTime() override
   {
      return mp_engine->GetCurrentTime();
   }

   double GetDuration()
   {
      return mp_engine->GetDuration();
   }

   void Play() override
   {
      mp_engine->Play();
   }

   void Stop() override
   {
      mp_engine->Pause();
      mp_engine->SetCurrentTime(0.0);
   }

   void SetVolume(float volume) override
   {
      mp_engine->SetVolume(volume);
   }

   void SetLoopCount(int loop_count)
   {
      m_loop_count=loop_count;
      if(loop_count<0 || loop_count>1)
         mp_engine->SetLoop(true);
   }

   void Create(Array<const BYTE> data)
   {
      CntPtrTo<IMFMediaEngineEx> pEngineEx; mp_engine->QueryInterface(pEngineEx.Address());
      CntPtrTo<IStream> p_stream=Storage::CreateIStream(data);
      CntPtrTo<IMFByteStream> p_MFByteStream;
      MFCreateMFByteStreamOnStreamEx(p_stream, p_MFByteStream.Address());

      if(FAILED(pEngineEx->SetSourceFromByteStream(p_MFByteStream, UnconstPtr(UTF16("file://").stringz()))))
         return;
   }

   bool Play(ConstString filename, float volume)
   {
      if(FAILED(mp_engine->SetSource(UnconstPtr(UTF16(filename).stringz()))))
         return false;

      mp_engine->SetVolume(volume);
      mp_engine->Play();
      return true;
   }

private:
   CntPtrTo<MediaNotify> mp_notify{MakeCounting<MediaNotify>(*this)};
   CntPtrTo<IMFAttributes> mp_attributes;
   CntPtrTo<IMFMediaEngine> mp_engine;
   int m_loop_count{1};
   friend MediaNotify;
};

STDMETHODIMP MediaNotify::EventNotify(DWORD event, DWORD_PTR param1, DWORD param2)
{
   switch(event)
   {
      case MF_MEDIA_ENGINE_EVENT_RATECHANGE: break;
      case MF_MEDIA_ENGINE_EVENT_VOLUMECHANGE: break;
      case MF_MEDIA_ENGINE_EVENT_PLAYING: break;

      case MF_MEDIA_ENGINE_EVENT_SEEKING:
         if(m_engine.m_loop_count>1)
         {
            if(m_engine.m_loop_count==2)
               m_engine.mp_engine->Pause();

            if(--m_engine.m_loop_count==2) // Set looping to false with two more to go, as it'll loop one more time after this
               m_engine.mp_engine->SetLoop(false);
         }
         break;
      case MF_MEDIA_ENGINE_EVENT_ENDED:
      case MF_MEDIA_ENGINE_EVENT_ERROR: 
      {
// No good way to let the user know as this would be annoying
//         MessageBox(nullptr, STR_CantPlaySound, STR_Error, MB_OK|MB_ICONEXCLAMATION);
         m_engine.mp_engine->Pause(); // If we don't pause, we're marked as still playing. So pause to be able to re-use this slot
         break;
      }

      case MF_MEDIA_ENGINE_EVENT_CANPLAY: break;
   }
   return S_OK;
}

struct SoundPlayer
{
   SoundPlayer()
   {
      MFStartup(MF_VERSION);

      if(FAILED(mp_factory.CoCreateInstance(CLSID_MFMediaEngineClassFactory)))
         return;
   }

   ~SoundPlayer()
   {
      // Destroy all existing sounds before shutting down media foundation
      for(auto &p_engine : mp_engines)
         p_engine=nullptr;
      MFShutdown();
   }

   UniquePtr<ISound> Create(Array<const BYTE> data)
   {
      auto pEngine=MakeUnique<Engine>(*mp_factory);
      pEngine->Create(data);
      return pEngine;
   }

   bool Play(ConstString filename, float volume)
   {
      if(!mp_factory)
         return PlaySoundW(UTF16(filename).stringz(), nullptr, SND_FILENAME|SND_ASYNC);

      if(auto *p_engine=GetEngine())
         return p_engine->Play(filename, volume);

      return false;
   }

   Engine *GetEngine()
   {
      for(auto &p_engine : mp_engines)
      {
         // If this object is busy, keep looking
         if(p_engine && p_engine->IsPlaying())
            continue;

         try
         {
            if(!p_engine)
               p_engine=MakeUnique<Engine>(*mp_factory);
         }
         catch(const std::exception &)
         {
            break;
         }

         return p_engine;
      }

      return nullptr;
   }

private:

   static const unsigned c_engine_count{16};

   CntPtrTo<IMFMediaEngineClassFactory> mp_factory;
   UniquePtr<Engine> mp_engines[c_engine_count];
};

bool PlaySound(ConstString filename, float volume)
{
   if(!gp_sound_player)
      gp_sound_player=MakeUnique<SoundPlayer>();

   return gp_sound_player->Play(filename, volume);
} 

UniquePtr<ISound> CreateSoundFromMemory(Array<const BYTE> data)
{
   if(!gp_sound_player)
      gp_sound_player=MakeUnique<SoundPlayer>();

   return gp_sound_player->Create(data);
}

void StopSounds()
{
   if(!gp_sound_player)
      return;

   gp_sound_player=nullptr;
}