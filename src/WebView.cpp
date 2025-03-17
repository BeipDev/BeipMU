#include "Main.h"
#include "Wnd_Main.h"
#include "Wnd_Main_OM.h"
#include "Wnd_Text_OM.h"
#include "Automation.h"
#include <wrl.h>
#include <WebView2.h>
#include "WebView.h"
#include "HTML.h"

// To get to work on Wine:
// https://web.archive.org/web/20210626091814/https://msedge.sf.dl.delivery.mp.microsoft.com/filestreamingservice/files/ee4e97c1-89a3-456f-b9f3-f29651316b7e/MicrosoftEdgeWebView2RuntimeInstallerX64.exe

CntPtrTo<ICoreWebView2Environment> gp_environment;

struct WebView2EnvironmentCreator : General::Unknown<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>, Events::SendersOf<Event_WebViewEnvironmentCreated>
{
	WebView2EnvironmentCreator()
	{
		sp_instance=this;
		CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, this);
	}

	// IUnknown methods
	STDMETHODIMP QueryInterface(const IID &id, void **ppvObj) override
	{
		return TQueryInterface(id, ppvObj);
	}

	STDMETHODIMP Invoke(HRESULT error_code, ICoreWebView2Environment *p_environment) override
	{
		if(FAILED(error_code))
			return S_OK;

		gp_environment=p_environment;
		Send(Event_WebViewEnvironmentCreated());
		return S_OK;
	}

	static inline CntPtrTo<WebView2EnvironmentCreator> sp_instance;
};

