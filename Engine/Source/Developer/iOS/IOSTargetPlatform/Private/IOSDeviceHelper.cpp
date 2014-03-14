// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved

#include "IOSTargetPlatformPrivatePCH.h"

#if PLATFORM_MAC
#include <CoreFoundation/CoreFoundation.h>

struct AMDeviceNotificationCallbackInformation
{
	void 		*deviceHandle;
	uint32_t	msgType;
};

#ifdef __cplusplus
extern "C" {
#endif

int AMDeviceConnect (void *device);
int AMDeviceValidatePairing (void *device);
int AMDeviceStartSession (void *device);
int AMDeviceStopSession (void *device);
int AMDeviceDisconnect (void *device);
int AMDeviceNotificationSubscribe(void*, int, int, int, void**);
CFStringRef AMDeviceCopyValue(void*, int unknown, CFStringRef cfstring);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_MAC


/* FIOSDeviceHelper structors
 *****************************************************************************/

static TMap<void*, FIOSLaunchDaemonPong> ConnectedDevices;

void FIOSDeviceHelper::Initialize()
{
#if PLATFORM_MAC
    static bool bIsInitialized = false;
    
    if (!bIsInitialized)
    {
        void *subscribe;
        int32 rc = AMDeviceNotificationSubscribe((void*)FIOSDeviceHelper::DeviceCallback, 0, 0, 0, &subscribe);
        if (rc < 0)
        {
            //@todo right out to the log that we can't subscribe
        }
        bIsInitialized = true;
    }
#else

	// Create a dummy device to hand over
	const FString DummyDeviceName(FString::Printf(TEXT("All_iOS_On_%s"), FPlatformProcess::ComputerName()));
	
	FIOSLaunchDaemonPong Event;
	Event.DeviceID = FString::Printf(TEXT("IOS@%s"), *DummyDeviceName);
	Event.bCanReboot = false;
	Event.bCanPowerOn = false;
	Event.bCanPowerOff = false;
	Event.DeviceName = DummyDeviceName;
	FIOSDeviceHelper::OnDeviceConnected().Broadcast(Event);
#endif
}

void FIOSDeviceHelper::DeviceCallback(void* CallbackInfo)
{
#if PLATFORM_MAC
    struct AMDeviceNotificationCallbackInformation* cbi = (AMDeviceNotificationCallbackInformation*)CallbackInfo;
    
    void* deviceHandle = cbi->deviceHandle;
    
    switch(cbi->msgType)
    {
        case 1:
            FIOSDeviceHelper::DoDeviceConnect(deviceHandle);
            break;
            
        case 2:
            FIOSDeviceHelper::DoDeviceDisconnect(deviceHandle);
            break;
            
        case 3:
            break;
    }
#endif
}

void FIOSDeviceHelper::DoDeviceConnect(void* deviceHandle)
{
#if PLATFORM_MAC
    // connect to the device
    int32 rc = AMDeviceConnect(deviceHandle);
    if (!rc)
    {
        // validate the pairing
        rc = AMDeviceValidatePairing(deviceHandle);
        if (!rc)
        {
            // start a session
            rc = AMDeviceStartSession(deviceHandle);
            if (!rc)
            {
                // get the needed data
                CFStringRef deviceName = AMDeviceCopyValue(deviceHandle, 0, CFSTR("DeviceName"));
                CFStringRef deviceId = AMDeviceCopyValue(deviceHandle, 0, CFSTR("UniqueDeviceID"));

                // fire the event
                TCHAR idBuffer[128];
				TCHAR nameBuffer[256];
                FPlatformString::CFStringToTCHAR(deviceId, idBuffer);
				FPlatformString::CFStringToTCHAR(deviceName, nameBuffer);
                FIOSLaunchDaemonPong Event;
                Event.DeviceID = FString::Printf(TEXT("IOS@%s"), idBuffer);
				Event.DeviceName = FString::Printf(TEXT("%s"), nameBuffer);
                Event.bCanReboot = false;
                Event.bCanPowerOn = false;
                Event.bCanPowerOff = false;
                Event.DeviceName = nameBuffer;
                FIOSDeviceHelper::OnDeviceConnected().Broadcast(Event);
                
                // add to the device list
                ConnectedDevices.Add(deviceHandle, Event);
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("Couldn't start session with device"));
            }
            // close the session
            rc = AMDeviceStopSession(deviceHandle);
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("Couldn't validate pairing to device"));
        }
       // disconnect from the device
        rc = AMDeviceDisconnect(deviceHandle);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Couldn't connect to device"));
    }
#endif
}

void FIOSDeviceHelper::DoDeviceDisconnect(void* deviceHandle)
{
#if PLATFORM_MAC
    if (ConnectedDevices.Contains(deviceHandle))
    {
        // extract the device id from the connected list¯
		FIOSLaunchDaemonPong Event = ConnectedDevices.FindAndRemoveChecked(deviceHandle);
    
        // fire the event
        FIOSDeviceHelper::OnDeviceDisconnected().Broadcast(Event);
    }
#endif
}

