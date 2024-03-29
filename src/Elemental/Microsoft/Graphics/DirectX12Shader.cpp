#include "DirectX12Shader.h"
#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_LIBRARIES UINT16_MAX
#define DIRECTX12_MAX_PIPELINESTATES UINT16_MAX

#define AddDirectX12StateSubObject(stateSubObjects, index, type, value) \
{ \
    auto stateValue = SystemPushArray<uint8_t>(stackMemoryArena, sizeof(value)); \
    SystemCopyBuffer(stateValue, ReadOnlySpan<uint8_t>((uint8_t*)&value, sizeof(value))); \
    stateSubObjects[stateSubObjectsIndex++] = { type, stateValue.Pointer }; \
}

SystemDataPool<DirectX12ShaderLibraryData, SystemDataPoolDefaultFull> directX12ShaderLibraryPool;
SystemDataPool<DirectX12PipelineStateData, DirectX12PipelineStateDataFull> directX12PipelineStatePool;

void InitDirectX12ShaderMemory()
{
    if (!directX12ShaderLibraryPool.Storage)
    {
        directX12ShaderLibraryPool = SystemCreateDataPool<DirectX12ShaderLibraryData>(DirectX12MemoryArena, DIRECTX12_MAX_LIBRARIES);
        directX12PipelineStatePool = SystemCreateDataPool<DirectX12PipelineStateData, DirectX12PipelineStateDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_PIPELINESTATES);
    }
}

DirectX12ShaderLibraryData* GetDirectX12ShaderLibraryData(ElemShaderLibrary shaderLibrary)
{
    return SystemGetDataPoolItem(directX12ShaderLibraryPool, shaderLibrary);
}

DirectX12PipelineStateData* GetDirectX12PipelineStateData(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItem(directX12PipelineStatePool, pipelineState);
}

DirectX12PipelineStateDataFull* GetDirectX12PipelineStateDataFull(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItemFull(directX12PipelineStatePool, pipelineState);
}

bool CheckDirectX12ShaderDataHeader(ElemDataSpan data, const char* headerValue)
{
    for (uint32_t i = 0; i < 4; i++)
    {
        if (data.Items[i] != headerValue[i])
        {
            return false;
        }
    }

    return true;
}

ElemShaderLibrary DirectX12CreateShaderLibrary(ElemDataSpan shaderLibraryData)
{
    InitDirectX12ShaderMemory();

    D3D12_SHADER_BYTECODE directX12LibraryData = {};
    Span<D3D12_SHADER_BYTECODE> graphicsShaderData = {};

    if (CheckDirectX12ShaderDataHeader(shaderLibraryData, "DXBC"))
    {
        // TODO: Need to copy the data here
        directX12LibraryData =
        {
            .pShaderBytecode = shaderLibraryData.Items,
            .BytecodeLength = shaderLibraryData.Length
        };
    }
    else 
    {
        auto dataSpan = Span<uint8_t>(shaderLibraryData.Items, shaderLibraryData.Length); 
        auto shaderCount = *(uint32_t*)dataSpan.Pointer;
        dataSpan = dataSpan.Slice(sizeof(uint32_t));

        // HACK: This is bad to allocate this here but this should be temporary
        graphicsShaderData = SystemPushArray<D3D12_SHADER_BYTECODE>(DirectX12MemoryArena, shaderCount);

        for (uint32_t i = 0; i < shaderCount; i++)
        {
            auto size = *(uint32_t*)dataSpan.Pointer;
            dataSpan = dataSpan.Slice(sizeof(uint32_t));

            graphicsShaderData[i] =
            {
                .pShaderBytecode = dataSpan.Pointer,
                .BytecodeLength = size
            };

            dataSpan = dataSpan.Slice(size);
        }
    }

    auto handle = SystemAddDataPoolItem(directX12ShaderLibraryPool, {
        .ShaderLibraryData = directX12LibraryData,
        .GraphicsShaders = graphicsShaderData
    }); 

    return handle;
}

void DirectX12FreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
    // Free data
}

ElemPipelineState DirectX12CompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    InitDirectX12ShaderMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    auto graphicsDeviceDataFull = GetDirectX12GraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData= GetDirectX12ShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);
    
    auto stackMemoryArena = SystemGetStackMemoryArena();

    D3D12_RT_FORMAT_ARRAY renderTargets = {};

    renderTargets.NumRenderTargets = 1;
    renderTargets.RTFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // TODO: Fill Correct Back Buffer Format
   
    DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;

    /*if (renderPassDescriptor.DepthTexturePointer.HasValue)
    {
        // TODO: Change that
        depthFormat = DXGI_FORMAT_D32_FLOAT;
    }*/

    D3D12_RASTERIZER_DESC rasterizerState = {};
    rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerState.CullMode = D3D12_CULL_MODE_NONE; // D3D12_CULL_MODE_BACK;
    rasterizerState.FrontCounterClockwise = false;
    rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerState.DepthClipEnable = true;
    rasterizerState.MultisampleEnable = false;
    rasterizerState.AntialiasedLineEnable = false;

    // TODO: Check those 2
    rasterizerState.ForcedSampleCount = 0;
    rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

   /* 
    D3D12_DEPTH_STENCIL_DESC depthStencilState = {};
    
    if (renderPassDescriptor.DepthBufferOperation != GraphicsDepthBufferOperation::DepthNone)
    {
        depthStencilState.DepthEnable = true;
        depthStencilState.StencilEnable = false;

        if (renderPassDescriptor.DepthBufferOperation == GraphicsDepthBufferOperation::ClearWrite ||
            renderPassDescriptor.DepthBufferOperation == GraphicsDepthBufferOperation::Write)
        {
            depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        }

        if (renderPassDescriptor.DepthBufferOperation == GraphicsDepthBufferOperation::CompareEqual)
        {
            depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        }

        else
        {
            depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        }
    }
*/
    D3D12_BLEND_DESC blendState = {};
    blendState.AlphaToCoverageEnable = false;
    blendState.IndependentBlendEnable = false;
