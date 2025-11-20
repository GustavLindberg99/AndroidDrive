# Translating AndroidDrive

AndroidDrive is currently available in the following languages:

* English
* French
* Hungarian (translation by [gidano](https://github.com/gidano))
* Italian (translation by [bovirus](https://github.com/bovirus))
* Swedish

If your language is not listed above, you can help translate it. Do do so, follow the instructions below:

1. Install Qt Linguist. If you already have Qt installed, Qt Linguist is already installed with it, so you don't have to do anything. Otherwise, you can install Qt Linguist as a standalone program by downloading the [portable zip file](https://raw.githubusercontent.com/GustavLindberg99/QtLinguistWeb/main/qt-linguist-portable.zip) or the [installer](https://raw.githubusercontent.com/GustavLindberg99/QtLinguistWeb/main/qt-linguist-web-install.exe) (in the installer, when it asks you to select components, you only need Qt Linguist, the other stuff is for translating websites and isn't needed here). If you plan on developing your own programs with Qt, you can also download Qt as a whole [here](https://www.qt.io/download-qt-installer-oss).
2. Download [this file](https://raw.githubusercontent.com/GustavLindberg99/AndroidDrive/main/sources/translations/androiddrive_empty_translation.xlf) and save it as `androiddrive_<LANGUAGE CODE>.xlf`, where `<LANGUAGE CODE>` is the two-letter code of the language you're translating to. For example, this is `fr` for French, `it` for Italian, `sv` for Swedish, etc.
3. Open the file in Qt Linguist. You will get a dialog where you can select languages. Normally it auto-detects everything correctly and you just need to click OK:

   <img width="418" src="https://github.com/GustavLindberg99/AndroidDrive/assets/95423695/81674d06-1cf2-48d8-96e8-a81d114992b1">
   
   The source language (the one on the top) should always be English (United States). The target language (the one on the bottom) should be the language you're translating to. If you saved the file with the correct name, normally Qt Linguist should auto-detect the language and you don't need to do anything there either. If you want, though, you can select a country or region.

4. Next you need to do the actual translation. To do so, click on an English source text in "Strings" and translate it to your language in the text box below. Then click on the question mark to the left of the English source text, and it should turn into a green checkmark. The following screenshot is what it should look like when you're done translating the first two strings:

   <img width="960" src="https://github.com/GustavLindberg99/AndroidDrive/assets/95423695/9e9eb0f7-124f-4788-ae1e-5b1c52416112">

   Pay attention to the warnings on the bottom right under "Warnings" to make sure that your translation has the same puctuation, keyboard shortcuts (`&`) and placeholders (`%1`, `%2`, etc) as the original.

   If it says "File ... not available" on the top right under "Sources and Forms", don't worry about it, that's perfectly normal. You will still be able to translate it just as easily and your translation will still be useable.

6. When you're done translating, to publish your translation, you need to start by forking this repository by clicking [here](https://github.com/GustavLindberg99/AndroidDrive/fork). Then go to `https://github.com/<your username>/AndroidDrive/upload/main/sources/translations` and upload your translation.
7. Next create a pull request by going to `https://github.com/GustavLindberg99/AndroidDrive/compare/main...<your username>:AndroidDrive:main` and click Create pull request. Make sure to allow edits by maintainers so that I can make the necessary changes to the source code.
8. Now all you need to do is wait for me to approve your pull request and publish your translation.
