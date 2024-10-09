//
// Logging.cpp
//

#include "Main.h"
#include "Connection.h"
#include "Wnd_Main.h"
#include "HTML.h"

struct LogPath : File::Path
{
   LogPath(Connection &connection, ConstString filename, bool append_date, ConstString format_string)
    : File::Path{ExpandEnvironmentStrings(filename)},
      m_date_log{append_date}
   {
      if(IsRelative())
      {
         if(IsStoreApp())
            MakeAbsolute(GetDocumentsPath());
         else
            MakeAbsolute(GetConfigPath());
      }

      // Append the Date to the filename?
      if(m_date_log)
      {
         operator()(" - ");
         Time::Local().FormatDate(*this, format_string);
      }
      else while(auto range=IFindRange("%date%"))
      {
         FixedStringBuilder<256> date_string;
         Time::Local().FormatDate(date_string, format_string);
         Replace(range, date_string);
         m_date_log=true;
      }

      while(auto range=IFindRange("%server%"))
      {
         if(connection.GetServer())
            Replace(range, connection.GetServer()->pclName());
         else
            connection.Text("<icon error> <font color='red'> %server% used in log filename but there is no current server");
      }

      while(auto range=IFindRange("%character%"))
      {
         if(connection.GetCharacter())
            Replace(range, connection.GetCharacter()->pclName());
         else
            connection.Text("<icon error> <font color='red'> %character% used in log filename but there is no current character");
      }
   }

   bool fDateLog() const { return m_date_log; }

private:
   bool m_date_log;
};

