#include "OM_Help.h"

namespace OM
{

interface ITextWindow;
interface IConnection;

struct Docking : Dispatch<IDocking>
{
   Docking(Wnd_Docking *pWnd);

   // IDocking
   STDMETHODIMP Dock(Side side); // Docks the window to a particular side

private:
   NotifiedPtrTo<Wnd_Docking> m_pWnd;
};

struct SpawnTabs
 : DLNode<SpawnTabs>,
   Dispatch<IWindow_SpawnTabs>,
   Events::ReceiversOf<SpawnTabs, SpawnTabsWindow::Event_Activate, Events::Event_Deleted>
{
   SpawnTabs(MainWindow &main_window, SpawnTabsWindow &window);

   STDMETHODIMP SetOnTabActivate(IDispatch *pDisp, VARIANT var) override;
   void On(SpawnTabsWindow::Event_Activate &event);
   void On(Events::Event_Deleted &event);

   MainWindow &m_main_window;
   SpawnTabsWindow *mp_window;

private:
   HookVariant m_hook_tab_activate;
};

struct MainWindow
 : Dispatch<IWindow_Main>,
   Events::ReceiversOf<MainWindow, Wnd_Main::Event_Command, Wnd_Main::Event_Activate, Wnd_Main::Event_Close, Wnd_Main::Event_Key>
{
   MainWindow(Wnd_Main *pWnd_Main);
   ~MainWindow() noexcept;
   void Destroyed() { mp_wnd_main=nullptr; }

   USE_INHERITED_UNKNOWN(IWindow_Main)

   // IUnknown
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;

   // IWindow methods
   STDMETHODIMP get_Output(IWindow_Text **retval) override
   {
      mp_text_window_output->AddRef(); *retval=mp_text_window_output;
      return S_OK;
   }

   STDMETHODIMP get_History(IWindow_Text **retval) override
   {
      mp_text_window_history->AddRef(); *retval=mp_text_window_history;
      return S_OK;
   }

   STDMETHODIMP get_Connection(IConnection **retval) override
   {
      mp_connection->AddRef(); *retval=mp_connection;
      return S_OK;
   }

   STDMETHODIMP get_Input(IWindow_Input **retval) override
   {
      mp_input->AddRef(); *retval=mp_input;
      return S_OK;
   }

   STDMETHODIMP GetInput(BSTR title, IWindow_Input **retval);
   STDMETHODIMP GetSpawnTabs(BSTR title, IWindow_SpawnTabs **retval);

   STDMETHODIMP get_UserData(VARIANT *pVar) override { VariantCopy(pVar, &m_varUserData); return S_OK; }
   void STDMETHODCALLTYPE put_UserData(VARIANT var) override { m_varUserData=var; }

   STDMETHODIMP Close() override;

   STDMETHODIMP SetOnCommand(IDispatch *pDisp, VARIANT var) override;
   STDMETHODIMP SetOnActivate(IDispatch *pDisp, VARIANT var) override;
   STDMETHODIMP SetOnClose(IDispatch *pDisp, VARIANT var) override;

   STDMETHODIMP Run(BSTR bstr) override;
   STDMETHODIMP RunFile(BSTR bstr) override;

   STDMETHODIMP CreateDialogConnect() override;

   STDMETHODIMP put_TitlePrefix(BSTR bstr) override;
   STDMETHODIMP get_TitlePrefix(BSTR *retval) override;
   STDMETHODIMP get_Title(BSTR *retval) override;

   STDMETHODIMP GetVariable(BSTR name, BSTR *value) override;
   STDMETHODIMP SetVariable(BSTR name, BSTR value) override;
   STDMETHODIMP DeleteVariable(BSTR name) override;

   // Events
   void On(Wnd_Main::Event_Command &event);
   void On(Wnd_Main::Event_Activate &event);
   void On(Wnd_Main::Event_Close &event);
   void On(Wnd_Main::Event_Key &event);
   void On(Events::Event_Deleted &event, SpawnTabs &tab);

private:
   CntPtrTo<IWindow_Text> mp_text_window_output;
   CntPtrTo<IWindow_Text> mp_text_window_history;
   CntPtrTo<IConnection> mp_connection;
   CntPtrTo<IWindow_Input> mp_input;
   Wnd_Main *mp_wnd_main;

   VariantNode m_varUserData;

   HookVariant m_hookCommand;
   HookVariant m_hookActivate;
   HookVariant m_hookDeactivate;
   HookVariant m_hookClose;

   Collection<CntPtrTo<SpawnTabs>> m_spawn_tabs;
};

struct Windows : Dispatch<IWindows>
{
   // IWindows
   STDMETHODIMP get_Item(VARIANT var, IWindow_Main **retval);
   STDMETHODIMP get_Count(long *retval);
};

};
