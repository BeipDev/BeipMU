struct Wnd_TileMap;
struct IWnd_TileMap
{
   virtual ConstString GetTitle()=0;
   virtual Wnd_Docking &GetDocking()=0;
};

struct TileMaps
{
   TileMaps(Wnd_Main &wndMain);
   ~TileMaps();

   void On(ConstString command, ConstString json);

   Wnd_TileMap *Find(ConstString name);
   IWnd_TileMap &Create(ConstString name);

private:
   Wnd_Main &m_wnd_main;
   OwnedDLNodeList<Wnd_TileMap> m_maps;

   friend struct Wnd_TileMap;
};