Log::Log(Connection &connection, Prop::Logging &propLogging, IError &error, ConstString strFilename, int iAppendDateFormat)
 : mp_prop_logging(propLogging),
   m_prop_text_window(connection.GetMainWindow().GetOutputProps()),
   m_error(error),
   m_date_log((iAppendDateFormat&Text::Time32::F_Visible)!=0)
{
   ConstString lstrExtension;
   if(strFilename.RSplit('.', strFilename, lstrExtension))
      m_HTML=lstrExtension.ICompare("html")==0 || lstrExtension.ICompare("htm")==0;
   else
      lstrExtension="txt";

   LogPath sBuffer(connection, strFilename, m_date_log, mp_prop_logging.pclFileDateFormat());
   m_date_log=sBuffer.fDateLog();

   sBuffer('.', lstrExtension);

   m_filename=sBuffer;
   File::EnsurePathExists(m_filename.BeforeLast('\\')); // If this fails, we'll just fail to create the log file next
   m_file_log.Create(m_filename, FILE_SHARE_READ, OPEN_ALWAYS);

   if(!m_file_log)
   {
      m_error.Text(FixedStringBuilder<512>(STR_CantCreateLogfile, "\"<font color='aqua'>", m_filename, "</font>\" <font color='white'>", LastError{}));
      throw std::runtime_error{""};
   }

   if(m_file_log.Size()>0)
   {
      m_file_log.Seek(0, FILE_END);
      m_error.Text(STR_AppendingToExisting);
   }
   else
   {
      if(m_HTML) // Write out new HTML header
      {
         HybridStringBuilder<1024> string(
            "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>" CRLF
            "<HTML>" CRLF
            "<HEAD>" CRLF
            "<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>" CRLF
            "<TITLE>Log for "); connection.GetWorldTitle(string, 0); string("</TITLE>" CRLF
            "<SCRIPT>" CRLF
            "const stamptoggle = () => document.querySelectorAll('.timestamp').forEach(timestamp => timestamp.classList.toggle('show'))" CRLF
            "const formattoggle = () => document.querySelectorAll('p span').forEach(item => item.classList.toggle('unformatted'))" CRLF
            "window.addEventListener(\"DOMContentLoaded\", (event) => {" CRLF
            "  document.querySelector('#stamptoggle').addEventListener('change', stamptoggle);" CRLF
            "  document.querySelector('#formattoggle').addEventListener('change', formattoggle);" CRLF
            "});" CRLF
            "</SCRIPT>" CRLF
            "<STYLE TYPE='text/css' MEDIA=screen>" CRLF
            "BODY { background:", HTML::HTMLColor(m_prop_text_window.clrBack()),
               "; color:", HTML::HTMLColor(m_prop_text_window.clrFore()),
               "; font-family:'", m_prop_text_window.propFont().pclName(), "'; font-size:", m_prop_text_window.propFont().Size(), "px; } " CRLF
            "H2 { border-style: solid; border-width: 2px 0; padding: .5em; text-align: center; }" CRLF
            ".startlog { border-bottom-style: dotted; margin: .75em 0 .5em; }" CRLF
            ".stoplog { border-top-style: dotted; margin: .5em 0 .75em; }" CRLF
            "p { margin: 0 0 ", m_prop_text_window.ParagraphSpacing(), "px 0; display: flex; flex-flow: row; }" CRLF
            ".stamp, .format{position: absolute; top: 2.25em; display: inline-flex; align-items: center;}" CRLF
            ".format{right: .75em;}" CRLF
            ".timestamp{opacity: .25; font-size: .875em; position: relative; top: .175em; width: 11.5em; flex: none; white-space: nowrap; display: none; margin-right: 1em !important; text-align: right !important;}" CRLF
            "p:hover.timestamp{opacity : 1;}" CRLF
            ".line{flex: auto; white-space: pre-wrap;}" CRLF
            "input{position: absolute; clip: rect(0 0 0 0);}" CRLF
            "label{display: inline-block; cursor: pointer; outline: none; user-select: none; padding: 2px; width: 2em; height: 1em; background-color: #dddddd; border-radius: 2em; z-index: 50; position: relative; margin-right: .5em;}" CRLF
            "label::before, label::after{display: block; position: absolute; top: 1px; left: 1px; bottom: 1px; content: '';}" CRLF
            "label::before{right: 1px; background-color: #c0c0c0; border-radius: 1em; transition: background 0.4s;}" CRLF
            "label::after{width: 1em; background-color: #fff; border-radius: 100%; box-shadow: 0 2px 5px rgba(0, 0, 0, 0.3); transition: margin 0.4s;}" CRLF
            "input:checked + span label::after{margin-left: 1em;}" CRLF
            ".unformatted{font-size: unset !important; font-family: unset !important; color: unset !important; background: unset !important; border: unset !important; margin: unset !important; text-align: left !important; font-weight: normal !important; font-style: normal !important; padding: 0 !important; line-height: unset !important; top: unset !important;}" CRLF
            ".timestamp{margin-right: 1em !important; text-align: right !important; }" CRLF
            ".show{display: block;}" CRLF
            "</STYLE>" CRLF
            "</HEAD>" CRLF
            "<BODY>" CRLF
            "<input id='stamptoggle' class='toggle' type='checkbox'>" CRLF
            "<span class='stamp'><label for='stamptoggle'></label> Timestamps</span>" CRLF
            "<input id='formattoggle' class='toggle' type='checkbox'>" CRLF
            "<span class='format'><label for='formattoggle'></label> Unformat</span>" CRLF
         );
         m_file_log.Write(string);
      }
   }

   {
      FixedStringBuilder<256> string(STR_LoggingStarted_ToLog);
      Time::Time time; time.Local();
      time.FormatDate(string);
      string(' ');
      time.FormatTime(string, ConstString(), mp_prop_logging.TimeFormat()&Text::Time32::F_24HR ? (TIME_FORCE24HOURFORMAT|TIME_NOTIMEMARKER) : 0);

      if(m_HTML)
      {
         m_file_log.Write(ConstString("<h2 class='startlog'>"));
         m_file_log.Write(string);
         m_file_log.Write(ConstString("</h2>" CRLF));
      }
      else
      {
         Write("************************************************************" CRLF);
         Write(string);
         Write(CRLF "------------------------------------------------------------" CRLF);
      }
   }

   m_error.Text(FixedStringBuilder<512>(STR_LoggingToFile "\"<font color='aqua'>", m_filename, "</font>\"" STR_LoggingStarted));
}

