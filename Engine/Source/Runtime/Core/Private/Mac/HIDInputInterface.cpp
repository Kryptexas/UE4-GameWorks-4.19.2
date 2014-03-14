// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "HIDInputInterface.h"

TSharedRef< HIDInputInterface > HIDInputInterface::Create( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	return MakeShareable( new HIDInputInterface( InMessageHandler ) );
}

HIDInputInterface::HIDInputInterface( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
	: MessageHandler( InMessageHandler )
{
	// Init HID Manager
	HIDManager = IOHIDManagerCreate( kCFAllocatorDefault, 0L );
	if( !HIDManager )
	{
		return;	// This will cause all subsequent calls to return information that nothing's connected
	}

	// Set HID Manager to detect devices of two distinct types: Gamepads and joysticks
	CFMutableArrayRef MatchingArray = CFArrayCreateMutable( kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks );
	if( !MatchingArray )
	{
		CFRelease( HIDManager );
		HIDManager = NULL;
		return;
	}

	CFDictionaryRef MatchingJoysticks = CreateDeviceMatchingDictionary( kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick );
	if( !MatchingJoysticks )
	{
		CFRelease( MatchingArray );
		CFRelease( HIDManager );
		HIDManager = 0;
		return;
	}

	CFArrayAppendValue( MatchingArray, MatchingJoysticks );
	CFRelease( MatchingJoysticks );

	CFDictionaryRef MatchingGamepads = CreateDeviceMatchingDictionary( kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad );
	if( !MatchingGamepads )
	{
		CFRelease( MatchingArray );
		CFRelease( HIDManager );
		HIDManager = 0;
		return;
	}

	CFArrayAppendValue( MatchingArray, MatchingGamepads );
	CFRelease( MatchingGamepads );

	CFDictionaryRef MatchingMouses = CreateDeviceMatchingDictionary( kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse );
	if( !MatchingMouses )
	{
		CFRelease( MatchingArray );
		CFRelease( HIDManager );
		HIDManager = 0;
		return;
	}

	CFArrayAppendValue( MatchingArray, MatchingMouses );
	CFRelease( MatchingMouses );

	IOHIDManagerSetDeviceMatchingMultiple( HIDManager, MatchingArray );
	CFRelease( MatchingArray );

	// Setup HID Manager's add/remove devices callbacks
	IOHIDManagerRegisterDeviceMatchingCallback( HIDManager, HIDDeviceMatchingCallback, this );
	IOHIDManagerRegisterDeviceRemovalCallback( HIDManager, HIDDeviceRemovalCallback, this );

	// Add HID Manager to run loop
	IOHIDManagerScheduleWithRunLoop( HIDManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode );

	// Open HID Manager - this will cause add device callbacks for all presently connected devices
	IOReturn Result = IOHIDManagerOpen( HIDManager, kIOHIDOptionsTypeNone );
	if( Result != kIOReturnSuccess )
	{
		IOHIDManagerUnscheduleFromRunLoop( HIDManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode );
		IOHIDManagerRegisterDeviceMatchingCallback( HIDManager, NULL, NULL );
		IOHIDManagerRegisterDeviceRemovalCallback( HIDManager, NULL, NULL );
		CFRelease( HIDManager );
		HIDManager = NULL;
		return;
	}

	for ( int32 ControllerIndex=0; ControllerIndex < MAX_NUM_HIDINPUT_CONTROLLERS; ++ControllerIndex )
	{
		FControllerState& ControllerState = ControllerStates[ControllerIndex];
		FMemory::Memzero( &ControllerState, sizeof(FControllerState) );

		ControllerState.ControllerId = ControllerIndex;
	}

	InitialButtonRepeatDelay = 0.2f;
	ButtonRepeatDelay = 0.1f;


	HIDToXboxControllerMapping[0] = -1;
	HIDToXboxControllerMapping[1] = 0;		// A
	HIDToXboxControllerMapping[2] = 1;		// B
	HIDToXboxControllerMapping[3] = 2;		// X
	HIDToXboxControllerMapping[4] = 3;		// Y
	HIDToXboxControllerMapping[5] = 4;		// L1
	HIDToXboxControllerMapping[6] = 5;		// R1
	HIDToXboxControllerMapping[7] = 10;		// L2
	HIDToXboxControllerMapping[8] = 11;		// R2
	HIDToXboxControllerMapping[9] = 7;		// Back
	HIDToXboxControllerMapping[10] = 6;		// Start
	HIDToXboxControllerMapping[11] = 8;		// Left thumbstick
	HIDToXboxControllerMapping[12] = 9;		// Right thumbstick
	HIDToXboxControllerMapping[13] = -1;
	HIDToXboxControllerMapping[14] = -1;
	HIDToXboxControllerMapping[15] = -1;

	Buttons[0] = EControllerButtons::FaceButtonBottom;
	Buttons[1] = EControllerButtons::FaceButtonRight;
	Buttons[2] = EControllerButtons::FaceButtonLeft;
	Buttons[3] = EControllerButtons::FaceButtonTop;
	Buttons[4] = EControllerButtons::LeftShoulder;
	Buttons[5] = EControllerButtons::RightShoulder;
	Buttons[6] = EControllerButtons::SpecialRight;
	Buttons[7] = EControllerButtons::SpecialLeft;
	Buttons[8] = EControllerButtons::LeftThumb;
	Buttons[9] = EControllerButtons::RightThumb;
	Buttons[10] = EControllerButtons::LeftTriggerThreshold;
	Buttons[11] = EControllerButtons::RightTriggerThreshold;
	Buttons[12] = EControllerButtons::DPadUp;
	Buttons[13] = EControllerButtons::DPadDown;
	Buttons[14] = EControllerButtons::DPadLeft;
	Buttons[15] = EControllerButtons::DPadRight;
}

void HIDInputInterface::Tick( float DeltaTime )
{
	const float DeltaScale = 2.1f;

	MouseDelta = FVector2D( 0, 0 );

	for( int32 Index = 0; Index < MouseStates.Num(); ++Index )
	{
		int32 AxisXValue = 0;
		int32 AxisYValue = 0;

		IOHIDValueRef Value;
		IOReturn Result = IOHIDDeviceGetValue( MouseStates[Index].DeviceRef, MouseStates[Index].AxisXElement, &Value );
		if( Result == kIOReturnSuccess )
		{
			AxisXValue = IOHIDValueGetIntegerValue( Value );
		}

		Result = IOHIDDeviceGetValue( MouseStates[Index].DeviceRef, MouseStates[Index].AxisYElement, &Value );
		if( Result == kIOReturnSuccess )
		{
			AxisYValue = IOHIDValueGetIntegerValue( Value );
		}

		if( FMath::Abs( AxisXValue ) > 1 )
		{
			MouseDelta.X += AxisXValue * DeltaScale;
		}

		if( FMath::Abs( AxisYValue ) > 1 )
		{
			MouseDelta.Y += AxisYValue * DeltaScale;
		}
	}
}

void HIDInputInterface::SendControllerEvents()
{
	for ( int32 ControllerIndex=0; ControllerIndex < MAX_NUM_HIDINPUT_CONTROLLERS; ++ControllerIndex )
	{
		FControllerState& ControllerState = ControllerStates[ControllerIndex];

		if( ControllerState.Device.DeviceRef )
		{
			bool CurrentStates[MAX_NUM_CONTROLLER_BUTTONS] = {0};

			for( int ButtonIndex = 0; ButtonIndex < ControllerState.Device.Buttons.Num(); ++ButtonIndex )
			{
				FHIDButtonInfo& Button = ControllerState.Device.Buttons[ButtonIndex];
				IOHIDValueRef Value;
				IOReturn Result = IOHIDDeviceGetValue( ControllerState.Device.DeviceRef, Button.Element, &Value );

				if( Result == kIOReturnSuccess )
				{
					CurrentStates[HIDToXboxControllerMapping[Button.Usage]] = IOHIDValueGetIntegerValue( Value ) > 0;
				}
			}

			if( ControllerState.Device.DPad.Element )
			{
				IOHIDValueRef Value;
				IOReturn Result = IOHIDDeviceGetValue( ControllerState.Device.DeviceRef, ControllerState.Device.DPad.Element, &Value );
					
				if( Result == kIOReturnSuccess )
				{
					int16 NewValue = IOHIDValueGetIntegerValue( Value );
						
					switch( NewValue )
					{
						case 0: CurrentStates[12] = true;								break; // Up
						case 1: CurrentStates[12] = true;	CurrentStates[13] = true;	break; // Up + right
						case 2: CurrentStates[13] = true;								break; // Right
						case 3: CurrentStates[13] = true;	CurrentStates[14] = true;	break; // Right + down
						case 4: CurrentStates[14] = true;								break; // Down
						case 5: CurrentStates[14] = true;	CurrentStates[15] = true;	break; // Down + left
						case 6: CurrentStates[15] = true;								break; // Left
						case 7: CurrentStates[15] = true;	CurrentStates[12] = true;	break; // Left + up
					}
				}
			}

			for( int AxisIndex = 0; AxisIndex < ControllerState.Device.Axes.Num(); ++AxisIndex )
			{
				FHIDAxisInfo& Axis = ControllerState.Device.Axes[AxisIndex];

				IOHIDValueRef Value;
				IOReturn Result = IOHIDDeviceGetValue( ControllerState.Device.DeviceRef, Axis.Element, &Value );

				if( Result == kIOReturnSuccess )
				{
					int16 NewValue = IOHIDValueGetIntegerValue( Value );

					int16 HalfRange = ( Axis.MaxValue + Axis.MinValue ) / 2;
					float FloatValue = FMath::Clamp( ( ( NewValue - HalfRange ) / ( float )Axis.MaxValue ) * 2.0f, -1.0f, 1.0f );

					switch( Axis.Usage )
					{
						case kHIDUsage_GD_X:
							if( ControllerState.LeftXAnalog != FloatValue )
							{
								MessageHandler->OnControllerAnalog( EControllerButtons::LeftAnalogX, ControllerState.ControllerId, FloatValue );
								ControllerState.LeftXAnalog = FloatValue;
							}
							break;

						case kHIDUsage_GD_Y:
							if( ControllerState.LeftYAnalog != FloatValue )
							{
								MessageHandler->OnControllerAnalog( EControllerButtons::LeftAnalogY, ControllerState.ControllerId, FloatValue * -1 );
								ControllerState.LeftYAnalog = FloatValue;
							}
							break;

						case kHIDUsage_GD_Z:
							if( ControllerState.RightXAnalog != FloatValue )
							{
								MessageHandler->OnControllerAnalog( EControllerButtons::RightAnalogX, ControllerState.ControllerId, FloatValue );
								ControllerState.RightXAnalog = FloatValue;
							}
							CurrentStates[10] = FloatValue > 0.01f;
							break;

						case kHIDUsage_GD_Rz:
							if( ControllerState.RightYAnalog != FloatValue )
							{
								MessageHandler->OnControllerAnalog( EControllerButtons::RightAnalogY, ControllerState.ControllerId, FloatValue * -1 );
								ControllerState.RightYAnalog = FloatValue;
							}
							CurrentStates[11] = FloatValue > 0.01f;
							break;

						case kHIDUsage_GD_Rx:
							if( ControllerState.LeftTriggerAnalog != FloatValue )
							{
								MessageHandler->OnControllerAnalog( EControllerButtons::LeftTriggerAnalog, ControllerState.ControllerId, FloatValue );
								ControllerState.LeftTriggerAnalog = FloatValue;
							}
							break;

						case kHIDUsage_GD_Ry:
							if( ControllerState.RightTriggerAnalog != FloatValue )
							{
								MessageHandler->OnControllerAnalog( EControllerButtons::RightTriggerAnalog, ControllerState.ControllerId, FloatValue );
								ControllerState.RightTriggerAnalog = FloatValue;
							}
							break;
					}
				}
			}

			const double CurrentTime = FPlatformTime::Seconds();

			// For each button check against the previous state and send the correct message if any
			for (int32 ButtonIndex = 0; ButtonIndex < MAX_NUM_CONTROLLER_BUTTONS; ++ButtonIndex)
			{
				if (CurrentStates[ButtonIndex] != ControllerState.ButtonStates[ButtonIndex])
				{
					if( CurrentStates[ButtonIndex] )
					{
						MessageHandler->OnControllerButtonPressed( Buttons[ButtonIndex], ControllerState.ControllerId, false );
					}
					else
					{
						MessageHandler->OnControllerButtonReleased( Buttons[ButtonIndex], ControllerState.ControllerId, false );
					}

					if ( CurrentStates[ButtonIndex] != 0 )
					{
						// this button was pressed - set the button's NextRepeatTime to the InitialButtonRepeatDelay
						ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + InitialButtonRepeatDelay;
					}
				}
				else if ( CurrentStates[ButtonIndex] != 0 && ControllerState.NextRepeatTime[ButtonIndex] <= CurrentTime )
				{
					MessageHandler->OnControllerButtonPressed( Buttons[ButtonIndex], ControllerState.ControllerId, true );

					// set the button's NextRepeatTime to the ButtonRepeatDelay
					ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + ButtonRepeatDelay;
				}

				// Update the state for next time
				ControllerState.ButtonStates[ButtonIndex] = CurrentStates[ButtonIndex];
			}	
		}
	}
}

CFMutableDictionaryRef HIDInputInterface::CreateDeviceMatchingDictionary( UInt32 UsagePage, UInt32 Usage )
{
	// Create a dictionary to add usage page/usages to
	CFMutableDictionaryRef Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	if( !Dict )
	{
		return NULL;
	}

	// Add key for device type to refine the matching dictionary.
	CFNumberRef PageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &UsagePage );

	if( !PageCFNumberRef )
	{
		CFRelease( Dict );
		return NULL;
	}

	CFDictionarySetValue( Dict, CFSTR( kIOHIDDeviceUsagePageKey ), PageCFNumberRef );
	CFRelease( PageCFNumberRef );

	// Note: the usage is only valid if the usage page is also defined
	CFNumberRef UsageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &Usage );

	if( !UsageCFNumberRef )
	{
		CFRelease( Dict );
		return NULL;
	}

	CFDictionarySetValue( Dict, CFSTR( kIOHIDDeviceUsageKey ), UsageCFNumberRef );
	CFRelease( UsageCFNumberRef );

    return Dict;
}

