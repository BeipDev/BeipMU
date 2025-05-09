//
// Sockets Object Model
//
#include "OM_Help.h"

namespace OM
{

struct Socket
 : Dispatch<ISocket>,
   Sockets::Socket::INotify,
   Sockets::GetHost::INotify
{
   Socket(SOCKET socket=INVALID_SOCKET);

   USE_INHERITED_UNKNOWN(ISocket)

   // IUnknown
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);

   // ISocket
   STDMETHODIMP Connect(BSTR host, unsigned int iPort) override;
   STDMETHODIMP Close() override;

   STDMETHODIMP IsConnected(VARIANT_BOOL *retval) override;
   STDMETHODIMP Send(VARIANT var) override;

   STDMETHODIMP SetOnReceive(IDispatch *pDisp) override;
   STDMETHODIMP SetOnConnect(IDispatch *pDisp) override;
   STDMETHODIMP SetOnDisconnect(IDispatch *pDisp) override;

   STDMETHODIMP SetFlag(int iFlag, VARIANT_BOOL fValue) override;

   STDMETHODIMP get_UserData(VARIANT *pVar) override { VariantCopy(pVar, &m_userdata); return S_OK; }
   void STDMETHODCALLTYPE put_UserData(VARIANT var) override { m_userdata=var; }

private:

   // Socket::INotify
   void OnConnect() override;
   void OnDisconnect(DWORD error) override;
   void OnReceive(Array<const BYTE> data) override;

   // Sockets::GetHost::INotify
   void OnHostLookupFailure(DWORD error) override;
   void OnHostLookup(const Sockets::Address &address) override;

   VariantNode m_userdata;

   Sockets::Socket m_socket;
   AsyncOwner<Sockets::GetHost> m_pHostLookup;

   Hook m_hookConnect, m_hookDisconnect, m_hookReceive;
};

struct SocketServer
 : Dispatch<ISocketServer>,
   Sockets::Server::INotify
{
   SocketServer(unsigned int iPort, IDispatch *pDisp, VARIANT &var);

   USE_INHERITED_UNKNOWN(ISocketServer)

   // IUnknown
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;

   // ISocketServer
   STDMETHODIMP Shutdown() override;

private:
   void OnConnection(SOCKET socket, const Sockets::Address &address) override;

   Sockets::Server m_server;
   HookVariant m_hookConnection;
};

struct ReverseDNS : Dispatch<IReverseDNS>, Sockets::GetHost::INotify, Sockets::GetName::INotify, DLNode<ReverseDNS>
{
   ReverseDNS(ReverseDNS *pInsertAfter, BSTR bstrIP, IDispatch *pDisp, VARIANT &var);

private:
   // Sockets::GetHost::INotify
   void OnHostLookupFailure(DWORD error) override;
   void OnHostLookup(const Sockets::Address &address) override;

   // Sockets::GetName::INotify
   void OnNameLookupFailure(DWORD error) override;
   void OnNameLookup(ConstString name) override;

   HookVariant m_hook;
   AsyncOwner<Sockets::GetHost> m_pGetHost;
   AsyncOwner<Sockets::GetName> m_pGetName;
};

struct ForwardDNS : Dispatch<IForwardDNS>, Sockets::GetHost::INotify, DLNode<ForwardDNS>
{
   ForwardDNS(ForwardDNS *pInsertAfter, BSTR bstrName, IDispatch *pDisp, VARIANT &var);

private:
   // Sockets::GetHost::INotify
   void OnHostLookupFailure(DWORD error) override;
   void OnHostLookup(const Sockets::Address &address) override;

   HookVariant m_hook;
   AsyncOwner<Sockets::GetHost> m_pGetHost;
};

};
