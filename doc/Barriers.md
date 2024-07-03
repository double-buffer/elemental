# Barriers

## Decisions to make

- [ ] Do we use descriptors when describing a barrier?
- [ ] Do we use descriptor for BeginRenderTarget?
- [x] Do we automatically insert barriers in Begin/EndRender but allow overriding them?
- [ ] Do we separate layout transition from synchronization into separate functions? (Even if we specify
      layout and sync separately we could merge them in one barrier)

## Examples

All using the same command list.
Note that for textures even if are using separate commandlists (with separate execute), we still
need to transition the layout.

1. Texture: Generate background in compute => Render geometry to RTV => post process in compute shader

```c
    // Transition from ??? to UAV
    // For buffer we don't care for the sync and access because we start the execution of a new
    // batch of commandlist

    // Lib code
    ElemGraphicsResourceBarrier(commandList, &(ElemGraphicsResourceBarrierParameters) {
        .DestinationDescriptor = RenderTextureUav,

    });

    // Ideal barrier
    barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
    barrier.AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED; // or D3D12_BARRIER_LAYOUT_COMMON?
    barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;

    ElemBindPipelineState(backgroundComputePso)
    ElemPushConstants// with UAV
    ElemDispatchCompute(...)

    // Transition from UAV to RTV?
    // Here if we have an automatic barrier management in BeginRenderPass, we miss the opportunity
    // to sync with compute only
    // Also, we don't know the previous layout and access of the texture

    // Ideal barrier
    barrier.SyncBefore = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    barrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;

    ElemBeginRenderPass // with RTV resource (do we use descriptor?)
    ElemEndRenderPass

    // Transition from RTV to SRV?

    // Ideal barrier
    barrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    barrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;

    ElemBindPipelineState(computePso)
    ElemPushConstants // with SRV 
    ElemDispatchCompute(...)
```

2. Texture: RenderToTexture only

```c
    // Transition from ??? to RTV?
    // The sync and access before are not really needed here because it the the only command in the list
    // We need the barrier here because we need to change the layout of the texture

    // Ideal barrier
    // That barrier could be automatted by the beginrender pass
    barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
    barrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED;  // or D3D12_BARRIER_LAYOUT_COMMON?
    barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;

    ElemBeginRenderPass // with RTV resource (do we use descriptor?)
    ElemEndRenderPass
```
