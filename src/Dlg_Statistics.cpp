#include "Main.h"

struct Dlg_Statistics : Wnd_Dialog, Singleton<Dlg_Statistics>
{
   Dlg_Statistics(Window wndParent);
   ~Dlg_Statistics();

private:

   LRESULT WndProc(const Message &msg) override;
   friend TWindowImpl;
   LRESULT On(const Msg::Create &msg);
   LRESULT On(const Msg::Command &msg);
   LRESULT On(const Msg::Notify &msg);

   enum
   {
      IDC_LIST=100,
      IDC_DONE,
   };

   AL::Button *m_pbtDone;

   struct ListItem
   {
      CntPtrTo<Prop::Server>    mp_server;
      CntPtrTo<Prop::Character> mp_character;
      int m_days_to_deletion;
      int m_last_used;
   };

   int Sort(const ListItem &item1, const ListItem &item2) const;
   unsigned m_sort_by{};
   bool m_sort_descending{};

   Controls::ListView m_lv;
};

int Dlg_Statistics::Sort(const ListItem &item1, const ListItem &item2) const
{
   int comparison=0;

   switch(m_sort_by)
   {
      case 0: comparison=item1.mp_character->pclName().ICompare(item2.mp_character->pclName()); break;
      case 1: 
         comparison=item1.mp_server->pclName().ICompare(item2.mp_server->pclName());
         if(comparison==0)
            comparison=item1.mp_character->pclName().ICompare(item2.mp_character->pclName());
         break;
      case 2: comparison=item1.m_days_to_deletion-item2.m_days_to_deletion; break;
      case 3: comparison=item1.m_last_used-item2.m_last_used; break;
      case 4: comparison=int32(item2.mp_character->iConnectionCount())-int32(item1.mp_character->iConnectionCount()); break;
      case 5: comparison=int32(item2.mp_character->iSecondsConnected())-int32(item1.mp_character->iSecondsConnected()); break;
      case 6: comparison=int64(item2.mp_character->iBytesSent()+item2.mp_character->iBytesReceived())-
                         int64(item1.mp_character->iBytesSent()+item1.mp_character->iBytesReceived()); break;
   }

   if(m_sort_descending)
      return -comparison;
   return comparison;
}

LRESULT Dlg_Statistics::WndProc(const Message &msg)
{
   return Dispatch<Wnd_Dialog, Msg::Create, Msg::Command, Msg::Notify>(msg);
}

Dlg_Statistics::Dlg_Statistics(Window wndParent)
{
   Create("Statistics", WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_VISIBLE, 0 /*dwExStyle*/, wndParent);
}

Dlg_Statistics::~Dlg_Statistics()
{
   m_lv.Destroy(); // To handle tree item delete callbacks before we are deleted
}

