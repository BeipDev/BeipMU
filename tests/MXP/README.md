# MXP parser tests

`src/MXP.cpp` compiled on its own against stand-ins for the BeipMU types it uses
(`Main.h` here), driven through a copy of the MXP arms of `TextToLine::Parse`.
It checks the parser's rules, not the real `Text::LineBuilder` or `HTMLParser`.

```
cd tests/MXP
cp ../../src/MXP.cpp ../../src/MXP.h .
g++ -std=c++20 test.cpp MXP.cpp -o mxp_test && ./mxp_test
```

The copy matters: `MXP.cpp` includes `"Main.h"`, and a quoted include looks beside the
source file first, so compiling it in `src/` picks up the real one.
