#include "Main.h"

#define INITGUID
#include <initguid.h>
#include <comcat.h>
#include <activscp.h>
#include "activdbg.h"

//DEFINE_GUID(CATID_ActiveScript, 0xf0b7a1a1, 0x9847, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);
//DEFINE_GUID(CATID_ActiveScriptParse, 0xf0b7a1a2, 0x9847, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);

// These are apparently not defined in activdbg.h although they are shown in there
DEFINE_GUID(IID_IProcessDebugManager, 0x51973C2f, 0xCB0C, 0x11d0, 0xB5, 0xC9, 0x00, 0xA0, 0x24, 0x4A, 0x0E, 0x7A);
DEFINE_GUID(IID_IActiveScriptSiteDebug, 0x51973C11, 0xCB0C, 0x11d0, 0xB5, 0xC9, 0x00, 0xA0, 0x24, 0x4A, 0x0E, 0x7A);

#include "Automation.h"
#include "Scripter.h"

#include "Wnd_Main.h"

// To define all of the GUIDS
#include "OM_Help.h"
#include "App_OM.h"
#include "Wnd_Main_OM.h"

#ifdef CHAKRA

#define USE_EDGEMODE_JSRT
#include "jsrt.h" // JavaScript Runtime APIs
#pragma comment(lib, "chakrart.lib")
//DEFINE_GUID(IID_Chakra, 0x16d51579, 0xa30b, 0x4c8b, 0xa2, 0x76, 0x0f, 0xf4, 0xdc, 0x41, 0xe7, 0x55);
#endif

namespace Windows
{
namespace Msg
{
   struct _State : Message
   {
      static const MessageID ID=WM_USER;

      _State(bool fStart) : Message(WM_USER, (bool)fStart, 0) { }

      bool fStart() const { return m_wParam!=0; }
   };
};
};

//
// Wnd_Console
//
struct Wnd_Console : Wnd_Dialog, Text::IHost, Singleton<Wnd_Console>
{
   Wnd_Console();
   ~Wnd_Console();

   void OutputHTML(ConstString string);
   void OutputText(ConstString string);

private:
   friend TWindowImpl;
   LRESULT WndProc(const Message &msg) override;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Close &msg);

   AL::Button *m_pbtClear;
   Text::Wnd *mp_wnd_text;
};

Wnd_Console::Wnd_Console()
{
   Create("Console Output", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0 /*dwExStyle*/, nullptr);
}

Wnd_Console::~Wnd_Console()
{
}

LRESULT Wnd_Console::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command, Msg::Close>(msg);
}

LRESULT Wnd_Console::On(const Msg::Create &msg)
{
   auto pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   {
      mp_wnd_text=new Text::Wnd(*this, *this);
      mp_wnd_text->SetFont("Arial", 15);
      mp_wnd_text->SetMargins(Rect(2, 2, 2, 2));
      AL::ChildWindow *pWnd=m_layout.AddChildWindow(*mp_wnd_text); *pGV << pWnd;
      pWnd->szMinimum()=int2(500, 100)*g_dpiScale;
   }
   {
      AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH; pGH->weight(0);
      m_pbtClear=m_layout.CreateButton(0, "Clear");
      *pGH << m_pbtClear;
   }

   return msg.Success();
}

LRESULT Wnd_Console::On(const Msg::Close &msg)
{
   Show(SW_HIDE);
   return msg.Success();
}

LRESULT Wnd_Console::On(const Msg::Command &msg)
{
   if(msg.wndCtl()==*m_pbtClear)
   {
      mp_wnd_text->Clear(); return msg.Success();
   }

   return msg.Success();
}

void Wnd_Console::OutputHTML(ConstString string)
{
   mp_wnd_text->AddHTML(string);
}

void Wnd_Console::OutputText(ConstString string)
{
   mp_wnd_text->Add(Text::Line::CreateFromText(string));
}

Wnd_Console &ConsoleGet()
{
   if(!Wnd_Console::HasInstance())
      new Wnd_Console();
   auto &console=Wnd_Console::GetInstance();
   console.Show(SW_SHOW);
   return console;
}

void ConsoleSetFocus()
{
   ConsoleGet().Show(SW_SHOWNORMAL);
   ConsoleGet().SetFocus();
}

