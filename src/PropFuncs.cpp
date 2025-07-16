//
// Property Tree Functions
//

#include "Main.h"
#include "Sounds.h"
#include "Logging.h"

namespace Prop
{

//
// Font
//

Handle<HFONT> Font::CreateFont() const
{
   return ::CreateFont(-Size()*g_dpiScale, 0, 0, 0, (m_fBold ? FW_BOLD : FW_NORMAL),
                       m_fItalic, m_fUnderline, m_fStrikeout,
                       m_CharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, UTF16(pclName()).stringz());
}

Handle<HFONT> Font::CreateFont(bool fBold, bool fItalic, bool fUnderline, bool fStrikeout) const
{
   return ::CreateFont(-Size()*g_dpiScale, 0, 0, 0,
                       fBold ? FW_BOLD : FW_NORMAL, fItalic, fUnderline, fStrikeout,
                       m_CharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, UTF16(pclName()).stringz());
}

void Font::ToLogFont(LOGFONT &lf) const
{
   lf.lfHeight=-Size()*g_dpiScale;
   lf.lfWidth=0;
   lf.lfEscapement=0;
   lf.lfOrientation=0;
   lf.lfWeight=(m_fBold ? FW_BOLD : FW_NORMAL);
   lf.lfItalic=m_fItalic;
   lf.lfUnderline=m_fUnderline;
   lf.lfStrikeOut=m_fStrikeout;
   lf.lfCharSet=m_CharSet;
   lf.lfOutPrecision=OUT_DEVICE_PRECIS;
   lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
   lf.lfQuality=DEFAULT_QUALITY;
   lf.lfPitchAndFamily=DEFAULT_PITCH|FF_MODERN;
   UTF16_Inplace(lf.lfFaceName, pclName());
}

void Font::FromLogFont(const LOGFONT &lf)
{
   Size(-lf.lfHeight/g_dpiScale);
   m_fBold=(lf.lfWeight==FW_BOLD) ? true : false;
   m_fItalic=lf.lfItalic!=0;
   m_fUnderline=lf.lfUnderline!=0;
   m_fStrikeout=lf.lfStrikeOut!=0;
   m_CharSet=lf.lfCharSet;

   pclName(WzToString(lf.lfFaceName));
}

bool Font::ChooseFont(HWND hWnd)
{
   CHOOSEFONT cf{};

   cf.lStructSize=sizeof(CHOOSEFONT);
   cf.hwndOwner=hWnd;
   cf.Flags=CF_INITTOLOGFONTSTRUCT;
   cf.nFontType=SCREEN_FONTTYPE;

   LOGFONT lf;
   ToLogFont(lf);

   cf.lpLogFont=&lf;

   OSModalDialog _;
   if(::ChooseFont(&cf))
   {
      FromLogFont(lf);
      return true;
   }

   return false;
}

//
// FindString
//
bool FindString::Find(ConstString string, unsigned start_index, uint2 &range) const
{
   FixedArray<uint2, 15> ranges;
   auto result=Find(string, start_index, ranges);
   if(result.Count()==0)
      return false;
   range=ranges[0];
   return true;
}

Array<uint2> FindString::Find(ConstString string, unsigned start_index, Array<uint2> ranges) const
{
   if(fRegularExpression())
   {
      if(!mp_regex_cache)
         mp_regex_cache=MakeUnique<RegEx::Expression>(pclMatchText(), (fMatchCase() ? 0 : PCRE2_CASELESS)|PCRE2_UTF);

      if(fForward())
         return mp_regex_cache->Find(string, start_index, ranges);

      // For a backwards search, look for the earliest search that ends before the start character
      // We do that by iterating forward, then backing up to the last valid match
      uint2 found;
      while(true)
      {
         auto result=mp_regex_cache->Find(string, found.end, ranges);
         if(!result)
            break;

         // If this search ends at the same location as the previous search, skip it, it's smaller.
         // Also skip it if it goes past the start character as it's out of bounds.
         if(result[0].end==found.end || result[0].end>start_index)
            break;

         found=result[0];
      }

      if(found.end==0)
         return {};

      return mp_regex_cache->Find(string, found.begin, ranges);
   }

   // Special case for an empty match, this will find 'nothing' at the start of the line
   if(!pclMatchText())
   {
      ranges[0]={};
      return ranges.First(1);
   }

   unsigned found_index=Strings::Result::Not_Found;
   ConstString match_text=pclMatchText();

   auto IsWholeWord=[&]()
   {
      return (found_index==0 || !IsLetter(string[found_index-1]) &&
         (found_index+match_text.Length()==string.Length() || !IsLetter(string[found_index+match_text.Length()])));
   };

   while(true)
   {
      if(fForward())
      {
         if(fStartsWith())
         {
            if(start_index==0 && (fMatchCase() ? string.StartsWith(match_text) : string.IStartsWith(match_text)))
            {
               if(fEndsWith() && string.Length()!=match_text.Length())
                  return {}; // Not found

               found_index=0;
               break;
            }
            return {}; // Not found
         }

         if(fEndsWith())
         {
            if(start_index==0 && start_index<=string.Length()-match_text.Length() && (fMatchCase() ? string.EndsWith(match_text) : string.IEndsWith(match_text) ))
            {
               found_index=string.Length()-match_text.Length();
               break;
            }
            return {}; // Not found
         }

         while(true)
         {
            found_index=fMatchCase() ? string.FindAt(match_text, start_index) : string.IFindAt(match_text, start_index);
            if(fWholeWord() && found_index!=Strings::Result::Not_Found && !IsWholeWord())
            {
               start_index=found_index+1;
               continue;
            }
            break;
         }
      }
      else
      {
         // TODO: We don't scan for Starts with or Ends With when going backwards
         Assert(fStartsWith()==false && fEndsWith()==false);
         found_index=fMatchCase() ? string.First(start_index).RFind(match_text) : string.First(start_index).RIFind(match_text);
      }

      if(found_index==Strings::Result::Not_Found)
         return {};

      break;
   }

   if(fWholeWord() && !IsWholeWord())
      return {};

   ranges[0]=uint2(found_index, found_index+match_text.Length());
   return ranges.First(1);
}

bool FindString::operator==(const FindString &pfs) const
{
   // Right now we don't care if these differ, it's still the same trigger string
   if(
//fForward!=fs.fForward ||
//      fStartsWith!=fs.fStartsWith ||
//      fEndsWith!=fs.fEndsWith ||
//      fWholeWord!=fs.fWholeWord ||
      fMatchCase()!=pfs.fMatchCase())
      return false;

   if(fMatchCase() && pclMatchText()==pfs.pclMatchText())
      return true;

   if(pclMatchText().ICompare(pfs.pclMatchText())==0)
      return true;

   return false;
}

unsigned Variables::Find(ConstString name) const
{
   for(unsigned i=0; i<Count(); i++)
      if((*this)[i]->pclName()==name)
         return i;
   return Strings::Result::Not_Found;
}

void Variables::Add(ConstString name, ConstString value)
{
   auto &variable=*Push(MakeUnique<Prop::Variable>());
   variable.pclName(name);
   variable.pclValue(value);
}

bool TextWindow::ShouldCopy() const
{
   // Only copy if we're not the global settings
   return this!=&GlobalTextSettings();
}

bool InputWindow::ShouldCopy() const
{
   // Only copy if we're not the global settings
   return this!=&GlobalInputSettings();
}


RegEx::Expression *Trigger_Spawn::GetCaptureUntilRegEx() const
{
   if(!mp_regex_cache && m_pclCaptureUntil)
      mp_regex_cache=MakeUnique<RegEx::Expression>(m_pclCaptureUntil, PCRE2_UTF);

   return mp_regex_cache;
}

//
// KeyboardMacros
//

const KeyboardMacro *KeyboardMacros2::Macro(KEY_ID key_pressed) const
{
   for(auto &p : *this)
   {
      if(p->key().Matches(key_pressed))
         return p;

      if(p->fPropKeyboardMacros2())
      {
         if(auto *nested=p->propKeyboardMacros2().Macro(key_pressed))
            return nested;
      }
   }

   return nullptr;
}

//
// Puppets
//

Puppet &Puppets::New()
{
   auto p_new=MakeCounting<Puppet>();
   p_new->pclName(ConstString("*NewPuppet"));
   Push(p_new);
   return *p_new;
}

Puppet &Puppets::Copy(const Puppet &copy)
{
   auto p_new=MakeCounting<Puppet>(copy);
   return *Push(p_new);
}

//
// Characters
//

Character &Characters::New()
{
   auto p_new=MakeCounting<Character>();
   p_new->timeCreated(Time::Local());
   p_new->Rename(*this, "");
   return *Push(p_new);
}

Character &Characters::Copy(const Character &copy)
{
   auto p_new=MakeCounting<Character>(copy);
   p_new->timeCreated(Time::Local());
   p_new->BytesReceived(0);
   p_new->BytesSent(0);
   p_new->ConnectionCount(0);
   p_new->SecondsConnected(0);
   p_new->pclName(""); // Reset the name to nothing
   p_new->Rename(*this, copy.pclName()); // Then rename it to get a unique name
   p_new->RestoreLogIndex(-1); // We can never copy this
   return *Push(p_new);
}

void Characters::Delete(Character &c)
{
   if(c.RestoreLogIndex()!=-1)
      gp_restore_logs->Free(c);

   FindAndDelete(&c);
}

bool Character::Rename(Characters &characters, ConstString name)
{
   if(!name)
      name="*New Character";

   if(m_pclName.ICompare(name)==0) // Same name, then just rename
   {
      m_pclName=name;
      return true;
   }

   FixedStringBuilder<256> unique_name(name);
   
   unsigned duplicate=0;
   while(characters.FindByName(unique_name))
   {
      duplicate++;
      unique_name.Clear();
      unique_name(name, ' ', duplicate);
   }
   m_pclName=unique_name;
   return false;
}

//
// Shortcuts
//
Server &Servers::New()
{
   auto p_new=MakeCounting<Server>();
   p_new->Rename(*this, "");
   return *Push(p_new);
}

Server &Servers::Copy(const Server &copy)
{
   auto p_new=MakeCounting<Server>(copy);
   p_new->pclName(""); // Reset the name to nothing
   p_new->Rename(*this, copy.pclName()); // Then rename it to get a unique name
   for(auto &p_character : p_new->propCharacters())
      p_character->RestoreLogIndex(-1); // We can't copy this
   return *Push(p_new);
}

void Servers::Delete(Server &server)
{
   for(auto &p_character : server.propCharacters())
      if(p_character->RestoreLogIndex()!=-1)
         gp_restore_logs->Free(*p_character);

   FindAndDelete(&server);
}

bool Server::Rename(Servers &servers, ConstString name)
{
   if(!name)
      name="*New Server";

   if(m_pclName.ICompare(name)==0) // Same name, then just rename
   {
      m_pclName=name;
      return true;
   }

   FixedStringBuilder<256> unique_name(name);
   
   unsigned duplicate=0;
   while(servers.FindByName(unique_name))
   {
      duplicate++;
      unique_name.Clear();
      unique_name(name, ' ', duplicate);
   }
   m_pclName=unique_name;
   return false;
}

//
// Ansi
//

void Ansi::PlayBeep() const
{
   if(!fBeep())
      return;

   if(fBeepSystem() || !PlaySound(pclBeepFileName()))
      MessageBeep(MB_OK);
}

};
