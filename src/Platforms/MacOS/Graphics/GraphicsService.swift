import Cocoa
import Metal
import QuartzCore.CAMetalLayer
import NativeElemental

let sharedEventListener: MTLSharedEventListener = MTLSharedEventListener()

@_cdecl("Native_InitGraphicsService")
public func initGraphicsService(optionsPointer: UnsafePointer<GraphicsServiceOptions>) {
}

@_cdecl("Native_FreeGraphicsService")
public func freeGraphicsService() {
}

@_cdecl("Native_GetAvailableGraphicsDevices")
public func getAvailableGraphicsDevices(graphicsDevices: UnsafeMutablePointer<GraphicsDeviceInfo>, graphicsDeviceCount: UnsafeMutablePointer<Int>) {
    let devices = MTLCopyAllDevices()
    var i = 0

    for device in devices {
        graphicsDevices[i] = constructGraphicsDeviceInfo(device)
        i += 1
    }
    
    graphicsDeviceCount.pointee = devices.count
}

@_cdecl("Native_CreateGraphicsDevice")
public func createGraphicsDevice(optionsPointer: UnsafePointer<GraphicsDeviceOptions>) -> UnsafeMutableRawPointer? {
    var initMetalDevice: MTLDevice? = nil
    let options = optionsPointer.pointee

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
    let initMetalCommandQueue = graphicsDevice.metalDevice.makeCommandQueue(maxCommandBufferCount: 100)

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
    let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)

    guard let metalCommandBuffer = commandQueue.deviceObject.makeCommandBufferWithUnretainedReferences() else {
        return nil
    }

    let commandList = MetalCommandList(commandQueue.metalDevice, metalCommandBuffer)
    return Unmanaged.passRetained(commandList).toOpaque()
}

@_cdecl("Native_FreeCommandList")
public func freeCommandList(_ commandListPointer: UnsafeRawPointer) {
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
    commandList.commandEncoder = nil
}

@_cdecl("Native_ExecuteCommandLists")
public func executeCommandLists(commandQueuePointer: UnsafeRawPointer, commandLists: UnsafePointer<UnsafeRawPointer>, commandListCount: Int, fencesToWait: UnsafePointer<Fence>, fenceToWaitCount: Int) -> Fence {
    let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)
    var fenceValue = UInt64(0)

    if (fenceToWaitCount > 0) {
        // TODO: Can we avoid creating an empty command buffer?
        guard let waitCommandBuffer = commandQueue.deviceObject.makeCommandBufferWithUnretainedReferences() else {
            print("executeCommandLists: Error while creating wait command buffer object.")
            return Fence()
        }

        waitCommandBuffer.label = "WaitCommandBuffer"

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
        freeCommandList(commandLists[i])
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
public func createSwapChain(windowPointer: UnsafeRawPointer, commandQueuePointer: UnsafeRawPointer, optionsPointer: UnsafePointer<SwapChainOptions>) -> UnsafeMutableRawPointer? {
    let window = MacOSWindow.fromPointer(windowPointer)
    let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)
    let options = optionsPointer.pointee

    // TODO: This code is only working on MacOS!
    let contentView = window.window.contentView! as NSView
    let metalView = MacOSMetalView()
    metalView.translatesAutoresizingMaskIntoConstraints = false
    metalView.frame = contentView.frame
    contentView.addSubview(metalView)

    // TODO: Can we use something else to apply the contrains?
    contentView.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "|[metalView]|", options: [], metrics: nil, views: ["metalView" : metalView]))
    contentView.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "V:|[metalView]|", options: [], metrics: nil, views: ["metalView" : metalView]))

    // TODO: Check options
    let frameLatency = 2
    let renderSize = getWindowRenderSize(windowPointer)

    let metalLayer = metalView.metalLayer
    metalLayer.device = commandQueue.metalDevice
    metalLayer.pixelFormat = .bgra8Unorm_srgb
    metalLayer.framebufferOnly = true
    metalLayer.allowsNextDrawableTimeout = true
    metalLayer.displaySyncEnabled = true
    metalLayer.maximumDrawableCount = 3
    metalLayer.drawableSize = CGSize(width: Int(renderSize.Width), height: Int(renderSize.Height))

    let presentSemaphore = DispatchSemaphore.init(value: frameLatency);

    //let descriptor = createTextureDescriptor(textureFormat, RenderTarget, width, height, 1, 1, 1)

    let swapChain = MetalSwapChain(commandQueue.metalDevice, metalLayer, commandQueue, presentSemaphore)
    return Unmanaged.passRetained(swapChain).toOpaque()
}

