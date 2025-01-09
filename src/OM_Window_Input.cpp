#include "Main.h"
#include "OM_Window_Input.h"
#include "Wnd_Input.h"

namespace OM
{
#define ZOMBIECHECK if(!mp_input) return E_ZOMBIE;

Window_Input::Window_Input(InputControl &input) : mp_input{&input}
{
}

HRESULT Window_Input::Set(BSTR bstr)
{
   ZOMBIECHECK
   mp_input->SetText(BSTRToLStr(bstr));
   return S_OK;
}

HRESULT Window_Input::Get(BSTR *retval)
{
   ZOMBIECHECK
   *retval=OwnedBSTR(mp_input->GetText()).Extract();
   return S_OK;
}

HRESULT Window_Input::SetSel(int start, int end)
{
   ZOMBIECHECK
   mp_input->SetSel(start, end);
   return S_OK;
}

HRESULT Window_Input::GetSelStart(int *retval)
{
   ZOMBIECHECK
   *retval=mp_input->GetSel().cpMin;
   return S_OK;
}

HRESULT Window_Input::GetSelEnd(int *retval)
{
   ZOMBIECHECK
   *retval=mp_input->GetSel().cpMax;
   return S_OK;
}

HRESULT Window_Input::get_Length(int *retval)
{
   ZOMBIECHECK
   *retval=mp_input->GetTextLength();
   return S_OK;
}

HRESULT Window_Input::get_Prefix(BSTR *retval)
{
   ZOMBIECHECK
   *retval=OwnedBSTR(mp_input->GetProps().pclPrefix()).Extract();
   return S_OK;
};

HRESULT Window_Input::put_Prefix(BSTR bstr)
{
   ZOMBIECHECK
   mp_input->GetProps().pclPrefix()=BSTRToLStr(bstr);
   mp_input->ApplyProps();
   return S_OK;
};

HRESULT Window_Input::get_Title(BSTR *retval)
{
   ZOMBIECHECK
   *retval=OwnedBSTR(mp_input->GetProps().pclTitle()).Extract();
   return S_OK;
};

HRESULT Window_Input::put_Title(BSTR bstr)
{
   ZOMBIECHECK
   mp_input->GetProps().pclTitle()=BSTRToLStr(bstr);
   mp_input->ApplyProps();
   return S_OK;
};

} // OM