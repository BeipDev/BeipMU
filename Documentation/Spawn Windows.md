To make the best use of spawn windows, a little extra info is useful. Combining them with regular expressions it's possible to use them to redirect a WHO list to a separate window, or even automatically group chat channels together.

Grouped tabs can be rearranged by dragging the tabs, or closed through a right click menu.

# To capture a WHO list:

For a muck with a standard WHO list, the regex to detect the start of a WHO looks like this:
    ^Player Name +On For +Idle +Doing...

Then set the 'Capture Until' to:
    ^[0-9]+ players are connected.

# To group chat channels together:

With chat channel lines resembling something like '[public] text' the regex is this:
    ^\[([^\]]+)\]

Then to have the window title be the name of the channel set 'Window Title' to
    \1

To group all channels together into a single window, set 'Add to tab group (optional)' to something like:
    Channels

# To make a grouped tab pane visible

See the /switchtab command
