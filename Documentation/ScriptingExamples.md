## A Dockable Filter Window

    // Create a Text output window
    var wndText=app.NewWindow_Text();

    // Make it into a dockable window and dock it on the right side of the current window
    // 0=left, 1=top, 2=right, 3=bottom (or remove the .Dock(...) part and dock it manually)
    window.MakeDocking(wndText.properties.hwnd).Dock(2 /*Right*/);

    // Define the function to set in the trigger
    function ShowText(window, text) { wndText.Add(text); }

In any trigger that you want to route text to this window, go to the Script tab and put 'ShowText' as the function (be sure to enable it too)


## Script to remove empty lines of text.

This script is used to remove zero length lines of text in the Output window.  If the length is = 0, it does not print.

    /@function Test(string, window) { return string.length == 0; }
    window.connection.SetOnReceive(Test, window);


## Script to place tags at the front of incoming text.

    /@function Test(string, window) { window.connection.display("<Trigger>" + string); return false; } window.connection.SetOnReceive(Test, window);

It can be used to place the word <Trigger> at the start of each line in Beip's output window.  
    
    For example:
    
    <Trigger>This  could be used as a way to set the font for a specific connection or player.
    <Trigger>If you use the tag as your matcharoo set the font for the whole line and then turn on the option to filter out the text to return it to looking nice.
    

## Script that will activate the second time a trigger happens on a separate line.


    /@
    var triggercount =0; 
    function Toggle(string, window) 
      { 
    if (string.search("trigger")>-1) 
    { 
      triggercount = triggercount+1;
    } 
    if (triggercount==2)
    { 
      window.connection.display("<Triggered>" + string); 
      triggercount=0; 
    }
    return false; 
    } 
    window.connection.SetOnReceive(Toggle, window);
    
    For example:
    
    The code will place the tag at the start of the line where the trigger text has been detected.
    <Triggered>Not actually by the word that trigger itself.
    If someone mentions the trigger string (in this case "trigger") twice in the same string.
    <Triggered>The script will only detect the first use of trigger, and consider the next time it sees it as the second instance.
    
## Script to handle text received and text sent to the server.

First, the receive script is similar to what we have before, it looks for the state of the 'trigger' flag, and if it is true, puts <flag> at the start of the text received by Beip, until it sees the words 'players are connected'.  The send script below checks input text for the letters "WHO".

    /@
    var trigger = false; 
    function Toggle(string, window) 
    { 
    if (trigger)
      { 
        window.connection.display("<Flag>" + string); 
        if (string.search("players are connected")>-1) { trigger = false; }
        return true;
      }
    return false; 
    } 
    window.connection.SetOnReceive(Toggle, window);

    function Who(string,window)
    {
    if (string.search("WHO")>-1) { trigger = true; }
    return false;
    }
    window.connection.SetOnSend(Who, window);

The result of this would be something like so:

    <Flag>Player Name    On For Idle  Doing....
    <Flag>Guest 1    00:12   0s @ Doing message
    <Flag>Guest 2   00:40  0s @ Doing guest things
<Flag>2 players are connected.  (Max was 40)


## A method for breaking out elements of a WHO list, based on fixed length strings.

    /@
    var trigger = false; 
    function Toggle(string, window) 
    { 
    if (trigger)
      { 
        if (string.search("Player Name")>-1) { return true;  }
          else {
            if (string.search("players are connected")>-1) 
              { 
                trigger = false; 
              } else  {
          window.connection.display((string.substring(0,20)).trim());
          window.connection.display((string.substring(22,28)).trim());
          window.connection.display((string.substring(29,33)).trim());
          window.connection.display(string.substring(36,string.length).trim());
        }
      return true;
      } 
      }
      return false; 
      } 
      window.connection.SetOnReceive(Toggle, window);

## A method for capturing a player's skills by pulling data from a table.

Consider the following table:

    axe                100%  dagger             100%  flail               75%  
    mace                75%  polearm            100%  shield block       100%  
    spear              100%  sword              100%  staff               75%  
    whip                75%  berserk             75%  blind fighting     100%  
    disarm             100%  dodge              100%  dual wield         100%  
    enhanced damage    100%  parry               75%  rescue              75%  
    second attack      100%  third attack       100%  fourth attack      100%  
    You have 39 practice sessions left. Do not forget to improve! (help improve)

Normally this would be hard to make a trigger for, but because we can check for the player typing 'practice' we can parse this table using the following script:

    /@var 
    trigger = false; 
    function Toggle(string, window) 
    { 
    if (trigger)
    { 
    if (string.search("practice sessions left")>-1) 
         { 
            trigger = false; 
         } else  {
          window.connection.display((string.substring(0,17)).trim());
          window.connection.display((string.substring(18,23)).trim());
          window.connection.display((string.substring(25,42)).trim());
          window.connection.display((string.substring(44,49)).trim());
          window.connection.display((string.substring(50,67)).trim());
          window.connection.display((string.substring(68,73)).trim());
      return true;
      }
    }
    return false; 
    } 
    window.connection.SetOnReceive(Toggle, window);


    function Inventory(string,window)
    {
    if (string.search("practice")>-1) { trigger = true; }
    return false;
    }
    window.connection.SetOnSend(Inventory, window);

