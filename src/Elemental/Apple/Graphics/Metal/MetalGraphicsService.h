#pragma once

#include "ElementalOld.h"

#include "MetalShader.h"

GraphicsDeviceInfo ConstructGraphicsDeviceInfo(NS::SharedPtr<MTL::Device> device);
void MetalDeletePipelineCacheItem(uint64_t key, void* data);
void InitRenderPassDescriptor(MTL::RenderPassColorAttachmentDescriptor* metalDescriptor, RenderPassRenderTarget* renderTargetDescriptor);
uint64_t ComputeRenderPipelineStateHash(MetalShader* shader, RenderPassDescriptor* renderPassDescriptor);
MetalPipelineStateCacheItem CreateRenderPipelineState(MetalShader* shader, RenderPassDescriptor* renderPassDescriptor);
