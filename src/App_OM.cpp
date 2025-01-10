#include "Main.h"
#include "Scripter.h"
#include "Wnd_Main.h"

#include "Wnd_Text_OM.h"

#include "Wnd_Main_OM.h"
#include "App_OM.h"
#include "OM_Sockets.h"
#include "Sounds.h"

namespace OM
{

STDMETHODIMP ArrayUInt::get_Item(VARIANT var, unsigned *retval)
{
   Variant v2;
   if(v2.Convert<int32>(var))
   {
      unsigned index=v2.Get<int32>();

      if(index>=m_array.Count())
         return E_INVALIDARG;

      *retval=m_array[index];
      return S_OK;
   }

   return E_FAIL;
}

STDMETHODIMP ArrayUInt::get_Count(long *retval)
{
   *retval=m_array.Count(); return S_OK;
}

//
// FindString
//

FindString::FindString(Prop::Trigger &propTrigger) : m_ppropFindString(&propTrigger.propFindString()), m_propTrigger(propTrigger)
{
}

STDMETHODIMP FindString::get_MatchText(BSTR *retval)
{
   *retval=LStrToBSTR(m_ppropFindString->pclMatchText()); return S_OK;
}

STDMETHODIMP FindString::put_MatchText(BSTR bstr)
{
   m_ppropFindString->pclMatchText(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP FindString::get_RegularExpression(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_ppropFindString->fRegularExpression()); return S_OK;
}

STDMETHODIMP FindString::put_RegularExpression(VARIANT_BOOL flag)
{
   m_ppropFindString->fRegularExpression(VariantBool(flag)); return S_OK;
}

STDMETHODIMP FindString::get_MatchCase(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_ppropFindString->fMatchCase()); return S_OK;
}

STDMETHODIMP FindString::put_MatchCase(VARIANT_BOOL flag)
{
   m_ppropFindString->fMatchCase(VariantBool(flag)); return S_OK;
}
STDMETHODIMP FindString::get_StartsWith(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_ppropFindString->fStartsWith()); return S_OK;
}

STDMETHODIMP FindString::put_StartsWith(VARIANT_BOOL flag)
{
   m_ppropFindString->fStartsWith(VariantBool(flag)); return S_OK;
}
STDMETHODIMP FindString::get_EndsWith(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_ppropFindString->fEndsWith()); return S_OK;
}

STDMETHODIMP FindString::put_EndsWith(VARIANT_BOOL flag)
{
   m_ppropFindString->fEndsWith(VariantBool(flag)); return S_OK;
}
STDMETHODIMP FindString::get_WholeWord(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_ppropFindString->fWholeWord()); return S_OK;
}

STDMETHODIMP FindString::put_WholeWord(VARIANT_BOOL flag)
{
   m_ppropFindString->fWholeWord(VariantBool(flag)); return S_OK;
}

//
// Trigger
//

Trigger::Trigger(Prop::Trigger &propTrigger, Prop::Triggers *ppropTriggers)
 : m_propTrigger(propTrigger), m_ppropTriggers(ppropTriggers)
{
}

STDMETHODIMP Trigger::Delete(VARIANT_BOOL *retval)
{
   *retval=BoolVariant(false);

   if(m_ppropTriggers)
   {
      for(unsigned int i=0;i<m_ppropTriggers->Count();i++)
      {
         if((*m_ppropTriggers)[i]==m_propTrigger)
         {
            m_ppropTriggers->Delete(i);
            *retval=BoolVariant(true);
            break;
         }
      }

      m_ppropTriggers=nullptr; // Regardless if we succeeded or not, we have no owner
   }

   return S_OK;
}

STDMETHODIMP Trigger::get_FindString(IFindString **retval)
{
   return RefReturner(retval)(MakeCounting<FindString>(*m_propTrigger));
}

STDMETHODIMP Trigger::get_Disabled(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_propTrigger->fDisabled()); return S_OK;
}

