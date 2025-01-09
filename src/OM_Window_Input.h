#include "OM_Help.h"
struct InputControl;

namespace OM
{

struct Window_Input : Dispatch<IWindow_Input>
{
   Window_Input(InputControl &input);

   STDMETHODIMP Set(BSTR bstr) override;
   STDMETHODIMP Get(BSTR *retval) override;

   STDMETHODIMP SetSel(int start, int end) override;
   STDMETHODIMP GetSelStart(int *retval) override;
   STDMETHODIMP GetSelEnd(int *retval) override;

   STDMETHODIMP get_Length(int *retval) override;

   STDMETHODIMP get_Prefix(BSTR *retval) override;
   STDMETHODIMP put_Prefix(BSTR bstr) override;
   STDMETHODIMP get_Title(BSTR *retval) override;
   STDMETHODIMP put_Title(BSTR bstr) override;

private:
   NotifiedPtrTo<InputControl> mp_input;
};

};
