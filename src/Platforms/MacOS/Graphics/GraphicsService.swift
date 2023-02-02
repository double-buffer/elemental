import Cocoa
import Metal
import NativeElemental

@_cdecl("Native_GraphicsServiceInit")
public func graphicsServiceInit() {
   
}

@_cdecl("Native_GraphicsServiceDispose")
public func graphicsServiceDispose() {
   
}

@_cdecl("Native_CreateGraphicsDevice")
public func createGraphicsDevice(options: GraphicsDeviceOptions) -> UnsafeMutableRawPointer? {
    if (options.GraphicsDiagnostics == GraphicsDiagnostics_Debug) {
        print("DEBUG")
    }

    let devices = MTLCopyAllDevices()

    for device in devices {
        // TODO: Test for metal3 mandatory
        print(device)
        if (device.supportsFamily(.metal3)) {
            print("==> metal 3")
        }

        if (device.supportsFamily(.mac2)) {
            print("==> mac 2")
        }
        
        if (device.supportsFamily(.apple7)) {
            print("==> apple 7")
        }
    }

    // TODO: Enumerate GPUs
    let metalDevice = MTLCreateSystemDefaultDevice()!
    let graphicsDevice = MetalGraphicsDevice(metalDevice)
    return Unmanaged.passRetained(graphicsDevice).toOpaque()
}

@_cdecl("Native_DeleteGraphicsDevice")
public func deleteGraphicsDevice(graphicsDevicePointer: UnsafeRawPointer) {
    MetalGraphicsDevice.release(graphicsDevicePointer)
}

@_cdecl("Native_GetGraphicsDeviceInfo")
public func getGraphicsDeviceInfo(graphicsDevicePointer: UnsafeRawPointer) -> GraphicsDeviceInfo {
    let graphicsDevice = MetalGraphicsDevice.fromPointer(graphicsDevicePointer)

    return GraphicsDeviceInfo(DeviceName: convertString(from: graphicsDevice.metalDevice.name),
                              GraphicsApiName: convertString(from: graphicsDevice.metalDevice.supportsFamily(.metal3) ? "Metal 3" : "Metal 2"),
                              DriverVersion: convertString(from: "TEst3"))
}

func convertString(from str: String) -> UnsafeMutablePointer<UInt8> {
    var utf8 = Array(str.utf8)
    utf8.append(0)  // adds null character
    let count = utf8.count
    let result = UnsafeMutableBufferPointer<UInt8>.allocate(capacity: count)
    _ = result.initialize(from: utf8)
    return result.baseAddress!
}