Log::~Log()
{
   Assert(m_file_log);
   if(!m_file_log)
   {
      m_error.Text(STR_NoLog);
      return;
   }

   {
      FixedStringBuilder<256> string(STR_LoggingStopped_ToLog);
      Time::Time time; time.Local();
      time.FormatDate(string);
      string(' ');
      time.FormatTime(string, ConstString(), mp_prop_logging.TimeFormat()&Text::Time32::F_24HR ? (TIME_FORCE24HOURFORMAT|TIME_NOTIMEMARKER) : 0);

      if(m_HTML)
      {
         m_file_log.Write(ConstString("<h2 class='stoplog'>"));
         m_file_log.Write(string);
         m_file_log.Write(ConstString("</h2>" CRLF));
      }
      else
      {
         Write("------------------------------------------------------------" CRLF);
         Write(string);
         Write(CRLF "************************************************************" CRLF);
      }
   }

   m_error.Text(STR_LoggingStopped);
}

void Log::LogTextList(const Text::List &list, const Text::Line *pStart)
{
   if(pStart)
   {
      auto iter=DLNode<Text::Line>::const_iterator(*pStart);
      auto end=list.GetLines().GetList().end();
      while(iter!=end)
      {
         LogTextLine(*iter);
         ++iter;
      }
   }

   m_error.Text(STR_LoggingWindowContents);
}

void Log::WriteHTMLPrefix(StringBuilder &string, Text::Time32 time)
{
   string("<p><span class='timestamp'>");
   time.Format(string, mp_prop_logging.TimeFormat()|Text::Time32::F_Date|Text::Time32::F_Time);
   string("</span><span class='line'");
}

void Log::WriteHTMLSuffix(StringBuilder &string)
{
   string(" </span></p>" CRLF);
}

void Log::LogTextLine(const Text::Line &line)
{
   if(m_HTML)
   {
      HybridStringBuilder string;
      WriteHTMLPrefix(string, line.Time());
      line.HTMLCopy(string, m_prop_text_window.clrFore(), m_prop_text_window.clrBack(), m_prop_text_window.clrLink(), m_prop_text_window.propFont().pclName(), m_prop_text_window.propFont().Size());
      WriteHTMLSuffix(string);
      m_file_log.Write(string);
      return;
   }

   Write(line.Time());
   HybridStringBuilder line_text; line.TextCopy(line_text);
   Write(line_text);
   Write(CRLF);
   if(mp_prop_logging.fDoubleSpace())
      Write(CRLF);
}

void Log::LogTyped(ConstString line)
{
   if(!mp_prop_logging.fLogTyped()) return;

   if(m_HTML)
   {
      HybridStringBuilder string;
      WriteHTMLPrefix(string, Time::Local());
      {
         HTML::Writer writer(string, m_prop_text_window.clrFore(), m_prop_text_window.clrBack(), m_prop_text_window.clrLink(), m_prop_text_window.propFont().pclName(), m_prop_text_window.propFont().Size(), ConstString{});
         writer << mp_prop_logging.pclTypedPrefix() << line;
      }
      WriteHTMLSuffix(string);
      m_file_log.Write(string);
      return;
   }

   Write(Text::Time32{Time::Local()});
   Write(mp_prop_logging.pclTypedPrefix());
   Write(line);
   Write(CRLF);
   if(mp_prop_logging.fDoubleSpace())
      Write(CRLF);
}

void Log::LogSent(ConstString line)
{
   if(!mp_prop_logging.fLogSent()) return;

   if(m_HTML)
   {
      HybridStringBuilder string;
      WriteHTMLPrefix(string, Time::Local());
      {
         HTML::Writer writer(string, m_prop_text_window.clrFore(), m_prop_text_window.clrBack(), m_prop_text_window.clrLink(), m_prop_text_window.propFont().pclName(), m_prop_text_window.propFont().Size(), ConstString{});
         writer << mp_prop_logging.pclSentPrefix() << line;
      }
      WriteHTMLSuffix(string);
      m_file_log.Write(string);
      return;
   }

   Write(Text::Time32{Time::Local()});
   Write(mp_prop_logging.pclSentPrefix());
   Write(line);
   Write(CRLF);
   if(mp_prop_logging.fDoubleSpace())
      Write(CRLF);
}

