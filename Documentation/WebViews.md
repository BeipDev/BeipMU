# WebViews

WebViews are web browser windows that are integrated into the client. They can dock in a tab, and scripts in the webview can send and receive client events.

## Examples

A little bit of everything:
https://beipdev.github.io/BeipMU/WebViews/samples.html

## API

Inside the webview HTML, JavaScript can access the host through this object:

`window.chrome.webview.hostObjects.client`

As the webview runs in a separate process, all functions are asynchronous by default. To avoid needing to use 'await' on everything, do this first:
`window.chrome.webview.hostObjects.options.defaultSyncProxy=true`

`CloseWindow()` Close the WebView window

`Send(string);` Send the given string as text over the connection

`SetOnSend(function);` Calls the callback when text is sent over the connection.

Call back parameters: `([text sent])`

`Display(string);` Display the given string (

`SetOnDisplay(int id, function, string regex);`

`ClearOnDisplay(int id);` 

Callback parameters `(line, regex matches);`

`SetOnDisplayCapture(int id, function capture, function capture_changed, string regex_begin, string regex_end);`

Callbacks: `capture(id, line) capture_changed(id, line, bool starting)` starting = true on regex_begin, false on regex_end

`bool ClearOnDisplayCapture(int id);` Remove the capture callback hook with the given id, returns true if successful

`SetOnGMCP(function callback, string package_prefix);`
Callback: `callback(string package, string json)`

`Receive(string received)` Act as though the given data string was received from the connection

`SetOnReceive(function callback);` Calls the callback when data is received from the connection

Callback: `callback([text received]);`
