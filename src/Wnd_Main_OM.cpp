#include "Main.h"

#include "Connection.h"

#include "Wnd_Main.h"
#include "Scripter.h"

#include <activscp.h>

#include "Connection_OM.h"
#include "Wnd_Text_OM.h"
#include "Wnd_Main_OM.h"
#include "OM_Window_Input.h"

namespace OM
{

#define ZOMBIECHECK if(!m_pWnd) return E_ZOMBIE;

Docking::Docking(Wnd_Docking *pWnd)
:  m_pWnd(pWnd)
{
}

HRESULT Docking::Dock(Side side)
{
   ZOMBIECHECK
   if(side<0 || side>Side_Bottom)
      return E_INVALIDARG;

   m_pWnd->Dock((::Docking::Side)side);
   return S_OK;
}

#undef ZOMBIECHECK
#define ZOMBIECHECK if(!m_pWnd_Main) return E_ZOMBIE;

MainWindow::MainWindow(Wnd_Main *pWnd_Main)
:  m_pWnd_Main(pWnd_Main)
{
   m_pTextWindowOutput=new Window_Text(pWnd_Main->mp_wnd_text);
   m_pTextWindowHistory=new Window_Text(pWnd_Main->mp_wnd_text_history);
   mp_connection=new Connection(&pWnd_Main->GetConnection());
   mp_input=new Window_Input(pWnd_Main->m_input);
}

MainWindow::~MainWindow()
{
}

STDMETHODIMP MainWindow::QueryInterface(REFIID riid, void **ppvObj)
{
   return TQueryInterface(riid, ppvObj);
}

HRESULT MainWindow::Run(BSTR bstr)
{
   ZOMBIECHECK
   auto stringRun=WzToString(bstr);
   OwnedString string; string.Allocate(stringRun.Count()+2); // For CR/LF
   StringBuilder{string}(stringRun, CRLF);

   m_pWnd_Main->SendLines(string);
   return S_OK;
}

HRESULT MainWindow::RunFile(BSTR bstr)
{
   ZOMBIECHECK
   if(!m_pWnd_Main->GetScripter()->RunFile(BSTRToLStr(bstr)))
      m_pWnd_Main->mp_wnd_text->AddHTML("<font color='red'>Couldn't run script file");
   return S_OK;
}

HRESULT MainWindow::CreateDialogConnect()
{
   ZOMBIECHECK
   if(m_pWnd_Main->GetConnection().IsConnected())
      return E_FAIL;
   m_pWnd_Main->On(Msg::Command(ID_CONNECTION_CONNECT, 0, 0));
   return S_OK;
}

HRESULT MainWindow::put_TitlePrefix(BSTR bstr)
{
   ZOMBIECHECK
   m_pWnd_Main->m_title_prefix=BSTRToLStr(bstr);
   m_pWnd_Main->UpdateTitle();
   return S_OK;
}

HRESULT MainWindow::get_TitlePrefix(BSTR *retval)
{
   ZOMBIECHECK
   *retval=LStrToBSTR(m_pWnd_Main->m_title_prefix);
   return S_OK;
}

HRESULT MainWindow::get_Title(BSTR *retval)
{
   ZOMBIECHECK
   *retval=LStrToBSTR(m_pWnd_Main->m_title);
   return S_OK;
}

HRESULT MainWindow::GetVariable(BSTR name, BSTR *retval)
{
   ZOMBIECHECK
   auto index=m_pWnd_Main->m_variables.Find(BSTRToLStr(name));
   if(index==~0U)
      *retval=nullptr;
   else
      *retval=LStrToBSTR(m_pWnd_Main->m_variables[index].m_value);
   return S_OK;
}

HRESULT MainWindow::SetVariable(BSTR name, BSTR value)
{
   ZOMBIECHECK
   auto index=m_pWnd_Main->m_variables.Find(BSTRToLStr(name));
   if(index==~0U)
      m_pWnd_Main->AddVariable(BSTRToLStr(name), BSTRToLStr(value));
   else
      m_pWnd_Main->m_variables[index].m_value=BSTRToLStr(value);
   return S_OK;
}

HRESULT MainWindow::DeleteVariable(BSTR name)
{
   ZOMBIECHECK
   auto index=m_pWnd_Main->m_variables.Find(BSTRToLStr(name));
   if(index==~0U)
      return E_INVALIDARG;

   m_pWnd_Main->m_variables.UnsortedDelete(index);
   return S_OK;
}


#if 0
HRESULT MainWindow::get_HWND(long *hwnd)
{
   ZOMBIECHECK
   *hwnd=(long)(HWND)*m_pWnd_Main;
   return S_OK;
}

HRESULT MainWindow::MakeDocking(__int3264 _hwnd, IDocking **retval)
{
   ZOMBIECHECK
   HWND hwnd=reinterpret_cast<HWND>(_hwnd);
   if(!IsWindow(hwnd))
      return E_INVALIDARG;
   Wnd_Docking *pWnd=&m_pWnd_Main->CreateDocking(hwnd);
   *retval=new Docking(pWnd); (*retval)->AddRef();
   return S_OK;
}
#endif

HRESULT MainWindow::GetInput(BSTR title, IWindow_Input **retval)
{
   ZOMBIECHECK
   auto p_input=m_pWnd_Main->FindInputWindow(BSTRToLStr(title));
   if(!p_input)
      return E_INVALIDARG;
   *retval=new Window_Input(*p_input); (*retval)->AddRef();
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MainWindow::Close()
{
   ZOMBIECHECK
   m_pWnd_Main->Close();
   return S_OK;
}

HRESULT MainWindow::SetOnCommand(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<Wnd_Main::Event_Command>(this, m_hookCommand, *m_pWnd_Main, pDisp, var);
}

HRESULT MainWindow::SetOnActivate(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<Wnd_Main::Event_Activate>(this, m_hookActivate, *m_pWnd_Main, pDisp, var);
}

HRESULT MainWindow::SetOnClose(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<Wnd_Main::Event_Close>(this, m_hookClose, *m_pWnd_Main, pDisp, var);
}

HRESULT MainWindow::SetOnSpawnTabActivate(BSTR title16, IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   auto title=BSTRToLStr(title16);
   SpawnTabActivate *p_hook{};
   for(auto &hook : m_spawn_tab_activates)
   {
      if(hook.m_title==title)
      {
         p_hook=&hook;
         break;
      }
   }

   if(!p_hook)
   {
      p_hook=new SpawnTabActivate();
      p_hook->DLNode::Link(m_spawn_tab_activates.Prev());
      p_hook->m_title=title;
   }

   auto *p_window=m_pWnd_Main->FindSpawnTabsWindow(title);
   if(!p_window)
      return E_FAIL;

   return ManageHook<SpawnTabsWindow::Event_Activate>(p_hook, p_hook->m_hook, *p_window, pDisp, var);
}

void MainWindow::SpawnTabActivate::On(SpawnTabsWindow::Event_Activate &event)
{
   if(m_hook)
      m_hook(m_hook.var, event.tab);
}


void MainWindow::On(Wnd_Main::Event_Command &event)
{
   if(m_hookCommand)
      event.Stop(m_hookCommand.CallWithResult(m_hookCommand.var, event.GetParams(), event.GetCommand())==true);
}

void MainWindow::On(Wnd_Main::Event_Activate &event)
{
   if(m_hookActivate)
      m_hookActivate(m_hookActivate.var, event.fActivated());
}


void MainWindow::On(Wnd_Main::Event_Close &event)
{
   if(m_hookClose)
      m_hookClose.Call();
}

void MainWindow::On(Wnd_Main::Event_Key &event)
{
}

STDMETHODIMP Windows::get_Item(VARIANT var, IWindow_Main **retval)
{
   Variant v2;
   if(v2.Convert<int32>(var))
   {
      unsigned index=v2.Get<int32>();

      if(index>=Wnd_MDI::GetInstance().GetWindowCount())
         return E_INVALIDARG;

      return RefReturner(retval)(Wnd_MDI::GetInstance().GetWindow(index).GetDispatch());
   }

   return E_FAIL;
}

STDMETHODIMP Windows::get_Count(long *retval)
{
   *retval=Wnd_MDI::GetInstance().GetWindowCount(); return S_OK;
}

};
