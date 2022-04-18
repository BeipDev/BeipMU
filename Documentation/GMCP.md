# GMCP Packages 

On connect, if the server sends the telnet 'WILL GMCP' BeipMU will reply with these two GMCP messages:

    Core.Hello {"client":"Beip", "version":"build number"}
    Core.Supports.Set [ "Beip.Stats 1", "Beip.Tilemap 1", "Beip.Id 1", "Client.Media 1" ]
    
* [beip.stats](#beipstats)
* [beip.id](#beipid)
* [beip.tilemap](../TileMap.md)
* [client.media](#clientmedia)

# beip.stats

This package lets you display and update stats in multiple stat windows.

```
beip.stats {
   "Player":
   {
      "values":
      {
         "0_Name": { "prefix-length": 2, "string": "Bennet", "name-color":"Ansi256(56)" },
         "1_Hit Points": { "prefix-length": 2, "range": { "value":823, "max": 1000, "bar-fill": "#00FF00" }, "value-color": "#345678" },
         "2_Energy Points": { "prefix-length": 2, "range": { "value": 60, "max": 100, "bar-fill": "#8080FF" }, "color":"#FF0000" },
         "3_PP": { "prefix-length":2, "string":"30/60 \u0041 \u2648\u2640 \u849c\u8089" },
         "3_": { "prefix-length":2, "string":"" },
         "4_Money": { "prefix-length": 2, "int":123456, "color":"#FFFF00", "name-alignment":"right" },
         "5_Progress": { "prefix-length": 2, "progress": { "label": "75%", "value":0.75, "fill-color": "#FF0000" } },
         "6_Experience": { "prefix-length": 2, "progress": { "label": "1,234 XP", "value":0.65, "fill-color": "#C07070", "empty-color": "#804040", "outline-color":"transparent" } }
      },
      "background-color": "#002040"
   }
   "Attributes":
   {
      "values":
      {
         "Strength": { "int": 10 },
         "Dexterity": { "int": 5 },
         "Charisma": { "int": 1 },
         "Stamina": { "int": 15 }
      }
   }
}
```

![Image of Sample](/images/GMCP_Stats.png)

## JSON Structure

```
beip.stats {
   "<window pane title>":
   {
     "background-color": "Optional window background color"
     "values": { JSON object that is the list of stats to update }
   }
   
   "<another window pane title>":
   {
     ... same as above
   }
   
   etc...
}
```

## The values object

"values" is a JSON object where the name is the name (obviously) and the value is another object that holds settings on how to display the whole name/value stat in the window.

**name-alignment** - A string with the value of left/right/center for how to align the name

**color** - The color to use when drawing the name & value (overrides **name-color** or **value-color** with this color)

**name-color** - The color to draw the name

**value-color** - The color to draw the value

**prefix-length** - The stat entries are sorted alphabetically by name. An easy way to change the sorting order is to prefix your names to change how they sort, but you might want to hide that prefix when displaying the name. This length is the number of starting characters to ignore when displaying the name.

### The three types of values that can be displayed:

A single stat can be one of three types, an int, a string, or a range. The presence of the values below determines the type. If multiple values are listed, the last is the one that wins. To change the type, just send a new value for the desired type.

**int** - A signed integer

**string** - A string

**range** - A range, defined by an object with the following values:

* **value** - A signed integer that is the current range value
* **min** - The minimum value of the range (default is 0)
* **max** - The maximum value of the range (default is 0)
* **fill-color** -  The color of the completed range bar area (named bar-color in pre 309, kept for compatibility)
* **empty-color** - The color of the empty area of the range bar
* **outline-color** - The color of the range bar outline

**progress** - (Starting in 309) A progress bar, defined by an object with the following values:

* **label** - The text to display
* **value** - A floating point value going from 0.0 to 1.0 that indicates the progress amount
* **fill-color** - The color of the completed progress bar area
* **empty-color** - The color of the empty area of the progress bar
* **outline-color** - The color of the progress bar outline

## Updating values

Only values that are changing need to be set, and only the properties changing need to be specified.

## Deleting values

Starting in build 309: To delete individual stats or all stats in a pane at once, just use the JSON null value:
```
beip.stats {
   "Player":
   {
      "values":
      {
         "0_Name": null,
         "4_Money": null
      }
   }
   "Online Players": null
}
```

Pre 309 you can only send an empty object for that value. For example, this will delete "0_Name" and "4_Money" if they exist:

```
beip.stats {
   "Player":
   {
      "values":
      {
         "0_Name": {},
         "4_Money": {}
      }
   }
}
```

## Colors

Colors are string values in the format of #RRGGBB or ansi256([decimal number from 0-255]). A value of 'transparent' is also valid for some properties.

Ansi256 is to simplify porting colors from 256-color ansi to colors here.

Examples:

```
"#8080FF"
"ansi256(34)"
"transparent"
```

# beip.id

![Image of Sample](/images/GMCP_Line.png)

Aka "Player Avatars"

Lets the server specify image links that are displayed to the left of a line of text. Typically this is to show an avatar for the player speaking, but it can be used to show any image on any line of text.

## image-url
The simplest way to get an image on a line is to send this before the line:
```
beip.line.image-url "a url of an image"
```

This is simple, but less efficient and flexible than the id messages below. Typically URLs will be much longer than an ID string, and the server has no way to update this URL after it is sent, or provide any extra text on hover. It's a good place to start as a simple test of inline images.

## id
The server will send this GMCP before a line of text:
```
beip.line.id "1234"
```

It is simply a single JSON string value that is the id to associate with this line.

## ids
The server will send this GMCP to specify details about an id. Sent in response to the client sending beip.id.request, or preemptively by servers keeping track of it themselves (see id.request):
```
beip.ids
{
  "1234":
  {
    "url":"a url",
    "click-url":"the url to open when clicking on the image",
    "hover-text":"text to show in the on-hover tooltip, can include newlines"
  },
  "another ID value":
  {
     "url":"update just the url"
  },
  "etc...":
  {
  }
}
```

**url** is self explanatory. The URL of the image to display in a column to the left of a single line of text

**click-url** the URL to use when clicking on the avatar image. This is optional. If not set it will use the "url" field.

**hover-text** When showing a tooltip for the image, this text will be shown to the right. Useful for extra information about a player.

If the server wishes to update the id information on the client, it can send this message at any time. It only has to send the fields that are changing. If "url" is changed, then the existing images will update to show the new image URL.

## id.request

Sent by the client to the server to request an 'ids' response:
```
beip.id.request "1234"
```

This is only sent if the client doesn't already have information about a previously received id. A server could just send an "ids" message before the first time it sends an "id" to a client, but then the server has to keep track of what ids it has already sent to the client. This message makes it easier for the server to know when to send ids.

# client.media

Aka "MCMP" Aka "Mud Client Media Protocol"

BeipMU supports a subset of MCMP that still allows the server full functionality. With this the server can still play any number of sounds, have them loop as needed, and stop them when needed. Instead of relying on the *type* *tag* *priority* *continue* and *key* values to play and stop sounds, the server can just do it directly by name.

## Client.Media.Default

Fully supported

## Client.Media.Load

Fully supported

## Client.Media.Play

Only *name*, *url*, *volume*, and *loops* are supported

## Client.Media.Stop

Only *name* is supported
