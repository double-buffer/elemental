using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Elemental.Tools;

internal unsafe class DirectXShaderCompilerProvider : IShaderCompilerProvider
{
    private readonly string _dxcPath;

    public ShaderLanguage ShaderLanguage => ShaderLanguage.Hlsl;
    public ReadOnlySpan<ShaderLanguage> TargetShaderLanguages => new[] { ShaderLanguage.Dxil, ShaderLanguage.Spirv };

    public DirectXShaderCompilerProvider()
    {
        var processPath = Path.GetDirectoryName(Environment.ProcessPath)!;
        _dxcPath = string.Empty;

        if (OperatingSystem.IsWindows())
        {
            _dxcPath = Path.Combine(processPath, "ShaderCompilers", "dxc.exe");
        }
        else if (OperatingSystem.IsMacOS())
        {
            _dxcPath = Path.Combine(processPath, "ShaderCompilers", "bin", "dxc");
        }
    }

    public bool IsCompilerInstalled()
    {
        return File.Exists(_dxcPath);
    }

    public unsafe ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        TestDxc();

        var inputFilePath = $"{Path.GetTempFileName()}.hlsl";
        var outputFilePath = Path.GetTempFileName();

        File.WriteAllBytes(inputFilePath, shaderCode.ToArray());
        
        var arguments = new List<string>()
        {
            "-HV 2021"
        };

        if (graphicsApi != ToolsGraphicsApi.Direct3D12)
        {
            arguments.Add("-spirv");
            arguments.Add("-fspv-target-env=vulkan1.3");
        }
        else
        {
            // HACK: For the moment we hard code the root signature def
            arguments.Add("-rootsig-define RootSignatureDef");
        }

        var shaderTarget = "ms_6_7";

        if (shaderStage == ToolsShaderStage.AmplificationShader)
        {
            shaderTarget = "as_6_7";
        }
        else if (shaderStage == ToolsShaderStage.PixelShader)
        {
            shaderTarget = "ps_6_7";
        }

        using var process = Process.Start(new ProcessStartInfo 
        { 
            FileName = _dxcPath, 
            Arguments = $"-T {shaderTarget} -E {entryPoint} -Fo {outputFilePath} {string.Join(' ', arguments.ToArray())} {inputFilePath}", 
            RedirectStandardError = true, 
            RedirectStandardOutput = true
        });

        if (process == null)
        {
            return ShaderCompilerResult.CreateErrorResult(shaderStage, entryPoint, "Cannot start Metal shader compiler process.");
        }

        process.WaitForExit();
        string? line;
        var logList = new List<ShaderCompilerLogEntry>();
        var currentLogType = ShaderCompilerLogEntryType.Error;
        var hasErrors = false;

        while ((line = process.StandardError.ReadLine()) != null)
        {
            if (line.Contains("warning:"))
            {
                currentLogType = ShaderCompilerLogEntryType.Warning;
            }
            else if (line.Contains("error:"))
            {
                currentLogType = ShaderCompilerLogEntryType.Error;
                hasErrors = true;
            }

            logList.Add(new() { Type = currentLogType, Message = line });
        }
        
        while ((line = process.StandardOutput.ReadLine()) != null)
        {
            logList.Add(new() { Type = ShaderCompilerLogEntryType.Message, Message = line });
        }
        
        var shaderBytecode = File.ReadAllBytes(outputFilePath);

        File.Delete(inputFilePath);
        File.Delete(outputFilePath);

        return new ShaderCompilerResult
        {
            IsSuccess = !hasErrors,
            Stage = shaderStage,
            EntryPoint = entryPoint,
            LogEntries = logList.ToArray(),
            ShaderData = shaderBytecode
        };
    }

    private void TestDxc()
    {
        nint? dxilLibrary = null;

        if (File.Exists("ShaderCompilers/dxil"))
        {
            dxilLibrary = NativeLibrary.Load("ShaderCompilers/dxil");
        }

        var compilerLibrary = NativeLibrary.Load("ShaderCompilers/dxcompiler");

        Guid CLSID_DxcCompiler = new("73e22d93-e6ce-47f3-b5bf-f0664f39c1b0");
        Guid rfId = typeof(IDxcCompiler3).GUID;
        nint objectPointer;

        var DxcCreateInstance = (delegate* unmanaged[Stdcall]<Guid*, Guid*, nint*, int>)NativeLibrary.GetExport(compilerLibrary, "DxcCreateInstance");
        var result = DxcCreateInstance(&CLSID_DxcCompiler, &rfId, &objectPointer);

        //IDxcCompiler3 compiler = Marshal.

        Console.WriteLine($"Result: {result}");

        NativeLibrary.Free(compilerLibrary);

        if (dxilLibrary != null)
        {
            NativeLibrary.Free(dxilLibrary.Value);
        }
    }
}


[Guid("228B4687-5A6A-4730-900C-9702B2203F54")]
internal unsafe struct IDxcCompiler3
{
    public void** lpVtbl;

    public int QueryInterface(Guid* riid, void** ppvObject)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, Guid*, void**, int>)(lpVtbl[0]))((IDxcCompiler3*)Unsafe.AsPointer(ref this), riid, ppvObject);
    }

    uint AddRef()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, uint>)(lpVtbl[1]))((IDxcCompiler3*)Unsafe.AsPointer(ref this));
    }

    public uint Release()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, uint>)(lpVtbl[2]))((IDxcCompiler3*)Unsafe.AsPointer(ref this));
    }
/*
    public int Compile(DxcBuffer* pSource, ushort** pArguments, uint argCount, IDxcIncludeHandler* pIncludeHandler, Guid* riid, void** ppResult)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, DxcBuffer*, ushort**, uint, IDxcIncludeHandler*, Guid*, void**, int>)(lpVtbl[3]))((IDxcCompiler3*)Unsafe.AsPointer(ref this), pSource, pArguments, argCount, pIncludeHandler, riid, ppResult);
    }
    
    public int Disassemble(DxcBuffer* pObject, Guid* riid, void** ppResult)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, DxcBuffer*, Guid*, void**, int>)(lpVtbl[4]))((IDxcCompiler3*)Unsafe.AsPointer(ref this), pObject, riid, ppResult);
    }*/

    public partial struct Vtbl
    {
        public delegate* unmanaged[Stdcall]<IDxcCompiler3*, Guid*, void**, int> QueryInterface;
        public delegate* unmanaged[Stdcall]<IDxcCompiler3*, uint> AddRef;
        public delegate* unmanaged[Stdcall]<IDxcCompiler3*, uint> Release;
        //public delegate* unmanaged[Stdcall]<IDxcCompiler3*, DxcBuffer*, ushort**, uint, IDxcIncludeHandler*, Guid*, void**, int> Compile;
        //public delegate* unmanaged[Stdcall]<IDxcCompiler3*, DxcBuffer*, Guid*, void**, int> Disassemble;
    }
}