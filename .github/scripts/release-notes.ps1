param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$Repository,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

if ($Version -notmatch '^(?<product>\d+\.\d+\.\d+)(?:-(?<stage>dev|alpha|beta|rc)\.(?<build>\d+))?$')
{
    throw "Unsupported release version: $Version"
}

$productVersion = $Matches["product"]
$stage = $Matches["stage"]

function Get-PreviousTag
{
    param(
        [string]$Revision
    )

    $tag = & git describe --tags --abbrev=0 $Revision 2>$null

    if ($LASTEXITCODE -ne 0)
    {
        return $null
    }

    return ($tag | Select-Object -First 1).Trim()
}

function Get-LatestStableTag
{
    $tags = @(
        & git tag --merged HEAD --sort=-version:refname --list "v*" |
            Where-Object { $_ -match '^v\d+\.\d+\.\d+$' }
    )

    if ($tags.Count -eq 0)
    {
        return $null
    }

    return $tags[0].Trim()
}

$previousTag = Get-PreviousTag "HEAD^"
$baseTag = $null
$rangeDescription = $null
$sameStageAsPreviousRelease = $false

if (![string]::IsNullOrWhiteSpace($stage) -and $null -ne $previousTag -and
    $previousTag -match '^v(?<product>\d+\.\d+\.\d+)-(?<stage>dev|alpha|beta|rc)\.\d+$')
{
    $sameStageAsPreviousRelease =
        $Matches["product"] -eq $productVersion -and
        $Matches["stage"] -eq $stage
}

if ($sameStageAsPreviousRelease)
{
    $baseTag = $previousTag
    $rangeDescription = "Includes changes since $baseTag."
}
else
{
    $baseTag = Get-LatestStableTag

    $rangeDescription = if ($null -eq $baseTag)
    {
        "Includes all changes in the $productVersion release cycle since the beginning of the project."
    }
    else
    {
        "Includes all changes in the $productVersion release cycle since $baseTag."
    }
}

$commitArguments = if ($null -eq $baseTag)
{
    @("rev-list", "--reverse", "HEAD")
}
else
{
    @("rev-list", "--reverse", "$baseTag..HEAD")
}

$commits = @(& git @commitArguments)

if ($LASTEXITCODE -ne 0)
{
    throw "Unable to determine commits for the release notes."
}

$pullRequests = @{}
$directCommits = [System.Collections.Generic.List[object]]::new()

foreach ($commit in $commits)
{
    $sha = $commit.Trim()

    if ([string]::IsNullOrWhiteSpace($sha))
    {
        continue
    }

    $json = (& gh api "repos/$Repository/commits/$sha/pulls" | Out-String)

    if ($LASTEXITCODE -ne 0)
    {
        throw "Unable to find pull requests associated with commit $sha."
    }

    $associatedPullRequests = @(
        $json | ConvertFrom-Json |
            Where-Object { $null -ne $_.merged_at -and $_.base.ref -eq "main" }
    )

    if ($associatedPullRequests.Count -gt 0)
    {
        foreach ($pullRequest in $associatedPullRequests)
        {
            $pullRequests[$pullRequest.number.ToString()] = $pullRequest
        }

        continue
    }

    $subject = (& git show -s --format=%s $sha | Out-String).Trim()
    $shortSha = (& git rev-parse --short $sha | Out-String).Trim()

    $directCommits.Add([pscustomobject]@{
        Sha = $shortSha
        Subject = $subject
    })
}

$orderedPullRequests = @(
    $pullRequests.Values |
        Sort-Object @{ Expression = { [DateTime]$_.merged_at } }, @{ Expression = { [int]$_.number } }
)

$releaseUrl = "https://github.com/$Repository/releases/tag/v$Version"
$notes = [System.Collections.Generic.List[string]]::new()
$notes.Add("# Elemental [**$Version**]($releaseUrl)")
$notes.Add("")
$notes.Add($rangeDescription)
$notes.Add("")

if ($orderedPullRequests.Count -gt 0)
{
    $notes.Add("## Pull requests")
    $notes.Add("")

    foreach ($pullRequest in $orderedPullRequests)
    {
        $notes.Add("### [#$($pullRequest.number)]($($pullRequest.html_url)) — $($pullRequest.title)")
        $notes.Add("")

        if ([string]::IsNullOrWhiteSpace($pullRequest.body))
        {
            $notes.Add("_No description provided._")
        }
        else
        {
            $notes.Add($pullRequest.body.Trim())
        }

        $notes.Add("")
    }
}

if ($directCommits.Count -gt 0)
{
    $notes.Add("## Direct commits")
    $notes.Add("")

    foreach ($commit in $directCommits)
    {
        $notes.Add("- ``$($commit.Sha)`` $($commit.Subject)")
    }

    $notes.Add("")
}

if ($orderedPullRequests.Count -eq 0 -and $directCommits.Count -eq 0)
{
    $notes.Add("No changes were found for this release range.")
    $notes.Add("")
}

$parentDirectory = Split-Path -Parent $OutputPath

if (![string]::IsNullOrWhiteSpace($parentDirectory))
{
    New-Item -ItemType Directory -Path $parentDirectory -Force | Out-Null
}

$notes | Set-Content -Path $OutputPath -Encoding utf8
