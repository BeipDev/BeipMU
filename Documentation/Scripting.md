Scripting is by default using JavaScript (and on Windows 10 it will use the Chakra engine vs the old ActiveScript version). The startup script in the 'Preferences' window is the filename of the script to run at startup (vs having to enter your scripts every time you launch manually).

Scripting isn't used by many folks so this documentation doesn't get much activity. If you any questions or suggestions please let us know!

## API Reference

[API Reference](ScriptingAPI.md)

## Examples

[Examples](ScriptingExamples.md)

## Global Objects

    App app
The main interface

    Window_Main window
Set to the current window the script was run from, or which window triggered the event.


## Immediate Mode

To run a script immediately (or to add new functions), simply prefix your input with '/@'

For example, in Javascript:

    /@window.output.write("Testing");

Will write something into the output window.

Or entered as a function, like so:

    /@function foo(userdata)
    {  
      window.output.write(userdata);
    }

## Errors/Crashing/Hanging 

Scripts can cause errors and crash themselves, but they should never crash BeipMU.  So experiment!  When an error occurs, a separate Script Debug Output window will popup showing the information from the error.  You can clear the Script Debug contents, or close it as it will reappear when necessary.

If a script hangs for a long period of time, BeipMU will pop up a window that allows you to stop the currently running script.  Scripts are currently processed synchronously, so while a script is running, BeipMU is frozen.  Therefore scripts should be made to do their job and finish, large calculations are not recommended!  If a need arises we could make it so that scripts can be run in other threads, let us know!
