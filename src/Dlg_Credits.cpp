#include "Main.h"

void CreateDialog_Credits(Window wnd)
{
   ConstString credits=
      "Bennet - Programming\n" \
      "Bryan 'Sennard' Travis - Design, Testing & Buttons\n" \
      "Jamie 'Squirrelly' Wilmoth - Script Design, Scripts, and Web Hosting\n" \
      "Krishva - Artwork\n" \
      "Cuprohastes - Store art & wild ideas\n" \
      "Shiveneve - Muck Integration\n" \
      "Foxbird - Feature Rejection Engineer\n";

   MessageBox(wnd, credits, "Credits:", MB_OK|MB_ICONINFORMATION);
}
