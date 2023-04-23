import Foundation
import IOKit
import IOKit.hid
import SystemFunctions
import NativeElemental
import HidDevices

var hidInputDevices: [HidInputDevice] = []
var inputState: UnsafeMutablePointer<InputState>? = nil

// TODO: This works only on MacOS
let checkInputDevicesThread = Thread {
    let hidManager = IOHIDManagerCreate(kCFAllocatorDefault, IOOptionBits(kIOHIDOptionsTypeNone))
    let deviceDictionary = [kIOHIDPrimaryUsagePageKey: kHIDPage_GenericDesktop, kIOHIDPrimaryUsageKey: kHIDUsage_GD_GamePad] as CFDictionary

    IOHIDManagerSetDeviceMatching(hidManager, deviceDictionary)
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, handle_DeviceMatchingCallback, nil)
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, handle_DeviceRemovalCallback, nil)
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), CFRunLoopMode.defaultMode.rawValue)

    IOHIDManagerOpen(hidManager, IOOptionBits(kIOHIDOptionsTypeNone))
    CFRunLoopRun()
}

func handle_DeviceMatchingCallback(context: UnsafeMutableRawPointer?, result: IOReturn, sender: UnsafeMutableRawPointer?, device: IOHIDDevice) {
    let usagePage = IOHIDDeviceGetProperty(device, kIOHIDPrimaryUsagePageKey as CFString) as! UInt32
    let usage = IOHIDDeviceGetProperty(device, kIOHIDPrimaryUsageKey as CFString) as! UInt32
    
    if usagePage == kHIDPage_GenericDesktop && usage == kHIDUsage_GD_GamePad {
        let name = IOHIDDeviceGetProperty(device, kIOHIDProductKey as CFString) as! String
        let productID = IOHIDDeviceGetProperty(device, kIOHIDProductIDKey as CFString) as! UInt32
        let vendorID = IOHIDDeviceGetProperty(device, kIOHIDVendorIDKey as CFString) as! UInt32
        
        print("Gamepad connected: \(name) (ProductID: \(productID), VendorID: \(vendorID))")

        let convertHidInputDeviceDataFunctionPointer = GetConvertHidInputDeviceDataFuncPtr(vendorID, productID)

        guard let convertHidInputDeviceDataFunction = convertHidInputDeviceDataFunctionPointer else {
            print("Cannot find an HID Input Device data converter function for: \(name) (ProductID: \(productID), VendorID: \(vendorID))")
            return
        }

        guard let reportSizeNumber = IOHIDDeviceGetProperty(device, kIOHIDMaxInputReportSizeKey as CFString) as CFTypeRef? else {
            return
        }

        let reportSize = reportSizeNumber as! CFIndex

        let hidInputDevice = HidInputDevice(hidInputDevices.count, device, inputRawReportSize: reportSize, inputDataConvertFunction: convertHidInputDeviceDataFunction)
        hidInputDevices.append(hidInputDevice)

        let context = UnsafeMutableRawPointer(Unmanaged.passUnretained(hidInputDevice).toOpaque())
        IOHIDDeviceRegisterInputReportCallback(device, hidInputDevice.inputRawReport, hidInputDevice.inputRawReportSize, handle_InputReportCallback, context)
    }
}

func handle_DeviceRemovalCallback(context: UnsafeMutableRawPointer?, result: IOReturn, sender: UnsafeMutableRawPointer?, device: IOHIDDevice) {
    // Handle the device removal event here
    // TODO: Call a reset function from the C Lib
    print("Device removed")
}

func handle_InputReportCallback(context: UnsafeMutableRawPointer?, result: IOReturn, sender: UnsafeMutableRawPointer?, reportType: IOHIDReportType, reportID: UInt32, report: UnsafeMutablePointer<UInt8>, reportLength: CFIndex) {
    guard reportType == kIOHIDReportTypeInput else {
        return
    }

    guard let context = context else {
        return
    }
    
    guard let localInputState = inputState else {
        print("Input state was not initialized correctly.")
        return
    }

    let hidInputDevice = Unmanaged<HidInputDevice>.fromOpaque(context).takeUnretainedValue()
    hidInputDevice.inputDataConvertFunction(inputState, Int32(hidInputDevice.deviceId), report, UInt32(reportLength))
    
    // DEBUG
    //print(report.withMemoryRebound(to: XboxOneWirelessGamepadReport.self, capacity: 1) { $0.pointee })
}

@_cdecl("Native_InitInputsService")
public func initInputsService() {
    inputState = InitInputState()
    
    checkInputDevicesThread.start()
}

@_cdecl("Native_FreeInputsService")
public func freeInputsService() {
    // TODO: clean thread?
    guard let localInputState = inputState else {
        print("Input state was not initialized correctly.")
        return
    }

    FreeInputState(localInputState)
}

@_cdecl("Native_GetInputState")
public func getInputState(application: UnsafeMutableRawPointer?) -> InputState {
    guard let localInputState = inputState else {
        print("Input state was not initialized correctly.")
        return InputState(DataPointer: nil, DataSize: 0, InputObjectsPointer: nil, InputObjectsSize: 0)
    }    
    
    return localInputState.pointee
}