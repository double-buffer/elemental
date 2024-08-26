#include "DirectX12Shader.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12Resource.h"
#include "DirectX12CommandList.h"
#include "Graphics/Resource.h"
#include "Graphics/ShaderReader.h"
#include "Graphics/Shader.h"
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

D3D12_FILL_MODE ConvertToDirectX12FillMode(ElemGraphicsFillMode fillMode)
{
    switch (fillMode) 
    {
        case ElemGraphicsFillMode_Solid:
            return D3D12_FILL_MODE_SOLID;

        case ElemGraphicsFillMode_Wireframe:
            return D3D12_FILL_MODE_WIREFRAME;
    }
}

D3D12_CULL_MODE ConvertToDirectX12CullMode(ElemGraphicsCullMode cullMode)
{
    switch (cullMode) 
    {
        case ElemGraphicsCullMode_BackFace:
            return D3D12_CULL_MODE_BACK;

        case ElemGraphicsCullMode_FrontFace:
            return D3D12_CULL_MODE_FRONT;

        case ElemGraphicsCullMode_None:
            return D3D12_CULL_MODE_NONE;
    }
}

D3D12_BLEND_OP ConvertToDirectX12BlendOperation(ElemGraphicsBlendOperation blendOperation)
{
    switch (blendOperation)
    {
        case ElemGraphicsBlendOperation_Add:
            return D3D12_BLEND_OP_ADD;

        case ElemGraphicsBlendOperation_Subtract:
            return D3D12_BLEND_OP_SUBTRACT;

        case ElemGraphicsBlendOperation_ReverseSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;

        case ElemGraphicsBlendOperation_Min:
            return D3D12_BLEND_OP_MIN;

        case ElemGraphicsBlendOperation_Max:
            return D3D12_BLEND_OP_MAX;
    }
}

D3D12_BLEND ConvertToDirectX12Blend(ElemGraphicsBlendFactor blendFactor)
{
    switch (blendFactor)
    {
        case ElemGraphicsBlendFactor_Zero:
            return D3D12_BLEND_ZERO;

        case ElemGraphicsBlendFactor_One:
            return D3D12_BLEND_ONE;

        case ElemGraphicsBlendFactor_SourceColor:
            return D3D12_BLEND_SRC_COLOR;

        case ElemGraphicsBlendFactor_InverseSourceColor:
            return D3D12_BLEND_INV_SRC_COLOR;

        case ElemGraphicsBlendFactor_SourceAlpha:
            return D3D12_BLEND_SRC_ALPHA;

        case ElemGraphicsBlendFactor_InverseSourceAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;

        case ElemGraphicsBlendFactor_DestinationColor:
            return D3D12_BLEND_DEST_COLOR;

        case ElemGraphicsBlendFactor_InverseDestinationColor:
            return D3D12_BLEND_INV_DEST_COLOR;

        case ElemGraphicsBlendFactor_DestinationAlpha:
            return D3D12_BLEND_DEST_ALPHA;

        case ElemGraphicsBlendFactor_InverseDestinationAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;

        case ElemGraphicsBlendFactor_SourceAlphaSaturated:
            return D3D12_BLEND_SRC_ALPHA_SAT;
    }
}

D3D12_COMPARISON_FUNC ConvertToDirectX12CompareFunction(ElemGraphicsCompareFunction compareFunction)
{
    switch (compareFunction)
    {
        case ElemGraphicsCompareFunction_Never:
            return D3D12_COMPARISON_FUNC_NEVER;

        case ElemGraphicsCompareFunction_Less:
            return D3D12_COMPARISON_FUNC_LESS;

        case ElemGraphicsCompareFunction_Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;

        case ElemGraphicsCompareFunction_LessEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;

        case ElemGraphicsCompareFunction_Greater:
            return D3D12_COMPARISON_FUNC_GREATER;

        case ElemGraphicsCompareFunction_NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;

        case ElemGraphicsCompareFunction_GreaterEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;

        case ElemGraphicsCompareFunction_Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
    }
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

    // TODO: Extract methods (this will be easier to switch to new PipelineState later)

    // TODO: Render targets
    D3D12_RT_FORMAT_ARRAY renderTargets = 
    {
        .NumRenderTargets = parameters->RenderTargets.Length
    };

    D3D12_BLEND_DESC blendState = 
    {
        .IndependentBlendEnable = false, // TODO: I think that one is needed
    };

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
    
        renderTargets.RTFormats[i] = ConvertToDirectX12TextureFormat(renderTargetParameters.Format);

        blendState.RenderTarget[i] =
        {
            .BlendEnable = IsBlendEnabled(renderTargetParameters),
            .SrcBlend = ConvertToDirectX12Blend(renderTargetParameters.SourceBlendFactor),
            .DestBlend = ConvertToDirectX12Blend(renderTargetParameters.DestinationBlendFactor),
            .BlendOp = ConvertToDirectX12BlendOperation(renderTargetParameters.BlendOperation),
            .SrcBlendAlpha = ConvertToDirectX12Blend(renderTargetParameters.SourceBlendFactorAlpha),
            .DestBlendAlpha = ConvertToDirectX12Blend(renderTargetParameters.DestinationBlendFactorAlpha),
            .BlendOpAlpha = ConvertToDirectX12BlendOperation(renderTargetParameters.BlendOperationAlpha),
            .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
        };
    }

    DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;
    D3D12_DEPTH_STENCIL_DESC2 depthStencilState = {};

    if (CheckDepthStencilFormat(parameters->DepthStencil.Format))
    {
        depthFormat = ConvertToDirectX12TextureFormat(parameters->DepthStencil.Format);
        depthStencilState.DepthEnable = true;

        if (!parameters->DepthStencil.DepthDisableWrite)
        {
            depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        }

        depthStencilState.DepthFunc = ConvertToDirectX12CompareFunction(parameters->DepthStencil.DepthCompareFunction);
    }

    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;

    D3D12_RASTERIZER_DESC2 rasterizerState = {};
    rasterizerState.FillMode = ConvertToDirectX12FillMode(parameters->FillMode);
    rasterizerState.CullMode = ConvertToDirectX12CullMode(parameters->CullMode);
    rasterizerState.FrontCounterClockwise = false;
    rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerState.DepthClipEnable = true;
    rasterizerState.LineRasterizationMode = D3D12_LINE_RASTERIZATION_MODE_ALIASED;

    // TODO: Check those 2
    rasterizerState.ForcedSampleCount = 0;
    rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    GraphicsPso psoDesc = {};
    psoDesc.RootSignature = graphicsDeviceData->RootSignature.Get();

    if (parameters->AmplificationShaderFunction)
    {
        auto shaderByteCode = GetDirectX12ShaderFunctionByteCode(shaderLibraryData, ShaderType_Amplification, parameters->AmplificationShaderFunction);

        if (!shaderByteCode.pShaderBytecode)
        {
            return ELEM_HANDLE_NULL;
        }

        psoDesc.AS = shaderByteCode;
    }

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
    psoDesc.DepthStencilFormat = depthFormat;
    psoDesc.DepthStencilState = depthStencilState;
    psoDesc.RasterizerState = rasterizerState;
    psoDesc.BlendState = blendState;
    psoDesc.SampleDesc = sampleDesc;

    D3D12_PIPELINE_STATE_STREAM_DESC psoStream = {};
    psoStream.SizeInBytes = sizeof(GraphicsPso);
    psoStream.pPipelineStateSubobjectStream = &psoDesc;

    ComPtr<ID3D12PipelineState> pipelineState;
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