void HIDInputInterface::HIDDeviceMatchingCallback( void* Context, IOReturn Result, void* Sender, IOHIDDeviceRef DeviceRef )
{
	HIDInputInterface* HIDInput = ( HIDInputInterface* )Context;

	const bool bIsMouse = IOHIDDeviceConformsTo( DeviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse );

	if( bIsMouse )
	{
		HIDInput->OnNewHIDMouse( Result, DeviceRef );
	}
	else
	{
		HIDInput->OnNewHIDController( Result, DeviceRef );
	}
}

void HIDInputInterface::HIDDeviceRemovalCallback( void* Context, IOReturn Result, void* Sender, IOHIDDeviceRef DeviceRef )
{
	HIDInputInterface* HIDInput = ( HIDInputInterface* )Context;
	for( int32 Index = 0; Index < MAX_NUM_HIDINPUT_CONTROLLERS; ++Index )
	{
		if( HIDInput->ControllerStates[Index].Device.DeviceRef == DeviceRef )
		{
			HIDInput->ControllerStates[Index].Device.DeviceRef = NULL;
			break;
		}
	}

	int32 MouseToRemove = -1;
	for( int32 Index = 0; Index < HIDInput->MouseStates.Num(); ++Index )
	{
		if( HIDInput->MouseStates[Index].DeviceRef == DeviceRef )
		{
			MouseToRemove = Index;
			break;
		}
	}

	if( MouseToRemove != -1 )
	{
		HIDInput->MouseStates.RemoveAtSwap( MouseToRemove );
	}
}

