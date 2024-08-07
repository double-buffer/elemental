#include "DirectX12Shader.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12Resource.h"
#include "DirectX12CommandList.h"
#include "Graphics/ShaderReader.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_LIBRARIES UINT16_MAX
#define DIRECTX12_MAX_PIPELINESTATES UINT16_MAX

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

bool CheckDirectX12PipelineStateType(const DirectX12CommandListData* commandListData, DirectX12PipelineStateType type)
{
    if (commandListData->PipelineStateType != type)
    {
        if (type == DirectX12PipelineStateType_Compute)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "A compute pipelinestate must be bound to the commandlist before calling a compute command.");
        }
        else
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "A graphics pipelinestate must be bound to the commandlist before calling a rendering command.");
        }

        return false;
    }

    return true;
}

D3D12_SHADER_BYTECODE GetDirectX12ShaderFunctionByteCode(DirectX12ShaderLibraryData* shaderLibraryData, ShaderType shaderType, const char* function)
{
    SystemAssert(function);
    D3D12_SHADER_BYTECODE result = {};

    for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
    {
        auto shader = shaderLibraryData->GraphicsShaders[i];

        if (shader.ShaderType == shaderType && SystemFindSubString(shader.Name, function) != -1)
        {
            auto byteCodeData = D3D12_SHADER_BYTECODE { .pShaderBytecode = shader.ShaderCode.Pointer, .BytecodeLength = shader.ShaderCode.Length };
            result = byteCodeData;
            break;
        }
    }
        
    if (!result.pShaderBytecode)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot find shader function '%s'", function);
    }

    return result;
}

ElemShaderLibrary DirectX12CreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData)
{
    InitDirectX12ShaderMemory();

    D3D12_SHADER_BYTECODE directX12LibraryData = {};
    ReadOnlySpan<Shader> graphicsShaderData = {};

    if (CheckDirectX12ShaderDataHeader(shaderLibraryData, "DXBC"))
    {
        auto dest = SystemPushArray<uint8_t>(DirectX12MemoryArena, shaderLibraryData.Length);
        SystemCopyBuffer(dest, ReadOnlySpan<uint8_t>(shaderLibraryData.Items, shaderLibraryData.Length));

        directX12LibraryData =
        {
            .pShaderBytecode = dest.Pointer,
            .BytecodeLength = dest.Length
        };
    }
    else 
    {
        auto dataSpan = Span<uint8_t>(shaderLibraryData.Items, shaderLibraryData.Length); 
        graphicsShaderData = ReadShaders(DirectX12MemoryArena, dataSpan);
    }

    auto handle = SystemAddDataPoolItem(directX12ShaderLibraryPool, {
        .ShaderLibraryData = directX12LibraryData,
        .GraphicsShaders = graphicsShaderData
    }); 

    return handle;
}

void DirectX12FreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
    // TODO: Free data
}

ComPtr<ID3D12PipelineState> CreateDirectX12OldPSO(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData= GetDirectX12ShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);

    ComPtr<ID3D12PipelineState> pipelineState;

    D3D12_RT_FORMAT_ARRAY renderTargets = {};

    renderTargets.NumRenderTargets = 1;
    renderTargets.RTFormats[0] = ConvertToDirectX12TextureFormat(parameters->TextureFormats.Items[0]); // TODO: Fill Correct Back Buffer Format
   
    DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;

    /*if (renderPassDescriptor.DepthTexturePointer.HasValue)
    {
        // TODO: Change that
        depthFormat = DXGI_FORMAT_D32_FLOAT;
    }*/

    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;

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

    D3D12_PIPELINE_STATE_STREAM_DESC psoStream = {};
    GraphicsPso psoDesc = {};
    
    psoDesc.RootSignature = graphicsDeviceData->RootSignature.Get();

    if (parameters->MeshShaderFunction)
    {
        auto shaderByteCode = GetDirectX12ShaderFunctionByteCode(shaderLibraryData, ShaderType_Mesh, parameters->MeshShaderFunction);

        if (!shaderByteCode.pShaderBytecode)
        {
            return ELEM_HANDLE_NULL;
        }

        psoDesc.MS = shaderByteCode;
    }
    
    if (parameters->PixelShaderFunction)
    {
        auto shaderByteCode = GetDirectX12ShaderFunctionByteCode(shaderLibraryData, ShaderType_Pixel, parameters->PixelShaderFunction);

        if (!shaderByteCode.pShaderBytecode)
        {
            return ELEM_HANDLE_NULL;
        }

        psoDesc.PS = shaderByteCode;
    }

    psoDesc.RenderTargets = renderTargets;
    psoDesc.SampleDesc = sampleDesc;
    psoDesc.RasterizerState = rasterizerState;
    psoDesc.DepthStencilFormat = depthFormat;
    //psoDesc.DepthStencilState = depthStencilState;
    psoDesc.BlendState = blendState;

    psoStream.SizeInBytes = sizeof(GraphicsPso);
    psoStream.pPipelineStateSubobjectStream = &psoDesc;

    AssertIfFailed(graphicsDeviceData->Device->CreatePipelineState(&psoStream, IID_PPV_ARGS(pipelineState.GetAddressOf())));

    return pipelineState;
}