@_cdecl("Native_FreeSwapChain")
public func freeSwapChain(swapChainPointer: UnsafeRawPointer) {
    MetalSwapChain.release(swapChainPointer)
}
    
@_cdecl("Native_ResizeSwapChain")
public func resizeSwapChain(swapChainPointer: UnsafeRawPointer, width: Int, height: Int) {
    let swapChain = MetalSwapChain.fromPointer(swapChainPointer)
    swapChain.deviceObject.drawableSize = CGSize(width: width, height: height)
}

@_cdecl("Native_GetSwapChainBackBufferTexture")
public func getSwapChainBackBufferTexture(swapChainPointer: UnsafeRawPointer) -> UnsafeMutableRawPointer? {
    let swapChain = MetalSwapChain.fromPointer(swapChainPointer)

    guard let backBufferTexture = swapChain.backBufferTexture else {
        print("getSwapChainBackBufferTexture: backBufferTexture is null")
        return nil
    }

    return Unmanaged.passRetained(backBufferTexture).toOpaque()
}

@_cdecl("Native_PresentSwapChain")
public func presentSwapChain(swapChainPointer: UnsafeRawPointer) {
    let swapChain = MetalSwapChain.fromPointer(swapChainPointer)

    // TODO: Can we avoid creating an empty command buffer?
    guard let commandBuffer = swapChain.commandQueue.deviceObject.makeCommandBufferWithUnretainedReferences() else {
        print("presentSwapChain: Error while creating command buffer object.")
        return
    }

    guard let backBufferDrawable = swapChain.backBufferDrawable else {
        return
    }

    let presentSemaphore = swapChain.presentSemaphore

    commandBuffer.addCompletedHandler { cb in
        presentSemaphore.signal()
    }

    commandBuffer.label = "PresentSwapChainCommandBuffer"
    commandBuffer.present(backBufferDrawable)

    commandBuffer.commit()

    swapChain.backBufferTexture = nil
    swapChain.backBufferDrawable = nil
}

@_cdecl("Native_WaitForSwapChainOnCpu")
public func waitForSwapChainOnCpu(swapChainPointer: UnsafeRawPointer) {
    let swapChain = MetalSwapChain.fromPointer(swapChainPointer)
    swapChain.presentSemaphore.wait()
    
    if (swapChain.backBufferDrawable == nil) {
        guard let nextMetalDrawable = swapChain.deviceObject.nextDrawable() else {
            print("waitForSwapChainOnCpu: Cannot acquire a back buffer")
            return
        }

        let texture = MetalTexture(swapChain.metalDevice, nextMetalDrawable.texture)
        texture.isPresentTexture = true

        swapChain.backBufferTexture = texture
        swapChain.backBufferDrawable = nextMetalDrawable
    }
}
    