void HIDInputInterface::OnNewHIDController( IOReturn Result, IOHIDDeviceRef DeviceRef )
{
	int32 ControllerIndex = -1;

	for( int32 Index = 0; Index < MAX_NUM_HIDINPUT_CONTROLLERS; ++Index )
	{
		if( ControllerStates[Index].Device.DeviceRef == NULL )
		{
			ControllerIndex = Index;
			break;
		}
	}

	if( ControllerIndex == -1 )
	{
		return; // There's no place for a new device. Ignoring it.
	}

	Result = IOHIDDeviceOpen( DeviceRef, kIOHIDOptionsTypeNone );

	if( Result != kIOReturnSuccess )
	{
		return;	// couldn't open the device
	}

	FHIDDeviceInfo& DeviceInfo = ControllerStates[ControllerIndex].Device;

	// Get all elements from the device
	CFArrayRef Elements = IOHIDDeviceCopyMatchingElements( DeviceRef, NULL, 0 );
	if( !Elements )
	{
		IOHIDDeviceClose( DeviceRef, kIOHIDOptionsTypeNone );
		return;
	}

	CFIndex ElementsCount = CFArrayGetCount( Elements );

	DeviceInfo.DeviceRef = DeviceRef;
	DeviceInfo.Buttons.Empty();
	DeviceInfo.Axes.Empty();
	DeviceInfo.DPad.Element = NULL;

	for( CFIndex ElementIndex = 0; ElementIndex < ElementsCount; ++ElementIndex )
	{
		IOHIDElementRef Element = (IOHIDElementRef)CFArrayGetValueAtIndex( Elements, ElementIndex );

		IOHIDElementType ElementType = IOHIDElementGetType( Element );
		uint32 UsagePage = IOHIDElementGetUsagePage( Element );
		uint32 Usage = IOHIDElementGetUsage( Element );

		if( ( ElementType == kIOHIDElementTypeInput_Button ) && ( UsagePage == kHIDPage_Button ) && ( Usage < MAX_NUM_CONTROLLER_BUTTONS ) && ( HIDToXboxControllerMapping[Usage] != -1 ) )
		{
			FHIDButtonInfo Button;
			Button.Element = Element;
			Button.Usage = Usage;
			DeviceInfo.Buttons.Push( Button );
		}
		else if( ( ( ElementType == kIOHIDElementTypeInput_Axis ) || ( ElementType == kIOHIDElementTypeInput_Misc ) ) && UsagePage == kHIDPage_GenericDesktop && Usage != kHIDUsage_GD_Hatswitch )
		{
			IOHIDValueRef Value;
			IOHIDDeviceGetValue( DeviceRef, Element, &Value );

			FHIDAxisInfo Axis;
			Axis.Element = Element;
			Axis.Usage = Usage;
			Axis.MinValue = IOHIDElementGetPhysicalMin( Element );
			Axis.MaxValue = IOHIDElementGetPhysicalMax( Element );

			DeviceInfo.Axes.Push( Axis );
		}
		else if( Usage == kHIDUsage_GD_Hatswitch && DeviceInfo.DPad.Element == NULL )
		{
			DeviceInfo.DPad.Element = Element;
			DeviceInfo.DPad.Usage = Usage;
			DeviceInfo.DPad.MinValue = IOHIDElementGetPhysicalMin( Element );
			DeviceInfo.DPad.MaxValue = IOHIDElementGetPhysicalMax( Element );
		}
	}

	CFRelease( Elements );
}

