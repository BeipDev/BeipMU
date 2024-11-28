#include "Main.h"
#include "JSON.h"
#include "Wnd_Main.h"
#include "TileMap.h"

#include "ZLib-1.2.8\Interface.h"

// <tilemap tiles="http://fs.com/Tiles32x32.png" tile_size="32,32" offset="32" title="Overworld map" size="16,9">(144 printable ascii chars)

enum struct Encoding
{
   Hex_4,
   Hex_8,
   Base64_8,
   ZBase64_8,
   MAX
};

static constexpr ConstString c_encodingNames[]=
{
   "Hex_4",
   "Hex_8",
   "Base64_8",
   "ZBase64_8",
};

struct Wnd_TileMap;
struct Wnd_TilePicker : TWindowImpl<Wnd_TilePicker>, OwnerPtrData
{
   static ATOM Register();
   Wnd_TilePicker(Wnd_TileMap &tileMap, HBITMAP bitmap, uint2 tileSize);

   LRESULT WndProc(const Windows::Message &msg) override;
   LRESULT On(const Msg::LButtonDown &msg);
   LRESULT On(const Msg::Paint &msg);

private:

   int2 ClientToBitmap(int2 position) const;

   Wnd_TileMap &m_tile_map;
   HBITMAP m_bitmap;
   uint2 m_bitmap_size;
   uint2 m_tile_size;
   int m_selected{};
};

struct EditState;
struct Selection;

struct Command
{
   Command(EditState &state);
   virtual ~Command() noexcept { }
   virtual void Do()=0;

protected:
   EditState &m_state;
};

namespace Commands
{
struct SetTiles;
struct OffsetSelection;
}

struct EditState
{
   Selection *mp_selection{};

   void Reset();
   void Undo();
   void Redo();

   void PushCommand(Command &command);

   Array2D<BYTE> m_map;

private:

   Collection<UniquePtr<Command>> m_undos;
   Collection<UniquePtr<Command>> m_redos;
};

Command::Command(EditState &state) : m_state(state)
{
   m_state.PushCommand(*this);
}

void EditState::Undo()
{
   if(!m_undos) return;

   auto pCommand(m_undos.Pop());
   pCommand->Do();
   m_redos.Push(std::move(pCommand));
}

void EditState::Redo()
{
   if(!m_redos) return;

   auto pCommand(m_redos.Pop());
   pCommand->Do();
   m_undos.Push(std::move(pCommand));
}

void EditState::PushCommand(Command &command)
{
   m_redos.Empty();
   m_undos.Push(&command);
}

struct Selection
{
   Selection(Rect rc, Array2D<BYTE> map); // Select an existing area (making m_tiles_covered be empty, and m_tiles be what was already on the map)
   Selection(int2 position, Array2D<const BYTE> tiles); // Draw a new area onto the map

   void Draw(Array2D<BYTE> map);
   void Undraw(Array2D<BYTE> map);

   void OffsetPosition(int2 offset) { Assert(!m_has_drawn); m_position+=offset; }
   Rect GetRect() const { return Rect(m_position, m_position+m_tiles.Shape()); }

   int2 m_position;
   OwnedArray2D<BYTE> m_tiles; // When valid, we own a floating block of tiles
   OwnedArray2D<BYTE> m_tiles_covered; // Saved area that m_pTiles is covering on the map
#ifdef USE_ASSERTS
   bool m_has_drawn{}; // Set to true on Draw(), False() on Undraw
#endif
};

Selection::Selection(Rect rc, Array2D<BYTE> map) : m_position(rc.ptLT())
{
   m_tiles=map.Slice(rc);
   m_tiles_covered=OwnedArray2D<BYTE>(rc.size());
   MemoryZero(m_tiles_covered);
#ifdef USE_ASSERTS
   m_has_drawn=true;
#endif
}

Selection::Selection(int2 position, Array2D<const BYTE> tiles) : m_position(position)
{
   m_tiles=tiles;
   m_tiles_covered=OwnedArray2D<BYTE>(tiles.Shape());
}

void Selection::Draw(Array2D<BYTE> map)
{
   // Crop to the area that's overlapping with the map
   Rect rcMap=GetRect()&Rect(int2(), map.Shape());
   Rect rcTiles=rcMap-m_position;

   if(rcMap.size()==int2())
      return;

   // Save covered area
   m_tiles_covered.Slice(rcTiles).CopyFrom(map.Slice(rcMap));
   // Draw new area
   map.Slice(rcMap).CopyFrom(m_tiles.Slice(rcTiles));
#ifdef _DEBUG
   Assert(!m_has_drawn);
   m_has_drawn=true;
#endif
}

