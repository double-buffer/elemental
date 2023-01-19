param ($outputDirectory, $Configuration)

function RegisterVisualStudioEnvironment
{
    $registeredVisualStudioVersion = Get-Content -Path Env:VisualStudioVersion -ErrorAction SilentlyContinue

    if (-not($registeredVisualStudioVersion -eq "16.0") -And -not($registeredVisualStudioVersion -eq "17.0"))
    {
        Write-Output "[93mRegistering Visual Studio Environment...[0m"

        # TODO: Do something better here
        $vsPath = ""
        $vs2019ComPath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
        $vs2019ProfPath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
        $vs2019EntPath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
        $vs2019PreviewPath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Auxiliary\Build\vcvars64.bat"
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
    $outputDirectory = Resolve-Path $outputDirectory

    Write-Output "[93mCompiling Windows Platform Library...[0m"
    Push-Location ./../Platforms/Windows

    RegisterVisualStudioEnvironment
    #TODO: Nuget restore
    msbuild "Elemental.vcxproj" /p:configuration=$Configuration /p:outDir=$outputDirectory /p:platform=x64
    
    if (-Not $?) {
        Pop-Location
        Exit 1
    }
}

finally
{
    Pop-Location
}