void ConsoleHTML(ConstString string)
{
   ConsoleGet().OutputHTML(string);
}

void ConsoleText(ConstString string)
{
   ConsoleGet().OutputText(string);
}

void ConsoleDelete()
{
   if(Wnd_Console::HasInstance())
      delete &Wnd_Console::GetInstance();
}

//
// Wnd_ScriptAbort
//
struct Wnd_ScriptAbort : Wnd_Dialog
{
   Wnd_ScriptAbort();
   ~Wnd_ScriptAbort();

private:
   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::_State &msg);
   LRESULT On(const Msg::Timer &msg);

   bool m_has_timer{};

   AL::Button *m_pbtAbort, *m_pbtMinimize;
};

Wnd_ScriptAbort::Wnd_ScriptAbort()
{
   Create("Script Aborter", 0, 0, nullptr);
   InsertAfter(HWND_TOPMOST);
}

Wnd_ScriptAbort::~Wnd_ScriptAbort()
{
   PostQuitMessage(0);
}

LRESULT Wnd_ScriptAbort::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::_State, Msg::Timer, Msg::Command>(msg);
}

LRESULT Wnd_ScriptAbort::On(const Msg::Create &msg)
{
   auto pG=m_layout.CreateGroup_Horizontal(); m_layout.SetRoot(pG);

   m_pbtAbort=m_layout.CreateButton(IDOK, "Abort currently running script");
   m_pbtMinimize=m_layout.CreateButton(0, "Minimize");

   pG->Add(m_pbtAbort, m_pbtMinimize);

   return msg.Success();
}

LRESULT Wnd_ScriptAbort::On(const Msg::Command &msg)
{
   if(msg.iID()==IDOK)
   {
      Scripter::GetInstance().StopRunningScript();
      return msg.Success();
   }

   if(msg.wndCtl()==*m_pbtMinimize)
   {
      Show(SW_MINIMIZE);
      return msg.Success();
   }

   return msg.Success();
}

LRESULT Wnd_ScriptAbort::On(const Msg::_State &msg)
{
   if(msg.fStart()) 
   {
      SetTimer(0, 3000); m_has_timer=true;
   }
   else
   {
      if(m_has_timer)
      {
         KillTimer(0); m_has_timer=false;
      }
      Show(SW_HIDE);
   }

   return msg.Success();
}

LRESULT Wnd_ScriptAbort::On(const Msg::Timer &msg)
{
   Assert(m_has_timer);
   KillTimer(0); m_has_timer=false;
   Show(SW_SHOWNOACTIVATE);
   return msg.Success();
}

// This class manages the watchdog thread and will create the abort window after 3 seconds
struct ScriptAbort
{
   ScriptAbort();
   ~ScriptAbort() noexcept;

   void Start();
   void Stop();

private:
   ScriptAbort(const ScriptAbort &);

   void Main(Kernel::Event &started);

   Wnd_ScriptAbort *mp_wnd_abort;

   Kernel::Thread m_thread;

   struct Startup
   {
      Startup(ScriptAbort *p_abort) noexcept : mp_abort(p_abort) { }
      Startup(const Startup &) = delete;

      Kernel::Event eventStart{false, false};
      ScriptAbort *mp_abort;
   };
};

ScriptAbort::ScriptAbort()
{
   Startup startup(this);
   m_thread=Kernel::Thread([&startup]() { startup.mp_abort->Main(startup.eventStart); });
   WaitForSingleObject(startup.eventStart, INFINITE);
}

ScriptAbort::~ScriptAbort()
{
   mp_wnd_abort->Close();
   m_thread.Join();
}

void ScriptAbort::Main(Kernel::Event &started)
{
   DebugOnly(SetThreadDescription(GetCurrentThread(), L"ScriptAbort::Main"));

   mp_wnd_abort=new Wnd_ScriptAbort();
   started.Set();

   MSG msg;
   while(GetMessage(&msg, nullptr, 0, 0))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}

void ScriptAbort::Start()
{
   Msg::_State(true).Post(*mp_wnd_abort);
}

void ScriptAbort::Stop()
{
   Msg::_State(false).Post(*mp_wnd_abort);
}

