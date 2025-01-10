#include "OM_Help.h"

namespace OM
{

struct TextWindowLine : Dispatch<ITextWindowLine>
{
   TextWindowLine(ConstString string, bool fHTML) : mp_line(Text::Line::CreateFromHTML(string))
   {
   }

   TextWindowLine(ConstString string) : mp_line(Text::Line::CreateFromText(string))
   {
   }

   TextWindowLine(const Text::Line &line) : mp_line(MakeUnique<Text::Line>(line))
   {
   }

   // ITextWindow methods
   STDMETHODIMP get_Length(long *retval) override { *retval=mp_line->GetText().Count(); return S_OK; }
   STDMETHODIMP get_String(BSTR *retval) override;
   STDMETHODIMP get_HTMLString(BSTR *retval) override;

   STDMETHODIMP Insert(unsigned position, ITextWindowLine *pLine) override;
   STDMETHODIMP Delete(unsigned start, unsigned end) override;

   STDMETHODIMP Color(unsigned start, unsigned end, long lColor) override;
   STDMETHODIMP BgColor(unsigned start, unsigned end, long lColor) override;
   STDMETHODIMP Bold(unsigned start, unsigned end, VARIANT_BOOL fSet) override;
   STDMETHODIMP Italic(unsigned start, unsigned end, VARIANT_BOOL fSet) override;
   STDMETHODIMP Underline(unsigned start, unsigned end, VARIANT_BOOL fSet) override;
   STDMETHODIMP Strikeout(unsigned start, unsigned end, VARIANT_BOOL fSet) override;
   STDMETHODIMP Flash(unsigned start, unsigned end, VARIANT_BOOL fSet) override;
   STDMETHODIMP FlashMode(unsigned start, unsigned end, int iMode) override;
   STDMETHODIMP Blink(unsigned start, unsigned end, unsigned iBits, unsigned mask) override;

   const Text::Line &GetInternal() { return *mp_line; }

private:

   UniquePtr<Text::Line> mp_line;
};

struct Window_Text
:  Dispatch<IWindow_Text>,
   Text::IHost,
   Events::ReceiversOf<Window_Text, Text::Wnd::Event_Pause>
{
   Window_Text(Text::Wnd *pWnd, bool fOwned=false) : m_pWnd(pWnd), m_fOwned(fOwned)
   {
   }

   Window_Text(int2 size) : m_fOwned(true)
   {
      m_pWnd=new Text::Wnd(nullptr, *this);

      Prop::TextWindow &prop=g_ppropGlobal->propWindows().propMainWindowSettings().propOutput();
      m_pWnd->SetFont(prop.propFont().pclName(), prop.propFont().Size(), prop.propFont().CharSet());
      m_pWnd->SetSize(size);
      m_pWnd->Show(SW_SHOWNOACTIVATE);
   }

   ~Window_Text() noexcept;

   USE_INHERITED_UNKNOWN(IWindow_Text)

   // IUnknown
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;

   // ITextWindow methods
   STDMETHODIMP get_Properties(IWindow_Properties **retval) override;
   STDMETHODIMP get_Paused(VARIANT_BOOL *retval) override;

   STDMETHODIMP SetOnPause(IDispatch *pDisp) override;
   STDMETHODIMP Write(BSTR bstr) override;
   STDMETHODIMP WriteHTML(BSTR bstr) override;
   STDMETHODIMP Create(BSTR bstr, ITextWindowLine **ppLine) override;
   STDMETHODIMP CreateHTML(BSTR bstr, ITextWindowLine **ppLine) override;
   STDMETHODIMP Add(ITextWindowLine *pLine) override;

   // Events
   void On(const Text::Wnd::Event_Pause &event);

private:
   NotifiedPtrTo<Text::Wnd> m_pWnd;
   bool m_fOwned;

   Hook m_hookPause;
};

};
