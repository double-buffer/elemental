function Get-GithubRelease {
    param (
        $repo,
        $filenamePattern,
        $pathExtract
    )

    $releasesUri = "https://api.github.com/repos/$repo/releases/latest"
    $downloadUri = ((Invoke-RestMethod -Method GET -Uri $releasesUri).assets | Where-Object name -like $filenamePattern ).browser_download_url

    $pathZip = Join-Path -Path $([System.IO.Path]::GetTempPath()) -ChildPath $(Split-Path -Path $downloadUri -Leaf)

    Invoke-WebRequest -Uri $downloadUri -Out $pathZip

    Remove-Item -Path $pathExtract -Recurse -Force -ErrorAction SilentlyContinue
    Expand-Archive -Path $pathZip -DestinationPath $pathExtract -Force
    Remove-Item $pathZip -Force
}

if (-not(Test-Path -Path "$PSScriptRoot/shader-compilers")) {
    mkdir $PSScriptRoot/shader-compilers > $null
}

Write-Output "[93mDownloading DirectX Shader Compiler...[0m"
Get-GithubRelease -repo "microsoft/DirectXShaderCompiler" -filenamePattern "dxc_*.zip" -pathExtract "$PSScriptRoot\shader-compilers\dxc\"