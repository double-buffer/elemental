param(
    [Parameter(Mandatory = $false)]
    [string]$BuildNumber = "local",

    [Parameter(Mandatory = $false)]
    [string]$VersionFile = "version.cmake",

    [Parameter(Mandatory = $false)]
    [string]$HeaderPath = "src/Elemental/Elemental.h"
)

$ErrorActionPreference = "Stop"

function Get-CMakeValue
{
    param(
        [string]$Content,
        [string]$Name
    )

    $pattern = '(?m)^\s*set\(\s*' + [Regex]::Escape($Name) + '\s+"(?<value>[^"]*)"\s*\)\s*$'
    $match = [Regex]::Match($Content, $pattern)

    if (!$match.Success)
    {
        throw "Unable to read $Name from $VersionFile."
    }

    return $match.Groups["value"].Value
}

$versionContent = Get-Content -Path $VersionFile -Raw
$major = Get-CMakeValue $versionContent "ELEM_VERSION_MAJOR"
$minor = Get-CMakeValue $versionContent "ELEM_VERSION_MINOR"
$patch = Get-CMakeValue $versionContent "ELEM_VERSION_PATCH"
$stage = Get-CMakeValue $versionContent "ELEM_VERSION_STAGE"

if ($major -notmatch '^\d+$' -or $minor -notmatch '^\d+$' -or $patch -notmatch '^\d+$')
{
    throw "Elemental semantic version components must be numeric."
}

if (![string]::IsNullOrWhiteSpace($stage) -and $stage -notmatch '^(dev|alpha|beta|rc)$')
{
    throw "Unsupported Elemental release stage: $stage"
}

$productVersion = "$major.$minor.$patch"

if ([string]::IsNullOrWhiteSpace($stage))
{
    $version = $productVersion
}
else
{
    if ($BuildNumber -notmatch '^(local|\d+)$')
    {
        throw "Prerelease build number must be 'local' or numeric: $BuildNumber"
    }

    $version = "$productVersion-$stage.$BuildNumber"
}

$header = Get-Content -Path $HeaderPath -Raw
$versionCommentPattern = '(?m)^// Version: .*$'
$versionMacroPattern = '(?m)^#define ELEM_VERSION_LABEL ".*"$'

if ([Regex]::Matches($header, $versionCommentPattern).Count -ne 1)
{
    throw "Expected exactly one version comment in $HeaderPath."
}

if ([Regex]::Matches($header, $versionMacroPattern).Count -ne 1)
{
    throw "Expected exactly one ELEM_VERSION_LABEL definition in $HeaderPath."
}

$header = [Regex]::Replace($header, $versionCommentPattern, "// Version: $version")
$header = [Regex]::Replace($header, $versionMacroPattern, "#define ELEM_VERSION_LABEL `"$version`"")

[System.IO.File]::WriteAllText(
    $HeaderPath,
    $header,
    [System.Text.UTF8Encoding]::new($false)
)

Write-Host "Elemental build version: $version"