void Selection::Undraw(Array2D<BYTE> map)
{
   // Crop to the area that's overlapping with the map
   Rect rcMap=GetRect()&Rect(int2(), map.Shape());
   Rect rcTiles=rcMap-m_position;

   if(rcMap.size()==int2())
      return;

   // Redraw covered area
   map.Slice(rcMap).CopyFrom(m_tiles_covered.Slice(rcTiles));

#ifdef _DEBUG
   Assert(m_has_drawn);
   m_has_drawn=false;
#endif
}

namespace Commands
{

struct ClearSelection : Command
{
   ClearSelection(EditState &state) : Command(state) { Assert(state.mp_selection); Do(); }

   void Do() override
   {
      std::swap(m_state.mp_selection, mp_selection);
   }

private:
   Selection *mp_selection{};
};

struct SelectRect : Command
{
   SelectRect(EditState &state, Rect rc) : Command(state), m_selection(rc, state.m_map) { Do(); }

   void Do() override
   {
      std::swap(m_state.mp_selection, mp_selection); // We're already drawn, so no Do();
   }

private:
   Selection m_selection;
   Selection *mp_selection{&m_selection};
};

struct PasteTiles : Command
{
   PasteTiles(EditState &state, int2 pos, Array2D<const BYTE> tiles) : Command(state), m_selection(pos, tiles) { Do(); }

   void Do() override
   {
      if(m_fDo)
         m_selection.Draw(m_state.m_map);
      else
         m_selection.Undraw(m_state.m_map);

      std::swap(m_state.mp_selection, mp_selection);
      m_fDo^=true;
   }

private:
   Selection m_selection;
   bool m_fDo{true};
   Selection *mp_selection{&m_selection};
};

struct OffsetSelection : Command
{
   OffsetSelection(EditState &state, int2 offset) : Command(state), m_offset(offset) { Do(); }

   void Do() override
   {
      Selection &selection=*m_state.mp_selection;

      selection.Undraw(m_state.m_map);
      selection.OffsetPosition(m_offset);
      selection.Draw(m_state.m_map);

      m_offset=-m_offset;
   }

   int2 Offset() const { return -m_offset; }

private:
   int2 m_offset;
};

struct SetTiles : Command
{
   SetTiles(EditState &state) : Command(state) { }

   void Do() override
   {
      // Swap the tile drawn with the tile saved
      for(auto &p : m_entries)
         std::swap(m_state.m_map[p.m_position], p.m_tile);
   }

   void SetTile(int2 position, BYTE newTile)
   {
      if(!Rect(int2(), m_state.m_map.Shape()).IsInside(position))
         return;

      auto &tile=m_state.m_map[position];
      // If already set, don't add otherwise we'll screw up on undo (since if the same tile is set multiple times in a single operation, the 'previous value' won't work
      // unless the undo operation iterates in the reverse of the redo. Simpler to just optimize these cases away entirely as they're useless)
      if(tile==newTile)
         return;

      m_entries.Push(position, tile);
      tile=newTile;
   }

private:

   struct Entry
   {
      int2 m_position;
      BYTE m_tile;
   };
   Collection<Entry> m_entries;
};

}

struct Wnd_TileMap : IWnd_TileMap, TWindowImpl<Wnd_TileMap>, IDownload, DLNode<Wnd_TileMap>
{
   struct Msg_Update : Windows::Message
   {
      static const MessageID ID=WM_USER;
      Msg_Update() : Windows::Message{ID, 0, 0} { }
   };

   // IDownload
   void OnDownloadUpdate() override
   {
      Msg_Update().Post(*this);
   }

   static ATOM Register();

   Wnd_TileMap(TileMaps &maps, ConstString name);
   ~Wnd_TileMap();

   ConstString GetTitle() override { return m_name; }
   Wnd_Docking &GetDocking() override { return *mp_docking; }

   void InvalidateRect(Rect rc);
   void InvalidateSelection();
   void SetURL(ConstString url);
   void SetMapSize(uint2 map_size);
   void OnMapData(ConstString value);
   void OnBitmap(Handle<HBITMAP> &&bitmap);
   void Cut();
   void Copy();
   void Paste();

   bool IsEditing() const { return mp_tile_picker; }

