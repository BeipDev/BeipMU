#include "Main.h"
#include "MCP.h"
#include "Connection.h"
#include "Wnd_Main.h"

//
// MCP-negotiate
//
// http://www.awns.com/mcp/packages/README.dns-com-awns-status
//
namespace MCP
{

struct Wnd_Status : TWindowImpl<Wnd_Status>, OwnerPtrData
{
   static ATOM Register()
   {
      WndClass wc(L"Wnd_Status");
      wc.LoadIcon(IDI_APP, g_hInst);
      wc.hCursor=LoadCursor(nullptr, IDC_ARROW);
      wc.hbrBackground=CreateSolidBrush(GetSysColor(COLOR_3DFACE));

      return wc.Register();
   }

   Wnd_Status(Wnd_Main &wndParent)
   {
      Create("Status", WS_OVERLAPPED|WS_THICKFRAME, Window::Position_Default, wndParent, WS_EX_NOACTIVATE);

      int2 frameSize=FrameSize();
      SetSize(frameSize+int2(100, Windows::Controls::Control::m_tmButtons.tmHeight));
      Show(SW_SHOW);

      mp_docking=&wndParent.CreateDocking(*this);
      mp_docking->Dock(Docking::Side::Bottom);
   }

   LRESULT WndProc(const Windows::Message &msg) override
   {
      return Dispatch<WindowImpl, Msg::Size, Msg::Paint>(msg);
   }

   LRESULT On(const Msg::Size &msg)
   {
      Invalidate(true);
      return msg.Success();
   }

   LRESULT On(const Msg::Paint &msg)
   {
      PaintStruct ps(this);
      ps.SetBackgroundMode(TRANSPARENT);
      ps.SelectFont(Windows::Controls::Control::m_font_buttons);
      ps.TextOut(int2(), m_text);
      return msg.Success();
   }

   void SetText(OwnedString &&text)
   {
      m_text=std::move(text);
      Invalidate(true);
   }

private:
   OwnedString m_text;
   Wnd_Docking *mp_docking;
};

struct Package_Status : Package
{
   Package_Status(const PackageInfo &info, Parser &parser)
   :  Package(parser, info)
   {
      m_pWndStatus=MakeUnique<Wnd_Status>(m_parser.connection().GetMainWindow());
   }

   ~Package_Status()
   {
   }

   void On(const Message &msg);

private:

   OwnerPtr<Wnd_Status> m_pWndStatus;
};

void Package_Status::On(const Message &msg)
{
   m_pWndStatus->SetText(msg["text"]);
}


struct PackageFactory_Status : PackageFactory
{
   UniquePtr<Package> Create(Parser &parser, int iVersion) override;
   const PackageInfo &info() const override { return m_info; }

private:
   PackageInfo m_info{"dns-com-awns-status", MakeVersion(1,0), MakeVersion(1,0)};
};


UniquePtr<Package> PackageFactory_Status::Create(Parser &parser, int iVersion)
{
   return MakeUnique<Package_Status>(info(), parser);
}

UniquePtr<PackageFactory> CreatePackageFactory_dns_com_awns_status()
{
   return MakeUnique<PackageFactory_Status>();
}

}
