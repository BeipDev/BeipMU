//
// MCP-SimpleEdit
//

#include "Main.h"
#include "MCP.h"
#include "MCP_SimpleEdit.h"
#include "Connection.h"

namespace MCP
{

Editor::Editor(Parser &parser, const Message &msg)
:  m_parser(parser),
   m_reference(msg["reference"]),
   m_type(msg["type"]),
   m_name(msg["name"])
{
   parser.connection().Text(FixedStringBuilder<256>("mcp-simpleedit Editing:\"", m_name, "\" Type:", m_type));

   if(!m_filename)
      throw std::exception{};

   {
      File::Write_Only file;
      if(!file.Create(m_filename))
         throw std::exception{};

      const Keylist *p_list=msg.GetKeylist("content");
      if(p_list)
      {
         for(auto &item : p_list->m_root)
         {
            file.Write(item.value);
            file.Write(ConstString(CRLF));
         }
      }
      else
      {
         const Keyval *p_val=msg.GetKeyval("content");
         if(p_val)
         {
            file.Write(p_val->value());
            file.Write(ConstString(CRLF));
         }
         else
         {
            // No list, so it's an empty file
         }
      }

      m_originalsize=file.Size();
      m_lastwrite=file.GetLastWrite();
   }

   STARTUPINFO si{};
   si.cb=sizeof(si);

   HybridStringBuilder<> string("notepad.exe ", m_filename);

   AssertReturned<0>()!=CreateProcess(nullptr, UnconstPtr(UTF16(string).stringz()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &m_piEdit);
   m_thread=Kernel::Thread([this]() { ThreadProc(); });
}

Editor::~Editor()
{
   // Wait for the editor to finish
   m_event.Set();
   m_thread.Join();
}

void Editor::ThreadProc()
{
   HANDLE objects[]={ m_piEdit.hProcess, m_event };
   DWORD dwObject=WaitForMultipleObjects(_countof(objects), objects, FALSE, INFINITE);

   if(dwObject==WAIT_OBJECT_0) // Editor exited?
   {
      // Save file back
      m_poster.Post([this]() { EditorClosed(); });
   }
   else
   {
      // Kill the editor
      TerminateProcess(m_piEdit.hProcess, 0);
   }
}

void Editor::EditorClosed()
{
   UniquePtr<Editor> p(this); // To delete ourselves on exit

   File::Read_Only file;
   if(!file.Open(m_filename))
      return; // Something bad happened

   auto lastwrite=file.GetLastWrite();
   DWORD filesize=file.Size();

   if(lastwrite==m_lastwrite && filesize==m_originalsize) // Nothing changed
   {
      m_parser.connection().Text("mcp-simpleedit Done editing, no changes detected, so nothing to upload (same write time and same size)");
      return;
   }

   m_parser.connection().Text("mcp-simpleedit Done editing, uploading changes...");

   Message message("dns-org-mud-moo-simpleedit", "set");
   message.AddParam("reference", m_reference);
   message.AddParam("type", m_type);

   if(unsigned size=file.Size())
   {
      auto buffer=file.Read();
      if(!buffer)
         return; // Something bad happened

      auto p_key_list=MakeUnique<Keylist>();
      p_key_list->m_key="content";

      unsigned start=0;

      for(unsigned i=0;i<size;i++)
      {
         if(buffer[i]==CHAR_CR)
         {
            auto p_item=MakeUnique<Keylist::Item>();
            p_item->value=ConstString(reinterpret_cast<const char *>(&buffer[start]), i-start);
            p_item->Link(p_key_list->m_root.Prev());
            p_item.Extract();
            start=i+1;
            continue;
         }

         if(start==i && buffer[i]==CHAR_LF)
            start=i+1; // Skip the CHAR_LF
      }

      // Final partial line?
      if(start<size)
      {
         auto p_item=MakeUnique<Keylist::Item>();
         p_item->value=ConstString(reinterpret_cast<const char *>(&buffer[start]), size-start);
         p_item->Link(p_key_list->m_root.Prev());
         p_item.Extract();
      }

      message.AddKeylist(p_key_list);
   }
   // else size is zero

   m_parser.Send(message);
   m_parser.connection().Text("mcp-simpleedit Changes uploaded");
}

Package_SimpleEdit::Package_SimpleEdit(const PackageInfo &info, Parser &parser)
:  Package(parser, info)
{
}

Package_SimpleEdit::~Package_SimpleEdit()
{
}

void Package_SimpleEdit::On(const Message &msg)
{
   try
   {
      (new Editor(m_parser, msg))->Link(m_editors.Prev());
   }
   catch(const std::exception &)
   {  
      m_parser.connection().Text("mcp-simpleedit Error - couldn't create temp file");
   }
}

PackageFactory_SimpleEdit::PackageFactory_SimpleEdit()
{

}

UniquePtr<Package> PackageFactory_SimpleEdit::Create(Parser &parser, int iVersion)
{
   return MakeUnique<Package_SimpleEdit>(info(), parser);
}

UniquePtr<PackageFactory> CreatePackageFactory_SimpleEdit()
{
   return MakeUnique<PackageFactory_SimpleEdit>();
}

}