ElemPipelineState DirectX12CompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    InitDirectX12ShaderMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData= GetDirectX12ShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);
    
    // TODO: Test for now without stack
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto stateSubObjects = SystemPushArray<D3D12_STATE_SUBOBJECT>(stackMemoryArena, 32);
    auto stateSubObjectsIndex = 0u;

    auto dxilLibraries = SystemPushArray<D3D12_DXIL_LIBRARY_DESC>(stackMemoryArena, 8);
    auto dxilLibrariesIndex = 0u;

    if (shaderLibraryData->ShaderLibraryData.BytecodeLength > 0)
    {
        D3D12_DXIL_LIBRARY_DESC desc = { .DXILLibrary = shaderLibraryData->ShaderLibraryData };
        dxilLibraries[dxilLibrariesIndex++] = desc;
    }
    else
    { 
        for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
        {
            //D3D12_DXIL_LIBRARY_DESC desc = { .DXILLibrary = shaderLibraryData->GraphicsShaders[i] };
            //dxilLibraries[dxilLibrariesIndex++] = desc;
        }
    }

 /*   D3D12_GLOBAL_ROOT_SIGNATURE rootSignatureDesc = { .pGlobalRootSignature = graphicsDeviceData->RootSignature.Get() };
    
    D3D12_RASTERIZER_DESC2 rasterizerDesc = 
    {
        .FillMode = D3D12_FILL_MODE_SOLID,
        .CullMode = D3D12_CULL_MODE_NONE, // D3D12_CULL_MODE_BACK;
        .FrontCounterClockwise = false,
        .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable = true,
        .LineRasterizationMode = D3D12_LINE_RASTERIZATION_MODE_ALIASED,
        .ForcedSampleCount = 0,
        .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
    };
    
    for (uint32_t i = 0; i < dxilLibrariesIndex; i++)
    {
        stateSubObjects[stateSubObjectsIndex++] = { D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibraries[i] };
    }

    stateSubObjects[stateSubObjectsIndex++] = { D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &rootSignatureDesc };
    //stateSubObjects[stateSubObjectsIndex++] = { D3D12_STATE_SUBOBJECT_TYPE_RASTERIZER, &rasterizerDesc };
    D3D12_STATE_SUBOBJECT* rasterizerSubObject = &stateSubObjects[stateSubObjectsIndex - 1];

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0] = {
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
    };

    //stateSubObjects[stateSubObjectsIndex++] = { D3D12_STATE_SUBOBJECT_TYPE_BLEND, &blendDesc };
    D3D12_STATE_SUBOBJECT* blendSubObject = &stateSubObjects[stateSubObjectsIndex - 1];

    D3D12_RT_FORMAT_ARRAY renderTargetsDesc = 
    {
        .RTFormats = { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB }, // TODO: Temporary
        .NumRenderTargets = 1
    };
    stateSubObjects[stateSubObjectsIndex++] = { D3D12_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, &renderTargetsDesc };
    D3D12_STATE_SUBOBJECT* rtFormatsSubObject = &stateSubObjects[stateSubObjectsIndex - 1];

    auto programSubObjects = SystemPushArray<D3D12_STATE_SUBOBJECT*>(stackMemoryArena, 32);
    auto programSubObjectsIndex = 0u;

    //programSubObjects[programSubObjectsIndex++] = rasterizerSubObject;
    //programSubObjects[programSubObjectsIndex++] = blendSubObject;
    programSubObjects[programSubObjectsIndex++] = rtFormatsSubObject;

    LPCWSTR exports[] = { L"MeshMain", L"PixelMain" };
    D3D12_GENERIC_PROGRAM_DESC genericProgramDesc =
    {
        .ProgramName = L"GraphicsProgram",
        .NumExports = 2,
        .pExports = exports,
        .NumSubobjects = programSubObjectsIndex,
        .ppSubobjects = programSubObjects.Pointer
    };
    
    stateSubObjects[stateSubObjectsIndex++] = { D3D12_STATE_SUBOBJECT_TYPE_GENERIC_PROGRAM, &genericProgramDesc };

    D3D12_STATE_OBJECT_DESC stateObjectDesc = 
    {
        .Type = D3D12_STATE_OBJECT_TYPE_EXECUTABLE,
        .NumSubobjects = stateSubObjectsIndex,
        .pSubobjects = stateSubObjects.Pointer
    };

    ComPtr<ID3D12StateObject> stateObject;
    AssertIfFailed(graphicsDeviceData->Device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(stateObject.GetAddressOf())));

    ComPtr<ID3D12StateObjectProperties1> pSOProperties;
    AssertIfFailed(stateObject->QueryInterface(IID_PPV_ARGS(&pSOProperties)));
    auto programIdentifier = pSOProperties->GetProgramIdentifier(L"GraphicsProgram");*/

    // TODO: Temporary
    auto oldPso = CreateDirectX12OldPSO(graphicsDevice, parameters);

    if (DirectX12DebugLayerEnabled && parameters->DebugName)
    {
        oldPso->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, parameters->DebugName).Pointer);
    }

    auto handle = SystemAddDataPoolItem(directX12PipelineStatePool, {
        .PipelineState = oldPso,
        .PipelineStateType = DirectX12PipelineStateType_Graphics,
        //.ProgramIdentifier = programIdentifier
    }); 

    SystemAddDataPoolItemFull(directX12PipelineStatePool, handle, {
        //.StateObject = stateObject,
    });

    return handle;
}

