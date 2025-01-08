#include "Main.h"

#include "Wnd_Main.h"

#include <RichOle.h>
#include <TextServ.h>
#include <TOM.h>
#include <uxtheme.h>
#include "Speller.h"
#include "OM_Help.h"

void CreateDialog_Find(Window wndParent, Controls::RichEdit &wndText);
void CreateDialog_InputWindow(Window wndParent, InputControl &input_window, Prop::InputWindow &propInputWindow);

DEFINE_GUID(IID_ITextServices, 0x8d33f740, 0xcf58, 0x11ce, 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5);
DEFINE_GUID(IID_ITextDocument, 0x8CC497C0, 0xA1DF, 0x11ce, 0x80, 0x98, 0x00, 0xAA, 0x00, 0x47, 0xBE, 0x5D);

struct Wnd_WordMenu : TWindowImpl<Wnd_WordMenu>
{
   static ATOM Register();
   Wnd_WordMenu(InputControl &input, CHARRANGE range, Collection<OwnedString> &&suggestions, int2 position);

private:
   void Dismiss();
   void UseSuggestion();

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Paint &msg);
   LRESULT On(const Msg::Activate &msg);
   LRESULT On(const Msg::Key &msg);
   LRESULT On(const Msg::Char &msg);
   LRESULT On(const Msg::MouseMove &msg);
   LRESULT On(const Msg::LButtonDown &msg);

   static constexpr unsigned ciPadding=4;

   InputControl &m_input;
   CHARRANGE m_range;
   Collection<OwnedString> m_suggestions;
   Handle<HFONT> m_font;
   int m_height;
   unsigned m_selection{};
   bool m_ignore_char{}; // Ignores the next char message
};

Wnd_WordMenu::Wnd_WordMenu(InputControl &input, CHARRANGE range, Collection<OwnedString> &&suggestions, int2 position)
 : m_input(input),
   m_range(range),
   m_suggestions(std::move(suggestions))
{
   auto &font=input.GetWndMain().GetActiveInputWindow().GetProps().propFont();
   m_font=font.CreateFont();

   int width=0;
   {
      TEXTMETRIC tm;

      ScreenDC dc;
      DC::FontSelector _(dc, m_font);
      dc.GetTextMetrics(tm);

      for(auto &word : m_suggestions)
         PinAbove(width, dc.TextExtent(word).x);

      m_height=tm.tmHeight;
   }

   // Position the window so that it's aligned with the current line of text
   Window::Position wp;
   wp.pt=position-int2(0, (m_suggestions.Count()-1)*m_height+ciPadding*g_dpiScale);
   wp.sz=int2(width, m_suggestions.Count()*m_height)+2*ciPadding*g_dpiScale;

   Create("", WS_POPUP|WS_VISIBLE, wp, input);
}

void Wnd_WordMenu::Dismiss()
{
   m_input.SetFocus();
   Destroy();
}

void Wnd_WordMenu::UseSuggestion()
{
   Controls::RichEdit::EventMaskRestorer _(m_input, 0);
   m_input.HideSelection();
   m_input.SetSel(m_range);

   // Clear out all current underlines in the range
   Controls::RichEdit::CharFormat cf;
   cf.SetUnderline(false);
   m_input.SetCharFormat(cf);

   m_input.ReplaceSel(m_suggestions[m_selection]);
   m_input.HideSelection(false);

   int cursor=m_range.cpMin+m_suggestions[m_selection].Count();
   m_input.SetSel(cursor, cursor);
}

LRESULT Wnd_WordMenu::WndProc(const Message &msg)
{
   return Dispatch<WindowImpl, Msg::Create, Msg::Paint, Msg::Activate, Msg::KeyDown, Msg::KeyUp, Msg::Char, Msg::MouseMove, Msg::LButtonDown>(msg);
}

LRESULT Wnd_WordMenu::On(const Msg::Create &msg)
{
   return msg.Success();
}

LRESULT Wnd_WordMenu::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);

   Rect rc=ClientRect();

   Handle<HBRUSH> brushBackground(CreateSolidBrush(Color(32, 32, 32)));
   Handle<HBRUSH> brushSelection(CreateSolidBrush(Color(64, 64, 255)));
   ps.SelectBrush(brushBackground);
   ps.SelectPen((HPEN)GetStockObject(WHITE_PEN));
   ps.Rectangle(rc);

   unsigned padding=ciPadding*g_dpiScale;
   rc.top+=padding;
   rc.bottom-=padding;

   ps.SelectFont(m_font);
   ps.SetBackgroundMode(TRANSPARENT);
   ps.SetTextColor(Colors::White);

   for(unsigned i=0;i<m_suggestions.Count();i++)
   {
      auto &word=m_suggestions[i];
      int2 pos(rc.left+padding, rc.bottom-(i+1)*m_height);

      if(i==m_selection)
         ps.FillRect(Rect(int2(1, pos.y), int2(rc.right-1, pos.y+m_height)), brushSelection);

      ps.TextOut(pos, word);
      pos.y+=m_height;
   }

   return msg.Success();
}

LRESULT Wnd_WordMenu::On(const Msg::Activate &msg)
{
   if(msg.uState()==WA_INACTIVE)
      Close();

   return msg.Success();
}

LRESULT Wnd_WordMenu::On(const Msg::Key &msg)
{
   if(msg.direction()==Direction::Up)
      return msg.Success();

   BoundedIndex bound{m_suggestions.Count()};

   switch(msg.iVirtKey())
   {
      case VK_BACK:
      case VK_ESCAPE:
         Dismiss(); // NOTE: Deletes us
         return msg.Success();

      case VK_TAB:
         if(IsKeyPressed(VK_SHIFT))
            bound.WrapIncrement(m_selection);
         else
            bound.WrapDecrement(m_selection);

         Invalidate(true);
         m_ignore_char=true;
         break;

      case VK_UP:
         bound.WrapIncrement(m_selection);
         Invalidate(true);
         break;

      case VK_DOWN:
         bound.WrapDecrement(m_selection);
         Invalidate(true);
         break;
   }

   return msg.Success();
}

