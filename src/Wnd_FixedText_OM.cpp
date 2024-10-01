#include "Main.h"
#include "WindowEvents.h"
#include "Wnd_FixedText.h"
#include "Wnd_Graphics.h"

#include "OM_Help.h"
#include "Window_Events_OM.h"
#include "OM_Window_Properties.h"
#include "Wnd_FixedText_OM.h"

namespace OM
{

#define ZOMBIECHECK if(!m_pWnd) return E_ZOMBIE;

Window_FixedText::Window_FixedText(int2 size)
{
   m_pWnd=MakeUnique<Wnd_FixedText>(nullptr, size);
   AttachTo<Events::Event_Deleted>(*m_pWnd);

   m_pEvents=new Window_Events(m_pWnd);
   m_pProperties=new Window_Properties(*m_pWnd, *m_pWnd);
}

Window_FixedText::~Window_FixedText()
{
   m_pWnd=nullptr; // To force notifications while we're still a valid object
}

void Window_FixedText::On(const Events::Event_Deleted &event)
{
   m_pEvents=nullptr; m_pProperties=nullptr; m_pWnd.Extract();
}

HRESULT Window_FixedText::get_Events(IWindow_Events **retval)
{
   return RefReturner(retval)(m_pEvents);
}

HRESULT Window_FixedText::get_Properties(IWindow_Properties **retval)
{
   return RefReturner(retval)(m_pProperties);
}

HRESULT Window_FixedText::put_CursorX(int x)
{
   ZOMBIECHECK
   m_pWnd->SetCursorX(x);
   return S_OK;
}

HRESULT Window_FixedText:: get_CursorX(int *pX)
{
   ZOMBIECHECK
   *pX=m_pWnd->GetCursor().x;
   return S_OK;
}

HRESULT Window_FixedText::put_CursorY(int y)
{
   ZOMBIECHECK
   m_pWnd->SetCursorY(y);
   return S_OK;
}

HRESULT Window_FixedText::get_CursorY(int *pY)
{
   ZOMBIECHECK
   *pY=m_pWnd->GetCursor().y;
   return S_OK;
}

HRESULT Window_FixedText::Clear()
{
   ZOMBIECHECK
   m_pWnd->Clear();
   return S_OK;
}

HRESULT Window_FixedText::Write(BSTR bstr)
{
   ZOMBIECHECK
   m_pWnd->Write(BSTRToLStr(bstr));
   return S_OK;
}

IWindow_FixedText *Create_Window_FixedText(int2 sz)
{
   return new Window_FixedText(sz);
}

};
