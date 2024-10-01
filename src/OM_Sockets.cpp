//
// Sockets Object Model
//

#include "Main.h"

#include "OM_Sockets.h"

namespace OM
{

// ***
// *** Socket
// ***
STDMETHODIMP Socket::QueryInterface(REFIID riid, void **ppvObj)
{
   return TQueryInterface(riid, ppvObj);
}

Socket::Socket(SOCKET socket)
{
   // Handle the already existing socket case
   if(socket!=INVALID_SOCKET)
      m_socket.Connect(*this, socket);
}

STDMETHODIMP Socket::Connect(BSTR host, unsigned iPort)
{
   FixedStringBuilder<32> port; port << iPort;

   if(m_pHostLookup) m_pHostLookup.Cancel();
   new Sockets::GetHost(m_pHostLookup, *this, BSTRToLStr(host), port);
   return S_OK;
}

STDMETHODIMP Socket::Close()
{
   if(m_socket.IsConnected())
      m_socket.Close();
   return S_OK; // Report success even if the socket was already disconnected
}

STDMETHODIMP Socket::IsConnected(VARIANT_BOOL *retval)
{
   *retval=VariantBool(m_socket.IsConnected());
   return S_OK;
}

STDMETHODIMP Socket::Send(VARIANT var)
{
   if(var.vt==VT_BSTR)
   {
      BSTRToLStr string(var.bstrVal);
      m_socket.Send(string);
      return S_OK;
   }

   if(var.vt==(VT_ARRAY|VT_UI1) )
   {
      long lBound, uBound;
      HRESULT hr;

      hr=SafeArrayGetLBound(var.parray, 1, &lBound);
      if(hr!=S_OK)
         return E_FAIL;
      hr=SafeArrayGetUBound(var.parray, 1, &uBound);
      if(hr!=S_OK)
         return E_FAIL;

      const BYTE *pData;
      hr=SafeArrayAccessData(var.parray, (void **)&pData);
      m_socket.Send(Array(pData, uBound+1-lBound));
      SafeArrayUnaccessData(var.parray);
      return S_OK;
   }

   return E_FAIL;
}

STDMETHODIMP Socket::SetOnReceive(IDispatch *pDisp)
{
   m_hookReceive.Set(pDisp); return S_OK;
}

STDMETHODIMP Socket::SetOnConnect(IDispatch *pDisp)
{
   m_hookConnect.Set(pDisp); return S_OK;
}

STDMETHODIMP Socket::SetOnDisconnect(IDispatch *pDisp)
{
   m_hookDisconnect.Set(pDisp); return S_OK;
}

STDMETHODIMP Socket::SetFlag(int iFlag, VARIANT_BOOL fValue)
{
   return S_OK;
}

void Socket::OnConnect()
{
   if(fAdvised())
      MultipleInvoke(2);

   if(m_hookConnect)
      m_hookConnect(this);
}

void Socket::OnDisconnect(DWORD error)
{
   if(fAdvised())
      MultipleInvoke(3);

   if(m_hookDisconnect)
      m_hookDisconnect(this);
}

void Socket::OnReceive(Array<const BYTE> data)
{
   if(fAdvised())
      MultipleInvoke(1);

   if(m_hookReceive)
      m_hookReceive(ConstString(reinterpret_cast<const char *>(&*data.begin()), data.Count()), this);
}

void Socket::OnHostLookup(const Sockets::Address &address)
{
   m_socket.Connect(*this, address);
}

void Socket::OnHostLookupFailure(DWORD error)
{

}

// ***
// *** SocketServer
// ***
STDMETHODIMP SocketServer::QueryInterface(REFIID riid, void **ppvObj)
{
   return TQueryInterface(riid, ppvObj);
}

SocketServer::SocketServer(unsigned int iPort, IDispatch *pDisp, VARIANT &var)
: m_server(*this, iPort)
{
   if(pDisp)
      m_hookConnection.Set(pDisp, var);
}

void SocketServer::OnConnection(SOCKET socket, const Sockets::Address &address)
{
   CntPtrTo<ISocket> pSocket=new Socket(socket);

   if(fAdvised())
   {
      Variant var[]={ static_cast<IDispatch*>(pSocket) };
      DISPPARAMS dp={ var, 0, _countof(var), 0 };
      MultipleInvoke(1, dp);
   }

   if(m_hookConnection)
      m_hookConnection(m_hookConnection.var, static_cast<IDispatch*>(pSocket));
}

HRESULT SocketServer::Shutdown()
{
   m_server.Shutdown();
   m_hookConnection.Clear();

   return S_OK;
}

// ***
// *** ForwardDNS
// ***

ForwardDNS::ForwardDNS(ForwardDNS *pInsertAfter, BSTR bstrHost, IDispatch *pDisp, VARIANT &var)
: DLNode<ForwardDNS>(pInsertAfter)
{
   Assert(pDisp);
   m_hook.Set(pDisp, var);

   new Sockets::GetHost(m_pGetHost, *this, BSTRToLStr(bstrHost), ConstString());
   AddRef();  
}

void ForwardDNS::OnHostLookup(const Sockets::Address &address)
{
   char pcHost[256];
   if(inet_ntop(address.m_address.sa_family, UnconstPtr(address.m_address.sa_data), pcHost, _countof(pcHost))==nullptr)
   {
      OnHostLookupFailure(WSAGetLastError());
      return;
   }
   m_hook(m_hook.var, ConstString(pcHost));
   Release(); // We're done
}

void ForwardDNS::OnHostLookupFailure(DWORD error)
{
   m_hook(m_hook.var, ConstString());
   Release(); // We're done
}

// ***
// *** ReverseDNS
// ***

ReverseDNS::ReverseDNS(ReverseDNS *pInsertAfter, BSTR bstrIP, IDispatch *pDisp, VARIANT &var)
: DLNode<ReverseDNS>(pInsertAfter)
{
   Assert(pDisp);
   m_hook.Set(pDisp, var);

   new Sockets::GetHost(m_pGetHost, *this, BSTRToLStr(bstrIP), ConstString());
   AddRef(); // As we're in the list
}

void ReverseDNS::OnHostLookup(const Sockets::Address &address)
{
   new Sockets::GetName(m_pGetName, *this, address);
}

void ReverseDNS::OnHostLookupFailure(DWORD error)
{
   OnNameLookupFailure(error);
}

void ReverseDNS::OnNameLookup(ConstString name)
{
   m_hook(m_hook.var, name);
   Release(); // We're done
}

void ReverseDNS::OnNameLookupFailure(DWORD error)
{
   m_hook(m_hook.var, ConstString());
   Release(); // We're done
}

}