## A method for finding multiple triggers in the same line of output

Here is an updated script that splits the line on the matching text, inserts a <trigger> tag and then glues the string back together again.

    /@
    function Test(string, window) 
    { 
    var output = "";
    var searchtext = "north";
    var splitstring = string.split(searchtext);
    if (splitstring.length)
    { output = splitstring[0]; for (i = 1; i < splitstring.length; i++) { output += "<Trigger>" + searchtext + splitstring[i]; }
    window.connection.display(output);
    return true;
    } else { return false;}
    } 
    window.connection.SetOnReceive(Test, window);
    
    For example:

    You say, "<Trigger>north <Trigger>north <Trigger>north"
    
## A method for finding the lowest skill on the player's skill sheet using the above table of skills.

I used a for loop to glue the string back together again, the same sort of thing can be used to parse the skill table above, when you consider that the table is in a 25 character width column, the first 18 characters are the skill name, followed by a space, then 3 characters for the skill proficiency followed by a percent sign which I ignore.  A script like so would remove the three checks per line, by using a for loop to combine them:

    /@var worstskill="";
    worstpercentage=100;
    trigger = false; 
    function Toggle(string, window) 
    { 
    var percentage = 0;
    if (trigger)
      { 
      if (string.search("practice sessions left")>-1) 
        { 
           window.output.write("Your worst skill is " + (worstskill).trim() + " at " + worstpercentage + "%");
           trigger = false; 
        } else  {

          for (i=0;i<76;i=i+25) 
          {
            percentage = parseInt(string.substring(i+19,i+22));
            if (percentage < worstpercentage)
              { 
                worstskill = (string.substring(i,i+17));
                worstpercentage = percentage;
              }
          }
        return true;
        }
    }
    return false; 
    } 
    window.connection.SetOnReceive(Toggle, window);

    function Inventory(string,window)
    {
    if (string.search("practice")>-1) { trigger = true; }
    return false;
    }
    window.connection.SetOnSend(Inventory, window);


## A script for creating your own command line commands.

If you are interested in creating your own client-side commands, you could use a script like so:

    /@var trigger=False;
    function Inventory(string,window)
    {
    if (string= "@@practice") 
      { 
        trigger = !trigger; 
        window.output.write(trigger); 
        return true;
      }
    return false;
    }
    window.connection.SetOnSend(Inventory, window);

This script creates the command '@@practice' which basically flips a variable between true/false and displays the result. 

When processing an OnSend event, returning TRUE means that your script has handled the sent string from the user and it should not be sent to the server.  Returning FALSE means that more processing is needed, and the string, modified or not, is sent onwards.  If your server has a @@practice command on it, the script will flip the value of the trigger variable, then the server will respond to @@practice.  If you return TRUE instead, you won't get a "HUH? Please try that again." message following your script output.


## A script to pull data from a one line prompt.

Lets look at data stored on a single line.  Lets consider the following prompt:
 
    <1824/1824hp 350m 406mv -25542xp>

It contains a player's current HP, max HP, mana points, movement points, and experience needed to level. I want most of these to persist between script triggers, so make them global variables.

    /@
    var hp = 0;
    var maxhp = 0;
    var mana = 0;
    var move = 0;
    var xp = 0;

    function Prompt(string, window) 
    { 
    } 
    window.connection.SetOnReceive(Prompt, window);

In Beip I use the matacharoo 
    <(?:\d*\/\d*)hp (?:\d*)m (?:\d*)mv (?:-\d*)xp> 
as a regular expression to detect this prompt and spawn it on a new window placed below the input window.

