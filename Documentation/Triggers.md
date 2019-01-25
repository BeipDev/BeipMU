# Triggers

## What is a trigger?
A trigger is a set of actions that is performed when an event occurs that matches it’s criteria. This is called "firing".
For example:
If you receive a Page from a friend, you might like it to be a different color from the rest of your text.
To to this you need to have some text for Beipmu to Match, and then an action to perform. When the text is matched, the trigger "fire" and the text is colored.
Luckily, this is *very easy.*

## Your first trigger
Here is our text:

    You page, "hello" to Somebodyname.
    Somebodyname pages, "hello" to you.

We want to make it more visible.

In the menu bar, go to Options > Triggers

You now have three options. 

* You can make a trigger just for one Character. (A "Local" trigger)
* You can make a trigger that applies to all characters on a server.
* You can make a trigger that applies to all characters on all servers (A "Global" trigger)

In this example we will make it a Local trigger - You can change this later.

Select your characters name.

Click the "New" button

In the “Description” text box, add a name that makes sense like “**Highlight Pages To Me**”: This name can be anything you like. Try to keep each trigger named differently, as it will make things easy for editing them later.

In the "Matcharoo" text field, we are going to add some text to match on:

    pages, 

This will make Beipmu match “pages, “. This is a simple trigger that just looks for text, like you'd see in the main window.

In the "Appearance" tab, tick the check box for “Change Foreground” - We'll go through the options in more detail later on - For now, You can use the Foreground button to pick a colour for your text if you like, or leave it as the default. Optionally on the right you can select **bold**, *Italic*, _Underline_, ~Strikeout~ or Flashing. Or any ***combination*** you like.
Optionally you can change the background colour, and even the font.

Finally there is an option marked “Whole Line”: Selecting this will apply your colours and styles to the entire line of text, before and after “pages, “.

## Duplicating a trigger
If you now press the “Copy” button, the trigger you have selected will be duplicated. It’s Description will be removed.
Select your trigger “**Highlight Pages To Me**” and copy it.

## Editing a Trigger
In the new trigger, change the empty Description to “Highlight Pages From Me”.
In the Matcheroo field, change the match text to: 

    You page,

Hit the “OK” button, and now page yourself a friendly "Hello!". Your incoming and out-going page should now be highlighted using the options you chose.

# Regular Expressions

## What is Regular Expression

It’s a way to explain to a computer what you want it to look for. It contains codes for ‘Any of these specific things things’, ‘Not that’, ‘Pretty much anything’, and ‘Only if it’s the start of the line’ and many other useful concepts.

You can use websites like https://regexr.com to test your Regular Expression (RegEx) code on any text you like (You can just copy and paste it from BeipMu), as well as use it’s built in reference guide.

## Why would I need it?

Sometimes a simple match won’t do the job, or you might want to have a trigger match on more than one thing.
In our example we have:

    You page, "hello" to Somebodyname.
    Somebodyname pages, "hello" to you.

In the simple match we used “pages, ” and “You page,” - But that can lead to the trigger activating incorrectly (Known as a "False Positive"). For example:

    “Every so often you page through a book and see something amazing”

This would cause your trigger to fire.

We can however use RegEx to fix this and combine both triggers into one.

In your trigger, select the “Regular Expression” option.

Now we want to tell BeipMu that we only want this trigger to activate if the text is at the start of the line. To do this we use ^.

Secondly we want to tell it that we’d like it to fire on _one_ of **two** matches.

We can do this by putting ( ) around the text, and adding | between each option.
    
    ^(You page,|pages,)

This match “You page, “hello!” to Somebodyname” - But it **won’t** match “Somebodyname pages, “hello!” to you.”
The reason is that we have told Beip that the text **has to be** at the start of the line.
We can get around this by describing what we expect to see at the beginning of the line: A name.
In this case _we_ know a name is a bunch of letter (Or even letters and numbers), followed by a space. Sometimes there’s two or more names, and they might even have a hyphen, or an apostrophe in:

    Alex
    Alex51
    Alex Smith
    Alex-Smith
    Alex O'Smith

So to explain to BeipMu that “We’re looking for a set of things that aren’t spaces or line ending codes (These are called Whitespace), followed by a space. There may be more than one of these.” we'd use the following codes:

