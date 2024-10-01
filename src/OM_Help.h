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
struct CntPtrArray
{
   CntPtrArray() noexcept
   {
   }

   ~CntPtrArray() noexcept
   {
      for(unsigned i=0;i<m_iUsed;i++)
         if(m_pObjects[i])
            m_pObjects[i]->Release();

      free(m_pObjects);
   }

   int Add(T *pT)
   {
      m_iItems++;
      if(m_iUsed<m_iSize)
         return Append(pT);

      // Look for unused spots
      for(unsigned i=0;i<m_iSize;i++)
         if(m_pObjects[i]==nullptr)
         {
            Store(i, pT); return i;
         }

      // Grow!
      m_pObjects=(T **)realloc(m_pObjects, sizeof(T *)*(m_iSize+16));
      for(int i=0;i<16;i++)
         m_pObjects[m_iSize+i]=nullptr;
      m_iSize+=16;

      return Append(pT);
   }

   void Remove(unsigned i)
   {
      m_iItems--;
      Assert(m_pObjects[i]);
      T *pObject=m_pObjects[i]; m_pObjects[i]=nullptr; pObject->Release(); 
   }

   T *operator[](unsigned i) { if(i>m_iSize) return nullptr; return m_pObjects[i]; }
   unsigned Count() const { return m_iItems; }

private:
   void Store(unsigned iPosition, T *pT) { m_pObjects[iPosition]=pT; pT->AddRef(); }
   int Append(T *pT) { Store(m_iUsed, pT); return m_iUsed++; }

   T **m_pObjects{};
   unsigned m_iSize{};
   unsigned m_iUsed{};
   unsigned m_iItems{};
};

struct EnumConnections : General::Unknown<IEnumConnections>
{
   EnumConnections(OwnedArray<CONNECTDATA> &&data) : m_data(std::move(data)) { }
   ~EnumConnections() noexcept
   {
      for(auto &x : m_data)
         x.pUnk->Release();
   }

   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override
   {
      return TQueryInterface(riid, ppvObj);
   }

   // EnumConnectionPoints
   STDMETHODIMP Next(ULONG cConnections, CONNECTDATA *rgcd, ULONG *pcFetched) override
   {
      if(cConnections==0)
         return E_INVALIDARG;
      if(rgcd==nullptr || (cConnections!=1 && pcFetched==nullptr) )
         return E_POINTER;

      ULONG cFetched=cConnections;

      PinBelow(cFetched, ULONG(m_data.Count()-m_iItem));

      for(unsigned int i=m_iItem;i<m_iItem+cFetched;i++)
      {
         rgcd[i]=m_data[i]; rgcd[i].pUnk->AddRef();
      }

      if(pcFetched)
         *pcFetched=cFetched;

      m_iItem+=cFetched;
      return cConnections==cFetched ? S_OK : S_FALSE;
   }

   STDMETHODIMP Skip(ULONG cSkip) override
   {
      if(cSkip==0)
         return E_INVALIDARG;

      if(m_iItem+cSkip>m_data.Count())
      {
         m_iItem=m_data.Count(); return S_FALSE;
      }

      m_iItem+=cSkip; return S_OK;
   }

   STDMETHODIMP Reset() override
   {
      m_iItem=0; return S_OK;
   }

   STDMETHODIMP Clone(IEnumConnections **ppEnum) override
   {
      OwnedArray<CONNECTDATA> data(m_data);
      for(auto &x : data)
         x.pUnk->AddRef();

      EnumConnections *pEnum=new EnumConnections(std::move(data)); pEnum->m_iItem=m_iItem;
      *ppEnum=pEnum; (*ppEnum)->AddRef();
      return S_OK;
   }

private:
   unsigned m_iItem{};
   OwnedArray<CONNECTDATA> m_data;
};

template<typename T, typename TEvents>
struct __declspec(novtable) ConnectionPoint : IConnectionPoint
{
   STDMETHODIMP GetConnectionInterface(IID *pIID) override
   {
      *pIID=__uuidof(TEvents); return S_OK;
   }

   STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer **ppCPC) override
   {
      T *pT=static_cast<T *>(this);
      return pT->QueryInterface(__uuidof(**ppCPC), (void **)ppCPC);
   }

   STDMETHODIMP Advise(IUnknown *pUnkSink, DWORD *pdwCookie) override
   {
      if(pUnkSink==nullptr || pdwCookie==nullptr)
         return E_POINTER;

      CntPtrTo<TEvents> pIEvents; pUnkSink->QueryInterface(pIEvents.Address()); 
      if(!pIEvents)
         return CONNECT_E_CANNOTCONNECT;

      DebugOnly(OutputDebugString("Advised: " STRINGIZE(T) "\n");)

      *pdwCookie=m_events.Add(pIEvents)+1;
      if(m_events.Count()==1) // First event?
         static_cast<T *>(this)->Advised();
      return S_OK;
   }

   STDMETHODIMP Unadvise(DWORD dwCookie) override
   {
      unsigned int iIndex=dwCookie-1; // Because we added one when passing it in
      if(m_events[iIndex]==nullptr)
         return CONNECT_E_NOCONNECTION;

      DebugOnly(OutputDebugString("Unadvised: " STRINGIZE(T) "\n");)

      m_events.Remove(iIndex);
      if(m_events.Count()==0) // No more events?
         static_cast<T *>(this)->Unadvised();
      return S_OK;
   }

   STDMETHODIMP EnumConnections(IEnumConnections **ppEnum) override
   {
      OwnedArray<CONNECTDATA> data(m_events.Count());
      for(unsigned int iEvent=0, iIndex=0;iEvent<m_events.Count();iEvent++,iIndex++)
      {
         while(m_events[iIndex]==nullptr)
            iIndex++;

         data[iEvent].pUnk=m_events[iIndex]; data[iEvent].pUnk->AddRef();
         data[iEvent].dwCookie=iIndex+1;
      }

      *ppEnum=new OM::EnumConnections(std::move(data)); (*ppEnum)->AddRef(); return S_OK;
   }

   HRESULT MultipleInvoke(DISPID dispid, DISPPARAMS &dp, VariantBool *pHandled=nullptr)
   {
      HRESULT hr=E_FAIL;

      for(unsigned int iEvent=0, iIndex=0;iEvent<m_events.Count();iEvent++,iIndex++)
      {
         while(m_events[iIndex]==nullptr)
            iIndex++;

         hr=m_events[iIndex]->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dp, nullptr, 0, 0);
         if(pHandled && *pHandled) break;
      }
      return hr;
   }

   HRESULT MultipleInvoke(DISPID dispid)
   {
      VariantBool fHandled(false); // Dummy that will never be set
      DISPPARAMS dp={0, 0, 0, 0};
      return MultipleInvoke(dispid, dp, &fHandled);
   }

   bool fAdvised() const { return m_events.Count()>0; }

