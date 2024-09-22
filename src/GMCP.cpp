#include "Main.h"
#include "JSON.h"
#include "Connection.h"
#include "Wnd_Stats.h"
#include "Wnd_main.h"
#include "Wnd_Map.h"
#include "GMCP.h"

Color Translate256(unsigned i); // From AnsiParser.cpp

namespace GMCP
{

bool Parse_Color(ConstString value, Color &color)
{
   Streams::Input stream{value};

   if(stream.StringSkip("ansi256("))
   {
      unsigned index;
      if(!stream.Parse(index) || !stream.CharSkip(')') || index>255)
         return false;

      color=Translate256(index);
      return true;
   }

   if(stream.StringSkip("transparent"))
   {
      color=Colors::Transparent;
      return true;
   }

   return stream.Parse_RGB(color);
}

struct JSON_Stat_Range : JSON::Element
{
   JSON_Stat_Range(Stats::Types::Range &range) : m_range{range} { }

   void OnString(ConstString name, ConstString value) override
   {
      if(name=="fill-color" || name=="bar-fill")
         Parse_Color(value, m_range.m_fill_color);
      else if(name=="empty-color")
         Parse_Color(value, m_range.m_empty_color);
      else if(name=="outline-color")
         Parse_Color(value, m_range.m_outline_color);
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      if(name=="value")
         m_range.m_value=value;
      else if(name=="min")
         m_range.m_lower=value;
      else if(name=="max")
         m_range.m_upper=value;
      else
         Assert(false);
   }

private:

   Stats::Types::Range &m_range;
};

struct JSON_Stat_Progress : JSON::Element
{
   JSON_Stat_Progress(Stats::Types::Progress &progress) : m_progress{progress} { }

   void OnString(ConstString name, ConstString value) override
   {
      if(name=="fill-color")
         Parse_Color(value, m_progress.m_fill_color);
      else if(name=="empty-color")
         Parse_Color(value, m_progress.m_empty_color);
      else if(name=="outline-color")
         Parse_Color(value, m_progress.m_outline_color);
      else if(name=="label")
         m_progress.m_label=value;
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      if(name=="value")
         m_progress.m_value=value;
      else
         Assert(false);
   }

private:

   Stats::Types::Progress &m_progress;
};

struct JSON_Stat : JSON::Element
{
   JSON_Stat(Stats::Wnd &wnd, Stats::Item &item) : m_wnd{wnd}, m_item{item} { }

   void OnString(ConstString name, ConstString value) override
   {
      if(name=="string")
      {
         m_item.m_type=Stats::Types::String{value};
      }
      else if(name=="name-alignment")
      {
         if(value=="left")
            m_item.m_name_alignment=Stats::Alignment::Left;
         else if(value=="center")
            m_item.m_name_alignment=Stats::Alignment::Center;
         else if(value=="right")
            m_item.m_name_alignment=Stats::Alignment::Right;
      }
      else if(name=="color")
      {
         Color color;
         if(Parse_Color(value, color))
         {
            m_item.m_name_color=color;
            m_item.m_value_color=color;
         }
      }
      else if(name=="name-color")
         Parse_Color(value, m_item.m_name_color);
      else if(name=="value-color")
         Parse_Color(value, m_item.m_value_color);
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      if(name=="int")
      {
         m_item.m_type=Stats::Types::Int{int(value)};
      }
      else if(name=="prefix-length")
      {
         m_item.m_prefix_length=value;
         PinBelow(m_item.m_prefix_length, m_item.m_name.Count());
      }
      else
         Assert(false);
   }

   JSON::Element &OnObject(ConstString name) override
   {
      if(name=="range")
      {
         m_pStat_range.Empty();
         if(!m_item.m_type.Is<Stats::Types::Range>())
            m_item.m_type=Stats::Types::Range();
         return m_pStat_range.New(m_item.m_type.Get<Stats::Types::Range>());
      }
      else if(name=="progress")
      {
         mp_stat_progress.Empty();
         if(!m_item.m_type.Is<Stats::Types::Progress>())
            m_item.m_type=Stats::Types::Progress();
         return mp_stat_progress.New(m_item.m_type.Get<Stats::Types::Progress>());
      }

      Assert(false);
      return __super::OnObject(name);
   }

