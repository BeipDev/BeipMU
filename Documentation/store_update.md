# Why is the store app stuck on build 312?

The Microsoft Store rejected the latest update!

`The app is assigned to a game category but doesn’t appear to have game functionality. Please create a new app in an app category.`

So, time to get serious. **This is no longer a game!**

Unfortunately this required creating a new non game app as there is no way to switch from game to app, so the following instructions are how to seamlessly do a one time switch.

This will also have the benefit of no longer having the Xbox game bar show up or say that you're "Playing BeipMU" in any Xbox gaming apps (this affected some people).

For anyone who left a review on the previous version, please feel free to copy/paste it onto the new one!

## In your original BeipMU:

1. Launch app
2. 'File->Export Configuration...' to save all current settings
   * Note: Restore logs aren't exported
4. Optionally close BeipMU (can leave it open, just might get confusing)

## Now to install the new non-game BeipMU:

1. https://www.microsoft.com/store/apps/9NFS86LKJLRX
2. Launch new BeipMU
   * The new one will say it's version 4.00.314 (if it launches with all the settings, it's likely the original one)
3. Do 'File->Import Configuration...' and choose the file saved earlier.
   * When first closing it'll warn you about the original config, this is expected as you just replaced it.
   * Dark mode will not be applied until the app is closed and restarted.
6. Optionally uninstall old BeipMU
   * Be careful! In Apps & Features be sure to uninstall the older BeipMU as they look identical