   LRESULT WndProc(const Windows::Message &msg) override;

   LRESULT On(const Msg::LButtonDown &msg);
   LRESULT On(const Msg::LButtonUp &msg);
   LRESULT On(const Msg::CaptureChanged &msg);
   LRESULT On(const Msg::MouseMove &msg);
   LRESULT On(const Msg::RButtonDown &msg);
   LRESULT On(const Msg::Key &msg);
   LRESULT On(const Msg::Paint &msg);
   LRESULT On(const Msg::_GetTypeID &msg) { return msg.Success(this); }
   LRESULT On(const Msg::_GetThis &msg) { return msg.Success(this); }
   LRESULT On(const Msg_Update &msg);

   void SetTile(int tile)
   {
      InvalidateSelection();
      if(m_edit_state.mp_selection)
         new Commands::ClearSelection(m_edit_state);
      m_selecting_start=int2(-1);
      m_tile=tile;
      m_last_paint_preview=int2(-1);
   }

private:

   Rect GetSelectionRect() const { return Rect::FromPoints(m_selecting_start, m_selecting_end).Inset(0, 0, -1, -1)&Rect(int2(), m_map_size); }

   void CopyToClipboard();
   int2 ClientToMap(int2 position) const;
   int2 MapToClient(int2 position) const;
   BYTE *PointToTile(int2 position);

   TileMaps &m_maps;
   Handle<HBITMAP> m_bitmap;
   int m_bitmap_tile_width;
   Wnd_Docking *mp_docking;

   int m_tile{-1};

   UniquePtr<AsyncDownloader> mp_downloader;
   AsyncDownloader *m_pDownloader_complete{};

   OwnerPtr<Wnd_TilePicker> mp_tile_picker;
   int2 m_selecting_start{-1}, m_selecting_end;
   int2 m_moving_start{-1};
   int2 m_last_paint_preview{-1};

public:

   struct Scrap
   {
      OwnedArray2D<BYTE> m_tiles;
      int2 m_position;
   };
   UniquePtr<Scrap> m_pScrap;

   OwnedString m_name;
   OwnedString m_url;
   uint2 m_tile_size;
   uint2 m_map_size;
   Encoding m_encoding{Encoding::ZBase64_8};
   OwnedArray2D<BYTE> m_map_data;

   EditState m_edit_state;
   Commands::SetTiles *mp_set_tiles{};
   Commands::OffsetSelection *mp_offset_selection{};
   friend struct TileMaps;
};

ATOM Wnd_TilePicker::Register()
{
   WndClass wc(L"Wnd_TilePicker");
   wc.style=CS_HREDRAW|CS_VREDRAW;
   wc.LoadIcon(IDI_APP, g_hInst);
   wc.hCursor=LoadCursor(nullptr, IDC_ARROW);
   wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);

   return wc.Register();
}

Wnd_TilePicker::Wnd_TilePicker(Wnd_TileMap &tile_map, HBITMAP bitmap, uint2 tile_size)
   : m_tile_map(tile_map), m_bitmap(bitmap), m_tile_size(tile_size)
{
   Create("Tile Picker", WS_OVERLAPPEDWINDOW, Window::Position_Default, tile_map, WS_EX_NOACTIVATE);
   m_bitmap_size=BitmapInfo(m_bitmap).size();
   SetSize(FrameSize()+m_bitmap_size*g_dpiScale);
   Show(SW_SHOWNOACTIVATE);
}

LRESULT Wnd_TilePicker::WndProc(const Windows::Message &msg)
{
   return Dispatch<WindowImpl, Msg::LButtonDown, Msg::Paint>(msg);
}

int2 Wnd_TilePicker::ClientToBitmap(int2 position) const
{
   Rect rect=TouchFromInside(RectF(float2(), ClientRect().size()), m_bitmap_size);
   return ((position-rect.ptLT())*m_bitmap_size)/rect.size();
}

LRESULT Wnd_TilePicker::On(const Msg::LButtonDown &msg)
{
   int2 pos=ClientToBitmap(msg.position());

   // Out of bounds?
   if(!Rect(int2(), m_bitmap_size).IsInside(pos))
      return msg.Success();

   int selected=pos.x/m_tile_size.x+(pos.y/m_tile_size.y)*(m_bitmap_size.x/m_tile_size.x);
   if(selected<256)
   {
      m_selected=selected;
      m_tile_map.SetTile(m_selected);
      Invalidate(false);
   }
   return msg.Success();
}

