struct Image;
namespace Image_Events
{
   struct Changed { bool erase_background{true}; };
}

namespace GMCP
{
   struct Avatar_Info;
   struct Avatar_Changed;
   struct Avatar_Destroyed;
}

struct ImageBanner : Text::Banner
{
   ImageBanner();
   ~ImageBanner();

   void AddURL(Text::ImageURL &&url);

   int2 Size() const override { return {0, m_height}; }
   int Padding() const { return 2*g_dpiScale; }

   Rect GetImagesRect(const Rect &rect);
   unsigned GetIndex(int x, const Rect &rect);

   void OnLButton(int2 pos, const Rect &rect) override;
   void GetToolTip(int2 pos, const Rect &rect, Handle<HBITMAP> &bitmap, StringBuilder &string) override;

   void AddContextMenu(ContextMenu &menu, int2 pos, const Rect &rect) override;

   void Draw(DC dc, const Rect &rect) override;

   int m_height{abs(g_ppropGlobal->iInlineImageHeight())*g_dpiScale};

   struct Entry : Events::ReceiversOf<Entry, Image_Events::Changed>
   {
      Entry(ImageBanner &banner, CntPtrTo<Image> &&p_image);

      void On(const Image_Events::Changed &event) { m_banner.Invalidate(event.erase_background); }

      ImageBanner &m_banner;
      CntPtrTo<Image> mp_image;
   };

   Collection<UniquePtr<Entry>> mp_images;
};

struct ImageAvatar : Text::InlineImage, Events::ReceiversOf<ImageAvatar, Image_Events::Changed, GMCP::Avatar_Changed, GMCP::Avatar_Destroyed>
{
   ImageAvatar(Text::ImageURL &&url);
   ImageAvatar(GMCP::Avatar_Info &info);
   ~ImageAvatar();

   int2 Size() const override { return m_size; }

   void OnLButton(int2 pos, const Rect &rect) override;
   void GetToolTip(int2 pos, const Rect &rect, Handle<HBITMAP> &bitmap, StringBuilder &string) override;

   void AddContextMenu(ContextMenu &menu, int2 pos, const Rect &rect) override;

   void Draw(DC dc, const Rect &rect) override;

   void On(const Image_Events::Changed &event) { Invalidate(true); }
   void On(const GMCP::Avatar_Changed &event);
   void On(const GMCP::Avatar_Destroyed &event);

   int2 m_size{abs(g_ppropGlobal->iAvatarWidth())*g_dpiScale, abs(g_ppropGlobal->iAvatarHeight())*g_dpiScale};

   OwnedString m_description;
   CntPtrTo<Image> mp_image;
   GMCP::Avatar_Info *mp_avatar_info{};
};