If you look at the string as provided by the network debugger however, it looks like so:
    ESC [0m< ESC [1;37m1824 ESC [0m/ ESC [1;37m1824 ESC [0mhp ESC [1;37m350 ESC [0mm ESC [1;37m406 ESC [0mmv -25542xp>

Basically a there are two types of ANSI color codes here which are inflating the string and causing it to not look right.  There is 
    ESC [0m 
 and
    ESC [1;37m
ANSI sequences start with ESC and a [ bracket, and can have up to three arguments, which are numbers separated by semicolons, and finally ending with a letter m.  It took me a while to hash this out, but it looks like the following can strip out ANSI sequences.

## Script to remove ANSI sequences from received text.

    function Prompt(string, window) 
    { 
    var workstring ="";
    workstring = string.replace(/(\033)[([0-9;])*m/g,"");
     window.output.write(workstring);
    } 
    window.connection.SetOnReceive(Prompt, window);

Before:
    [0m<[1;37m1824[0m/[1;37m1824[0mhp [1;37m350[0mm [1;37m406[0mmv -25542xp>
After:
    <1824/1824hp 350m 406mv -25542xp>


## Script to detect the presence of data for processing. (Did we receive a prmopt?)

Once the ANSI sequences are removed, you can use the matcharoo from Beip's trigger to detect the presence of the MUD's health prompt.  Cleaning up the string and detecting the right text format should be done before processing any of the text, so you don't have problems activating the script on a false positive.

    /@
    var hp = 0;
    var maxhp = 0;
    var mana = 0;
    var move = 0;
    var xp = 0;

    function Prompt(string, window) 
    { 
    var workstring ="";
    workstring = string.replace(/(\033)\[([0-9;])*([0-9])*m/g,"");

    if (workstring.search(/<(?:\d*\/\d*)hp (?:\d*)m (?:\d*)mv (?:-\d*)xp>/) >-1)
      { 
        window.output.write ("Prompt Detected"); 
      } 
        else 
      { 
        window.output.write("Prompt Not Found");
      }
      } 
      window.connection.SetOnReceive(Prompt, window);


## Script to parse prompt data.

There are several ways to parse the prompt.  In this example I used the substring method as before, but because the prompt is not a fixed length as the values change, I searched the string for the start and ending characters around each health value, then applied an offset (the size of the search string) to trim out just the number portion.

    <1824/1824hp 350m 406mv -25542xp>
    ^    ^    ^     ^    ^  ^     ^ --string.search() areas

    /@
    var hp = 0;
    var maxhp = 0;
    var mana = 0;
    var mmove = 0;
    var xp = 0;

    function Prompt(string, window) 
    { 
    var work = string.replace(/(\033)\[([0-9;])*([0-9])*m/g,"");

    if (work.search(/<(?:\d*\/\d*)hp (?:\d*)m (?:\d*)mv (?:-\d*)xp>/) >-1)
      { 
        hp = parseInt(work.substring(work.search("<")+1,work.search("/")));
        maxhp = parseInt(work.substring(work.search("/")+1,work.search("hp")));
        mana = parseInt(work.substring(work.search("hp ")+3,work.search("m ")));
        mmove = parseInt(work.substring(work.search("m ")+2,work.search("mv ")));
        xp = parseInt(work.substring(work.search(" -"),work.search("xp>")));
        window.output.write(hp + " " + maxhp + " " + mana + " " + mmove + " " + xp);
      } 
      } 
      window.connection.SetOnReceive(Prompt, window);

Output for this is:
    1824 1824 350 406 -25542

## Script to provide a health tracker.

Adding a few more variables to track the player's last hitpoint status, and some checks to see if it raises or lowers, you can start building a combat results script that tells the user how much healing and damage they have taken over time.  The two new commands @@report and @@reset are used to control the time frame checked and provide the report.

    /@
    var hp = 0;
    var lasthp =0;
    var damage =0;
    var healing =0;
    var maxhp = 0;
    var mana = 0;
    var move = 0;
    var xp = 0;

    function Prompt(string, window) 
    { 
    var work = string.replace(/(\033)\[([0-9;])*([0-9])*m/g,"");

    if (work.search(/<(?:\d*\/\d*)hp (?:\d*)m (?:\d*)mv (?:-\d*)xp>/) >-1)
      { 
        hp = parseInt(work.substring(work.search("<")+1,work.search("/")));
        maxhp = parseInt(work.substring(work.search("/")+1,work.search("hp")));
        mana = parseInt(work.substring(work.search("hp ")+3,work.search("m ")));
        move = parseInt(work.substring(work.search("m ")+2,work.search("mv ")));
        xp = parseInt(work.substring(work.search(" -"),work.search("xp>")));
        if (lasthp== 0) { lasthp=hp; }
        if (hp > lasthp)
          {
            healing += (hp - lasthp);
            window.output.write("Healed for " + (hp - lasthp) + "hp");
            lasthp =hp;
          } else 
          {
            if (lasthp < hp)
            {
            damage += (lasthp - hp);
            window.output.write( "Damaged for " + (lasthp - hp) + "hp");
        lasthp = hp;
            }
          }
      } 
      } 
      window.connection.SetOnReceive(Prompt, window);

    function Display(string,window)
    {
    if (string.search("@@report")==0) 
      { 
        window.output.write("Healing taken: " + healing); 
        window.output.write("Damage taken: " + damage);
        return true;
      }
    else 
      {
      if (string.search("@@reset")==0) 
      { 
        healing=0;
        damage=0;
        window.output.write("Health tracker reset.");
        return true;
      }
      }
    return false;
    }
    window.connection.SetOnSend(Display, window);


# Script example of making a bar view of two data points.

If you have two values and would like to create a bar view of them, you could use the following script example:

    /@ 
    var hp = 100;
    var maxhp = 200;

    function hpbar(width)
    {
    var output="";
    var filledchars =0;
    filledchars = parseInt(width*(hp/maxhp));
    for (i=0;i<width;i++)
    {
    if (i < filledchars) 
      { output = output + "H"; } 
    else 
      { output = output + " "; }
      }
      window.output.write ("[" + output + "]" + hp);
      }

Output is as follows:
    /@hpbar(30)
    [HHHHHHHHHHHHHHH               ]100

    /@ hp=45; hpbar(20);
    [HHHH                ]45

    /@ maxhp=1000; hpbar(20);
    [                    ]45
