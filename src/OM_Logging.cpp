//
// Logging Object Model
//

#include "Main.h"

#include "OM_Logging.h"
#include "Wnd_Text_OM.h"

namespace OM
{

#define ZOMBIECHECK if(!mp_log) return E_ZOMBIE;

Log::Log(::Log *pLog)
 : mp_log(pLog)
{
}

HRESULT Log::Write(BSTR bstr)
{
   ZOMBIECHECK
   mp_log->LogTextLine(*Text::Line::CreateFromText(BSTRToLStr(bstr)));
   return S_OK;
}

HRESULT Log::WriteLine(ITextWindowLine *pLine)
{
   ZOMBIECHECK
   mp_log->LogTextLine(static_cast<TextWindowLine *>(pLine)->GetInternal());
   return S_OK;
}

HRESULT Log::get_FileName(BSTR *retval)
{
   ZOMBIECHECK
   *retval=LStrToBSTR(mp_log->GetFileName()); return S_OK;
}

}
