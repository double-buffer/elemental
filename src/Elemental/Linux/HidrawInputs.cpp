#include "HidrawInputs.h"
#include "WaylandApplication.h"
#include "Inputs/Inputs.h"
#include "Inputs/HidDevices.h"
#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

#define HID_REPORT_SIZE 255

struct LinuxHidDevice
{
    ReadOnlySpan<char> Path;
    int32_t FileDescriptor;
    uint32_t VendorId;
    uint32_t ProductId;

};

SystemDictionary<void*, ElemInputDevice> hidrawInputDeviceDictionary;
Span<LinuxHidDevice> linuxHidDevices;
uint32_t currentLinuxHidDeviceIndex = 0;

ElemInputDevice AddHidrawInputDevice(void* device, ElemInputDeviceType deviceType, uint32_t vendorId, uint32_t productId)
{
    if (SystemDictionaryContainsKey(hidrawInputDeviceDictionary, device))
    {
        return *SystemGetDictionaryValue(hidrawInputDeviceDictionary, device);
    }

    auto stackMemoryArena = SystemGetStackMemoryArena();

    InputDeviceData deviceData =
    {
        .PlatformHandle = device,
        .InputDeviceType = deviceType,
        .HidVendorId = vendorId,
        .HidProductId = productId
    };

    InputDeviceDataFull deviceDataFull = {};

    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Create HID Input device.");
    auto handle = AddInputDevice(&deviceData, &deviceDataFull);
    SystemAddDictionaryEntry(hidrawInputDeviceDictionary, device, handle);

    return handle;
}

void LinuxHidDiscoverDevices()
{
    auto udev = udev_new();
    SystemAssert(udev);

    auto hidEnumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(hidEnumerate, "hidraw");
    udev_enumerate_scan_devices(hidEnumerate);

    auto hidDevices = udev_enumerate_get_list_entry(hidEnumerate);
    udev_list_entry* entry;

    udev_list_entry_foreach(entry, hidDevices) 
    {
        auto path = udev_list_entry_get_name(entry);
        auto hidDevice = udev_device_new_from_syspath(udev, path);
        const char *devnode = udev_device_get_devnode(hidDevice);

        if (hidDevice) 
        {
            auto parent = udev_device_get_parent_with_subsystem_devtype(hidDevice, "hid", nullptr);

            if (parent)
            {
                char vendor[5] = {0};  // 4 hex digits + null terminator
                char product[5] = {0}; // 4 hex digits + null terminator

                if (sscanf(path, "/sys/devices/virtual/misc/uhid/%*4c:%4s:%4s", vendor, product) == 2) 
                {
                    char *end;
                    long vendor_id = strtol(vendor, &end, 16);
                    long product_id = strtol(product, &end, 16);

                    if (IsHidDeviceSupported(vendor_id, product_id))
                    {
                        linuxHidDevices[currentLinuxHidDeviceIndex++] =
                        {
                            .Path = SystemDuplicateBuffer<char>(ApplicationMemoryArena, devnode),
                            .FileDescriptor = -1,
                            .VendorId = (uint32_t)vendor_id,
                            .ProductId = (uint32_t)product_id
                        };
                    }
                }
            }
            udev_device_unref(hidDevice);
        }
    }

    udev_enumerate_unref(hidEnumerate);
    udev_unref(udev);
}

void RunHidrawLoop(void* payload)
{
    // TODO: Support gamepad connect/disconnect

    LinuxHidDiscoverDevices(); 

    pollfd pollingFileDescriptors[MAX_INPUT_DEVICES];

    for (uint32_t i = 0; i < currentLinuxHidDeviceIndex; i++)
    {
        auto hidDevice = &linuxHidDevices[i];

        auto fileDescriptor = open(hidDevice->Path.Pointer, O_RDONLY | O_NONBLOCK);
        
        if (fileDescriptor < 0) 
        {
            continue;
        }

        hidDevice->FileDescriptor = fileDescriptor;

        pollingFileDescriptors[i].fd = fileDescriptor;
        pollingFileDescriptors[i].events = POLLIN;
    }
    
    uint8_t buf[HID_REPORT_SIZE];

    while (true)
    {
        auto result = poll(pollingFileDescriptors, currentLinuxHidDeviceIndex, -1); 

        if (result < 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Inputs, "Error while polling HID devices.");
            break;
        }

        for (uint32_t i = 0; i < currentLinuxHidDeviceIndex; i++)
        {
            if (pollingFileDescriptors[i].revents & POLLIN)
            {
                auto hidDevice = &linuxHidDevices[i];

                if (hidDevice->FileDescriptor >= 0)
                {
                    result = read(hidDevice->FileDescriptor, buf, sizeof(buf));

                    if (result > 0)
                    {
                        auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - WaylandPerformanceCounterStart) / WaylandPerformanceCounterFrequencyInSeconds;
                        auto inputDevice = AddHidrawInputDevice((void*)hidDevice->Path.Pointer, ElemInputDeviceType_Gamepad, hidDevice->VendorId, hidDevice->ProductId);
                        ProcessHidDeviceData((ElemWindow)payload, inputDevice, ReadOnlySpan<uint8_t>((uint8_t*)buf, HID_REPORT_SIZE), elapsedSeconds);
                    }
                }
            }
        }
    }

    for (uint32_t i = 0; i < currentLinuxHidDeviceIndex; i++)
    {
        auto hidDevice = linuxHidDevices[i];

        if (hidDevice.FileDescriptor >= 0)
        {
            close(hidDevice.FileDescriptor);
        }
    }
}

void InitHidrawInputs(ElemWindow window)
{
    SystemAssert(window != ELEM_HANDLE_NULL);
    hidrawInputDeviceDictionary = SystemCreateDictionary<void*, ElemInputDevice>(ApplicationMemoryArena, MAX_INPUT_DEVICES);
    linuxHidDevices = SystemPushArrayZero<LinuxHidDevice>(ApplicationMemoryArena, MAX_INPUT_DEVICES);

    SystemCreateThread(RunHidrawLoop, (void*)window);
}
