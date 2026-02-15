//
// IApp
//
#include "OM_Help.h"

struct Scripter;

namespace OM
{

struct ForwardDNS;
struct ReverseDNS;

struct ArrayUInt : Dispatch<I::ArrayUInt>
{
   OwnedArray<uint32> m_array;

   STDMETHODIMP get_Item(VARIANT var, unsigned *retval);
   STDMETHODIMP get_Count(long *retval);
};

struct FindString : Dispatch<I::FindString>
{
   FindString(Prop::Trigger &propTrigger);

   STDMETHODIMP get_MatchText(BSTR *) override;
   STDMETHODIMP put_MatchText(BSTR) override;

   STDMETHODIMP get_RegularExpression(VARIANT_BOOL *) override;
   STDMETHODIMP put_RegularExpression(VARIANT_BOOL) override;
   STDMETHODIMP get_MatchCase(VARIANT_BOOL *) override;
   STDMETHODIMP put_MatchCase(VARIANT_BOOL) override;
   STDMETHODIMP get_StartsWith(VARIANT_BOOL *) override;
   STDMETHODIMP put_StartsWith(VARIANT_BOOL) override;
   STDMETHODIMP get_EndsWith(VARIANT_BOOL *) override;
   STDMETHODIMP put_EndsWith(VARIANT_BOOL) override;
   STDMETHODIMP get_WholeWord(VARIANT_BOOL *) override;
   STDMETHODIMP put_WholeWord(VARIANT_BOOL) override;

private:
   Prop::FindString *m_ppropFindString;
   CntRefTo<Prop::Trigger> m_propTrigger;
};

struct Trigger : Dispatch<I::Trigger>
{
   Trigger(Prop::Trigger &propTrigger, Prop::Triggers *ppropTriggers);

   STDMETHODIMP Delete(VARIANT_BOOL *) override;

   STDMETHODIMP get_FindString(I::FindString **) override;

   STDMETHODIMP get_Disabled(VARIANT_BOOL *) override;
   STDMETHODIMP put_Disabled(VARIANT_BOOL ) override;
   STDMETHODIMP get_StopProcessing(VARIANT_BOOL *) override;
   STDMETHODIMP put_StopProcessing(VARIANT_BOOL ) override;
   STDMETHODIMP get_OncePerLine(VARIANT_BOOL *) override;
   STDMETHODIMP put_OncePerLine(VARIANT_BOOL ) override;

   STDMETHODIMP get_AwayPresent(VARIANT_BOOL *) override;
   STDMETHODIMP put_AwayPresent(VARIANT_BOOL ) override;
   STDMETHODIMP get_AwayPresentOnce(VARIANT_BOOL *) override;
   STDMETHODIMP put_AwayPresentOnce(VARIANT_BOOL ) override;
   STDMETHODIMP get_Away(VARIANT_BOOL *) override;
   STDMETHODIMP put_Away(VARIANT_BOOL ) override;

   STDMETHODIMP get_Triggers(I::Triggers **) override;

private:
   CntRefTo<Prop::Trigger> m_propTrigger;
   CntPtrTo<Prop::Triggers> m_ppropTriggers; // The list that we are in
   friend struct Triggers;
};

struct Triggers : Dispatch<I::Triggers>
{
   Triggers(Prop::Triggers &propTriggers);

   STDMETHODIMP Item(VARIANT var, I::Trigger **retval) override;
   STDMETHODIMP get_Count(long *retval) override;

   STDMETHODIMP get_Active(VARIANT_BOOL *) override;
   STDMETHODIMP put_Active(VARIANT_BOOL) override;

   STDMETHODIMP Delete(long index);
   STDMETHODIMP AddCopy(I::Trigger *, I::Trigger **);
   STDMETHODIMP Move(long from, long to);

private:
   CntRefTo<Prop::Triggers> m_propTriggers;
};

struct Alias : Dispatch<I::Alias>
{
   Alias(Prop::Alias &prop_alias, Prop::Aliases *prop_aliases);

   STDMETHODIMP get_StopProcessing(VARIANT_BOOL *) override;
   STDMETHODIMP put_StopProcessing(VARIANT_BOOL) override;
   STDMETHODIMP get_Folder(VARIANT_BOOL *) override;
   STDMETHODIMP put_Folder(VARIANT_BOOL) override;

private:
   CntRefTo<Prop::Alias> m_prop_alias;
   CntPtrTo<Prop::Aliases> mp_prop_aliases;
};

struct Aliases : Dispatch<I::Aliases>
{
   Aliases(Prop::Aliases &prop_aliases);

