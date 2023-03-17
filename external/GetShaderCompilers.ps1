function Get-GithubRelease {
    param (
        $repo,
        $tag,
        $filenamePattern,
        $pathExtract,
        $isTar
    )

    $releasesUri = "https://api.github.com/repos/$repo/releases/tags/$tag"
    $downloadUri = ((Invoke-RestMethod -Method GET -Uri $releasesUri).assets | Where-Object name -like $filenamePattern ).browser_download_url

    $pathZip = Join-Path -Path $([System.IO.Path]::GetTempPath()) -ChildPath $(Split-Path -Path $downloadUri -Leaf)

    Invoke-WebRequest -Uri $downloadUri -Out $pathZip

    Remove-Item -Path $pathExtract -Recurse -Force -ErrorAction SilentlyContinue
    
    if (-not($isTar)) {
        Expand-Archive -Path $pathZip -DestinationPath $pathExtract -Force
    } else {
        . ./Expand-Tar -FileToExtract $pathZip -TargetFolder $pathExtract
    }
    Remove-Item $pathZip -Force
}

if (-not(Test-Path -Path "$PSScriptRoot/shader-compilers")) {
    mkdir $PSScriptRoot/shader-compilers > $null
}

# TODO: Check the OS and use other patterns
Write-Output "[93mDownloading DirectX Shader Compiler...[0m"
Get-GithubRelease -repo "microsoft/DirectXShaderCompiler" -tag "v1.7.2212.1" -filenamePattern "dxc_*.zip" -pathExtract "$PSScriptRoot\shader-compilers\dxc\"

Write-Output "[93mDownloading SPIRV-Cross Shader Compiler...[0m"
Get-GithubRelease -repo "KhronosGroup/SPIRV-Cross" -tag "2021-01-15" -filenamePattern "spirv-cross-vs2017-64bit-*.gz" -isTar true -pathExtract "$PSScriptRoot\shader-compilers\spirv-cross\"