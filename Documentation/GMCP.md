# GMCP Packages 

beip.stats.[Pane title] package lets you update stats in a stat window.

```
beip.stats.Player
{
   "Name": { "prefix": "00", "value": "Bennet", "name-color":"Ansi256(56)" },
   "Hit Points": { "prefix": "10", "value": 823, "max": 1000, "value-color": "#345678", "bar-color": "#00FF00" },
   "Energy Points": { "prefix": "20", "value": 60, "max": 100, "color":"#FF0000", "bar-color": "#8080FF" },
   "PP": { "prefix":"30", "value":"30/60" },
   "Money": { "prefix": "40", "value":123, "color":"#FFFF00", "name-alignment":"right" }
}
```

![Image of Sample](/images/GMCP_Stats.png)

## JSON Structure

The values inside are a collection of name/value pairs where the name is the name (obviously) and the value is another object that holds settings on how to display the whole name/value stat in the window.

**value** - A string or number value that is the value to display
* Only number values can be used with a ranged value display

**min** - If present, implies the value is a range and this is the minimum value of the range

**max** - If present, implies the value is a range and this is the maximum value of the range
* To show as a range, **value** must be a number and **max** must have a number value (if **value** is a string, it will just show as a string)

**bar-color** - When drawing as a range, this is the color used to draw the range bar

**name-alignment** - A string with the value of left/right/center for how to align the name

**color** - The color to use when drawing the name & value (overrides **name-color** or **value-color** with this color)

**name-color** - The color to draw the name

**value-color** - The color to draw the value

**prefix** - The stat entries are sorted alphabetically by name. To make things sort in a desired order this string value is prefixed to the name for sorting purposes.

## Updating values

Only values that are changing need to be set, and only the properties changing need to be specified. For ranges, **value** **min** **max** and **bar-color** must all be sent again.

## Deleting values

Simply send an empty object for that value. For example, this will delete "Name" and "Money" if they exist:


```
beip.stats.Player
{
   "Name": {},
   "Money": {}
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
