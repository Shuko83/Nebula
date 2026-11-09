# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The installer of NebulaStudio (#140), as a user runs it: installed without a word, for the user who runs the script,
# in Folder, then NebulaStudio started from there, its window shown, then closed, then uninstalled. Run by release.yml
# once CPack built it, on a computer where it is not installed yet:
#   TestInstaller.ps1 -Installer <NebulaStudio_..._setup.exe> -Folder <folder to install it to, which must not exist>
# Writes what goes wrong, and exits with 1 when something does.
param(
    [Parameter(Mandatory)] [string] $Installer,
    [Parameter(Mandatory)] [string] $Folder
)

$ErrorActionPreference = 'Stop'
$Folder = [IO.Path]::GetFullPath($Folder)
# What it puts outside its folder: its shortcut, at the top of the Start menu, and its entry in Settings > Apps, its
# AppId (CPACK_PACKAGE_INSTALL_REGISTRY_KEY) followed by _is1.
$shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'NebulaStudio.lnk'
$entry = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\FabienDG.NebulaStudio_is1'
# Out of Folder, which the uninstaller removes.
$log = Join-Path ([IO.Path]::GetTempPath()) 'NebulaStudio-installed.log'

function Fail([string] $why) {
    Write-Host $why
    exit 1
}

# Waits up to seconds for condition to hold, checking it twice a second; tells whether it does.
function Wait-Until([scriptblock] $condition, [int] $seconds) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while (-not (& $condition)) {
        if ((Get-Date) -gt $deadline) { return $false }
        Start-Sleep -Milliseconds 500
    }
    return $true
}

function Install-Studio {
    $arguments = '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', '/CURRENTUSER', "/DIR=`"$Folder`""
    $setup = Start-Process -FilePath $Installer -ArgumentList $arguments -Wait -PassThru
    if ($setup.ExitCode -ne 0) { Fail "The installer ended with the code $($setup.ExitCode)" }
}

# What NebulaStudio needs to start, the shortcut, and the version Settings > Apps shows, the one of NebulaStudio.exe.
function Test-Installed {
    foreach ($file in 'NebulaStudio.exe', 'Core.dll', 'Application.dll', 'Qt6Widgets.dll', 'platforms\qwindows.dll',
        'plugins\StudioPlugin.nbdl', 'plugins\StandardPlugin.nbdl', 'scene\NebulaStudio.nbp', 'unins000.exe') {
        if (-not (Test-Path -LiteralPath (Join-Path $Folder $file))) { Fail "$file is not installed in $Folder" }
    }
    if (-not (Test-Path -LiteralPath $shortcut)) { Fail "The shortcut $shortcut is not created" }
    if (-not (Test-Path -LiteralPath $entry)) { Fail "Settings > Apps has no entry $entry" }
    $version = (Get-Item -LiteralPath (Join-Path $Folder 'NebulaStudio.exe')).VersionInfo.ProductVersion
    $shown = (Get-ItemProperty -LiteralPath $entry).DisplayVersion
    if ($shown -cne $version) { Fail "Settings > Apps shows the version '$shown', NebulaStudio.exe is '$version'" }
    Write-Host "NebulaStudio $version installed in $Folder"
}

# Its log, each message written with its type, empty when it wrote nothing.
function Get-Log {
    if (-not (Test-Path -LiteralPath $log)) { return '' }
    return [string](Get-Content -LiteralPath $log -Raw)
}

# Starts the NebulaStudio installed, each message written to its log with its type; returns its process.
function Start-Studio {
    Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
    $env:QT_MESSAGE_PATTERN = '%{type} %{if-category}%{category}: %{endif}%{message}'
    $executable = Join-Path $Folder 'NebulaStudio.exe'
    return Start-Process -FilePath $executable -ArgumentList '--log-file', "`"$log`"" -PassThru
}

# Fails, studio stopped, unless it shows a window and keeps running past its splash screen, read after two seconds,
# its log telling no failure: a galaxy that does not load or does not start has it show a box. Returns its title.
function Wait-Running($studio) {
    $shown = Wait-Until { $studio.Refresh(); $studio.HasExited -or $studio.MainWindowHandle -ne 0 } 60
    if ($shown -and -not $studio.HasExited) { Start-Sleep -Seconds 5 }
    $written = Get-Log
    if ($studio.HasExited) { Fail "NebulaStudio ended, with the code $($studio.ExitCode):`n$written" }
    if (-not $shown -or $written -match '(?m)^(critical|fatal) ') {
        Stop-Process -Id $studio.Id
        Fail "NebulaStudio did not show its window in 60 seconds, or did not run:`n$written"
    }
    $studio.Refresh()
    return $studio.MainWindowTitle
}

# Started, then closed as a user closes it: its window asked to close, which ends it.
function Test-Starts {
    $studio = Start-Studio
    $title = Wait-Running $studio
    $studio.CloseMainWindow() | Out-Null
    if (-not $studio.WaitForExit(30000)) { Stop-Process -Id $studio.Id; Fail 'NebulaStudio did not close in 30 seconds' }
    if ($studio.ExitCode -ne 0) { Fail "NebulaStudio closed with the code $($studio.ExitCode)" }
    Write-Host "NebulaStudio started, showed '$title', and closed"
}

# The uninstaller runs a copy of itself, which removes it: the folder is gone once it is done.
function Uninstall-Studio {
    $arguments = '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
    Start-Process -FilePath (Join-Path $Folder 'unins000.exe') -ArgumentList $arguments -Wait
    if (-not (Wait-Until { -not (Test-Path -LiteralPath $Folder) } 60)) {
        Fail "The uninstaller left in ${Folder}:`n$((Get-ChildItem -LiteralPath $Folder -Recurse -Name) -join "`n")"
    }
    if (Test-Path -LiteralPath $shortcut) { Fail "The uninstaller left the shortcut $shortcut" }
    if (Test-Path -LiteralPath $entry) { Fail "The uninstaller left the entry $entry" }
    Write-Host 'NebulaStudio uninstalled'
}

if (Test-Path -LiteralPath $Folder) { Fail "$Folder exists already: the installer would not be the only one to fill it" }
Install-Studio
Test-Installed
Test-Starts
Uninstall-Studio