LRESULT Wnd_TilePicker::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);

   Rect rcWindow=TouchFromInside(RectF(float2(), ClientRect().size()), m_bitmap_size);
   Rect rcBitmap(int2(), m_bitmap_size);
   ps.SetMapping(rcBitmap, rcWindow);

   BitmapDC dcTiles{m_bitmap, ps};
   ps.BitBlt(rcBitmap, dcTiles, 0, 0, SRCCOPY);

   if(m_selected>=0)
   {
      ps.SelectPen((HPEN)GetStockObject(WHITE_PEN));
      int pitch=m_bitmap_size.x/m_tile_size.x;
      int2 position=int2(m_selected%pitch, m_selected/pitch);
      ps.DrawFocusRect(Rect(position, position+1)*m_tile_size);
   }

   return msg.Success();
}

ATOM Wnd_TileMap::Register()
{
   WndClass wc(L"Wnd_TileMap");
   wc.style=CS_HREDRAW|CS_VREDRAW;
   wc.LoadIcon(IDI_APP, g_hInst);
   wc.hCursor=LoadCursor(nullptr, IDC_ARROW);
   wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);

   return wc.Register();
}

Wnd_TileMap::Wnd_TileMap(TileMaps &maps, ConstString name)
   : DLNode<Wnd_TileMap>(maps.m_maps.Prev()),
     m_maps{maps},
     m_name{name}
{
   Create(name, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME, Window::Position_Default, m_maps.m_wnd_main);
   SetMapSize(1);
   SetSize(FrameSize()+uint2(16, 16)*uint2(16, 16)*g_dpiScale);
   mp_docking=&m_maps.m_wnd_main.CreateDocking(*this);
//      mp_docking->Dock(Docking::Side::Bottom);
}

Wnd_TileMap::~Wnd_TileMap()
{
}

void Wnd_TileMap::InvalidateRect(Rect rc)
{
   Rect rcInvalid(MapToClient(rc.ptLT()), MapToClient(rc.ptRB())+1);
   Invalidate(rcInvalid, false);
}

void Wnd_TileMap::InvalidateSelection()
{
   if(m_edit_state.mp_selection)
      InvalidateRect(m_edit_state.mp_selection->GetRect());
   else
      InvalidateRect(GetSelectionRect());
}

void Wnd_TileMap::SetURL(ConstString url)
{
   if(m_url==url)
      return;

   m_url=url;
   mp_downloader=MakeUnique<AsyncDownloader>(*this, m_url);
   Invalidate(true);
}

void Wnd_TileMap::SetMapSize(uint2 map_size)
{
   if(m_map_size==map_size)
      return;

   m_map_size=map_size;
   m_map_data=OwnedArray2D<BYTE>(m_map_size);
   m_edit_state.m_map=m_map_data;
   MemoryZero(m_map_data);
   Invalidate(true);
}

void Wnd_TileMap::OnMapData(ConstString value)
{
   unsigned size=::content(m_map_data.Shape());

   switch(m_encoding)
   {
      case Encoding::Hex_4:
      {
         if(value.Count()!=size)
            throw Exceptions::Message(FixedStringBuilder<256>("Expected ", size, " characters but received ", value.Count()));

         auto dest=m_map_data.begin();
         for(unsigned i=0;i<size;i++)
         {
            unsigned tile{};
            value.Sub(i,i+1).HexTo(tile);
            *dest++=tile;
         }
         break;
      }

      case Encoding::Hex_8:
      {
         if(value.Count()!=size*2)
            throw Exceptions::Message(FixedStringBuilder<256>("Expected ", size*2, " characters but received ", value.Count()));

         auto dest=m_map_data.begin();
         for(unsigned i=0;i<size;i++)
         {
            unsigned tile{};
            value.Sub(i*2,(i+1)*2).HexTo(tile);
            *dest++=tile;
         }
         break;
      }

      case Encoding::Base64_8:
      {
         try
         {
            auto buffer=Buffer::Existing{m_map_data};
            if(!Base64_Decode(buffer, value))
               throw std::runtime_error("Error decoding Base64 content");
         }
         catch(const BufferOverflow &)
         {
            throw std::runtime_error("Error, buffer too small, please report this!");
         }
         break;
      }

      case Encoding::ZBase64_8:
      {
         Buffer::Fixed buffer(Base64_Decode_Length(value.Count()));
         try
         {
            if(!Base64_Decode(buffer, value))
               throw std::runtime_error("Error decoding Base64 content");
         }
         catch(const BufferOverflow &)
         {
            throw std::runtime_error("Error, buffer too small, please report this!");
         }

         if(!UncompressRaw(buffer, m_map_data))
            throw std::runtime_error("Error unzipping data, or map size is incorrect");
         break;
      }
   }
   Invalidate(false);
}

