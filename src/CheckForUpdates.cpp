#include "Main.h"
#include "JSON.h"
#include "LibWin32\Http.h"

struct GetLatestReleaseRequest : HttpRequest
{
   struct JSON_Release : JSON::Element
   {
      void OnString(ConstString name, ConstString value) override
      {
         if(name=="tag_name")
            m_tag_name=value;
         else if(name=="html_url")
            m_html_url=value;
         else if(name=="name")
            m_name=value;
         else if(name=="body")
            m_body=value;
         else if(name=="published_at")
            m_published_at=value;
      }

      OwnedString m_tag_name;
      OwnedString m_html_url;
      OwnedString m_name;
      OwnedString m_body;
      OwnedString m_published_at;
   } m_release;

   GetLatestReleaseRequest(HttpConnection &http_connection, ConstString owner, ConstString repo,
      std::function<void()> on_complete, std::function<void(ConstString)> on_error)
      : HttpRequest{http_connection},
      m_on_complete{std::move(on_complete)},
      m_on_error{std::move(on_error)}
   {
      HybridStringBuilder<> path("/repos/", owner, "/", repo, "/releases/latest");
      if(!Open("GET", path))
         return;

      Send("Accept: application/vnd.github+json\r\n"
           "X-GitHub-Api-Version: 2026-03-10\r\n");
   }

   void OnComplete(Array<const uint8> data) override
   {
      try
      {
         JSON::ParseObject(m_release, ToString(data));
         m_on_complete();
      }
      catch(const std::exception &)
      {
         m_on_error("Failed to parse GitHub latest release response");
      }
   }

   void OnError(ConstString error) override
   {
      m_on_error(error);
   }

   std::function<void()> m_on_complete;
   std::function<void(ConstString)> m_on_error;
};

ConstString g_download_url{"https://github.com/BeipDev/BeipMU/releases/latest"};

struct CheckForUpdates
{
   CheckForUpdates(Window parent, bool manual_check)
      : m_parent{parent},
        m_manual_check{manual_check}
   {
      s_owner=UniquePtr(this);
      if(!m_manual_check)
         m_timer.Set(5*24*60*60, true); // 5 days (5*24 hours*60 minutes*60 seconds)

      StartRequest();
   }

   void StartRequest()
   {
      m_request=MakeUnique<GetLatestReleaseRequest>(m_http_connection, "BeipDev", "BeipMU", [this]() { OnComplete(); }, [this](ConstString error) { OnError(error); });
   }

   static HRESULT CALLBACK OnTaskDialogCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR ref_data)
   {
      auto *self=reinterpret_cast<CheckForUpdates *>(ref_data);
      if(!self)
         return S_OK;

      if(msg==TDN_HYPERLINK_CLICKED)
         OpenURLAsync(g_download_url);

      return S_OK;
   }

   void OnComplete()
   {
      int build_number;
      if(!m_request->m_release.m_tag_name.WithoutFirst(1).To(build_number))
      {
         OnError("Error parsing release number");
         return;
      }

#if BETA_BUILD!=0
      // If we're on a beta, then if the latest release is equal to our build number we should update to the real release
      if(build_number<g_build_number)
#else
      if(build_number<=g_build_number)
#endif
      {
         if(m_manual_check)
         {
            MessageBox(m_parent, L"You are running the latest version of BeipMU", L"Up to Date", MB_ICONINFORMATION);
            s_owner=nullptr;
         }
         return;
      }

      HybridStringBuilder<> content;
      content("Name: ", m_request->m_release.m_name, "\n",
         "Published: ", m_request->m_release.m_published_at, "\n\n",
         "Changes:\n", m_request->m_release.m_body, "\n\n",
         "Download: <a href=\"", g_download_url, "\">", g_download_url, "</a>");

      UTF16 content16(content);

      TASKDIALOGCONFIG tdc{};
      tdc.cbSize=sizeof(tdc);
      tdc.hwndParent=m_parent;
      tdc.hInstance=g_hInst;
      tdc.pszMainIcon=MAKEINTRESOURCE(IDI_APP);
      tdc.dwFlags=TDF_POSITION_RELATIVE_TO_WINDOW | TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION | (g_ppropGlobal->fCheckForUpdates() ? TDF_VERIFICATION_FLAG_CHECKED : 0);
      tdc.dwCommonButtons=TDCBF_OK_BUTTON;
      tdc.pszWindowTitle=L"Update Available";
      tdc.pszMainInstruction=L"A new version of BeipMU is available";
      tdc.pszContent=content16.stringz();
      tdc.pszVerificationText=L"Check for updates at startup";
      tdc.pfCallback=OnTaskDialogCallback;
      tdc.lpCallbackData=reinterpret_cast<LONG_PTR>(this);

      BOOL verification_checked{};
      TaskDialogIndirect(&tdc, nullptr, nullptr, &verification_checked);
      g_ppropGlobal->fCheckForUpdates(verification_checked!=0);
      s_owner=nullptr;
   }

   void OnError(ConstString error)
   {
      if(m_manual_check)
      {
         MessageBox(m_parent, error, "Update Check Failed", MB_ICONERROR);
         s_owner=nullptr;
      }
   }

   inline static UniquePtr<CheckForUpdates> s_owner;
   Window m_parent;
   bool m_manual_check;
   Time::Timer m_timer{[this]() { StartRequest(); }};
   HttpConnection m_http_connection{GetHttpSession(), "api.github.com:443"};
   UniquePtr<GetLatestReleaseRequest> m_request;
};

void CheckForUpdatesAsync(Window parent, bool manual_check)
{
   new CheckForUpdates(parent, manual_check);
}