LRESULT Wnd_WordMenu::On(const Msg::Char &msg)
{
   if(m_ignore_char)
   {
      m_ignore_char=false;
      return msg.Success();
   }

   UseSuggestion();
   msg.Send(m_input);
   Dismiss(); // NOTE: deletes us
   return msg.Success();
}

LRESULT Wnd_WordMenu::On(const Msg::MouseMove &msg)
{
   int lower=ciPadding*g_dpiScale;
   int upper=lower+m_suggestions.Count()*m_height-1;

   if(!IsBetween(msg.y(), lower, upper))
      return msg.Success();

   unsigned selection=(upper-msg.y())/m_height;
   if(m_selection!=selection)
   {
      m_selection=selection;
      Invalidate(true);
   }
   return msg.Success();
}

LRESULT Wnd_WordMenu::On(const Msg::LButtonDown &msg)
{
   UseSuggestion();
   Dismiss(); // NOTE: deletes us
   return msg.Success();
}

ATOM Wnd_WordMenu::Register()
{
   WndClass wc(L"WordMenu");
   wc.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
   wc.hCursor=LoadCursor(NULL, IDC_ARROW);
   return wc.Register();
}


struct __declspec(uuid("00020D03-0000-0000-00C0-000000000046")) IRichEditOleCallback;

struct RichEditOleCallback : General::Unknown<IRichEditOleCallback>
{
   RichEditOleCallback(InputControl &input) : m_input(input) { }

   STDMETHODIMP QueryInterface(const GUID &id, void **ppvObj) override { return TQueryInterface(id, ppvObj); }

   // *** IRichEditOleCallback methods ***
   STDMETHOD(GetNewStorage) (THIS_ LPSTORAGE FAR * lplpstg) override { return E_NOTIMPL; }
   STDMETHOD(GetInPlaceContext) (THIS_ LPOLEINPLACEFRAME FAR * lplpFrame, LPOLEINPLACEUIWINDOW FAR * lplpDoc, LPOLEINPLACEFRAMEINFO lpFrameInfo) override { return E_NOTIMPL; }
   STDMETHOD(ShowContainerUI) (THIS_ BOOL fShow) override { return E_NOTIMPL; }
   STDMETHOD(QueryInsertObject) (THIS_ LPCLSID lpclsid, LPSTORAGE lpstg, LONG cp) override { return E_NOTIMPL; }
   STDMETHOD(DeleteObject) (THIS_ LPOLEOBJECT lpoleobj) override { return E_NOTIMPL; }
   STDMETHOD(QueryAcceptData) (THIS_ IDataObject *p_data_object, CLIPFORMAT FAR * lpcfFormat, DWORD reco, BOOL fReally, HGLOBAL hMetaPict) override 
   {
#if 0
      if(fReally)
      {
         FORMATETC format;
         format.cfFormat=CF_DIB;
         format.ptd=nullptr;
         format.dwAspect=DVASPECT_CONTENT;
         format.lindex=-1;
         format.tymed=TYMED_HGLOBAL;

         STGMEDIUM storage;

         if(SUCCEEDED(p_data_object->GetData(&format, &storage)))
         {
            auto p_stream=CreateIStream();

            {
               GlobalLocker dib_data{storage.hGlobal};
               BITMAPFILEHEADER header;
               header.bfType=MAKEWORD('B','M');
               header.bfSize=dib_data.SizeInBytes()+sizeof(BITMAPFILEHEADER);
               header.bfReserved1=0;
               header.bfReserved2=0;
               header.bfOffBits=sizeof(BITMAPFILEHEADER);

               p_stream->Write(&header, sizeof(header), nullptr);
               p_stream->Write(dib_data.begin(), dib_data.SizeInBytes(), nullptr);
               p_stream->Seek(LARGE_INTEGER{}, STREAM_SEEK_SET, nullptr);
            }

            auto image=Imaging::Image(*p_stream);
            auto image_size=image.GetSize();

            if(max(image_size)>64)
               image_size=int2(TouchFromInside(int2(64, 64), image_size));
            auto wic_bitmap=image.GetBitmapSource(image_size, GUID_WICPixelFormat32bppBGR);

            CntPtrTo<IStream> p_stream_png=CreateIStream();
            {
               CntPtrTo<IWICBitmapEncoder> pWICEncoder;
               GetWICImagingFactory().CreateEncoder(GUID_ContainerFormatPng, nullptr, pWICEncoder.Address());
               pWICEncoder->Initialize(p_stream_png, WICBitmapEncoderNoCache);
               {
                  CntPtrTo<IWICBitmapFrameEncode> pWICFrameEncode;
                  CntPtrTo<IPropertyBag2> pPropertyBag2;
                  pWICEncoder->CreateNewFrame(pWICFrameEncode.Address(), pPropertyBag2.Address());
#if 0
                  {
                     PROPBAG2 option{}; option.pstrName=UnconstRef(L"ImageQuality");
                     OM::Variant value{0.1f};
                     pPropertyBag2->Write(1, &option, &value);
                  }
                  pWICFrameEncode->Initialize(pPropertyBag2);
#endif
                  pWICFrameEncode->Initialize(nullptr);
                  auto pixel_format=GUID_WICPixelFormat8bppIndexed;
                  pWICFrameEncode->SetPixelFormat(&pixel_format);
                  pWICFrameEncode->WriteSource(wic_bitmap, nullptr);
                  pWICFrameEncode->Commit();
               }
               pWICEncoder->Commit();
            }

            p_stream_png->Seek(LARGE_INTEGER{}, STREAM_SEEK_SET, nullptr);

            STATSTG stat_storage; p_stream_png->Stat(&stat_storage, STATFLAG_NONAME);
            OwnedArray<uint8> png_data{stat_storage.cbSize.LowPart};
            p_stream_png->Read(png_data.begin(), png_data.Count(), nullptr);

//            CntPtrTo<IStream> pStream=Storage::CreateIStreamOnNewFile("C:\\Users\\Ryan\\Documents\\TestPng.png");
//            pStream->Write(png_data.begin(), png_data.Count(), nullptr);

            HybridStringBuilder data_uri{"data:image/png;base64,"};
            Base64_Encode(data_uri, png_data);

            m_input.ReplaceSel(data_uri);
            return S_FALSE;
         }
      }
#endif

      *lpcfFormat=CF_TEXT;
      return S_OK;
   }

   STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) override { return E_NOTIMPL; }
   STDMETHOD(GetClipboardData) (THIS_ CHARRANGE FAR * lpchrg, DWORD reco, LPDATAOBJECT FAR * lplpdataobj) override { return E_NOTIMPL; }
   STDMETHOD(GetDragDropEffect) (THIS_ BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect) override { return E_NOTIMPL; }
   STDMETHOD(GetContextMenu) (THIS_ WORD seltype, LPOLEOBJECT lpoleobj, CHARRANGE FAR * lpchrg, HMENU FAR * lphmenu) override
   {
      int2 mouse=m_input.ScreenToClient(GetCursorPos());
      m_input.RichEditPopup(m_input.CharFromPos(mouse), mouse);
      return S_OK;
   }

private:
   InputControl &m_input;
};

bool IsDelimiter(Controls::RichEdit &edit, int index)
{
   constexpr int buffer_size=1;
   std::array<wchar_t, buffer_size+1> buffer;
   TextRange text_range{buffer};
   text_range.chrg.cpMin=index;
   text_range.chrg.cpMax=index+1;
   edit.GetTextRange(text_range);

   return IsWordBreak(buffer[0]);
}

int MoveWordLeft(Controls::RichEdit &edit, int index)
{
   if(index<=0)
      return 0;

   constexpr int buffer_size=31;
   std::array<wchar_t, buffer_size+1> buffer;
   TextRange text_range{buffer};
   text_range.chrg.cpMin=Greater(index-buffer_size, 0);
   text_range.chrg.cpMax=index;
   edit.GetTextRange(text_range);

   // Find the first non whitespace character, then keep going until we finds the first whitespace character and stop before it.
   while(index>text_range.chrg.cpMin && IsWordBreak(buffer[index-1-text_range.chrg.cpMin]))
      index--;
   while(index>text_range.chrg.cpMin && !IsWordBreak(buffer[index-1-text_range.chrg.cpMin]))
      index--;

   return index;
}

int MoveWordRight(Controls::RichEdit &edit, int index)
{
   auto text_length=edit.GetTextLength();
   if(index>=text_length)
      return text_length;

   constexpr int buffer_size=31;
   std::array<wchar_t, buffer_size+1> buffer;
   TextRange text_range{buffer};
   text_range.chrg.cpMin=index;
   text_range.chrg.cpMax=Lesser(index+buffer_size, text_length);
   edit.GetTextRange(text_range);

   // Find the first whitespace character, then keep going until we finds the first non whitespace character and stop on it.
   while(index<text_range.chrg.cpMax && !IsWordBreak(buffer[index-text_range.chrg.cpMin]))
      index++;
   while(index<text_range.chrg.cpMax && IsWordBreak(buffer[index-text_range.chrg.cpMin]))
      index++;

   return index;
}

InputControl::InputControl(Wnd_Main &wnd_main, Prop::InputWindow &props, Wnd_InputPane *p_input_pane, IEditHost *pIEditHost)
 : m_wnd_main{wnd_main}, mp_props{&props}, mp_input_pane{p_input_pane}, m_pIEditHost{pIEditHost}
{
}

InputControl::~InputControl()=default;

void InputControl::Create(Window wndParent)
{
   DWORD style=WS_CHILD|ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL;
   if(m_pIEditHost)
      style|=ES_SELECTIONBAR|ES_DISABLENOSCROLL;

   Controls::RichEdit::Create(wndParent, 1 /*commandID*/, int2(1, 1), style, 0);
   Controls::RichEdit::LimitText(4*1024*1024); // 4 MB input limit
   SendMessage(hWnd(), EM_SETEVENTMASK, 0, ENM_CHANGE|ENM_SELCHANGE);
   SendMessage(hWnd(), EM_SETOLECALLBACK, 0, (LPARAM)new RichEditOleCallback(*this));

   // Turn off the annoying beep sound when going out of bounds
   CntPtrTo<IRichEditOle> pRichEdit; SendMessage(hWnd(), EM_GETOLEINTERFACE, 0, (LPARAM)pRichEdit.Address());
   pRichEdit->QueryInterface(IID_ITextDocument, (void **)p_text_document.Address());
   CntPtrTo<ITextServices> pTextServices; pRichEdit->QueryInterface(IID_ITextServices, (void **)pTextServices.Address());
   if(pTextServices)
      pTextServices->OnTxPropertyBitsChange(TXTBIT_ALLOWBEEP, 0);

// RichEdit supports a built in speller, but we don't use it
//   LRESULT options=SendMessage(hWnd(), EM_GETLANGOPTIONS, 0, 0)|IMF_SPELLCHECKING;
//   SendMessage(hWnd(), EM_SETLANGOPTIONS, 0, options);
   Init_EditSendEnter(*this, *this);
   if(g_dark_mode)
      SetWindowTheme(*this, L"DarkMode_Explorer", nullptr);

   ApplyProps();
}

