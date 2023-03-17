using System.Runtime.CompilerServices;

namespace Elemental.Tools;

internal class DirectXShaderCompilerProvider : IShaderCompilerProvider
{
    private readonly string _dxcLibraryPath;
    private readonly string _dxilLibraryPath;

    public ShaderLanguage ShaderLanguage => ShaderLanguage.Hlsl;
    public ReadOnlySpan<ShaderLanguage> TargetShaderLanguages => new[] { ShaderLanguage.Dxil, ShaderLanguage.Spirv };

    public DirectXShaderCompilerProvider()
    {
        var processPath = Path.GetDirectoryName(Environment.ProcessPath)!;

        _dxcLibraryPath = Path.Combine(processPath, "ShaderCompilers", "dxcompiler.dll");
        _dxilLibraryPath = Path.Combine(processPath, "ShaderCompilers", "dxil.dll");
    }

    public unsafe bool IsCompilerInstalled()
    {
        // TODO: Change extension based on OS
/*
        NativeLibrary.Load(_dxilLibraryPath);

        if (!DirectXShaderCompilerInterop.DxcCreateInstance(DirectXShaderCompilerInterop.CLSID_DxcCompiler, out IDxcCompiler compiler))
        {
            Console.WriteLine("ERRRRROOOOR");
            return false;
        }

        var arguments = new string[]
        {
            "-HV 2021"
        };

        var compileResult = compiler.Compile(new StringBlob(testData), "test.hlsl", "MeshMain", "ms_6_7", arguments, arguments.Length, null, 0, null);

        var errors = compileResult.GetErrors();

        var bufferPointer = errors.GetBufferPointer();

        var outputString = Utf8StringMarshaller.ConvertToManaged((byte*)bufferPointer);

        Console.WriteLine($"Compiler Errors: {outputString}");
*/
        return File.Exists(_dxcLibraryPath) && File.Exists(_dxilLibraryPath);
    }

    public ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
    {
        throw new NotImplementedException();
    }

    private static string testData = """
struct ShaderParameters
{
    float RotationX;
    float RotationY;
};

float3 rotate(float3 position, float pitch, float roll, float yaw) 
{
    float cosa = cos(yaw);
    float sina = sin(yaw);

    float cosb = cos(pitch);
    float sinb = sin(pitch);

    float cosc = cos(roll);
    float sinc = sin(roll);

    float Axx = cosa*cosb;
    float Axy = cosa*sinb*sinc - sina*cosc;
    float Axz = cosa*sinb*cosc + sina*sinc;

    float Ayx = sina*cosb;
    float Ayy = sina*sinb*sinc + cosa*cosc;
    float Ayz = sina*sinb*cosc - cosa*sinc;

    float Azx = -sinb;
    float Azy = cosb*sinc;
    float Azz = cosb*cosc;

    float px = position.x;
    float py = position.y;
    float pz = position.z;

    float3 result;

    result.x = Axx*px + Axy*py + Axz*pz;
    result.y = Ayx*px + Ayy*py + Ayz*pz;
    result.z = Azx*px + Azy*py + Azz*pz;

    return result;
}

[[vk::push_constant]]
ConstantBuffer<ShaderParameters> parameters : register(b0);

struct VertexOutput
{
    float4 Position: SV_Position;
    float4 Color: TEXCOORD0;
};

static float3 triangleVertices[] =
{
    float3(-0.5, 0.5, 0),
    float3(0.5, 0.5, 0),
    float3(-0.5, -0.5, 0)
};

static float4 triangleColors[] =
{
    float4(1.0, 0.0, 0, 1),
    float4(0.0, 1.0, 0, 1),
    float4(0.0, 0.0, 1, 1)
};

static uint3 rectangleIndices[] =
{
    uint3(0, 1, 2)
};

[OutputTopology("triangle")]
[NumThreads(32, 1, 1)]
void MeshMain(in uint groupId : SV_GroupID, in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[128], out indices uint3 indices[128])
{
    const uint meshVertexCount = 3;
    const uint meshPrimitiveCount = 1;

    SetMeshOutputCounts(meshVertexCount, meshPrimitiveCount);

    if (groupThreadId < meshVertexCount)
    {
        float3 position = triangleVertices[groupThreadId];
        
        position = rotate(position, parameters.RotationY, parameters.RotationX, 0);
        position.z = 0.5;

        vertices[groupThreadId].Position = float4(position, 1);
        vertices[groupThreadId].Color = triangleColors[groupThreadId];
    }

    if (groupThreadId < meshPrimitiveCount)
    {
        indices[groupThreadId] = rectangleIndices[groupThreadId];
    }
}
""";
}

internal unsafe class StringBlob : IDxcBlob
{
    private readonly string _data;
    private readonly byte* _unmanagedData;

    public StringBlob(string data)
    {
        _data = data;
        _unmanagedData = Utf8StringMarshaller.ConvertToUnmanaged(data);
    }

    public unsafe char* GetBufferPointer()
    {
        return (char*)_unmanagedData;
    }

    public uint GetBufferSize()
    {
        return (uint)_data.Length;
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

