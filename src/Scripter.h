//
// Scripter
//

interface IActiveScript;
interface IActiveScriptSite;
interface IActiveScriptParse;
interface IActiveScriptDebug;
interface IProcessDebugManager;
interface IDebugApplication;
interface IDebugDocumentHelper;
namespace OM
{
struct ActiveScriptSite;
struct Variant;
struct Beip;
struct MainWindow;
};

struct Wnd_ScriptDebug;
struct ScriptAbort;

#define CHAKRA
#ifdef CHAKRA
struct Chakra;
#endif

struct Scripter : Singleton<Scripter>
{
   Scripter(ConstString string);
   ~Scripter() noexcept;

   void Run(LPCOLESTR pstrCode);
   bool RunFile(ConstString fileName);
   void StopRunningScript();

   void SetWindow(OM::MainWindow *pWindow);

   template<typename... Params> void Call(LPOLESTR wzName, Params&&... params)
   {
      OM::Variant var[]={ std::forward<Params>(params)... };
      DISPPARAMS dp={ var, 0, _countof(var), 0 };
      Invoke(wzName, dp);
   }

private:

   void Invoke(LPOLESTR pstrName, DISPPARAMS &params);

   CntPtrTo<IActiveScript> m_pas;
   CntPtrTo<IActiveScriptParse> m_pasp;
   CntPtrTo<OM::Beip>           mp_beip;

#ifdef CHAKRA
   UniquePtr<Chakra> mp_chakra;
#endif

   UniquePtr<ScriptAbort> mp_abort;

   bool m_fDebug{g_ppropGlobal->fScriptDebug()};

   // Script Debugging
//   CntPtrTo<IActiveScriptDebug>   m_pasd;
//   CntPtrTo<IProcessDebugManager> m_ppdm;
//   CntPtrTo<IDebugApplication>    m_pda;
//   CntPtrTo<IDebugDocumentHelper> m_ddh;
   CntPtrTo<OM::ActiveScriptSite> mp_site;
   friend struct OM::ActiveScriptSite;
};