void Wnd_TileMap::OnBitmap(Handle<HBITMAP> &&bitmap)
{
   m_bitmap=std::move(bitmap);
   m_bitmap_tile_width=BitmapInfo(m_bitmap).width()/m_tile_size.x;
   Invalidate(true);
}

LRESULT Wnd_TileMap::WndProc(const Windows::Message &msg)
{
   return Dispatch<WindowImpl, Msg::_GetThis, Msg::_GetTypeID, Msg_Update, Msg::LButtonDown, Msg::LButtonUp, Msg::CaptureChanged, Msg::RButtonDown, Msg::MouseMove, Msg::KeyDown, Msg::Paint>(msg);
}

LRESULT Wnd_TileMap::On(const Msg_Update &msg)
{
   if(mp_downloader)
   {
      switch(mp_downloader->GetStatus())
      {
         case AsyncDownloader::Status::Downloading:
            Invalidate(false);
            break; // Repaint progress
         case AsyncDownloader::Status::Complete:
            OnBitmap(Imaging::LoadImageFromFile(mp_downloader->GetFilename()));
            mp_downloader=nullptr;
            break;
         case AsyncDownloader::Status::Failed:
            Invalidate(false);
            mp_downloader=nullptr;
            break;
      }
   }
   return msg.Success();
}

LRESULT Wnd_TileMap::On(const Msg::LButtonDown &msg)
{
   if(!mp_tile_picker)
      return msg.Success();

   int2 position=ClientToMap(msg.position());
   SetCapture();

   if(m_tile>=0)
   {
      mp_set_tiles=new Commands::SetTiles(m_edit_state);
      mp_set_tiles->SetTile(position, m_tile);
      InvalidateRect(Rect(position, position+1));
      return msg.Success();
   }

   if(m_edit_state.mp_selection)
   {
      if(m_edit_state.mp_selection->GetRect().IsInside(position))
      {
         m_moving_start=position;
         mp_offset_selection=new Commands::OffsetSelection(m_edit_state, int2());
         return msg.Success();
      }
      else
      {
         InvalidateRect(m_edit_state.mp_selection->GetRect());
         new Commands::ClearSelection(m_edit_state);
      }
   }
   m_selecting_end=m_selecting_start=position;

   return msg.Success();
}

LRESULT Wnd_TileMap::On(const Msg::LButtonUp &msg)
{
   ReleaseCapture();
   return msg.Success();
}

LRESULT Wnd_TileMap::On(const Msg::CaptureChanged &msg)
{
   if(msg.wndNewCapture()==*this) // Gaining the capture?
      return msg.Success();

   if(mp_set_tiles)
      mp_set_tiles=nullptr;
   else if(m_selecting_start!=int2(-1))
   {
      new Commands::SelectRect(m_edit_state, GetSelectionRect());
      InvalidateSelection();
      m_selecting_start=int2(-1);
   }
   else if(mp_offset_selection)
   {
      mp_offset_selection=nullptr;
   }
   return msg.Success();
}

LRESULT Wnd_TileMap::On(const Msg::MouseMove &msg)
{
   int2 position=ClientToMap(msg.position());
   if(mp_set_tiles)
   {
      mp_set_tiles->SetTile(position, m_tile);
      InvalidateRect(Rect(position, position+1));
      return msg.Success();
   }

   if(mp_tile_picker && m_tile>=0)
   {
      if(position!=m_last_paint_preview)
      {
         InvalidateRect(Rect(m_last_paint_preview, m_last_paint_preview+1));
         m_last_paint_preview=position;
         InvalidateRect(Rect(m_last_paint_preview, m_last_paint_preview+1));
      }
      return msg.Success();
   }

   if(m_selecting_start!=int2(-1) && m_selecting_end!=position)
   {
      InvalidateSelection();
      m_selecting_end=position;
      InvalidateSelection();
   }

   if(mp_offset_selection)
   {
      int2 offset=position-m_moving_start;
      if(mp_offset_selection->Offset()!=offset)
      {
         InvalidateRect(m_edit_state.mp_selection->GetRect());
         m_edit_state.Undo();
         mp_offset_selection=new Commands::OffsetSelection(m_edit_state, offset);
         InvalidateRect(m_edit_state.mp_selection->GetRect());
      }
   }

   return msg.Success();
}

