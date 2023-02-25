param ($outputDirectory, $configuration)

try
{
    Write-Output "[93mCompiling MacOS Platform Library ($configuration)...[0m"

    if (-not(Test-Path -Path $outputDirectory)) {
        mkdir $outputDirectory > $null
    }

    $outputDirectory = Resolve-Path $outputDirectory

    $currentDirectory = (Get-Item -Path .).BaseName
    Write-Output $currentDirectory

    if (-not($currentDirectory -eq "MacOS")) {
        Push-Location ./../Platforms/MacOS
    }

    mkdir obj | Out-Null
    mkdir obj/$configuration | Out-Null

    # Add support to universal binaries
    # https://stackoverflow.com/questions/71084674/hot-to-compile-a-swift-script-to-a-universal-binary
    if ($configuration -eq "Debug") {
        Write-Output "Debug"
        swiftc *.swift ./**/*.swift -wmo -emit-library -module-name "ElementalPlatformNative" -v -Onone -g -o "obj/Debug/Elemental.Native.dylib" -I "." -debug-info-format=dwarf -swift-version 5 -target x86_64-apple-macosx13 -Xlinker -rpath -Xlinker "@executable_path/../Frameworks"
    } else {
        Write-Output "Release"
        swiftc *.swift ./**/*.swift -wmo -emit-library -module-name "ElementalPlatformNative" -v -O -g -o "obj/Release/Elemental.Native.dylib" -I "." -debug-info-format=dwarf -swift-version 5 -target x86_64-apple-macosx13 -Xlinker -rpath -Xlinker "@executable_path/../Frameworks"
    }
    
    if (-Not $?) {
        Pop-Location
        Exit 1
    }
    
    Copy-Item obj/$configuration/* $outputDirectory -Recurse -Force
}

finally
{
    Pop-Location
}