void HIDInputInterface::OnNewHIDMouse( IOReturn Result, IOHIDDeviceRef DeviceRef )
{
	if( IsMouseAlreadyKnown( DeviceRef ) )
	{
		return;
	}

	Result = IOHIDDeviceOpen( DeviceRef, kIOHIDOptionsTypeNone );
	if( Result != kIOReturnSuccess )
	{
		return;	// couldn't open the device
	}

	// Get all elements from the device
	CFArrayRef Elements = IOHIDDeviceCopyMatchingElements( DeviceRef, NULL, 0 );
	if( !Elements )
	{
		IOHIDDeviceClose( DeviceRef, kIOHIDOptionsTypeNone );
		return;
	}

	CFIndex ElementsCount = CFArrayGetCount( Elements );

	FMouseState MouseState;
	MouseState.DeviceRef = DeviceRef;

	for( CFIndex ElementIndex = 0; ElementIndex < ElementsCount; ++ElementIndex )
	{
		IOHIDElementRef Element = (IOHIDElementRef)CFArrayGetValueAtIndex( Elements, ElementIndex );

		IOHIDElementType ElementType = IOHIDElementGetType( Element );
		uint32 UsagePage = IOHIDElementGetUsagePage( Element );
		uint32 Usage = IOHIDElementGetUsage( Element );

		if( ( ElementType == kIOHIDElementTypeInput_Axis || ElementType == kIOHIDElementTypeInput_Misc ) && UsagePage == kHIDPage_GenericDesktop )
		{
			if( Usage == kHIDUsage_GD_X )
			{
				MouseState.AxisXElement = Element;
			}
			else if( Usage == kHIDUsage_GD_Y )
			{
				MouseState.AxisYElement = Element;
			}
		}
	}

	if( MouseState.AxisXElement && MouseState.AxisYElement )
	{
		MouseStates.Add( MouseState );
	}

	CFRelease( Elements );
}

