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

class MainWindow
:  public Dispatch<IWindow_Main>,
   public CntReceiverOf<MainWindow, Wnd_Main::Event_Command>,
   public CntReceiverOf<MainWindow, Wnd_Main::Event_Activate>,
   public CntReceiverOf<MainWindow, Wnd_Main::Event_Close>,
   public CntReceiverOf<MainWindow, Wnd_Main::Event_Key>
{
public:
   MainWindow(Wnd_Main *pWnd_Main);
   ~MainWindow() noexcept;
   void Destroyed() { m_pWnd_Main=nullptr; }

   USE_INHERITED_UNKNOWN(IWindow_Main)

   // IUnknown
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;

   // IWindow methods
   STDMETHODIMP get_Output(IWindow_Text **retval) override
   {
      m_pTextWindowOutput->AddRef(); *retval=m_pTextWindowOutput;
      return S_OK;
   }

   STDMETHODIMP get_History(IWindow_Text **retval) override
   {
      m_pTextWindowHistory->AddRef(); *retval=m_pTextWindowHistory;
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

   STDMETHODIMP SetOnSpawnTabActivate(BSTR title, IDispatch *pDisp, VARIANT var) override;

//   STDMETHODIMP get_HWND(long *hwnd);
//   STDMETHODIMP MakeDocking(__int3264 hwnd, IDocking **retval) override;

   // Events
   void On(Wnd_Main::Event_Command &event);
   void On(Wnd_Main::Event_Activate &event);
   void On(Wnd_Main::Event_Close &event);
   void On(Wnd_Main::Event_Key &event);

private:
   CntPtrTo<IWindow_Text> m_pTextWindowOutput;
   CntPtrTo<IWindow_Text> m_pTextWindowHistory;
   CntPtrTo<IConnection> mp_connection;
   CntPtrTo<IWindow_Input> mp_input;
   Wnd_Main *m_pWnd_Main;

   VariantNode m_varUserData;

   HookVariant m_hookCommand;
   HookVariant m_hookActivate;
   HookVariant m_hookDeactivate;
   HookVariant m_hookClose;

   struct SpawnTabActivate
    : DLNode<SpawnTabActivate>,
      CntReceiverOf<SpawnTabActivate, SpawnTabsWindow::Event_Activate>
   {
      void On(SpawnTabsWindow::Event_Activate &event);

      OwnedString m_title;
      HookVariant m_hook;
   };

   OwnedDLNodeList<SpawnTabActivate> m_spawn_tab_activates;
};

struct Windows : Dispatch<IWindows>
{
   // IWindows
   STDMETHODIMP get_Item(VARIANT var, IWindow_Main **retval);
   STDMETHODIMP get_Count(long *retval);
};

};