* \S means ‘A character that isn’t whitespace'. E.g a letter, number or punctuation.
* . means ‘Any single character - number, letter, punctuation, space.’
* \+ means ‘One, or more of the thing that was just before me’

This means we can say:

    \S.+ pages,

And Beip will understand that it should look for “Somebodyname pages,”. So let’s add it to our trigger:

    ^(You page,|\S.+pages,)

This should now match any page that comes in or goes out, using one trigger.

## Advanced options
Let's have a look at the tab options in a little more detail. Most of the settings are very straightforward and intuitive.

But first we need to know in what order triggers are checked agaisnt the text. Triggers, by default, will stack: You can fire one then have a number of others get applied. In this case you'd want to work out the order of events!

### Processing order
BeipMu will process triggers in heirachy order. Either the Global triggers first, then the Server Triggers then the Character specific triggers, or the other way around, and Character triggers will fire first, then Server, then Global
***Processing order of trigger note***
Triggers are chacked from the top of the list, then down to the bottom, per type. So The list of global triggers from top to bottom, then moving on to the Server list then the Character list. If you need re-arrange them... you can just drag them around! It is possible to drag a trigger from Chracter to Server or Global and vice versa if you need to.

### Description
Text, for your benefit, that lets you describe what this trigger is doing. You can leave it blank - BeipMu doesn’t care that you didn’t put any effort in. BeipMu will just feel disappointment.
### Matcheroo
This is where you match text or regular expression goes.
### Match Case
In a Simple Matcheroo trigger, this tells Beipmu that if the match text is “Page”, then it should only match “Page” with a capital P, and not “page”. It is off by default.
### Whole Word
By default, BeipMu will look for a match, even inside other words. So “page” will also match “pages, paged, empaged’. Tick this box to make sure BeipMu matches what you _expect_ it to match, unless you need it to do otherwise.
### Line Start With
Exactly what it seems: This makes BeipMu only fire a trigger if the line starts with the match text. It will ignore it otherwise.
### Line Ends With
Fires the trigger only if the line of texts ends with what’s in Matcheroo.
### Limit to once every…
You can put a cool down timer on your trigger so it won’t fire for X seconds, where X is the number of seconds you type in.
### Stop Processing
If you need it to, you can make BeipMu stop processing any further triggers if this is checked.
### Once Per Line
This tells BeipMu to only run this trigger the first time it sees a match, not every time it sees one in a single line of text.
### Only When..
* Away
Only fire this trigger when BeipMu isn’t the window that’s being used. If you have BeipMu open and you're using it, the window is **in focus**. If you ware using another window, then BeipMu is **out of focus**.
* Present
Only fire this if BeipMu is **in focus**.
### Once
Fire this trigger once and never again.

## The Tabs
The various tabs control the output of the triggers:

### Appearance
This changes the appearance of text:
#### Font
When selected, this enables you to pick a font. Leave this blank to use the default “Output window” font. You can select the type of font you like as well as size and type, if more than one style is supported. For example Thin, Regular, Black.
#### Foreground
This selects the font’s colour.
#### Background
This changes the background colour for the text. It does not extend to the edge of the screen, it **only** is applied to the area of the text.
#### Font styles
These may all be selected or not at all, singly or in any combination, apart from “Use fast flash” which can only be enabled if “Flashing” is also selected.
#### Bold
Makes the font **Bold.**
#### Italic
Makes fonts *Italic.*
#### Underline
Makes fonts _Underlined_.
#### Strikeout
Puts a ~line through the text~.
#### Flashing
Makes the text flash.
“Use fast flash” makes it flash *faster!* - For when you relly need to see something!

### Sound
This enables you to add a sound to your trigger.
#### Play sound
Enables you to turn the this feature on or off.
#### Browse
Lets you look for a sound file. Any file supported by Windows is available.
#### Play
Lets you check you got the right file.

