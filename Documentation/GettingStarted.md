#Getting Started Guide

This guide will introduce you to the main features of BeipMU so that you can get the most out of it.

By the end of this guide you should be able to:

* Add a server to the World List 
* Connect to a world (server) 
* Add a character 
* Enable automated logging 
* Connect to a character 
* Create a macro 
* Create a trigger 
* Create an alias 
* Recognize the other main features of BeipMU 
 

## Before we begin

This help file assumes that you are already familiar with using MUCKs/MUDs/etc. If you are new to these terms then look on the web for more information.

Note that BeipMU's internal commands use / as a prefix, so you will need to use // if you need to send a string that begins with a slash to a connected server. See the Command Line Commands page for a list of BeipMU's commands.

## Connecting to a World

To send text just type it in the input window and press enter. To type multiple lines of text before sending, use Ctrl+Enter to separate the lines before pressing Enter.

As a simple example, let's connect to FluffMUCK. Simply type '/connect fluffmuck.org:8888' in the input window and press enter.  

This utilizes BeipMU’s ‘direct connection’ feature which has some options disabled by default.  If the server you are connecting to has special extensions or encryption, you will want to create an entry for it in the World List where you can set these parameters.

## Using the World List

The World List is used to organize your servers, characters, and puppets. Many of BeipMU’s features use the information in the World list to arrange triggers, aliases, and other connection specific data.

