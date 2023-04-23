import Foundation
import IOKit
import IOKit.hid
import HidDevices

public class HidInputDevice {
    public init(_ deviceId: Int, _ device: IOHIDDevice, inputRawReportSize: CFIndex, inputDataConvertFunction: ConvertHidInputDeviceDataFuncPtr) {
        self.deviceId = deviceId
        self.device = device
        self.inputRawReportSize = inputRawReportSize

        // TODO: Free memory
        self.inputRawReport = UnsafeMutablePointer<UInt8>.allocate(capacity: inputRawReportSize)
        inputRawReport.initialize(repeating: 0, count: inputRawReportSize)
        
        self.inputDataConvertFunction = inputDataConvertFunction
    }

    public let deviceId: Int 
    public let device: IOHIDDevice
    public let inputRawReport: UnsafeMutablePointer<UInt8>
    public let inputRawReportSize: CFIndex
    public let inputDataConvertFunction: ConvertHidInputDeviceDataFuncPtr
}

