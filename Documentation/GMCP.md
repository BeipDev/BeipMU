# GMCP Packages 

On connect, if the server sends the telnet 'WILL GMCP' BeipMU will reply with these two GMCP messages:

    Core.Hello {"client":"Beip", "version":"build number"}
    Core.Supports.Set [ "Beip.Stats 1", "Client.Media 1" ]
    
* [beip.stats](#beipstats)
* [client.media](#clientmedia)

# beip.stats

This package lets you display and update stats in multiple stat windows.

```
beip.stats
{
   "Player":
   {
      "background-color": "#002040",
      "values":
      {
         "0_Name": { "prefix-length": 2, "string": "Bennet", "name-color":"Ansi256(56)" },
         "1_Hit Points": { "prefix-length": 2, "range": { "value": 823, "max": 1000, "bar-fill": "#00FF00" }, "value-color": "#345678" },
         "2_Energy Points": { "prefix-length": 2, "range": { "value": 60, "max": 100, "bar-fill": "#8080FF" }, "color":"#FF0000" },
         "3_PP": { "prefix-length":2, "string":"30/60" },
         "4_Money": { "prefix-length": 2, "int":123, "color":"#FFFF00", "name-alignment":"right" }
      }
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
beip.stats
{
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
* **bar-fill** - This is the color used to fill the range bar

## Updating values

Only values that are changing need to be set, and only the properties changing need to be specified.

## Deleting values

Simply send an empty object for that value. For example, this will delete "0_Name" and "4_Money" if they exist:

```
beip.stats
{
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

Colors are string values in the format of #RRGGBB or ansi256([decimal number from 0-255]).

Ansi256 is to simplify porting colors from 256-color ansi to colors here.

Examples:

```
"#8080FF"
"ansi256(34)"
```

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
