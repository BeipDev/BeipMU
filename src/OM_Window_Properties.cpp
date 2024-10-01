//
// OM Window Properties
//

#include "Main.h"
#include "WindowEvents.h"
#include "OM_Help.h"
#include "OM_Window_Properties.h"

namespace OM
{
#define ZOMBIECHECK if(!m_window.IsValid()) return E_ZOMBIE;

Window_Properties::Window_Properties(Window window, Events::SenderOf<Events::Event_Deleted> &deleted)
:  m_window(window)
{
   AttachTo<Events::Event_Deleted>(deleted);
}

STDMETHODIMP Window_Properties::put_Title(BSTR bstr)
{
   ZOMBIECHECK
   m_window.SetText(BSTRToLStr(bstr));
   return S_OK;
}

STDMETHODIMP Window_Properties::get_Title(BSTR *bstr)
{
   ZOMBIECHECK
   OwnedString lstrTitle{m_window.GetText()}; *bstr=LStrToBSTR(lstrTitle);
   return S_OK;
}

STDMETHODIMP Window_Properties::get_HWND(__int3264 *hwnd)
{
   ZOMBIECHECK
   *hwnd=(__int3264)(HWND)m_window;
   return S_OK;
}

}