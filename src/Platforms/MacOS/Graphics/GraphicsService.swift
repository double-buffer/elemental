import Cocoa
import Metal
import QuartzCore.CAMetalLayer
import NativeElemental

let sharedEventListener: MTLSharedEventListener = MTLSharedEventListener()

@_cdecl("Native_InitGraphicsService")
public func initGraphicsService(options: GraphicsServiceOptions) {
}

@_cdecl("Native_FreeGraphicsService")
public func freeGraphicsService() {
}

@_cdecl("Native_GetAvailableGraphicsDevices")
public func Native_GetAvailableGraphicsDevices(graphicsDevices: UnsafeMutablePointer<GraphicsDeviceInfo>, graphicsDeviceCount: UnsafeMutablePointer<Int>) {
    let devices = MTLCopyAllDevices()
    var i = 0

    for device in devices {
        graphicsDevices[i] = constructGraphicsDeviceInfo(device)
        i += 1
    }
    
    graphicsDeviceCount.pointee = devices.count
}

@_cdecl("Native_CreateGraphicsDevice")
public func createGraphicsDevice(options: GraphicsDeviceOptions) -> UnsafeMutableRawPointer? {
    var initMetalDevice: MTLDevice? = nil

    if (options.DeviceId != 0) {
        let devices = MTLCopyAllDevices()
        
        for device in devices {
            if (options.DeviceId == device.registryID) {
                initMetalDevice = device
                break
            }
        }
    } else {
        initMetalDevice = MTLCreateSystemDefaultDevice()
    }

    guard let metalDevice = initMetalDevice else {
        return nil
    }

    let graphicsDevice = MetalGraphicsDevice(metalDevice)
    return Unmanaged.passRetained(graphicsDevice).toOpaque()
}

@_cdecl("Native_FreeGraphicsDevice")
public func freeGraphicsDevice(graphicsDevicePointer: UnsafeRawPointer) {
    MetalGraphicsDevice.release(graphicsDevicePointer)
}

@_cdecl("Native_GetGraphicsDeviceInfo")
public func getGraphicsDeviceInfo(graphicsDevicePointer: UnsafeRawPointer) -> GraphicsDeviceInfo {
    let graphicsDevice = MetalGraphicsDevice.fromPointer(graphicsDevicePointer)
    return constructGraphicsDeviceInfo(graphicsDevice.metalDevice)
}

@_cdecl("Native_CreateCommandQueue")
public func createCommandQueue(graphicsDevicePointer: UnsafeRawPointer, type: CommandQueueType) -> UnsafeMutableRawPointer? {
    let graphicsDevice = MetalGraphicsDevice.fromPointer(graphicsDevicePointer)
    let initMetalCommandQueue = graphicsDevice.metalDevice.makeCommandQueue()

    guard 
        let metalCommandQueue = initMetalCommandQueue,
        let fence = graphicsDevice.metalDevice.makeSharedEvent()
    else {
        return nil
    }

    let commandQueue = MetalCommandQueue(graphicsDevice.metalDevice, metalCommandQueue, fence)
    return Unmanaged.passRetained(commandQueue).toOpaque()
}

@_cdecl("Native_FreeCommandQueue")
public func freeCommandQueue(commandQueuePointer: UnsafeRawPointer) {
    MetalCommandQueue.release(commandQueuePointer)
}

@_cdecl("Native_SetCommandQueueLabel")
public func setCommandQueueLabel(commandQueuePointer: UnsafeRawPointer, label: UnsafeMutablePointer<Int8>) {
    let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)
    commandQueue.deviceObject.label = String(cString: label)
}

@_cdecl("Native_CreateCommandList")
public func createCommandList(commandQueuePointer: UnsafeRawPointer) -> UnsafeMutableRawPointer? {
    // TODO: We should reuse on the same queue and on the same thread the commandbuffer for multiple
    // commandlists.
    // Commandlists in metal are the encoders, command buffers are the allocators
    let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)
    
    print("Create Command buffer")
    guard let metalCommandBuffer = commandQueue.deviceObject.makeCommandBufferWithUnretainedReferences() else {
        return nil
    }

    let commandList = MetalCommandList(commandQueue.metalDevice, metalCommandBuffer)
    return Unmanaged.passRetained(commandList).toOpaque()
}

@_cdecl("Native_FreeCommandList")
public func freeCommandList(commandListPointer: UnsafeRawPointer) {
    MetalCommandList.release(commandListPointer)
}

@_cdecl("Native_SetCommandListLabel")
public func setCommandListLabel(commandListPointer: UnsafeRawPointer, label: UnsafeMutablePointer<Int8>) {
    let commandList = MetalCommandList.fromPointer(commandListPointer)
    commandList.deviceObject.label = String(cString: label)
}