void Log::Write(Text::Time32 time32)
{
   if(mp_prop_logging.TimeFormat()&Text::Time32::F_Visible)
   {
      FixedStringBuilder<80> string;
      time32.Format(string, mp_prop_logging.TimeFormat());
      string("  ");
      Write(string);

      m_time_indent=m_line_pos; // Force a wrap indent of the time
   }
   else
      m_time_indent=0;
}

// Logging assumes that anything we're asked to log won't have a final CR/LF at the end.  That's why we write it out ourselves.
// But, it can handle having CR/LFs in the middle of the text being logged.
void Log::Write(ConstString text)
{
   Assert(m_file_log);
   Assert(!m_HTML);

   if(!text) return;

   // No wrapping or nothing to wrap
   if(!mp_prop_logging.fWrap())
   {
      m_file_log.Write(text);
      return;
   }

   while(true)
   {
      unsigned maxLength=max(mp_prop_logging.Wrap()-m_line_pos, 20U); // Minimum length of 20 chars for sanity
      maxLength=min(maxLength, text.Length());

      // Is there an embedded CR/LF in the string?
      unsigned iCRPos=text.First(maxLength).Find(ConstString{CRLF});
      if(iCRPos!=Strings::Result::Not_Found)
      {
         m_file_log.Write(text.First(iCRPos+2));
         m_line_pos=0;
         text=text.WithoutFirst(iCRPos+2); // Skip over the CR/LF in the string

         if(!text)
            return; // All done
      }
      else // We have to figure out where to wrap the text
      {
         if(maxLength==text.Length()) // No wrap?
         {
            m_line_pos+=text.Length(); m_file_log.Write(text);
            return;
         }
         else // Wrap
         {
            if(mp_prop_logging.fWrapNearestWord())
               maxLength=text.First(maxLength).FindLastOf(' ', maxLength);

            m_file_log.Write(text.First(maxLength));
            text=text.WithoutFirst(maxLength);
            text=text.WithoutFirst(text.FindFirstNotOf(' ', text.Length()));
            m_file_log.Write(ConstString(CRLF));
            m_line_pos=0;
         }
      }

      if(mp_prop_logging.fHangingIndent())
      {
         m_line_pos=mp_prop_logging.HangingIndent()+m_time_indent;
         for(unsigned i=0;i<m_line_pos;i++)
            m_file_log.Write(ConstString(" "));
      }
   }
}

UniquePtr<RestoreLogs> RestoreLogs::Create()
{
   auto &propLogging=g_ppropGlobal->propConnections().propLogging();

   if(!propLogging.fRestoreLogs())
      return MakeUnique<RestoreLogs>(0);

   auto buffer_size=propLogging.RestoreBufferSizeCurrent()*1024;
   auto default_size=propLogging.RestoreBufferSize()*1024;

   // Ensure default size is a multiple of 64k
   {
      constexpr const unsigned multiple=64*1024; // Must be a multiple of 64K
      PinAbove(default_size, multiple);

      auto rounded_size=((default_size+multiple-1)/multiple)*multiple;
      if(default_size!=rounded_size)
      {
         default_size=rounded_size;
         propLogging.RestoreBufferSize(default_size/1024);
      }
   }

   // If the buffer size doesn't match the default size, resize it
   if(buffer_size!=default_size)
   {
      if(Resize(buffer_size, default_size))
      {
         buffer_size=default_size;
         propLogging.RestoreBufferSizeCurrent(buffer_size/1024);
         SaveConfig(nullptr);
      }
   }

   if(buffer_size%(64*1024)!=0)
   {
      Assert(false); // Can't happen unless the resize fails and we have an invalid size
      ConsoleHTML("<icon error> Restore log buffer size is invalid, setting to default size.");
      DeleteFile("Restore.dat");
      buffer_size=default_size; // Set to minimum size
   }

   return MakeUnique<RestoreLogs>(buffer_size);
}

