import Foundation
import IOKit
import IOKit.hid
import HidDevices

public class HidInputDevice {
    public init(_ deviceId: Int, _ device: IOHIDDevice, inputRawReportSize: CFIndex, inputDataConvertFunction: ConvertHidInputDeviceDataFuncPtr) {
        self.deviceId = deviceId
        self.device = device
        self.inputRawReportSize = inputRawReportSize
        self.inputRawReport = UnsafeMutablePointer<UInt8>.allocate(capacity: inputRawReportSize)
        self.inputDataConvertFunction = inputDataConvertFunction
    }

    public let deviceId: Int 
    public let device: IOHIDDevice
    public let inputRawReport: UnsafeMutablePointer<UInt8>
    public let inputRawReportSize: CFIndex
    public let inputDataConvertFunction: ConvertHidInputDeviceDataFuncPtr
}

