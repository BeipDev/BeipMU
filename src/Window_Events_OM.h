namespace OM
{

struct Window_Events
:  Dispatch<OM::IWindow_Events>,
   Events::ReceiversOf<Window_Events, Windows::Event_MouseMove, Windows::Event_Close, Windows::Event_Key>
{
   Window_Events(WindowEvents *pEvents);

   STDMETHODIMP SetOnClose(IDispatch *pDisp, VARIANT var);
   STDMETHODIMP SetOnMouseMove(IDispatch *pDisp, VARIANT var);
   STDMETHODIMP SetOnKey(IDispatch *pDisp, VARIANT var);

   void On(const Windows::Event_MouseMove &event);
   void On(const Windows::Event_Close &event);
   void On(const Windows::Event_Key &event);

private:
   WindowEvents *m_pEvents;

   HookVariant m_hookMouseMove;
   HookVariant m_hookClose;
   HookVariant m_hookKey;
};

}
