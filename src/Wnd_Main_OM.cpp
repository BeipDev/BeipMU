#include "Main.h"

#include "Connection.h"

#include "Wnd_Main.h"
#include "Scripter.h"

#include <activscp.h>

#include "Connection_OM.h"
#include "Wnd_Text_OM.h"
#include "Wnd_Main_OM.h"
#include "Wnd_Control_Edit.h"

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
   m_pInput=new Window_Control_Edit(pWnd_Main->m_input);
}

MainWindow::~MainWindow()
{
}

STDMETHODIMP MainWindow::QueryInterface(REFIID riid, void **ppvObj)
{
   return TQueryInterface(riid, ppvObj);
}

void MainWindow::Advised()
{
   CntReceiverOf<MainWindow, Wnd_Main::Event_Activate>::AttachTo(*m_pWnd_Main);
   CntReceiverOf<MainWindow, Wnd_Main::Event_Command>::AttachTo(*m_pWnd_Main);
   CntReceiverOf<MainWindow, Wnd_Main::Event_Close>::AttachTo(*m_pWnd_Main);
   CntReceiverOf<MainWindow, Wnd_Main::Event_Key>::AttachTo(*m_pWnd_Main);
}

void MainWindow::Unadvised()
{
   CntReceiverOf<MainWindow, Wnd_Main::Event_Activate>::Detach();
   CntReceiverOf<MainWindow, Wnd_Main::Event_Command>::Detach();
   CntReceiverOf<MainWindow, Wnd_Main::Event_Close>::Detach();
   CntReceiverOf<MainWindow, Wnd_Main::Event_Key>::Detach();
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
   if(fAdvised())
   {
      VariantBool fHandled(false);
      Variant var[]={ &fHandled, event.GetParams(), event.GetCommand() };
      DISPPARAMS dp = { var, 0, _countof(var), 0 };
      MultipleInvoke(1, dp, &fHandled);
      if(fHandled)
      {
         event.Stop(true); return;
      }
   }

   if(m_hookCommand)
      event.Stop(m_hookCommand.CallWithResult(m_hookCommand.var, event.GetParams(), event.GetCommand())==true);
}

void MainWindow::On(Wnd_Main::Event_Activate &event)
{
   if(fAdvised())
   {
      Variant var[]={ event.fActivated() };
      DISPPARAMS dp = { var, 0, _countof(var), 0 };
      MultipleInvoke(2, dp);
   }

   if(m_hookActivate)
      m_hookActivate(m_hookActivate.var, event.fActivated());
}


void MainWindow::On(Wnd_Main::Event_Close &event)
{
   if(fAdvised())
      MultipleInvoke(3);

   if(m_hookClose)
      m_hookClose.Call();
}

void MainWindow::On(Wnd_Main::Event_Key &event)
{
   if(fAdvised())
   {
      VariantBool fHandled(false);
      Variant var[]={ &fHandled, static_cast<int32>(event.GetVKey()) };
      DISPPARAMS dp = { var, 0, _countof(var), 0 };
      MultipleInvoke(4, dp, &fHandled);
      if(fHandled)
      {
         event.Stop(true); return;
      }
   }
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