bool HIDInputInterface::IsMouseAlreadyKnown( IOHIDDeviceRef DeviceRef )
{
	const CFNumberRef NewVendor = (CFNumberRef)IOHIDDeviceGetProperty(DeviceRef, CFSTR(kIOHIDVendorIDKey));
	const CFNumberRef NewProduct = (CFNumberRef)IOHIDDeviceGetProperty(DeviceRef, CFSTR(kIOHIDProductIDKey));
	const CFNumberRef NewLocation = (CFNumberRef)IOHIDDeviceGetProperty(DeviceRef, CFSTR(kIOHIDLocationIDKey));

	for( int32 Index = 0; Index < MouseStates.Num(); ++Index )
	{
		const CFNumberRef Vendor = (CFNumberRef)IOHIDDeviceGetProperty(MouseStates[Index].DeviceRef, CFSTR(kIOHIDVendorIDKey));
		const CFNumberRef Product = (CFNumberRef)IOHIDDeviceGetProperty(MouseStates[Index].DeviceRef, CFSTR(kIOHIDProductIDKey));
		const CFNumberRef Location = (CFNumberRef)IOHIDDeviceGetProperty(MouseStates[Index].DeviceRef, CFSTR(kIOHIDLocationIDKey));
		if( CFNumberCompare( NewVendor, Vendor, NULL ) == kCFCompareEqualTo
		   && CFNumberCompare( NewProduct, Product, NULL ) == kCFCompareEqualTo
		   && CFNumberCompare( NewLocation, Location, NULL ) == kCFCompareEqualTo )
		{
			return true;
		}
	}

	return false;
}

void HIDInputInterface::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	MessageHandler = InMessageHandler;
}