You can open the Worlds dialog by using CTRL-\[ or by choosing Connect under the Connection drop-down menu.

## Adding a server

For this example, we will use a fictitious world called "Compy MUCK".

We want to add a world to the World List, so click on New, and then select Server. Type in the name of this world, e.g. "Compy MUCK".

Next fill in the server details, especially the host field.  The server host is the hostname or IP address of the world you wish to connect to followed by the port on the server to communicate with. The format is server:port

The Server Info field is a place to keep notes and can be used to store any information useful to you about the world.
 
## Adding a character

To add a character to the World List, select the server you want to add it to, click on New, then select Character. Type in the name of the character, e.g. "Beip".

The Connect String is what BeipMU will automatically send to the server when you connect using this character. This will save you from typing it in every time you connect. Our example uses "connect Beip fakepasswd" on the first line and pages Bennet "Hi!" on the second line. To type in multiple lines use Ctrl+Enter to separate each line of commands.

The Character Info field is the same as the Server Info in the previous example and can be used as a place to store notes or information about the character.
 
## Automated logging

If you would like BeipMU to automatically save a log of your session every time you connect your character then click on Log File and browse to the location to save the log(s). You can also just manually type the filename into the text box beside the button as pictured above. Our logs will be saved to a file located at "C:\logs\beip.txt".

If you would prefer BeipMU to include the current date in the log file name, select Append Current Date. Otherwise all logs for this character will be saved to the same file.

Note that automated logging only works if you connect to your character as described in the next section.

## Connecting to a character

To connect to a character, click on Connect, expand the character list next to the server, then double-click on the character to connect. 

An alternative method is to use the /connect and /char BeipMU commands in the input window. Apart from the direct connect method, the /connect command will also use the name for the server that you gave it in the World List and connects you to the server. Likewise, the /char command uses the name for the character on the server you have connected to. The /char command tells BeipMU which character you have connected to, sends the character’s connect string to the server, and starts logging if you have specified a file under the character settings.

To connect to our example character using the input window we would use the following two commands:

    /connect Compy
    /char Beip

## Macros, triggers and aliases

Keyboard macros, triggers and aliases make your BeipMU experience more effective by automating commands and responding to world output.

A keyboard macro, also called a shortcut, allows you to send text to the server or input window with the single press of a key or key combination. This can be used to quickly enter frequently-typed or long commands.

A trigger, also sometimes called an event, responds to output text from the world. It has many uses, such as highlighting words or lines, playing sounds, hiding or filtering the text, or automatically sending a response.

An alias allows you to abbreviate commands so that you can type something much shorter into the Input Window. An alias automatically replaces text that appears in the Input Window before it is sent to the server.

These features are placed in the World List according to where they should be applied. Global macros, triggers and aliases apply to all servers and characters. They can also be set up to apply to specific servers or characters by creating the macro, trigger, or alias while selecting the server or character or dragging the macro to the desired location.

 
### Creating a macro

To create a macro, go to Options, then Macros on the pull-down menu.

Using the World List, select where you want the macro to apply, then press New.

Type in the text that you want to create a macro for into the Macro Text box. You can press Ctrl+Enter for multiple lines.

Check the Type Into Input Window checkbox if you would prefer the text to appear in the Input Window instead of being sent straight to the server.

Select the box beside where it says Press Key To Use Here. For a new macro, this will show (No Key) to indicate that no key has been set. Press the key that you would like to activate the macro.

The modifiers checkboxes show which keys you press with the shortcut key.

Our example sets Ctrl+Q to send "QUIT" to Compy MUCK whenever we press it. (Note that if we press Ctrl+Q when connected to a different world then nothing will happen. We would need to make our macro global if we wanted it to work for all connections.)

 
### Creating a trigger

To create a trigger, go to Options, then Triggers on the pull-down menu. We will create an example trigger that makes noise and pops up BeipMU whenever our name appears. This can be useful to get our attention when we are playing a game in another window.

Using the World List, select where you want the trigger to apply, then press New.

In the long empty textbox near the top right of the Triggers window, type in the text that you want your trigger to match on. In our example, we want to trigger whenever the text "Beip" appears (because we are vain;).

The checkboxes below the textbox allow you to refine text matching. For example, we want to match "Beip" only when it appears as a word, and not for example as part of the word "Beipiaosaurus", so we select Whole Word. We also select Only When and Away so that the trigger only happens when we're using a different program at the time (we do not want BeipMU to get our attention when it already has it!).

The list of tabs let you select what you want the trigger to do when it has matched the text. Most are self-explanatory. We want our trigger to play a sound, so we select the Sound tab, select Play Sound, and then Browse for our sound file. You can press the Play button to test the sound. We also want BeipMU to pop up (for that real sense of urgency) so we select the Activate tab and select Activate Window On Trigger.

You can send text automatically to the world when the trigger is matched. Click on the Send tab, select Send Text, then type in the text you want to be sent. Use Ctrl+Enter to type in multiple lines. In our example we send "say Beip is the best!" whenever "Beip" is matched.

### Creating an alias

An alias allows you to abbreviate commands so that you can type something much shorter into the Input Window. An alias automatically replaces text that appears in the Input Window before it is sent to the server.

To create an alias, press the Aliases button  on the toolbar.

Using the World List, select where you want the alias to apply, then press New.

In the Match Text box, type in the text you want to be matched.

In the Alias For box, type in what you want the matched text to be replaced with.
 
In our example, the fictional Compy MUCK does not have a "time" command, however it does have a "page #time" command, so we create an alias for "time" that changes it to "page #time". We select Line Starts With and Line Ends With so that it only matches when we type "time" and nothing else. This way we get the time displayed when we type "time" instead of a missing command error!

## Keyboard Shortcuts

BeipMU allows all the built in keyboard shortcuts to be redefined. Go to Options, Keyboard Shortcuts.

## Other Features of BeipMU

### History Window

The History Window stores a list of your recently used commands. To cycle through them, use the Ctrl+Up and Ctrl+Down keys. You can search the history using Ctrl+H. (These keys can be changed from the Options, Keyboard Shortcuts menu.)

A nifty trick allows you to save commands in the history without sending them. This can be useful if you are in the middle of typing out a long sequence of text and want to send something else quickly to the server. Just press Ctrl+Down instead of Enter. You can then come back to your saved command later with Ctrl+Up and continue editing.

### Auto Copy

Highlighting text with the mouse in either the Output Window or the History Window automatically copies it to the clipboard. Try it!

To highlight text in the Output Window without automatically copying it, first Pause the output.

### Smart Paste

Sometimes you have several lines of text in the clipboard or a file that you want to send to the server. Smart Paste allows you to do this easily.

Select Edit from the menu bar, then Smart Paste. Select either Clipboard or File.

Prefix and Suffix allows you to add text to the start and end respectively of each line.

### Pause

Pause allows you to suspend the Output Window, halting the flow of new text. This is particularly useful when reading scrollback since the Output window will not automatically scroll when paused.

To turn Pause on or off, use the pause key on your keyboard. A pause symbol will appear in the top right corner when a window is paused. This symbol will also appear when selecting text to indicate that the window is paused while a selection is being made.

A status window will show the number of new lines that have arrived since the display was halted. Unpause the output to display the new lines.

### Logging From Beginning

When starting a log file, if you wish to log the existing scrollback text, choose Logging then Starting From Beginning.  

### Local Echo

Local Echo will echo text in the Output Window that you have sent to the server.

To turn Local Echo on or off, press the Local Echo button  on the toolbar.

### Tooltips

By default, BeipMU will display date and time tooltips for each line of the output window. To turn this behaviour off, go to Options, Output Window. Or right click the window and choose 'settings...'

## Config Files

The configuration file for BeipMU is saved as "config.txt" in the working directory. By default this is the %appdata%/BeipMU directory. If a config file is found in BeipMU's startup directory it will load and save to that file instead. For the Microsoft Store version BeipMU always saves it's data to the hidden application settings folder, which is why 'import/export configuration' options were added as these directories are not meant to be easily browsed by the user (on my system it's in something resembling C:\Users\Username\AppData\Local\Packages\94827398274.BeipMU_ixjzv98j98\LocalCache\Roaming\BeipMU).
