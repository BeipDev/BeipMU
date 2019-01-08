#EMOJI

As BeipMu can display Emoji, there are a set of options for display and parsing. You can also edit the behaviour of Emoji matching and display.

##Enable
Under Options is the Emoji dialogue.

Ticking the Enable checkbox will turn Emoji on.

###Default behaviour
The default behaviour is to replace common emoticons with their graphical counterparts. E.g.:-

    :) becomes 🙂

Other matched words will have the relevant emoji appended to the end:

    # Fox   becomes   Fox🦊
    # Kiwi  becomes   Kiwi🥝

... and so on.

##Editing
The first button on the Emoji Dialogue is "Make an editable copy in config location".
The Config location is usually:
    `c:\users\yourname\APPDATA\BeipMU`

However, once this button has been clicked, you can select "Edit editable copy in notepad" - This will open Notepad with the configuration file.

While this may look complex, it is in fact really easy to deal with.
Any line that you do not want BeipMu to try and read starts with #.

The syntax is explained at the start of the text file but basically, you have the emoji, a space the list of words you want it to trigger on, separated by a comma. Eg:

    To append a star emoji (⭐) to the word "star":
    `⭐ star`
    The match text is not case sensitive.
    To match two words:
    `⭐ star, stars, nova`

##Creating a custom List
In this example we will be disabling all the emoji apart form a small list of our favourites.

Press Ctrl-F or select 'Find' form the Edit menu.

Search for `# subgroup: country-flag`.

Copy `IGNORE_BEGIN` from the start of the list of country codes, and put `#` before  `IGNORE_BEGIN`.

Search for `# subgroup: face-fantasy` and at the top of the list, paste `IGNORE_BEGIN`.

This has now ignored all the emoji apart form the list of emoticon replacements (e.g. `:)` to 🙂).

We can now create our own custom **replace** group:

    `REPLACE_BEGIN
    💤 zzz, zzzz
    ⚠️ {warning}
    💬 {..}
    ❤️ <3
    💯 100%
    REPLACE_END`

Simple. You can copy any emoji you like, paste it into this list and add a word to be replaced. The 'word' can be a code that you don't normally see - for instance {warning}.

You can now use {warning} in triggers with a filter, to add the warning sign to text.

To make a custom append list, you do not need to do anything but create a list of emoji and the match word:

    ``🧜‍♀️ mermaid
    🧜‍♂️ merman
    🧝‍♀️ elf woman
    🧝‍♂️ elf man
    ❤ <3, love
    ✨ woot, magic, magical, special
    👍 OK
    👏 congratulations`