ElemPipelineState DirectX12CompileComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    InitDirectX12ShaderMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData= GetDirectX12ShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);

    // TODO: Use the new pso model
    ComPtr<ID3D12PipelineState> pipelineState;

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.pRootSignature = graphicsDeviceData->RootSignature.Get();
    
    if (parameters->ComputeShaderFunction)
    {
        auto shaderByteCode = GetDirectX12ShaderFunctionByteCode(shaderLibraryData, ShaderType_Compute, parameters->ComputeShaderFunction);

        if (!shaderByteCode.pShaderBytecode)
        {
            return ELEM_HANDLE_NULL;
        }

        psoDesc.CS = shaderByteCode;
    }

	AssertIfFailed(graphicsDeviceData->Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.ReleaseAndGetAddressOf())));

    if (DirectX12DebugLayerEnabled && parameters->DebugName)
    {
        pipelineState->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, parameters->DebugName).Pointer);
    }

    auto handle = SystemAddDataPoolItem(directX12PipelineStatePool, {
        .PipelineState = pipelineState,
        .PipelineStateType = DirectX12PipelineStateType_Compute,
        //.ProgramIdentifier = programIdentifier
    }); 

    SystemAddDataPoolItemFull(directX12PipelineStatePool, handle, {
        //.StateObject = stateObject,
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

    pipelineStateDataFull->StateObject.Reset();
    pipelineStateData->PipelineState.Reset();

    SystemRemoveDataPoolItem(directX12PipelineStatePool, pipelineState);
}

void DirectX12BindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(pipelineState != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto pipelineStateData = GetDirectX12PipelineStateData(pipelineState);
    SystemAssert(pipelineStateData);

    D3D12_SET_PROGRAM_DESC programDesc =
    {
        .Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
        .GenericPipeline = 
        {
            .ProgramIdentifier = pipelineStateData->ProgramIdentifier
        }
    };

    commandListData->PipelineStateType = pipelineStateData->PipelineStateType;

    //commandListData->DeviceObject->SetProgram(&programDesc);
    commandListData->DeviceObject->SetPipelineState(pipelineStateData->PipelineState.Get());
}

void DirectX12PushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data)
{
    // TODO: Check if a pipeline state was bound
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    if (commandListData->PipelineStateType == DirectX12PipelineStateType_Graphics)
    {
        commandListData->DeviceObject->SetGraphicsRoot32BitConstants(0, data.Length / 4, data.Items, offsetInBytes / 4);
    }
    else
    {
        commandListData->DeviceObject->SetComputeRoot32BitConstants(0, data.Length / 4, data.Items, offsetInBytes / 4);
    }
}

void DirectX12DispatchCompute(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    if (!CheckDirectX12PipelineStateType(commandListData, DirectX12PipelineStateType_Compute))
    {
        return;
    }

    InsertDirectX12ResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_Compute);

    commandListData->DeviceObject->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