void Wnd_TileMap::Cut()
{
   if(!m_edit_state.mp_selection)
      return;
   Copy();

   OwnedArray2D<BYTE> emptyArea(m_edit_state.mp_selection->GetRect().size());
   MemoryZero(emptyArea);

   new Commands::PasteTiles(m_edit_state, m_edit_state.mp_selection->GetRect().ptLT(), emptyArea);
   InvalidateRect(m_edit_state.mp_selection->GetRect());
   m_edit_state.mp_selection=nullptr;
}

void Wnd_TileMap::Copy()
{
   if(!m_edit_state.mp_selection)
      return;
   if(!m_pScrap)
      m_pScrap=MakeUnique<Scrap>();

   m_pScrap->m_position=m_edit_state.mp_selection->m_position;
   m_pScrap->m_tiles=m_edit_state.mp_selection->m_tiles;
}

void Wnd_TileMap::Paste()
{
   if(!m_pScrap)
      return;

   new Commands::PasteTiles(m_edit_state, m_pScrap->m_position, m_pScrap->m_tiles);
   InvalidateRect(m_edit_state.mp_selection->GetRect());
}

LRESULT Wnd_TileMap::On(const Msg::RButtonDown &msg)
{
   if(m_tile>=0)
   {
      m_tile=-1;
      InvalidateRect(Rect(m_last_paint_preview, m_last_paint_preview+1));
      return msg.Success();
   }

   if(m_edit_state.mp_selection)
   {
      InvalidateRect(m_edit_state.mp_selection->GetRect());
      new Commands::ClearSelection(m_edit_state);
      return msg.Success();
   }

   enum struct Commands : UINT_PTR
   {
      Encoding=100,
      Encoding_Hex_4,
      Encoding_Hex_8,
      Encoding_Base64_8,
      Encoding_ZBase64_8,
      Edit,
      Copy,
      Help,
   };

   PopupMenu menu;
   menu.Append(0, (UINT_PTR)Commands::Edit, "Edit");
   menu.Append(0, (UINT_PTR)Commands::Copy, "Copy To Clipboard");
   PopupMenu menuEncode;
   menuEncode.Append(0, (UINT_PTR)Commands::Encoding_Hex_4, c_encodingNames[(int)Encoding::Hex_4]);
   menuEncode.Append(0, (UINT_PTR)Commands::Encoding_Hex_8, c_encodingNames[(int)Encoding::Hex_8]);
   menuEncode.Append(0, (UINT_PTR)Commands::Encoding_Base64_8, c_encodingNames[(int)Encoding::Base64_8]);
   menuEncode.Append(0, (UINT_PTR)Commands::Encoding_ZBase64_8, c_encodingNames[(int)Encoding::ZBase64_8]);
   CheckMenuItem(menuEncode, (UINT_PTR)Commands::Encoding+(UINT_PTR)m_encoding, MF_CHECKED);
   menu.Append(std::move(menuEncode), "Encoding");
   menu.AppendSeparator();
   menu.Append(0, (UINT_PTR)Commands::Help, "Help");

   switch(Commands(TrackPopupMenu(menu, TPM_RETURNCMD, GetCursorPos(), *this, nullptr)))
   {
      case Commands::Edit:
         if(m_bitmap && !mp_tile_picker)
            mp_tile_picker=MakeUnique<Wnd_TilePicker>(*this, m_bitmap, m_tile_size);
         break;
      case Commands::Copy:
         CopyToClipboard();
         break;

      case Commands::Encoding_Hex_4: m_encoding=Encoding::Hex_4; break;
      case Commands::Encoding_Hex_8: m_encoding=Encoding::Hex_8; break;
      case Commands::Encoding_Base64_8: m_encoding=Encoding::Base64_8; break;
      case Commands::Encoding_ZBase64_8: m_encoding=Encoding::ZBase64_8; break;

      case Commands::Help:
         MessageBox(*this, "In edit mode:\nClick a tile on the tile picker to then paint with that tile.\nRight click to stop painting.\nClick and drag to select an area, then you can click and drag to move a selected area.\n\nCtrl+C = Copy, Ctrl+V = Paste, Ctrl+X = Cut\nCtrl+Z = Undo, Ctrl+Y = Redo", "Help", MB_OK);
         break;
   }
   return msg.Success();
}

