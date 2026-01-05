# AndroidDrive
AndroidDrive is a program that allows mounting Android smartphones as drives on Windows. It does not require rooting.

<img width="394" src="https://github.com/GustavLindberg99/AndroidDrive/assets/95423695/f7e16581-2ab0-4353-ac31-b0fd722f9048">
<img width="394" src="https://github.com/GustavLindberg99/AndroidDrive/assets/95423695/e6d81783-8c3c-4dd5-bf53-ca5d0bb87c05">
<img width="165" src="https://github.com/GustavLindberg99/AndroidDrive/assets/95423695/7aa8a35b-53b5-40a2-b843-de569eaa7363">


# Setup
To be able to use AndroidDrive, you need to do three things:

- **Enable USB debugging on your Android device**: For AndroidDrive to be able to detect and interact with your Android device, you need to enable USB debugging. To do so, follow these steps:

  1. Open Settings on your Android device.
  2. Go to "About phone".
  3. Scroll down to the bottom and tap "Build Number" seven times until you get a message saying that you're a developer (on some phones, "Build Number" might be under "Software Information").
  4. Go back to the main settings screen, then go to System > Advanced > Developer Settings and enable USB debugging.

- **Install Dokan on your Windows computer**: The easiest way is to use their [installation program](https://github.com/dokan-dev/dokany/releases/download/v2.0.6.1000/DokanSetup.exe) (this is the installation program for Dokan 2.0.6, which is the version AndroidDrive was tested on). If you prefer, you can find other ways to install Dokan in [their documentation](https://github.com/dokan-dev/dokany/wiki/Installation).

- **Install AndroidDrive on your Windows computer**: Download the latest version of AndroidDrive from the [releases page](https://github.com/GustavLindberg99/AndroidDrive/releases/). You can choose either an installer program (`AndroidDrive-install.exe`) or a portable version in a zip file (`AndroidDrive-portable.zip`). If you want to start AndroidDrive when Windows starts, create a shortcut to AndroidDrive in `%appdata%\Microsoft\Windows\Start Menu\Programs\Startup`.

# Usage
When AndroidDrive is running and you connect an Android device, AndroidDrive will automatically mount a drive containing that device's internal storage.

If you *don't* want AndroidDrive to mound a drive for a specific Android device, you can right click on the AndroidDrive icon in the task bar, go to Devices and click "Device settings", then uncheck "Automatically connect drive". To actually disconnect it, you also need to click "Disconnect drive" in the Devices window. You can also temporarily disconnect a drive by clicking "Disconnect drive" without changing the device settings.

When AndroidDrive detects a new Android device, it will automatically be assigned the first available drive letter after C (for example if your only drive is the hard drive, this will mean that it will be assigned the letter D). You can change the drive letter assigned to a specific Android device by right clicking on the AndroidDrive icon in the task bar, going to Devices, clicking "Device settings" and selecting a drive letter under "Drive letter". If you do this to a drive that's already connected, you will need to disconnect and re-connect the drive for the changes to take effect.


# Files app
I've also created a [Files app for Android](https://play.google.com/store/apps/details?id=io.github.gustavlindberg99.files) that you can install on your Android device. This app and AndroidDrive can be used independently (none of the two is necessary for the other one to work), but this app is made specifically to work well with AndroidDrive.

For example, you can change the icon of a folder in the app and the new icon will be visible on your computer on the drive connected with AndroidDrive. You can also create a shortcut (LNK file) on the Android drive on with your computer and use the shortcut on your phone.


# Languages

AndroidDrive is currently available in the following languages:

* Chinese (translation by [sr093906](https://github.com/sr093906))
* English
* French
* German (translation by [flaviusgh](https://github.com/flaviusgh))
* Hungarian (translation by [gidano](https://github.com/gidano))
* Italian (translation by [bovirus](https://github.com/bovirus))
* Japanese (translation by [reindex-ot](https://github.com/reindex-ot))
* Korean (translation by [VenusGirl](https://github.com/VenusGirl))
* Portuguese (Brazil) (translation by [igorruckert](https://github.com/igorruckert))
* Swedish

If your language is not listed above and you would like to help translate it, you can find instructions for how to do that [here](https://github.com/GustavLindberg99/AndroidDrive/blob/main/sources/translations/contribute.md).


# Compatibility

AndroidDrive works on any 64-bit computer with Windows 10 or later.

Windows 7 is no longer supported since upgrading to Qt 6, but if you want it to work on Windows 7, you can download an older version of AndroidDrive (version 2.0.6) [here](https://github.com/GustavLindberg99/AndroidDrive/releases/tag/2.0.6). Note that this is an old version of AndroidDrive, so it doesn't have the latest features and won't be updated. If you have Windows 10 or later, it's highly recommended that you instead use the latest version as described in the Setup section above.


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

If your device *is* listed in the command prompt when running the commands above but *not* in AndroidDrive, you can report that as a bug [here](https://github.com/GustavLindberg99/AndroidDrive/issues/new/choose). If you do, please include the output that you got when running `adb.exe devices`.

## Can I use AndroidDrive together with file recovery software to recover deleted files on my phone?

No, unfortunately that won't work. File recovery software works by reading parts of the disk that aren't currently assigned to any file. AndroidDrive doesn't have direct access to a disk, instead it receives requests to read from files from Dokan which it forwards to ADB through specific commands that can only read files that currently exist. So there's no way for AndroidDrive to access parts of the disk that aren't written to.

To recover permanently deleted files on your phone, you would probably need to root your phone and find a file recovery tool that uses the Linux command line (since Android is Linux-based). You can then access the Linux command line on your phone through ADB by running `adb.exe shell enter command here` in the Windows command line.


# Credits

Icons from https://www.iconfinder.com/ are made by [Alpár-Etele Méder](https://www.iconfinder.com/pocike) and [Tango](https://www.iconfinder.com/iconsets/tango-icon-library).

This program uses [Qt](https://www.qt.io/), [ADB](https://android.googlesource.com/platform/packages/modules/adb/) and [Dokan](https://dokan-dev.github.io/).