void InputControl::ApplyProps()
{
   ScreenDC dc;

   CHARFORMAT2 cf;
   cf.cbSize=sizeof(cf);
   cf.dwMask=CFM_COLOR|CFM_SIZE|CFM_FACE|CFM_BOLD|CFM_ITALIC|CFM_CHARSET;
   cf.crTextColor=mp_props->clrFore();
   cf.dwEffects=(mp_props->propFont().fBold() ? CFE_BOLD : 0) | (mp_props->propFont().fItalic() ? CFE_ITALIC : 0);
   // 1440 Twips/Inch, LOGPIXELSY = Pixels/Inch
   cf.yHeight=MulDiv(mp_props->propFont().Size()*g_dpiScale, 1440 /*twips*/, dc.GetCaps(LOGPIXELSY));
   cf.bCharSet=mp_props->propFont().CharSet();
   cf.bPitchAndFamily=DEFAULT_PITCH;
   UTF16_Inplace(cf.szFaceName, mp_props->propFont().pclName());

   // In Windows, only need to call SCF_ALL, but for Wine, we need to do both so that selected history items also keep the color. Bug in wine?
   SendMessage(hWnd(), EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
   SendMessage(hWnd(), EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);

   // Set the font for all keyboard languages in the system. Without this, the font will not stick properly on clearing the window, as RichEdit
   // will try to be helpful and see that your keyboard needs a font with more language support than the one you chose, even if you don't type it.
   HKL keyboards[16];
   int keyboard_count=GetKeyboardLayoutList(std::size(keyboards), keyboards);
   for(int i=0;i<keyboard_count;i++)
   {
      cf.dwMask=CFM_FACE|CFM_SIZE|CFM_LCID;
      cf.lcid=LOWORD(keyboards[i]);
      SendMessage(hWnd(), EM_SETCHARFORMAT, SCF_ASSOCIATEFONT, (LPARAM)&cf);
   }

   if(GetTextLength()==0)
      SetText(""); // For some reason, if the font is FixedSys the font won't change on the first text typed until we do this. Bug in RichEdit?

   Rect rect=ClientRect();
   rect.Inset(mp_props->rcMargins().left, mp_props->rcMargins().top, mp_props->rcMargins().right, mp_props->rcMargins().bottom);
   if(rect.left<rect.right && rect.top<rect.bottom) // If we go too far, do nothing
      SendMessage(hWnd(), EM_SETRECT, 0, (LPARAM)&rect);

   SetBackgroundColor(mp_props->clrBack());
   if(mp_input_pane)
      mp_input_pane->UpdateTitle();
}

void InputControl::SetText(ConstString text)
{
   // If the prefix is already in the text, remove it, since we'll add it again later
   if(mp_props->pclPrefix() && text.StartsWith(mp_props->pclPrefix()))
      text=text.WithoutFirst(mp_props->pclPrefix().Count());

   Controls::RichEdit::SetText(text);
}

bool InputControl::IsPrimary() const
{
   return &m_wnd_main.GetInputWindow()==this;
}

void InputControl::ConvertReturns()
{
   OwnedString string{GetText()};

   while(true)
   {
      auto location=string.Find(ConstString{CRLF});
      if(location==Strings::Result::Not_Found)
         break;
      string.Replace(uint2(location, location+2), "%R");
   }

   SetText(string);
}

void InputControl::ConvertTabs()
{
   OwnedString string{GetText()};

   while(true)
   {
      auto location=string.FindFirstOf('\t');
      if(location==Strings::Result::Not_Found)
         break;
      string.Replace(uint2(location, location+1), "%T");
   }

   SetText(string);
}

void InputControl::ConvertSpaces()
{
   OwnedString string{GetText()};

   while(true)
   {
      auto location=string.FindFirstOf(' ');
      if(location==Strings::Result::Not_Found)
         break;
      string.Replace(uint2(location, location+1), "%B");
   }

   SetText(string);
}

void InputControl::EnableSpellChecking(bool fEnable)
{
   m_fSpellCheck=fEnable;

   // If we're turning off spell check, hide all underlines
   if(!m_fSpellCheck)
   {
      Controls::RichEdit::CharFormat cfNoUnderline;
      cfNoUnderline.SetUnderline(false);
      SetCharFormat(cfNoUnderline, SCF_ALL);
   }
   else
   {
      // Don't spell check the whole range as it's too big to do all at once
//      CHARRANGE range; range.cpMin=0; range.cpMax=GetTextLength();
//      SpellCheckRange(range);
   }
}

void InputControl::SpellCheckRange(CHARRANGE checkRange)
{
   if(!m_fSpellCheck || !g_ppropGlobal->fSpellCheck())
      return;

   p_text_document->Undo(tomSuspend, nullptr);

   bool wasModified=IsModified();
   Controls::RichEdit::EventMaskRestorer _(*this, 0);
   HideSelection();
   CHARRANGE selection=GetSel();

   wchar_t wordBuffer[64];
   TextRange range(wordBuffer);
   range.chrg.cpMin=checkRange.cpMin;

   Controls::RichEdit::CharFormat cfNoUnderline;
   cfNoUnderline.SetUnderline(false);

   Controls::RichEdit::CharFormat cfUnderline;
   cfUnderline.SetUnderline(true);
   if(!IsWine()) // Wine will draw nothing if we use the wavy underline
      cfUnderline.SetUnderlineType(CFU_UNDERLINEWAVE);
   static BYTE redUnderlineColor=IsWindows8OrGreater() ? 6 : 5; // 6 Appears to be red on Windows 8+, and 5 on Windows 7
   cfUnderline.bUnderlineColor=redUnderlineColor;

   // Clear out all current underlines in the range
   SetSel(checkRange);
   SetCharFormat(cfNoUnderline);

   while(range.chrg.cpMin<checkRange.cpMax)
   {
      range.chrg.cpMax=range.chrg.cpMin+1;

      if(!IsDelimiter(*this, range.chrg.cpMax))
         range.chrg.cpMax=MoveWordRight(*this, range.chrg.cpMax);

      PinBelow(range.chrg.cpMax, checkRange.cpMax);
      Assert(range);

      // Only check the word if it's small enough
      if(range.Length()<_countof(wordBuffer))
      {
         GetTextRange(range);
         wchar_t *word=wordBuffer;

         // Skip punctuation at start
         while(range && !IsLetter(word[0]))
         {
            range.chrg.cpMin++;
            word++; 
         }

         // Skip punctuation at end
         while(range && !IsLetter(word[range.Length()-1]))
         {
            range.chrg.cpMax--;
            word[range.Length()]='\0';
         }

         if(!Speller::Check(ConstWString(word, range.Length())))
         {
            SetSel(range.chrg);
            SetCharFormat(cfUnderline);

            // Turn off underline effect if the cursor is at the end of a misspelled word
            if(selection.cpMin==range.chrg.cpMax)
            {
               SetSel(selection);
               SetCharFormat(cfNoUnderline);
            }
         }
      }

//      range.chrg.cpMin=SendMessage(hWnd(), EM_FINDWORDBREAK, WB_MOVEWORDRIGHT, range.chrg.cpMin);
      range.chrg.cpMin=MoveWordRight(*this, range.chrg.cpMin);
   }
   SetSel(selection);
   HideSelection(false);
   if(!wasModified) // If we didn't start as modified, clear the modified flag since spelling shouldn't count as modifying
      SetModified(false);

   p_text_document->Undo(tomResume, nullptr);
}

void InputControl::RichEditPopup(unsigned charIndex, int2 position)
{
   wchar_t wordBuffer[64];
   TextRange range(wordBuffer);
#if 0
   range.chrg.cpMin=SendMessage(hWnd(), EM_FINDWORDBREAK, WB_MOVEWORDLEFT, charIndex);
   range.chrg.cpMax=SendMessage(hWnd(), EM_FINDWORDBREAK, WB_MOVEWORDRIGHT, range.chrg.cpMin+1);
#else
   range.chrg.cpMin=MoveWordLeft(*this, charIndex);
   range.chrg.cpMax=MoveWordRight(*this, range.chrg.cpMin+1);
#endif

   GetTextRange(range);

   // Skip punctuation at end
   while(range && IsWordBreak(range.lpstrText[range.Length()-1]))
      range.chrg.cpMax--;

   ConstWString word{range.lpstrText, unsigned(range.Length())};

   PopupMenu menu;
   if(!m_pIEditHost)
   {
      menu.Append(mp_props==&GlobalInputSettings() ? MF_CHECKED : MF_UNCHECKED, 8, "Use global settings");
      menu.Append(0, 1, "Settings...");
      menu.AppendSeparator();
   }
   {
      PopupMenu menu_text;
//TODO:      menu_text.Append(MF_CHECKED, 12, "Apply % Escaping on Send");
      menu_text.AppendSeparator();
      menu_text.Append(0, 9, "Convert Returns to %R\tF1");
      menu_text.Append(0, 10, "Convert Tabs to %T\tShift+F1");
      menu_text.Append(0, 11, "Convert Spaces to %B\tShift+F2");
      menu.Append(std::move(menu_text), "Conversion");
   }
   menu.Append(word ? 0 : MF_GRAYED, 2, "Cut");
   menu.Append(word ? 0 : MF_GRAYED, 3, "Copy");
   menu.Append(SendMessage(hWnd(), EM_CANPASTE, CF_TEXT, 0)==0 ? MF_GRAYED : 0, 4, "Paste");
   menu.AppendSeparator();
   menu.Append(0, 5, "Select All");

   Collection<OwnedString> suggestions; 
   if(word)
   {
      if(!Speller::Check(word))
      {
         Speller::Suggest(word, suggestions);
         menu.AppendSeparator();
         if(suggestions.Count())
         {
            for(unsigned i=0;i<suggestions.Count();i++)
               menu.Append(0, 100+i, suggestions[i]);
         }
         else
            menu.Append(MF_DISABLED, 99, "(No suggestions)");
         menu.AppendSeparator();
         menu.Append(0, 6, "Ignore");
      }
      else
      {
         menu.AppendSeparator();
         menu.Append(0, 7, "Thesaurus.com lookup");
      }
   }

   int command=TrackPopupMenu(menu, TPM_RETURNCMD|TPM_NONOTIFY, ClientToScreen(position), *this, nullptr);
   switch(command)
   {
      case 0: break;
      case 1: CreateDialog_InputWindow(*this, *this, *mp_props); break;
      case 2: SendMessage(hWnd(), WM_CUT, 0, 0); break;
      case 3: SendMessage(hWnd(), WM_COPY, 0, 0); break;
      case 4: SendMessage(hWnd(), WM_PASTE, 0, 0); break;
      case 5: SetSelAll(); break;
      case 6: Speller::Add(word); SpellCheckRange(range.chrg); break;
      case 7: OpenURLAsync(FixedStringBuilder<256>{"https://www.thesaurus.com/browse/", word}); break;
      case 8: 
      {
         auto &global=GlobalInputSettings();
         if(mp_props==&global) // Using global, so make a copy so edits are no longer global
            mp_props=MakeCounting<Prop::InputWindow>(*mp_props);
         else
         {
            if(*mp_props!=global && MessageBox(*this, "Switching to global settings will overwrite your custom changes; are you sure?", "Note", MB_ICONQUESTION|MB_YESNO)!=IDYES)
               break;

            mp_props=&global;
            if(IsPrimary())
               m_wnd_main.ApplyInputProperties();
            else
               ApplyProps(); // We switched to global properties, so redo settings
         }
         break;
      }

      case 9:
         ConvertReturns();
         if(IsPrimary())
            m_wnd_main.CheckInputHeight();
         break;
      case 10: ConvertTabs(); break;
      case 11: ConvertSpaces(); break;

      default:
      {
         CHARRANGE selection=GetSel();
         {
            Controls::RichEdit::EventMaskRestorer _(*this, 0);
            HideSelection();
            SetSel(range.chrg);

            // Clear out all current underlines in the range
            Controls::RichEdit::CharFormat cf;
            cf.SetUnderline(false);
            SetCharFormat(cf);

            ReplaceSel(suggestions[command-100]);
            selection=GetSel();

            HideSelection(false);
         }
         SetSel(selection); // Done without the event mask so we recheck the current word typo'd flag
         break;
      }
   }
}

// Handles generating a list of words sorted by popularity
struct Words
{
   struct Word
   {
      OwnedString m_string;
      unsigned m_hits{1};
   };

   Collection<Word> m_words;

   void Add(ConstString match)
   {
      for(unsigned i=0;i<m_words.Count();i++)
      {
         if(m_words[i].m_string==match)
         {
            unsigned hits=++m_words[i].m_hits;
            while(i>0 && m_words[i-1].m_hits<hits)
            {
               std::swap(m_words[i], m_words[i-1]);
               i--;
            }
            return;
         }
      }

      Assert(m_words.Count()<=1000);
      if(m_words.Count()>1000)
         return; // Too many suggestions, don't make it worse

      m_words.Push(match);
   }

   Collection<OwnedString> Finalize()
   {
      Collection<OwnedString> results;

      unsigned count=min(10U, m_words.Count());
      for(unsigned i=0;i<count;i++)
         results.Push(std::move(m_words[i].m_string));

      return results;
   }
};

void InputControl::Autocomplete(Collection<const Text::Lines*> &linesCollection, bool whole_line)
{
   wchar_t wordBuffer[64];
   TextRange range(wordBuffer);
   range.chrg.cpMax=GetSel().cpMax;
   range.chrg.cpMin=whole_line ? 0 : MoveWordLeft(*this, range.chrg.cpMax);

   if(!range || range.Length()>=_countof(wordBuffer))
      return;

   GetTextRange(range);
   if(range.Length()<3)
   {
      MessageBeep(MB_OK);
      return;
   }

   OwnedString word=ConstWString(wordBuffer, range.Length());

   Words words;

   static constexpr ConstString strWhitespace=" @#!\"£$%^&*().,:;~[]{}€|`<>/\t=";
   auto IsWhitespace = [](char c) { return strWhitespace.Find(c)!=Strings::Result::Not_Found; };

   for(auto *pLines : linesCollection)
   {
      unsigned limit=0;
      auto begin=pLines->begin();
      for(auto iter=pLines->end(); iter!=begin && limit<1000;++limit)
      {
         --iter;
         auto text=iter->GetText();

         if(whole_line)
         {
            if(text.IStartsWith(word))
               words.Add(text);
         }
         else
         {
            while(auto range=text.IFindRange(word))
            {
               // Only add the word if it's a whole word
               if(range.begin==0 || IsWhitespace(text[range.begin-1]))
               {
                  for(;range.end<text.Length();range.end++) // Look for the end of the word
                     if(IsWhitespace(text[range.end]))
                        break;

                  words.Add(text.Sub(range));
               }

               text=text.WithoutFirst(range.end);
            }
         }
      }
   }

   auto suggestions=words.Finalize();
   if(!suggestions)
      return;

   new Wnd_WordMenu(*this, range.chrg, std::move(suggestions), ClientToScreen(PosFromChar(whole_line ? 0 : range.chrg.cpMax)));
}

bool InputControl::EditChar(const Msg::Char &msg)
{
   if(m_pIEditHost)
      return m_pIEditHost->EditChar(msg);
   return m_wnd_main.EditChar(*this, msg.chCharCode());
}

bool InputControl::EditKey(const Msg::Key &msg)
{
   if(msg.iVirtKey()==VK_APPS)
   {
      CHARRANGE selection=GetSel();
      RichEditPopup(selection.cpMax, PosFromChar(selection.cpMax));
      return true;
   }

   if(m_pIEditHost)
      return m_pIEditHost->EditKey(msg);

   return m_wnd_main.EditKey(*this, msg);
}

void InputControl::OnChange()
{
   CHARRANGE selection=GetSel();
   // Ignore selected characters, we only care about the insertion point moving
   if(selection.cpMax-selection.cpMin>0)
      return;

   bool fCheck=false;
   {
      unsigned newLength=GetTextLength();
      int delta=newLength-m_inputLastSize;
      if(delta>1 && delta<64 && selection.cpMin>delta-1) // More than 1 char typed and less than 64, the user must have pasted. So check this whole range
      {
         selection.cpMin-=delta-1;
         fCheck=true;
      }
      m_inputLastSize=newLength;
   }
   if(!fCheck)
   {
      fCheck|=(GetCharFormat().dwEffects&CFE_UNDERLINE)!=0; // We are on a typo
   }
   if(!fCheck && selection.cpMin>0)
   {
      wchar_t nearbyChars[3];
      TextRange typed(nearbyChars); typed.chrg.cpMin=selection.cpMin-1; typed.chrg.cpMax=selection.cpMin+1;
      GetTextRange(typed);
      fCheck|=!IsLetter(nearbyChars[0]); // We typed a delimeter
      fCheck|=IsLetter(nearbyChars[1]); // We typed in the middle of a word
   }

   if(fCheck)
   {
      // Note: Start the scan from the last word start or the selection-2, so if someone pastes a large blob, we start at the word start of the paste
      CHARRANGE range;
//      range.cpMin=SendMessage(hWnd(), EM_FINDWORDBREAK, WB_MOVEWORDLEFT, selection.cpMin-2); // -1 because RichEdit doesn't see a ',' as a delimeter, so we check an extra word to the left
//      range.cpMax=SendMessage(hWnd(), EM_FINDWORDBREAK, WB_MOVEWORDRIGHT, selection.cpMax+1);
      range.cpMin=MoveWordLeft(*this, selection.cpMin-1);
      range.cpMax=MoveWordRight(*this, selection.cpMax+1);

//            OutputDebugString(TString<256>("Checking range:", range.cpMin, " to ", range.cpMax, CRLF));
      SpellCheckRange(range);
      m_uncheckedSelection=~0U;
      return;
   }

   m_uncheckedSelection=selection.cpMin;
}

LRESULT InputControl::On(const Msg::Command &msg)
{
   Assert(msg.wndCtl()==*this);

   switch(msg.uCodeNotify())
   {
      case EN_SETFOCUS:
         if(!m_pIEditHost)
            m_wnd_main.SetActiveInputWindow(*this);
         break;
      case EN_CHANGE: OnChange(); break;
   }

   return msg.Success();
}

LRESULT InputControl::On(SELCHANGE &selchange)
{
   if(m_pIEditHost)
      m_pIEditHost->EditSelChanged();

   if(selchange.chrg.cpMax-selchange.chrg.cpMin>1) // Ignore multiple char selection
      return 0;

   if(m_uncheckedSelection!=~0U && abs(selchange.chrg.cpMin-(int)m_uncheckedSelection)>1)
   {
      // Note: Start the scan from the last word start or the selection-1, so if someone pastes a large blob, we start at the word start of the paste
      CHARRANGE range;
//      range.cpMin=SendMessage(hWnd(), EM_FINDWORDBREAK, WB_MOVEWORDLEFT, m_uncheckedSelection-1);
//      range.cpMax=SendMessage(hWnd(), EM_FINDWORDBREAK, WB_MOVEWORDRIGHT, m_uncheckedSelection+1);
      range.cpMin=MoveWordLeft(*this, m_uncheckedSelection-1);
      range.cpMax=MoveWordRight(*this, m_uncheckedSelection+1);
      SpellCheckRange(range);
      m_uncheckedSelection=~0U;
   }
   return 0;
}

LRESULT InputControl::On(const Msg::Notify &msg)
{
   Assert(Window{msg.pnmh()->hwndFrom}==*this);

   switch(msg.pnmh()->code)
   {
      case EN_SELCHANGE: return On(*reinterpret_cast<SELCHANGE *>(msg.pnmh()));
   }
   return 0;
}

ATOM Wnd_InputPane::Register()
{
   WndClass wc(L"Wnd_InputPane");
   wc.LoadIcon(IDI_APP, g_hInst);
   wc.hCursor=LoadCursor(nullptr, IDC_ARROW);
   return wc.Register();
}

Wnd_InputPane::Wnd_InputPane(Wnd_Main &wndMain, Prop::InputWindow &props)
  : m_input{wndMain, props, this}
{
   AttachTo<GlobalInputSettingsModified>(g_text_events);
   Create("", WS_OVERLAPPEDWINDOW, Window::Position_Default, wndMain);

   int2 frameSize=FrameSize();
   SetSize(frameSize+int2(100, Windows::Controls::Control::m_tmButtons.tmHeight));
   mp_docking=&wndMain.CreateDocking(*this);
}

void Wnd_InputPane::UpdateTitle()
{
   auto &props=m_input.GetProps();
   FixedStringBuilder<256> string;
   if(auto &title=props.pclTitle())
      string(title);
   else
      string("Input");

   if(auto &prefix=props.pclPrefix())
      string(" - ", prefix);

   SetText(string);
}

Wnd_InputPane::~Wnd_InputPane()
{
}

void Wnd_InputPane::On(const GlobalInputSettingsModified &event)
{
   if(&m_input.GetProps()==&event.prop)
      m_input.ApplyProps();
}

LRESULT Wnd_InputPane::WndProc(const Windows::Message &msg)
{
   return Dispatch<WindowImpl, Msg::Create, Msg::Close, Msg::Size, Msg::Command, Msg::Notify, Msg::_GetTypeID, Msg::_GetThis>(msg);
}

LRESULT Wnd_InputPane::On(const Msg::Create &msg)
{
   m_input.Create(*this);
   UpdateTitle();
   return msg.Success();
}

LRESULT Wnd_InputPane::On(const Msg::Close &msg)
{
   m_input.GetWndMain().RemoveInputPane(*this);
   return __super::WndProc(msg);
}

LRESULT Wnd_InputPane::On(const Msg::Size &msg)
{
   m_input.SetPosition(ClientRect());
   return msg.Success();
}

LRESULT Wnd_InputPane::On(const Msg::Command &msg)
{
   if(msg.wndCtl()==m_input)
      return m_input.On(msg);

   return msg.Success();
}

LRESULT Wnd_InputPane::On(const Msg::Notify &msg)
{
   if(Window{msg.pnmh()->hwndFrom}==m_input)
      return m_input.On(msg);

   return 0;
}

static Rect s_rcWnd_EditPane;

Wnd_EditPane::Wnd_EditPane(Wnd_Main &wndMain, ConstString title, bool fDockable, bool fSpellCheck)
 : m_wnd_main(wndMain),
   m_title(title),
   m_input(wndMain, m_wnd_main.GetInputWindow().GetProps(), nullptr, this)
{
   Rect rcPrevious=s_rcWnd_EditPane; // Save here as the window will get WM_SIZE messages and overwrite it

   FixedStringBuilder<256> full_title(title ? title : ConstString("Editor"), " - ");
   m_wnd_main.GetConnection().GetWorldTitle(full_title, 0);
   Create(full_title, WS_OVERLAPPEDWINDOW, 0, fDockable ? wndMain : Window());

   if(fDockable)
      mp_docking=&wndMain.CreateDocking(*this);
   else
      CenterInScreen();
   m_input.EnableSpellChecking(fSpellCheck);

   if(rcPrevious!=Rect{})
   {
      SetPosition(rcPrevious);
      EnsureOnScreen();
   }

   Show(SW_SHOWNOACTIVATE);
}

Wnd_EditPane::~Wnd_EditPane()
{
}

LRESULT Wnd_EditPane::WndProc(const Windows::Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Close, Msg::WindowPosChanged, Msg::Command, Msg::SetFocus, Msg::Notify, Msg::_GetTypeID, Msg::_GetThis>(msg);
}

LRESULT Wnd_EditPane::On(const Msg::Create &msg)
{
   m_layout.SetWindowPadding(0);
   auto *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);
   *pGV << m_layout.CreateSpacer(AL::szControlPadding);

   {
      auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);
      pGH->MatchWidth(false);

      m_pButton_send=m_layout.CreateButton(IDC_SEND, "Send");
      m_pButton_edit=m_layout.CreateButton(IDC_EDIT, "Edit...");
      m_pButton_options=m_layout.CreateButton(IDC_OPTIONS, "Options...");
      m_pButton_send->weight(0);
      m_pButton_edit->weight(0);
      m_pButton_options->weight(0);

      SetAllToMax(m_pButton_send->szMinimum().x, m_pButton_edit->szMinimum().x, m_pButton_options->szMinimum().x);

      mp_info=m_layout.CreateStatic("Ln 1/1, Col 1          ");
      mp_info->weight(1);

      *pGH << m_layout.CreateSpacer(AL::szControlPadding) << mp_info << m_pButton_send << m_pButton_edit << m_pButton_options << m_layout.CreateSpacer(AL::szControlPadding);
   }

   m_input.Create(*this);
   auto *pChild=m_layout.AddChildWindow(m_input);
   pChild->szMinimum()=int2(Windows::Controls::Control::m_tmButtons.tmAveCharWidth*80, Windows::Controls::Control::m_tmButtons.tmHeight*20);
   *pGV << pChild;

   return msg.Success();
}