LRESULT Wnd_TileMap::On(const Msg::Key &msg)
{
   // Ignore keys while mouse button is pressed
   if(IsKeyPressed(VK_LBUTTON) || IsKeyPressed(VK_RBUTTON))
      return msg.Success();

   bool fControl=IsKeyPressed(VK_CONTROL);
   if(msg.direction()==Direction::Down && !msg.fRepeating() && !fControl)
   {
      switch(msg.iVirtKey())
      {
         case VK_DELETE: Cut(); break;
      }
   }
   else if(msg.direction()==Direction::Down && fControl)
   {
      if(!msg.fRepeating())
      {
         switch(msg.iVirtKey())
         {
            case 'X': Cut(); return msg.Success();
            case 'C': Copy(); return msg.Success();
            case 'V': Paste(); return msg.Success();
         }
      }

      // These are ok repeating
      switch(msg.iVirtKey())
      {
         case 'Z': m_edit_state.Undo(); Invalidate(false); return msg.Success();
         case 'Y': m_edit_state.Redo(); Invalidate(false); return msg.Success();
      }
   }

   return msg.Success();
}

void Wnd_TileMap::CopyToClipboard()
{
   HybridStringBuilder<256> string("beip.tilemap.info { \"", m_name, "\":{\"tile-url\":\"", m_url, "\", \"tile-size\":\"", m_tile_size.x, ',', m_tile_size.y, "\", \"map-size\":\"", m_map_size.x, ',', m_map_size.y, "\", \"enc\":\"", c_encodingNames[(int)m_encoding], "\" }}" CRLF);
   string("beip.tilemap.data { \"", m_name, "\":\"");

   switch(m_encoding)
   {
      case Encoding::Hex_4:
         for(auto c : m_map_data)
            string(Strings::Hex32(c, 1));
         break;

      case Encoding::Hex_8:
         for(auto c : m_map_data)
            string(Strings::Hex32(c, 2));
         break;

      case Encoding::Base64_8: Base64_Encode(string, m_map_data); break;
      default:
      case Encoding::ZBase64_8: Base64_Encode(string, CompressRaw(m_map_data)); break;
   }
   string("\" }}");
   ::Clipboard::SetText(string);
}

int2 Wnd_TileMap::ClientToMap(int2 position) const
{
   Rect rect=TouchFromInside(RectF(float2(), ClientRect().size()), m_map_size);
   return ((position-rect.ptLT())*m_map_size)/rect.size();
}

int2 Wnd_TileMap::MapToClient(int2 position) const
{
   Rect rect=TouchFromInside(RectF(float2(), ClientRect().size()), m_map_size);
   return (position*rect.size())/m_map_size+rect.ptLT();
}

BYTE *Wnd_TileMap::PointToTile(int2 position)
{
   position=ClientToMap(position);
   // Out of bounds?
   if(!Rect(int2(), m_map_data.Shape()).IsInside(position))
      return nullptr;

   return &m_map_data[position];
}

LRESULT Wnd_TileMap::On(const Msg::Paint &msg)
{
   PaintStruct ps(this);

   if(m_bitmap)
   {
      int2 size=m_map_size;
      Rect rcWindow=TouchFromInside(RectF(float2(), ClientRect().size()), size);
      ps.SetMapping(Rect(int2(), size), rcWindow);

      BitmapDC dcTiles{m_bitmap, ps};

      const BYTE *pTile=m_map_data.begin();
      for(int2 pos;pos.y<size.y;pos.y++)
         for(pos.x=0;pos.x<size.x;pos.x++)
         {
            auto tileIndex=*pTile++;
            int2 tilePos=int2(tileIndex%m_bitmap_tile_width, tileIndex/m_bitmap_tile_width);
            ps.StretchBlt(Rect(pos, pos+1), dcTiles, Rect(tilePos, tilePos+1)*m_tile_size, SRCCOPY);
         }

      if(mp_tile_picker && m_tile>=0)
      {
         int2 pos=ClientToMap(ScreenToClient(Windows::GetMessagePos()));
         if(Rect(int2(), m_map_data.Shape()).IsInside(pos))
         {
            int2 tilePos=int2(m_tile%m_bitmap_tile_width, m_tile/m_bitmap_tile_width);
            ps.StretchBlt(Rect(pos, pos+1), dcTiles, Rect(tilePos, tilePos+1)*m_tile_size, SRCCOPY);
         }
      }

      if(m_selecting_start!=int2(-1) || m_edit_state.mp_selection)
      {
         Rect rc=m_edit_state.mp_selection ? m_edit_state.mp_selection->GetRect()&Rect(int2(), m_map_size) : GetSelectionRect();

         ps.SelectPen((HPEN)GetStockObject(WHITE_PEN));
         ps.SelectBrush((HBRUSH)GetStockObject(NULL_BRUSH));
         ps.Rectangle(rc);
      }
   }
   else
   {
      if(mp_downloader)
         ps.TextOut(int2(0, 5), FixedStringBuilder<256>("Downloading: ", mp_downloader->GetPercent(), "% complete"));
   }
   return msg.Success();
}