### Gag
This panel helps you remove unwanted text from your screen using a trigger. It does **not** have to be enabled to remove text frem your main window to a spawn panel.
#### Gag this line
The main event. Tick this and whatever is matched will not be displayed.
#### Gag this line in the log file
This option prevents the text from being logged. This means you can have it show onyour main window but not inyour log file, if you have logging turned on.
#### Gag messages (For gag testing)
This removes the matched text and then replaces it with a message stating that a line was removed. Used for testing, or if you like knowing that the trigger is activated.
### Example
You would like to gag the exit message in one of your favorite hangouts:

    The huge oak doors that have stood the test of a thousand year's creak open ponderously, letting the harsh and unforgiving light of the outer realms into the snug and cozy bar (Don't for get to pay your tab or Grognog the Half Orc hobbit will come for your knees: 17% sales tax pplies and a 15% house gratuity for service. We do not accept personal cheques or cursed daggers. Please quaff responsibly.)
    
Ok that was cute the first time but... yeesh! So let's just slip 

    The huge oak doors
    
...into Matcheroo, set it as "Line starts with", turn Gag on and also select "Gag in the log file". And there. Now you never have to see that message again.

### Spawn
This option creates a panel on the right of your main window that has the same color and font settings as your main window, but does not have an input bar. Any matched text will be moved to the spawn window.
Spawn windows can be moved to any edge of the main window or, using ctrl, pulled off and left as a floating window. They can be re-attached at any time by just dragging them back onto the main window.
Spawn windows can be re-sized at will, and stacked side-by-side or on top of each other. You can re-arrange the order or get rid of the: They will re-appear next time a trigger is fired that specifies one.
#### Active
Simply put, this lets you turn the window on or off. If it’s off, nothing happens - the trigger runs all it’s other options as normal. If this is on, then it will create a spawn window when the trigger fires.
#### Window Title
A title so you can see what window is doing what. This is displayed on the title bar of the spawn window panel. Fun tip: You can add emoji, which will appear in the outline style!
If two triggers share the same title, they will use the same window. This means you can send the results of multiple triggers to the same spawn window. It will be created if it's not already open, and if it's already open, new text will just be added.
#### Capture Until
If you need to capture more than one line, you can use regular expression to define the ‘end capture’ scenario:
### Example
The command “+stare” will generate a list of everyone in the room, their ‘doing’ tags and idle time. The output always starts with “You stare at everyone:”, has a list of results, and ends with “All done!”

    You stare at everyone: ------------
    Name:         Doing  Idle
    Alex-Smith    AFK    13m
    Cuprohastes   SMRT   0m
    Bennet        CODE   37h 5m
    ------------------------- All done!

Therefore the main Trigger should be “You stare at everyone:” and the ‘Capture until’ match should be “All done!”
This will move the entire output to the spawn window, then stop capturing text.

### Send
This enables you to send text back to the MU* if the trigger fires.
#### Send text
Toggles this action on, or off.
#### Text to be sent
The actual text you wish to send
#### Example
If you enable the “away” option, and set your main trigger to detect an incoming page, you could set your text to
Page #R=[Automatic message] sorry I’m not here right now I’ll reply when I get back.

### Filter
The Filter option allows you to change the incoming text form the MU* using a set of capture numbers. It optionally lets you use the Pueblo HTML subset.
#### Filter Text
Enables or disables this option.
#### Parse HTML Tags on replacement
Allows use of the PUEBLO HTML subset.
#### Text to replace match text with
This is where you type the text you want.
### Example
In a simple match, you can type the text you want. For example
Match text is: *has disconnected somewhere on the MUCK!*
Incoming text is:* Anyname has disconnected somewhere on the MUCK!*
Replacement text is: *quit.*
Result output to main window is: *Anyname quit.*
BeipMu will display the text that was matched with \0
\0 Means ‘The whole thing that got matched, all of it’.

In a Regular Expression match, each subset that is in brackets is a capture group e.g. 
    (Alex|Betty|Chaz) (has (partially disconnected|disconnected) somewhere on the MUCK!)