LRESULT Wnd_EditPane::On(const Msg::Close &msg)
{
   m_wnd_main.RemoveEditPane(*this);
   return __super::WndProc(msg);
}

LRESULT Wnd_EditPane::On(const Msg::WindowPosChanged &msg)
{
   if(!IsMinimized() && !IsMaximized())
      s_rcWnd_EditPane=WindowRect()+16*g_dpiScale;
   return __super::WndProc(msg);
}

LRESULT Wnd_EditPane::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_SEND:
         m_wnd_main.GetConnection().Send(m_input.GetText(), false);
         break;

      case IDC_EDIT:
      {
         PopupMenu menu;
         menu.Append(0, 1, "Find...\tCtrl+F");
         menu.AppendSeparator();
         menu.Append(0, 2, "Clear");

         int command=TrackPopupMenu(menu, TPM_RETURNCMD|TPM_NONOTIFY, m_pButton_edit->WindowRect().ptLB(), *this, nullptr);
         switch(command)
         {
            case 1: Find(); break;
            case 2: m_input.SetText({}); break;
         }
         break;
      }

      case IDC_OPTIONS:
      {
         PopupMenu menu;
         menu.Append(m_input.IsSpellChecking() ? MF_CHECKED : 0, 1, "Spellcheck");

         int command=TrackPopupMenu(menu, TPM_RETURNCMD|TPM_NONOTIFY, m_pButton_options->WindowRect().ptLB(), *this, nullptr);
         switch(command)
         {
            case 1: m_input.EnableSpellChecking(!m_input.IsSpellChecking()); break;
         }
         break;
      }
   }

   if(msg.wndCtl()==m_input)
      m_input.On(msg);

   return msg.Success();
}