struct JSON_TileMapInfo : JSON::Element
{
   JSON_TileMapInfo(Wnd_TileMap &map) : m_map{map} { }

   void OnString(ConstString name, ConstString value) override;

   Wnd_TileMap &m_map;
};

void JSON_TileMapInfo::OnString(ConstString name, ConstString value)
{
   if(name=="tile-url")
   {
      m_map.SetURL(value);
      return;
   }

   if(name=="tile-size")
   {
      uint2 tile_size;
      value.To(tile_size);

      if(tile_size.x==0 || tile_size.y==0)
         throw Exceptions::Message("tileSize cannot be zero");
      if(tile_size.x>256 || tile_size.y>256)
         throw Exceptions::Message("tileSize is too large, maximum size is 256 in each dimension");

      m_map.m_tile_size=tile_size;
      return;
   }

   if(name=="encoding")
   {
      for(unsigned i=0;i<_countof(c_encodingNames);i++)
         if(IEquals(value, c_encodingNames[i]))
         {
            m_map.m_encoding=Encoding(i);
            return;
         }
      throw Exceptions::Message("Unknown encoding");
      return;
   }

   if(name=="map-size")
   {
      uint2 map_size;
      value.To(map_size);

      if(map_size.x==0 || map_size.y==0)
         throw Exceptions::Message("mapSize cannot be zero");
      if(map_size.x>256 || map_size.y>256)
         throw Exceptions::Message("mapSize is too large, maximum size is 256 in each dimension");

      m_map.SetMapSize(map_size);
      return;
   }

}

struct JSON_TileMapInfoName : JSON::Element
{
   JSON_TileMapInfoName(TileMaps &maps) : m_maps{maps} { }

   JSON::Element &OnObject(ConstString name) override
   {
      auto *pMap=m_maps.Find(name);
      if(!pMap)
      {
         pMap=new Wnd_TileMap(m_maps, name);
         pMap->Show(SW_SHOWNOACTIVATE);
      }

      if(pMap->IsEditing()) // Ignore updates
         return __super::OnObject(name);

      m_pInfo.Empty();
      return m_pInfo.New(*pMap);
   }

   TileMaps &m_maps;
   PlacementUniquePtr<JSON_TileMapInfo> m_pInfo;
};

struct JSON_TileMapData : JSON::Element
{
   JSON_TileMapData(TileMaps &maps) : m_maps{maps} { }

   void OnString(ConstString name, ConstString value) override
   {
      auto *pMap=m_maps.Find(name);
      if(!pMap)
         throw Exceptions::Message(FixedStringBuilder<256>("Received map data for non existant map: ", name));

      if(pMap->IsEditing()) // Ignore updates
         return;

      pMap->OnMapData(value);
   }

   TileMaps &m_maps;
};

TileMaps::TileMaps(Wnd_Main &wndMain) : m_wnd_main(wndMain)
{
}

TileMaps::~TileMaps()
{
}

Wnd_TileMap *TileMaps::Find(ConstString name)
{
   for(auto &map : m_maps)
   {
      if(map.m_name==name)
         return &map;
   }
   return nullptr;
}

IWnd_TileMap &TileMaps::Create(ConstString name)
{
   return *new Wnd_TileMap(*this, name);
}

void TileMaps::On(ConstString command, ConstString json)
{
   if(command=="info")
   {
      JSON_TileMapInfoName element{*this};
      JSON::ParseObject(element, json);
   }
   else if(command=="data")
   {
      JSON_TileMapData element{*this};
      JSON::ParseObject(element, json);
   }
   else
      Assert(false);
}
