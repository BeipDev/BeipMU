#include "OM_Help.h"

namespace OM
{

struct Window_Control_Edit : Dispatch<IWindow_Control_Edit>
{
   Window_Control_Edit(Controls::Edit wnd);

   STDMETHODIMP Set(BSTR bstr) override;
   STDMETHODIMP Get(BSTR *retval) override;

   STDMETHODIMP SetSel(int start, int end) override;
   STDMETHODIMP GetSelStart(int *retval) override;
   STDMETHODIMP GetSelEnd(int *retval) override;

   STDMETHODIMP get_Length(int *retval) override;

//   STDMETHODIMP get_HWND(void **hwnd) override;

private:
   Controls::Edit m_edit;
};

};
