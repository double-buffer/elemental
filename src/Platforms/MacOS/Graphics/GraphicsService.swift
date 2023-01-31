import Cocoa
import NativeElemental

@_cdecl("Native_CreateGraphicsDevice")
public func createGraphicsDevice(diagnostics: GraphicsDiagnostics) -> UnsafeMutableRawPointer? {
    let graphicsDevice = MetalGraphicsDevice()
    return Unmanaged.passRetained(graphicsDevice).toOpaque()
}

@_cdecl("Native_DeleteGraphicsDevice")
public func deleteGraphicsDevice(graphicsDevicePointer: UnsafeRawPointer) {
    MetalGraphicsDevice.release(graphicsDevicePointer)
}

@_cdecl("Native_GetGraphicsDeviceInfo")
public func getGraphicsDeviceInfo(graphicsDevicePointer: UnsafeRawPointer) -> GraphicsDeviceInfo {
    let graphicsDevice = MetalGraphicsDevice.fromPointer(graphicsDevicePointer)

    return GraphicsDeviceInfo(DeviceName: convertString(from: "Test1"),
                              GraphicsApiName: convertString(from: "Test2"),
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