   STDMETHODIMP Item(VARIANT var, I::Alias **retval) override;
   STDMETHODIMP get_Count(long *retval) override;

private:
   CntRefTo<Prop::Aliases> m_prop_aliases;
};

struct Puppet : Dispatch<I::Puppet>
{
   Puppet(Prop::Puppet &propPuppet);

   STDMETHODIMP get_Name(BSTR *);
   STDMETHODIMP put_Name(BSTR);
   STDMETHODIMP get_Info(BSTR *);
   STDMETHODIMP put_Info(BSTR);
   STDMETHODIMP get_ReceivePrefix(BSTR *);
   STDMETHODIMP put_ReceivePrefix(BSTR);
   STDMETHODIMP get_SendPrefix(BSTR *);
   STDMETHODIMP put_SendPrefix(BSTR);
   STDMETHODIMP get_LogFileName(BSTR *);
   STDMETHODIMP put_LogFileName(BSTR);
   STDMETHODIMP get_AutoConnect(VARIANT_BOOL *);
   STDMETHODIMP put_AutoConnect(VARIANT_BOOL);
   STDMETHODIMP get_ConnectWithPlayer(VARIANT_BOOL *);
   STDMETHODIMP put_ConnectWithPlayer(VARIANT_BOOL);

   STDMETHODIMP get_Triggers(I::Triggers **);
   STDMETHODIMP get_Aliases(I::Aliases **);

private:
   CntRefTo<Prop::Puppet> m_propPuppet;
};

struct Puppets : Dispatch<I::Puppets>
{
   Puppets(Prop::Puppets &propPuppets);

   STDMETHODIMP Item(VARIANT var, I::Puppet **retval) override;
   STDMETHODIMP get_Count(long *retval) override;

private:

   CntRefTo<Prop::Puppets> m_propPuppets;
};

struct Character : Dispatch<I::Character>
{
   Character(Prop::Character &propCharacter);

   STDMETHODIMP get_Shortcut(BSTR *retval);
   STDMETHODIMP put_Shortcut(BSTR bstr);
   STDMETHODIMP get_Name(BSTR *retval);
   STDMETHODIMP put_Name(BSTR bstr);
   STDMETHODIMP get_Connect(BSTR *retval);
   STDMETHODIMP put_Connect(BSTR bstr);
   STDMETHODIMP get_Info(BSTR *retval);
   STDMETHODIMP put_Info(BSTR bstr);
   STDMETHODIMP get_LogFileName(BSTR *retval);
   STDMETHODIMP put_LogFileName(BSTR bstr);

   STDMETHODIMP get_LastUsed(VARIANT *date);
   STDMETHODIMP get_TimeCreated(VARIANT *date);

   STDMETHODIMP get_Triggers(I::Triggers **retval);
   STDMETHODIMP get_Aliases(I::Aliases **retval);
   STDMETHODIMP get_Puppets(I::Puppets **retval);

private:
   CntRefTo<Prop::Character> m_propCharacter;
};

struct Characters : Dispatch<I::Characters>
{
   Characters(Prop::Server &propServer);

   STDMETHODIMP Item(VARIANT var, I::Character **retval) override;
   STDMETHODIMP get_Count(long *retval) override;

private:

   CntRefTo<Prop::Server> m_propServer;
};

struct World : Dispatch<I::World>
{
   World(Prop::Server &propServer);

   STDMETHODIMP get_Shortcut(BSTR *retval);
   STDMETHODIMP put_Shortcut(BSTR bstr);
   STDMETHODIMP get_Name(BSTR *retval);
   STDMETHODIMP put_Name(BSTR bstr);
   STDMETHODIMP get_Info(BSTR *retval);
   STDMETHODIMP put_Info(BSTR bstr);
   STDMETHODIMP get_Host(BSTR *retval);
   STDMETHODIMP put_Host(BSTR bstr);

   STDMETHODIMP get_Characters(I::Characters **retval);
   STDMETHODIMP get_Triggers(I::Triggers **retval);
   STDMETHODIMP get_Aliases(I::Aliases **retval);

private:
   CntRefTo<Prop::Server> m_propServer;
};

struct Worlds : Dispatch<I::Worlds>
{
   Worlds();

   STDMETHODIMP Item(VARIANT var, I::World **retval) override;
   STDMETHODIMP get_Count(long *retval) override;

private:

