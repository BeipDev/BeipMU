#include "Main.h"
#include "Connection.h"
#include "Wnd_Main.h"

struct Dlg_SmartPaste : Wnd_Dialog
{
   Dlg_SmartPaste(Window wndParent, Connection &connection, Prop::Connections &propConnections);

   void Paste();
   void Paste(ConstString text, ConstString prefix, ConstString suffix, float delay);

private:

   ~Dlg_SmartPaste();

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;

   // Window Messages
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Paint &msg);

   enum
   {
      IDC_PASTE   = 100,
   };

   AL::Radio *m_pcbSource_Clipboard, *m_pcbSource_File;
   AL::Edit *m_pedPrefix, *m_pedSuffix;
   AL::Edit *m_pedDelay;

   AL::Static *m_pstClipboard; // Clipboard Contents Preview
   AL::Static *m_pstClipboardSize; // Size of clipboard data

   OwnedString m_clipboardText;

   Connection &m_connection;
   Prop::Connections &m_propConnections;
};

LRESULT Dlg_SmartPaste::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command, Msg::Paint>(msg);
}

Dlg_SmartPaste::Dlg_SmartPaste(Window wndParent, Connection &connection, Prop::Connections &propConnections)
 : m_connection(connection), m_propConnections(propConnections)
{
   Create(STR_Title_SmartPaste, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE | Wnd_Dialog::Style_Modal, 0 /*dwExStyle*/, wndParent);
}

Dlg_SmartPaste::~Dlg_SmartPaste()
{
}

void Dlg_SmartPaste::Paste()
{
   auto prefix=m_pedPrefix->GetText();
   auto suffix=m_pedSuffix->GetText();
   float delay{};
   m_pedDelay->GetText().To(delay);

   m_propConnections.pclPastePrefix(prefix);
   m_propConnections.pclPasteSuffix(suffix);

   if(m_pcbSource_Clipboard->IsChecked())
   {
      Paste(m_clipboardText, prefix, suffix, delay);
      return;
   }

   if(m_pcbSource_File->IsChecked())
   {
      File::Chooser cf;
      cf.SetTitle(STR_FileToPaste);
      cf.SetFilter(STR_PasteFileFilter, 0);

      File::Path filename;
      if(!cf.Choose(*this, filename, false))
         return;

      File::Read_Only file(filename);
      if(!file)
      {
         MessageBox(*this, STR_FileNotFound, STR_Error, MB_OK|MB_ICONEXCLAMATION);
         return;
      }

      if(file.Size()>65536 && MessageBox(*this, STR_FileLarger64K, "Note:", MB_YESNO|MB_ICONQUESTION)==IDNO)
         return;

      auto buffer=file.Read();
      if(!buffer)
      {
         MessageBox(*this, STR_CantReadFile, STR_Error, MB_OK|MB_ICONSTOP);
         return;
      }

      Paste(ConstString((const char *)&*buffer.begin(), buffer.Count()), prefix, suffix, delay);
   }
}

void Dlg_SmartPaste::Paste(ConstString text, ConstString prefix, ConstString suffix, float delay)
// Actually Paste the Text
{
   float time{};
   unsigned lines_pasted=0;
   while(text)
   {
      ConstString line;
      if(!text.Split(CHAR_LF, line, text))
      {
         line=text; text=ConstString(); // Line is the end, and there's nothing after this
      }

      HybridStringBuilder<> string(prefix, line, suffix);

      // Strip the CR
      if(string.EndsWith(CHAR_CR))
         string.AddLength(-1);

      if(delay)
         m_connection.GetMainWindow().DelaySend(string, time+=delay);
      else
         m_connection.Send(string, false);
      lines_pasted++;
   }

   m_connection.Text(FixedStringBuilder<256>("<font color='cyan'>", lines_pasted, STR_LinesPasted));
}

