# How to install on MacOS using Wineskin

## Install Fonts

If you don't already have it, find and download the Segoe UI and the Segoe UI Emoji ttf files. Any friend with a Windows PC can give these to you. I don't know a guaranteed safe place for it.

These should be manually dragged to the Mac (Or Macintosh HD)/Library/Font folder and dropped there. That should be all it takes to activate them. It seems to be more reliable than dragging it into the user one.

## Install Homebrew

Install Homebrew: https://brew.sh/

Click on the link and the rest is self-explanatory. You'll need to paste the command string it gives you on that page into your MacOS Terminal shell prompt.

## Install Wineskin

Install Wineskin Winery: https://github.com/Gcenx/WineskinServer
* You can paste this into your MacOS Terminal shell prompt:
 
`brew install --cask --no-quarantine gcenx/wine/unofficial-wineskin`

## Using Wineskin

* Download the latest x64 version of Beipmu: https://github.com/BeipDev/BeipMU/releases
  * Unzip it and put the folder in a convenient location. You'll need it later.
 
* Open Wineskin Winery. The first time you do this, you'll need to have it download the rest of what it requires to work. 
  * Locate and press the '+' button near where it says 'New Engines available'. Then select 'Download and Install', then 'Update'.
  * You should now have an engine that says something like 
 
If this step was done correctly, it'll now look something like this with both an Installed Engine and a Wrapper Version: 
![Sample](/images/Wineskin.jpeg)
 
* After that, you'll want to click 'Create New Blank Wrapper'. 
  * Name it 'beipmu' (it'll append .app to it)
  * Press ok. It'll take a few moments.
 
* When it finishes, a popup will appear. Select 'Show Wrapper in Finder'
  * Move it to an easily accessible location.
  * Click it and select 'Install Software'.
  * Select 'Move a Folder Inside'.
  * Find the unzipped Beipmu folder (like beipmu_323_x64 or whatever) and select it.
  * In the new 'Choose Executable' menu, select beipmu.exe. Press OK, then Quit.

# Running BeipMU

Click the beipmu.app wrapper again. It should now launch Beipmu in Wineskin Winery.

At this point, you'll definitely find out if you have the correct font file or not. There won't be any icons in the bottom left corner.

Even if there are icons, you might not see the burger menu (you can click on it, you just can't see the icon for it). Either way, hit the option key (which is the alt key equivalent for Macs), navigate to where you can see the Font button for the UI Interface, and then select another font, like Segoe UI. Or anything really. Beipmu defaults to Calibri, and that doesn't seem to be a font that comes with most Macs.

Everything else should work as expected!
