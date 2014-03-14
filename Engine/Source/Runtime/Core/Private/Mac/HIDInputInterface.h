// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include <IOKit/hid/IOHIDLib.h>

#define MAX_NUM_HIDINPUT_CONTROLLERS 4

/** Max number of controller buttons.  Must be < 256*/
#define MAX_NUM_CONTROLLER_BUTTONS 16


/**
 * Interface class for HID Input devices
 */
class HIDInputInterface
{
public:

	static TSharedRef< HIDInputInterface > Create( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	void Tick( float DeltaTime );

	void SendControllerEvents();

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	const FVector2D& GetMouseDelta() const { return MouseDelta; }

private:

	HIDInputInterface( const TSharedRef< FGenericApplicationMessageHandler >& MessageHandler );

	static CFMutableDictionaryRef CreateDeviceMatchingDictionary( UInt32 UsagePage, UInt32 Usage );

	static void HIDDeviceMatchingCallback( void* Context, IOReturn Result, void* Sender, IOHIDDeviceRef DeviceRef );

	static void HIDDeviceRemovalCallback( void* Context, IOReturn Result, void* Sender, IOHIDDeviceRef DeviceRef );

	void OnNewHIDController( IOReturn Result, IOHIDDeviceRef DeviceRef );

	void OnNewHIDMouse( IOReturn Result, IOHIDDeviceRef DeviceRef );

	bool IsMouseAlreadyKnown( IOHIDDeviceRef DeviceRef );

private:

	struct FHIDButtonInfo
	{
		IOHIDElementRef Element;
		uint32 Usage;
	};

	struct FHIDAxisInfo
	{
		IOHIDElementRef Element;
		uint32 Usage;
		int16 MinValue;
		int16 MaxValue;
	};

	struct FHIDDeviceInfo
	{
		IOHIDDeviceRef DeviceRef;
		TArray<FHIDButtonInfo> Buttons;
		TArray<FHIDAxisInfo> Axes;
		FHIDAxisInfo DPad;
	};

	struct FControllerState
	{
		/** Last frame's button states, so we only send events on edges */
		bool ButtonStates[MAX_NUM_CONTROLLER_BUTTONS];

		/** Next time a repeat event should be generated for each button */
		double NextRepeatTime[MAX_NUM_CONTROLLER_BUTTONS];

		/** Raw Left thumb x analog value */
		int16 LeftXAnalog;
		
		/** Raw left thumb y analog value */
		int16 LeftYAnalog;

		/** Raw Right thumb x analog value */
		int16 RightXAnalog;

		/** Raw Right thumb x analog value */
		int16 RightYAnalog;

		/** Left Trigger analog value */
		int16 LeftTriggerAnalog;

		/** Right trigger analog value */
		int16 RightTriggerAnalog;

		/** Id of the controller */
		int32 ControllerId;

		FHIDDeviceInfo Device;
	};

	struct FMouseState
	{
		IOHIDDeviceRef DeviceRef;
		IOHIDElementRef AxisXElement;
		IOHIDElementRef AxisYElement;

		FMouseState() : DeviceRef(NULL), AxisXElement(NULL), AxisYElement(NULL) {}
	};

private:

	/** In the engine, all controllers map to xbox controllers for consistency */
	uint8 HIDToXboxControllerMapping[MAX_NUM_CONTROLLER_BUTTONS];

	/** Names of all the buttons */
	EControllerButtons::Type Buttons[MAX_NUM_CONTROLLER_BUTTONS];

	/** Controller states */
	FControllerState ControllerStates[MAX_NUM_HIDINPUT_CONTROLLERS];

	/** Mouse states */
	TArray<FMouseState> MouseStates;

	FVector2D MouseDelta;

	/** Delay before sending a repeat message after a button was first pressed */
	float InitialButtonRepeatDelay;

	/** Delay before sendign a repeat message after a button has been pressed for a while */
	float ButtonRepeatDelay;

	IOHIDManagerRef HIDManager;

	TSharedRef< FGenericApplicationMessageHandler > MessageHandler;
};
