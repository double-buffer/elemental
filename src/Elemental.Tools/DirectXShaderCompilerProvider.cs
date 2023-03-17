using System.Runtime.CompilerServices;

namespace Elemental.Tools;

internal class DirectXShaderCompilerProvider : IShaderCompilerProvider
{
    private readonly string _dxcLibraryPath;
    private readonly string? _dxilLibraryPath;

    public ShaderLanguage ShaderLanguage => ShaderLanguage.Hlsl;
    public ReadOnlySpan<ShaderLanguage> TargetShaderLanguages => new[] { ShaderLanguage.Dxil, ShaderLanguage.Spirv };

    public DirectXShaderCompilerProvider()
    {
        var processPath = Path.GetDirectoryName(Environment.ProcessPath)!;

        if (OperatingSystem.IsWindows())
        {
            _dxcLibraryPath = Path.Combine(processPath, "ShaderCompilers", "dxcompiler.dll");
            _dxilLibraryPath = Path.Combine(processPath, "ShaderCompilers", "dxil.dll");
        }
        else if (OperatingSystem.IsMacOS())
        {
            _dxcLibraryPath = Path.Combine(processPath, "ShaderCompilers", "libdxcompiler.dylib");
        }
    }

    public bool IsCompilerInstalled()
    {
        return _dxcLibraryPath != null && File.Exists(_dxcLibraryPath) && (_dxilLibraryPath == null || File.Exists(_dxilLibraryPath));
    }

    public unsafe ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        if (_dxilLibraryPath != null)
        {
            NativeLibrary.Load(_dxilLibraryPath);
        }

        if (!DirectXShaderCompilerInterop.DxcCreateInstance(DirectXShaderCompilerInterop.CLSID_DxcCompiler, out IDxcCompiler compiler))
        {
            return ShaderCompilerResult.CreateErrorResult("Cannot instantiate DirectX Shader Compiler.");
        }

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

        var compileResult = compiler.Compile(new BinaryBlob(shaderCode), "SourceShader.hlsl", entryPoint, shaderTarget, arguments.ToArray(), arguments.Count, null, 0, null);

        var errors = compileResult.GetErrors();
        var bufferPointer = errors.GetBufferPointer();
        var outputString = Utf8StringMarshaller.ConvertToManaged((byte*)bufferPointer);

        var dataResult = compileResult.GetResult();
        var shaderData = new Span<byte>(dataResult.GetBufferPointer(), (int)dataResult.GetBufferSize());

        var logList = new List<ShaderCompilerLogEntry>();
        var currentLogType = ShaderCompilerLogEntryType.Error;
        var hasErrors = false;

        if (outputString != null)
        {
            foreach (var line in outputString.Split('\n'))
            {
                if (string.IsNullOrEmpty(line))
                {
                    continue;
                }

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
        }

        return new ShaderCompilerResult
        {
            IsSuccess = !hasErrors,
            LogEntries = logList.ToArray(),
            ShaderData = shaderData.ToArray()
        };
    }
}

internal unsafe readonly struct BinaryBlob : IDxcBlob
{
    private readonly void* _unmanagedData;
    private readonly uint _dataSize;

    public BinaryBlob(ReadOnlySpan<byte> data)
    {
        _dataSize = (uint)data.Length;

        _unmanagedData = NativeMemory.Alloc(_dataSize);

        fixed (void* dataPointer = data)
        {
            NativeMemory.Copy(dataPointer, _unmanagedData, _dataSize);
        }
    }

    // TODO: Disposable and desctructor

    public unsafe char* GetBufferPointer()
    {
        return (char*)_unmanagedData;
    }

    public uint GetBufferSize()
    {
        return _dataSize;
    }
}

[ComImport]
[Guid("8BA5FB08-5195-40e2-AC58-0D989C3A0102")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
internal interface IDxcBlob
{
    [PreserveSig]
    unsafe char* GetBufferPointer();
    [PreserveSig]
    UInt32 GetBufferSize();
}

[ComImport]
[Guid("8BA5FB08-5195-40e2-AC58-0D989C3A0102")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
internal interface IDxcBlobEncoding : IDxcBlob
{
    System.UInt32 GetEncoding(out bool unknown, out UInt32 codePage);
}

[ComImport]
[Guid("CEDB484A-D4E9-445A-B991-CA21CA157DC2")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
internal interface IDxcOperationResult
{
    Int32 GetStatus();
    IDxcBlob GetResult();
    IDxcBlobEncoding GetErrors();
}

[ComImport]
[Guid("7f61fc7d-950d-467f-b3e3-3c02fb49187c")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
internal interface IDxcIncludeHandler
{
    IDxcBlob LoadSource(string fileName);
}

[StructLayout(LayoutKind.Sequential)]
internal struct DXCDefine
{
    [MarshalAs(UnmanagedType.LPWStr)]
    public string pName;
    [MarshalAs(UnmanagedType.LPWStr)]
    public string pValue;
}

[ComImport]
[Guid("8c210bf3-011f-4422-8d70-6f9acb8db617")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
internal interface IDxcCompiler
{
    IDxcOperationResult Compile(IDxcBlob source, string sourceName, string entryPoint, string targetProfile,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType =UnmanagedType.LPWStr)]
            string[] arguments,
            int argCount, DXCDefine[] defines, int defineCount, IDxcIncludeHandler includeHandler);
}

internal static partial class DirectXShaderCompilerInterop
{
    public static Guid CLSID_DxcCompiler = new("73e22d93-e6ce-47f3-b5bf-f0664f39c1b0");

    [UnmanagedCallConv(CallConvs = new Type[] { typeof(CallConvStdcall) })]
    [DllImport("ShaderCompilers/dxcompiler")]
    private static extern int DxcCreateInstance(ref Guid clsid, ref Guid iid, [MarshalAs(UnmanagedType.IUnknown)] out object instance);

    internal static bool DxcCreateInstance<T>(Guid clsid, out T instance)
    {
        var guid = typeof(T).GUID;
        var result = DxcCreateInstance(ref clsid, ref guid, out object outputObject);
        instance = (T)outputObject;
        return result == 0;
    }
}

