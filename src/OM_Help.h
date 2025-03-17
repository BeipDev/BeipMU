#pragma once
//
// Object Model Helper Functions & Classes
// Include this instead of OM.h (or put OM.h in the OM namespace)
//
namespace OM
{

// Object Creation (where the internal header isn't needed)
interface IWindow_Graphics;

IWindow_Graphics  *Create_Window_Graphics(int2 sz);
IWindow_FixedText *Create_Window_FixedText(int2 sz);

HRESULT LoadTypeInfoFromThisModule(REFIID riid, ITypeInfo **ppti);

#define E_ZOMBIE E_POINTER

template<typename T> struct RefReturner
// Enforces the addref behavior when returning interface pointers to a caller
// also simplifies it.
//
// Usage:
//   RefReturner returner(retval); // retval should be the return value name 99.9% of the time
//   return returner(<object to return>);
{
   RefReturner(T **ppT) : m_ppT(ppT) { *m_ppT=nullptr; }

   HRESULT operator()(T *p, HRESULT hr=S_OK)  { *m_ppT=CntPtrTo<T>(p).Extract(); return hr; }
   template<typename TDerived> HRESULT operator()(CntPtrTo<TDerived> &&p, HRESULT hr=S_OK) { *m_ppT=p.Extract(); return hr; }

private:
   T **m_ppT;
};

// Function version of the above for trivial cases
template<typename T, typename V> HRESULT ReturnRef(T **retval, V &pValueToReturn) { return RefReturner<T>(retval)(pValueToReturn); }

struct Variant;

struct VariantBool
{
   VariantBool(bool f) : m_bool(f ? VARIANT_TRUE : VARIANT_FALSE) { }
   VariantBool(VARIANT_BOOL f) : m_bool(f) { }

   operator bool() const { return m_bool==VARIANT_TRUE; }

private:
   VARIANT_BOOL m_bool;
   friend struct Variant;
};

// Done this way because if they are combined, the operators are ambiguous
// This guy is used to return a boolean value as a VARIANT_BOOL
inline VARIANT_BOOL ToVariantBool(bool f) { return f ? VARIANT_TRUE : VARIANT_FALSE; }
inline VARIANT_BOOL BoolVariant(bool f) { return f ? VARIANT_TRUE : VARIANT_FALSE; }

template<typename T> constexpr VARTYPE TypeToVT = VT_UNKNOWN;
template<> constexpr VARTYPE TypeToVT<float> = VT_R4;
template<> constexpr VARTYPE TypeToVT<int8> = VT_I1;
template<> constexpr VARTYPE TypeToVT<uint8> = VT_UI1;
template<> constexpr VARTYPE TypeToVT<int32> = VT_I4;
template<> constexpr VARTYPE TypeToVT<uint32> = VT_UI4;
template<> constexpr VARTYPE TypeToVT<BSTR> = VT_BSTR;
template<> constexpr VARTYPE TypeToVT<IDispatch*> = VT_DISPATCH;
template<> constexpr VARTYPE TypeToVT<VARIANT_BOOL> = VT_BOOL;

static_assert(sizeof(VARIANT::intVal)==sizeof(int32));
static_assert(sizeof(VARIANT::uintVal)==sizeof(uint32));

struct Variant : VARIANT
{
   Variant() noexcept { VariantInit(this); }
   Variant(Variant &&var) noexcept : Variant() { VARIANT::operator=(std::move(var)); var.vt=VT_EMPTY; }
   Variant(const Variant &var) noexcept : Variant() { operator=(var); }
   Variant(const VARIANTARG &var) noexcept : Variant() { operator=(var); }
   Variant(ConstString string) noexcept : Variant() { operator=(string); }
   Variant(IDispatch *pdisp) noexcept : Variant() { operator=(pdisp); }
   Variant(int32 v) noexcept : Variant() { operator=(v); }
   Variant(int8 v) noexcept : Variant() { operator=(v); }
   Variant(bool v) noexcept : Variant() { operator=(v); }
   Variant(float v) noexcept : Variant() { operator=(v); }
   Variant(VariantBool *p) : Variant() { vt=TypeToVT<VARIANT_BOOL>|VT_BYREF; pboolVal=&p->m_bool; }
   ~Variant() noexcept { Clear(); }
   void Clear() noexcept { if(vt!=VT_EMPTY) VariantClear(this); }

