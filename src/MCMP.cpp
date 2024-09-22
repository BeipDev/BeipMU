#include "Main.h"
#include "JSON.h"
#include "Sounds.h"
#include "MCMP.h"

struct Media_Default : JSON::Element
{
   void OnString(ConstString name, ConstString value) override
   {
      if(name=="url")
         m_url=value;
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      Assert(false);
   }

   OwnedString m_url;
};

struct Media_Load : JSON::Element
{
   void OnString(ConstString name, ConstString value) override
   {
      if(name=="name")
         m_name=value;
      else if(name=="url")
         m_url=value;
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      Assert(false);
   }

   OwnedString m_name, m_url;
};

struct Media_Play : JSON::Element
{
   void OnString(ConstString name, ConstString value) override
   {
      if(name=="name")
         m_name=value;
      else if(name=="url")
         m_url=value;
      else if(name=="type")
         m_type=value;
      else if(name=="continue")
         m_continue=value=="true";
      else if(name=="tag")
         m_tag=value;
      else if(name=="key")
         m_key=value;
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      if(name=="loops")
         m_loops=value;
      else if(name=="volume")
         m_volume=value;
      else if(name=="priority")
         m_priority=value;
      else
         Assert(false);
   }

   OwnedString m_name, m_url, m_key, m_tag, m_type;
   int m_loops{1};
   int m_volume{50}, m_priority{1};
   bool m_continue{true};
};

struct Media_Stop : JSON::Element
{
   void OnString(ConstString name, ConstString value) override
   {
      if(name=="name")
         m_name=value;
      else if(name=="type")
         m_type=value;
      else if(name=="tag")
         m_tag=value;
      else if(name=="key")
         m_key=value;
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      if(name=="priority")
         m_priority=value;
      else
         Assert(false);
   }

   OwnedString m_name, m_type, m_tag, m_key;
   int m_priority{1};
};

void DisplayMediaTime(StringBuilder &string, double time)
{
   int ms=int (time*1000.0)%1000;
   int seconds=time;
   int minutes=seconds/60; seconds%=60;
   int hours=minutes/60;

   if(hours)
      string(Strings::Int(hours, 2), ':');

   string(Strings::Int(minutes, 2, '0'), ':', Strings::Int(seconds, 2, '0'), '.', Strings::Int(ms, 3, '0'));
}

void Client_Media::DumpInfo(Text::Wnd &wnd)
{
   wnd.AddHTML("<icon information> GMCP Sound status");
   wnd.AddHTML(FixedStringBuilder<1024>("Default URL: <font color='aqua'>", Text::NoHTML_Start, m_url, Text::NoHTML_End));

   for(auto &file : m_files)
   {
      FixedStringBuilder<1024> string;
      string("Name: <font color='aqua'>", Text::NoHTML_Start, file->m_name, Text::NoHTML_End, "</font>  Size: <font color='aqua'>");
      ByteCountToStringAbbreviated(string, file->m_data.Count());
      string("</font>  Volume: <font color='aqua'>", file->m_volume, "</font>  Loop Count: <font color='aqua'>", file->m_loop_count, "</font> ");
      if(file->mp_sound)
      {
         if(file->mp_sound->IsPlaying())
         {
            string(" <font color='green'>Playing </font>");
            DisplayMediaTime(string, file->mp_sound->GetCurrentTime());
            string(" / ");
         }
         DisplayMediaTime(string, file->mp_sound->GetDuration());
      }
      wnd.AddHTML(string);
   }

   wnd.AddHTML("<icon information> GMCP Sound status end");
}

void Client_Media::OnGMCP(ConstString command, ConstString json)
{
   if(command=="default")
   {
      Media_Default element;
      JSON::ParseObject(element, json);
      m_url=std::move(element.m_url);
   }
   else if(command=="load")
   {
      Media_Load element;
      JSON::ParseObject(element, json);

      if(auto pFile=Find(element.m_name))
         return; // Already exists

      Load(element.m_url, element.m_name);
   }
   else if(command=="play")
   {
      Media_Play element;
      JSON::ParseObject(element, json);
      auto *p_file=Find(element.m_name);
      if(!p_file)
         p_file=Load(element.m_url, element.m_name);

      if(element.m_type=="music")
         p_file->m_type=File::Type::Music;
      else if(element.m_type=="sound")
         p_file->m_type=File::Type::Sound;
      else if(element.m_type=="video")
         p_file->m_type=File::Type::Video;

      p_file->m_continue=element.m_continue;
      p_file->m_loop_count=element.m_loops;
      p_file->m_volume=element.m_volume/100.0f;
      p_file->Play();
   }
   else if(command=="stop")
   {
      Media_Stop element;
      JSON::Parse(element, json);

      if(!element.m_name)
      {
         for(auto &file : m_files)
            file->Stop();
         return;
      }

      if(auto *pFile=Find(element.m_name))
         pFile->Stop();
   }
   else
      Assert(false);
}

Client_Media::File *Client_Media::Find(ConstString name)
{
   for(auto &file : m_files)
   {
      if(file->m_name==name)
         return file;
   }
   return nullptr;
}

Client_Media::File *Client_Media::Load(ConstString url_start, ConstString name)
{
   auto file=MakeUnique<File>(*this);
   file->m_name=name;

   FixedStringBuilder<256> url;
   if(url_start)
      url(url_start);
   else
   {
      if(!m_url)
         return nullptr;
      url(m_url);
   }

   url(name);
   file->mp_downloader=MakeUnique<AsyncDownloader>(*file, url);
   return m_files.Push(std::move(file));
}

void Client_Media::File::OnDownloadUpdate()
{
   if(mp_downloader->GetStatus()==AsyncDownloader::Status::Downloading)
      return; // No need to pass these on

   Post([this]() { OnDownloadUpdate_UI(); });
}

void Client_Media::File::OnDownloadUpdate_UI()
{
   AssertMainThread();
   switch(mp_downloader->GetStatus())
   {
      case AsyncDownloader::Status::Complete:
         m_data=::File::Load(mp_downloader->GetFilename());
         if(m_data)
            mp_sound=CreateSoundFromMemory(m_data);
         if(m_play_after_download)
            Play();
         mp_downloader=nullptr;
         break;

      case AsyncDownloader::Status::Failed:
         mp_downloader=nullptr;
         break;
   }
}

void Client_Media::File::Play()
{
   if(!mp_sound) // The sound isn't ready yet, so set it to play when ready
   {
      Assert(mp_downloader);
      m_play_after_download=true;
      return;
   }

   mp_sound->SetLoopCount(m_loop_count);
   mp_sound->SetVolume(m_volume);

   if(!m_continue || !mp_sound->IsPlaying())
      mp_sound->Play();
}

void Client_Media::File::Stop()
{
   m_play_after_download=false;
   if(mp_sound)
      mp_sound->Stop();
}

#if 0
void TestJSON()
{
   // Client.Media.Load
   Media_Load command_load;
   JSON{command_load, R"({"name": "sword1.wav","url": "hxxps://www.example.com/media/"})"};

   // Client.Media.Stop 
   Media_Stop command_stop;
   JSON{command_stop, R"({ "name": "city.mp3","type": "music","tag": "environment","priority": 60,"key": "area-background-music"})"};

   // Client.Media.Play 
   Media_Play command_play;
   JSON{command_play, R"({ "continue": "true", "loops": -1, "name": "ambience/loops/hammer.wav", "tag": "", "type": "music", "url": "deathcult.today/soundhell", "volume": 25 })"};
}
bool _=(TestJSON(), true);
#endif