STDMETHODIMP Trigger::put_Disabled(VARIANT_BOOL flag)
{
   m_propTrigger->fDisabled(VariantBool(flag)); return S_OK;
}

STDMETHODIMP Trigger::get_StopProcessing(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_propTrigger->fStopProcessing()); return S_OK;
}

STDMETHODIMP Trigger::put_StopProcessing(VARIANT_BOOL flag)
{
   m_propTrigger->fStopProcessing(VariantBool(flag)); return S_OK;
}

STDMETHODIMP Trigger::get_OncePerLine(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_propTrigger->fOncePerLine()); return S_OK;
}

STDMETHODIMP Trigger::put_OncePerLine(VARIANT_BOOL flag)
{
   m_propTrigger->fOncePerLine(VariantBool(flag)); return S_OK;
}

STDMETHODIMP Trigger::get_AwayPresent(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_propTrigger->fAwayPresent()); return S_OK;
}

STDMETHODIMP Trigger::put_AwayPresent(VARIANT_BOOL flag)
{
   m_propTrigger->fAwayPresent(VariantBool(flag)); return S_OK;
}

STDMETHODIMP Trigger::get_AwayPresentOnce(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_propTrigger->fAwayPresentOnce()); return S_OK;
}

STDMETHODIMP Trigger::put_AwayPresentOnce(VARIANT_BOOL flag)
{
   m_propTrigger->fAwayPresentOnce(VariantBool(flag)); return S_OK;
}

STDMETHODIMP Trigger::get_Away(VARIANT_BOOL *flag)
{
   *flag=BoolVariant(m_propTrigger->fAway()); return S_OK;
}

STDMETHODIMP Trigger::put_Away(VARIANT_BOOL flag)
{
   m_propTrigger->fAway(VariantBool(flag)); return S_OK;
}

STDMETHODIMP Trigger::get_Triggers(ITriggers **retval)
{
   return RefReturner(retval)(MakeCounting<Triggers>(m_propTrigger->propTriggers()));
}

Triggers::Triggers(Prop::Triggers &propTriggers) : m_propTriggers(propTriggers)
{
}

STDMETHODIMP Triggers::get_Item(VARIANT var, ITrigger** retval)
{
   Variant v2;
   if(v2.Convert<uint32>(var))
   {
      unsigned iTrigger=v2.Get<uint32>();
      if(iTrigger>=m_propTriggers->Count())
         return E_INVALIDARG;

      return RefReturner(retval)(MakeCounting<Trigger>(*(*m_propTriggers)[iTrigger], m_propTriggers));
   }

   return E_FAIL;
}

STDMETHODIMP Triggers::get_Count(long *retval)
{
   *retval=m_propTriggers->Count(); return S_OK;
}

STDMETHODIMP Triggers::Delete(long index)
{
   if(static_cast<unsigned int>(index)>=m_propTriggers->Count())
      return E_INVALIDARG;
   m_propTriggers->Delete(index);
   return S_OK;
}

STDMETHODIMP Triggers::AddCopy(ITrigger *pITrigger, ITrigger **retval)
{
   Trigger *pTrigger=static_cast<Trigger *>(pITrigger);
   auto pCopy=MakeCounting<Prop::Trigger>(*pTrigger->m_propTrigger);
   m_propTriggers->Push(pCopy);

   return RefReturner(retval)(MakeCounting<Trigger>(*pCopy, m_propTriggers));
}

STDMETHODIMP Triggers::Move(long from, long to)
{
   if(from<0 || from >= (long)m_propTriggers->Count() )
      return E_FAIL;
   if(to<0 || to > (long)m_propTriggers->Count() )
      return E_FAIL;

   return E_FAIL; // NYI
}

Puppet::Puppet(Prop::Puppet &propPuppet) : m_propPuppet(propPuppet)
{
}

STDMETHODIMP Puppet::get_Name(BSTR *retval)
{
   *retval=LStrToBSTR(m_propPuppet->pclName()); return S_OK;
}

