# WebViews

WebViews are web browser windows that are integrated into the client. They can dock in a tab, and scripts in the webview can send and receive client events.

## How to use

The `/webview` command line command is the simplest:
https://github.com/BeipDev/BeipMU/blob/master/Documentation/CommandLine.md#webview

`/webview url="https://beipdev.github.io/BeipMU/WebViews/samples.html"`

You can also right click hyperlinks on a page to open them as a webview vs the default browser. Useful if you want to dock it inside your client.

The mu* server can also send a GMCP `webview.open` message. [See here](#gmcp)

## Examples

Note that these examples will not function in the browser, but you'll still get to see what they look like. To have them be functional, use the `/webview` command with the `url` set to one of them.

A little bit of everything:
https://beipdev.github.io/BeipMU/WebViews/samples.html

Navigation UI (buttons that send stuff)
https://beipdev.github.io/BeipMU/WebViews/macros.html

A GMCP handler example
https://beipdev.github.io/BeipMU/WebViews/gmcp.html

A GMCP graphical tilemap package
https://beipdev.github.io/BeipMU/WebViews/tilemap.html

## API

Inside the webview HTML, JavaScript can access the host through this object:

`window.chrome.webview.hostObjects.client` Note that this part: `window.chrome.webview.hostObjects.` is just where WebViews give access to host objects (host = BeipMU in this case). It's long because they want to avoid colliding with any user scripts.

As the webview runs in a separate process, all functions are asynchronous by default. To avoid needing to use 'await' on everything, do this first (it's done in the example page above):
```js
window.chrome.webview.hostObjects.options.defaultSyncProxy=true
```

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

<details><summary>SendGMCP</summary>

### `SendGMCP(string package, string json)`
Send the given package & json as a GMCP telnet message. This is a convenience function that joins the parameters together plus the telnet codes to make a valid GMCP message. It also gets captured by the GMCP debugger.

```js
let gmcp=["BeipTest1 1", "BeipTest2 1"];
window.chrome.webview.hostObjects.client.SendGMCP("Core.Supports.Add "+JSON.stringify(gmcp));
```

#### Parameters
* `package` The GMCP package name
* `json` A string of the JSON

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

### `SetOnDisplay(int id, function callback, string regex, bool gag=false)`
Watch text lines about to be displayed and call `callback` when `regex` results in a match. Basically a script defined trigger. If gag is true, the line will be gagged in the client display.

```js
function OnDisplay(id, line)
{
  // id is the id passed to SetOnDiplsay
  // line is a TextWindowLine object (see link below)
}

window.chrome.webview.hostObjects.client.SetOnDisplay(1, OnDisplay, "^\\d+");
```
[TextWindowLine interface](ScriptingAPI.md#textwindowline)

<B>Remember</B> to escape \\'s in regex parameters, as it's a Javascript string literal.
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
  // line is a TextWindowLine object (see link below)

  let text=line.string;
  let length=line.length;
}

function OnCaptureChanged(id, line, starting)
{
  // id is the 'id' you passed to SetOnDisplayCapture
  // line is a TextWindowLine object (see link below)
  // starting is a bool. 'true' when capture is beginning, 'false' when it is ending
}

window.chrome.webview.hostObjects.client.SetOnDisplayCapture(1, OnCapture, OnCaptureChanged, "^Players online:", "^\\d+ players");
```
[TextWindowLine interface](ScriptingAPI.md#textwindowline)

<B>Remember</B> to escape \\'s in regex parameters, as it's a Javascript string literal.
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

## GMCP

If enabled, a server can send a `webview.open` GMCP message to have the client open a webview.

Can be as simple as just opening a URL:

```
webview.open { "url":"https://our_cool_website.html" }
```

Or having it auto dock, plus providing http-request-header to auto-login to your website. Useful for doing online player editing through the website with a simple in-game command!

```
webview.open { "id":"Character editor", "dock":"right", "url":"value", "http-request-headers":{ "name1":"value1", "name2":"value2" } }
```

The attributes:
* id - A way to refer to a webview. If a later `webview.open` comes in, it will replace the original one with the same id
* dock - (optional) For a new webview, will dock it on creation.
* url - (optional) The URL to open
* source - (optional) If URL isn't specified, this can directly provide the HTML source of the webview page
* http-request-headers - (optional) A list of name/value pairs to be added to the http request.

```
webview.close { "id":"Character editor" }
```

The attributes:
* id - An id of an existing webview to close. If there is no open webview with this id, it does nothing
  
