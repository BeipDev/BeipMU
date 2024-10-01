#include "OM_Help.h"

namespace OM
{

struct Window_FixedText
:  public Dispatch<IWindow_FixedText>,
   public Events::ReceiversOf<Window_FixedText, Events::Event_Deleted>
{
   Window_FixedText(int2 size);
   ~Window_FixedText() noexcept;

   STDMETHODIMP get_Events(IWindow_Events **retval);
   STDMETHODIMP get_Properties(IWindow_Properties **retval);

   STDMETHODIMP put_CursorX(int x);
   STDMETHODIMP get_CursorX(int *pX);
   STDMETHODIMP put_CursorY(int y);
   STDMETHODIMP get_CursorY(int *pY);

   STDMETHODIMP Clear();
   STDMETHODIMP Write(BSTR bstr);

   void On(const Events::Event_Deleted &event);

private:
   UniquePtr<Wnd_FixedText> m_pWnd;
   CntPtrTo<Window_Events> m_pEvents;
   CntPtrTo<Window_Properties> m_pProperties;
};

};
