# AndroidDrive
AndroidDrive is a program that allows mounting Android smartphones as drives on Windows.

<img width="394" src="https://github.com/GustavLindberg99/AndroidDrive/assets/95423695/f7e16581-2ab0-4353-ac31-b0fd722f9048">
<img width="394" src="https://github.com/GustavLindberg99/AndroidDrive/assets/95423695/e6d81783-8c3c-4dd5-bf53-ca5d0bb87c05">
<img width="165" src="https://github.com/GustavLindberg99/AndroidDrive/assets/95423695/7aa8a35b-53b5-40a2-b843-de569eaa7363">


# Setup
To be able to use AndroidDrive, you need to do three things (the order in which you do them doesn't matter):
- **Install AndroidDrive on your Windows computer**: Either use the [installation program](https://raw.githubusercontent.com/GustavLindberg99/AndroidDrive/main/AndroidDrive-install.exe), or download and extract the [zip file](https://raw.githubusercontent.com/GustavLindberg99/AndroidDrive/main/AndroidDrive-portable.zip). If you want to start AndroidDrive when Windows starts, create a shortcut to AndroidDrive in `%appdata%\Microsoft\Windows\Start Menu\Programs\Startup`.

- **Install Dokan on your Windows computer**: The easiest way is to use their [installation program](https://github.com/dokan-dev/dokany/releases/download/v2.0.6.1000/DokanSetup.exe) (this is the installation program for Dokan 2.0.6, which is the version AndroidDrive was tested on). If you prefer, you can find other ways to install Dokan in [their documentation](https://github.com/dokan-dev/dokany/wiki/Installation).

- **Enable USB debugging on your Android device**: For AndroidDrive to be able to detect and interact with your Android device, you need to enable USB debugging. To do so, follow these steps:

  1. Open Settings on your Android device.
  2. Go to "About phone".
  3. Scroll down to the bottom and tap "Build Number" seven times until you get a message saying that you're a developer.
  4. Go back to the main settings screen, then go to System > Advanced > Developer Settings and enable USB debugging.

# Usage
When AndroidDrive is running and you connect an Android device, AndroidDrive will automatically mount a drive containing that device's internal storage.

If you *don't* want AndroidDrive to mound a drive for a specific Android device, you can right click on the AndroidDrive icon in the task bar, go to Devices and click "Device settings", then uncheck "Automatically connect drive". To actually disconnect it, you also need to click "Disconnect drive" in the Devices window. You can also temporarily disconnect a drive by clicking "Disconnect drive" without changing the device settings.

When AndroidDrive detects a new Android device, it will automatically be assigned the first available drive letter after C (for example if your only drive is the hard drive, this will mean that it will be assigned the letter D). You can change the drive letter assigned to a specific Android device by right clicking on the AndroidDrive icon in the task bar, going to Devices, clicking "Device settings" and selecting a drive letter under "Drive letter". If you do this to a drive that's already connected, you will need to disconnect and re-connect the drive for the changes to take effect.


# Languages

AndroidDrive is currently available in the following languages:

* English
* French
* Hungarian (translation by [gidano](https://github.com/gidano))
* Italian (translation by [bovirus](https://github.com/bovirus))
* Swedish

If your language is not listed above and you would like to help translate it, you can find instructions for how to do that [here](https://github.com/GustavLindberg99/AndroidDrive/blob/main/sources/translations/contribute.md).


# Compatibility

AndroidDrive works on any 64-bit computer with Windows 10 or later.

Windows 7 is no longer supported since upgrading to Qt 6, but if you want it to work on Windows 7, you can download an older version of AndroidDrive (version 2.0.6) as an installer [here](https://github.com/GustavLindberg99/AndroidDrive/raw/a36e464a665bafd11866644507b5e900ef8c0e90/AndroidDrive-setup.exe) or a ZIP file [here](https://github.com/GustavLindberg99/AndroidDrive/raw/a36e464a665bafd11866644507b5e900ef8c0e90/AndroidDrive.zip). Note that this is an old version of AndroidDrive, so it doesn't have the latest features and won't be updated. If you have Windows 10 or later, it's highly recommended that you instead use the latest version as described in the Setup section above.


# FAQ

## AndroidDrive is slow and/or uses a lot of CPU

What's happening is that Windows Explorer is waiting for the driver to respond, and the driver needs to make requests to ADB, which is slower than just reading a regular hard drive. So unfortunately there is no way to make it as fast as a hard drive.

There are already a few optimizations so that it isn't too slow, and if I find ways to optimize it more in the future I will do so. But there is no easy fix that I can do right now to make it faster.

## AndroidDrive isn't detecting my device

First of all, make sure that you enabled USB debugging as described above under "Setup" (by following all the steps 1-4, just enabling developer options isn't enough).

If you're still having issues, try running this in the command prompt:

```
cd "C:\Program Files\AndroidDrive"
adb.exe devices
```

If your device isn't listed in the command prompt after running this, that's a problem with ADB, not AndroidDrive. You may be able to find solutions [here](https://stackoverflow.com/q/21170392/4284627).

In the unlikely event that your device *is* listed in the command prompt when running the commands above but *not* in AndroidDrive, you can report that as a bug [here](https://github.com/GustavLindberg99/AndroidDrive/issues/new/choose).

## Windows Explorer gives an error that location is not available

This can happen if Android is asking for your permission to use USB debugging on that specific computer. If that's the case, when you unlock your Android device, it will show you a dialog asking if you want to allow this computer to access phone data. Tap "Allow". For convenience, you can also check the checkbox to always allow from this computer. If it still isn't working, try unplugging and then re-plugging your device.


# Credits

Icons from https://www.iconfinder.com/ are made by [Alpár-Etele Méder](https://www.iconfinder.com/pocike) and [Tango](https://www.iconfinder.com/iconsets/tango-icon-library).

This program uses [Qt](https://www.qt.io/), [ADB](https://android.googlesource.com/platform/packages/modules/adb/) and [Dokan](https://dokan-dev.github.io/).