LRESULT Dlg_SmartPaste::On(const Msg::Create &msg)
{
   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   {
      AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;

      {
         AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Source); *pGH << pGB; pGB->weight(0);
         AL::Group *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV); pGV->weight(0);

         m_pcbSource_Clipboard=m_layout.CreateRadio(-1, STR_Clipboard, true);
         m_pcbSource_File=m_layout.CreateRadio(-1, STR_File);

         *pGV << m_pcbSource_Clipboard << m_pcbSource_File;
      }
      {
         AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_ClipboardPreview); *pGH << pGB;
         AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); pGB->SetChild(pGV);

         m_pstClipboard=m_layout.CreateStatic(" ", SS_ETCHEDFRAME); m_pstClipboard->weight(1);
         m_pstClipboard->szMinimum()=int2(80, 80);
         m_pstClipboardSize=m_layout.CreateStatic(" ", SS_SUNKEN);

         *pGV << m_pstClipboard << m_pstClipboardSize;
      }
   }
   {
      AL::GroupBox *pGB=m_layout.CreateGroupBox(""); *pGV << pGB; pGB->weight(0);

      AL::Group *pG=m_layout.CreateGroup(Direction::Vertical);
      pGB->SetChild(pG);

      m_pedPrefix=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
      m_pedSuffix=m_layout.CreateEdit(-1, int2(30, 1), ES_AUTOHSCROLL);
      m_pedDelay=m_layout.CreateEdit(-1, int2(30, 1));

      AL::Static *pPrefix=m_layout.CreateStatic(STR_Prefix);
      AL::Static *pSuffix=m_layout.CreateStatic(STR_Suffix);

      SetAllToMax(pPrefix->szMinimum().x, pSuffix->szMinimum().x);

      {
         AL::Group *pG1=m_layout.CreateGroup(Direction::Horizontal);
         *pG1 << pPrefix << m_pedPrefix;
         *pG << pG1;
      }
      {
         AL::Group *pG1=m_layout.CreateGroup(Direction::Horizontal);
         *pG1 << pSuffix << m_pedSuffix;
         *pG << pG1;
      }
      {
         AL::Group *pG1=m_layout.CreateGroup(Direction::Horizontal);
         *pG1 << m_layout.CreateStatic("Delay between lines (seconds)") << m_pedDelay;
         *pG << pG1;
      }
      *pG << m_layout.CreateStatic("To stop a running delayed send, use command: \\delay killall");
   }

   {
      AL::Group_Horizontal *pG=m_layout.CreateGroup_Horizontal(); *pGV << pG;
      pG->weight(0); *pG >> AL::Style::Attach_Left; pG->MatchWidth(true);

      AL::Button *pbtPaste=m_layout.CreateButton(IDC_PASTE, STR_Paste);
      AL::Button *pbtCancel=m_layout.CreateButton(IDCANCEL, STR_Cancel);

      pbtPaste->SetStyle(BS_DEFPUSHBUTTON, true);

      *pG << pbtPaste << pbtCancel;
   }

   // TODO: Move the next set of ifs into this first set
   m_clipboardText=::Clipboard::GetText();
   if(!m_clipboardText)
      m_pcbSource_Clipboard->Enable(false);
   else
   {
      FixedStringBuilder<256> string(STR_Bytes, m_clipboardText.Length());
      m_pstClipboardSize->SetText(string);
   }

   if(m_clipboardText)
      m_pcbSource_Clipboard->Check(true);
   else
      m_pcbSource_File->Check(true);

   m_pedPrefix->LimitText(Prop::Connections::pclPastePrefix_MaxLength());
   m_pedSuffix->LimitText(Prop::Connections::pclPasteSuffix_MaxLength());

   m_pedPrefix->SetText(m_propConnections.pclPastePrefix());
   m_pedSuffix->SetText(m_propConnections.pclPasteSuffix());
   m_pedDelay->SetText("0.0");

   m_defID=IDC_PASTE;

   return msg.Success();
}

LRESULT Dlg_SmartPaste::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);

   Rect rcClipboard=ScreenToClient(m_pstClipboard->WindowRect());
   rcClipboard.Inset(1);

   ps.FillRect(rcClipboard, (HBRUSH)GetStockObject(WHITE_BRUSH));
   rcClipboard.Inset(1);

   if(m_clipboardText)
   {
      DC::FontSelector _(ps, Windows::Controls::Control::m_font_buttons);
      ps.DrawText(m_clipboardText, rcClipboard, DT_NOPREFIX);
   }
   return msg.Success();
}

LRESULT Dlg_SmartPaste::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDC_PASTE:
         Paste();
      case IDCANCEL:
         Close();
         break;
   }

   return msg.Success();
}

void CreateDialog_SmartPaste(Window wnd, Connection &connection, Prop::Connections &propConnections)
{
   new Dlg_SmartPaste(wnd, connection, propConnections);
}
