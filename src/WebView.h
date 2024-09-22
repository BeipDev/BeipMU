struct ICoreWebView2Controller;
struct ICoreWebView2;
struct WebView_OM;

struct Event_WebViewEnvironmentCreated { };
struct Event_WebViewCreated { };

struct Wnd_WebView
 : DLNode<Wnd_WebView>, 
	TWindowImpl<Wnd_WebView>,
	Events::Sends_Deleted,
	Events::ReceiversOf<Wnd_WebView, Event_WebViewEnvironmentCreated>
{
	struct Header
	{
		OwnedString name, value;
	};

	static ATOM Register();
	Wnd_WebView(Wnd_Main &wnd_main, ConstString id={});

	ConstString GetURL() const { return m_url; }
	void SetURL(ConstString url, Array<Header> headers={});
	void SetSource(ConstString source);

	ConstString GetID() const { return m_id; }

	bool HostObjects() const { return m_host_objects; }

	Wnd_Docking &GetDocking() const { return *mp_docking; }
	Wnd_Main &GetWndMain() const { return m_wnd_main; }

	void On(const Event_WebViewEnvironmentCreated &);
	void On(const Event_WebViewCreated &);

private:

	void Navigate();

	LRESULT WndProc(const Message &msg) override;
	friend TWindowImpl;

	LRESULT On(const Msg::Create &msg);
	LRESULT On(const Msg::WindowPosChanged &msg);
	LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
	LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }

	Wnd_Main &m_wnd_main;
	OwnedString m_url, m_source;
	OwnedString m_id;
	bool m_host_objects{true};

	Collection<Header> m_headers;

	CntPtrTo<ICoreWebView2Controller> mp_webviewController;
	CntPtrTo<ICoreWebView2> mp_webview;
	Wnd_Docking *mp_docking;
	CntPtrTo<WebView_OM> mp_webview_om;
};