   void OnComplete(bool empty) override
   {
      if(empty)
      {
         m_wnd.Delete(m_item.m_name);
         return;
      }
   }

   Stats::Wnd &m_wnd;
   Stats::Item &m_item;

   PlacementUniquePtr<JSON_Stat_Range> m_pStat_range;
   PlacementUniquePtr<JSON_Stat_Progress> mp_stat_progress;
};

struct JSON_Stats_Values : JSON::Element
{
   JSON_Stats_Values(Stats::Wnd &wnd) : m_wnd{wnd} { }

   void OnString(ConstString name, ConstString value) override
   {
      Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      Assert(false);
   }

   void OnNull(ConstString name) override
   {
      m_wnd.Delete(name);
   }

   JSON::Element &OnObject(ConstString name) override
   {
      m_pStat.Empty();
      return m_pStat.New(m_wnd, m_wnd.Get(name));
   }

   Stats::Wnd &m_wnd;
   PlacementUniquePtr<JSON_Stat> m_pStat;
};

struct JSON_Stats_Window : JSON::Element
{
   JSON_Stats_Window(Stats::Wnd &wnd) : m_wnd{wnd}, m_stats_values{m_wnd} { }

   void OnString(ConstString name, ConstString value) override
   {
      if(name=="background-color")
      {
         Color color;
         if(Parse_Color(value, color))
            m_wnd.SetBackground(color);
      }
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      Assert(false);
   }

   void OnNull(ConstString name) override
   {
      if(name=="values")
         m_wnd.DeleteAll();
      else
         Assert(false);
   }

   JSON::Element &OnObject(ConstString name) override
   {
      if(name=="values")
         return m_stats_values;
      Assert(false);
      return __super::OnObject(name);
   }

   Stats::Wnd &m_wnd;
   JSON_Stats_Values m_stats_values;
};

struct JSON_Stats : JSON::Element
{
   JSON_Stats(Wnd_Main &wnd) : m_wnd{wnd} { }

   void OnString(ConstString name, ConstString value) override
   {
      Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      Assert(false);
   }

   JSON::Element &OnObject(ConstString name) override
   {
      m_pStats_window.Empty();
      return m_pStats_window.New(m_wnd.GetStatsWindow(name));
   }

   Wnd_Main &m_wnd;
   PlacementUniquePtr<JSON_Stats_Window> m_pStats_window;
};

void On_Stats(Wnd_Main &wnd, ConstString json)
{
   JSON_Stats element{wnd};
   JSON::ParseObject(element, json);
}

//
//
//

/*
room.info {
    "id": "1|1511929083#5301",
    "area": "Arborwatch Manor",
    "name": "Lounge",
    "flags": [],
    "coords": {
        "floor": -1,
        "x": -100,
        "y": -100
    },
    "size": {
        "x": 10,
        "y": 10
    },
    "description": "<description here>"
    "exits": [{
        "description": "",
        "destination": "1|1511929083#5358",
        "direction": "up",
        "id": "1|1511929083#5356",
        "name": "[U] Third Floor Corridor;up;u"
    }]
}
*/

struct JSON_RoomCoords : JSON::Element
{
   void OnNumber(ConstString name, double value) override
   {
      if(name=="floor")
         m_floor=value;
      else if(name=="x")
         m_pos.x=value;
      else if(name=="y")
         m_pos.y=value;
      else
         Assert(false);
   }

   int m_floor{};
   int2 m_pos;
};

struct JSON_RoomSize : JSON::Element
{
   void OnNumber(ConstString name, double value) override
   {
      if(name=="x")
         m_size.x=value;
      else if(name=="y")
         m_size.y=value;
      else
         Assert(false);
   }

