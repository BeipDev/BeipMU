//
// Logging Object Model
//
#include "OM_Help.h"
#include "Logging.h"

namespace OM
{

struct Log : Dispatch<ILog>
{
   Log(::Log *pLog);

   // IUnknown

   // ILog
   STDMETHODIMP Write(BSTR bstr) override;
   STDMETHODIMP WriteLine(ITextWindowLine *pLine) override;
   STDMETHODIMP get_FileName(BSTR *bstr) override;

   NotifiedPtrTo<::Log> mp_log;
};

}
