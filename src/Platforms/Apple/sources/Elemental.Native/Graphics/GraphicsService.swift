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
    autoreleasepool {
        let devices = MTLCopyAllDevices()
        var i = 0

        for device in devices {
            graphicsDevices[i] = constructGraphicsDeviceInfo(device)
            i += 1
        }
        
        graphicsDeviceCount.pointee = devices.count
    }
}

@_cdecl("Native_CreateGraphicsDevice")
public func createGraphicsDevice(optionsPointer: UnsafePointer<GraphicsDeviceOptions>) -> UnsafeMutableRawPointer? {
    autoreleasepool {
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
        return graphicsDevice.toPointer()
    }
}

@_cdecl("Native_FreeGraphicsDevice")
public func freeGraphicsDevice(graphicsDevicePointer: UnsafeRawPointer) {
    autoreleasepool {
        MetalGraphicsDevice.release(graphicsDevicePointer)
    }
}

@_cdecl("Native_GetGraphicsDeviceInfo")
public func getGraphicsDeviceInfo(graphicsDevicePointer: UnsafeRawPointer) -> GraphicsDeviceInfo {
    autoreleasepool {
        let graphicsDevice = MetalGraphicsDevice.fromPointer(graphicsDevicePointer)
        return constructGraphicsDeviceInfo(graphicsDevice.metalDevice)
    }
}

@_cdecl("Native_CreateCommandQueue")
public func createCommandQueue(graphicsDevicePointer: UnsafeRawPointer, type: CommandQueueType) -> UnsafeMutableRawPointer? {
    autoreleasepool {
        let graphicsDevice = MetalGraphicsDevice.fromPointer(graphicsDevicePointer)
        let initMetalCommandQueue = graphicsDevice.metalDevice.makeCommandQueue(maxCommandBufferCount: 100)

        guard 
            let metalCommandQueue = initMetalCommandQueue,
            let fence = graphicsDevice.metalDevice.makeSharedEvent()
        else {
            return nil
        }

        let commandQueue = MetalCommandQueue(graphicsDevice.metalDevice, metalCommandQueue, fence)
        return commandQueue.toPointer()
    }
}

@_cdecl("Native_FreeCommandQueue")
public func freeCommandQueue(commandQueuePointer: UnsafeRawPointer) {
    autoreleasepool {
        MetalCommandQueue.release(commandQueuePointer)
    }
}

@_cdecl("Native_SetCommandQueueLabel")
public func setCommandQueueLabel(commandQueuePointer: UnsafeRawPointer, label: UnsafeMutablePointer<Int8>) {
    autoreleasepool {
        let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)
        commandQueue.deviceObject.label = String(cString: label)
    }
}

@_cdecl("Native_CreateCommandList")
public func createCommandList(commandQueuePointer: UnsafeRawPointer) -> UnsafeMutableRawPointer? {
    autoreleasepool {
        let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)

        guard let metalCommandBuffer = commandQueue.deviceObject.makeCommandBufferWithUnretainedReferences() else {
            return nil
        }

        let commandList = MetalCommandList(commandQueue.metalDevice, metalCommandBuffer)
        return commandList.toPointer()
    }
}

@_cdecl("Native_FreeCommandList")
public func freeCommandList(_ commandListPointer: UnsafeRawPointer) {
    autoreleasepool {
        MetalCommandList.release(commandListPointer)
    }
}

@_cdecl("Native_SetCommandListLabel")
public func setCommandListLabel(commandListPointer: UnsafeRawPointer, label: UnsafeMutablePointer<Int8>) {
    autoreleasepool {
        let commandList = MetalCommandList.fromPointer(commandListPointer)
        commandList.deviceObject.label = String(cString: label)
    }
}

@_cdecl("Native_CommitCommandList")
public func commitCommandList(commandListPointer: UnsafeRawPointer) {
    autoreleasepool {
        let commandList = MetalCommandList.fromPointer(commandListPointer)

        guard let commandEncoder = commandList.commandEncoder else {
            return
        }
        
        commandEncoder.endEncoding()
        commandList.commandEncoder = nil
    }
}

@_cdecl("Native_ExecuteCommandLists")
public func executeCommandLists(commandQueuePointer: UnsafeRawPointer, commandLists: UnsafePointer<UnsafeRawPointer>, commandListCount: Int, fencesToWait: UnsafePointer<Fence>, fenceToWaitCount: Int) -> Fence {
    autoreleasepool {
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
                fenceValue = commandQueue.fenceValue.loadThenWrappingIncrement(ordering: .relaxed)
                commandList.deviceObject.encodeSignalEvent(commandQueue.fence, value: fenceValue)
            }

            commandList.deviceObject.commit()
            commandList.commandEncoder = nil
        }

        var fence = Fence()
        fence.CommandQueuePointer = UnsafeMutableRawPointer(mutating: commandQueuePointer)
        fence.FenceValue = fenceValue
        return fence
    }
}
    
