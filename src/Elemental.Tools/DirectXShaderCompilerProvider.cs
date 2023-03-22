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

    // TODO: Support includes
    public unsafe ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        TestDxc(shaderCode);

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

    private void TestDxc(ReadOnlySpan<byte> shaderCode)
    {
        nint? dxilLibrary = null;

        if (File.Exists("ShaderCompilers/dxil"))
        {
            dxilLibrary = NativeLibrary.Load("ShaderCompilers/dxil");
        }

        var compilerPath = "ShaderCompilers/dxcompiler";

        if (OperatingSystem.IsMacOS())
        {
            compilerPath = "ShaderCompilers/lib/libdxcompiler.dylib";
        }

        var compilerLibrary = NativeLibrary.Load(compilerPath);

        Guid CLSID_DxcCompiler = new("73e22d93-e6ce-47f3-b5bf-f0664f39c1b0");
        Guid rfId = typeof(IDxcCompiler3).GUID;
        nint objectPointer;

        var DxcCreateInstance = (delegate* unmanaged[Stdcall]<Guid*, Guid*, nint*, uint>)NativeLibrary.GetExport(compilerLibrary, "DxcCreateInstance");
        var result = DxcCreateInstance(&CLSID_DxcCompiler, &rfId, &objectPointer);
        Console.WriteLine($"Result: {result}");

        var compiler = Marshal.PtrToStructure<IDxcCompiler3>(objectPointer);
        var refCount = compiler.AddRef();
        Console.WriteLine($"RefCount: {refCount}");
        
        refCount = compiler.AddRef();
        Console.WriteLine($"RefCount: {refCount}");

        result = compiler.Release();
        Console.WriteLine($"Result: {result}");
        
        refCount = compiler.AddRef();
        Console.WriteLine($"RefCount: {refCount}");

        fixed (byte* bufferPointer = shaderCode)
        {
            var buffer = new DxcBuffer();
            buffer.Ptr = bufferPointer;
            buffer.Size = (nuint)shaderCode.Length;

            var dxcArgs = new List<string>
        {
            "simpleVertex.hlsl",
            "-E",
            "LightVertexShader",
            "-T",
            "vs_6_6",
            "-Zi",
            "-Qstrip_debug",
            "-Qstrip_reflect"
        };

        // https://github.com/microsoft/DirectXShaderCompiler/blob/d04c296ad7220bcf4875b0f9366c547a5211204d/tools/clang/tools/dxcompiler/dxcompilerobj.cpp#LL635C2-L635C15

        var intPtrArray = new List<IntPtr>();
        foreach (var arg in dxcArgs)
        {
            intPtrArray.Add(Marshal.StringToHGlobalUni(arg));
        }
        
        var allocated = GCHandle.Alloc(intPtrArray.ToArray(), GCHandleType.Pinned);
        var arrayPtr = (ushort**)allocated.AddrOfPinnedObject();

        var includeHandler = new IDxcIncludeHandler();

            rfId = typeof(IDxcResult).GUID;
            result = compiler.Compile(&buffer, arrayPtr,(uint)dxcArgs.Count, &includeHandler, &rfId, &objectPointer);
            Console.WriteLine($"Compile Result: {result}");
        }

        NativeLibrary.Free(compilerLibrary);

        if (dxilLibrary != null)
        {
            NativeLibrary.Free(dxilLibrary.Value);
        }
    }
}

internal unsafe struct DxcBuffer
{
    public byte* Ptr;
    public nuint Size;
    public uint Encoding;
}

[Guid("8BA5FB08-5195-40E2-AC58-0D989C3A0102")]
internal unsafe partial struct IDxcBlob
{
    public void** lpVtbl;


/*
    /// <inheritdoc cref="IUnknown.QueryInterface" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(0)]
    public HRESULT QueryInterface([NativeTypeName("const IID &")] Guid* riid, void** ppvObject)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcBlob*, Guid*, void**, int>)(lpVtbl[0]))((IDxcBlob*)Unsafe.AsPointer(ref this), riid, ppvObject);
    }

    /// <inheritdoc cref="IUnknown.AddRef" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(1)]
    [return: NativeTypeName("ULONG")]
    public uint AddRef()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcBlob*, uint>)(lpVtbl[1]))((IDxcBlob*)Unsafe.AsPointer(ref this));
    }

    /// <inheritdoc cref="IUnknown.Release" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(2)]
    [return: NativeTypeName("ULONG")]
    public uint Release()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcBlob*, uint>)(lpVtbl[2]))((IDxcBlob*)Unsafe.AsPointer(ref this));
    }

    /// <include file='IDxcBlob.xml' path='doc/member[@name="IDxcBlob.GetBufferPointer"]/*' />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(3)]
    [return: NativeTypeName("LPVOID")]
    public void* GetBufferPointer()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcBlob*, void*>)(lpVtbl[3]))((IDxcBlob*)Unsafe.AsPointer(ref this));
    }

    /// <include file='IDxcBlob.xml' path='doc/member[@name="IDxcBlob.GetBufferSize"]/*' />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(4)]
    [return: NativeTypeName("SIZE_T")]
    public nuint GetBufferSize()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcBlob*, nuint>)(lpVtbl[4]))((IDxcBlob*)Unsafe.AsPointer(ref this));
    }

    public partial struct Vtbl
    {
        [NativeTypeName("HRESULT (const IID &, void **) __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcBlob*, Guid*, void**, int> QueryInterface;

        [NativeTypeName("ULONG () __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcBlob*, uint> AddRef;

        [NativeTypeName("ULONG () __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcBlob*, uint> Release;

        [NativeTypeName("LPVOID () __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcBlob*, void*> GetBufferPointer;

        [NativeTypeName("SIZE_T () __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcBlob*, nuint> GetBufferSize;
    }*/
}

