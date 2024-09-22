//
// Text Parsing
//
struct AnsiParser
{
   AnsiParser(const Prop::Ansi &prop_ansi) : m_prop_ansi{prop_ansi} { Reset(); }

   void Reset() { m_state=State(); }
   bool Parse(Streams::Input &ts, Text::LineBuilder &line_builder);

   enum eBlink
   {
      Blink_None,
      Blink_Slow,
      Blink_Fast
   };

   struct State
   {
      unsigned m_foreground{7}; // Ansi Foreground Color
      Color m_color_foreground; // If m_foreground is set to '8' this is the color
      bool m_faint{};      // ANSI Faint attribute
      bool m_reverse{};    // ANSI Reverse attribute
      bool m_bold{};
      bool m_italic{};
      bool m_underline{};
      bool m_strikeout{};
      bool m_symbol{};
      eBlink m_blink{Blink_None};
   };

private:

   Color GetForegroundColor() const;

   const Prop::Ansi &m_prop_ansi;

   State m_state;
};