bool Wnd_EditPane::EditKey(const Msg::Key &msg)
{
   if(msg.iVirtKey()=='F' && !IsKeyPressed(VK_MENU) && IsKeyPressed(VK_CONTROL) && !IsKeyPressed(VK_SHIFT))
   {
      Find();
      return true;
   }

   return false;
}

void Wnd_EditPane::EditSelChanged()
{
   int index=m_input.GetSel().cpMax;
   int line=m_input.LineFromChar(index);
   int col=index-m_input.GetLineIndex(line);
   FixedStringBuilder<256> string{"Ln ", line+1, "/", m_input.GetLineCount(), ", Col ", col+1};
   mp_info->SetText(string);
}

void Wnd_EditPane::Find()
{
   CreateDialog_Find(*this, m_input);
}

LRESULT Wnd_EditPane::On(const Msg::Notify &msg)
{
   if(Window{msg.pnmh()->hwndFrom}==m_input)
      return m_input.On(msg);

   return 0;
}

LRESULT Wnd_EditPane::On(const Msg::SetFocus &msg)
{
   m_input.SetFocus();
   return __super::WndProc(msg);
}


Wnd_EditPropertyPane::Wnd_EditPropertyPane(Wnd_Main &wndMain, Prop::Character &propCharacter)
 : m_wnd_main(wndMain),
   m_input(wndMain, m_wnd_main.GetInputWindow().GetProps(), nullptr, this),
   m_propCharacter(propCharacter)
{
   Create("Character Info", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0, wndMain);

   m_input.SetText(m_propCharacter.pclInfo());
   m_input.LimitText(m_propCharacter.pclInfo_MaxLength());
   m_propCharacter.fInfo_Editing(true);

   mp_docking=&wndMain.CreateDocking(*this);
   Show(SW_SHOWNOACTIVATE);
}

