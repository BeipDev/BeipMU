# Tips and Tricks
There are many features in BeipMU - Some more obvious than others. This guide lets you know some of the cool things you can do and provides a few ideas for you to play with!

## The Input window.
## Clear things down.
The input window is of course where you type everything you want to send to the MU*.
Sometimnes you'll want to clear everything out - Luckily there's:
    `/clear`
This removes all the text from your main window.

## Pause your typing, come back to it.
The History window lets you scroll back using Ctrl-Up/down - But did you know if you use Ctrl-Down, whatever is currently in the input window will be saved to the history list and the input window ill be cleared for you to type and send something else?

This means you can use Ctrl-Up to go back to what you were doing.

But you're not limited to just **one** unfinished bit of text - you can keep doing it as many times as you like. Just scroll back up (or down) to where you want to be!

## Fan Fold
The fan fold option is a mysterious technology that science has yet to explain that lets you pick two extra background colors (Or the same colour twice), and apply them to the Output Window.
Each new line, even if it wraps into a big chunk o' text, will then get alternating stripes of background, making it easy to see the difference between lines.

## Pick your Font!
MU* servers traditionally have always expected you to use a _Monospace_ font, like Consolas, Courier New (which is BeipMU's default font).
Each letter in a _Monospace_ font is the same width, so they all line up into a pleasing grid. Orderly, soothing. Yesss.
But most text in the real world is a _Proportional_ font - each letter is a different width, because it looks nicer.
You can re-set your default font to anything you like... Or you can mix and match!

### Example
The default font for BeipMU can be set to Consolas. But you can then create a new trigger called 'Quoted text'.
Tick the Regular Expression option and paste:

    (?:"\S[^"]+\S")

Into Matcheroo.

Now, select the tick boxes for 'Change Font' and 'Change Foreground', then pick a font and colour.

You may of course choose any font or colour, or only one of these option. But we both know you're goign to go with Comic Sands and pink because _nobody will ever know_.

## Talk to me (Sort of)?
While BeipMU cannot currently use speech synthesis to tell you what is happening, you can use an online service such as

    http://www.fromtexttospeech.com

To create spoken sound files, such as 'New message', which can be used with your triggers - Especially with the 'Only when... Away' option.

## The Power of... EMOJI! ❤
So Kawaii.

BeipMU is a modern Mu* client with modern text technology - it can render any language or font installed on your computer. And that includes ✨Emoji!✨.

By default, turning Emoji on will append the emoji symbol to the end of any word it recognises.

You can edit the text file to make your own custom set quite easily - But you can also use Windows own emoji panel (the Windows key+. brings it up) to add emoji to the names of your spawn windows.

Not only can you add them to titles, but if you create a trigger that uses the Filter text, you can add them to text.

### Example.
I have a friend called Bob.
I have created a new trigger called 'My buddy Bob' - "Bob" is the matcheroo text, and I have selected 'Match Case' and 'Whole Word'.

In the Filter tab I can now select 'Activate' and use the following in the text box:

      ⭐\0

Now every time I see ⭐Bob, he'll definitely stand out. Good old ⭐Bob.

You could also add 💬 to the front of your incoming 'Whisper' or 'Page' highlights. Be creative, find a use for 🍤.

##Swap tabs from the keyboard.
Now **evryone** knows you can cycle tabs using Ctrl-Tab and Ctrl-Shift-Tab in BeipMu, right? ... right?
Well you can also use alt-1 for the first tab, alt-2 for the second... and so on.

## Better Channel Chat

While it's pretty easy to group your channels into a window, because channels have a specific prefix for incoming and out-going lines, you can also use a **Puppet Window**.

### Example

Our channel name is `[Public]` and our command is `Scream` because apparently it's PossumMUCK. All the incoming #Public channel lines look like this:

`[Public] Welcome to the #public channel. Read the rules and stay out of my trash!`

And the command to send text to the channel is:

`scream public=`

... And that means if you open up the connection list, select your character, and hit the **New** button, and make a Puppet window, you can use `[Public]` as the 'Receive Prefix' and and `scream public=` as the "Send Prefix".

Now you have an entire window just for that channel! Don't forget: If you already set up triggers for channels, you might need to exempt any that are being used with a puppet window, or else you might find the conflict causes ingoble and shameful faiulure.
