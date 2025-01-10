#include "Main.h"
#include "Connection.h"

#include <activscp.h>

#include "Connection_OM.h"
#include "App_OM.h"
#include "Wnd_Text_OM.h"
#include "OM_Logging.h"

namespace OM
{

#define ZOMBIECHECK if(!mp_connection) return E_ZOMBIE;

Connection::Connection(::Connection *pConnection)
:  mp_connection(pConnection)
{
}

STDMETHODIMP Connection::QueryInterface(REFIID riid, void **ppvObj)
{
   return TQueryInterface(riid, ppvObj);
}

STDMETHODIMP Connection::Send(BSTR bstr)
{
   ZOMBIECHECK
   mp_connection->Send(BSTRToLStr(bstr), true);
   return S_OK;
}

STDMETHODIMP Connection::Transmit(BSTR bstr)
{
   ZOMBIECHECK
   mp_connection->Send(BSTRToLStr(bstr), false);
   return S_OK;
}

STDMETHODIMP Connection::Receive(BSTR bstr)
{
   ZOMBIECHECK
   if(mp_connection->IsInReceive())
   {
      ConsoleHTML("<icon error> <font color='red'>Error</font>: calling Receive() during a Receive() is not allowed, see if Display() can be used instead");
      return E_FAIL;
   }
   mp_connection->Receive(OwnedString(bstr));
   return S_OK;
}

STDMETHODIMP Connection::Display(BSTR bstr)
{
   ZOMBIECHECK
   mp_connection->Display(BSTRToLStr(bstr));
   return S_OK;
}

STDMETHODIMP Connection::SetOnSend(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<::Connection::Event_Send>(this, m_hookSend, *mp_connection, pDisp, var);
}

STDMETHODIMP Connection::SetOnReceive(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<::Connection::Event_Receive>(this, m_hookReceive, *mp_connection, pDisp, var);
}

STDMETHODIMP Connection::SetOnDisplay(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK 
   return ManageHook<::Connection::Event_Display>(this, m_hookDisplay, *mp_connection, pDisp, var);
}

STDMETHODIMP Connection::SetOnConnect(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<::Connection::Event_Connect>(this, m_hookConnect, *mp_connection, pDisp, var);
}

STDMETHODIMP Connection::SetOnDisconnect(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<::Connection::Event_Disconnect>(this, m_hookDisconnect, *mp_connection, pDisp, var);
}

STDMETHODIMP Connection::SetOnGMCP(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<::Connection::Event_GMCP>(this, m_hookGMCP, *mp_connection, pDisp, var);
}


void Connection::On(::Connection::Event_Receive &event)
{
   if(m_hookReceive)
      event.Stop(m_hookReceive.CallWithResult(m_hookReceive.var, event.GetString())==true);
}

void Connection::On(::Connection::Event_Display &event)
{
   if(m_hookDisplay) {
      auto p_line=MakeCounting<TextWindowLine>(event.GetTextLine());
      event.Stop(m_hookDisplay.CallWithResult(m_hookDisplay.var, static_cast<IDispatch *>(p_line))==true);
   }
}

void Connection::On(::Connection::Event_Connect &event)
{
   if(m_hookConnect)
      m_hookConnect.Call();
}

void Connection::On(::Connection::Event_Disconnect &event)
{
   if(m_hookDisconnect)
      m_hookDisconnect.Call();
}

void Connection::On(::Connection::Event_Send &event)
{
   if(m_hookSend)
      event.Stop(m_hookSend.CallWithResult(m_hookSend.var, event.GetString())==true);
}

void Connection::On(::Connection::Event_GMCP &event)
{
   if(m_hookGMCP)
      m_hookGMCP(m_hookGMCP.var, event.GetString());
}

void Connection::On(::Connection::Event_Log &event)
{
}

STDMETHODIMP Connection::IsConnected(VARIANT_BOOL *retval)
{
   ZOMBIECHECK
   *retval=VariantBool(mp_connection->IsConnected()); return S_OK;
}

STDMETHODIMP Connection::Reconnect(VARIANT_BOOL *retval)
{
   ZOMBIECHECK
   *retval=VariantBool(mp_connection->Reconnect()); return S_OK;
}

STDMETHODIMP Connection::IsLogging(VARIANT_BOOL *retval)
{
   ZOMBIECHECK
   *retval=VariantBool(mp_connection->IsLogging()); return S_OK;
}

STDMETHODIMP Connection::get_World(IWorld **retval)
{
   ZOMBIECHECK
   Prop::Server *pServer=mp_connection->GetServer();
   if(!pServer) { *retval=nullptr; return S_OK; }

   *retval=new World(*pServer); (*retval)->AddRef(); return S_OK;
}

STDMETHODIMP Connection::get_Character(ICharacter **retval)
{
   ZOMBIECHECK
   Prop::Character *pCharacter=mp_connection->GetCharacter();
   if(!pCharacter) { *retval=nullptr; return S_OK; }

   *retval=new Character(*pCharacter); (*retval)->AddRef(); return S_OK;
}

STDMETHODIMP Connection::get_Puppet(IPuppet **retval)
{
   ZOMBIECHECK
   Prop::Puppet *pPuppet=mp_connection->GetPuppet();
   if(!pPuppet) { *retval=nullptr; return S_OK; }

   *retval=new Puppet(*pPuppet); (*retval)->AddRef(); return S_OK;
}

STDMETHODIMP Connection::get_Window_Main(IWindow_Main **retval)
{
   ZOMBIECHECK
   return RefReturner(retval)(mp_connection->GetWindow_Main());
}

STDMETHODIMP Connection::get_Log(ILog **retval)
{
   ZOMBIECHECK
   if(!mp_connection->IsLogging()) { *retval=nullptr; return S_OK; }

   *retval=new Log(&mp_connection->GetLog()); (*retval)->AddRef(); return S_OK;
}

};

