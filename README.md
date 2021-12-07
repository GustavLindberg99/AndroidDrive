# AndroidDrive
AndroidDrive is a program that allows mounting Android smartphones as drives on Windows.

<img src="https://i.stack.imgur.com/Y1f9d.png" width="350"/> <img src="https://i.stack.imgur.com/lyJzD.png" width="350"/>


# Setup
To install AndroidDrive, download and extract [AndroidDrive.zip](https://github.com/Gustav-Lindberg/AndroidDrive/raw/main/AndroidDrive.zip). If you want to start AndroidDrive when Windows starts, create a shortcut to AndroidDrive.exe in `%appdata%\Microsoft\Windows\Start Menu\Programs\Startup`.

For AndroidDrive to be able to detect and interact with your Android device, you need to enable USB debugging on your Android device. To do so, follow these steps:

1. Open Settings on your Android device.
2. Go to "About phone".
3. Scroll down to the bottom and tap "Build Number" seven times until you get a message saying that you're a developer.
4. Go back to the main settings screen, then go to System > Advanced > Developer Settings and enable USB debugging.

# Usage
When AndroidDrive is running and you connect an Android device, AndroidDrive will automatically mount a drive containing that device's internal storage. It may take some time for AndroidDrive to mount the drive. How long it takes depends on how many files you have on your Android device and how fast your computer is, but it usually takes about 15 seconds (though it may take longer if you for example have a lot of photos).

If you *don't* want AndroidDrive to mound a drive for a specific Android device, you can right click on the AndroidDrive icon in the task bar, go to Devices and click "Disconnect drive". If you disconnect a drive like this, it won't be connected again until you connect it manually (by going through the same process and clicking "Connect drive").

By default, AndroidDrive drives will have the same name as your hard drive (usually "Local disk"). However, you can easily rename drives in "This PC" in Windows Explorer. If you rename a drive like this, the new name will be saved.

When AndroidDrive detects a new Android device, it will automatically be assigned the first available drive letter after C (for example if your only drive is the hard drive, this will mean that it will be assigned the letter D). You can change the drive letter assigned to a specific Android device by right clicking on the AndroidDrive icon in the task bar, going to Devices and clicking "Select drive letter". If you do this to a drive that's already connected, it will be disconnected and then reconnected with the new drive letter.


# Credits

Icons from https://www.iconfinder.com/ are made by [Alpár-Etele Méder](https://www.iconfinder.com/pocike) and [Tango](https://www.iconfinder.com/iconsets/tango-icon-library).

This program uses [ADB](https://android.googlesource.com/platform/packages/modules/adb/) by Google and [AdbSync](http://www.temblast.com/adbsync.htm) by temblast.com.
