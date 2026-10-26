#pragma once
//
// MXP (MUD eXtension Protocol)
// https://www.zuggsoft.com/zmud/mxp.htm
//
// TextToLine owns one of these. It is fed through the same '<' and '&' hooks the
// Pueblo parser uses, plus ESC [ n z for the line modes. The connection takes the
// replies (VERSION, SUPPORT) and the images after each line.
//
namespace MXP
{

enum struct Mode
{
   Open,   // Only open tags (formatting) are parsed, the rest is shown as typed
   Secure, // All tags are parsed
   Locked, // No tags are parsed
};

struct Image
{
   OwnedString m_url;
   OwnedString m_name;
   int m_height{}; // In pixels, 0 means the banner's default
   int m_rows{};   // H=8c: the server left this many rows for the picture, starting with the tag's own
};

// Cell-sized pictures (H=8c) waiting for the box drawn round them to finish. The server
// reserved H rows starting with the tag's own; a picture drawn under any of them would cut
// the box, so it waits until they have all arrived and then for the box's bottom edge.
struct Figures
{
   void Add(Image &&image); // From the line the tag was on
   void Empty() { m_waiting.Empty(); }

   // Called for each line after the tag's. Pictures whose place has come are moved into
   // above (draw before this line) or below (draw after it).
   void Step(ConstString line, Collection<Image> &above, Collection<Image> &below);

private:
   Collection<Image> m_waiting; // m_rows counts the reserved rows still to come
};

struct Parser
{
   void Reset(); // New connection, or ESC[3z

   void BeginLine();
   void EndLine(Text::LineBuilder &line_builder); // Auto-closes open tags and reverts the mode

   // Called after ESC has been read. Returns false if this is not ESC [ n z, with the stream untouched
   bool ParseLineTag(Streams::Input &stream, Text::LineBuilder &line_builder);

   // Called after '<' has been read. Returns false if the tag should be shown as text, with the stream untouched
   bool ParseTag(Streams::Input &stream, Text::HTMLParser &html, Text::LineBuilder &line_builder);

   void NotATag() { m_temp_secure=false; }

   // True if this character is one of the blanks a cell-sized IMAGE's label stands in for, so it is not shown
   bool PayBlank(char c);
   bool ParsesEntities() const { return CurrentMode()!=Mode::Locked; }

   // What the connection does after the line
   Collection<OwnedString> m_replies; // Send these to the server as is
   Collection<Image> m_images;        // Show these under the line

private:

   enum struct Tag
   {
      Bold, Italic, Underline, Strikeout, Color, Send, Anchor,
   };

   struct Frame
   {
      Tag m_tag;
      bool m_opened_in_open_mode;

      // What the tag changed, so the close can put it back
      bool m_bold{}, m_italic{}, m_underline{}, m_strikeout{};
      Text::Records::Color m_color{};
      Text::Records::ColorBg m_color_bg{};
   };

   Mode CurrentMode() const { return m_line_mode; }
   bool IsSecure() const { return m_line_mode==Mode::Secure || m_temp_secure; }

   void Open(Tag tag, Text::LineBuilder &line_builder, Frame &frame);
   bool Close(Tag tag, Text::LineBuilder &line_builder, bool secure);
   void Restore(const Frame &frame, Text::LineBuilder &line_builder);
   void CloseOpenModeTags(Text::LineBuilder &line_builder);

   bool HandleSend(Streams::Input &stream, Text::HTMLParser &html, Text::LineBuilder &line_builder);
   bool HandleAnchor(Text::HTMLParser &html, Text::LineBuilder &line_builder);
   bool HandleColor(Text::HTMLParser &html, Text::LineBuilder &line_builder, Frame &frame);
   bool HandleImage(Text::HTMLParser &html, Text::LineBuilder &line_builder, unsigned blanks_after);
   static unsigned CountBlanks(ConstString text);
   static bool ParseExtent(ConstString text, unsigned &value, bool &in_cells);
   void HandleVersion();
   void HandleSupport();

   Mode m_default_mode{Mode::Open}; // Set by ESC[5z..7z, restored at every line end
   Mode m_line_mode{Mode::Open};    // Set by ESC[0z..2z, lasts until the line end
   bool m_temp_secure{};            // ESC[4z, spent by the very next tag
   unsigned m_owed_blanks{};        // Blanks after a cell-sized IMAGE that its label already took the place of

   Collection<Frame> m_stack;
};

// The URL an IMAGE tag names, or empty if it is not http(s)
OwnedString ResolveImageURL(ConstString fname, ConstString url, ConstString type);

}
