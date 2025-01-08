//
// OM Helper Functions
//

#include "Main.h"
#include "OM_Help.h"

IServiceProvider *GetServiceProvider();

namespace OM
{

static DLNode<VariantNode> *g_pFirstVariantNode;

VariantNode::VariantNode() : DLNode<VariantNode>(g_pFirstVariantNode->Prev())
{
}

static DLNode<DispatchNode> *g_pFirstDispatchNode;

DispatchNode::DispatchNode() : DLNode<DispatchNode>(g_pFirstDispatchNode->Prev())
{
}

void InitHooks()
{
   Assert(!g_pFirstVariantNode);

   g_pFirstVariantNode=new DLNode<VariantNode>();
   g_pFirstDispatchNode=new DLNode<DispatchNode>();

   CallAtShutdown([]()
   {
      ClearHooks();
      delete g_pFirstVariantNode;
      delete g_pFirstDispatchNode;
   });
}

void ClearHooks()
{
   if(!g_pFirstVariantNode)
      return; // Already gone!

   for(auto &node : *g_pFirstVariantNode)
      node.Clear();

   for(auto &node : *g_pFirstDispatchNode)
   {
      node.mp_disp=nullptr;
      node.mp_dispex=nullptr;
   }
}

HRESULT LoadTypeInfoFromThisModule(REFIID riid, ITypeInfo **ppti) 
{
   *ppti = 0;

   HRESULT hr = E_FAIL;
   CntPtrTo<ITypeLib> ptl;
   if(SUCCEEDED(LoadTypeLibEx(UTF16(ModuleFileName()).stringz(), REGKIND_NONE, ptl.Address())))
      hr = ptl->GetTypeInfoOfGuid(riid, ppti);

   Assert(SUCCEEDED(hr));
   return hr;
}

OwnedBSTR ReadFileAsBSTR(ConstString fileName)
{
   auto data=File::Load(fileName);
   return OwnedBSTR(ConstString((char *)data.begin(), data.Count()));
}

void Variant::operator=(ConstString string)
{
   Clear();
   vt=VT_BSTR; bstrVal=OwnedBSTR(string).Extract();
}

HRESULT Hook::Invoke(Variant *pvars, unsigned varCount, Variant *pResult)
{
   DISPPARAMS dp = { pvars, 0, varCount, 0 };
   if(mp_disp.mp_dispex)
      return mp_disp.mp_dispex->InvokeEx(DISPID_VALUE, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dp, pResult, 0, GetServiceProvider());
   else
      return mp_disp.mp_disp->Invoke(DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dp, pResult, 0, 0);
}

bool HookVariant::Call()
{
   Variant varResult=CallWithResult(var);
   return varResult.vt==VT_BOOL ? varResult.boolVal!=0 : false;
}

HRESULT SystemTimeToVariant(VARIANT &var, const Time::Time &timeObject)
{
   VariantClear(&var);

   Time::Time time=timeObject; time.Stop(); // Make time stop in case it was running

   if(time.fNone()) // Return with the cleared variant (VT_EMPTY)
      return S_OK;

   DATE date;
   SystemTimeToVariantTime(const_cast<Time::Time *>(&time), &date);

   var.vt=VT_DATE;
   var.date=date;
   return S_OK;
}

};
