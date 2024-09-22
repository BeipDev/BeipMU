struct Wnd_Image : TWindowImpl<Wnd_Image>, IDownload, OwnerPtrData, AnimatedGif::INotify
{
   static ATOM Register();

   Wnd_Image(Wnd_Main &wnd_main);
   ~Wnd_Image();

   Wnd_Docking &GetDocking() { return *mp_docking; }

   void OpenUrl(Text::ImageURL &&url);
   void OpenUrl(unsigned index);
   void OnImage(const Imaging::Image &image);
   void OnFrame() override;

   struct Msg_Update : Windows::Message
   {
      static const MessageID ID=WM_USER;
      Msg_Update() : Windows::Message{ID, 0, 0} { }
   };

   LRESULT WndProc(const Windows::Message &msg) override;

   LRESULT On(const Msg::XButtonDown &msg);
   LRESULT On(const Msg::LButtonDown &msg);
   LRESULT On(const Msg::LButtonUp &msg);
   LRESULT On(const Msg::CaptureChanged &msg);
   LRESULT On(const Msg::MouseMove &msg);
   LRESULT On(const Msg::RButtonDown &msg);
   LRESULT On(const Msg::Key &msg);
   LRESULT On(const Msg::Paint &msg);
   LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
   LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }
   LRESULT On(const Msg_Update &msg);

   // IDownload
   void OnDownloadUpdate() override;

private:

   Wnd_Main &m_wnd_main;
   Handle<HBITMAP> m_bitmap;
   Wnd_Docking *mp_docking;
   Collection<Text::ImageURL> m_urls;
   unsigned m_url_index{};

   UniquePtr<AnimatedGif> m_agif;
   bool m_animate_gifs{!g_ppropGlobal->fAnimatedImagesStartPaused()};

   Time::Timer m_timerHide{[this] { m_show_arrows=false; Invalidate(true); }};
   bool m_show_arrows{};

   UniquePtr<AsyncDownloader> mp_downloader;
};

UniquePtr<Wnd_Image> CreateImageWindow(Wnd_Main &wndMain);
