# Define the Get-GithubRelease function to download a release asset from GitHub
function Get-GithubRelease {
    param (
        [string]$repo,
        [string]$tag,
        [string]$filenamePattern,
        [string]$pathExtract
    )

    # Set the authorization headers if the GITHUB_TOKEN environment variable is defined
    if ($env:GITHUB_TOKEN) {
        $headers = @{ Authorization = "Bearer $($env:GITHUB_TOKEN)" }
    } else {
        $headers = @{ }
    }
    
    # Get the download URL of the asset with the given filename pattern from the specified GitHub repository and tag
    $releasesUri = "https://api.github.com/repos/$repo/releases/tags/$tag"
    $downloadUri = ((Invoke-RestMethod -Method GET -Uri $releasesUri -Headers $headers).assets | Where-Object name -like $filenamePattern ).browser_download_url

    # Download the asset to a temporary zip file
    $pathZip = Join-Path -Path $([System.IO.Path]::GetTempPath()) -ChildPath $(Split-Path -Path $downloadUri -Leaf)
    Invoke-WebRequest -Uri $downloadUri -Out $pathZip

    # Delete the output directory if it already exists
    if (Test-Path -Path $pathExtract) {
        Remove-Item -Path $pathExtract -Recurse -Force -ErrorAction SilentlyContinue
    }

    # Create the output directory if it does not exist
    if (-not(Test-Path -Path $pathExtract)) {
        mkdir $pathExtract > $null
    }

    # Extract the downloaded zip file to the output directory
    Expand-Archive -Path $pathZip -DestinationPath $pathExtract -Force
    Remove-Item $pathZip -Force
}

# Create the output directory if it does not exist
if (-not(Test-Path -Path "$PSScriptRoot/shader-compilers")) {
    mkdir $PSScriptRoot/shader-compilers > $null
}

# Define the version of the shader compilers to download
$shaderCompilersBinTag = "v2023-03-27"

# Download the DirectX Shader Compiler and SPIRV-Cross Shader Compiler based on the operating system
if ($IsMacOS) {
    Write-Output "[93mDownloading DirectX Shader Compiler...[0m"
    Get-GithubRelease -repo "double-buffer/shader-compilers-bin" -tag $shaderCompilersBinTag -filenamePattern "macos_dxc_*.zip" -pathExtract "$PSScriptRoot/shader-compilers/dxc/"

    Write-Output "[93mDownloading SPIRV-Cross Shader Compiler...[0m"
    Get-GithubRelease -repo "double-buffer/shader-compilers-bin" -tag $shaderCompilersBinTag -filenamePattern "macos_spirv-cross_*_x64.zip" -pathExtract "$PSScriptRoot/shader-compilers/spirv-cross/"
} else {
    Write-Output "[93mDownloading DirectX Shader Compiler...[0m"
    Get-GithubRelease -repo "microsoft/DirectXShaderCompiler" -tag "v1.7.2212.1" -filenamePattern "dxc_*.zip" -pathExtract "$PSScriptRoot\shader-compilers\dxc\"

    Write-Output "[93mDownloading SPIRV-Cross Shader Compiler...[0m"
    Get-GithubRelease -repo "double-buffer/shader-compilers-bin" -tag $shaderCompilersBinTag -filenamePattern "windows_spirv-cross_*_x64.zip" -pathExtract "$PSScriptRoot\shader-compilers\spirv-cross\"
}