RestoreLogs::RestoreLogs(unsigned buffer_size)
 : m_buffer_size(buffer_size),
   m_data_size(m_buffer_size-sizeof(Buffer))
{
   if(buffer_size==0)
   {
      if(DeleteFile("Restore.dat"))
         EraseAll();
      return;
   }

   File::Path path{GetConfigPath(), "Restore.dat"};
   if(!m_file.Open(path, true, true, OPEN_ALWAYS))
   {
      ConsoleHTML("<icon error> Restore logs - Can't open or create 'Restore.dat' file");
   }
   else
   {
      auto size=m_file.Size();
      if(size%m_buffer_size)
         ConsoleHTML("<icon error> Restorelogs - File is a bogus size");
      else
         m_buffer_count=size/m_buffer_size;
   }
}

RestoreLogs::RestoreLogs(ConstString path, unsigned buffer_size, unsigned buffer_count)
 : m_buffer_size(buffer_size),
   m_data_size(m_buffer_size-sizeof(Buffer))
{
   Assert(buffer_size!=0);
   if(!m_file.Open(path, true, true, OPEN_ALWAYS))
      return;

   // Try to grow the file
   if(!m_file.Seek(buffer_count*buffer_size) || !m_file.SetEnd())
      throw std::runtime_error{""};

   m_buffer_count=buffer_count;
}

RestoreLogs::~RestoreLogs()
{
   m_file.SetLastModifiedTime();
}

bool RestoreLogs::Resize(unsigned oldSize, unsigned newSize)
{
   File::Path path    {GetConfigPath(), "Restore.dat"};
   File::Path path_new{GetConfigPath(), "Restore.new"};

   try
   {
      {
         RestoreLogs old_logs(oldSize);
         if(old_logs.m_buffer_count==0)
            return true; // Nothing to do, an empty file works for any size

         // Create a new one
         RestoreLogs new_logs(path_new, newSize, old_logs.m_buffer_count);
         if(!new_logs.m_buffer_count)
            return false;

         // Copy over all of the buffers
         for(unsigned i=0;i<old_logs.m_buffer_count;i++)
         {
            auto *p_view_source=old_logs.Load(i);
            auto *p_view_dest=new_logs.Load(i);

            if(!p_view_source || !p_view_dest)
               throw std::runtime_error{""};

            MemoryZero(Array<BYTE>(reinterpret_cast<BYTE*>(static_cast<Buffer*>(p_view_dest->m_buffer)), newSize));

            RestoreLogReplay replay(old_logs, i);
            while(auto entry=replay.Read())
               p_view_dest->Write(Header{entry.m_type, entry.m_data.Count(), entry.m_time}, entry.m_data);

            old_logs.Unload(i);
            new_logs.Unload(i);
         }
      }

      // Delete the original file and rename the new one to take it's place
      if(!DeleteFile(path) || 
         !MoveFile(path_new, path))
         throw std::runtime_error{""};

      return true;
   }
   catch(const std::runtime_error &)
   {
      AssertReturned<TRUE>()==DeleteFile(path_new);
      return false;
   }
}

