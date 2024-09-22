struct UTF8ToCodePage : String
{
   UTF8ToCodePage(ConstString string, UINT code_page);

private:
   char mp_buffer[256]; // Big enough for 99% of the cases
   UniquePtr<char> mp_buffer_huge; // And for the 1% when it isn't
};

ConstString CP1252toUTF8(unsigned char c); // https://en.wikipedia.org/wiki/Windows-1252
ConstString CP437toUTF8(unsigned char c); // https://en.wikipedia.org/wiki/Code_page_437
