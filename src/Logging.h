struct Connection;

struct Log : Events::Sends_Deleted
{
   Log(Connection &connection, Prop::Logging &prop_logging, IError &error, ConstString filename, int append_date_format=0);
   ~Log() noexcept;

   void LogTextList(const Text::List &list, const Text::Line *pStart=nullptr);
   void LogTextLine(const Text::Line &line);
   void LogTyped(ConstString string);
   void LogSent(ConstString string);

   bool fDateLog() const { return m_date_log; }
   ConstString GetFileName() const { return m_filename; }

private:

   void WriteHTMLPrefix(StringBuilder &string, Text::Time32 time);
   void WriteHTMLSuffix(StringBuilder &string);

   void Write(Text::Time32 time32); // Uses WriteText() to output so no formatting occurs
   void Write(ConstString text); // Will translate to HTML and add a CRLF if m_HTML is set

   unsigned m_line_pos{};
   unsigned m_time_indent{};

   OwnedString m_filename;
   File::Write_Only m_file_log;
   Prop::Logging &mp_prop_logging;
   Prop::TextWindow &m_prop_text_window;
   IError &m_error;
   bool m_date_log; // True if the log file has a date on it (so that we create a new one on a new day)
   bool m_HTML{};
};

struct RestoreLogs
{
   static UniquePtr<RestoreLogs> Create();

   RestoreLogs(unsigned bufferSize);
   RestoreLogs(ConstString path, unsigned bufferSize, unsigned bufferCount); // Creates a blank Restore (used by Resize)
   ~RestoreLogs();

   [[nodiscard]] static bool Resize(unsigned oldSize, unsigned newSize);

   void CheckAndRepair();
   void EraseAll();

   unsigned GetBufferSize() const { return m_buffer_size; }
   unsigned GetUsed(unsigned index) const;
   unsigned Count() const { return m_buffer_count; }

   void Allocate(Prop::Character &propCharacter);
   void Free(Prop::Character &propCharacter);
   void Free(unsigned index); // Only used to repair an invalid restore log database

   void WriteStart(unsigned index);
   void WriteReceived(unsigned index, Array<const BYTE> data);
   void WriteReceived_GMCP(unsigned index, Array<const BYTE> data);
   void WriteSent(unsigned index, Array<const BYTE> data);

   enum EntryType : unsigned
   {
      Received,
      Sent,
      Start, // Start of a new connection, so reset all parsing
      End, // Not a real entry type, just a marker that there is no more data
      Received_GMCP,
   };

private:

   static void FixupMovedLog(int from, int to);

   // The format in the Buffer is a list of entries with the format of:
   // BYTE(type) BYTE[3](size) uint64(time) data... LineFeed
   // Type: 0 = Received, 1 = Sent

   #pragma pack(push, 1)
   struct Header
   {
      EntryType m_type : 8;
      unsigned m_size : 24; // Size does not include the size of the header
      uint64 m_time{Time::System().GetFileTime()};
   };
   #pragma pack(pop)

   static_assert(sizeof(Header)==12);

   struct Buffer
   {
      void Reset() { m_start=0; m_count=0; }

      uint32 m_start, m_count;
      BYTE m_data[0];
   };

   struct View
   {
      View(unsigned data_size) : m_data_size{data_size} { }

      Header GetHeader(unsigned index) const;
      void DeleteOldest();

      void Write(Header header, Array<const BYTE> data);
      void Write(Array<const BYTE> data);

      Array<const BYTE> Read(unsigned index, unsigned count, Storage::Buffer::Fixed &wrap_buffer) const;

      void WrapAdd(unsigned &v, unsigned offset) const;

      Kernel::FileView<Buffer> m_buffer;
      unsigned m_data_size;
   };

   void DeleteOldest(Buffer &buffer); // Free the oldest entry from the queue to make space for a new one
   void Write(Buffer &view, Array<const BYTE> data);
   unsigned Allocate();
   View *Load(unsigned index) const; // Initialize the m_views for this index
   void Unload(unsigned index) const; // Close the view (optimization, not required)

   unsigned m_buffer_size{64*1024}; // 64K default
   unsigned m_data_size{};
   unsigned m_buffer_count{};

   mutable File::File m_file;
   mutable Kernel::FileMapping m_mapping;
   mutable Collection<View> m_views;

   friend struct RestoreLogReplay;
};

struct RestoreLogReplay
{
   RestoreLogReplay(RestoreLogs &logs, unsigned index);

   struct Entry
   {
      explicit operator bool() const { return m_type!=RestoreLogs::EntryType::End; }

      ConstString GetDataString() const { return ConstString(reinterpret_cast<const char *>(m_data.begin()), m_data.Count()); }

      RestoreLogs::EntryType m_type;
      uint64 m_time;
      Array<const BYTE> m_data;
   };

   Entry Read(); // Entry is only valid until the next Read call

private:
   RestoreLogs::View *mp_view{};
   Storage::Buffer::Fixed m_wrap_buffer; // Buffer to piece together the line that wraps
   unsigned m_index{};
};
