# How to Map!

## Common Controls

Mouse Wheel - Zooms in/out to the location of the cursor

Middle Click - Pans the map

Smooth movement - If something snaps to the grid, holding down Control will disable snapping. 

Shift - Stickies an action, for example to create multiple rooms just hold shift while doing it and it'll stay in room create mode

Note: The grid isn't a limit for anything, it simply draws around the bounds of your current rooms. You can draw rooms outside the grid bounds and drag them outside. The grid will then grow to fit whatever you do! It just exists to help align things, it can be disabled in the options.

## Navigation Pane

Clicking and dragging on anywhere that isn't a room will pan with the navigation actions

### 📍 Push Pin (Set Current Room)

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

## Editing Pane

### 👆 Hand with finger (Selecting Mode)

You can select rooms/rectangles/exits with clicks or by dragging (and shift dragging or clicking to modify an existing selection).

When only one item is selected the adjust handles will appear and you can drag them around to resize or move things.

When a single room is selected clicking in the top half (with the box) will edit the room's name. The font will auto shrink to fit if the room name text gets too large to fit at its default size.

To set options like colors for multiple objects, just select them all, then right click on any of them. The color will apply to everything selected.

### 🟦 Blue Square (Create Room)

Left click to create one of default size or click and drag to set the exact area

### 🟩 Green Square (Draw Rectangle)

Left click and drag to create background color rectangles. Useful to mark out areas on the map.

### 🤚 Pan (hand)

Left click and drag to pan the map. You can also use the middle mouse button/hold down space to pan at any time.

## Misc options

### 🎨 Palette

Edit the colors used on the map (any changes can be undone with undo/redo).

### 🔍 Find

Search for rooms through a partial name. Clicking on the rooms will center them in the view

### 🗺 Maps

Create/delete/switch between maps.

The map name to the right is editable, just click on it to rename it.

## Menu commands (the left hamburger menu)

File -  open/save/etc...

Edit - All the typical goodies, undo/redo/cut/copy/paste/find/etc...

Options lets you change the room/exit fonts, etc..

# Exits

To create an exit, just select a room, then click and drag on the circle at the center. To create an exit to a room on a different map, right click the room and choose 'Set as exit target room' then go to the other map and right click on the destination room and say 'Create exit to room'. Exit endpoints can be attached to other rooms and also the edges of a room (to make the routes look nicer).

To route exits around things, just select an exit by clicking, then right click somewhere on the line and 'add point'. You can delete existing points by right clicking on them and choosing 'delete point'.