private:
   CntPtrArray<TEvents> m_events;
};

/*
template<class T, class TEvents>
class EnumConnectionPoints : Unknown<IEnumConnectionPoints>
{
public:

   STDMETHODIMP QueryInterface(

   // EnumConnectionPoints
   STDMETHODIMP Next(ULONG cConnections, LPCONNECTIONPOINT *ppCP, ULONG *pcFetched)
   {
      return E_FAIL;
   }

   STDMETHODIMP Skip(ULONG cConnections)
   {
      return E_FAIL;
   }

   STDMETHODIMP Reset()
   {
      return E_FAIL;
   }

   STDMETHODIMP Clone(IEnumConnectionPoints *ppEnum)
   {
      return E_FAIL;
   }
};
*/

template<typename T, typename TEvents>
struct __declspec(novtable) ConnectionPointContainer : IConnectionPointContainer
{
   STDMETHODIMP EnumConnectionPoints(IEnumConnectionPoints **ppEnum) override
   {
      if(ppEnum==nullptr)
         return E_POINTER;

      Assert(false); // Untested!
      return E_FAIL;
//      *ppEnum=new OM::EnumConnectionPoints<T, TEvents>(); *ppEnum->AddRef(); return S_OK;
   }

   STDMETHODIMP FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP) override
   {
      if(ppCP==nullptr)
         return E_POINTER;

      if(riid!=__uuidof(TEvents))
         return CONNECT_E_NOCONNECTION;

      *ppCP=static_cast<IConnectionPoint *>(static_cast<T *>(this)); (*ppCP)->AddRef();
      return S_OK;
   }
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

// Counting Receiver, where calling AttachTo/Detach increments and decrements a reference
// count.  It only works if you're always attaching/detaching to the same single event sender
// This way the OM code will keep the event hooked as long as there is at least 1 reference
// As soon as the references drop to zero, the event is unhooked.
template<typename TBase, typename TEvent>
struct CntReceiverOf : private Events::Receiver
{
   void AttachTo(Events::SenderOf<TEvent> &sender)
   {
      if(m_iRefs++==0)
         Events::Receiver::AttachTo(sender);
   }

   void Detach()
   {
      if(--m_iRefs==0)
         Events::Receiver::Detach();
   }

private:
   void Receive(void *event) noexcept { static_cast<TBase *>(this)->On(*reinterpret_cast<TEvent *>(event)); }
   // NOTE: Cannot convert errors are because the class hooking this event didn't impliment an On(Event_Foo) handler

   unsigned m_iRefs{};
};

template<typename TBase, typename... TEvents>
struct CntReceiversOf : CntReceiverOf<TBase, TEvents>...
{
   template<typename TEvent> CntReceiverOf<TBase, TEvent> &Get() { return *this; }
   template<typename TEvent> void AttachTo(Events::SenderOf<TEvent> &sender) { Get<TEvent>().AttachTo(sender); }
   template<typename TEvent> void Detach() { Get<TEvent>().Detach(); }
};

void InitHooks();
void ClearHooks();

struct Hook
{
   operator bool() const { return m_pDisp.mp_disp!=nullptr; }

   void Clear() { m_pDisp.mp_disp=nullptr; m_pDisp.mp_dispex=nullptr;  }
   void Set(IDispatch *pNewDisp) { m_pDisp.mp_disp=pNewDisp; m_pDisp.mp_dispex=nullptr;  pNewDisp->QueryInterface(m_pDisp.mp_dispex.Address()); }

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
   DispatchNode m_pDisp; // The dispatch of the event hook
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
   auto &receiver=static_cast<CntReceiverOf<TBase, TEvent> &>(*pBase);

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
   auto &receiver=static_cast<CntReceiverOf<TBase, TEvent> &>(*pBase);

   if(hook)
      receiver.Detach();
   hook.Set(pDisp, var);

   if(pDisp)
      receiver.AttachTo(sender);
   return S_OK;
}

HRESULT LoadTypeInfoFromThisModule(REFIID riid, ITypeInfo **ppti);
OwnedBSTR ReadFileAsBSTR(ConstString fileName);

HRESULT SystemTimeToVariant(VARIANT &var, const Time::Time &time);

};
