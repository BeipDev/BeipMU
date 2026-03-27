struct ISound
{
   virtual ~ISound() { }

   virtual void Play()=0;
   virtual void Stop()=0;
   virtual bool IsPlaying()=0;
   virtual double GetCurrentTime()=0;
   virtual double GetDuration()=0;

   virtual void SetLoopCount(int loop_count)=0; // -1 to loop forever (0 and 1 do the same thing)
   virtual void SetVolume(float volume)=0;
};

bool PlaySound(ConstString filename, float volume=1.0f);
UniquePtr<ISound> CreateSoundFromMemory(Array<const BYTE> data);
void StopSounds();