@_cdecl("Native_WaitForFenceOnCpu")
public func waitForFenceOnCpu(fence: Fence) {
    autoreleasepool {
        let commandQueueToWait = MetalCommandQueue.fromPointer(fence.CommandQueuePointer)

        if (commandQueueToWait.fence.signaledValue < fence.FenceValue) {
            // HACK: Review that?
            let group = DispatchGroup()
            group.enter()

            commandQueueToWait.fence.notify(sharedEventListener, atValue: UInt64(fence.FenceValue)) { (sEvent, value) in
                group.leave()
            }

            group.wait()
        }
    }
}

@_cdecl("Native_ResetCommandAllocation")
public func resetCommandAllocation(graphicsDevicePointer: UnsafeRawPointer) {
}
    
@_cdecl("Native_CreateSwapChain")
public func createSwapChain(windowPointer: UnsafeRawPointer, commandQueuePointer: UnsafeRawPointer, optionsPointer: UnsafePointer<SwapChainOptions>) -> UnsafeMutableRawPointer? {
    autoreleasepool {
        let window = MacOSWindow.fromPointer(windowPointer)
        let commandQueue = MetalCommandQueue.fromPointer(commandQueuePointer)
        let options = optionsPointer.pointee

        // TODO: This code is only working on MacOS!
        let contentView = window.window.contentView! as NSView
        let metalView = MacOSMetalView()
        metalView.translatesAutoresizingMaskIntoConstraints = false
        metalView.frame = contentView.frame
        contentView.addSubview(metalView)

        // HACK: Can we use something else to apply the contrains?
        contentView.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "|[metalView]|", options: [], metrics: nil, views: ["metalView" : metalView]))
        contentView.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "V:|[metalView]|", options: [], metrics: nil, views: ["metalView" : metalView]))

        var renderSize = getWindowRenderSize(windowPointer)

        if (options.Width != 0) {
            renderSize.Width = options.Width
        }
        
        if (options.Height != 0) {
            renderSize.Height = options.Height
        }

        let metalLayer = metalView.metalLayer
        metalLayer.device = commandQueue.metalDevice
        metalLayer.pixelFormat = options.Format == SwapChainFormat_HighDynamicRange ? .rgba16Float : .bgra8Unorm_srgb
        metalLayer.framebufferOnly = true
        metalLayer.allowsNextDrawableTimeout = true
        metalLayer.displaySyncEnabled = true
        metalLayer.maximumDrawableCount = 3
        metalLayer.drawableSize = CGSize(width: Int(renderSize.Width), height: Int(renderSize.Height))

        let presentSemaphore = DispatchSemaphore.init(value: Int(options.MaximumFrameLatency));

        let swapChain = MetalSwapChain(commandQueue.metalDevice, metalLayer, commandQueue, presentSemaphore)
        return swapChain.toPointer()
    }
}

@_cdecl("Native_FreeSwapChain")
public func freeSwapChain(swapChainPointer: UnsafeRawPointer) {
    autoreleasepool {
        MetalSwapChain.release(swapChainPointer)
    }
}
    
@_cdecl("Native_ResizeSwapChain")
public func resizeSwapChain(swapChainPointer: UnsafeRawPointer, width: Int, height: Int) {
    autoreleasepool {
        let swapChain = MetalSwapChain.fromPointer(swapChainPointer)
        swapChain.deviceObject.drawableSize = CGSize(width: width, height: height)
    }
}

@_cdecl("Native_GetSwapChainBackBufferTexture")
public func getSwapChainBackBufferTexture(swapChainPointer: UnsafeRawPointer) -> UnsafeMutableRawPointer? {
    autoreleasepool {
        let swapChain = MetalSwapChain.fromPointer(swapChainPointer)

        guard let backBufferDrawable = swapChain.backBufferDrawable else {
            print("getSwapChainBackBufferTexture: backBufferDrawable is null")
            return nil
        }
        
        let texture = MetalTexture(swapChain.metalDevice, backBufferDrawable.texture)
        texture.isPresentTexture = true

        return texture.toPointer()
    }
}