   template<typename T> bool Convert(const VARIANTARG &var) { return SUCCEEDED(VariantChangeType(this, &var, 0, TypeToVT<T>)); }
   template<typename T> bool Is() const noexcept { return vt==TypeToVT<T>; }

   template<typename T> T &Get();
   template<> float &Get() { Assert(vt==TypeToVT<float>); return fltVal; }
   template<> int8 &Get() { Assert(vt==TypeToVT<int8>); return *reinterpret_cast<int8*>(&cVal); }
   template<> uint8 &Get() { Assert(vt==TypeToVT<uint8>); return *reinterpret_cast<uint8*>(&cVal); }
   template<> int32 &Get() { Assert(vt==TypeToVT<int32>); return intVal; }
   template<> uint32 &Get() { Assert(vt==TypeToVT<uint32>); return uintVal; }
   template<> IDispatch* &Get() { Assert(vt==TypeToVT<IDispatch*>); return pdispVal; }
   template<> VARIANT_BOOL &Get() { Assert(vt==TypeToVT<VARIANT_BOOL>); return boolVal; }

   void operator=(Variant &&var) noexcept { Clear(); *this=var; var.vt=VT_EMPTY; }
   void operator=(const Variant &var) noexcept { VariantCopy(this, &var); }
   void operator=(const VARIANTARG &var) noexcept { VariantCopy(this, &var); }
   void operator=(ConstString string);
   void operator=(IDispatch *v) noexcept { Clear(); vt=TypeToVT<IDispatch*>; Get<IDispatch*>()=v; Get<IDispatch*>()->AddRef(); }
   void operator=(int32 v) noexcept { Clear(); vt=TypeToVT<int32>; Get<int32>()=v; }
   void operator=(int8 v) noexcept { Clear(); vt=TypeToVT<int8>; Get<int8>()=v; }
   void operator=(bool f) noexcept { Clear(); vt=TypeToVT<VARIANT_BOOL>; boolVal=BoolVariant(f); }
   void operator=(float v) noexcept { Clear(); vt=TypeToVT<float>; Get<float>()=v; }

   bool operator==(bool f) const noexcept { return Is<VARIANT_BOOL>() && boolVal==BoolVariant(f); }
};

template<typename T>
struct __declspec(novtable) Dispatch : General::Unknown<T>
{
   Dispatch()
   {
      LoadTypeInfoFromThisModule(__uuidof(T), m_pTypeInfo.Address());
   }

   // IUnknown methods
   STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
   {
      return TQueryInterface(riid, ppv);
   }

   // IDispatch methods
   STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) override
   {
      *pctinfo = 1; return S_OK;
   }

   STDMETHODIMP GetTypeInfo(UINT ctinfo, LCID lcid, ITypeInfo **ppti) override
   {
      if(ctinfo!=0)
      {
         if(ppti)
            *ppti=nullptr;
         return ResultFromScode(DISP_E_BADINDEX);
      }

      (*ppti = m_pTypeInfo)->AddRef(); return S_OK;
   }

   STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames,
                         UINT cNames, LCID lcid, DISPID *rgdispid) override
   {
      return m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
   }

   STDMETHODIMP Invoke(DISPID id, REFIID riid, LCID lcid, WORD wFlg,
                     DISPPARAMS *pdp, VARIANT *pvr, EXCEPINFO *pei, UINT *pu) override
   {
      // WebView2 causes an access violation in Invoke when parameters are invalid. Catch it here vs just crashing entirely
      __try
      {
         return m_pTypeInfo->Invoke((T *)this, id, wFlg, pdp, pvr, pei, pu);
      }
      __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
      {
         return E_INVALIDARG;
      }
   }

   using General::Unknown<T>::TQueryInterface;

private:

   CntPtrTo<ITypeInfo> m_pTypeInfo;
};

