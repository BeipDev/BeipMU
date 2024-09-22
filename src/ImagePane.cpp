#include "Main.h"
#include "Wnd_Main.h"
#include "AnimatedGif.h"
#include "ImagePane.h"
#pragma comment(lib,"urlmon.lib")

ATOM Wnd_Image::Register()
{
   WndClass wc(L"Wnd_Image");
   wc.style=CS_HREDRAW|CS_VREDRAW;
   wc.LoadIcon(IDI_APP, g_hInst);
   wc.hCursor=LoadCursor(nullptr, IDC_ARROW);
   wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);

   return wc.Register();
}

Wnd_Image::Wnd_Image(Wnd_Main &wndMain)
 : m_wnd_main(wndMain)
{
   Create("Image Viewer", WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME, Window::Position_Default, m_wnd_main);

   // Default to main window width and 1/4 main window height
   Rect rc=wndMain.ClientRect();
   SetSize(FrameSize()+int2(rc.size().x, rc.size().y/4));
   mp_docking=&m_wnd_main.CreateDocking(*this);
}

Wnd_Image::~Wnd_Image()
{
}

// IDownload
void Wnd_Image::OnDownloadUpdate()
{
   Msg_Update().Post(*this);
}

void Wnd_Image::OpenUrl(Text::ImageURL &&url)
{
   if(m_urls.Count()>=20)
      m_urls.Delete(0);

   m_urls.Push(std::move(url));
   OpenUrl(m_urls.Count()-1);
}

void Wnd_Image::OpenUrl(unsigned index)
{
   if(index>=m_urls.Count())
      return;

   m_url_index=index;
   mp_downloader=nullptr; // Cancel old one first
   mp_downloader=MakeUnique<AsyncDownloader>(*this, m_urls[m_url_index].GetDownloadURL());
   SetText(FixedStringBuilder<256>("Image Viewer (", m_url_index+1, " of ", m_urls.Count(), ')'));
}

void Wnd_Image::OnImage(const Imaging::Image &image)
{
   if(image)
   {
      m_agif=nullptr;
      m_bitmap=nullptr;

      if(image.IsAnimated())
         m_agif=MakeUnique<AnimatedGif>(*this, *image.m_pIDecoder);
      else
         m_bitmap=image.GetBitmap();
   }
   Invalidate(true);
}

void Wnd_Image::OnFrame()
{
   Invalidate(false);
   if(!m_animate_gifs && m_agif)
      m_agif->Stop();
}

LRESULT Wnd_Image::WndProc(const Windows::Message &msg)
{
   return Dispatch<WindowImpl, Msg::_GetThis, Msg::_GetTypeID, Msg::MouseMove, Msg::XButtonDown, Msg::LButtonDown, Msg::RButtonDown, Msg::Paint, Msg_Update>(msg);
}

LRESULT Wnd_Image::On(const Msg_Update &msg)
{
   if(mp_downloader)
   {
      switch(mp_downloader->GetStatus())
      {
         case AsyncDownloader::Status::Downloading:
            Invalidate(false);
            break; // Repaint progress
         case AsyncDownloader::Status::Complete:
            OnImage(Imaging::Image(mp_downloader->GetFilename()));
            mp_downloader=nullptr;
            break;
         case AsyncDownloader::Status::Failed:
            Invalidate(false);
            mp_downloader=nullptr;
            break;
      }
   }
   return msg.Success();
}

LRESULT Wnd_Image::On(const Msg::MouseMove &msg)
{
   m_show_arrows=true;
   Invalidate(false);
   m_timerHide.Set(1.0f);
   return msg.Success();
}

LRESULT Wnd_Image::On(const Msg::XButtonDown &msg)
{
   if(msg.button()==XBUTTON1)
      OpenUrl(m_url_index-1);

   if(msg.button()==XBUTTON2)
      OpenUrl(m_url_index+1);

   return msg.Success();
}

LRESULT Wnd_Image::On(const Msg::LButtonDown &msg)
{
   int width=ClientRect().size().x;

   if(msg.position().x<width/2) // Left?
      OpenUrl(m_url_index-1);

   if(msg.position().x>width/2)
      OpenUrl(m_url_index+1);

   return msg.Success();
}