   Prop::Servers &m_propServers;
};

struct App
:  Dispatch<I::App>,
   Events::ReceiversOf<App, Event_NewWindow>
{
   App();
   ~App() noexcept;

   USE_INHERITED_UNKNOWN(I::App)

   // IUnknown
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;

   // IApp
   STDMETHODIMP get_BuildNumber(long *retval) override { *retval=g_build_number; return S_OK; }
   STDMETHODIMP get_BuildDate(DATE *retval) override;
   STDMETHODIMP get_Version(long *retval) override { *retval=g_ciVersion; return S_OK; }

   STDMETHODIMP get_ConfigPath(BSTR *retval) override { *retval=LStrToBSTR(GetConfigPath()); return S_OK;}

   STDMETHODIMP get_Worlds(I::Worlds **retval) override;
   STDMETHODIMP get_Windows(I::Windows **retval) override;
   STDMETHODIMP get_Triggers(I::Triggers **retval) override;
   STDMETHODIMP get_Aliases(I::Aliases **retval) override;

   STDMETHODIMP ActiveXObject(BSTR name, IDispatch **retval) override;

   STDMETHODIMP NewTrigger(I::Trigger **retval) override;
   STDMETHODIMP NewWindow_FixedText(int iWidth, int iHeight, I::Window_FixedText **retval) override;
   STDMETHODIMP NewWindow_Text(int iWidth, int iHeight, I::Window_Text **retval) override;

   STDMETHODIMP NewWindow(I::Window_Main **retval) override;
   STDMETHODIMP SetOnNewWindow(IDispatch *pDisp, VARIANT var) override;

   STDMETHODIMP CreateInterval(int iTimeOut, IDispatch *pDisp, VARIANT var, I::Timer **retval) override;
   STDMETHODIMP CreateTimeout(int iTimeOut, IDispatch *pDisp, VARIANT var, I::Timer **retval) override;

   STDMETHODIMP NewWindow_Graphics(int iWidth, int iHeight, I::Window_Graphics **retval) override;

   STDMETHODIMP PlaySound(BSTR filename, float volume) override;
   STDMETHODIMP StopSounds() override;

   STDMETHODIMP OutputDebugHTML(BSTR bstr) override;
   STDMETHODIMP OutputDebugText(BSTR bstr) override;

   STDMETHODIMP New_Socket(I::Socket **retval) override;
   STDMETHODIMP New_SocketServer(unsigned int iPort, IDispatch *pDisp, VARIANT var, I::SocketServer **retval) override;

   STDMETHODIMP ForwardDNSLookup(BSTR bstrHost, IDispatch *pDisp, VARIANT var) override;
   STDMETHODIMP ReverseDNSLookup(BSTR bstrIP, IDispatch *pDisp, VARIANT var) override;

   STDMETHODIMP IsAddress(BSTR bstrIP, VARIANT_BOOL *retval) override;

   void On(Event_NewWindow &event);

private:

   CntPtrTo<I::Worlds> m_pIWorlds;
   CntPtrTo<I::Windows> m_pIWindows;

   HookVariant m_hookNewWindow;

   struct OMTimer : Dispatch<I::Timer>, DLNode<OMTimer>


   {
      OMTimer(OMTimer *pInsertAfter, IDispatch *pDispatch, VARIANTARG &var, int iTimeOut, bool fRepeating);

      // ITimer
      void STDMETHODCALLTYPE Kill();
      STDMETHODIMP get_UserData(VARIANT *pVar) override { VariantCopy(pVar, &m_hook.var); return S_OK; }
      void STDMETHODCALLTYPE put_UserData(VARIANT var) override { m_hook.var=var; }
      STDMETHODIMP get_Active(VARIANT_BOOL *retval) override { *retval=BoolVariant(m_timer); return S_OK; }

      // Event_Timer
      void OnTimer();

   private:

      Time::Timer m_timer{[this]() { OnTimer(); }};
      bool m_fRepeating;
      HookVariant m_hook;
      bool m_fExecuting{}; // Timer is currently being processed
   };

   DLNode<OMTimer> m_firstTimer; // Active timer list (to keep at least 1 refcount)
   DLNode<ForwardDNS> m_firstForwardDNS; // Active ForwardDNS list (to keep at least 1 refcount)
   DLNode<ReverseDNS> m_firstReverseDNS; // Active ReverseDNS list (to keep at least 1 refcount)
};

};