[Guid("7F61FC7D-950D-467F-B3E3-3C02FB49187C")]
internal unsafe partial struct IDxcIncludeHandler
{
    public void** lpVtbl;

    public uint QueryInterface(Guid* riid, void** ppvObject)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcIncludeHandler*, Guid*, void**, uint>)(lpVtbl[0]))((IDxcIncludeHandler*)Unsafe.AsPointer(ref this), riid, ppvObject);
    }

    public uint AddRef()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcIncludeHandler*, uint>)(lpVtbl[1]))((IDxcIncludeHandler*)Unsafe.AsPointer(ref this));
    }

    public uint Release()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcIncludeHandler*, uint>)(lpVtbl[2]))((IDxcIncludeHandler*)Unsafe.AsPointer(ref this));
    }

    public uint LoadSource(ushort* pFilename, IDxcBlob** ppIncludeSource)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcIncludeHandler*, ushort*, IDxcBlob**, uint>)(lpVtbl[3]))((IDxcIncludeHandler*)Unsafe.AsPointer(ref this), pFilename, ppIncludeSource);
    }

    public partial struct Vtbl
    {
        public delegate* unmanaged[Stdcall]<IDxcIncludeHandler*, Guid*, void**, uint> QueryInterface;
        public delegate* unmanaged[Stdcall]<IDxcIncludeHandler*, uint> AddRef;
        public delegate* unmanaged[Stdcall]<IDxcIncludeHandler*, uint> Release;
        public delegate* unmanaged[Stdcall]<IDxcIncludeHandler*, ushort*, IDxcBlob**, uint> LoadSource;
    }
}

