param (
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$outputDirectory,
    
    [Parameter(Mandatory=$true)]
    [ValidateSet("Debug", "Release")]
    [string]$Configuration
)

function CompileNativeTools {
    try {
        Write-Output "[93mCompiling Tools Native ($configuration)...[0m"
        if (-not(Test-Path -Path $outputDirectory)) {
            mkdir $outputDirectory > $null
        }

        $outputDirectory = Resolve-Path $outputDirectory

        $currentDirectory = (Get-Item -Path .).BaseName
        Write-Output $currentDirectory
        
        if (-not($currentDirectory -eq "Elemental.Tools.Native")) {
            Push-Location ./../Elemental.Tools.Native
        }

        if (-not(Test-Path -Path ./obj)) {
            mkdir ./obj > $null
        }
            
        Push-Location ./obj/

        if ($IsMacOS) {
            # TODO: Release mode
            # TODO: Define Debug
            clang -Wall -std=c++20 -lc++ -fms-extensions -g -liconv -lspirv-cross-core -lspirv-cross-cpp -lspirv-cross-msl -lspirv-cross-glsl -lspirv-cross-reflect -lspirv-cross-util -L../../../external/shader-compilers/spirv-cross/lib/ -dynamiclib -I../../Platforms/Common/ -I../Interop/ -I../../../external/DirectX-Headers/include/directx/ -I../../../external/shader-compilers/dxc/include/dxc/ -I../../../external/shader-compilers/spirv-cross/include/ -o $outputDirectory/Elemental.Tools.Native.dylib ../UnityBuild.cpp
        } else {
            RegisterVisualStudioEnvironment

            if ($Configuration -eq "Debug") {
                cl.exe /MD /Wall /nologo /D_DEBUG /DUNICODE /D_UNICODE /D_WINDOWS /D_USRDLL /D_WINDLL /std:c++20 -I../../Platforms/Common -I../../../external/shader-compilers/dxc/inc/dxc/ -I../../../external/shader-compilers/spirv-cross/include/ /Zi /EHs "../UnityBuild.cpp" /link /DLL /OUT:$outputDirectory/Elemental.Tools.Native.dll /SUBSYSTEM:WINDOWS /DEBUG /MAP /OPT:ref /INCREMENTAL:NO /WINMD:NO /NOLOGO /LIBPATH:../../../external/shader-compilers/spirv-cross/lib/lib/ uuid.lib kernel32.lib user32.lib gdi32.lib ole32.lib advapi32.lib Winmm.lib spirv-cross-msl.lib spirv-cross-glsl.lib spirv-cross-util.lib spirv-cross-core.lib spirv-cross-reflect.lib spirv-cross-cpp.lib
            } else {
                cl.exe /MD /Wall /nologo /DUNICODE /D_UNICODE /D_WINDOWS /D_USRDLL /D_WINDLL /std:c++20 -I../../Platforms/Common -I../../../external/shader-compilers/dxc/inc/dxc/ -I../../../external/shader-compilers/spirv-cross/include/ /O2 /Zi /EHs "../UnityBuild.cpp" /link /DLL /OUT:$outputDirectory/Elemental.Tools.Native.dll /SUBSYSTEM:WINDOWS /MAP /OPT:ref /INCREMENTAL:NO /WINMD:NO /NOLOGO /LIBPATH:../../../external/shader-compilers/spirv-cross/lib/lib/ uuid.lib kernel32.lib user32.lib gdi32.lib ole32.lib advapi32.lib Winmm.lib spirv-cross-msl.lib spirv-cross-glsl.lib spirv-cross-util.lib spirv-cross-core.lib spirv-cross-reflect.lib spirv-cross-cpp.lib
            }
        }
        Write-Output "[92mTools Native Compiled Successfully![0m"
    } finally
    {
        Pop-Location
        Pop-Location
    }
}


# TODO: Use cmake?

function RegisterVisualStudioEnvironment {
    $registeredVisualStudioVersion = Get-Content -Path Env:VisualStudioVersion -ErrorAction SilentlyContinue
    Write-Output $registeredVisualStudioVersion

    if (-not($registeredVisualStudioVersion -eq "17.0") -And -not($registeredVisualStudioVersion -eq "17.5")) {
        Write-Output "[93mRegistering Visual Studio Environment...[0m"

        $vsPath = Get-VSPath
        
        if ($vsPath) {
            Write-Output "[92mFound Visual Studio at $vsPath.[0m"
            $batchCommand = "`"$vsPath`" > nul & set"

            cmd /c $batchCommand | Foreach-Object {
                $p, $v = $_.split('=')
                Set-Item -path env:$p -value $v
            }
        } else {
            Write-Warning "[91mCould not find a supported version of Visual Studio. Environment not registered.[0m"
        }
    }
}

function Get-VSPath {
    $vsVersions = @{
        '2022Ent'      = 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat'
        '2022Preview'  = 'C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat'
    }

    foreach ($vsVersion in $vsVersions.Keys) {
        $vsPath = $vsVersions[$vsVersion]
        if (Test-Path -Path $vsPath) {
            return $vsPath
        }
    }

    return $null
}

CompileNativeTools