   int2 m_size;
};

struct JSON_RoomExit : JSON::Element
{
   void OnString(ConstString name, ConstString value) override
   {
      if(name=="destination")
      {
      }
      else if(name=="direction")
      {
      }
      else if(name=="id")
      {
      }
      else if(name=="name")
      {
      }
      else
         Assert(false);
   }
};

struct JSON_RoomExits : JSON::Element
{
   JSON::Element &OnObject(ConstString name) override
   {
      return m_exits.Push();
//      m_pStats_window.Empty();
//      return m_pStats_window.New(m_wnd.GetStatsWindow(name));
   }

   Collection<JSON_RoomExit> m_exits;
};

struct JSON_RoomInfo : JSON::Element
{
   JSON_RoomInfo(Wnd_Main &wnd) : m_wnd{wnd} { }

   void OnString(ConstString name, ConstString value) override
   {
      if(name=="area")
      {
         m_area=value;
      }
      else if(name=="description")
      {

      }
      else if(name=="id")
      {

      }
      else if(name=="name")
      {
         m_name=value;
      }
      else
         Assert(false);
   }

   void OnNumber(ConstString name, double value) override
   {
      Assert(false);
   }

   JSON::Element &OnArray(ConstString name) override
   {
      if(name=="exits")
         return m_exits;
      return __super::OnArray(name);
   }

   JSON::Element &OnObject(ConstString name) override
   {
      if(name=="coords")
         return m_coords;
      if(name=="size")
         return m_size;

      return __super::OnObject(name);
   }

   void OnComplete(bool empty) override
   {
      Rect rect=Rect(m_size.m_size)+m_coords.m_pos;
      if(rect.size().x<=0 && rect.size().y<=0 || !m_area || !m_name)
         return;

      auto &map_wnd=m_wnd.EnsureMapWindow();
      map_wnd.EnsureMap(m_area);

      map_wnd.EnsureRoom(m_name, rect);
      map_wnd.CenterOnPosition(rect.center());
   }

   Wnd_Main &m_wnd;
   OwnedString m_area;
   OwnedString m_name;

   JSON_RoomCoords m_coords;
   JSON_RoomSize m_size;
   JSON_RoomExits m_exits;
//   PlacementUniquePtr<JSON_Stats_Window> m_pStats_window;
};

void On_RoomInfo(Wnd_Main &wnd, ConstString json)
{
   JSON_RoomInfo element{wnd};
   JSON::ParseObject(element, json);
}

//
//
//

struct JSON_ID : JSON::Element
{
   JSON_ID(Avatars &avatars, Avatar_Info &info) : m_avatars{avatars}, m_info{info} { }

   void OnString(ConstString name, ConstString value) override
   {
      if(name=="hover-text")
      {
         m_info.m_text=value;
      }
      else if(name=="url")
      {
         m_info.m_url.m_original=value;
         m_url_changed=true;
      }
      else if(name=="click-url")
      {
         m_info.m_url_click=value;
      }
      else if(name=="hover-url")
      {
         
      }
      else
         Assert(false);
   }

   void OnComplete(bool empty) override
   {
      m_info.Send(Avatar_Changed{.url_changed=m_url_changed});
   }

   Avatars &m_avatars;
   Avatar_Info &m_info;
   bool m_url_changed{};
};

struct JSON_IDs : JSON::Element
{
   JSON_IDs(Avatars &avatars) : m_avatars{avatars} { }

   JSON::Element &OnObject(ConstString name) override
   {
      mp_id.Empty();
      mp_id.New(m_avatars, m_avatars.Lookup(name));
      return *mp_id;
   }

   Avatars &m_avatars;
   PlacementUniquePtr<JSON_ID> mp_id;
};

void Avatars::OnGMCP(ConstString json)
{
   JSON_IDs element{*this};
   JSON::ParseObject(element, json);
}

Avatar_Info &Avatars::Lookup(ConstString id)
{
   // Do we already know about the id?
   if(auto iter=m_map.find(id);iter!=m_map.end())
      return iter->second;

   if(m_connection.IsConnected())
   {
      // We don't know about it, so request it from the server
      m_connection.RawSend(ConstString(GMCP_BEGIN "beip.id.request \""));
      m_connection.RawSend(id);
      m_connection.RawSend(ConstString("\"" GMCP_END));
   }

   return m_map.try_emplace(id).first->second;
}

}
