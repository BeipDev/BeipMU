- [Interface List](#interface-list)
  * [Interface Descriptions](#interface-descriptions)
  * [Windows OLE Types](#windows-ole-types)
  * [App](#app)
  * [ArrayUInt](#arrayuint)
  * [Character](#character)
  * [Characters](#characters)
  * [Connection](#connection)
  * [FindString](#findstring)
  * [Puppet](#puppet)
  * [Puppets](#puppets)
  * [Socket](#socket)
  * [SocketServer](#socketserver)
  * [TextWindowLine](#textwindowline)
  * [Timer](#timer)
  * [Trigger](#trigger)
  * [Triggers](#triggers)
  * [Window_Control_Edit](#window_control_edit)
  * [Window_Events](#window_events)
  * [Window_FixedText](#window_fixedtext)
  * [Window_Graphics](#window_graphics)
  * [Window_Main](#window_main)
  * [Window_Properties](#window_properties)
  * [Window_Text](#window_text)
  * [Windows](#windows)
  * [World](#world)
  * [Worlds](#worlds)
- [Advanced](#advanced)
  * [Using BeipMU from Visual Basic](#using-beipmu-from-visual-basic)

<small><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></small>

# Interface List

This document assumes a familiarity with VB, JavaScript, or C/C++.  The interfaces are described in a JavaScript/C++ fashion which should be easier to understand in the long run. 

## Interface Descriptions 

All interface methods are in the form of:

    Return_Type Function_Name(Parameter_type Parameter_Name, ...)

The return type can be any of the language's built in types (like int, or long), and it can also be an object (such as a Connection). 
For the parameters, some parameters may say 'parameter_type name=<default value>'.  This just means that if you leave out this parameter (which is easy as they are always the last parameters in the function) the <default value> will be passed in automatically.  Parameters might also have an [optional] tag before them when you can omit the parameter yet there is no default value.

Methods with the [default] tag before them mean that they can be accessed on the object without using the function name.  For example: app.windows(1) is the same as app.windows.item(1).

Methods in the form of:

    <type> Name

Are not functions, but properties.  Properties which you can overwrite with new values have a [read/write] tag before their type.  For example, the 'int Version' property in IApp is read only, you cannot change BeipMU's version number. 

A sample of using a property in JScript: (displays BeipMU's build number in the output window)

    /@window.output.write(app.BuildNumber);

For some properties it looks redundant:

    Worlds Worlds

But this is just a property named 'Worlds' that returns an object of type 'Worlds'.

Some callbacks will pass 'this' as their first parameter on the callback function, and when setting the callback there is no 'variant' parameter.  The 'this' object is the interface the hook was set on.  For example, in a Socket interface, the 'this' is the Socket you set the callback on.  This allows easy access to the socket object, and to the userdata property on the object. 

## Windows OLE Types 

Some of the parameter types are from Windows OLE. The two ones used here are IDispatch and VARIANT. Think of IDispatch as a pointer to an object (usually some script code in these interfaces), and VARIANT as a pointer to anything (number, string, object, etc.). Their main uses are for events where there is some code that needs to be run on an event, and some special userdata that is passed to the event code. So the code is the IDispatch and the user data is the VARIANT, this allows for maximum flexibility. I left the types in this help file as what they really and gave this explanation rather than make up fake types since if anyone knows the real types this will already be familiar to them.

For example, to setup a timer hook in JavaScript: 

    function foo(userdata)
    {
      window.output.write("UserData:"+userdata);
    }
    app.createtimeout(1000, foo, 1234)

This creates a timeout timer (one shot), that fires after 1 second, calling foo, and passing foo the '1234'. 'foo' is the IDispatch, and '1234' is the VARIANT in the parameter list. Experiment with what works, since script should never crash, and if it does, let us know what you did as scripting in Beip is still pretty new. 
DATE/Undefined - Some functions can return a DATE type and also return an Undefined type.  They are really returning a VARIANT type, but the only types in the variant are either a DATE type (VT_DATE) or the empty/undefined (VT_EMPTY) type. 
An example in JavaScript to tell the difference of the Date/Undefined by comparison to null: 

    if(app.worlds(0).characters(0).lastUsed==null)
      window.output.write("No last used time");
    
    // typeof also can be used (but is probably less efficient)
    
    if(typeof(app.worlds(0).characters(0).lastUsed)=="undefined")
      window.output.write("No last used time");

#Available Interfaces

## App

    unsigned int BuildNumber

The build number

    DATE BuildDate 

The build date 

    int Version 

Current version

    ConfigPath String

The path of the config file

    Worlds Worlds

The global list of worlds 

    Windows Windows

The global list of main windows 

    Triggers Triggers 

The global list of triggers 

    Trigger NewTrigger() 

Creates a new trigger object 

    Window_FixedText NewWindow_FixedText(int iWidth=80, int iHeight=25) 

Creates a new IWindow_FixedText window, the iWidth and iHeight are in characters. The default values will create a window the same size as a standard console window.

    Window_Graphics NewWindow_Graphics(int iWidth, int iHeight)

Creates a new IWindow_Graphics window, the iWidth and iHeight are in pixels.

    Window_Text NewWindow_Text()

Creates a floating IWindow_Text, the same window type as the output/history windows.

    Window_Main NewWindow()

Create a new Main Window

    SetOnNewWindow(IDispatch pDisp, VARIANT var)

Hook, called every time a new window is opened.  the window object during the hook is the new window

Prototype is: OnNewWindowEvent(var)

    Timer CreateInterval(int iTimeOut, IDispatch pDisp, [optional] VARIANT var)

Create a repeating timer

    Timer CreateTimeout(int iTimeOut, IDispatch pDisp, [optional] VARIANT var)

Create a one shot timer.

All timer intervals are in milliseconds. When a timer goes off, it calls the pDisp function with the 'var' object. To stop the timer, return 'true' from this hook function.

Prototype is: OnTimerEvent(var)

    Socket New_Socket()

Create a new unconnected socket

    SocketServer New_SocketServer(unsigned int port, IDispatch *pDisp, [optional] VARIANT var)

Prototype is: OnConnectionEvent(Socket newConnection, VARIANT var)

    ForwardDNSLookup(String name, IDispatch *pDisp, [optional] VARIANT var)

Performs an asynchronous forward DNS lookup, given the host name as a string. Calls pDisp with the IP address as a string when done.  If the lookup fails, calls OnLookup with an empty string.

Prototype is: OnLookup(String name, VARIANT var)

ReverseDNSLookup(String IP, IDispatch *pDisp, [optional] VARIANT var)

Performs an asynchronous reverse DNS lookup, given the IP address as a string. Calls pDisp with the name when done.  If the lookup fails, calls OnLookup with an empty string.

Prototype is: OnLookup(String name, VARIANT var)

    bool IsAddress(String IP)

Returns TRUE if the given string is a valid IP address. Useful for doing the proper DNS lookup.

    PlaySound(String filename, [optional] float volume=1.0)

Plays the sound 'filename' at volume level volume.

    StopSounds()

Stops all playing sounds

    OutputDebugText(String text)

Appends a string to the debug window.

    OutputDebugHTML(String text)

Same as OutputDebugText, except that HTML codes in the bstr are parsed. 

## ArrayUInt

    [default] unsigned Item(VARIANT var) 

Returns unsigned integer 'var'. var is typically an integer index

Note: instead of array.item(5), array(5) can also be used since it's a default method

    [read] unsigned int Count 

Used by the trigger script callback to pass an array of integers

## Character

    [read/write] String shortcut

    [read/write] String Name

    [read/write] String Connect

    [read/write] String Info

    [read/write] String LogFileName

    DATE/Undefined LastUsed

The date & time the character was last used. Will return 'undefined' (VT_EMPTY) if there was no last used date.

    DATE/Undefined TimeCreated

The date & time the character was created on. Will return 'undefined' (VT_EMPTY) if there is no recorded creation time.

    [read] Puppets Puppets

## Characters

    [default] Character Item(VARIANT var)

Returns character 'var'. var can be be a number.

Note: instead of characters.item(5), characters(5) can also be used since it's a default

    [read] unsigned int Count

The number of characters in this world 

## Connection

    Send(String string) 

Sends text to the server, first going through the OnSend hook. What the user types first goes here before going to transmit 

    SetOnSend(IDispatch *pDisp, [optional] VARIANT var)

The OnSend hook, called with these parameters: (bstr Text, VARIANT var) 

    Transmit(String string)

Send the text directly to the server, no further processing is done before sending. 

    Receive(String string)

Act as though the given string was received from the connection. When calling this remember to have a \n at the end of the text, otherwise it will be treated as a partial blob of data and won't be displayed until the end of line is seen! 

    SetOnReceive(IDispatch *pDisp, [optional] VARIANT var)

The OnReceive hook, Prototype: bool (String text, VARIANT var) 

    Display(String string)

Converts the bstr into an ITextWindowLine, handling triggers, ansi, and URLs. This then goes through the OnDisplay hook. 

     SetOnDisplay(IDispatch *pDisp, [optional] VARIANT var)

bool OnDisplayEvent(TextWindowLine, var)

    SetOnConnect(IDispatch *pDisp, [optional] VARIANT var)

OnConnect hook, called with (VARIANT var) when this connection connects

    SetOnDisconnect(IDispatch *pDisp, [optional] VARIANT var)

OnDisconnect hook, called with (VARIANT var) when this connection disconnects 

    bool IsConnected()

Returns true if the connection is currently connected 

    bool Reconnect() 

Will reconnect to the last server/character/puppet combination. Returns false if no reconnect is possible (there was no last server) 

    bool IsLogging() 

Returns true if the connection is currently logging 

    [read] Log Log 

Will fail if there is no current log (check IsLogging() before calling) 

    World World 

Currently connected world (or NULL if none) 

    Character Character 

Currently connected character (or NULL if none) 

    Puppet Puppet 

Currently connected puppet (or NULL if none) 

    [read] Window_Main Window_Main 

The Window_Main object associated with us 
 
The flow of outgoing data is as follows:

User types data -> Send() -> OnSend() -> Transmit()

For incoming data:

Server -> Receive() -> OnReceive() -> Display() -> OnDisplay()

The On<> functions are hooks that can intercept the flow and do anything they want with it.  To stop further processing in a hook, simply return 'true'

## FindString

    [read/write] String MatchText 

The string that we are searching for 

    [read/write] bool RegularExpression 

If MatchText defines a regular expression or just a string. When this flag is on, only MatchCase has any effect 

    [read/write] bool MatchCase 

    [read/write] bool StartsWith 

    [read/write] bool EndsWith 

    [read/write] bool WholeWord

## Puppet

    [read/write] String Name 

    [read/write] String Info 

    [read/write] String ReceivePrefix 

    [read/write] String SendPrefix 

    [read/write] String LogFileName 

    [read/write] bool AutoConnect 

    [read/write] bool ConnectWithPlayer 

    [read] Triggers Triggers 

## Puppets

    [default] Puppet Item(VARIANT var) 

Returns puppet 'var'. var can be be a number or a name

Note: instead of puppets.item(5), puppets(5) can also be used since it's a default 

    [read] unsigned int Count 

The number of puppets 

## Socket

    Connect(String host, unsigned int port) 

Connects the socket to the given hostname or IP address. If a hostname is given, an asynchronous DNS lookup is silently performed. 

    Disconnect() 

    bool IsConnected() 

    Send(String text) 

    String Receive() 

Returns the already received data as a BSTR so it can be treated as text easily 

    VARIANT ReceiveBytes() 

Returns the already received data as a BYTE array for binary processing 

    SetOnReceive(IDispatch *pDisp) 

Prototype: OnReceiveHook(this) - When this callback function is called, simply call Recieve() or ReceiveBytes() on the socket to get the received data how you want it. 

    SetOnConnect(IDispatch *pDisp) 

Prototype: OnConnect(this) 

    SetOnDisconnect(IDispatch *pDisp) 

Prototype: OnDisconnect(this) 

    [read/write] VARIANT UserData 

User data storage for this object (accessible through this.userdata on a callback) 

## SocketServer

    Shutdown()

Stops the server.  No more connections can be made and the port is no longer listening. 

A example server in JavaScript that simply welcomes new connections then terminates them:

    var server=app.new_socketserver(4098);
    function OnConnection(socket) { socket.send("Welcome!\r\n"); socket.Disconnect(); }

## TextWindowLine

    int length 

Length of the text in characters 

    String string

Returns the string of text 

    Delete(unsigned int start, unsigned int end)

Delete objects from index start through end 

    Insert(unsigned int position, ITextWindowLine) 

Inserts another ITextWindowLine into this line 

    Color(unsigned int start, unsigned int end, long lColor=0) 

Change the color of the character range specified to lColor 

    BgColor(unsigned int start, unsigned int end, lColor=0) 

Change the background color of the character range specified to lColor 

    Bold(unsigned int start, unsigned int end, BOOL fSet=true) 

Changes the bold attribute of the character range specified to fSet 

    Italic(unsigned int start, unsigned int end, BOOL fSet=true)

Changes the italic attribute of the character range specified to fSet 

    Underline(unsigned int start, unsigned int end, BOOL fSet=true)

Changes the underline attribute of the character range specified to fSet 

    Strikeout(unsigned int start, unsigned int end, BOOL fSet=true) 

Changes the strikeout attribute of the character range specified to fSet 

    Flash(unsigned int start, unsigned int end, BOOL fSet=true) 

Changes the flash attribute of the character range specified to fSet 

    FlashMode(unsigned int start, unsigned int end, int iMode=0) 

Changes the flash mode attribute of the character range specified to iMode 

0 = normal - text color matches background color during a blink 

1 = inverse - text color and background colors are switched during a blink 

    Blink(unsigned int start, unsigned int end, unsigned int iBits, unsigned int mask) 

Sets the low level blink settings for the character range specified. 

iBits = Number of bits to cycle through 

iMask = Bit mask for when the text is blinked 
 
## Timer

    [read/write] VARIANT UserData 

User data passed to the timer callback function 

    void Kill() 

Stops the timer

## Trigger

See the help topic on Triggers for an explanation on each of these flags. 

    FindString FindString 
    [read/write] bool Disabled 
    [read/write] bool StopProcessing 
    [read/write] bool OncePerLine 
    [read/write] bool AwayPresent 
    [read/write] bool AwayPresentOnce 
    [read/write] bool Away 
    Triggers Triggers 

The list of triggers that are nested under this trigger 

## Triggers

    [default] ITrigger Item(VARIANT var) 

Returns character 'var'. var can be be a number. 

Note: instead of triggers.item(5), triggers(5) can also be used since it's a default 

    [read] unsigned int Count 

The number of characters in this world 

    Trigger AddCopy(Trigger trigger)

Adds a copy of the passed in trigger, and returns a reference to the copy. 

## Window_Control_Edit

    SetSel(int start, int end)

Makes a selection from character 'start' to character 'end' 

    int GetSelStart() 

Returns the start of the current selection

    int GetSelEnd()

Returns the end of the current selection

    int Length

The length in characters of the contents of this window 

    Set(String string)

Replaces the current window contents with the text in 'bstr' 

    String Get()

Returns the current contents of the window as a string 

--------------------------------------------------------------------------------

When there is no selection, GetSelStart() and GetSelEnd() will both return the current position of the cursor.

SetSel( pos, pos ) can be used to put the cursor at position 'pos'.


## Window_Events

Provides hookable events for a window 

    SetOnClose(IDispatch *pDisp, VARIANT data)

Prototype: OnClose(data)

    SetOnMouseMove(IDispatch *pDisp, VARIANT data)

Prototype: OnMouseMove(x, y, data)

    SetOnKey(IDispatch *pDisp, VARIANT data)

Prototype: OnKey(key, data) 

--------------------------------------------------------------------------------
Run this script below to see how the OnMouseMove function works. 

    var t=app.newwindow_fixedtext(40,10)
    
    function onmove(x,y)
    {
      t.cursorx=0; t.cursory=0; t.write("X:"+x+" Y:"+y+"  ");
    }
    
    t.events.setonmousemove(onmove)
    t.cursory=3; t.write("Move the mouse in this window");


## Window_FixedText

A terminal type window using a fixed text font.  Perfect for output consoles/debugging, etc. 

    IWindow_Events Events

    IWindow_Properties Properties

    [read/write] CursorX

The X position of the text cursor 

    [read/write] CursorY

The Y position of the text cursor

    Clear()

Erases all text and puts the cursor at position (0,0)

    Write(String text)

Writes 'text' into the window, processing line feeds and carriage returns. Moves the cursor to the end of the written text. 

## Window_Graphics

All units are in pixels 

    Window_Events Events

    Window_Properties Properties

    int width() 

The width of the drawing area 

    int height() 

The height of the drawing area 

    Clear(long lColor=0)

Clears the drawing area, if a color is not given it is filled with black (color value 0)

    SetPixel(int x, int y, long lColor)

Sets the pixel at (x,y) to color lColor

    long GetPixel(int x, int y)

Returns the color of the pixel at (x,y)

    SetPen(long lColor, int width=0)

Changes the pen color and width used to draw lines. A width of 0 is always one pixel wide

    MoveTo(int x, int y)

Moves the pen to position (x,y) 

    LineTo(int x, int y)

Draws a line from the current pen position to (x,y). Updates the pen position 

    Text(int x, int y, String text) 

Writes the string 'text' at position (x,y) 

--------------------------------------------------------------------------------
Run this script to see how an OnMouseMove hook can be used to draw into the graphics window with the mouse. 

    var g=app.newwindow_graphics(256,256)
    
    function onmove(x,y)
    {
      g.setpixel(x,y,0x804080);
    }

    g.events.setonmousemove(onmove)
    g.text(10,10,"Move the mouse!");


## Window_Main

The current Window_Main object is accessible through the 'window' object.

So for example, to output text into the current main window's output window, just type: window.output.write("Testing")


    Window_Text Output 

The output window 

    Window_Text History 

The history window 

    Window_Control_Edit Input 

The input window 

    Connection Connection 

    [read/write] VARIANT UserData 

User data for each window (used in MCP implementation) 

    Close() 

Closes the window the same as if the user closed it. 

    SetOnCommand(IDispatch pDisp, [optional] VARIANT var) 

When the user types a / command, this is called. 

Prototype: OnCommandEvent(String command, String parameters, VARIANT var)

    SetOnActivate(IDispatch pDisp, [optional] VARIANT var)

Called when the window is activated/deactivated

Prototype: OnActivateEvent(VARIANT var, bool fActivated)

    SetOnClose(IDispatch pDisp, [optional] VARIANT var)

Called when the window gets closed 

Prototype: OnCloseEvent(VARIANT var)

    Run(String string)

Execute the script contained in the passed in string 

    RunFile(String string)

Execute the script in a file specified by string

    CreateDialogConnect()

Opens up the Connect Dialog in this window 

    [read/write] String TitlePrefix 

A string that is prepended to the title of the window 

    [read] String Title

The title of the window (not including the prefix) 

## Window_Properties

Basic properties that can be modified on some windows.

    [read/write] String Title

The window's titlebar text

    [read] HWND HWND

Returns the HWND of this window (Win32 internal) 

## Window_Text

    Window_Properties Properties 

    [read] bool Paused 

Returns true if this text window is paused. [/write] might be added at a future date if it gets requested. 

    TextWindowLine create(String string)

Creates an TextWindowLine object initialized to the given string 

    TextWindowLine createHTML(String string) 

Creates an TextWindowLine object initialized to the given string, and parsing HTML tags

    Add(TextWindowLine lineNew)

Adds a copy of the ITextWindowLine to the output window 

    Write(String string)

Writes the string directly to the output window 

    WriteHTML(String string)

Writes the string and processes any recognized html tags 

    SetOnPause(IDispatch pDisp) 

Called when the window gets Paused/Unpaused 

Prototype: OnPauseEvent(bool paused) 

## Windows

    [default] Window_Main Item(VARIANT var) 

Returns main window 'var'. var can be be a number. 

Note: instead of windows.item(5), windows(5) can also be used since it's a default 

    [read] unsigned int Count 

The number of main windows 

## World

    [read/write] String name 
    [read/write] String info 
    [read/write] String host

The host:port, for example www.example.com:8888

    Characters characters 

## Worlds

    [default] World Item(VARIANT var) 

Returns world 'var'. var can be be a number. 

Note: instead of worlds.item(5), worlds(5) can also be used since it's a default 

    [read] unsigned int Count 

The number of worlds in this list 

# Advanced

You can also use OLEVIEW.EXE or anything else that can view a type library resource on BeipMU's executable to see the raw interface in OLE-speak.

## Using BeipMU from Visual Basic

(Note, this was cool and exciting back in the early 2000's. I haven't tried using this for many years, but if anyone needs it here's the info)

OLE!  Really, BeipMU supports OLE Automation and this enables apps like Visual Basic (or even C++) to access it from other processes.  To control BeipMU through VB, the following steps need to be done:

1. Start BeipMU

2. Type '/register' (this only needs to be done once per new version)

3. Type '/activate' (needs to be done when you start BeipMU otherwise VB won't see it)

At this point, you can use Visual Basic's object browser and 'BeipMU 2.00' will be in the list.  Simply check it and use BeipMU as you would any other COM component.

/register - This writes BeipMU's COM interface information into the registry so VB knows what BeipMU can do.  Typically apps do this at install time.  BeipMU does not as it does not require an install or need to touch the registry for regular use.  To remove these entries from the registry simply type '/unregister'

/activate - This puts BeipMU into OLE's ROT (Running Object Table).  Think of it as a way to tell Windows that BeipMU is running and it's ready to handle requests from other apps.  Without doing this, VB will complain that BeipMU is not running.  BeipMU does not currently support creation from scratch by other apps.