Wnd_EditPropertyPane::~Wnd_EditPropertyPane()
{
   Save();
   m_propCharacter.fInfo_Editing(false);
}

void Wnd_EditPropertyPane::Save()
{
   if(m_input.IsModified())
   {
      m_propCharacter.pclInfo(m_input.GetText());
      m_input.SetModified(false);
   }
}

LRESULT Wnd_EditPropertyPane::WndProc(const Windows::Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Close, Msg::Command, Msg::SetFocus, Msg::Notify, Msg::_GetTypeID, Msg::_GetThis>(msg);
}

LRESULT Wnd_EditPropertyPane::On(const Msg::Create &msg)
{
   m_layout.SetWindowPadding(0);
   auto *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);
   *pGV << m_layout.CreateSpacer(AL::szControlPadding);

   {
      auto *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      auto *pButton_edit=m_layout.CreateButton(IDC_EDIT, "Edit...");
      auto *pButton_options=m_layout.CreateButton(IDC_OPTIONS, "Options...");

      *pGH << m_layout.CreateSpacer(AL::szControlPadding) << pButton_edit << pButton_options << m_layout.CreateSpacer(AL::szControlPadding);
   }

   m_input.Create(*this);
   auto *pChild=m_layout.AddChildWindow(m_input);
   pChild->szMinimum()=int2(Windows::Controls::Control::m_tmButtons.tmAveCharWidth*80, Windows::Controls::Control::m_tmButtons.tmHeight*20);
   *pGV << pChild;

   return msg.Success();
}