struct WebView_OM
 : OM::Dispatch<OM::IWebView>,
	Events::ReceiversOf<WebView_OM, ::Connection::Event_Connect, ::Connection::Event_Disconnect, ::Connection::Event_Receive, ::Connection::Event_Display,
	::Connection::Event_Send, ::Connection::Event_GMCP>
{
	WebView_OM(Wnd_WebView &wnd_webview) : mp_wnd_webview{&wnd_webview} { }

	STDMETHODIMP CloseWindow() override { mp_wnd_webview->Close(); return S_OK; }

	STDMETHODIMP Send(BSTR bstr, VARIANT_BOOL process_aliases) override
	{
		HybridStringBuilder string(bstr);

		if(OM::VariantBool(process_aliases))
			m_connection.ProcessAliases(string);

		m_connection.Send(string, true);
		return S_OK;
	}

	STDMETHODIMP IsConnected(VARIANT_BOOL *retval) override { *retval=OM::ToVariantBool(m_connection.IsConnected()); return S_OK; }
	STDMETHODIMP SetOnConnect(IDispatch *p_callback) override { return OM::ManageHook<::Connection::Event_Connect>(this, m_hook_connect, m_connection, p_callback, m_ignore); }
	STDMETHODIMP SetOnDisconnect(IDispatch *p_callback) override { return OM::ManageHook<::Connection::Event_Disconnect>(this, m_hook_disconnect, m_connection, p_callback, m_ignore); }

	STDMETHODIMP SetOnSend(IDispatch *pDisp) override { return OM::ManageHook<::Connection::Event_Send>(this, m_hook_send, m_connection, pDisp, m_ignore); }

	STDMETHODIMP Receive(BSTR bstr) override { m_connection.Receive(BSTRToLStr(bstr)); return S_OK; }
	STDMETHODIMP SetOnReceive(IDispatch *pDisp) override { return OM::ManageHook<::Connection::Event_Receive>(this, m_hook_receive, m_connection, pDisp, m_ignore); }

	STDMETHODIMP Display(BSTR bstr) override { m_connection.Display(BSTRToLStr(bstr)); return S_OK; }

	STDMETHODIMP SetOnDisplay(int id, IDispatch *p_disp, BSTR regex, VARIANT_BOOL gag) override
	{
		Displayer *p_displayer{};
		for(auto &p_hook : m_displays)
		{
			if(p_hook->m_id==id)
			{
				p_displayer=p_hook;
				break;
			}
		}

		if(!p_displayer)
		{
			p_displayer=m_displays.Push(MakeUnique<Displayer>());
			p_displayer->m_id=id;
			if(m_displays.Count()+m_captures.Count()==1)
				AttachTo<::Connection::Event_Display>(m_connection);
		}

		p_displayer->m_hook.Set(p_disp);
		p_displayer->mp_regex=MakeUnique<RegEx::Expression>(BSTRToLStr(regex), PCRE2_UTF);
		p_displayer->m_gag=OM::VariantBool(gag);
		return S_OK;
	}

	STDMETHODIMP ClearOnDisplay(int id) override
	{
		for(unsigned index=0;index<m_displays.Count();index++)
		{
			auto &p_hook=m_displays[index];
			if(p_hook->m_id==id)
			{
				m_displays.UnsortedDelete(index);
				if(m_displays.Count()+m_captures.Count()==0)
					Detach<::Connection::Event_Display>();
				break;
			}
		}

		return S_OK;
	}

	STDMETHODIMP SetOnDisplayCapture(int id, IDispatch *p_capture_line, IDispatch *p_capture_changed, BSTR regex_begin, BSTR regex_end) override
	{
		Capture *p_capture{};
		for(auto &p_hook : m_captures)
		{
			if(p_hook->m_id==id)
			{
				p_capture=p_hook;
				break;
			}
		}

		if(!p_capture)
		{
			p_capture=m_captures.Push(MakeUnique<Capture>());
			p_capture->m_id=id;
			if(m_displays.Count()+m_captures.Count()==1)
				AttachTo<::Connection::Event_Display>(m_connection);
		}

		p_capture->m_hook_capture_line.Set(p_capture_line);
		p_capture->m_hook_capture_changed.Set(p_capture_changed);

		p_capture->mp_regex_begin=MakeUnique<RegEx::Expression>(BSTRToLStr(regex_begin), PCRE2_UTF);
		p_capture->mp_regex_end=MakeUnique<RegEx::Expression>(BSTRToLStr(regex_end), PCRE2_UTF);
		return S_OK;
	}

	STDMETHODIMP ClearOnDisplayCapture(int id, VARIANT_BOOL *retval) override
	{
		*retval=OM::ToVariantBool(false);

		for(unsigned index=0;index<m_captures.Count();index++)
		{
			auto &p_hook=m_captures[index];
			if(p_hook->m_id==id)
			{
				if(p_hook==mp_capturing)
					break;

				m_captures.UnsortedDelete(index);
				if(m_displays.Count()+m_captures.Count()==0)
					Detach<::Connection::Event_Display>();
				*retval=OM::ToVariantBool(true);
				break;
			}
		}

		return S_OK;
	}

	STDMETHODIMP SendGMCP(BSTR package, BSTR json) override
	{
		if(!m_connection.IsConnected())
			return E_FAIL;
		m_connection.SendGMCP(UTF8(package), UTF8(json));
		return S_OK;
	}

	STDMETHODIMP SetOnGMCP(BSTR package_prefix16, IDispatch *p_callback) override
	{
		OwnedString package_prefix{package_prefix16};

		for(auto &p_handler : m_gmcp_handlers)
		{
			if(p_handler->m_prefix==package_prefix)
				return E_FAIL;
		}

		auto &p_handler=m_gmcp_handlers.Push(MakeUnique<GMCP_Handler>());
		p_handler->m_prefix=std::move(package_prefix);
		p_handler->m_hook.Set(p_callback);

		if(m_gmcp_handlers.Count()==1)
			AttachTo<::Connection::Event_GMCP>(m_connection);
		return S_OK;
	}

	STDMETHODIMP ClearOnGMCP(BSTR package_prefix16) override
	{
		UTF8 package_prefix{package_prefix16};
		for(unsigned index=0;index<m_gmcp_handlers.Count();index++)
		{
			auto &p_handler=m_gmcp_handlers[index];
			if(p_handler->m_prefix==package_prefix)
			{
				m_gmcp_handlers.UnsortedDelete(index);
				if(m_gmcp_handlers.Count()==0)
					Detach<::Connection::Event_GMCP>();
				return S_OK;
			}
		}

		return E_FAIL;
	}

	STDMETHODIMP ProcessAliases(BSTR bstr, BSTR *out) override
	{
		HybridStringBuilder string(bstr);
		m_connection.ProcessAliases(string);
		*out=LStrToBSTR(string);
		return S_OK;
	}

	STDMETHODIMP AddToInputHistory(BSTR bstr) override
	{
		mp_wnd_webview->GetWndMain().History_AddToHistory(UTF8(bstr), {});
		return S_OK;
	}

	STDMETHODIMP GetPropertyString(BSTR bstr, BSTR *out) override
	{
		auto property=BSTRToLStr(bstr);
		if(property=="WorldName")
		{
			*out=LStrToBSTR(mp_wnd_webview->GetWndMain().GetConnection().GetServer()->pclName());
			return S_OK;
		}
		else if(property=="CharacterName")
		{
			*out=LStrToBSTR(mp_wnd_webview->GetWndMain().GetConnection().GetCharacter()->pclName());
			return S_OK;
		}
		else if(property=="PuppetName")
		{
			if(auto *p_puppet=mp_wnd_webview->GetWndMain().GetConnection().GetPuppet())
				*out=LStrToBSTR(p_puppet->pclName());
			else
				*out=nullptr;
			return S_OK;
		}
		return S_FALSE;
	}

	void On(Connection::Event_Receive &event)
	{
		if(m_hook_receive)
			m_hook_receive(event.GetString());
	}

	void On(Connection::Event_Display &event)
	{
		if(HandleCaptures(event))
			return;

		if(m_displays)
		{
			FixedArray<uint2, 15> ranges;

			for(auto &p_display : m_displays)
				if(p_display->mp_regex->Find(event.GetTextLine().GetText(), 0, ranges))
				{
					CntPtrTo<IDispatch> p_line=new OM::TextWindowLine(event.GetTextLine());
					p_display->m_hook(&*p_line, p_display->m_id);
					event.Stop(p_display->m_gag);
					return;
				}
		}
	}

	bool HandleCaptures(Connection::Event_Display &event)
	{
		if(!m_captures)
			return false;

		FixedArray<uint2, 15> ranges;

		if(!mp_capturing)
		{
			for(auto &p_capture : m_captures)
				if(p_capture->mp_regex_begin->Find(event.GetTextLine().GetText(), 0, ranges))
				{
					mp_capturing=p_capture;
					break;
				}

			if(!mp_capturing)
				return false;

			CntPtrTo<IDispatch> p_line=new OM::TextWindowLine(event.GetTextLine());
			mp_capturing->m_hook_capture_changed(true, &*p_line, mp_capturing->m_id);
			event.Stop(mp_capturing->m_gag);
			return true;
		}

		CntPtrTo<IDispatch> p_line=new OM::TextWindowLine(event.GetTextLine());

		if(mp_capturing->mp_regex_end->Find(event.GetTextLine().GetText(), 0, ranges))
		{
			mp_capturing->m_hook_capture_changed(false, &*p_line, mp_capturing->m_id);
			event.Stop(mp_capturing->m_gag);
			mp_capturing=nullptr;
			return true;
		}

		mp_capturing->m_hook_capture_line(&*p_line, mp_capturing->m_id);
		event.Stop(mp_capturing->m_gag);
		return true;
	}

	void On(Connection::Event_Connect &event)
	{
		if(m_hook_connect)
			m_hook_connect();
	}

	void On(Connection::Event_Disconnect &event)
	{
		if(m_hook_disconnect)
			m_hook_disconnect();
	}

	void On(Connection::Event_Send &event)
	{
		if(m_hook_send)
			m_hook_send(event.GetString());
	}

	void On(Connection::Event_GMCP &event)
	{
		auto string=event.GetString();

		ConstString package;
		if(!string.Split(' ', package, string))
			return;

		ConstString prefix=package.First(package.FindFirstOf('.', package.Count()));

		for(auto &p_handler : m_gmcp_handlers)
		{
			if(auto remainder=package.RightOf(p_handler->m_prefix);remainder && remainder.StartsWith('.'))
			{
				p_handler->m_hook(string, package);
				return;
			}
		}
	}

private:
	NotifiedPtrTo<Wnd_WebView> mp_wnd_webview;
	Connection &m_connection{mp_wnd_webview->GetWndMain().GetConnection()};

	OM::Variant m_ignore;
	OM::HookVariant m_hook_connect;
	OM::HookVariant m_hook_disconnect;
	OM::HookVariant m_hook_send;
	OM::HookVariant m_hook_receive;

	struct Displayer
	{
		int m_id{};
		OM::Hook m_hook;
		bool m_gag{true};

		UniquePtr<RegEx::Expression> mp_regex;
	};

	Collection<UniquePtr<Displayer>> m_displays;

	struct Capture
	{
		int m_id{};
		OM::Hook m_hook_capture_line;
		OM::Hook m_hook_capture_changed;
		bool m_gag{true};

		UniquePtr<RegEx::Expression> mp_regex_begin, mp_regex_end;
	};

	struct GMCP_Handler
	{
		OwnedString m_prefix;
		OM::Hook m_hook;
	};

	Collection<UniquePtr<GMCP_Handler>> m_gmcp_handlers;
	Collection<UniquePtr<Capture>> m_captures;
	Capture *mp_capturing{}; // Set: We saw the mp_regex_begin, and we're capturing every line, waiting for mp_regex_end
};