namespace OM
{

void DisplayScriptingEngines(Controls::ComboBox &list)
{
   // get the component category manager for this machine
   CntPtrTo<ICatInformation> pci;
   if(FAILED(pci.CoCreateInstance(CLSID_StdComponentCategoriesMgr, CLSCTX_INPROC_SERVER)))
      return;

   // get the list of parseable script engines
   CATID rgcatidImpl[] = { CATID_ActiveScript };

   CntPtrTo<IEnumCLSID> pec;
   if(FAILED(pci->EnumClassesOfCategories(_countof(rgcatidImpl), rgcatidImpl, 0, nullptr, pec.Address())))
      return;

   while(true)
   {
      CLSID rgclsid[16];
      ULONG cActual;
      HRESULT hr = pec->Next(_countof(rgclsid), rgclsid, &cActual);
      if(FAILED(hr) || cActual==0)
         break;
      for(ULONG i=0; i<cActual; i++)
      {
         CoTaskHolder<OLECHAR> pProgID;
         if(SUCCEEDED(ProgIDFromCLSID(rgclsid[i], pProgID.Address())))
            list.AddString(UTF8(WzToString(pProgID)));
      }
   }
}

//
// Beip
//

struct Beip : public Dispatch<IBeip>
{
   Beip(IApp *p_app) : mp_app(p_app)
   {
   }

	STDMETHODIMP get_Window(IWindow_Main** retval) override { return ReturnRef(retval, mp_window); }
	STDMETHODIMP get_App(IApp** retval) override { return ReturnRef(retval, mp_app); }

   void SetWindow(IWindow_Main *pWindow) { mp_window=pWindow; }

//private:
   CntPtrTo<IApp> mp_app;
   CntPtrTo<IWindow_Main> mp_window;
};

//
// ActiveScriptSite
//

struct ActiveScriptSite : General::Unknown<IActiveScriptSite, IActiveScriptSiteWindow, IActiveScriptSiteDebug>
{
   ActiveScriptSite(Scripter &scripter, ScriptAbort *pAbort)
   : m_scripter(scripter), m_pAbort(pAbort)
   {
      if(m_scripter.m_fDebug)
      {
         m_ppdm.CoCreateInstance(CLSID_ProcessDebugManager, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER);
         if(m_ppdm)
         {
            m_ppdm->CreateApplication(m_pda.Address()); 
            m_pda->SetName(WIDEN(gd_pcTitle));
            m_ppdm->AddApplication(m_pda, &m_dwCookie); 
         }
      }
   }

   ~ActiveScriptSite()
   {
      if(m_ppdm)
         m_ppdm->RemoveApplication(m_dwCookie);
   }

   void SetBeip(IDispatch *pdispBeip) { m_pdispBeip=pdispBeip; }

   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override
   {
      if(riid==IID_IActiveScriptSiteDebug && m_pda)
      {
         *ppvObj=static_cast<IActiveScriptSiteDebug *>(this);
         AddRef();
         return S_OK;
      }

      return TQueryInterface(riid, ppvObj);
   }

   virtual HRESULT STDMETHODCALLTYPE GetLCID(LCID *plcid) override
   {
      *plcid = 9; return S_OK; 
   }

   virtual HRESULT STDMETHODCALLTYPE GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask,
      IUnknown **ppiunkItem, ITypeInfo **ppti) override
   {
      // IActiveScriptSite methods
      HRESULT hr = E_FAIL;
      if(dwReturnMask & SCRIPTINFO_IUNKNOWN)
         *ppiunkItem=0;
      if(dwReturnMask & SCRIPTINFO_ITYPEINFO)
         *ppti=0;

      // see if top-level name is "beip"
      if(Strings::Compare(ConstWString(pstrName, Strings::Length(pstrName)), L"beip") == 0)
      {
         if(dwReturnMask & SCRIPTINFO_IUNKNOWN)
            if(*ppiunkItem = m_pdispBeip)
            {
               (*ppiunkItem)->AddRef();
               hr = S_OK;
            }
             
         if(dwReturnMask & SCRIPTINFO_ITYPEINFO)
         {
            Assert(false);
            hr=E_FAIL;
         }
      }
      return hr;
   }

   virtual HRESULT STDMETHODCALLTYPE GetDocVersionString(BSTR *pbstrVersion) override
   {
      return E_NOTIMPL;
   }