LRESULT Wnd_EditPropertyPane::On(const Msg::Close &msg)
{
   return __super::WndProc(msg);
}

LRESULT Wnd_EditPropertyPane::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_EDIT:
      {
         PopupMenu menu;
         menu.Append(0, 1, "Find...\tCtrl+F");

         int command=TrackPopupMenu(menu, TPM_RETURNCMD|TPM_NONOTIFY, Windows::GetMessagePos(), *this, nullptr);
         switch(command)
         {
            case 1: Find(); break;
         }
         break;
      }

      case IDC_OPTIONS:
      {
         PopupMenu menu;
         menu.Append(m_input.IsSpellChecking() ? MF_CHECKED : 0, 1, "Spellcheck");

         int command=TrackPopupMenu(menu, TPM_RETURNCMD|TPM_NONOTIFY, Windows::GetMessagePos(), *this, nullptr);
         switch(command)
         {
            case 1: m_input.EnableSpellChecking(!m_input.IsSpellChecking()); break;
         }
         break;
      }
   }

   if(msg.wndCtl()==m_input)
      m_input.On(msg);

   return msg.Success();
}

bool Wnd_EditPropertyPane::EditKey(const Msg::Key &msg)
{
   if(msg.iVirtKey()=='F' && !IsKeyPressed(VK_MENU) && IsKeyPressed(VK_CONTROL) && !IsKeyPressed(VK_SHIFT))
   {
      Find();
      return true;
   }
         
   return false;
}

void Wnd_EditPropertyPane::Find()
{
   CreateDialog_Find(*this, m_input);
}

LRESULT Wnd_EditPropertyPane::On(const Msg::Notify &msg)
{
   if(Window{msg.pnmh()->hwndFrom}==m_input)
      return m_input.On(msg);

   return 0;
}

LRESULT Wnd_EditPropertyPane::On(const Msg::SetFocus &msg)
{
   m_input.SetFocus();
   return __super::WndProc(msg);
}
