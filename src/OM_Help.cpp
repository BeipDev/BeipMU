//
// OM Helper Functions
//

#include "Main.h"
#include "OM_Help.h"

static ConstString s_help[] =
{
// None
"",
// WebView::SetOnDisplayCapture
R"(Register a callback to call for a range of lines about to be displayed. The regex determines when the range starts and ends.
When the regex_begin matches, OnCaptureChanged will be called with 'starting=true', then for each line in the capture, OnCapture will be called. When a line matching regex_end occurs, OnCaptureChagned will be called with 'starting=false'

Callback function signatures:
  void OnCapture(int id, ITextWindowLine line)
  void OnCaptureChanged(int id, ITextWindowLine line, bool starting))",
// App::CreateInterval
R"(Register a callback to be called repeatedly every time interval.
Callback function signature: <font color='lime'>OnTimer</font>(variant userdata)

Example Usage:
function TimerCallback(ud) { ud.window.output.write('Timer called with userdata: ' + ud.text); }
app.CreateTimeout(1000, TimerCallback, { window: window, text: 'Test Timeout'});)",
// App::CreateTimeout
"Register a callback to be called once after the time elapses. See 'CreateInterval' for the callback syntax and code sample'"
};

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

static CntPtrTo<ITypeLib> sp_type_lib;

ITypeLib &GetTypeLib()
{
   if(!sp_type_lib)
      LoadTypeLibEx(UTF16(ModuleFileName()).stringz(), REGKIND_NONE, sp_type_lib.Address());

   Assert(sp_type_lib);
   return *sp_type_lib;
}

