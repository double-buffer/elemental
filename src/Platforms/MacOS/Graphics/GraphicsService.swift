import Cocoa
import Metal
import NativeElemental

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

    guard let metalCommandQueue = initMetalCommandQueue else {
        return nil
    }

    let commandQueue = MetalCommandQueue(graphicsDevice.metalDevice, metalCommandQueue)
    return Unmanaged.passRetained(commandQueue).toOpaque()
}

@_cdecl("Native_FreeCommandQueue")
public func freeCommandQueue(commandQueuePointer: UnsafeRawPointer) {
    //MetalGraphicsDevice.release(graphicsDevicePointer)
}

@_cdecl("Native_SetCommandQueueLabel")
public func setCommandQueueLabel(commandQueuePointer: UnsafeRawPointer, label: UnsafeMutablePointer<Int8>) {
    //MetalGraphicsDevice.release(graphicsDevicePointer)
}

@_cdecl("Native_CreateCommandList")
public func createCommandList(commandQueuePointer: UnsafeRawPointer) -> UnsafeMutableRawPointer? {
    print("Create CommandList")
    return nil
}

@_cdecl("Native_FreeCommandList")
public func freeCommandList(commandListPointer: UnsafeRawPointer) {
    //MetalGraphicsDevice.release(graphicsDevicePointer)
}

@_cdecl("Native_SetCommandListLabel")
public func setCommandListLabel(commandListPointer: UnsafeRawPointer, label: UnsafeMutablePointer<Int8>) {
    //MetalGraphicsDevice.release(graphicsDevicePointer)
}

@_cdecl("Native_CommitCommandList")
public func commitommandList(commandListPointer: UnsafeRawPointer) {
    //MetalGraphicsDevice.release(graphicsDevicePointer)
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