   virtual HRESULT STDMETHODCALLTYPE OnScriptTerminate(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo) override
   {
      return S_OK;
   }

   virtual HRESULT STDMETHODCALLTYPE OnStateChange(SCRIPTSTATE ssScriptState) override
   {
      return S_OK;
   }

   virtual HRESULT STDMETHODCALLTYPE OnScriptError(IActiveScriptError *pscripterror) override
   {
      DWORD dwCookie;
      LONG nChar;
      ULONG nLine;

      EXCEPINFO ei{};
      pscripterror->GetSourcePosition(&dwCookie, &nLine, &nChar);
      pscripterror->GetExceptionInfo(&ei);

      OwnedBSTR bstrCode;
      pscripterror->GetSourceLineText(bstrCode.Address());

      FixedStringBuilder<1024> string("<icon error> <font color='red'>Error: </font>", WzToString(ei.bstrSource), " <font color='lime'>[Line: ", nLine, " Char:", (DWORD)nChar, "]</font> <font color='teal'>");
      if(ei.bstrDescription)
         string(WzToString(ei.bstrDescription));
      else
      {
         string("HRESULT:", Strings::Hex32(ei.scode, 8));
         if(ei.scode==E_ZOMBIE)
            string(" Likely that script tried to access a zombied object");
      }

      if(bstrCode)
         string("</font> <font color='gray'>\"", WzToString(bstrCode), '\"');

      ConsoleHTML(string);
      ConsoleSetFocus();

      SysFreeString(ei.bstrSource);
      SysFreeString(ei.bstrDescription);
      SysFreeString(ei.bstrHelpFile);
      m_scripter.m_pas->SetScriptState(SCRIPTSTATE_STARTED);

      return S_OK;
   }

   virtual HRESULT STDMETHODCALLTYPE OnEnterScript() override
   {
      return S_OK;
   }

   virtual HRESULT STDMETHODCALLTYPE OnLeaveScript() override
   {
      return S_OK;
   }

   // IActiveScriptSiteWindow
   virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd) override
   {
      *phwnd=GetDesktopWindow();
      return S_OK;
   }

   virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable) override
   {
      return S_OK;
   }

   // IActiveScriptSiteDebug
   virtual HRESULT STDMETHODCALLTYPE GetDocumentContextFromPosition( 
      DWORD dwSourceContext, ULONG uCharacterOffset, ULONG uNumChars, IDebugDocumentContext **ppsc) override
   {
      return E_FAIL;
   }

   virtual HRESULT STDMETHODCALLTYPE GetApplication(IDebugApplication **ppda) override
   {
      *ppda=m_pda; (*ppda)->AddRef();
      return S_OK;
   }

   virtual HRESULT STDMETHODCALLTYPE GetRootApplicationNode(IDebugApplicationNode **ppdanRoot) override
   {
      return m_pda->GetRootNode(ppdanRoot);
   }

   virtual HRESULT STDMETHODCALLTYPE OnScriptErrorDebug(IActiveScriptErrorDebug *pErrorDebug,
      BOOL *pfEnterDebugger, BOOL *pfCallOnScriptErrorWhenContinuing) override
   {
      *pfEnterDebugger=TRUE;
      *pfCallOnScriptErrorWhenContinuing=TRUE;
      return S_OK;
   }

private:

   Scripter &m_scripter;

   CntPtrTo<IDispatch> m_pdispBeip;
   CntPtrTo<IProcessDebugManager> m_ppdm;
   CntPtrTo<IDebugApplication> m_pda;
   DWORD m_dwCookie;

   ScriptAbort *m_pAbort;
};

};

// *
// * Scripter
// *

#ifdef CHAKRA

struct Chakra
{
   static JsPropertyIdRef GetPropertyIdFromName(const wchar_t *wzName)
   {
      JsPropertyIdRef ref;
      AssertReturned<JsNoError>()==JsGetPropertyIdFromName(wzName, &ref);
      return ref;
   }

   static JsValueRef VariantToValue(OM::Variant &var)
   {
      JsValueRef ref;
      AssertReturned<JsNoError>()==JsVariantToValue(&var, &ref);
      return ref;
   }

