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
#define ZOMBIECHECK if(!mp_window) return E_ZOMBIE;

SpawnTabs::SpawnTabs(MainWindow &main_window, SpawnTabsWindow &window)
   : m_main_window{main_window}, mp_window{&window}
{
   ReceiverOf<Events::Event_Deleted>::AttachTo(*mp_window);
}

HRESULT SpawnTabs::SetOnTabActivate(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<SpawnTabsWindow::Event_Activate>(this, m_hook_tab_activate, *mp_window, pDisp, var);
}

void SpawnTabs::On(SpawnTabsWindow::Event_Activate &event)
{
   if(m_hook_tab_activate)
      m_hook_tab_activate(m_hook_tab_activate.var, event.tab);
}

void SpawnTabs::On(Events::Event_Deleted &event)
{
   m_main_window.On(event, *this);
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

HRESULT MainWindow::GetInput(BSTR title, IWindow_Input **retval)
{
   ZOMBIECHECK
   auto p_input=m_pWnd_Main->FindInputWindow(BSTRToLStr(title));
   if(!p_input)
      return E_INVALIDARG;
   *retval=new Window_Input(*p_input); (*retval)->AddRef();
   return S_OK;
}

HRESULT MainWindow::GetSpawnTabs(BSTR title16, IWindow_SpawnTabs **retval)
{
   ZOMBIECHECK
   auto title=BSTRToLStr(title16);
   for(SpawnTabs *p_window : m_spawn_tabs)
   {
      if(p_window->mp_window->m_title==title)
      {
         *retval=p_window; (*retval)->AddRef();
         return S_OK;
      }
   }

   auto *p_window=m_pWnd_Main->FindSpawnTabsWindow(title);
   if(!p_window)
      return E_FAIL;

   auto p_spawn_tabs=MakeCounting<SpawnTabs>(*this, *p_window);
   m_spawn_tabs.Push(p_spawn_tabs);
   *retval=p_spawn_tabs; (*retval)->AddRef();
   return S_OK;
}

void MainWindow::On(Events::Event_Deleted &event, SpawnTabs &tabs)
{
   m_spawn_tabs.FindAndUnsortedDelete(&tabs);
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
