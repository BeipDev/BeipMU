# WebViews

WebViews are web browser windows that are integrated into the client. They can dock in a tab, and scripts in the webview can send and receive client events.

## Examples

A little bit of everything:
https://beipdev.github.io/BeipMU/WebViews/samples.html

## API

Inside the webview HTML, JavaScript can access the host through this object:

`window.chrome.webview.hostObjects.client` Note that this part: `window.chrome.webview.hostObjects.` is just where WebViews give access to host objects (host = BeipMU in this case). It's long because they want to avoid colliding with any user scripts.

As the webview runs in a separate process, all functions are asynchronous by default. To avoid needing to use 'await' on everything, do this first:
`window.chrome.webview.hostObjects.options.defaultSyncProxy=true`

## client

This is the `client` part of `window.chrome.webview.hostObjects.client`

Methods:

<details><summary>CloseWindow</summary>

### `CloseWindow()`
Close the WebView window. Useful if your webview is a popup to choose an item and you want it to close after doing your selection.
</details>


<details><summary>Send</summary>
  
### `Send(string text, bool process_aliases=false)`
Send the given string as text over the connection.

```js
window.chrome.webview.hostObjects.client.send("page friend=\"Booo!");
```

#### Parameters
* `text` The text to send
* `process_aliases` Set to true to have the user's aliases processed when sending the text
</details>

<details><summary>Receive</summary>

### `Receive(string received)`
Act as though the given data string was received from the connection
```js
window.chrome.webview.hostObjects.client.Receive("Muhaha, this didn't really happen.");
```
</details>

<details><summary>Display</summary>

### `Display(string)`
Display the given string
```js
window.chrome.webview.hostObjects.client.Display("Something happened.");
```
</details>

<details><summary>SetOnSend</summary>

### `SetOnSend(function callback)`
Calls the callback when text is sent over the connection.

```js
function MyCallback(text)
{
  window.chrome.webview.hostObjects.client.Display("The user just sent: "+text);
}

window.chrome.webview.hostObjects.client.SetOnSend(MyCallback);
```
</details>


<details><summary>SetOnDisplay</summary>

### `SetOnDisplay(int id, function callback, string regex)`
Watch text lines about to be displayed and call `callback` when `regex` results in a match. Basically a script defined trigger.

```js
function OnDisplay(line, matches)
{
  // line is a text line object
  // matches is an array of the regex matches begin/end pairs. First match is matches(0) to matches(1)
}

window.chrome.webview.hostObjects.client.SetOnDisplay(1, OnDisplay, "^\\d+");
```

<B>Remember</B> to escape \'s in regex parameters, as it's a Javascript string literal.
</details>


<details><summary>ClearOnDisplay</summary>
  
### `ClearOnDisplay(int id)` 

Callback parameters `(line, regex matches)`
</details>

<details><summary>SetOnDisplayCapture</summary>

### `SetOnDisplayCapture(int id, function capture, function capture_changed, string regex_begin, string regex_end);`
Spawn capture hook, where your callbacks will be called for the start of capture, the lines during capture, and when capture ends. This saves you the trouble of scanning every line being displayed.

The 'id' is mainly useful if you set multiple display captures, as a way to tell them apart and to cancel them. If you only have one capture that you never cancel you can set this to something simple like `0`.

```js
function OnCapture(id, line)
{
  // id is the 'id' you passed to SetOnDisplayCapture
  // line is a text line object

  let text=line.string;
  let length=line.length;
}

function OnCaptureChanged(id, line, starting)
{
  // id is the 'id' you passed to SetOnDisplayCapture
  // line is a text line object
  // starting is a bool. 'true' when capture is beginning, 'false' when it is ending
}

window.chrome.webview.hostObjects.client.SetOnDisplayCapture(1, OnCapture, OnCaptureChanged, "^Players online:", "^\\d+ players");
```

<B>Remember</B> to escape \'s in regex parameters, as it's a Javascript string literal.
</details>

<details><summary>ClearOnDisplayCapture</summary>

### `bool ClearOnDisplayCapture(int id)`
Stop watching for the capture hook with the given id.
```js
window.chrome.webview.hostObjects.client.ClearOnDisplayCapture(1);
```

</details>

<details><summary>SetOnGMCP</summary>

### `SetOnGMCP(function callback, string package_prefix)`
Watch for specific GMCP messages.

```js
function OnGMCP(package, json)
{
  let obj=JSON.parse(json);

  // package will be the entire package name, so in our case could be like cool_package.player
  let action=package.substring(package.indexOf('.')+1);
  if(action==="player")
  {
    // Do player stuff
  }
  else if(action==="server")
  {
    // Do server stuff
  }
}

window.chrome.webview.hostObjects.client.SetOnGMCP(OnGMCP, "cool_package");
```
</details>



<details><summary>SetOnReceive</summary>

### `SetOnReceive(function callback)`
Calls the callback when data is received from the connection. Note that this is the raw data for a line of text, no ansi parsing or triggers have been run at this point. Use `SetOnDisplay` if you want a callback for what the user is going to actually see.

```js
function OnReceive(text)
{
  // Do stuff with the raw text
}

window.chrome.webview.hostObjects.client.SetOnReceive(OnReceive);
```
</details>