void RestoreLogs::CheckAndRepair()
{
   struct Info
   {
      Prop::Server *mp_server{};
      Prop::Character *mp_character{};
   };

   Collection<Info> infos;
   for(unsigned i=0;i<Count();i++)
      infos.Push();

   // Go through all the names, remove duplicates and put anything unique into the 'Info' field
   for(auto &p_server : g_ppropGlobal->propConnections().propServers())
   {
      // Iterate through the characters on the server
      for(auto &p_character : p_server->propCharacters())
      {
         auto index=p_character->RestoreLogIndex();
         if(index==-1) // No log
            continue;

         if(unsigned(index)>=Count())
         {
            if(m_buffer_size) // If this is zero, it's not an error as logs are just disabled
            {
               FixedStringBuilder<256> string{"<icon error> Restore Log ", index, " for <b>", p_server->pclName(), " - ", p_character->pclName(), "</b> is out of range, unlinking."};
               if(!Count())
                  string(" The restore log file is empty.");
               else
                  string(" Maximum index is ", infos.Count()-1);
               ConsoleHTML(string);
            }
            p_character->RestoreLogIndex(-1);
            continue;
         }

         Info &info=infos[index];
         if(info.mp_character)
         {
            ConsoleHTML(FixedStringBuilder<256>("<icon error> Restore Log ", index, " is already used by <b>", info.mp_server->pclName(), " - ", info.mp_character->pclName(), "</b>, unlinking <b>", p_server->pclName(), " - ", p_character->pclName(), "</b> from it."));
            p_character->RestoreLogIndex(-1);
            continue;
         }

         info.mp_server=p_server;
         info.mp_character=p_character;
      }
   }

   // Iterate in reverse order since Free() will not change the ordering of any previous indexes, only the later ones
   for(unsigned i=Count();i-->0;)
   {
      if(Info &info=infos[i];!info.mp_character)
      {
         ConsoleHTML(FixedStringBuilder<256>("<icon error> Deleting restore log with no characters using it, index: ", i));
         // We delete the log at index i and replace it with the last log index, so update any characters affected by it
         Free(i);
         infos.UnsortedDelete(i);
         if(i<infos.Count())
            infos[i].mp_character->RestoreLogIndex(i);
         continue;
      }
   }
}

void RestoreLogs::EraseAll()
{
   // Go through all the characters and set all restore logs to empty
   for(auto &p_server : g_ppropGlobal->propConnections().propServers())
   {
      // Iterate through the characters on the server
      for(auto &p_character : p_server->propCharacters())
         p_character->RestoreLogIndex(-1);
   }

   if(m_buffer_count)
   {
      // Close all existing mappings
      m_views.Empty();
      m_mapping.Empty();

      // Shrink the file to nothing
      m_buffer_count=0;
      AssertReturned<true>()==m_file.Seek(0);
      AssertReturned<true>()==m_file.SetEnd();
   }
}

unsigned RestoreLogs::GetUsed(unsigned index) const
{
   if(auto *p_view=Load(index))
      return p_view->m_buffer->m_count;
   return 0;
}

void RestoreLogs::WriteStart(unsigned index)
{
   if(auto *p_view=Load(index))
      p_view->Write(Header{EntryType::Start, 0}, {});
}

void RestoreLogs::WriteSent(unsigned index, Array<const BYTE> data)
{
   if(auto *p_view=Load(index))
      p_view->Write(Header{EntryType::Sent, data.Count()}, data);
}

void RestoreLogs::WriteReceived(unsigned index, Array<const BYTE> data)
{
   if(auto *p_view=Load(index))
      p_view->Write(Header{EntryType::Received, data.Count()}, data);
}

void RestoreLogs::WriteReceived_GMCP(unsigned index, Array<const BYTE> data)
{
   if(auto *p_view=Load(index))
      p_view->Write(Header{EntryType::Received_GMCP, data.Count()}, data);
}

RestoreLogs::Header RestoreLogs::View::GetHeader(unsigned index) const
{
   WrapAdd(index, m_buffer->m_start); // Make index relative to the start of the data

   // No wrap, just copy
   if(index+sizeof(Header)<m_data_size)
      return *reinterpret_cast<const Header *>(&m_buffer->m_data[index]);

   // Wrapping is rare, so this case isn't common
   std::array<BYTE, sizeof(Header)> buffer;

   // Copy the first part before the wrap, then the part after the wrap
   auto dest=std::copy(m_buffer->m_data+index, m_buffer->m_data+m_data_size, &buffer[0]);
   std::copy(m_buffer->m_data, m_buffer->m_data+index+sizeof(Header)-m_data_size, dest);

   return *reinterpret_cast<const Header *>(buffer.data());
}

