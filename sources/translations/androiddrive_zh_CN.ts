<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN" sourcelanguage="en_US">
<context>
    <name>QObject</name>
    <message>
        <location filename="../androiddrive.cpp" line="170"/>
        <source>Internal storage</source>
        <translation>内部存储</translation>
    </message>
    <message>
        <location filename="../androiddrive.cpp" line="173"/>
        <source>SD card %1</source>
        <translation>SD 卡 %1</translation>
    </message>
    <message>
        <location filename="../devicelistmodel.cpp" line="90"/>
        <source>Device</source>
        <translation>设备</translation>
    </message>
    <message>
        <location filename="../devicelistmodel.cpp" line="119"/>
        <source>Status</source>
        <translation>状态</translation>
    </message>
    <message>
        <location filename="../devicelistmodel.cpp" line="124"/>
        <source>Mounting...</source>
        <translation>挂载中...</translation>
    </message>
    <message>
        <location filename="../devicelistmodel.cpp" line="127"/>
        <source>Unmounting...</source>
        <translation>卸载中...</translation>
    </message>
    <message>
        <location filename="../devicelistmodel.cpp" line="130"/>
        <source>Mounted as %1</source>
        <translation>挂载为 %1</translation>
    </message>
    <message>
        <location filename="../devicelistmodel.cpp" line="134"/>
        <source>Not mounted</source>
        <translation>未挂载</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="16"/>
        <location filename="../devicelistwindow.cpp" line="168"/>
        <source>Mounts a drive containing the internal storage of the selected Android device, as well as a drive for each external SD card that the selected device has, if any.</source>
        <translation>挂载一个包含所选安卓设备内部存储的驱动器，并为所选安卓设备拥有的每一张外部 SD 卡挂载一个驱动器（如果有的话）。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="17"/>
        <source>Allows you to change the settings for this device, for example select a new drive letter or choose whether it should be mounted automatically.</source>
        <translation>允许你更改这台设备的设置，比如选择新的驱动器代号或选择它是否应自动挂载。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="18"/>
        <source>Opens the selected drive in Windows Explorer.</source>
        <translation>在 Windows 资源管理器中打开所选驱动器。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="20"/>
        <source>AndroidDrive - Devices</source>
        <translation>AndroidDrive - 设备</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="163"/>
        <source>&amp;Unmount all drives</source>
        <translation>卸载全部驱动器(&amp;U)</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="164"/>
        <source>Unmounts all drives corresponding to the selected Android device.&lt;br/&gt;&lt;br/&gt;This only unmounts the drives, the Android device itself will remain connected, so you will still be able to access it for example through ADB.</source>
        <translation>卸载对应所选安卓设备的全部驱动器。&lt;br/&gt;&lt;br/&gt;这只会卸载驱动器，安卓设备本身仍保持连接，因此你仍能通过 adb 等方式访问它。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="167"/>
        <source>&amp;Mount all drives</source>
        <translation>挂载全部驱动器(&amp;M)</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="173"/>
        <source>&amp;Unmount drive</source>
        <translation>卸载驱动器(&amp;U)</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="174"/>
        <source>Unmounts the selected drive.&lt;br/&gt;&lt;br/&gt;This only unmounts the drive, the Android device itself will remain connected, so you will still be able to access it for example through ADB.</source>
        <translation>卸载所选驱动器。&lt;br/&gt;&lt;br/&gt;这只会卸载驱动器，安卓设备仍保持连接，因此你仍可以通过 adb 等方式访问它。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="177"/>
        <location filename="../devicelistwindow.hpp" line="85"/>
        <source>&amp;Mount drive</source>
        <translation>挂载驱动器(&amp;M)</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="178"/>
        <source>Mounts a drive containing the selected internal storage or external SD card.</source>
        <translation>挂载包含所选内部存储或外部 SD 卡的驱动器。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="189"/>
        <source>Fatal error: Could not list Android devices.&lt;br/&gt;&lt;br/&gt;ADB exited with code %1.</source>
        <translation>致命错误：无法列举安卓设备。&lt;br/&gt;&lt;br/&gt;ADB 退出，代码 %1。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="220"/>
        <source>Device %1 is offline.&lt;br/&gt;&lt;br/&gt;Try unlocking the device, then unplugging it and re-plugging it.&lt;br/&gt;&lt;br/&gt;If this error persists, you may be able to find solutions &lt;a href=&quot;%2&quot;&gt;here&lt;/a&gt; (any adb commands mentioned there can be run in the command prompt after running &lt;code&gt;cd &quot;%3&quot;&lt;/code&gt;).</source>
        <translation>%1 设备处于离线状态。&lt;br/&gt;&lt;br/&gt;试着解锁设备，然后断开和计算机连接，之后重新连接。&lt;br/&gt;&lt;br/&gt;若这个错误持续，你也许能在 &lt;a href=&quot;%2&quot;&gt;此处&lt;/a&gt; 找到解决方法。（任何这里提到的 adb 命令都可以在运行 &lt;code&gt;cd &quot;%3&quot;&lt;/code&gt; 后在命令提示符中运行）。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="226"/>
        <source>Device %1 is unauthorized.&lt;br/&gt;&lt;br/&gt;Try unlocking your device. If it shows you a dialog asking if you want to allow this computer to access phone data, tap &quot;Allow&quot;. If it doesn&apos;t show that dialog, disable and re-enable USB debugging as explained &lt;a href=&quot;%2&quot;&gt;here&lt;/a&gt;.&lt;br/&gt;&lt;br/&gt;If it still isn&apos;t working, try unplugging and then re-plugging your device.</source>
        <translation>%1 设备未获授权。&lt;br/&gt;&lt;br/&gt;试着解锁你的设备。如果设备显示对话框，询问是否允许这台计算机访问手机数据，请轻按“允许”。如果手机未显示这个对话框，请按照 &lt;a href=&quot;%2&quot;&gt;此处&lt;/a&gt; 的解释停用并重新启用 USB 调试。.&lt;br/&gt;&lt;br/&gt;如果仍无法正常工作，请试着断开和计算机的连接并重新连接。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="253"/>
        <source>Could not create a drive with the given drive letter.</source>
        <translation>无法创建给定代号的驱动器。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="260"/>
        <source>Dokan doesn&apos;t seem to be installed.&lt;br/&gt;&lt;br/&gt;Would you like to install it now?</source>
        <translation>似乎未安装 Dokan。&lt;br/&gt;&lt;br/&gt;要立即安装它吗？</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="264"/>
        <source>Could not start the driver.</source>
        <translation>无法启动驱动器。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="268"/>
        <source>Could not assign a drive letter.&lt;br/&gt;&lt;br/&gt;Try changing the drive letter in Device Settings to an available drive letter.</source>
        <translation>无法分配驱动器代号。&lt;br/&gt;&lt;br/&gt;试着在设备设置中将驱动器代号更改为可用的代号。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="271"/>
        <source>Dokan version error.</source>
        <translation>Dokan 版本错误。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="274"/>
        <source>An unknown error occurred.</source>
        <translation>发生了未知错误。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="277"/>
        <source>Could not mount drive %1: %2</source>
        <translation>无法挂载驱动 %1：%2</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="293"/>
        <source>ADB timed out.</source>
        <translation>ADB 超时。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="296"/>
        <source>An error occurred when attempting to read from the ADB process.</source>
        <translation>尝试从 ADB 进程读取时发生了错误。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="299"/>
        <source>An error occurred when attempting to write to the ADB process.</source>
        <translation>尝试写入 ADB 进程时出错。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="302"/>
        <source>ADB failed to start.&lt;br/&gt;&lt;br/&gt;Either the adb.exe file is missing, or you may have insufficient permissions to invoke the program.</source>
        <translation>ADB 启动失败。&lt;br/&gt;&lt;br/&gt;要么缺少 adb.exe 文件，要么你没有足够权限来调用程序。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="305"/>
        <source>ADB crashed.</source>
        <translation>ADB 崩溃。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="308"/>
        <source>ADB encountered an unknown error.</source>
        <translation>ADB 遇到了未知错误。</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.cpp" line="311"/>
        <source>Fatal error: Could not list Android devices: %1</source>
        <translation>致命错误：无法列出安卓设备：%1</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.hpp" line="86"/>
        <source>Drive &amp;settings</source>
        <translation>驱动器设置(&amp;s)</translation>
    </message>
    <message>
        <location filename="../devicelistwindow.hpp" line="87"/>
        <source>&amp;Open in Explorer</source>
        <translation>在资源管理器中打开(&amp;O)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="70"/>
        <source>AndroidDrive is already running.&lt;br/&gt;&lt;br/&gt;If you&apos;re trying to restart AndroidDrive, you can close the existing process by right clicking on the AndroidDrive icon in the task bar and selecting Exit.</source>
        <translation>AndroidDrive 已经在运行。&lt;br/&gt;&lt;br/&gt;如果你正尝试重启它，可以右键单击任务栏的 AndroidDrive 图标并选择退出来关闭现有进程。</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="138"/>
        <source>&amp;Devices</source>
        <translation>设备(&amp;D)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="141"/>
        <source>&amp;Settings</source>
        <translation>设置(&amp;S)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="144"/>
        <location filename="../main.cpp" line="149"/>
        <source>Record Debug &amp;Logs</source>
        <translation>记录调试日志(&amp;L)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="153"/>
        <source>Finish Recording Debug &amp;Logs</source>
        <translation>完成记录调试日志(&amp;L)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="154"/>
        <source>Recording of debug logs has started.&lt;br/&gt;&lt;br/&gt;You will be able to find the log file in %1.&lt;br/&gt;&lt;br/&gt;If you&apos;re planning on attaching the log file to a bug report, keep in mind that the log file will contain the names of the files on your phone, so make sure that the filenames don&apos;t contain any sensitive information (The debug logs will only contain the file names, they won&apos;t contain the contents of any file).</source>
        <translation>调试日志记录已开始。&lt;br/&gt;&lt;br/&gt;你将能在 %1 找到日志文件。&lt;br/&gt;&lt;br/&gt;如果你正计划将日志文件附加到故障报告中，请记得日志文件包含手机上文件的名称，因此请确保文件名不包含任何敏感信息。（调试日志仅包含文件名，不含有任何文件的内容）。</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="157"/>
        <source>Failed to create log file.</source>
        <translation>创建日志文件失败。</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="162"/>
        <source>&amp;About AndroidDrive</source>
        <translation>关于 AndroidDrive(&amp;A)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="167"/>
        <source>About AndroidDrive</source>
        <translation>关于 AndroidDrive</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="168"/>
        <source>AndroidDrive version %1 by Gustav Lindberg.</source>
        <translation>Gustav Lindberg 开发的 AndroidDrive %1。</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="168"/>
        <source>Icons made by %3 and %4 from %1 are licensed by %2.</source>
        <translation>图标由来自 %1 的 %3 和 %4 创作，许可证为 %2。</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="168"/>
        <source>This program uses %1 and %2.</source>
        <translation>本程序使用 %1 和 %2。</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="172"/>
        <source>About &amp;Qt</source>
        <translation>关于 Qt(&amp;Q)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="177"/>
        <source>E&amp;xit</source>
        <translation>退出(&amp;X)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="20"/>
        <source>AndroidDrive - Settings</source>
        <translation>AndroidDrive - 设置</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="29"/>
        <source>Drive settings for %1</source>
        <translation>%1 的驱动器设置</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="34"/>
        <source>Allows you to select a preferred drive letter for the selected Android drive.&lt;br/&gt;&lt;br/&gt;If the preferred drive letter is unavailable when this drive is connected, it will use the next available drive letter in alphabetical order.&lt;br/&gt;&lt;br/&gt;If you change the drive letter while this drive is connected, you will have to unmount it and re-mount it again for the changes to take effect.</source>
        <translation>允许你选择所选安卓驱动器的首选驱动器代号。&lt;br/&gt;&lt;br/&gt;如果这个驱动器连接时，首选的驱动器代号不可用，那么它会使用字母表中下一个可用的字母作为驱动器代号。&lt;br/&gt;&lt;br/&gt;如果在这个驱动器连接时更改驱动器代号，你必须卸载它并重新挂载才能让更改生效。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="35"/>
        <source>Drive &amp;letter</source>
        <translation>驱动器代号(&amp;L)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="42"/>
        <source>Allows you to select a name for the selected Android drive.</source>
        <translation>允许你选择所选安卓设备的名称。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="45"/>
        <source>&amp;Reset</source>
        <translation>重置(&amp;R)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="51"/>
        <source>Resets the name of the drive to the default value.</source>
        <translation>重置驱动器名称为默认值。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="56"/>
        <source>Drive &amp;name</source>
        <translation>驱动器名(&amp;N)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="58"/>
        <source>Automatically mount &amp;drive</source>
        <translation>自动挂载驱动器(&amp;D)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="60"/>
        <source>If this checkbox is checked, the selected drive will be automatically connected as a drive whenever you plug it into your computer.&lt;br/&gt;&lt;br/&gt;Otherwise, you will have to mount it manually by going into Devices &gt; Mount drive.</source>
        <translation>如果勾选了这个单选框，那么无论何时你将所选驱动器插入计算机，它都会自动连接为驱动器。&lt;br/&gt;&lt;br/&gt;否则，你必须转到设备&gt;挂载驱动器来手动挂载。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="63"/>
        <source>These settings only affect the selected drive.</source>
        <translation>这些设置只影响所选驱动器。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="68"/>
        <source>Global settings</source>
        <translation>全局设置</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="71"/>
        <source>Open newly connected drives in &amp;Explorer</source>
        <translation>在资源管理器中打开新连接的驱动器(&amp;E)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="73"/>
        <source>If this checkbox is checked, whenever AndroidDrive is finished connecting a drive, it will open that drive in Windows Explorer.</source>
        <translation>如果勾选了这个单选框，无论何时 AndroidDrive 完成连接驱动器都会在 Windows 资源管理器中打开那个驱动器。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="76"/>
        <source>&amp;Hide files beginning with a dot</source>
        <translation>隐藏以英文.开头的文件(&amp;H)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="78"/>
        <source>If this checkbox is checked, files that begin with a dot will be treated as hidden files, and will only be visible in Windows Explorer if Windows Explorer&apos;s &quot;Show hidden files&quot; option is activated.</source>
        <translation>如果勾选了这个驱动器，那么以英文.开头的文件将被视作隐藏文件，且只有激活 Windows 资源管理器的“显示隐藏文件”选项才能看到这些文件。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="86"/>
        <source>Use system language</source>
        <translation>使用系统语言</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="92"/>
        <source>Allows you to select which language AndroidDrive&apos;s GUI will be displayed in.&lt;br/&gt;&lt;br/&gt;If you select &quot;Use system language&quot; but AndroidDrive isn&apos;t availiable in your system language, English will be used.&lt;br/&gt;&lt;br/&gt;This setting has no effect on how the actual drive works.&lt;br/&gt;&lt;br/&gt;You must restart AndroidDrive for this change to take effect.</source>
        <translation>允许选择用什么语言显示 AndroidDrive 的用户界面。&lt;br/&gt;&lt;br/&gt;如果你选择“使用系统语言”，但 AndroidDrive 没有你的系统语言，那么程序界面会以英语显示。&lt;br/&gt;&lt;br/&gt;此设置对实际驱动器如何工作没有影响。&lt;br/&gt;&lt;br/&gt;要让这个更改生效，你必须重启 AndroidDrive。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="93"/>
        <source>&amp;Language</source>
        <translation>语言(&amp;L)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="96"/>
        <source>AndroidDrive doesn&apos;t seem to be available in your system language.&lt;br/&gt;&lt;br/&gt;&lt;a %1&gt;Click here&lt;/a&gt; if you would like to help translate it.</source>
        <translation>AndroidDrive 似乎无法用你的系统语言显示。&lt;br/&gt;&lt;br/&gt;&lt;a %1&gt;单击此处&lt;/a&gt;如果你想帮忙翻译它。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="104"/>
        <source>These settings affect all drives connected with AndroidDrive.</source>
        <translation>这些设置影响所有和 AndroidDrive 连接的驱动器。</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="108"/>
        <source>&amp;OK</source>
        <translation>好(&amp;O)</translation>
    </message>
    <message>
        <location filename="../settingswindow.cpp" line="109"/>
        <source>&amp;Cancel</source>
        <translation>取消(&amp;C)</translation>
    </message>
    <message>
        <location filename="../settingswindow.hpp" line="66"/>
        <source>&amp;Apply</source>
        <translation>应用(&amp;A)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="113"/>
        <source>An update is available.&lt;br/&gt;&lt;br/&gt;Do you want to install it now?</source>
        <translation>有更新可用。&lt;br/&gt;&lt;br/&gt;要立即安装吗？</translation>
    </message>
</context>
</TS>