/*
    if (renderPassDescriptor.RenderTarget1BlendOperation.HasValue)
    {
        auto blendOperation = renderPassDescriptor.RenderTarget1BlendOperation.Value;
        blendState.RenderTarget[0] = InitBlendState(blendOperation);
    }

    else
    {*/
    blendState.RenderTarget[0] = {
        false,
        false,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    }; // InitBlendState(GraphicsBlendOperation::None);
    //}

    
    //psoDesc.RootSignature = shader->RootSignature.Get();

    /*if (shader->AmplificationShader.Length > 0)
    {
        psoDesc.AS = { shader->AmplificationShader.Pointer, shader->AmplificationShader.Length };
    }

    psoDesc.MS = { shader->MeshShader.Pointer, shader->MeshShader.Length };

    if (shader->PixelShader.Length > 0)
    {
        psoDesc.PS = { shader->PixelShader.Pointer, shader->PixelShader.Length };
    }*/

    //psoDesc.RenderTargets = renderTargets;
    //psoDesc.RasterizerState = rasterizerState;
    //psoDesc.DepthStencilFormat = depthFormat;
    //psoDesc.DepthStencilState = depthStencilState;
    //psoDesc.BlendState = blendState;



    auto stateSubObjects = SystemPushArray<D3D12_STATE_SUBOBJECT>(stackMemoryArena, 32);
    auto stateSubObjectsIndex = 0u;

    if (shaderLibraryData->ShaderLibraryData.BytecodeLength > 0)
    {
        D3D12_DXIL_LIBRARY_DESC desc = { .DXILLibrary = shaderLibraryData->ShaderLibraryData };
        AddDirectX12StateSubObject(stateSubObjects, stateSubObjectsIndex++, D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, desc);
    }
    else
    { 
        for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
        {
            D3D12_DXIL_LIBRARY_DESC desc = { .DXILLibrary = shaderLibraryData->GraphicsShaders[i] };
            AddDirectX12StateSubObject(stateSubObjects, stateSubObjectsIndex++, D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, desc);
        }
    }

    // TODO: Move that to shader library loading
    D3D12_GLOBAL_ROOT_SIGNATURE rootSignatureDesc = { .pGlobalRootSignature = graphicsDeviceDataFull->RootSignature.Get() };
    AddDirectX12StateSubObject(stateSubObjects, stateSubObjectsIndex++, D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, rootSignatureDesc);

    LPCWSTR exports[] = { L"MeshMain", L"PixelMain" };
    D3D12_GENERIC_PROGRAM_DESC genericProgramDesc =
    {
        .ProgramName = L"GraphicsProgram",
        .NumExports = 2,
        .pExports = exports,
        .NumSubobjects = 0,
        .ppSubobjects = nullptr
    };
    AddDirectX12StateSubObject(stateSubObjects, stateSubObjectsIndex++, D3D12_STATE_SUBOBJECT_TYPE_GENERIC_PROGRAM, genericProgramDesc);

    D3D12_RASTERIZER_DESC2 rasterizerDesc = 
    {
        .FillMode = D3D12_FILL_MODE_SOLID,
        .CullMode = D3D12_CULL_MODE_NONE
    };
    AddDirectX12StateSubObject(stateSubObjects, stateSubObjectsIndex++, D3D12_STATE_SUBOBJECT_TYPE_RASTERIZER, rasterizerDesc);

    D3D12_STATE_OBJECT_DESC stateObjectDesc = 
    {
        .Type = D3D12_STATE_OBJECT_TYPE_EXECUTABLE,
        .NumSubobjects = stateSubObjectsIndex,
        .pSubobjects = stateSubObjects.Pointer
    };

    ComPtr<ID3D12StateObject> stateObject;
    AssertIfFailed(graphicsDeviceData->Device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(stateObject.GetAddressOf())));

    auto handle = SystemAddDataPoolItem(directX12PipelineStatePool, {
        .DeviceObject = stateObject,
    }); 

    SystemAddDataPoolItemFull(directX12PipelineStatePool, handle, {
    });

    return handle;
}

void DirectX12FreePipelineState(ElemPipelineState pipelineState)
{
    SystemAssert(pipelineState != ELEM_HANDLE_NULL);

    auto pipelineStateData = GetDirectX12PipelineStateData(pipelineState);
    SystemAssert(pipelineStateData);

    auto pipelineStateDataFull = GetDirectX12PipelineStateDataFull(pipelineState);
    SystemAssert(pipelineStateDataFull);

    pipelineStateData->DeviceObject.Reset();

    SystemRemoveDataPoolItem(directX12PipelineStatePool, pipelineState);
}
