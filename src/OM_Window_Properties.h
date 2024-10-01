//
// OM Window Properties
//
namespace OM
{

struct Window_Properties
:  Dispatch<OM::IWindow_Properties>,
   Events::ReceiversOf<Window_Properties, Events::Event_Deleted>
{
   Window_Properties(Window window, Events::SenderOf<Events::Event_Deleted> &deleted);

   STDMETHODIMP put_Title(BSTR bstr) override;
   STDMETHODIMP get_Title(BSTR *bstr) override;

   STDMETHODIMP get_HWND(__int3264 *hwnd) override;

   void On(const Events::Event_Deleted &event) { m_window=nullptr; }
   
private:
   Window m_window;
};

}