template<typename T>
struct __declspec(novtable) ProvideClassInfo : IProvideClassInfo
{
   ProvideClassInfo()
   {
      LoadTypeInfoFromThisModule(__uuidof(T), m_pTypeInfo.Address());
   }

   STDMETHODIMP GetClassInfo(ITypeInfo **ppTI) override
   {
      if(ppTI==nullptr) return E_POINTER;
      *ppTI=m_pTypeInfo; m_pTypeInfo->AddRef(); return S_OK;
   }

private:

   CntPtrTo<ITypeInfo> m_pTypeInfo;
};

//
// ** WARNING **
//
// To prevent circular reference counts caused by scripts, where the user
// set data that Beip stores may reference a Beip object, we store all
// user data in a list that we 'release' when we're trying to shut down
// or reset the scripting engine.
//
struct VariantNode : Variant, DLNode<VariantNode>
{
   VariantNode();
   using Variant::operator=;
};

struct DispatchNode : DLNode<DispatchNode>
{
   DispatchNode();
   CntPtrTo<IDispatch> mp_disp;
   CntPtrTo<IDispatchEx> mp_dispex;
};

void InitHooks();
void ClearHooks();

struct Hook
{
   operator bool() const { return mp_disp.mp_disp!=nullptr; }

   void Clear() { mp_disp.mp_disp=nullptr; mp_disp.mp_dispex=nullptr;  }
   void Set(IDispatch *pNewDisp) { mp_disp.mp_disp=pNewDisp; mp_disp.mp_dispex=nullptr;  pNewDisp->QueryInterface(mp_disp.mp_dispex.Address()); }

   HRESULT operator()()
   {
      return Invoke(nullptr, 0, nullptr);
   }

   template<typename... Params> HRESULT operator()(Params&&... params)
   {
      Variant var[]={ std::forward<Params>(params)... };
      return Invoke(var, _countof(var), nullptr);
   }

   template<typename... Params> Variant CallWithResult(Params&&... params)
   {
      Variant var[]={ std::forward<Params>(params)... };
      Variant result;
      if(FAILED(Invoke(var, _countof(var), &result)))
         result.Clear();
      return result;
   }

protected:
   HRESULT Invoke(Variant *pvars, unsigned int iVars, Variant *pResult);
private:
   DispatchNode mp_disp; // The dispatch of the event hook
};

struct HookVariant : Hook
{
   bool Call(); // Call passing in our variant and returning true if the result is true or false otherwise (failure, etc).

   void Clear() { __super::Clear(); var.Clear(); }
   void Set(IDispatch *pNewDisp, VARIANTARG &newVar) { __super::Set(pNewDisp); var=newVar; }

   VariantNode var; // User data passed to the hook (and passed in when the hook is set)
};

// ManageHook for when the class inherits from CntReceiverOf (should fail to compile if you give the wrong class)
// Sets a Hook based on the pDisp, also attaches/detaches the event
template<typename TEvent, typename TBase, typename TSender>
HRESULT ManageHook(TBase *pBase, Hook &hook, TSender &sender, IDispatch *pDisp)
{
   auto &receiver=static_cast<Events::ReceiverOf<TEvent> &>(*pBase);

   if(hook)
      receiver.Detach();
   hook.Set(pDisp);

   if(pDisp)
      receiver.AttachTo(sender);
   return S_OK;
}

// ManageHook for when the class inherits from CntReceiverOf (should fail to compile if you give the wrong class)
// Sets a HookVariant based on the pDisp and var, also attaches/detaches the event
template<typename TEvent, typename TBase, typename TSender>
HRESULT ManageHook(TBase *pBase, HookVariant &hook, TSender &sender, IDispatch *pDisp, VARIANT &var)
{
   if(hook)
      pBase->Detach<TEvent>();
   hook.Set(pDisp, var);

   if(pDisp)
      pBase->AttachTo<TEvent>(sender);
   return S_OK;
}

OwnedBSTR ReadFileAsBSTR(ConstString fileName);
HRESULT SystemTimeToVariant(VARIANT &var, const Time::Time &time);

};
