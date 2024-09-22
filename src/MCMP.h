struct Client_Media
{
   void DumpInfo(Text::Wnd &wnd);
   void OnGMCP(ConstString command, ConstString json);

private:

   struct File : IDownload, UIThreadMessagePoster
   {
      File(Client_Media &client) : m_client{client} { }

      void OnDownloadUpdate() override;
      void OnDownloadUpdate_UI(); // UI Thread
      void Play();
      void Stop();

      enum struct Type { Sound, Music, Video } m_type;

      Client_Media &m_client;
      OwnedString m_name;
      OwnedArray<BYTE> m_data;
      bool m_play_after_download{};
      int m_loop_count{1};
      float m_volume{1.0f};
      bool m_continue{false};
      UniquePtr<AsyncDownloader> mp_downloader;
      UniquePtr<ISound> mp_sound;
   };

   File *Find(ConstString name);
   File *Load(ConstString url_start, ConstString name);

   OwnedString m_url;
   Collection<UniquePtr<File>> m_files;
};