   static OM::Variant ValueToVariant(JsValueRef ref)
   {
      OM::Variant variant;
      AssertReturned<JsNoError>()==JsValueToVariant(ref, &variant);
      return variant;
   }

   static JsValueRef GetProperty(JsValueRef object, JsPropertyIdRef propertyId)
   {
      JsValueRef ref;
	   AssertReturned<JsNoError>()==JsGetProperty(object, propertyId, &ref);
      return ref;
   }

   static ConstWString ValueToString(JsValueRef value)
   {
      const wchar_t *string;
      size_t length;
      AssertReturned<JsNoError>()==JsStringToPointer(value, &string, &length);

      return ConstWString(string, unsigned(length));
   };

   static JsValueRef GetAndClearException()
   {
      JsValueRef exception;
      AssertReturned<JsNoError>()==JsGetAndClearException(&exception);
      return exception;
   }

   Chakra(OM::Beip *pBeip) : mp_beip(pBeip)
   {
      AssertReturned<JsNoError>()==JsCreateRuntime(JsRuntimeAttributeAllowScriptInterrupt, nullptr, &m_handle);
      AssertReturned<JsNoError>()==JsCreateContext(m_handle, &m_context);
      AssertReturned<JsNoError>()==JsSetCurrentContext(m_context);
      if(g_ppropGlobal->fScriptDebug())
         AssertReturned<JsNoError>()==JsStartDebugging();

      AssertReturned<JsNoError>()==JsGetGlobalObject(&m_globalObject);
      auto variant_app=OM::Variant{mp_beip->mp_app};
      AssertReturned<JsNoError>()==JsSetProperty(m_globalObject, GetPropertyIdFromName(L"app"), VariantToValue(variant_app), true);

      AssertReturned<JsNoError>()==JsProjectWinRTNamespace(L"Windows");
   }

   ~Chakra()
   {
      AssertReturned<JsNoError>()==JsSetCurrentContext(nullptr);
      AssertReturned<JsNoError>()==JsCollectGarbage(m_handle);
      AssertReturned<JsNoError>()==JsDisposeRuntime(m_handle);
   }

   void Abort()
   {
      AssertReturned<JsNoError>()==JsDisableRuntimeExecution(m_handle);
   }

   void Run(LPCOLESTR pstrCode)
   {
      auto variant_window=OM::Variant{mp_beip->mp_window};
      AssertReturned<JsNoError>()==JsSetProperty(m_globalObject, GetPropertyIdFromName(L"window"), VariantToValue(variant_window), true);

      auto error=JsRunScript(pstrCode, sourceContext++, L"", nullptr);
      switch(error)
      {
         case JsNoError:
            break;

         case JsErrorScriptTerminated:
            JsEnableRuntimeExecution(m_handle);
            ConsoleHTML("<icon error> <font color='red'>Running script aborted by user</font>");
            break;

         default:
         {
            auto exception=GetAndClearException();
            ConstWString message=ValueToString(GetProperty(exception, GetPropertyIdFromName(L"message")));

            FixedStringBuilder<1024> string("<icon error> <font color='red'>Error: </font>", message, "<font color='teal'>");
            ConsoleHTML(string);
            break;
         }
      }

      JsValueRef wasDeleted;
      AssertReturned<JsNoError>()==JsDeleteProperty(m_globalObject, GetPropertyIdFromName(L"window"), true, &wasDeleted);
   }

   OM::Variant GetSymbol(const wchar_t *symbol)
   {
      JsValueRef object;
      AssertReturned<JsNoError>()==JsGetProperty(m_globalObject, GetPropertyIdFromName(symbol), &object);
      return ValueToVariant(object);
   }

   OM::Beip *mp_beip;
   JsRuntimeHandle m_handle;
   JsContextRef m_context;
   JsValueRef m_globalObject;
   JsSourceContext sourceContext{};
};

#endif

