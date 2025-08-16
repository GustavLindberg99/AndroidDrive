$MyAppExeName       = "AndroidDrive.exe"
$MyAppExePath       = "..\..\build\release"
$MyWinDeployQtPath  = "..\..\build\release\windeployqt"
$MyAdbPath          = "..\..\build\release\_deps\adb-src\1.0.41\win64"
$MyAppOutputDir     = "..\..\build"
$MyAppOutputZip     = "AndroidDrive-portable.zip"

If (Test-Path "$MyAppOutputDir\$MyAppOutputZip") {
    Remove-Item "$MyAppOutputDir\$MyAppOutputZip"
}

Compress-Archive `
    -Path "$MyAppExePath\$MyAppExeName","$MyWinDeployQtPath\*","$MyAdbPath\*" `
    -DestinationPath "$MyAppOutputDir\$MyAppOutputZip" `
    -CompressionLevel Optimal