@_cdecl("Native_PresentSwapChain")
public func presentSwapChain(swapChainPointer: UnsafeRawPointer) {
    autoreleasepool {
        let swapChain = MetalSwapChain.fromPointer(swapChainPointer)

        // HACK: Can we avoid creating an empty command buffer?
        guard let commandBuffer = swapChain.commandQueue.deviceObject.makeCommandBufferWithUnretainedReferences() else {
            print("presentSwapChain: Error while creating command buffer object.")
            return
        }

        guard let backBufferDrawable = swapChain.backBufferDrawable else {
            print("presentSwapChain: Cannot get drawable")
            return
        }
                
        swapChain.backBufferDrawable = nil

        commandBuffer.addScheduledHandler { cb in
            autoreleasepool {
                let localDrawable = backBufferDrawable
                localDrawable.present()
            }
        }

        commandBuffer.addCompletedHandler { cb in
            autoreleasepool {
                let localPresentSemaphore = swapChain.presentSemaphore
                localPresentSemaphore.signal()
            }
        }

        commandBuffer.label = "PresentSwapChainCommandBuffer"
        commandBuffer.commit()
    }
}

@_cdecl("Native_WaitForSwapChainOnCpu")
public func waitForSwapChainOnCpu(swapChainPointer: UnsafeRawPointer) {
    autoreleasepool {
        let swapChain = MetalSwapChain.fromPointer(swapChainPointer)
        swapChain.presentSemaphore.wait()
        
        guard let nextMetalDrawable = swapChain.deviceObject.nextDrawable() else {
            print("waitForSwapChainOnCpu: Cannot acquire a back buffer")
            return
        }

        swapChain.backBufferDrawable = nextMetalDrawable
    }
}

@_cdecl("Native_FreeTexture")
public func freeTexture(texturePointer: UnsafeRawPointer) {
    autoreleasepool {
        MetalTexture.release(texturePointer)
    }
}
    
@_cdecl("Native_BeginRenderPass")
public func beginRenderPass(commandListPointer: UnsafeRawPointer, renderPassDescriptorPointer: UnsafePointer<RenderPassDescriptor>) {
    autoreleasepool {
        let commandList = MetalCommandList.fromPointer(commandListPointer)

        // TODO: Add an util function that checks the current encoder and create a new one if it can.
        // It will also check if the command queue is compatible
        if (commandList.commandEncoder != nil) {
            commandList.commandEncoder!.endEncoding()
            commandList.commandEncoder = nil
        }

        // BUG: There is a memory leak when encoding more than one render pass with drawable
        let renderPassDescriptor = renderPassDescriptorPointer.pointee
        let metalRenderPassDescriptor = MTLRenderPassDescriptor()

        if (renderPassDescriptor.RenderTarget0.HasValue) {
            initRenderPassColorDescriptor(metalRenderPassDescriptor.colorAttachments[0], renderPassDescriptor.RenderTarget0.Value)
        }
        
        if (renderPassDescriptor.RenderTarget1.HasValue) {
            initRenderPassColorDescriptor(metalRenderPassDescriptor.colorAttachments[1], renderPassDescriptor.RenderTarget1.Value)
        }
        
        if (renderPassDescriptor.RenderTarget2.HasValue) {
            initRenderPassColorDescriptor(metalRenderPassDescriptor.colorAttachments[2], renderPassDescriptor.RenderTarget2.Value)
        }
        
        if (renderPassDescriptor.RenderTarget3.HasValue) {
            initRenderPassColorDescriptor(metalRenderPassDescriptor.colorAttachments[3], renderPassDescriptor.RenderTarget3.Value)
        }

        guard let renderCommandEncoder = commandList.deviceObject.makeRenderCommandEncoder(descriptor: metalRenderPassDescriptor) else {
            print("beginRenderPass: Render command encoder creation failed.")
            return
        }
        
        // TODO: Depth buffer

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
}
    
@_cdecl("Native_EndRenderPass")
public func endRenderPass(commandListPointer: UnsafeRawPointer) {
    autoreleasepool {
        let commandList = MetalCommandList.fromPointer(commandListPointer)

        guard let commandEncoder = commandList.commandEncoder else {
            print("error")
            return
        }
        
        commandEncoder.endEncoding()
        commandList.commandEncoder = nil
    }
}

private func constructGraphicsDeviceInfo(_ metalDevice: MTLDevice) -> GraphicsDeviceInfo {
    return GraphicsDeviceInfo(DeviceName: convertString(from: metalDevice.name),
                                GraphicsApi: GraphicsApi_Metal,
                                DeviceId: UInt64(metalDevice.registryID),
                                AvailableMemory: UInt64(metalDevice.recommendedMaxWorkingSetSize))
}

private func initRenderPassColorDescriptor(_ descriptor: MTLRenderPassColorAttachmentDescriptor, _ renderTargetDescriptor: RenderPassRenderTarget) {
    autoreleasepool {
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
}

private func convertString(from str: String) -> UnsafeMutablePointer<UInt8> {
    var utf8 = Array(str.utf8)
    utf8.append(0)  // adds null character
    let count = utf8.count
    let result = UnsafeMutableBufferPointer<UInt8>.allocate(capacity: count)
    _ = result.initialize(from: utf8)
    return result.baseAddress!
}
