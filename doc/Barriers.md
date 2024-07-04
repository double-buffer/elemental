# Barriers

## Decisions to make

- [x] Do we use descriptors when describing a barrier?
- [ ] Do we use descriptor for BeginRenderTarget?
- [x] Do we automatically insert barriers in Begin/EndRender but allow overriding them?
- [ ] Do we separate layout transition from synchronization into separate functions? (Even if we specify
      layout and sync separately we could merge them in one barrier)
- [x] For sync points, only support Mesh and Pixel stages.
- [ ] Do we provide in the options a way to control accesses so we can control the cache flushes.
- [ ] After EndRenderPass, do we restore the previous layout and accesses or revert to read?

## Examples

All using the same command list.
Note that for textures even if are using separate commandlists (with separate execute), we still
need to transition the layout (for RTV)

We can deduce the sync before and access before by recording the last one in the command list data. 
We could allow the user to specify more fine grained stages if needed.

1. Texture: Generate background in compute => Render geometry to RTV => post process in compute shader

```c
    // Transition from ??? to UAV
    // For buffer we don't care for the sync and access because we start the execution of a new
    // batch of commandlist

    // Lib code
    ElemGraphicsResourceBarrier(commandList, renderTextureWriteDescriptor, NULL); // With options
    //ElemGraphicsResourceBarrier(commandList, renderTexture, ElemGraphicsResourceUsage_Write); // With options

    // Ideal barrier (Layout change only)
    // If the resource is in a queue specific common layout, we can use it directly as UAV
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
    
    // Lib code: None

    // Ideal barrier
    barrier.SyncBefore = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    barrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;

    ElemBeginRenderPass // with RTV resource (do we use descriptor?)
    ElemEndRenderPass

    // Lib code: 
    ElemSetGraphicsResourceBarrier(commandList, renderTextureReadDescriptor, NULL); // With options
    //ElemSetGraphicsResourceBarrier(commandList, renderTexture, ElemGraphicsResourceUsage_Read); // With options

    // Ideal barrier
    barrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    barrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE; // Important: If possible we need to use the queue specific layout

    ElemBindPipelineState(computePso)
    ElemPushConstants // with SRV 
    ElemDispatchCompute(...

                        )
    // Lib code: 
    ElemSetGraphicsResourceBarrier(commandList, renderTextureWriteDescriptor, NULL); // With options
    //ElemSetGraphicsResourceBarrier(commandList, renderTexture, ElemGraphicsResourceUsage_Read); // With options

    // Ideal barrier
    barrier.SyncBefore = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    barrier.AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE;
    barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS; // Important: If possible we need to use the queue specific layout

    ElemBindPipelineState(computePso)
    ElemPushConstants // with UAV 
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

3. List of interesting sync points

Sync points are important because while we can for each resource take by default the sequential sync points and for the 
after the command that will execute all the barriers. We could override that to allow the gpu to reorder multiple commands
if we need the resource at a later point.

But in real code I don't see someone declaring a barrier not logically between the sync points. So for now maybe we should focus only
on the real sync point that matter. Which is Vertex and Pixel shading. Those 2 are serially executed by the GPU and there is a real value
here.

```c
    D3D12_BARRIER_SYNC_NONE	= 0,
    D3D12_BARRIER_SYNC_ALL	= 0x1,
    D3D12_BARRIER_SYNC_DRAW	= 0x2,
    D3D12_BARRIER_SYNC_VERTEX_SHADING	= 0x8,
    D3D12_BARRIER_SYNC_PIXEL_SHADING	= 0x10,
    D3D12_BARRIER_SYNC_DEPTH_STENCIL	= 0x20,
    D3D12_BARRIER_SYNC_RENDER_TARGET	= 0x40,
    D3D12_BARRIER_SYNC_COMPUTE_SHADING	= 0x80,
    D3D12_BARRIER_SYNC_RAYTRACING	= 0x100,
    D3D12_BARRIER_SYNC_NON_PIXEL_SHADING	= 0x2000,
    D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO	= 0x4000,
    D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW	= 0x8000,
    D3D12_BARRIER_SYNC_VIDEO_DECODE	= 0x100000,
    D3D12_BARRIER_SYNC_VIDEO_PROCESS	= 0x200000,
    D3D12_BARRIER_SYNC_VIDEO_ENCODE	= 0x400000,
    D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE	= 0x800000,
    D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE	= 0x1000000,
```
