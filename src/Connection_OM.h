#include "OM_Help.h"

namespace OM
{

struct Connection
:  Dispatch<IConnection>,
   Events::ReceiversOf<Connection, ::Connection::Event_Receive, ::Connection::Event_Display,
      ::Connection::Event_Connect, ::Connection::Event_Disconnect, ::Connection::Event_Send,
      ::Connection::Event_GMCP, ::Connection::Event_Log>
{
   Connection(::Connection *pConnection);

   USE_INHERITED_UNKNOWN(IConnection)

   // IUnknown
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);

   // IConnection methods
   STDMETHODIMP Send(BSTR bstr);
   STDMETHODIMP Transmit(BSTR bstr);

   STDMETHODIMP Receive(BSTR bstr);
   STDMETHODIMP Display(BSTR bstr);

   STDMETHODIMP SetOnSend(IDispatch *pDisp, VARIANT var);
   STDMETHODIMP SetOnReceive(IDispatch *pDisp, VARIANT var);
   STDMETHODIMP SetOnDisplay(IDispatch *pDisp, VARIANT var);
   STDMETHODIMP SetOnConnect(IDispatch *pDisp, VARIANT var);
   STDMETHODIMP SetOnDisconnect(IDispatch *pDisp, VARIANT var);

   STDMETHODIMP SetOnGMCP(IDispatch *pDisp, VARIANT var);

   STDMETHODIMP IsConnected(VARIANT_BOOL *retval);
   STDMETHODIMP Reconnect(VARIANT_BOOL *retval);
   STDMETHODIMP IsLogging(VARIANT_BOOL *retval);

   STDMETHODIMP get_World(IWorld **);
   STDMETHODIMP get_Character(ICharacter **);
   STDMETHODIMP get_Puppet(IPuppet **);
   STDMETHODIMP get_Window_Main(IWindow_Main **);
   STDMETHODIMP get_Log(ILog **);

   // Events
   void On(::Connection::Event_Receive &event);
   void On(::Connection::Event_Display &event);
   void On(::Connection::Event_Connect &event);
   void On(::Connection::Event_Disconnect &event);
   void On(::Connection::Event_Send &event);
   void On(::Connection::Event_Log &event);
   void On(::Connection::Event_GMCP &event);

private:
   NotifiedPtrTo<::Connection> mp_connection;

   HookVariant m_hookSend;
   HookVariant m_hookReceive;
   HookVariant m_hookDisplay;
   HookVariant m_hookConnect;
   HookVariant m_hookDisconnect;
   HookVariant m_hookGMCP;
};

};