Each capture group is automatically numbered. The incoming text is:
Betty has partially disconnected somewhere on the MUCK!
\0 would result in “Betty has partially disconnected somewhere on the the MUCK!” as it is all of the groups.
\1 is the first group - “Betty”
\2 Is the next group counting “(“ so it is “has partially disconnected somewhere on the MUCK!”
\3 is therefore “partially disconnected”.

We can use these in our text filter:
Filter is:
    [Message] \1 has \2!
Resulting in:
    [Message] Betty has partially disconnected!
#### Notes on text
As BeipMu is a modern client, it supports unicode characters, accented text and Emoji. You can therefore paste in any text you like. If you have enabled a spawn window, the filtered text and all other options will be applied before the output is sent to the window.

### Activate
This is a useful option that enables you to decide whether a match to your trigger sends an activity message.
#### Activate window on trigger
If your window is behind another app or not focused, this option will make BeipMu pop up in front of you like a needy puppy.
#### Don’t Show as activity
If this option is ticked, there will be no indication that something happened: No flashing icon or tabs.
### Example
While you might like to have BeipMu flash for attention when someone you know logs on, you might not care when they log off. You could therefore set disconnect messages to "Don't show activity".

### Script
If you have written a script for BeipMu, you can have it run as the result of a trigger by calling its function name.
#### Enabled
Turns the option on, or off.
#### Name of the function to call
Some sort of wizardy goes on here. I don’t trust it. our wizard says:

"All you put here is the function name, not the script. This is so that the scripts can be compiled in advance and execute efficiently."

But you know wizards. Shifty lot. The best thing to do would be to have a look at the documentation for scripting.

### Toast
This does not warm some bread into a deliciously crunchy snack suitable as a substrate for jams, preserves, marmalade, scrambled eggs or baked beans. In future versions this may be implemented, but at the present time it makes Windows display a pop-up, called a ‘Toast notification’.
#### Toast message when trigger hits
This option just turns the feature on or off.
### Example
There is no other option here because it's beautifully simple:- A small notification pops up with the matched text on it, and windows plays it's default chime.

This is most useful for triggers that are set to fire when BeipMu is not in focus. For example, you already have a trigger that matches pages and colors them, but you can duplicate it, activate "Toast message when trigger hit" and disable any other outputs that your original trigger had. Then both will run when you're away form BeipMU but only the text highlight versionw ill run when BeipMu is in focus.

## Sub Triggers

Each trigger can have sub-triggers that act on the text that was captured by the primary trigger. To make one, select your primary trigger and just hit the "New" button. You can also drag an existing trigger onto another to make it a sub-trigger.

## Example

You are capturing text that has a common prefix but the text after that is variable:

+ Game> Alex Checked your Stats
+ Game> Alice looked at you
+ Game> Bulletin Board Updated (BBoard 2/34: New rules for IC combat in the Golen Arena)
+ Game> Grognog has placed a bounty for Unpaid Bar Tab on you!

The captured text for the Primary trigger is passed to the sub triggers, so if you used:

`^Game> (.*)`

Then the Sub triggers would be operating on the text that came after "Game>" but would not include it, as (.\*) is the match group. 

You could also match the entire string:

`^Game> .*`

And use a sub trigger to divide the text using:

`^(Game>.)(.*)`

Which would then allow:

+ \0 : Game> Alex Checked your Stats
+ \1 : Game> 
+ \2 : Alex Checked your Stats

You could then use the Filter Text option of the sub trigger:

`📈 \2!`

Resulting in...

    📈 Alex Checked your Stats!
    
### Sub Sub Triggers.

This is a thing. Let's look at your unfortunate lapse of memory for paying your bar tab (Hey, goats milk ain't cheap!):

    Primary trigger:    ^Game> .*
    Captures:           Game> Grognog has placed a bounty for Unpaid Bar Tab on you!
     Sub Trigger1:      ^(Game>.)(.*bounty.*)
        Capture     \0  Game> Grognog has placed a bounty for Unpaid Bar Tab on you!
        Capture     \1  Game>
        Capture     \2  Grognog has placed a bounty for Unpaid Bar Tab on you!
      Sub Sub Trigger:  (\w+\s){1,3}has placed a bounty for (.*) on you!
                        We want to capture the name, and the reason. In this case I'm assuming that the name may be up to three words
                        long, seperated by a space or punctuation. Eg. Grognor, Grognor-half-Hobbit, El'rick, Fae O'Bannon.
        Filter:         ⚠ There was a bounty on you for \2 from \1!
        Send Text:      $pay/bounty \1
        Capture     \0  Grognog has placed a bounty for Unpaid Bar Tab on you!
        Capture     \1  Grognog
        Capture     \2  Unpaid Bar Tab
        Result:         ⚠ There was a bounty on you for Unpaid Bar Tab from Grognor!
                        You attempt to pay Grognor your bounty price.
                        Insufficient funds. Grognor is coming for your knees.
                        
And so the wheels of justice roll on, you monster.
        
