## A Dockable Filter Window

    // Create a Text output window
    var wndText=app.NewWindow_Text();

    // Make it into a dockable window and dock it on the right side of the current window
    // 0=left, 1=top, 2=right, 3=bottom (or remove the .Dock(...) part and dock it manually)
    window.MakeDocking(wndText.properties.hwnd).Dock(2 /*Right*/);

    // Define the function to set in the trigger
    function ShowText(window, text) { wndText.Add(text); }

In any trigger that you want to route text to this window, go to the Script tab and put 'ShowText' as the function (be sure to enable it too)