LRESULT Wnd_Image::On(const Msg::RButtonDown &msg)
{
   enum struct Commands : UINT_PTR
   {
      Clear=1,
      OpenInBrowser,
      AnimateGifs,
      Help,
   };

   PopupMenu menu;
   PopupMenu menuUrls;
   if(m_urls)
   {
      for(unsigned i=0;i<m_urls.Count();i++)
         menuUrls.Append(0, (UINT_PTR)100+i, m_urls[i].m_original);
      CheckMenuItem(menuUrls, 100+m_url_index, MF_CHECKED);
   }
   else
      menuUrls.Append(0, 0, "(Empty)");
   menu.Append(std::move(menuUrls), "History");

   menu.Append(0, (UINT_PTR)Commands::Clear, "Clear");
   menu.Append(0, (UINT_PTR)Commands::OpenInBrowser, "Open in browser");
   menu.Append(m_animate_gifs ? MF_CHECKED : 0, (UINT_PTR)Commands::AnimateGifs, "Animate gifs");
   menu.Append(0, (UINT_PTR)Commands::Help, "Help");

   auto command=TrackPopupMenu(menu, TPM_RETURNCMD, GetCursorPos(), *this, nullptr);

   switch(Commands(command))
   {
      case Commands::Clear:
         m_agif=nullptr;
         m_bitmap.Empty();
         Invalidate(true);
         break;
      case Commands::OpenInBrowser:
         if(m_url_index<m_urls.Count())
            OpenURLAsync(m_urls[m_url_index].m_original);
         break;
      case Commands::AnimateGifs:
         m_animate_gifs^=true;
         if(m_agif)
         {
            if(m_animate_gifs)
               m_agif->Start();
            else
               m_agif->Stop();
         }
         break;
      case Commands::Help:
         MessageBox(*this, "This window will download and display any URLs seen in the output window that end in .jpg/.png/.gif\n\n"
                    "To nagivate, click to the left or right half of the window, or use the mouse forward/back buttons\n\n"
                    "To prevent it from automatically displaying, go to Options->Preferences->'Show image viewer automatically'",
                    "Help", MB_OK);
         break;
   }

   if(command>=100)
      OpenUrl(command-100);

   return msg.Success();
}

LRESULT Wnd_Image::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);

   if(m_agif)
   {
      auto bitmapSize=m_agif->Size();
      Rect rcWindow=TouchFromInside(RectF(float2(), ClientRect().size()), bitmapSize);
      m_agif->Render(ps, rcWindow);
   }
   else if(m_bitmap)
   {
      auto bitmapSize=BitmapInfo(m_bitmap).size();

      Rect rect_window=TouchFromInside(RectF(float2(), ClientRect().size()), bitmapSize);
      Rect rect_bitmap(int2(), bitmapSize);

      BitmapDC dcBitmap{m_bitmap, ps};
      ps.StretchBltMode(HALFTONE);
      ps.StretchBlt(rect_window, dcBitmap, rect_bitmap, SRCCOPY);
   }

   if(mp_downloader)
   {
      ps.SelectFont(Controls::Control::m_font_buttons);
      ps.SetTextColor(Colors::White);
      ps.SetBackgroundColor(Colors::Black);

      ps.TextOut(int2(5, 5), FixedStringBuilder<256>("Downloading image: ", mp_downloader->GetPercent(), "% complete"));
//      else
//         ps.TextOut(int2(5, 5), "Couldn't decode image");
   }

   if(m_show_arrows)
   {
      ps.SelectPen((HPEN)GetStockObject(BLACK_PEN));

      Rect rcClient=ClientRect();

      int centerY=rcClient.center().y;
      int sidePad=rcClient.size().x/20;
      int arrowWidth=rcClient.size().x/20;
      int arrowHeight=min(arrowWidth*3/2, rcClient.size().y*2/6); // Size above and below the center

      int2 points[]={ int2(sidePad, centerY), 
                      int2(sidePad+arrowWidth, centerY-arrowHeight),
                     int2(sidePad+arrowWidth, centerY+arrowHeight)};

      ps.SelectBrush((HBRUSH)GetStockObject(m_url_index>0 ? WHITE_BRUSH : GRAY_BRUSH));
      ps.Polygon(points);

      for(auto &p : points)
         p.x=rcClient.right-p.x; // Flip arrow horizontally

      ps.SelectBrush((HBRUSH)GetStockObject(m_url_index+1<m_urls.Count() ? WHITE_BRUSH : GRAY_BRUSH));
      ps.Polygon(points);
   }

   return msg.Success();
}

UniquePtr<Wnd_Image> CreateImageWindow(Wnd_Main &wndMain)
{
   return MakeUnique<Wnd_Image>(wndMain);
}