LRESULT Dlg_Statistics::On(const Msg::Create &msg)
{
   m_lv.Create(*this, IDC_LIST, WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SORTASCENDING | LVS_SHOWSELALWAYS);
   m_lv.SetExtendedStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

   m_pbtDone=m_layout.CreateButton(IDC_DONE, STR_Done);
   m_pbtDone->SetStyle(BS_DEFPUSHBUTTON, true);
   m_defID=IDC_DONE;

   AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); m_layout.SetRoot(pGV);

   *pGV << m_layout.AddControl(m_lv);

   {
      AL::Group *pGH=m_layout.CreateGroup_Horizontal(); *pGV << pGH;
      pGH->weight(0);

      AL::Group_Vertical *pGV=m_layout.CreateGroup_Vertical(); *pGH << pGV;
      pGV->MatchHeight(true);
      *pGV << m_pbtDone;
   }

   Controls::ListView::Column lvc;
   lvc.Text("Name");
   lvc.Width(50);
   m_lv.InsertColumn(0, lvc);

   lvc.Text("Server");
   m_lv.InsertColumn(1, lvc);

   lvc.Text("Days to Deletion");
   lvc.Format(LVCFMT_RIGHT);
   m_lv.InsertColumn(2, lvc);

   lvc.Text("Last Time Used");
   lvc.Format(LVCFMT_RIGHT);
   m_lv.InsertColumn(3, lvc);

   lvc.Text("Times Connected");
   lvc.Format(LVCFMT_RIGHT);
   m_lv.InsertColumn(4, lvc);

   lvc.Text("Connected Time");
   lvc.Format(LVCFMT_RIGHT);
   m_lv.InsertColumn(5, lvc);

   lvc.Text("Total Bytes");
   lvc.Format(LVCFMT_RIGHT);
   m_lv.InsertColumn(6, lvc);

   for(auto& pServer : g_ppropGlobal->propConnections().propServers())
   {
      for(auto &pCharacter : pServer->propCharacters())
      {
         FixedStringBuilder<64> stringLast, stringDays;
         const Time::Time &time=pCharacter->timeLastUsed();

         int daysToDeletion=std::numeric_limits<int>::max();
         int daysSinceUse=std::numeric_limits<int>::max();

         if(time.fNone())
            stringDays << '?';
         else if(time.fRunning())
         {
            stringLast(STR_CurrentlyInUse);
            stringDays(pServer->iCharacterExpirationTime(), " Days");
         }
         else
         {
            daysSinceUse=(Time::Local()-time).ToDays();

            if(pServer->iCharacterExpirationTime()==0)
               stringDays('?');
            else
            {
               daysToDeletion=pServer->iCharacterExpirationTime()-daysSinceUse;
               if(daysToDeletion>=0)
                  stringDays(daysToDeletion, " Days");
               else
                  stringDays("Expired");
            }

            stringLast(STR_SincePrefix);
            if(daysSinceUse==0)
               stringLast("0 Days");
            else
               Time::SecondsToString(stringLast, daysSinceUse*(60*60*24));
            stringLast(STR_SinceSuffix);
         }

         Controls::ListView::Item lvi;
         lvi.Text(pCharacter->pclName());
         lvi.Param((LPARAM)new ListItem{pServer, pCharacter, daysToDeletion, daysSinceUse});
         int itemnum=m_lv.InsertItem(lvi);
         m_lv.SetItemText(itemnum, 1, pServer->pclName());
         m_lv.SetItemText(itemnum, 2, stringDays);
         m_lv.SetItemText(itemnum, 3, stringLast);
         m_lv.SetItemText(itemnum, 4, FixedStringBuilder<256>(pCharacter->iConnectionCount()));
         FixedStringBuilder<256> timeConnected; Time::SecondsToStringAbbreviated(timeConnected, pCharacter->iSecondsConnected());
         m_lv.SetItemText(itemnum, 5, timeConnected);

         FixedStringBuilder<256> totalBytes;
         ByteCountToStringAbbreviated(totalBytes, pCharacter->iBytesSent()+pCharacter->iBytesReceived());
         m_lv.SetItemText(itemnum, 6, totalBytes);
      }
   }

   m_lv.SetColumnWidth(0, LVSCW_AUTOSIZE);
   m_lv.SetColumnWidth(1, LVSCW_AUTOSIZE);
   m_lv.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
   m_lv.SetColumnWidth(3, LVSCW_AUTOSIZE_USEHEADER);
   m_lv.SetColumnWidth(4, LVSCW_AUTOSIZE_USEHEADER);
   m_lv.SetColumnWidth(5, LVSCW_AUTOSIZE_USEHEADER);
   m_lv.SetColumnWidth(6, LVSCW_AUTOSIZE_USEHEADER);

   unsigned columnWidth=0;
   for(unsigned i=0;i<7;i++)
      columnWidth+=m_lv.GetColumnWidth(i);

   m_lv.szMinimum().x=columnWidth+40;
   Rect rcItem; m_lv.GetItemRect(0, rcItem, LVIR_LABEL);
   m_lv.szMinimum().y=rcItem.size().y*20;

   m_lv.SetFocus();

   return msg.Success();
}

LRESULT Dlg_Statistics::On(const Msg::Notify &msg)
{
   if(Window{msg.pnmh()->hwndFrom}!=m_lv)
      return 0;

   auto &nm=*(Controls::ListView::NM *)msg.pnmh();

   switch(nm.hdr.code)
   {
      case LVN_COLUMNCLICK:
      {
         if(m_sort_by==unsigned(nm.iSubItem))
            m_sort_descending^=true;
         else
         {
            m_sort_descending=false;
            m_sort_by=nm.iSubItem;
         }

         auto callback=[](LPARAM param1, LPARAM param2, LPARAM userdata)
         {
            return reinterpret_cast<Dlg_Statistics*>(userdata)->Sort(*reinterpret_cast<ListItem*>(param1), *reinterpret_cast<ListItem*>(param2));
         };

         m_lv.SortItems(callback, this);
         return 0;
      }

      case LVN_DELETEITEM:
         delete reinterpret_cast<ListItem *>(nm.lParam);
         return 0;
   }

   return 0;
}

LRESULT Dlg_Statistics::On(const Msg::Command &msg)
{
   switch(msg.iID())
   {
      case IDCANCEL:
      case IDC_DONE:
         Close();
         break;
   }

   return msg.Success();
}

void CreateDialog_Statistics(Window wndParent)
{
   if(Dlg_Statistics::HasInstance())
      Dlg_Statistics::GetInstance().SetFocus();
   else
      new Dlg_Statistics(wndParent);
}