void RestoreLogs::View::DeleteOldest()
{
   Header header=GetHeader(0);
   unsigned total_size=sizeof(Header)+header.m_size;

   if(total_size>m_buffer->m_count) // This will more than wipe out the whole buffer
   {
      Assert(false); // Shouldn't happen!
      m_buffer->Reset();
      ConsoleHTML(FixedStringBuilder<256>("<icon error> Restore Log had a corrupted oldest record. Clearing it to fix"));
      return;
   }

   WrapAdd(m_buffer->m_start, total_size);
   m_buffer->m_count-=total_size;
}

void RestoreLogs::View::Write(Header header, Array<const BYTE> data)
{
   if(header.m_size>=m_data_size)
      return; // Can't save what's bigger than the buffer (huge sends typically)

   Assert(header.m_size==data.Count());
   unsigned count=sizeof(Header)+data.Count();
   Assert(count<=m_data_size); // No single write should ever be larger than the buffer

   // Remove starting lines until there's room
   while(count>m_data_size-m_buffer->m_count)
      DeleteOldest();

   Write(Array<const BYTE>(reinterpret_cast<BYTE *>(&header), sizeof(header)));
   Write(data);
}

void RestoreLogs::View::Write(Array<const BYTE> data)
{
   // We should have made enough space to write the data
   Assert(m_data_size-m_buffer->m_count>=data.Count());

   auto end=m_buffer->m_start;
   WrapAdd(end, m_buffer->m_count);

   // If we wrap on write, handle the portion before the wrap
   if(end+data.Count()>=m_data_size)
   {
      unsigned count=m_data_size-end;
      CopyMemory(m_buffer->m_data+end, data.begin(), count);
      m_buffer->m_count+=count;
      end=0;
      data=data.WithoutFirst(count);
   }

   CopyMemory(m_buffer->m_data+end, data.begin(), data.Count());
   m_buffer->m_count+=data.Count();
}

Array<const BYTE> RestoreLogs::View::Read(unsigned index, unsigned count, Storage::Buffer::Fixed &wrapBuffer) const
{
   Assert(index+count<=m_buffer->m_count);
   WrapAdd(index, m_buffer->m_start); // Make index relative to the start of the data

   // If it wraps, can't do it, so return an empty array
   if(index+count<=m_data_size)
      return Array<const BYTE>(m_buffer->m_data+index, count);

   wrapBuffer.Unuse();
   if(wrapBuffer.Unused()<count)
      wrapBuffer.Allocate(count);
   auto dest=wrapBuffer.Use(count);

   // Copy before the wrap and then the remainder
   unsigned before_wrap_size=m_data_size-index;
   MemoryCopy(dest, Array<const BYTE>(m_buffer->m_data+index, before_wrap_size));
   MemoryCopy(dest.WithoutFirst(before_wrap_size), Array<const BYTE>(m_buffer->m_data, dest.Count()-before_wrap_size));
   return dest;
}

void RestoreLogs::View::WrapAdd(unsigned &v, unsigned offset) const
{
   v+=offset;
   if(v>=m_data_size)
      v-=m_data_size;
}

void RestoreLogs::Allocate(Prop::Character &propCharacter)
{
   Assert(propCharacter.fRestoreLog());

   // Close all existing mappings
   m_views.Empty();
   m_mapping.Empty();

   // Try to grow the file
   if(!m_file.Seek((m_buffer_count+1)*m_buffer_size) || !m_file.SetEnd())
      return;

   auto *p_view=Load(m_buffer_count);
   if(!p_view)
   {
      m_file.Seek(m_buffer_count*m_buffer_size);
      m_file.SetEnd();
      return;
   }

   MemoryZero(Array<BYTE>(reinterpret_cast<BYTE*>(static_cast<Buffer*>(p_view->m_buffer)), m_buffer_size));
   propCharacter.RestoreLogIndex(m_buffer_count++);
   SaveConfig(nullptr);
}

