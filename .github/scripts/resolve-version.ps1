param(
    [Parameter(Mandatory = $false)]
    [string]$VersionFile = "version.cmake"
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

$tags = @(& git tag --merged HEAD --list "v*")

if ($LASTEXITCODE -ne 0)
{
    throw "Unable to inspect existing Elemental release tags."
}

$productVersion = "$major.$minor.$patch"
$isPrerelease = ![string]::IsNullOrWhiteSpace($stage)

if ($isPrerelease)
{
    $buildNumbers = @(
        $tags |
            ForEach-Object {
                if ($_ -match '^v\d+\.\d+\.\d+-(?:dev|alpha|beta|rc)\.(?<build>\d+)$')
                {
                    [int64]$Matches["build"]
                }
            }
    )

    $buildNumber = if ($buildNumbers.Count -eq 0)
    {
        1
    }
    else
    {
        ([int64]($buildNumbers | Measure-Object -Maximum).Maximum) + 1
    }

    $version = "$productVersion-$stage.$buildNumber"
}
else
{
    $buildNumber = 0
    $version = $productVersion

    $stableNotesPath = ".github/release-notes/$version.md"

    if (!(Test-Path -Path $stableNotesPath))
    {
        throw "Stable release $version requires editorial notes at $stableNotesPath."
    }
}

$tag = "v$version"

if ($tags -contains $tag)
{
    throw "Release identity already exists: $tag"
}

if ([string]::IsNullOrWhiteSpace($env:GITHUB_OUTPUT))
{
    Write-Host "version=$version"
    Write-Host "tag=$tag"
    Write-Host "build_number=$buildNumber"
    Write-Host "prerelease=$($isPrerelease.ToString().ToLowerInvariant())"
}
else
{
    "version=$version" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
    "tag=$tag" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
    "build_number=$buildNumber" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
    "prerelease=$($isPrerelease.ToString().ToLowerInvariant())" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
}