Scripter::Scripter(ConstString string)
{
   mp_beip=MakeCounting<OM::Beip>(Automation::GetApp());
   mp_abort=MakeUnique<ScriptAbort>();

#ifdef CHAKRA
   if(string=="JScript" && LoadLibrary(L"Chakra.dll")!=nullptr)
   {
      mp_chakra=MakeUnique<Chakra>(mp_beip);
      return;
   }
#endif

   if(string=="XML")
      throw std::exception{}; // Bail out on XML as it just doesn't work

   CLSID clsid;
   HRESULT hr = CLSIDFromProgID(OwnedBSTR(string), &clsid);
   if(SUCCEEDED(hr))
      m_pas.CoCreateInstance(clsid, CLSCTX_ALL);
   if(!m_pas)
      throw std::exception{};

   m_pas->QueryInterface(m_pasp.Address());
   if(!m_pasp)
      throw std::exception{};

   m_pasp->InitNew();
   mp_site=MakeCounting<OM::ActiveScriptSite>(*this, mp_abort);
   mp_site->SetBeip(mp_beip);
   m_pas->SetScriptSite(mp_site);

   m_pas->AddNamedItem(OLESTR("beip"), SCRIPTITEM_GLOBALMEMBERS);
   m_pas->SetScriptState(SCRIPTSTATE_STARTED);
}

Scripter::~Scripter()
{
   OM::ClearHooks();

   if(m_pas)
      m_pas->SetScriptState(SCRIPTSTATE_CLOSED);
}

struct ServiceProvider : General::Unknown<IServiceProvider, ICanHandleException>
{
   STDMETHODIMP QueryInterface(const IID &id, void **ppvObj) override
   {
      return TQueryInterface(id, ppvObj);
   }

   STDMETHODIMP QueryService(_In_ REFGUID guidService, _In_ REFIID riid, _Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override
   {
      return E_NOTIMPL;
   }

   STDMETHODIMP CanHandleException(EXCEPINFO *p_info, VARIANT *p_var) override
   {
      FixedStringBuilder<1024> string("<icon error> <font color='red'>Error: </font>", ConstWString(p_info->bstrSource), " : ", "<font color='teal'>", ConstWString(p_info->bstrDescription));
      ConsoleHTML(string);
      return S_OK;
   }
};

CntPtrTo<ServiceProvider> gp_service_provider;

IServiceProvider *GetServiceProvider()
{
   if(!gp_service_provider)
      gp_service_provider=MakeCounting<ServiceProvider>();

   return gp_service_provider;
}

void Scripter::Invoke(LPOLESTR pstrName, DISPPARAMS &params)
{
#ifdef CHAKRA
   if(mp_chakra)
   {
      auto variant=mp_chakra->GetSymbol(pstrName);
      if(!variant.Is<IDispatch*>())
         return;
      CntPtrTo<IDispatch> pDispatch(variant.Get<IDispatch*>());
      CntPtrTo<IDispatchEx> pDispatchEx; pDispatch->QueryInterface(pDispatchEx.Address());

      if(pDispatchEx)
         pDispatchEx->InvokeEx(DISPID_VALUE, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, GetServiceProvider());
      else
         pDispatch->Invoke(DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, 0);
      return;
   }
#endif

   CntPtrTo<IDispatch> pDispatch; m_pas->GetScriptDispatch(nullptr, pDispatch.Address());

   DISPID dispid;
   pDispatch->GetIDsOfNames(IID_NULL, &pstrName, 1, LOCALE_USER_DEFAULT, &dispid );
   pDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, 0, 0);
}

void Scripter::Run(LPCOLESTR pstrCode)
{
   if(!pstrCode) // Empty strings will crash the script engine, so exit early
      return;

   mp_abort->Start();
#ifdef CHAKRA
   if(mp_chakra)
      mp_chakra->Run(pstrCode);
   else
#endif
      m_pasp->ParseScriptText(pstrCode, nullptr, nullptr, nullptr, 0, 0, SCRIPTTEXT_ISVISIBLE, nullptr, nullptr);
   mp_abort->Stop();
}

bool Scripter::RunFile(ConstString fileName)
{
   auto bstr=OM::ReadFileAsBSTR(fileName);
   if(!bstr)
      return false;

   Run(bstr);
   return true;
}

void Scripter::StopRunningScript()
{
#ifdef CHAKRA
   if(mp_chakra)
      mp_chakra->Abort();
   else
#endif
      m_pas->InterruptScriptThread(SCRIPTTHREADID_ALL, nullptr, 0); //SCRIPTINTERRUPT_DEBUG);
}

void Scripter::SetWindow(OM::MainWindow *pWindow)
{
   mp_beip->SetWindow(pWindow);
}
