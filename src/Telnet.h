//
// Telnet Protocol
//
struct TelnetParser
{
   interface INotify
   {
      virtual void OnTelnet(ConstString string)=0; // Telnet code received, string is the response
      virtual void OnLine(ConstString string)=0;
      virtual void OnPrompt(ConstString string)=0;
      virtual void OnEncoding(Prop::Server::Encoding encoding)=0;
      virtual void OnDoNAWS()=0;
      virtual void OnGMCP(ConstString string)=0;
      virtual Prop::Server *GetServer()=0;
   };

   TelnetParser(INotify &notify);
   void Reset();

   void Parse(Array<const char> buffer); // Calls OnLine through INotify for every line received
   bool HasPartial() const { return m_buffer.Count()!=0; }
   ConstString GetPartial() const { return m_buffer.Count() ? ConstString(&m_buffer[0], m_buffer.Count()) : ConstString(); }

   // If there's a partial buffer, send it as a prompt
   void CheckPrompt() { if(!HasPartial()) return; m_notify.OnPrompt(GetPartial()); m_buffer.Empty(); }

   void SendNAWS(uint16_2 dims);
   bool m_do_naws{};

private:
   INotify &m_notify;

   void HandleCharset();

   enum struct State
   {
      Normal,
      IAC,
      Dont,
      Do,
      Wont,
      Will,
      SB,
      SB_GMCP,
      SB_GMCP_Request_IAC,
      SB_CHARSET,
      SB_CHARSET_Request_List,
      SB_CHARSET_Request_IAC,
      SB_TTYPE,
      SB_WaitForIAC, // Should be done, eat until we see IAC
      SB_IAC,
      SE,
   };

   State m_state{State::Normal};
   Collection<char> m_buffer;
   unsigned m_sb_start; // Index in buffer where subnegotation starts
};