@_cdecl("Native_CommitCommandList")
public func commitCommandList(commandListPointer: UnsafeRawPointer) {
    let commandList = MetalCommandList.fromPointer(commandListPointer)

    guard let commandEncoder = commandList.commandEncoder else {
        return
    }
    
    commandEncoder.endEncoding()
}

@_cdecl("Native_ExecuteCommandLists")
public func executeCommandLists(commandQueuePointer: UnsafeRawPointer, commandLists: UnsafePointer<UnsafeRawPointer>, commandListCount: Int, fencesToWait: UnsafePointer<Fence>, fenceToWaitCount: Int) -> Fence {
    let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)
    var fenceValue = UInt64(0)

    if (fenceToWaitCount > 0) {
        guard let waitCommandBuffer = commandQueue.deviceObject.makeCommandBufferWithUnretainedReferences() else {
            print("executeCommandLists: Error while creating wait command buffer object.")
            return Fence()
        }

        for i in 0..<fenceToWaitCount {
            let fenceToWait = fencesToWait[i]
            let commandQueueToWait = MetalCommandQueue.fromPointer(fenceToWait.CommandQueuePointer)

            waitCommandBuffer.encodeWaitForEvent(commandQueueToWait.fence, value: UInt64(fenceToWait.FenceValue))
            waitCommandBuffer.commit()
        }
    }

    for i in 0..<commandListCount {
        let commandList = MetalCommandList.fromPointer(commandLists[i])

        if (i == commandListCount - 1) {
            // TODO: Atomic inc with https://github.com/apple/swift-atomics
            fenceValue = commandQueue.fenceValue
            commandList.deviceObject.encodeSignalEvent(commandQueue.fence, value: fenceValue)
            commandQueue.fenceValue = commandQueue.fenceValue + 1
        }

        commandList.deviceObject.commit()
    }

    var fence = Fence()
    fence.CommandQueuePointer = UnsafeMutableRawPointer(mutating: commandQueuePointer)
    fence.FenceValue = fenceValue
    return fence
}
    
@_cdecl("Native_WaitForFenceOnCpu")
public func waitForFenceOnCpu(fence: Fence) {
    let commandQueueToWait = MetalCommandQueue.fromPointer(fence.CommandQueuePointer)

    if (commandQueueToWait.fence.signaledValue < fence.FenceValue) {
        // TODO: Review that?
        let group = DispatchGroup()
        group.enter()

        commandQueueToWait.fence.notify(sharedEventListener, atValue: UInt64(fence.FenceValue)) { (sEvent, value) in
            group.leave()
        }

        group.wait()
    }
}
    
@_cdecl("Native_CreateSwapChain")
public func createSwapChain(windowPointer: UnsafeRawPointer, commandQueuePointer: UnsafeRawPointer, options: SwapChainOptions) -> UnsafeMutableRawPointer? {
    let window = MacOSWindow.fromPointer(windowPointer)
    let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)

    let contentView = window.window.contentView! as NSView
    let metalView = MacOSMetalView()
    metalView.frame = contentView.frame
    contentView.addSubview(metalView)

    // TODO: Check options
    let renderSize = getWindowRenderSize(windowPointer)

    let metalLayer = metalView.metalLayer
    metalLayer.device = commandQueue.metalDevice
    metalLayer.pixelFormat = .bgra8Unorm_srgb
    metalLayer.framebufferOnly = true
    metalLayer.allowsNextDrawableTimeout = true
    metalLayer.displaySyncEnabled = true
    metalLayer.maximumDrawableCount = 2
    metalLayer.drawableSize = CGSize(width: Int(renderSize.Width), height: Int(renderSize.Height))

    //let descriptor = createTextureDescriptor(textureFormat, RenderTarget, width, height, 1, 1, 1)

    let swapChain = MetalSwapChain(commandQueue.metalDevice, metalLayer)
    return Unmanaged.passRetained(swapChain).toOpaque()
}

@_cdecl("Native_FreeSwapChain")
public func freeSwapChain(swapChainPointer: UnsafeRawPointer) {
    MetalSwapChain.release(swapChainPointer)
}

private func constructGraphicsDeviceInfo(_ metalDevice: MTLDevice) -> GraphicsDeviceInfo {
    return GraphicsDeviceInfo(DeviceName: convertString(from: metalDevice.name),
                                GraphicsApi: GraphicsApi_Metal,
                                DeviceId: UInt64(metalDevice.registryID),
                                AvailableMemory: UInt64(metalDevice.recommendedMaxWorkingSetSize))
}

private func convertString(from str: String) -> UnsafeMutablePointer<UInt8> {
    var utf8 = Array(str.utf8)
    utf8.append(0)  // adds null character
    let count = utf8.count
    let result = UnsafeMutableBufferPointer<UInt8>.allocate(capacity: count)
    _ = result.initialize(from: utf8)
    return result.baseAddress!
}