@_cdecl("Native_BeginRenderPass")
public func beginRenderPass(commandListPointer: UnsafeRawPointer, renderPassDescriptorPointer: UnsafePointer<RenderPassDescriptor>) {
    let commandList = MetalCommandList.fromPointer(commandListPointer)

    // TODO: Add an util function that checks the current encoder and create a new one if it can.
    // It will also check if the command queue is compatible
    if (commandList.commandEncoder != nil) {
        commandList.commandEncoder!.endEncoding()
        commandList.commandEncoder = nil
    }

    let renderPassDescriptor = renderPassDescriptorPointer.pointee
    let metalRenderPassDescriptor = MTLRenderPassDescriptor()

    if (renderPassDescriptor.RenderTarget0.HasValue) {
        initRenderPassColorDescriptor(metalRenderPassDescriptor.colorAttachments[0], renderPassDescriptor.RenderTarget0.Value)
    }

    // TODO: other render targets + Depth buffer
    
    guard let renderCommandEncoder = commandList.deviceObject.makeRenderCommandEncoder(descriptor: metalRenderPassDescriptor) else {
        print("beginRenderPass: Render command encoder creation failed.")
        return
    }

    commandList.commandEncoder = renderCommandEncoder

    /*
    if (renderPassDescriptor.DepthBufferOperation == Write || renderPassDescriptor.DepthBufferOperation == ClearWrite) {
        renderCommandEncoder.setDepthStencilState(self.depthWriteOperationState)
    } else if (renderPassDescriptor.DepthBufferOperation == CompareEqual) {
        renderCommandEncoder.setDepthStencilState(self.depthCompareEqualState)
    } else if (renderPassDescriptor.DepthBufferOperation == CompareGreater) {
        renderCommandEncoder.setDepthStencilState(self.depthCompareGreaterState)
    } else {
        renderCommandEncoder.setDepthStencilState(self.depthNoneOperationState)
    }

    if (renderPassDescriptor.BackfaceCulling == 1) {
        renderCommandEncoder.setCullMode(.back)
    } else {
        renderCommandEncoder.setCullMode(.none)
    }*/

    // TODO: Use an actual viewport object and if null do that
    if (metalRenderPassDescriptor.colorAttachments[0].texture != nil) {
        renderCommandEncoder.setViewport(MTLViewport(originX: 0.0, originY: 0.0, width: Double(metalRenderPassDescriptor.colorAttachments[0].texture!.width), height: Double(metalRenderPassDescriptor.colorAttachments[0].texture!.height), znear: 0.0, zfar: 1.0))
        renderCommandEncoder.setScissorRect(MTLScissorRect(x: 0, y: 0, width: metalRenderPassDescriptor.colorAttachments[0].texture!.width, height: metalRenderPassDescriptor.colorAttachments[0].texture!.height))
    }/* else if (metalRenderPassDescriptor.depthAttachment.texture != nil) {
        renderCommandEncoder.setViewport(MTLViewport(originX: 0.0, originY: 0.0, width: Double(metalRenderPassDescriptor.depthAttachment.texture!.width), height: Double(metalRenderPassDescriptor.depthAttachment.texture!.height), znear: 0.0, zfar: 1.0))
        renderCommandEncoder.setScissorRect(MTLScissorRect(x: 0, y: 0, width: metalRenderPassDescriptor.depthAttachment.texture!.width, height: metalRenderPassDescriptor.depthAttachment.texture!.height))
    }*/
}
    
@_cdecl("Native_EndRenderPass")
public func endRenderPass(commandListPointer: UnsafeRawPointer) {
    let commandList = MetalCommandList.fromPointer(commandListPointer)

    guard let commandEncoder = commandList.commandEncoder else {
        return
    }
    
    commandEncoder.endEncoding()
    commandList.commandEncoder = nil
}

private func constructGraphicsDeviceInfo(_ metalDevice: MTLDevice) -> GraphicsDeviceInfo {
    return GraphicsDeviceInfo(DeviceName: convertString(from: metalDevice.name),
                                GraphicsApi: GraphicsApi_Metal,
                                DeviceId: UInt64(metalDevice.registryID),
                                AvailableMemory: UInt64(metalDevice.recommendedMaxWorkingSetSize))
}

private func initRenderPassColorDescriptor(_ descriptor: MTLRenderPassColorAttachmentDescriptor, _ renderTargetDescriptor: RenderPassRenderTarget) {
    let texture = MetalTexture.fromPointer(renderTargetDescriptor.TexturePointer)

    descriptor.texture = texture.deviceObject

    if (texture.isPresentTexture) {
        descriptor.loadAction = .dontCare
        descriptor.storeAction = .store
    } /*else {
        let resourceFence = self.graphicsDevice.makeFence()!
        colorTexture.resourceFence = resourceFence

        commandList.resourceFences.append(resourceFence)

        metalRenderPassDescriptor.colorAttachments[0].storeAction = .store
    }*/

    // TODO: Texture transitions with metal fences
    
    if (renderTargetDescriptor.ClearColor.HasValue) {
        let clearColor = renderTargetDescriptor.ClearColor.Value

        descriptor.loadAction = .clear
        descriptor.clearColor = MTLClearColor.init(red: Double(clearColor.X), green: Double(clearColor.Y), blue: Double(clearColor.Z), alpha: Double(clearColor.W))
    }
}

private func convertString(from str: String) -> UnsafeMutablePointer<UInt8> {
    var utf8 = Array(str.utf8)
    utf8.append(0)  // adds null character
    let count = utf8.count
    let result = UnsafeMutableBufferPointer<UInt8>.allocate(capacity: count)
    _ = result.initialize(from: utf8)
    return result.baseAddress!
}
