param ($outputDirectory, $Configuration)

# TODO: Use cmake?

function RegisterVisualStudioEnvironment
{
    $registeredVisualStudioVersion = Get-Content -Path Env:VisualStudioVersion -ErrorAction SilentlyContinue
    Write-Output $registeredVisualStudioVersion

    if (-not($registeredVisualStudioVersion -eq "17.0") -And -not($registeredVisualStudioVersion -eq "17.5"))
    {
        Write-Output "[93mRegistering Visual Studio Environment...[0m"

        # TODO: Do something better here
        $vsPath = ""
        $vs2019ComPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
        $vs2019ProfPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
        $vs2019EntPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
        $vs2019PreviewPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"
        $vs2022PreviewPath = "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"
        
        if (Test-Path -Path $vs2019ComPath)
        {
            $vsPath = $vs2019ComPath
        }

        if (Test-Path -Path $vs2019ProfPath)
        {
            $vsPath = $vs2019ProfPath
        }

        if (Test-Path -Path $vs2019EntPath)
        {
            $vsPath = $vs2019EntPath
        }

        if (Test-Path -Path $vs2019PreviewPath)
        {
            $vsPath = $vs2019PreviewPath
        }

        if (Test-Path -Path $vs2022PreviewPath)
        {
            $vsPath = $vs2022PreviewPath
        }

        $batchCommand = "`"$vsPath`" > nul & set"

        cmd /c $batchCommand | Foreach-Object {
            $p, $v = $_.split('=')
            Set-Item -path env:$p -value $v
        }
    }
}

function ShowErrorMessage
{
    Write-Output "[91mError: Build has failed![0m"
}

try
{
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
        clang -std=c++17 -lc++ -liconv -dynamiclib -o $outputDirectory/Elemental.Tools.Native.dylib ../UnityBuild.cpp
    } else {
        RegisterVisualStudioEnvironment

        #TODO: Put the directxshader include real include path. For the moment, it takes the windows SDK header

        if ($Configuration -eq "Debug") {
            cl.exe /nologo /DUNICODE /D_UNICODE /DDEBUG /D_DEBUG /D_WINDOWS /D_USRDLL /D_WINDLL /std:c++17 /Zi /EHsc "../UnityBuild.cpp" /link /DLL /OUT:$outputDirectory/Elemental.Tools.Native.dll /SUBSYSTEM:WINDOWS /DEBUG /MAP /OPT:ref /INCREMENTAL:NO /WINMD:NO /NOLOGO uuid.lib libcmt.lib libvcruntimed.lib libucrtd.lib kernel32.lib user32.lib gdi32.lib ole32.lib advapi32.lib Winmm.lib
        } else {
            cl.exe /nologo /DUNICODE /D_UNICODE /D_WINDOWS /D_USRDLL /D_WINDLL /std:c++17 /O2 /Zi /EHsc "../UnityBuild.cpp" /link /DLL /OUT:$outputDirectory/Elemental.Tools.Native.dll /SUBSYSTEM:WINDOWS /MAP /OPT:ref /INCREMENTAL:NO /WINMD:NO /NOLOGO uuid.lib libcmt.lib libvcruntimed.lib libucrtd.lib kernel32.lib user32.lib gdi32.lib ole32.lib advapi32.lib Winmm.lib
        }
    }
}

finally
{
    Pop-Location
    Pop-Location
}