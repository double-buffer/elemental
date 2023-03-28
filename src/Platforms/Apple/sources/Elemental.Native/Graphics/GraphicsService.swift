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
        var devices = MTLCopyAllDevices()
        devices.sort(by: sortingClosure)

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

        var devices = MTLCopyAllDevices()
        devices.sort(by: sortingClosure)
        
        for device in devices {
            if (options.DeviceId == device.registryID || options.DeviceId == 0) {
                initMetalDevice = device
                break
            }
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

        let commandQueue = MetalCommandQueue(graphicsDevice, metalCommandQueue, fence)
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

        let commandList = MetalCommandList(commandQueue.graphicsDevice, metalCommandBuffer)
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

@_cdecl("Native_FreeTexture")
public func freeTexture(texturePointer: UnsafeRawPointer) {
    autoreleasepool {
        MetalTexture.release(texturePointer)
    }
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
        metalLayer.device = commandQueue.graphicsDevice.metalDevice
        metalLayer.pixelFormat = options.Format == SwapChainFormat_HighDynamicRange ? .rgba16Float : .bgra8Unorm_srgb
        metalLayer.framebufferOnly = true
        metalLayer.allowsNextDrawableTimeout = true
        metalLayer.displaySyncEnabled = true
        metalLayer.maximumDrawableCount = 3
        metalLayer.drawableSize = CGSize(width: Int(renderSize.Width), height: Int(renderSize.Height))

        let presentSemaphore = DispatchSemaphore.init(value: Int(options.MaximumFrameLatency));

        let swapChain = MetalSwapChain(commandQueue.graphicsDevice, metalLayer, commandQueue, presentSemaphore)
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
        
        let texture = MetalTexture(swapChain.graphicsDevice, backBufferDrawable.texture)
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

@_cdecl("Native_CreateShader")
public func createShader(graphicsDevicePointer: UnsafeRawPointer, shaderParts: UnsafePointer<ShaderPart>, shaderPartCount: Int) -> UnsafeMutableRawPointer? {
    autoreleasepool {
        let graphicsDevice = MetalGraphicsDevice.fromPointer(graphicsDevicePointer)
        let shader = MetalShader(graphicsDevice)

        for i in 0..<shaderPartCount {
            let shaderPart = shaderParts[i]
            let dispatchData = DispatchData(bytes: UnsafeRawBufferPointer(start: shaderPart.DataPointer, count: Int(shaderPart.DataCount)))
            let defaultLibrary = try! graphicsDevice.metalDevice.makeLibrary(data: dispatchData as __DispatchData)
            let shaderFunction = defaultLibrary.makeFunction(name: String(cString: shaderPart.EntryPoint))

            var threadCountX = 0
            var threadCountY = 0
            var threadCountZ = 0
            var threadCount: MTLSize? = nil

            for j in 0..<Int(shaderPart.MetaDataCount) {
                let metaData = shaderPart.MetaDataPointer[j]

                switch metaData.Type {
                    case ShaderPartMetaDataType_ThreadCountX:
                        threadCountX = Int(metaData.Value)

                    case ShaderPartMetaDataType_ThreadCountY:
                        threadCountY = Int(metaData.Value)
                    
                    case ShaderPartMetaDataType_ThreadCountZ:
                        threadCountZ = Int(metaData.Value)
                    default:
                        break
                }
            }
            
            threadCount = MTLSizeMake(threadCountX, threadCountY, threadCountZ)
            
            switch shaderPart.Stage {
                case ShaderStage_AmplificationShader:
                    shader.amplificationShader = shaderFunction
                    shader.amplificationThreadCount = threadCount

                case ShaderStage_MeshShader:
                    shader.meshShader = shaderFunction
                    shader.meshThreadCount = threadCount
                    
                case ShaderStage_PixelShader:
                    shader.pixelShader = shaderFunction

                default:
                    print("createShader: Unknown shader part")
                    return nil
            }
        }
        
        return shader.toPointer()
    }
}

@_cdecl("Native_FreeShader")
public func freeShader(shaderPointer: UnsafeRawPointer) {
    autoreleasepool {
        MetalShader.release(shaderPointer)
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
        
        commandList.isRenderPassActive = true
        commandList.currentRenderPassDescriptor = renderPassDescriptor

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
            print("endRenderPass: Command encoder is nil")
            return
        }
        
        commandEncoder.endEncoding()
        commandList.commandEncoder = nil
        commandList.isRenderPassActive = false
        commandList.currentRenderPassDescriptor = nil
    }
}

@_cdecl("Native_SetShader")
public func setShader(commandListPointer: UnsafeRawPointer, shaderPointer: UnsafeRawPointer) {
    autoreleasepool {
        let commandList = MetalCommandList.fromPointer(commandListPointer)
        let graphicsDevice = commandList.graphicsDevice
        let shader = MetalShader.fromPointer(shaderPointer)

        if (commandList.isRenderPassActive) {
            // TODO: Hash the parameters
            // TODO: Async compilation with mutlithread support. (Reserve a slot in the cache, and return the pipelinestate cache object)
            // TODO: Have a separate CompileShader function that will launch the async work.
            // TODO: Have a separate GetShaderStatus method
            // TODO: Block for this method, because it means the user wants to use the shader and wants to wait on purpose
            guard let renderPassDescriptor = commandList.currentRenderPassDescriptor else {
                return
            }

            // TODO: This method is not thread-safe!
            let hash = computeRenderPipelineStateHash(shader, renderPassDescriptor)

            if (graphicsDevice.pipelineStates[hash] == nil) {
                print("Create PipelineState for shader \(hash)...")
                let pipelineState = createRenderPipelineState(shader, renderPassDescriptor)
                graphicsDevice.pipelineStates[hash] = PipelineStateCacheItem(pipelineState)
            }

            guard let renderCommandEncoder = commandList.commandEncoder as? MTLRenderCommandEncoder else {
                return
            }

            let pipelineState = graphicsDevice.pipelineStates[hash]!;
            renderCommandEncoder.setRenderPipelineState(pipelineState.pipelineState)
            commandList.currentShader = shader
            commandList.currentPipelineState = pipelineState
        }
    }
}
    
@_cdecl("Native_SetShaderConstants")
public func setShaderConstants(commandListPointer: UnsafeRawPointer, slot: UInt32, constantValues: UnsafeRawPointer, constantValueCount: Int) {
    autoreleasepool {
        let commandList = MetalCommandList.fromPointer(commandListPointer)

        if (commandList.isRenderPassActive) {
            guard let renderCommandEncoder = commandList.commandEncoder as? MTLRenderCommandEncoder else {
                return
            }
            
            guard let currentShader = commandList.currentShader else {
                return
            }

            if (currentShader.amplificationShader != nil) {
                renderCommandEncoder.setObjectBytes(constantValues, length: constantValueCount, index: Int(slot))
            }
            
            if (currentShader.meshShader != nil) {
                renderCommandEncoder.setMeshBytes(constantValues, length: constantValueCount, index: Int(slot))
            }

            if (currentShader.pixelShader != nil) {
                renderCommandEncoder.setFragmentBytes(constantValues, length: constantValueCount, index: Int(slot))
            }
        }
    }
}

@_cdecl("Native_DispatchMesh")
public func dispatchMesh(commandListPointer: UnsafeRawPointer, threadGroupCountX: UInt32, threadGroupCountY: UInt32, threadGroupCountZ: UInt32) {
    autoreleasepool {
        let commandList = MetalCommandList.fromPointer(commandListPointer)

        guard let renderCommandEncoder = commandList.commandEncoder as? MTLRenderCommandEncoder else {
            print("dispatchMesh: Command encoder is not a render command encoder")
            return
        }

        guard let pipelineState = commandList.currentPipelineState else {
            print("dispatchMesh: No pipeline state bound.")
            return
        }
        
        guard let currentShader = commandList.currentShader else {
            print("dispatchMesh: No shader bound.")
            return
        }

        assert(currentShader.amplificationShader == nil || (currentShader.amplificationThreadCount!.width > 0 
                                                         && currentShader.amplificationThreadCount!.height > 0 
                                                         && currentShader.amplificationThreadCount!.depth > 0))
                                                         
        assert(currentShader.meshShader == nil || (currentShader.meshThreadCount!.width > 0 
                                                && currentShader.meshThreadCount!.height > 0 
                                                && currentShader.meshThreadCount!.depth > 0))

        renderCommandEncoder.drawMeshThreadgroups(MTLSizeMake(Int(threadGroupCountX), Int(threadGroupCountY), Int(threadGroupCountZ)),
                threadsPerObjectThreadgroup: currentShader.amplificationThreadCount != nil ? currentShader.amplificationThreadCount! : MTLSizeMake(32, 1, 1),
                threadsPerMeshThreadgroup: currentShader.meshThreadCount != nil ? currentShader.meshThreadCount! : MTLSizeMake(32, 1, 1))
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

public func computeRenderPipelineStateHash(_ shader: MetalShader, _ renderPassDescriptor: RenderPassDescriptor) -> UInt64 {
    let shaderAddress = Int(bitPattern: shader.toPointerUnretained())
    return UInt64(shaderAddress)
}

public func createRenderPipelineState(_ shader: MetalShader, _ renderPassDescriptor: RenderPassDescriptor) -> MTLRenderPipelineState {
    let pipelineStateDescriptor = MTLMeshRenderPipelineDescriptor()

    pipelineStateDescriptor.objectFunction = shader.amplificationShader
    pipelineStateDescriptor.meshFunction = shader.meshShader
    pipelineStateDescriptor.fragmentFunction = shader.pixelShader

    // TODO: Why is the triangle not back face culled by default?

    //pipelineStateDescriptor.supportIndirectCommandBuffers = true
    //pipelineStateDescriptor.sampleCount = (metalRenderPassDescriptor.MultiSampleCount.HasValue == 1) ? Int(metalRenderPassDescriptor.MultiSampleCount.Value) : 1

    // TODO: Use the correct render target format
    if (renderPassDescriptor.RenderTarget0.HasValue) {
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm_srgb
    }

/*
    if (metalRenderPassDescriptor.RenderTarget2TextureFormat.HasValue == 1) {
        pipelineStateDescriptor.colorAttachments[1].pixelFormat = convertTextureFormat(metalRenderPassDescriptor.RenderTarget2TextureFormat.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget3TextureFormat.HasValue == 1) {
        pipelineStateDescriptor.colorAttachments[2].pixelFormat = convertTextureFormat(metalRenderPassDescriptor.RenderTarget3TextureFormat.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget4TextureFormat.HasValue == 1) {
        pipelineStateDescriptor.colorAttachments[3].pixelFormat = convertTextureFormat(metalRenderPassDescriptor.RenderTarget4TextureFormat.Value)
    }

    if (metalRenderPassDescriptor.DepthTexturePointer.HasValue == 1) {
        pipelineStateDescriptor.depthAttachmentPixelFormat = .depth32Float
    } 

    if (metalRenderPassDescriptor.RenderTarget1BlendOperation.HasValue == 1) {
        initBlendState(pipelineStateDescriptor.colorAttachments[0]!, metalRenderPassDescriptor.RenderTarget1BlendOperation.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget2BlendOperation.HasValue == 1) {
        initBlendState(pipelineStateDescriptor.colorAttachments[1]!, metalRenderPassDescriptor.RenderTarget2BlendOperation.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget3BlendOperation.HasValue == 1) {
        initBlendState(pipelineStateDescriptor.colorAttachments[2]!, metalRenderPassDescriptor.RenderTarget3BlendOperation.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget4BlendOperation.HasValue == 1) {
        initBlendState(pipelineStateDescriptor.colorAttachments[3]!, metalRenderPassDescriptor.RenderTarget4BlendOperation.Value)
    }*/

    let options = MTLPipelineOption()

        let pipelineState = try! shader.graphicsDevice.metalDevice.makeRenderPipelineState(descriptor: pipelineStateDescriptor, options: options)
        return pipelineState.0
}

private func convertString(from str: String) -> UnsafeMutablePointer<UInt8> {
    var utf8 = Array(str.utf8)
    utf8.append(0)  // adds null character
    let count = utf8.count
    let result = UnsafeMutableBufferPointer<UInt8>.allocate(capacity: count)
    _ = result.initialize(from: utf8)
    return result.baseAddress!
}

let sortingClosure: (_ graphicsDeviceInfo: MTLDevice, _ graphicsDeviceInfo: MTLDevice) -> Bool = { device1, device2 in
    if device1.location == .external && device2.location != .external {
        return true // device1 is external, device2 is not
    } else if device1.location != .external && device2.location == .external {
        return false // device2 is external, device1 is not
    } else if device1.location == .slot && device2.location != .slot {
        return true // device1 is slot, device2 is not
    } else if device1.location != .slot && device2.location == .slot {
        return false // device2 is slot, device1 is not
    } else {
        return device1.recommendedMaxWorkingSetSize > device2.recommendedMaxWorkingSetSize
    }
}