STDMETHODIMP Puppet::put_Name(BSTR bstr)
{
   m_propPuppet->pclName(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Puppet::get_Info(BSTR *retval)
{
   *retval=LStrToBSTR(m_propPuppet->pclInfo()); return S_OK;
}

STDMETHODIMP Puppet::put_Info(BSTR bstr)
{
   m_propPuppet->pclInfo(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Puppet::get_ReceivePrefix(BSTR *retval)
{
   *retval=LStrToBSTR(m_propPuppet->pclReceivePrefix()); return S_OK;
}

STDMETHODIMP Puppet::put_ReceivePrefix(BSTR bstr)
{
   m_propPuppet->pclReceivePrefix(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Puppet::get_SendPrefix(BSTR *retval)
{
   *retval=LStrToBSTR(m_propPuppet->pclSendPrefix()); return S_OK;
}

STDMETHODIMP Puppet::put_SendPrefix(BSTR bstr)
{
   m_propPuppet->pclSendPrefix(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Puppet::get_LogFileName(BSTR *retval)
{
   *retval=LStrToBSTR(m_propPuppet->pclLogFileName()); return S_OK;
}

STDMETHODIMP Puppet::put_LogFileName(BSTR bstr)
{
   m_propPuppet->pclLogFileName(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Puppet::get_AutoConnect(VARIANT_BOOL *retval)
{
   *retval=VariantBool(m_propPuppet->fAutoConnect()); return S_OK;
}

STDMETHODIMP Puppet::put_AutoConnect(VARIANT_BOOL flag)
{
   m_propPuppet->fAutoConnect(VariantBool(flag)); return S_OK;
}

STDMETHODIMP Puppet::get_ConnectWithPlayer(VARIANT_BOOL *retval)
{
   *retval=VariantBool(m_propPuppet->fConnectWithPlayer()); return S_OK;
}

STDMETHODIMP Puppet::put_ConnectWithPlayer(VARIANT_BOOL flag)
{
   m_propPuppet->fConnectWithPlayer(VariantBool(flag)); return S_OK;
}

STDMETHODIMP Puppet::get_Triggers(ITriggers **retval)
{
   return RefReturner(retval)(MakeCounting<Triggers>(m_propPuppet->propTriggers()));
}

Puppets::Puppets(Prop::Puppets &propPuppets) : m_propPuppets(propPuppets)
{
}

STDMETHODIMP Puppets::get_Item(VARIANT var, IPuppet **retval)
{
   RefReturner returner(retval);

   if(var.vt==TypeToVT<BSTR>)
   {
      BSTRToLStr string(var.bstrVal);

      for(auto &v : *m_propPuppets)
         if(v->pclName().ICompare(string)==0)
            return returner(MakeCounting<Puppet>(*v));

      return E_FAIL;
   }

   Variant v2;
   if(v2.Convert<uint32>(var))
   {
      unsigned iPuppet=v2.Get<uint32>();

      if(iPuppet>=m_propPuppets->Count())
         return E_INVALIDARG;

      return returner(MakeCounting<Puppet>(*(*m_propPuppets)[iPuppet]));
   }

   return E_FAIL;
}

STDMETHODIMP Puppets::get_Count(long *retval)
{
   *retval=m_propPuppets->Count(); return S_OK;
}


Character::Character(Prop::Character &propCharacter) : m_propCharacter(propCharacter)
{
}

STDMETHODIMP Character::get_Shortcut(BSTR *retval)
{
   *retval=LStrToBSTR(m_propCharacter->pclName()); return S_OK;
}

STDMETHODIMP Character::put_Shortcut(BSTR bstr)
{
   m_propCharacter->pclName(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Character::get_Name(BSTR *retval)
{
   *retval=LStrToBSTR(m_propCharacter->pclName()); return S_OK;
}

STDMETHODIMP Character::put_Name(BSTR bstr)
{
   m_propCharacter->pclName(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Character::get_Connect(BSTR *retval)
{
   *retval=LStrToBSTR(m_propCharacter->pclConnect()); return S_OK;
}

STDMETHODIMP Character::put_Connect(BSTR bstr)
{
   m_propCharacter->pclConnect(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Character::get_Info(BSTR *retval)
{
   *retval=LStrToBSTR(m_propCharacter->pclInfo()); return S_OK;
}

STDMETHODIMP Character::put_Info(BSTR bstr)
{
   m_propCharacter->pclInfo(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Character::get_LogFileName(BSTR *retval)
{
   *retval=LStrToBSTR(m_propCharacter->pclLogFileName()); return S_OK;
}

STDMETHODIMP Character::put_LogFileName(BSTR bstr)
{
   m_propCharacter->pclLogFileName(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP Character::get_LastUsed(VARIANT *retval)
{
   return SystemTimeToVariant(*retval, m_propCharacter->timeLastUsed());
}

STDMETHODIMP Character::get_TimeCreated(VARIANT *retval)
{
   return SystemTimeToVariant(*retval, m_propCharacter->timeCreated());
}

STDMETHODIMP Character::get_Triggers(ITriggers **retval)
{
   return RefReturner(retval)(MakeCounting<Triggers>(m_propCharacter->propTriggers()));
}

STDMETHODIMP Character::get_Puppets(IPuppets **retval)
{
   return RefReturner(retval)(MakeCounting<Puppets>(m_propCharacter->propPuppets()));
}

Characters::Characters(Prop::Server &propServer) : m_propServer(propServer)
{
}

HRESULT Characters::get_Item(VARIANT var, ICharacter** retval)
{
   RefReturner returner(retval);

   if(var.vt==VT_BSTR)
   {
      BSTRToLStr string(var.bstrVal);

      for(auto &p : m_propServer->propCharacters())
         if(p->pclName().ICompare(string)==0)
            return returner(MakeCounting<Character>(*p));

      return E_FAIL;
   }

   Variant v2;
   if(v2.Convert<uint32>(var))
   {
      unsigned iCharacter=v2.Get<uint32>();
      if(iCharacter>=m_propServer->propCharacters().Count())
         return E_INVALIDARG;

      return returner(MakeCounting<Character>(*m_propServer->propCharacters()[iCharacter]));
   }

   return E_FAIL;
}

HRESULT Characters::get_Count(long *retval)
{
   *retval=m_propServer->propCharacters().Count(); return S_OK;
}

World::World(Prop::Server &propServer) : m_propServer(propServer)
{
}

STDMETHODIMP World::get_Shortcut(BSTR *retval)
{
   *retval=LStrToBSTR(m_propServer->pclName()); return S_OK;
}

STDMETHODIMP World::put_Shortcut(BSTR bstr)
{
   m_propServer->Rename(g_ppropGlobal->propConnections().propServers(), BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP World::get_Name(BSTR *retval)
{
   *retval=LStrToBSTR(m_propServer->pclName()); return S_OK;
}

STDMETHODIMP World::put_Name(BSTR bstr)
{
   m_propServer->Rename(g_ppropGlobal->propConnections().propServers(), BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP World::get_Info(BSTR *retval)
{
   *retval=LStrToBSTR(m_propServer->pclInfo()); return S_OK;
}

STDMETHODIMP World::put_Info(BSTR bstr)
{
   m_propServer->pclInfo(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP World::get_Host(BSTR *retval)
{
   *retval=LStrToBSTR(m_propServer->pclHost()); return S_OK;
}

STDMETHODIMP World::put_Host(BSTR bstr)
{
   m_propServer->pclHost(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP World::get_Characters(ICharacters **retval)
{
   return RefReturner(retval)(MakeCounting<Characters>(*m_propServer));
}

STDMETHODIMP World::get_Triggers(ITriggers **retval)
{
   return RefReturner(retval)(MakeCounting<Triggers>(m_propServer->propTriggers()));
}

Worlds::Worlds() : m_propServers(g_ppropGlobal->propConnections().propServers())
{
}

STDMETHODIMP Worlds::get_Item(VARIANT var, IWorld **retval)
{
   RefReturner returner(retval);

   if(var.vt==VT_BSTR)
   {
      BSTRToLStr string(var.bstrVal);

      for(auto &pServer : m_propServers)
         if(pServer->pclName().ICompare(string)==0)
            return returner(MakeCounting<World>(*pServer));

      return E_FAIL;
   }

   Variant v2;
   if(v2.Convert<uint32>(var))
   {
      unsigned iWorld=v2.Get<uint32>();
      if(iWorld>=m_propServers.Count())
         return E_INVALIDARG;

      return returner(MakeCounting<World>(*m_propServers[iWorld]));
   }

   return E_FAIL;
}

STDMETHODIMP Worlds::get_Count(long *retval)
{
   *retval=g_ppropGlobal->propConnections().propServers().Count(); return S_OK;
}

STDMETHODIMP App::QueryInterface(const GUID &id, void **ppvObj)
{
   return TQueryInterfaces<IApp>(id, ppvObj);
}

App::App()
{
}

App::~App()
{
   while(m_firstTimer.Linked())
      m_firstTimer.Next()->Kill();

   while(m_firstForwardDNS.Linked())
      m_firstForwardDNS.Next()->Release();

   while(m_firstReverseDNS.Linked())
      m_firstReverseDNS.Next()->Release();
}

STDMETHODIMP App::get_BuildDate(DATE *retval)
{
   Time::Build buildTime;
   return (SystemTimeToVariantTime(&buildTime, retval)!=0) ? S_OK : E_FAIL;
}

HRESULT App::get_Worlds(IWorlds **retval)
{
   RefReturner returner(retval);

   if(!m_pIWorlds) m_pIWorlds=new Worlds();
   return returner(m_pIWorlds);
}

HRESULT App::get_Windows(IWindows **retval)
{
   RefReturner returner(retval);

   if(!m_pIWindows) m_pIWindows=MakeCounting<Windows>();
   return returner(m_pIWindows);
}

HRESULT App::get_Triggers(ITriggers **retval)
{
   return RefReturner(retval)(MakeCounting<Triggers>(g_ppropGlobal->propConnections().propTriggers()));
}

STDMETHODIMP App::SetOnNewWindow(IDispatch *pDisp, VARIANT var)
{
   return ManageHook<Event_NewWindow>(this, m_hookNewWindow, GlobalEvents::GetInstance(), pDisp, var);
}

STDMETHODIMP App::NewWindow(IWindow_Main **retval)
{
   return RefReturner(retval)( (new Wnd_Main(Wnd_MDI::GetInstance()))->GetDispatch());
}

void App::On(Event_NewWindow &event)
{
   if(m_hookNewWindow)
      m_hookNewWindow.Call();
}

STDMETHODIMP App::ActiveXObject(BSTR name, IDispatch **retval)
{
   // Convert ProgID string to CLSID
   CLSID clsid;
   HRESULT hr = CLSIDFromProgID(name, &clsid);
   if(FAILED(hr))
      return E_FAIL;

   CntPtrTo<IDispatch> p_object;
   hr=p_object.CoCreateInstance(clsid);
   return RefReturner(retval)(p_object, hr);
}


STDMETHODIMP App::NewTrigger(ITrigger **retval)
{
   return RefReturner(retval)(new Trigger(*new Prop::Trigger(), nullptr));
}

STDMETHODIMP App::NewWindow_FixedText(int iWidth, int iHeight, IWindow_FixedText **retval)
{
   return RefReturner(retval)(Create_Window_FixedText(int2(iWidth, iHeight)));
}

STDMETHODIMP App::NewWindow_Graphics(int iWidth, int iHeight, IWindow_Graphics **retval)
{
   return RefReturner(retval)(Create_Window_Graphics(int2(iWidth, iHeight)));
}

STDMETHODIMP App::NewWindow_Text(int iWidth, int iHeight, IWindow_Text **retval)
{
   CntPtrTo<IWindow_Text> pText=new Window_Text(int2(iWidth, iHeight));
   return RefReturner(retval)(pText);
}

STDMETHODIMP App::PlaySound(BSTR filename, float volume)
{
   ::PlaySound(BSTRToLStr(filename), volume);
   return S_OK;
}

STDMETHODIMP App::StopSounds()
{
   ::StopSounds();
   return S_OK;
}

STDMETHODIMP App::OutputDebugHTML(BSTR bstr)
{
   ConsoleHTML(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP App::OutputDebugText(BSTR bstr)
{
   ConsoleText(BSTRToLStr(bstr)); return S_OK;
}

STDMETHODIMP App::CreateInterval(int iTimeOut, IDispatch *pDisp, VARIANT var, ITimer **retval)
{
   return RefReturner(retval)(new OMTimer(m_firstTimer.Prev(), pDisp, var, iTimeOut, true));
}

STDMETHODIMP App::CreateTimeout(int iTimeOut, IDispatch *pDisp, VARIANT var, ITimer **retval)
{
   return RefReturner(retval)(new OMTimer(m_firstTimer.Prev(), pDisp, var, iTimeOut, false));
}

App::OMTimer::OMTimer(OMTimer *pInsertAfter, IDispatch *pDispatch, VARIANT &var, int iTimeOut, bool fRepeating)
:  DLNode<OMTimer>(pInsertAfter), m_fRepeating(fRepeating)
{
   AddRef(); // Since we're in a linked list, addref() so we don't dissapear too soon
   m_hook.Set(pDispatch, var);
   m_timer.Set(iTimeOut/1000.0f, fRepeating);
}

void App::OMTimer::Kill()
{
   m_timer.Reset();
   m_hook.Clear();
   if(Linked())
   {
      Unlink();
      Release(); // See the constructor for matching addref()
   }
}

void App::OMTimer::OnTimer()
{
   if(!m_hook)
      return Kill();

   CntPtrTo<OMTimer> pThis(this); // We're using ourself here, so nobody deletes us on the Kill() or the Call()

   if(m_fExecuting)
      return; // Already executing this timer (recursion) or it was unlinked
   RestorerOf _(m_fExecuting); m_fExecuting=true;

   Variant varResult=m_hook.CallWithResult(m_hook.var);
   // Stop the timer if the call returned true
   if(varResult==true)
      Kill();
}

STDMETHODIMP App::New_Socket(ISocket **retval)
{
   *retval=new Socket(); (*retval)->AddRef(); return S_OK;
}

STDMETHODIMP App::New_SocketServer(unsigned int iPort, IDispatch *pDisp, VARIANT var, ISocketServer **retval)
{
   return RefReturner(retval)(new SocketServer(iPort, pDisp, var));
}

STDMETHODIMP App::ForwardDNSLookup(BSTR bstrHost, IDispatch *pDisp, VARIANT var)
{
   if(!pDisp) return E_FAIL;

   new OM::ForwardDNS(m_firstForwardDNS.Prev(), bstrHost, pDisp, var);
   return S_OK;
};

STDMETHODIMP App::ReverseDNSLookup(BSTR bstrIP, IDispatch *pDisp, VARIANT var)
{
   if(!pDisp) return E_FAIL;

   new OM::ReverseDNS(m_firstReverseDNS.Prev(), bstrIP, pDisp, var);
   return S_OK;
}

STDMETHODIMP App::IsAddress(BSTR bstrIP, VARIANT_BOOL *retval)
{
   IN_ADDR addr;
   IN6_ADDR addr6;

   // TODO: InetPtonW? or inet_pton?
   OwnedString strIP(bstrIP);
   if(inet_pton(AF_INET, strIP.stringz(), &addr)==1)
      *retval=ToVariantBool(true);
   else if(inet_pton(AF_INET6, strIP.stringz(), &addr6)==1)
      *retval=ToVariantBool(true);
   else
      *retval=ToVariantBool(false);
   return S_OK;
}

};