void LoadTypeInfoFromThisModule(REFIID riid, ITypeInfo **ppti) 
{
   *ppti = 0;
   HRESULT hr=GetTypeLib().GetTypeInfoOfGuid(riid, ppti);
   Assert(SUCCEEDED(hr));
   hr;
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

void TypeToString(ITypeInfo &ti, const TYPEDESC &td, StringBuilder &string)
{
   string("<font color='#00FFFF'>");
   scope_success _([&] { string("</font>"); });

   if(td.vt == VT_PTR && td.lptdesc)
   {
      TypeToString(ti, *td.lptdesc, string);
      string('*');
      return;
   }

   if(td.vt == VT_USERDEFINED)
   {
      CntPtrTo<ITypeInfo> pti;
      if(FAILED(ti.GetRefTypeInfo(td.hreftype, pti.Address())) || !pti)
      {
         string("unknown");
         return;
      }
      OwnedBSTR bstrName;
      pti->GetDocumentation(MEMBERID_NIL, bstrName.Address(), nullptr, nullptr, nullptr);
      BSTRToLStr name(bstrName);
      string("<run text='/shelp ", name, "'>", name, "</run>");
      return;
   }

   switch(td.vt)
   {
      case VT_I1: string("int8"); break;
      case VT_I2: string("int16"); break;
      case VT_I4: string("int32"); break;
      case VT_R4: string("float32"); break;
      case VT_R8: string("float64"); break;
      case VT_DATE: string("date"); break;
      case VT_BSTR: string("string"); break;
      case VT_DISPATCH: string("callback"); break;
      case VT_BOOL: string("bool"); break;
      case VT_VARIANT: string("variant"); break;
      case VT_UI1: string("uint8"); break;
      case VT_UI2: string("uint16"); break;
      case VT_UI4: string("uint32"); break;
      case VT_INT: string("int"); break;
      case VT_UINT: string("uint"); break;
      case VT_VOID: string("void"); break;
      case VT_PTR: string("pointer"); break;
      default: string("{unknown ", td.vt, "}"); break;
   }
}

void DisplayTypeInfo(ITypeInfo &ti, Text::Wnd &wnd)
{
   HybridStringBuilder string;

   // Prepend type name and documentation
   {
      OwnedBSTR type_name, type_help;
      DWORD help_context{};
      // MEMBERID_NIL requests documentation for the type itself
      ti.GetDocumentation(MEMBERID_NIL, type_name.Address(), type_help.Address(), &help_context, nullptr);

      string("<p background-color='#004000' stroke-color='#008000' stroke-width='2' border='10' border-style='round' padding='2'>",
         "<font size='20'>", BSTRToLStr(type_name), "</font>");

      if(type_help)
         string("\n", BSTRToLStr(type_help));
      if(help_context)
         string(BSTRToLStr(s_help[help_context]));

      string("</p>");
      wnd.Add(CreateLineInternal(string)); string.Clear();
   }

   TYPEATTR *p_ta{};
   if(FAILED(ti.GetTypeAttr(&p_ta)) || !p_ta)
      return;
   scope_exit _{[&]() { ti.ReleaseTypeAttr(p_ta); }};

   for(UINT i = 0; i < p_ta->cFuncs; i++)
   {
      FUNCDESC *p_fd{};
      if(FAILED(ti.GetFuncDesc(i, &p_fd) || !p_fd))
         continue;
      scope_exit _{[&]() { ti.ReleaseFuncDesc(p_fd); }};

      if(p_fd->wFuncFlags & FUNCFLAG_FHIDDEN)
         continue; // string("[hidden] ");
      if(p_fd->wFuncFlags & FUNCFLAG_FRESTRICTED)
         continue; // string("[restricted] ");
      if(p_fd->wFuncFlags & FUNCFLAG_FDEFAULTBIND)
         string("[defaultbind] ");
      if(p_fd->memid == DISPID_VALUE)
         string("[default value] ");

      // Get names: function name + parameter names
      UINT nameCount = p_fd->cParams + 1;
      OwnedArray<BSTR> names(nameCount);
      for(auto &name : names)
         name = nullptr;

      UINT cGot = 0;
      ti.GetNames(p_fd->memid, names.begin(), nameCount, &cGot);
      scope_exit _{[&]() {
         for(auto name : names)
            if(name)
               SysFreeString(name);
      }};

      if(cGot==0 || !names[0])
         continue;

      string("<font color='lime'>", names[0], "</font>");

      switch(p_fd->invkind)
      {
         case INVOKE_FUNC:
         {
            string('(');
            // Parameters
            for(UINT i = 0; i < UINT(p_fd->cParams); ++i)
            {
               if(i)
                  string(", ");

               auto &param=p_fd->lprgelemdescParam[i];
               auto &pd=param.paramdesc;
               TypeToString(ti, param.tdesc, string);
               if(i + 1 < cGot && names[i+1])
                  string(' ', names[i+1]);

               if(pd.wParamFlags & PARAMFLAG_FOPT)
               {
                  if((pd.wParamFlags & PARAMFLAG_FHASDEFAULT) && pd.pparamdescex)
                  {
                     string(" = ");
                     const VARIANT &v = pd.pparamdescex->varDefaultValue;
                     if(v.vt==VT_DISPATCH && v.pdispVal==nullptr)
                     {
                        string("null");
                     }
                     else if(v.vt==VT_BOOL)
                     {
                        if(v.boolVal!=0)
                           string("true");
                        else
                           string("false");
                     }
                     else
                     {
                        Variant vstring;
                        if(vstring.Convert<BSTR>(v))
                           string(BSTRToLStr(vstring.bstrVal));
                        else
                           string("(unsupported default value)");
                     }
                  }
               }
            }

            string(")");

            // Return type
            if(p_fd->elemdescFunc.tdesc.vt!=VT_VOID)
            {
               string(" -> ");
               TypeToString(ti, p_fd->elemdescFunc.tdesc, string);
            }
            break;
         }

         case INVOKE_PROPERTYGET:
         {
            string(' ');
            TypeToString(ti, p_fd->elemdescFunc.tdesc, string);
            bool read_write=false;

            // If next function is the INVOKE_PROPERTYPUT, then skip it and say [read/write]
            if(i + 1 < p_ta->cFuncs)
            {
               FUNCDESC *p_fd_next{};
               if(SUCCEEDED(ti.GetFuncDesc(i + 1, &p_fd_next)) && p_fd_next)
               {
                  scope_exit _{[&]() { ti.ReleaseFuncDesc(p_fd_next); }};
                  if(p_fd_next->invkind == INVOKE_PROPERTYPUT && p_fd_next->memid == p_fd->memid)
                  {
                     read_write=true;
                     ++i; // Skip the next one
                  }
               }
            }

            if(read_write)
               string(" [read/write]");
            else
               string(" [read]");
            break;
         }

         case INVOKE_PROPERTYPUT:
            string(' ');
            TypeToString(ti, p_fd->lprgelemdescParam[0].tdesc, string);
            string(" [write]");
            break;
         case INVOKE_PROPERTYPUTREF:
            string(' ');
            TypeToString(ti, p_fd->lprgelemdescParam[0].tdesc, string);
            string(" [writeref]");
            break;
      }

      wnd.Add(CreateLineInternal(string));
      string.Clear();

      // Documentation string
      OwnedBSTR bstrDoc, bstrHelpFile;
      DWORD help_context{};
      ti.GetDocumentation(p_fd->memid, nullptr, bstrDoc.Address(), &help_context, bstrHelpFile.Address());
      if(bstrDoc || help_context)
      {
         string("<p indent='2' border='10' border-style='round'>", BSTRToLStr(bstrDoc));
         string(s_help[help_context]);
         wnd.Add(CreateLineInternal(string));
         string.Clear();
      }
   }
}

void GetOMHelp(ConstString topic, Text::Wnd &wnd)
{
   auto &tl=GetTypeLib();

   if(topic.ICompare("all")==0)
   {
      // Show all types in the type library
      UINT type_count=tl.GetTypeInfoCount();
      for(UINT i=0; i<type_count; i++)
      {
         CntPtrTo<ITypeInfo> p_type_info;
         if(FAILED(tl.GetTypeInfo(i, p_type_info.Address())) || !p_type_info)
            continue;

         OwnedBSTR type_name;
         p_type_info->GetDocumentation(MEMBERID_NIL, type_name.Address(), nullptr, nullptr, nullptr);
         wnd.Add(CreateLineInternal(BSTRToLStr(type_name)));
      }
      return;
   }

   CntPtrTo<ITypeInfo> p_type_info;
   MEMBERID member_ids[1];
   USHORT ids_found=1;
   tl.FindName(OwnedBSTR(topic), 0, p_type_info.Address(), member_ids, &ids_found);
   if(p_type_info)
      DisplayTypeInfo(*p_type_info, wnd);
   else
      wnd.AddHTML("Type not found");
}

};