LRESULT Wnd_WebView::WndProc(const Message &msg)
{
	return Dispatch<WindowImpl, Msg::Create, Msg::WindowPosChanged, Msg::_GetTypeID, Msg::_GetThis>(msg);
}

ATOM Wnd_WebView::Register()
{
	WndClass wc(L"WebView");
	wc.LoadIcon(IDI_APP, g_hInst);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	return wc.Register();
}

Wnd_WebView::Wnd_WebView(Wnd_Main &wnd_main, ConstString id)
	: DLNode<Wnd_WebView>{wnd_main.GetWebViewRoot()}, m_wnd_main{wnd_main}, m_id{id}
{
	Create("WebView", WS_OVERLAPPEDWINDOW, Window::Position(int2(CW_USEDEFAULT, CW_USEDEFAULT), int2(800, 800)), wnd_main, WS_EX_APPWINDOW);
	mp_docking=&m_wnd_main.CreateDocking(*this);
	Show(SW_SHOWNOACTIVATE);

	AttachTo<GlobalInputSettingsModified>(g_text_events);
	AttachTo<GlobalTextSettingsModified>(g_text_events);
}

LRESULT Wnd_WebView::On(const Msg::Create &msg)
{
	if(!WebView2EnvironmentCreator::sp_instance)
		MakeCounting<WebView2EnvironmentCreator>();

	if(!gp_environment)
		AttachTo<Event_WebViewEnvironmentCreated>(*WebView2EnvironmentCreator::sp_instance);
	else
		On(Event_WebViewEnvironmentCreated());

	return msg.Success();
}

