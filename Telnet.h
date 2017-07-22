//
// Telnet Protocol
//
struct LineParser
{
   interface INotify
   {
      virtual void OnTelnet(ConstString lstr)=0; // Telnet code received, lstr is the response
      virtual void OnLine(ConstString lstr)=0;
      virtual void OnPrompt(ConstString lstr)=0;
   };

   LineParser(INotify &notify);
   void Reset();

   void Parse(Array<const char> buffer); // Calls OnLine through INotify for every line received
   bool HasPartial() const { return m_buffer.Count()!=0; }
   ConstString GetPartial() const { return m_buffer.Count() ? ConstString(&m_buffer[0], m_buffer.Count()) : ConstString(); }

private:
   INotify &m_notify;

   enum class State
   {
      Normal,
      IAC,
      Dont,
      Do,
      Wont,
      Will,
      SB,
      SB_TTYPE,
      SB_WaitForIAC, // Should be done, eat until we see IAC
      SB_IAC,
      SE,
   };

   State m_state{State::Normal};
   Collection<char> m_buffer;
};
