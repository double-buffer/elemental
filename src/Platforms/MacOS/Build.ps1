param ($outputDirectory)

try
{
    $outputDirectory = Resolve-Path $outputDirectory
    Write-Output "[93mCompiling MacOS Platform Library...[0m"

    Push-Location ../Platforms/MacOS

    mkdir obj | Out-Null
    swiftc *.swift -wmo -emit-library -module-name "ElementalPlatformNative" -O -g -o "obj/Elemental.Native.dylib" -I "." -debug-info-format=dwarf -swift-version 5 -target x86_64-apple-macosx13 -Xlinker -rpath -Xlinker "@executable_path/../Frameworks"

    if (-Not $?) {
        Pop-Location
        Exit 1
    }
    
    Copy-Item obj/* $outputDirectory -Recurse -Force
}

finally
{
    Pop-Location
}