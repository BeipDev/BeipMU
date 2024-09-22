struct Image;
struct Connection;

namespace GMCP
{

void On_Stats(Wnd_Main &wnd, ConstString json);
void On_RoomInfo(Wnd_Main &wnd, ConstString json);

struct Avatar_Changed { bool url_changed; };

struct Avatar_Info : Events::SendersOf<Avatar_Changed>
{
   Avatar_Info()=default;

   OwnedString m_text;
   Text::ImageURL m_url;
   OwnedString m_url_click;

private:
   Avatar_Info(const Avatar_Info &)=delete;
   void operator=(const Avatar_Info &)=delete;
};

struct Avatars
{
   Avatars(Connection &connection) : m_connection{connection} { }

   void OnGMCP(ConstString json);
   Avatar_Info &Lookup(ConstString id);

private:

   std::unordered_map<OwnedString, Avatar_Info> m_map;
   Connection &m_connection;
};

}
