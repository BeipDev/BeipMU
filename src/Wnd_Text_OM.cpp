#include "Main.h"
#include "Connection.h"

#include "Wnd_Text_OM.h"
#include "OM_Window_Properties.h"

namespace OM
{

HRESULT TextWindowLine::get_String(BSTR *retval)
{
   *retval=LStrToBSTR(*mp_line); return S_OK;
}

HRESULT TextWindowLine::get_HTMLString(BSTR *retval)
{
   HybridStringBuilder string;

   Windows::Color foreground_color=Colors::White;
   Windows::Color background_color=Colors::Black;
   Windows::Color hyperlink_color=Colors::Blue;

   string("<p");
   mp_line->HTMLCopy(string, foreground_color, background_color, hyperlink_color, "Courier New", 12);
   string("</p>");

   *retval=LStrToBSTR(string);
   return S_OK;
}

HRESULT TextWindowLine::Insert(unsigned iPosition, ITextWindowLine *pILine)
{
   const TextWindowLine *pLine=static_cast<TextWindowLine *>(pILine);
   if(pLine==this)
      return E_FAIL; // Inserting into ourselves is bad

   mp_line->InsertLine(iPosition, *pLine->mp_line);
   return S_OK;
}

HRESULT TextWindowLine::Delete(unsigned iStart, unsigned iEnd)
{
   if(iEnd<iStart)
      return E_FAIL;

   mp_line->DeleteText(iStart, iEnd-iStart);
   return S_OK;
}

HRESULT TextWindowLine::Color(unsigned iStart, unsigned iEnd, long lColor)
{
   mp_line->SetColor(uint2(iStart, iEnd), lColor); return S_OK;
}

HRESULT TextWindowLine::BgColor(unsigned iStart, unsigned iEnd, long lColor)
{
   mp_line->SetBackgroundColor(uint2(iStart, iEnd), lColor); return S_OK;
}

HRESULT TextWindowLine::Bold(unsigned iStart, unsigned iEnd, VARIANT_BOOL fSet)
{
   mp_line->SetBold(uint2(iStart, iEnd), !!fSet); return S_OK;
}

HRESULT TextWindowLine::Italic(unsigned iStart, unsigned iEnd, VARIANT_BOOL fSet)
{
   mp_line->SetItalic(uint2(iStart, iEnd), !!fSet); return S_OK;
}

HRESULT TextWindowLine::Underline(unsigned iStart, unsigned iEnd, VARIANT_BOOL fSet)
{
   mp_line->SetUnderline(uint2(iStart, iEnd), !!fSet); return S_OK;
}

HRESULT TextWindowLine::Strikeout(unsigned iStart, unsigned iEnd, VARIANT_BOOL fSet)
{
   mp_line->SetStrikeout(uint2(iStart, iEnd), !!fSet); return S_OK;
}

HRESULT TextWindowLine::Flash(unsigned iStart, unsigned iEnd, VARIANT_BOOL fSet)
{
   mp_line->SetFlash(uint2(iStart, iEnd), fSet ? 4 : 0, fSet ? 0x3 : 0); return S_OK;
}

HRESULT TextWindowLine::FlashMode(unsigned iStart, unsigned iEnd, int iMode)
{
   mp_line->SetFlashMode(uint2(iStart, iEnd), Text::Records::FlashMode(iMode)); return S_OK;
}

HRESULT TextWindowLine::Blink(unsigned iStart, unsigned iEnd, unsigned iBits, unsigned mask)
{
   mp_line->SetFlash(uint2(iStart, iEnd), iBits, mask); return S_OK;
}

#define ZOMBIECHECK if(!m_pWnd) return E_ZOMBIE;

STDMETHODIMP Window_Text::QueryInterface(REFIID riid, void **ppvObj)
{
   return TQueryInterface(riid, ppvObj);
}

Window_Text::~Window_Text()
{
   if(m_fOwned)
      delete m_pWnd;
}

STDMETHODIMP Window_Text::get_Properties(IWindow_Properties **retval)
{
   ZOMBIECHECK
   return RefReturner(retval)(MakeCounting<Window_Properties>(*m_pWnd, *m_pWnd));
}

STDMETHODIMP Window_Text::get_Paused(VARIANT_BOOL *retval)
{
   *retval=VariantBool(m_pWnd->IsPaused());
   return S_OK;
}

STDMETHODIMP Window_Text::SetOnPause(IDispatch *pDisp)
{
   return ManageHook<Text::Wnd::Event_Pause>(this, m_hookPause, *m_pWnd, pDisp);
}

STDMETHODIMP Window_Text::Write(BSTR bstr)
{
   ZOMBIECHECK
   m_pWnd->Add(Text::Line::CreateFromText(BSTRToLStr(bstr)));
   return S_OK;
}

STDMETHODIMP Window_Text::WriteHTML(BSTR bstr)
{
   ZOMBIECHECK
   m_pWnd->AddHTML(BSTRToLStr(bstr));
   return S_OK;
}

STDMETHODIMP Window_Text::Create(BSTR bstr, ITextWindowLine **ppLine)
{
   ZOMBIECHECK
   *ppLine=new TextWindowLine(BSTRToLStr(bstr), false);
   (*ppLine)->AddRef();
   return S_OK;
}

STDMETHODIMP Window_Text::CreateHTML(BSTR bstr, ITextWindowLine **ppLine)
{
   ZOMBIECHECK
   *ppLine=new TextWindowLine(BSTRToLStr(bstr), true);
   (*ppLine)->AddRef();
   return S_OK;
}

STDMETHODIMP Window_Text::Add(ITextWindowLine *pILine)
{
   ZOMBIECHECK
   m_pWnd->Add(MakeUnique<Text::Line>(static_cast<TextWindowLine *>(pILine)->GetInternal()));
   return S_OK;
}

void Window_Text::On(const Text::Wnd::Event_Pause &event)
{
   if(m_hookPause)
      m_hookPause(event.IsPaused() , this);
}

};