// When we delete a log, we move the last one to replace it. We need to update the character that was using that
// last index to the new one (or -1 if it was the last one)
void RestoreLogs::FixupMovedLog(int from, int to)
{
   // Find the char that's using the last buffer
   for(auto &p_server : g_ppropGlobal->propConnections().propServers())
   {
      // Iterate through the characters on the server
      for(auto &p_character : p_server->propCharacters())
      {
         if(p_character->RestoreLogIndex()==from)
         {
            p_character->RestoreLogIndex(to);
            return;
         }
      }
   }
   Assert(false); // The index wasn't used? That shouldn't be possible
}

void RestoreLogs::Free(Prop::Character &propCharacter)
{
   unsigned index=propCharacter.RestoreLogIndex();
   Free(index);
   if(index!=m_buffer_count) // If this wasn't the last buffer, the last was moved to this spot
      FixupMovedLog(m_buffer_count, index);
   propCharacter.RestoreLogIndex(-1);
   SaveConfig(nullptr);
}

void RestoreLogs::Free(unsigned index)
{
   Assert(index<m_buffer_count);
   unsigned last=m_buffer_count-1;

   // If we didn't delete the last buffer, move the last one to take it's place so we can shrink the file
   if(index!=last)
   {
      auto *p_view_last=Load(last);
      auto *p_view=Load(index);
      Assert(p_view && p_view_last);

      auto p_buffer     =Array<BYTE>(reinterpret_cast<BYTE*>(static_cast<Buffer*>(p_view     ->m_buffer)), m_buffer_size);
      auto p_buffer_last=Array<BYTE>(reinterpret_cast<BYTE*>(static_cast<Buffer*>(p_view_last->m_buffer)), m_buffer_size);

      MemoryCopy(p_buffer, p_buffer_last);
   }

   // Close all existing mappings
   m_views.Empty();
   m_mapping.Empty();

   // Shrink the file
   m_buffer_count--;
   AssertReturned<true>()==m_file.Seek(m_buffer_count*m_buffer_size);
   AssertReturned<true>()==m_file.SetEnd();
}

RestoreLogs::View *RestoreLogs::Load(unsigned index) const
{
   if(!m_mapping.IsValid() && !m_mapping.Create(m_file.GetHandle(), PAGE_READWRITE))
      return nullptr;

   while(m_views.Count()<=index)
      m_views.Push(m_data_size);

   if(!m_views[index].m_buffer && !m_views[index].m_buffer.Create(m_mapping, index*m_buffer_size, m_buffer_size))
      return nullptr;

   auto &view=m_views[index];
   if(view.m_buffer->m_count>m_data_size || view.m_buffer->m_start>m_data_size)
   {
      view.m_buffer->Reset();
      ConsoleHTML(FixedStringBuilder<256>("<icon error> Restore Log ", index, " had a corrupted header. Clearing it to fix"));
   }

   return &m_views[index];
}

void RestoreLogs::Unload(unsigned index) const
{
   m_views[index].m_buffer.Empty();
}

RestoreLogReplay::RestoreLogReplay(RestoreLogs &logs, unsigned index)
{
   mp_view=logs.Load(index);
}

RestoreLogReplay::Entry RestoreLogReplay::Read()
{
   if(!mp_view || m_index+sizeof(RestoreLogs::Header)>mp_view->m_buffer->m_count)
      return Entry{RestoreLogs::EntryType::End};

   auto header=mp_view->GetHeader(m_index);
   m_index+=sizeof(RestoreLogs::Header); // Advance by the header

   Entry entry;
   entry.m_type=header.m_type;
   entry.m_time=header.m_time;

   // Safety check to not read past the end of the buffer on an invalid sized record
   if(m_index+header.m_size>mp_view->m_buffer->m_count)
   {
      Assert(false); // This shouldn't happen
      mp_view->m_buffer->m_count=m_index-sizeof(RestoreLogs::Header);
      ConsoleHTML(FixedStringBuilder<256>("<icon error> Restore Log had a corrupted record seen on playback. Truncating to fix"));
      return Entry{RestoreLogs::EntryType::End};
   }

   entry.m_data=mp_view->Read(m_index, header.m_size, m_wrap_buffer);
   m_index+=header.m_size; // Advance by the data size

   return entry;
}
