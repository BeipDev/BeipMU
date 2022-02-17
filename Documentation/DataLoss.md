Help! I started BeipMU and I lost my settings!

I've had a few reports of this, typically after the computer has a blue screen/hang/etc, so it's not due to a BeipMU bug that I'm aware of. But here's what options you have to recover some data.

# Simple solution

Use 'File->Load Backup Configuration' and hopefully the date on it will be recent enough to not lose much.

# What are the configuration files?

The only settings BeipMU saves are these files:

* config.txt - All of the settings, this is the important file
* restore.dat - If using restore logs, this stores their data
* config.bak - On every upgrade, BeipMU copies config.txt to this file

# Where are the settings?

If you want to look at the files directly just to see what's there, here's where they are:

## Store version

C:\Users\(Username)\AppData\Local\Packages\38939Ratman.BeipMU_xsyr3b6qthb5g\LocalCache\Roaming\BeipMU

Replace (username) with your Windows username.

Why the weird folder? Store apps have the OS folders virtualized so that they're sandboxed from touching files that are not theirs. This also makes is easy for the OS to cleanly uninstall them as their files are all in this folder.
  
## Github version

%appdata%\BeipMU  

This only applies if you don't manually put your config.txt in the same folder as BeipMU.exe. If you did this, why are you reading this section?!
  
# How is config.txt saved? Couldn't this lose data?
  
When saving the config, there is never a time when config.txt can be lost. The steps for saving a new file are to first save new config to 'config.new', then in one single OS command to move this file onto the config.txt file.