void Wnd_WebView::On(const Event_WebViewEnvironmentCreated &)
{
	// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
	gp_environment->CreateCoreWebView2Controller(hWnd(), Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
		[this](HRESULT result, ICoreWebView2Controller *p_controller) -> HRESULT {
			if(FAILED(result) || !p_controller)
				return S_OK; // Window closed?

			mp_webview_controller = p_controller;
			mp_webview_controller->get_CoreWebView2(mp_webview.Address());
 
			// Add a few settings for the webview
			// The demo step is redundant since the values are the default settings
			CntPtrTo<ICoreWebView2Settings> settings;
			mp_webview->get_Settings(settings.Address());
			settings->put_IsScriptEnabled(TRUE);
			settings->put_AreDefaultScriptDialogsEnabled(TRUE);
			settings->put_AreHostObjectsAllowed(TRUE);

#if 0 // Set dark mode in the webview
			CntPtrTo<ICoreWebView2_13> p_webview13; mp_webview->QueryInterface(p_webview13.Address());
			CntPtrTo<ICoreWebView2Profile> p_profile;
			p_webview13->get_Profile(p_profile.Address());
			p_profile->put_PreferredColorScheme(COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK);
#endif

			// Resize WebView to fit the bounds of the parent window
			mp_webview_controller->put_Bounds(ToRECT(ClientRect()));
			mp_webview_controller->put_IsVisible(TRUE);

			Automation::GetApp(); // Must do this so that OM_Help initializes properly
#if 0
			OM::Variant v1{Automation::GetApp()};
			mp_webview->AddHostObjectToScript(L"app", &v1);
			OM::Variant v2{m_wnd_main.GetDispatch()};
			mp_webview->AddHostObjectToScript(L"window", &v2);
#endif

			EventRegistrationToken token;
			mp_webview->add_DocumentTitleChanged(Microsoft::WRL::Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
				[this](ICoreWebView2 *sender, IUnknown *args) -> HRESULT {
					CoTaskHolder<wchar_t> title;
					sender->get_DocumentTitle(title.Address());
					SetText(UTF8(Strings::WzToString(title)));
					return S_OK;
				}).Get(), &token);

			mp_webview->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT);

			// Add WebResourceRequested event handler.
			mp_webview->add_WebResourceRequested(Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
				[this](ICoreWebView2 *sender, ICoreWebView2WebResourceRequestedEventArgs *args)->HRESULT {

					// Get the request object.
					CntPtrTo<ICoreWebView2WebResourceRequest> p_request;
					args->get_Request(p_request.Address());

					CntPtrTo<ICoreWebView2HttpRequestHeaders> p_headers;
					p_request->get_Headers(p_headers.Address());

					// Add the headers
					for(auto &header : m_headers)
						p_headers->SetHeader(UTF16(header.name).stringz(), UTF16(header.value).stringz());

					return S_OK;
				}).Get(), &token);

			// register an ICoreWebView2NavigationStartingEventHandler to add script interface to each page
			mp_webview->add_NavigationStarting(Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
				[this](ICoreWebView2 *webview, ICoreWebView2NavigationStartingEventArgs *args) -> HRESULT {
					// Replace the old OM with a fresh one
					mp_webview_om=MakeCounting<WebView_OM>(*this);

					OM::Variant v_client{mp_webview_om};
					mp_webview->AddHostObjectToScript(L"client", &v_client);
					return S_OK;
				}).Get(), &token);

			mp_webview->add_NavigationCompleted(Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
				[this](ICoreWebView2 *webview, ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT {
					SetStyles();
					return S_OK;
				}).Get(), &token);

			if(m_url || m_source)
				Navigate();

			return S_OK;
					}).Get());
}

