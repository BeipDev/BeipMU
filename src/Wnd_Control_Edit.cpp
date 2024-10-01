#include "Main.h"

#include "Wnd_Control_Edit.h"

namespace OM
{

Window_Control_Edit::Window_Control_Edit(Controls::Edit wnd) : m_edit(wnd)
{
}

HRESULT Window_Control_Edit::Set(BSTR bstr)
{
   m_edit.SetText(BSTRToLStr(bstr));
   return S_OK;
}

HRESULT Window_Control_Edit::Get(BSTR *retval)
{
   OwnedString string{m_edit.GetText()};

   *retval=OwnedBSTR(string).Extract();
   return S_OK;
}

HRESULT Window_Control_Edit::SetSel(int start, int end)
{
   SendMessage(m_edit, EM_SETSEL, (WPARAM)(start), (LPARAM)(end));
   return S_OK;
}

HRESULT Window_Control_Edit::GetSelStart(int *retval)
{
   DWORD dwStart, dwEnd;

   SendMessage(m_edit, EM_GETSEL, (WPARAM)(&dwStart), (LPARAM)(&dwEnd));
   *retval=dwStart;
   return S_OK;
}

HRESULT Window_Control_Edit::GetSelEnd(int *retval)
{
   DWORD dwStart, dwEnd;

   SendMessage(m_edit, EM_GETSEL, (WPARAM)(&dwStart), (LPARAM)(&dwEnd));
   *retval=dwEnd;
   return S_OK;
}

HRESULT Window_Control_Edit::get_Length(int *retval)
{
   *retval=m_edit.GetTextLength(); return S_OK;
}

#if 0
STDMETHODIMP Window_Control_Edit::get_HWND(void **hwnd)
{
   *hwnd=(HWND)m_edit;
   return S_OK;
}
#endif

};
