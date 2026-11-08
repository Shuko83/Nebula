# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The details of a binary of Nebula (#139), from its version resource, run by ctest:
#   TestVersionDetails.ps1 -Binary <file> -Version <version> -Number <a.b.c.d> -Description <text>
# Writes what differs, and exits with 1 when something does.
param(
    [Parameter(Mandatory)] [string] $Binary,
    [Parameter(Mandatory)] [string] $Version,
    [Parameter(Mandatory)] [string] $Number,
    [Parameter(Mandatory)] [string] $Description
)

$info = (Get-Item -LiteralPath $Binary).VersionInfo
$found = '{0}.{1}.{2}.{3}' -f $info.FileMajorPart, $info.FileMinorPart, $info.FileBuildPart, $info.FilePrivatePart
$checks = @(
    @('File version', $info.FileVersion, $Version),
    @('Product version', $info.ProductVersion, $Version),
    @('Numeric version', $found, $Number),
    @('Description', $info.FileDescription, $Description)
)

$failed = $false
foreach ($check in $checks) {
    if ($check[1] -cne $check[2]) {
        Write-Host "$($check[0]) of ${Binary}: '$($check[1])', expected '$($check[2])'"
        $failed = $true
    }
}
if ($failed) { exit 1 }
Write-Host "${Binary}: $($info.FileDescription), $($info.FileVersion)"