void Wnd_WebView::On(const GlobalTextSettingsModified &event)
{
	SetStyles();
}

void Wnd_WebView::On(const GlobalInputSettingsModified &)
{
	SetStyles();
}

void Wnd_WebView::SetStyles()
{
	if(!mp_webview)
		return;

	auto &input_props = m_wnd_main.GetInputWindow().GetProps();
	auto &output_props = m_wnd_main.GetOutputProps();
	HybridStringBuilder<1024> script(
		"{ const styleId = 'clientInputOutputStyleVars';"
		"var style = document.getElementById(styleId);"
		"if (style === null) {"
		" style = document.createElement('style');"
		" style.id = styleId;"
		" document.head.appendChild(style);"
		" style = document.getElementById(styleId);"
		"}"
		"style.innerHTML = `"
		":root {"
		" \n --client-output-background:", HTML::HTMLColor(output_props.clrBack()),
		";\n --client-output-foreground:", HTML::HTMLColor(output_props.clrFore()),
		";\n --client-output-font:'", output_props.propFont().pclName(), "'"
		";\n --client-output-font-size:", output_props.propFont().Size(), "px"
		";\n --client-input-background:", HTML::HTMLColor(input_props.clrBack()),
		";\n --client-input-foreground:", HTML::HTMLColor(input_props.clrFore()),
		";\n --client-input-font:'", input_props.propFont().pclName(), "'"
		";\n --client-input-font-size:", input_props.propFont().Size(), "px"
		";\n}"
		"`; }"
	);
	mp_webview->ExecuteScript(UTF16(script).stringz(), nullptr);
}

void Wnd_WebView::SetURL(ConstString url, Array<Header> headers)
{
	m_headers.Empty();
	m_headers=headers;
	m_url=url;
	m_source.Clear();

	if(mp_webview)
		Navigate();
}

void Wnd_WebView::SetSource(ConstString source)
{
	SetURL({});
	m_source=source;

	if(mp_webview)
		Navigate();
}


void Wnd_WebView::Navigate()
{
	if(m_source)
		mp_webview->NavigateToString(UTF16(m_source).stringz());
	if(m_url)
		mp_webview->Navigate(UTF16(m_url).stringz());
}

LRESULT Wnd_WebView::On(const Msg::WindowPosChanged &msg)
{
	if(mp_webview_controller)
	{
		if(msg.WasResized())
			mp_webview_controller->put_Bounds(ToRECT(ClientRect()));
		if(msg.WasMoved())
			mp_webview_controller->NotifyParentWindowPositionChanged();
	}

	return msg.Success();
}
