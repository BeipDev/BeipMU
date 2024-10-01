//
// Keyboard Shortcuts Dialog
//

#include "Main.h"
#include "Dlg_Key.h"


ATOM Wnd_Key::Register()
{
   WndClass wc(L"Wnd_Key");
   wc.LoadIcon(IDI_APP, g_hInst);
   wc.hCursor=LoadCursor(nullptr, IDC_ARROW);
   wc.hbrBackground=CreateSolidBrush(GetSysColor(COLOR_3DFACE));

   return wc.Register();
}

LRESULT Wnd_Key::WndProc(const Message &msg)
{
   return Dispatch<WindowImpl, Msg::Create, Msg::Close, Msg::Size, Msg::KeyDown, Msg::SysKeyDown, Msg::LButtonDown>(msg);
}

Wnd_Key::Wnd_Key(Window wndParent, INotify &notify) : m_notify(notify)
{
   Create("", WS_POPUP | WS_DLGFRAME | WS_VISIBLE, Position_Default, wndParent);
   Owner().Enable(false);
}

Wnd_Key::~Wnd_Key()
{
   m_notify.Release();
}

LRESULT Wnd_Key::On(const Msg::Create &msg)
{
   AL::LayoutEngine::SetWindow(*this);
   AL::Group_Vertical *pGV=CreateGroup_Vertical(); SetRoot(pGV);
   *pGV >> AL::Style::Attach_All;
   *pGV << CreateStatic(STR_PressKeyCombination, SS_CENTER);

   int2 size=CalcMinSize();
   size+=size.y*2;
   SetMinSize(size);

   SetPosition(GetMinSize());
   CenterInParent();

   return msg.Success();
}

LRESULT Wnd_Key::On(const Msg::Close &msg)
{
   Owner().Enable(true);
   Owner().SetFocus();
   Destroy();
   return msg.Success();
}

LRESULT Wnd_Key::On(const Msg::Key &msg)
{
   KEY_ID key;

   key.iVKey=msg.iVirtKey();
   key.fControl=(GetKeyState(VK_CONTROL)&0x80)!=0;
   key.fAlt=msg.fAlt();
   key.fShift=(GetKeyState(VK_SHIFT)&0x80)!=0;

   if(key.iVKey==VK_CONTROL || key.iVKey==VK_SHIFT || key.iVKey==VK_MENU)
      return msg.Success();

   m_notify.Key(key);
   Close();

   return msg.Success();
}

LRESULT Wnd_Key::On(const Msg::LButtonDown &msg)
{
   Close();
   return msg.Success();
}

//
//
//
LRESULT Dlg_Key::WndProc(const Message &msg)
{
   return Dispatch<Wnd_ChildDialog, Msg::Create, Msg::Enable, Msg::Command>(msg);
}

Dlg_Key::Dlg_Key(Window wndParent)
{
   Create("", wndParent);
}

void Dlg_Key::SetKey(KEY_ID key)
{
   m_key=key;

   FixedStringBuilder<256> sBuffer; m_key.KeyName(sBuffer);

   m_pedKey->SetText(sBuffer);

   m_pcbControl->Check(m_key.fControl);
   m_pcbAlt    ->Check(m_key.fAlt);
   m_pcbShift  ->Check(m_key.fShift);
}

const KEY_ID &Dlg_Key::GetKey()
{
   m_key.fControl=m_pcbControl->GetTristateCheck();
   m_key.fAlt    =m_pcbAlt->GetTristateCheck();
   m_key.fShift  =m_pcbShift->GetTristateCheck();

   return m_key;
}

//
// Window Messages
//
LRESULT Dlg_Key::On(const Msg::Create &msg)
{
   AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Key); m_layout.SetRoot(pGB);
   AL::Group *pG=m_layout.CreateGroup(Direction::Vertical); pGB->SetChild(pG);

   {
      AL::Group *pG1=m_layout.CreateGroup_Horizontal(); *pG << pG1;

      m_pedKey=m_layout.CreateEdit(-1, int2(10, 1), ES_CENTER);
      m_pbtKey=m_layout.CreateButton(0, STR_PressKey___);

      *pG1 << m_layout.CreateStatic(STR_PressKeyHere) << m_pedKey << m_pbtKey;
   }
   {
      AL::GroupBox *pGB=m_layout.CreateGroupBox(STR_Modifiers); *pG << pGB;
      AL::Group_Horizontal *pG=m_layout.CreateGroup_Horizontal(); pGB->SetChild(pG);

      m_pcbControl=m_layout.CreateCheckBox(-1, STR_Control, BS_AUTO3STATE);
      m_pcbAlt=m_layout.CreateCheckBox(-1, STR_Alt, BS_AUTO3STATE);
      m_pcbShift=m_layout.CreateCheckBox(-1, STR_Shift, BS_AUTO3STATE);

      *pG << m_pcbControl << m_pcbAlt << m_pcbShift;
   }

   Init_EditSendEnter(*m_pedKey, *this);
   return msg.Success();
}

LRESULT Dlg_Key::On(const Msg::Enable &msg)
{
   EnableWindows(msg.fEnabled(), *m_pedKey, *m_pbtKey, *m_pcbControl, *m_pcbAlt, *m_pcbShift);
   return msg.Success();
}

LRESULT Dlg_Key::On(const Msg::Command &msg)
{
   struct Notify : Wnd_Key::INotify
   {
      Notify(Dlg_Key &dlg) : m_dlg(dlg) { }

      void Key(const KEY_ID &key) override { m_dlg.SetKey(key); }
      void Release() override { delete this; }

   private:
      Dlg_Key &m_dlg;
   };

   if(msg.wndCtl()==*m_pbtKey)
   {
      new Wnd_Key(*this, *new Notify(*this));
      return msg.Success();
   }

   return msg.Failure();
}

bool Dlg_Key::EditKey(const Msg::Key &msg)
{
   if(msg.direction()==Direction::Up) return true;

   m_key.iVKey=msg.iVirtKey();
   m_key.fControl=IsKeyPressed(VK_CONTROL);
   m_key.fAlt=IsKeyPressed(VK_MENU);
   m_key.fShift=IsKeyPressed(VK_SHIFT);

   m_pcbControl->Check(m_key.fControl);
   m_pcbAlt    ->Check(m_key.fAlt);
   m_pcbShift  ->Check(m_key.fShift);

   {
      FixedStringBuilder<256> sBuffer; m_key.KeyName(sBuffer);
      m_pedKey->SetText(sBuffer);
   }

   return true;
}