[Guid("58346CDA-DDE7-4497-9461-6F87AF5E0659")]
internal unsafe partial struct IDxcResult
{
    public void** lpVtbl;

/*
    /// <inheritdoc cref="IUnknown.QueryInterface" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(0)]
    public HRESULT QueryInterface([NativeTypeName("const IID &")] Guid* riid, void** ppvObject)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, Guid*, void**, int>)(lpVtbl[0]))((IDxcResult*)Unsafe.AsPointer(ref this), riid, ppvObject);
    }

    /// <inheritdoc cref="IUnknown.AddRef" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(1)]
    [return: NativeTypeName("ULONG")]
    public uint AddRef()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, uint>)(lpVtbl[1]))((IDxcResult*)Unsafe.AsPointer(ref this));
    }

    /// <inheritdoc cref="IUnknown.Release" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(2)]
    [return: NativeTypeName("ULONG")]
    public uint Release()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, uint>)(lpVtbl[2]))((IDxcResult*)Unsafe.AsPointer(ref this));
    }

    /// <inheritdoc cref="IDxcOperationResult.GetStatus" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(3)]
    public HRESULT GetStatus(HRESULT* pStatus)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, HRESULT*, int>)(lpVtbl[3]))((IDxcResult*)Unsafe.AsPointer(ref this), pStatus);
    }

    /// <inheritdoc cref="IDxcOperationResult.GetResult" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(4)]
    public HRESULT GetResult(IDxcBlob** ppResult)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, IDxcBlob**, int>)(lpVtbl[4]))((IDxcResult*)Unsafe.AsPointer(ref this), ppResult);
    }

    /// <inheritdoc cref="IDxcOperationResult.GetErrorBuffer" />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(5)]
    public HRESULT GetErrorBuffer(IDxcBlobEncoding** ppErrors)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, IDxcBlobEncoding**, int>)(lpVtbl[5]))((IDxcResult*)Unsafe.AsPointer(ref this), ppErrors);
    }

    /// <include file='IDxcResult.xml' path='doc/member[@name="IDxcResult.HasOutput"]/*' />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(6)]
    public BOOL HasOutput(DXC_OUT_KIND dxcOutKind)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, DXC_OUT_KIND, int>)(lpVtbl[6]))((IDxcResult*)Unsafe.AsPointer(ref this), dxcOutKind);
    }

    /// <include file='IDxcResult.xml' path='doc/member[@name="IDxcResult.GetOutput"]/*' />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(7)]
    public HRESULT GetOutput(DXC_OUT_KIND dxcOutKind, [NativeTypeName("const IID &")] Guid* iid, void** ppvObject, [NativeTypeName("IDxcBlobWide **")] IDxcBlobUtf16** ppOutputName)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, DXC_OUT_KIND, Guid*, void**, IDxcBlobUtf16**, int>)(lpVtbl[7]))((IDxcResult*)Unsafe.AsPointer(ref this), dxcOutKind, iid, ppvObject, ppOutputName);
    }

    /// <include file='IDxcResult.xml' path='doc/member[@name="IDxcResult.GetNumOutputs"]/*' />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(8)]
    [return: NativeTypeName("UINT32")]
    public uint GetNumOutputs()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, uint>)(lpVtbl[8]))((IDxcResult*)Unsafe.AsPointer(ref this));
    }

    /// <include file='IDxcResult.xml' path='doc/member[@name="IDxcResult.GetOutputByIndex"]/*' />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(9)]
    public DXC_OUT_KIND GetOutputByIndex([NativeTypeName("UINT32")] uint Index)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, uint, DXC_OUT_KIND>)(lpVtbl[9]))((IDxcResult*)Unsafe.AsPointer(ref this), Index);
    }

    /// <include file='IDxcResult.xml' path='doc/member[@name="IDxcResult.PrimaryOutput"]/*' />
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [VtblIndex(10)]
    public DXC_OUT_KIND PrimaryOutput()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcResult*, DXC_OUT_KIND>)(lpVtbl[10]))((IDxcResult*)Unsafe.AsPointer(ref this));
    }

    public partial struct Vtbl
    {
        [NativeTypeName("HRESULT (const IID &, void **) __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, Guid*, void**, int> QueryInterface;

        [NativeTypeName("ULONG () __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, uint> AddRef;

        [NativeTypeName("ULONG () __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, uint> Release;

        [NativeTypeName("HRESULT (HRESULT *) __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, HRESULT*, int> GetStatus;

        [NativeTypeName("HRESULT (IDxcBlob **) __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, IDxcBlob**, int> GetResult;

        [NativeTypeName("HRESULT (IDxcBlobEncoding **) __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, IDxcBlobEncoding**, int> GetErrorBuffer;

        [NativeTypeName("BOOL (DXC_OUT_KIND) __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, DXC_OUT_KIND, int> HasOutput;

        [NativeTypeName("HRESULT (DXC_OUT_KIND, const IID &, void **, IDxcBlobWide **) __attribute__((stdcall))")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, DXC_OUT_KIND, Guid*, void**, IDxcBlobUtf16**, int> GetOutput;

        [NativeTypeName("UINT32 ()")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, uint> GetNumOutputs;

        [NativeTypeName("DXC_OUT_KIND (UINT32)")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, uint, DXC_OUT_KIND> GetOutputByIndex;

        [NativeTypeName("DXC_OUT_KIND ()")]
        public delegate* unmanaged[Stdcall]<IDxcResult*, DXC_OUT_KIND> PrimaryOutput;
    }*/
}

[Guid("228B4687-5A6A-4730-900C-9702B2203F54")]
internal unsafe struct IDxcCompiler3
{
    public void** lpVtbl;

    public int QueryInterface(Guid* riid, void** ppvObject)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, Guid*, void**, int>)(lpVtbl[0]))((IDxcCompiler3*)Unsafe.AsPointer(ref this), riid, ppvObject);
    }

    public uint AddRef()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, uint>)(lpVtbl[1]))((IDxcCompiler3*)Unsafe.AsPointer(ref this));
    }

    public uint Release()
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, uint>)(lpVtbl[2]))((IDxcCompiler3*)Unsafe.AsPointer(ref this));
    }

    public uint Compile(DxcBuffer* pSource, ushort** pArguments, uint argCount, IDxcIncludeHandler* pIncludeHandler, Guid* riid, nint* ppResult)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, DxcBuffer*, ushort**, uint, IDxcIncludeHandler*, Guid*, nint*, uint>)(lpVtbl[3]))((IDxcCompiler3*)Unsafe.AsPointer(ref this), pSource, pArguments, argCount, pIncludeHandler, riid, ppResult);
    }
   /* 
    public int Disassemble(DxcBuffer* pObject, Guid* riid, void** ppResult)
    {
        return ((delegate* unmanaged[Stdcall]<IDxcCompiler3*, DxcBuffer*, Guid*, void**, int>)(lpVtbl[4]))((IDxcCompiler3*)Unsafe.AsPointer(ref this), pObject, riid, ppResult);
    }*/

    public partial struct Vtbl
    {
        public delegate* unmanaged[Stdcall]<IDxcCompiler3*, Guid*, void**, int> QueryInterface;
        public delegate* unmanaged[Stdcall]<IDxcCompiler3*, uint> AddRef;
        public delegate* unmanaged[Stdcall]<IDxcCompiler3*, uint> Release;
        public delegate* unmanaged[Stdcall]<IDxcCompiler3*, DxcBuffer*, ushort**, uint, IDxcIncludeHandler*, Guid*, void**, int> Compile;
        //public delegate* unmanaged[Stdcall]<IDxcCompiler3*, DxcBuffer*, Guid*, void**, int> Disassemble;
    }
}