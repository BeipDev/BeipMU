# How to Map!

[Map library](../Maps)  (Not sure the best way to download the maps besides opening in raw view on github and saving as a .atlas file, will need to figure this out at some point)

## Common Controls

Mouse Wheel - Zooms in/out to the location of the cursor

Middle Click & Drag - Pans the map

Control - Smooth movement, if something snaps to the grid, holding down Control will disable snapping.

Shift
* When creating, stickies the action, for example to create multiple rooms just hold shift while doing it and it'll stay in room create mode.
* When resizing, sizes around center vs moving the point selected

Note: The grid isn't a limit for anything, it simply draws around the bounds of the map objects. You can draw rooms outside the grid bounds and drag them outside. The grid will then grow to fit whatever you do! It just exists to help align things, it can be disabled in the options.

## Menu

File -  open/save/save as/etc...

  If PNG is chosen in Save As... it'll generate a PNG of the current map at the current scale

Edit - All the typical goodies, undo/redo/cut/copy/paste/find/etc...

## Navigation Pane

Clicking and dragging on anywhere that isn't a room will pan with the navigation actions

### 📌 Push Pin (Set Current Room)

Click on any room to set your current room

### ⌖ Crosshair (Center on current room)

If your current room isn't visible (or on another map) it switches to that map and center it on screen.

### ▶ Play (By Click)

Left click to set a destination room, which will hilight the shortest path if it's possible to reach the room. Once the path is shown, keep clicking in the destination room to move one step closer to it.

### ⏩ Fast Forward (Speed Run)

Left click to get to the destination room as fast as possible (sends all the exits immediately with no delay).

### 🧭 Compass (Try to determine current location)

If the current room isn't set, this will look through your output history to see if it can find a known room name. If one is found it will set it as your current room. This isn't guaranteed to work!

### 👀 Eyes (Try to live track the current location)

If you like having the map show where you are, but still want to type exits in the input window, try enabling this. It will watch for rooms reachable from your current room and move to them if it sees one.

## Create Pane

### 🔵 Blue Circle (Create Room)

Left click to create one of default size or click and drag to set the exact area

### 🟩 Green Square (Create Rectangle)

Left click and drag to create background color rectangles. Useful to mark out areas on the map.

### 🖼 Picture (Create Image)

Left click and drag to insert an image bounded by the area selected. Can also drag & drop image files directly into the map window

### 🏷 Label (Create Label)

Left click and drag to create a label.

## Select Pane

### 👆 Hand with finger (Select)

You can select rooms/rectangles/exits with clicks or by dragging (and shift dragging or clicking to modify an existing selection).

When only one item is selected the adjust handles will appear and you can drag them around to resize or move things.

To set options like colors for multiple objects, just select them all, then right click on any of them. The color will apply to everything selected.

### 🔵🟩🖼🏷 Selection Filters

To have certain objects be ignored when selecting on the map, just unselect them from the selection filter. Very handy to work with only rooms when there are many background objects (or foreground ones).

## View Pane

### 🤚 Pan (hand)

Left click and drag to pan the map. You can also use the middle mouse button/hold down space to pan at any time.

### ➕ Zoom In
### ➖ Zoom Out
### 💯 Zoom 100%

## Misc options

### 🗺 Maps

Create/delete/switch between maps.

### 🎨 Palette

Edit the colors & fonts used on the map (any changes can be undone with undo/redo).

### 🔍 Find

Search for rooms through a partial name. Clicking on the rooms will center them in the view

### ❔ Help

Opens this page

# Right Click Menu

Lets you do fancier things based on the object selected (which will apply to everything selected when possible).

Sent to Front/Back is handy for putting rooms on top of each other or in front of or behind images. Note that exits are always behind the lowest room they're attached to.

# Exits

To create an exit, just select a room, then click and drag on the circle at the center. To create an exit to a room on a different map, right click the room and choose 'Set as exit target room' then go to the other map and right click on the destination room and say 'Create exit to room'. Exit endpoints can be attached to other rooms and also the edges of a room (to make the routes look nicer).

To route exits around things, just select an exit by clicking, then right click somewhere on the line and 'add point'. You can delete existing points by right clicking on them and choosing 'delete point'.

# Console Commands for Maps

## /map_addroom

    /map_addroom <room name> <how to get there> <how to get back>

Example:

    /map_addroom "Crossroads Hotelry" east west

Used to easily create a room + exit. If you have a current room, this will create a new room matching the size of it with the given name, plus the exit already hooked up. Just drag to position the room how you want!

If the exit names are a known direction (like n/east/nw/southwest) it will place the room in that direction relative to the current room.

## /map_addexit

    /map_addexit <how to get there> <how to get back>
    
Example:

    /map_addexit north south
    
Will look for a room in the direction the exit names imply and if it sees one nearby it will create an exit to it.

# Customization

## Trigger/Alias Automation

Tested on MUX, but could be adapted to other codebases (using '@pemit me=' instead of 'think', or other methods)

Add an alias with these settings:
    
    Alias regex match: ^/map (.+) (.+)
    Alias send text: think MAP> Exit: <\1> - Room: <[name(room(\1))]> - Return: <\2>
    
Add a trigger with these settings:
    
    Trigger regex match: ^MAP> Exit: <(.+)> - Room: <(.+)> - Return: <(.+)> 
    Trigger action Send: /map_addroom "\2" \1 \3 

Usage: 
    
    /map <exit to target room> <return exit>
