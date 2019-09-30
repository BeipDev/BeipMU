# What are Stat Windows?

Stat windows are a way to display information in the form of `name = value`. So for example a health bar, your number of gold coins, or your character attributes.

![Sample](/images/Stats_Sample1.png)

![Sample](/images/Stats_Sample2.png)

Stats are just another trigger action, which means that two different triggers could update the same stat. Or with regex, one trigger could update multiple stats! The trigger that updates the stat is the one that decides the style (color/font/etc). So if you want a stat to change appearance, just setup multiple triggers to do it.

If you can't figure out how to get a stat to work, please just ask on our discord and we can add it as a new example below!

## There are three types of stats:

* String - The regex for the value is displayed exactly as it is.

* Integer - The regex for the value is interpreted as an integer. The main difference between this and a string is the 'Add value' option which instead of replacing the previous value, adds the integers together! (negative values can be used to subtract). Useful to count the times something happens, or sum values seen at various times.

* Range - The regex for the value is interpreted as an integer, plus there is a lower and upper value for the range and this enables showing it as a progress bar. Mostly useful for attributes with known maximums (or also minimums).

## Sorting

Stats are sorted alphabetically, with the 'Invisible Prefix' prefixed on the name field. This lets you prefix with an invisible number to enforce a sort ordering.

# Sample stats:

## Health bar

![Sample](/images/Stat_HealthBar.png)

Server sends: `Health 174/200`

Matcharoo: `Health (\d+)/(\d+)`

Stat tab:

Name:`Health`

Value:`\1` The first regex term is the current health value

Range:

Min:`0` Simple default, health's lowest value is zero

Max:`\2` The second regex term is the max health value

## Character attributes

![Sample](/images/Stat_Dexterity.png)

Server sends: `Dexterity 22/25` (also Strength, Intelligence, Constitution, etc..)

Matcharoo: `(Strength|Intelligence|Dexterity|Constitution|Charisma|Wisdom) (\d+)/(\d+)`

Stat tab:

String selected:

Name: `\1` The name of the stats is the first term matched in the regex.

Value: `\2/\3` The value is the 2nd term / 3rd term

Window Title: `Character Attributes` We might as well put this in a separate stat pane
