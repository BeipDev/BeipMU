#include "Main.h"
#include "WindowEvents.h"

#include "OM_Help.h"
#include "Window_Events_OM.h"

namespace OM
{

#define ZOMBIECHECK if(!m_pEvents) return E_ZOMBIE;

Window_Events::Window_Events(WindowEvents *pEvents)
:  m_pEvents(pEvents)
{
}

void Window_Events::On(const Windows::Event_MouseMove &event)
{
   m_hookMouseMove(m_hookMouseMove.var, event.pos().y, event.pos().x);
}

void Window_Events::On(const Windows::Event_Close &event)
{
   m_hookClose.Call();
}

void Window_Events::On(const Windows::Event_Key &event)
{
   m_hookKey(m_hookKey.var, event.Key());
}

STDMETHODIMP Window_Events::SetOnClose(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<Windows::Event_Close>(this, m_hookClose, *m_pEvents, pDisp, var);
}

STDMETHODIMP Window_Events::SetOnMouseMove(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<Windows::Event_MouseMove>(this, m_hookMouseMove, *m_pEvents, pDisp, var);
}

STDMETHODIMP Window_Events::SetOnKey(IDispatch *pDisp, VARIANT var)
{
   ZOMBIECHECK
   return ManageHook<Windows::Event_Key>(this, m_hookKey, *m_pEvents, pDisp, var);
}

};
