##API Reference

[API Reference](/r/BeipMU/wiki/scripting/interfaces)

##Examples

[Examples](/r/BeipMU/wiki/scripting/examples)

##Global Objects

    App app
The main interface

    Window_Main window
Set to the current window the script was run from, or which window triggered the event.


##Immediate Mode

To run a script immediately (or to add new functions), simply prefix the line with '/@'

For example, in Javascript:

    /@window.output.write("Testing");

Will write something into the output window.

##Errors/Crashing/Hanging 

Scripts can cause errors and crash themselves, but they should never crash BeipMU.  So experiment!  When an error occurs, a separate window will popup showing the information from the error.  Once done with this window simply close it.  It will reappear when necessary.

If a script hangs for a long period of time, BeipMU will pop up a window that allows you to stop the currently running script.  Scripts are currently processed synchronously, so while a script is running, BeipMU is frozen.  Therefore scripts should be made to do their job and finish, large calculations are not recommended!  If a need arises we could make it so that scripts can be run in other threads, let us know!
