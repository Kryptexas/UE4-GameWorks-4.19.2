// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#define MAX_NUM_XINPUT_CONTROLLERS 4

/** Max number of controller buttons.  Must be < 256*/
#define MAX_NUM_CONTROLLER_BUTTONS 24

/**
 * Interface class for XInput devices (xbox 360 controller)                 
 */
class XInputInterface
{
public:

	static TSharedRef< XInputInterface > Create( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	/**
	 * Poll for controller state and send events if needed
	 *
	 * @param PathToJoystickCaptureWidget	The path to the joystick capture widget.  If invalid this function does not poll 
	 */
	void SendControllerEvents();

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	void SetNeedsControllerStateUpdate() { bNeedsControllerStateUpdate = true; }

private:

	XInputInterface( const TSharedRef< FGenericApplicationMessageHandler >& MessageHandler );


	struct FControllerState
	{
		/** Last frame's button states, so we only send events on edges */
		bool ButtonStates[MAX_NUM_CONTROLLER_BUTTONS];

		/** Next time a repeat event should be generated for each button */
		double NextRepeatTime[MAX_NUM_CONTROLLER_BUTTONS];

		/** Raw Left thumb x analog value */
		SHORT LeftXAnalog;

		/** Raw left thumb y analog value */
		SHORT LeftYAnalog;

		/** Raw Right thumb x analog value */
		SHORT RightXAnalog;

		/** Raw Right thumb x analog value */
		SHORT RightYAnalog;

		/** Left Trigger analog value */
		uint8 LeftTriggerAnalog;

		/** Right trigger analog value */
		uint8 RightTriggerAnalog;

		/** Id of the controller */
		int32 ControllerId;

		/** If the controller is currently connected */
		bool bIsConnected;
		
	};

	/** If we've been notified by the system that the controller state may have changed */
	bool bNeedsControllerStateUpdate;

	/** In the engine, all controllers map to xbox controllers for consistency */
	uint8	X360ToXboxControllerMapping[MAX_NUM_CONTROLLER_BUTTONS];

	/** Controller states */
	FControllerState ControllerStates[MAX_NUM_XINPUT_CONTROLLERS];

	/** Delay before sending a repeat message after a button was first pressed */
	float InitialButtonRepeatDelay;

	/** Delay before sending a repeat message after a button has been pressed for a while */
	float ButtonRepeatDelay;

	/**  */
	EControllerButtons::Type Buttons[MAX_NUM_CONTROLLER_BUTTONS];

	/**  */
	TSharedRef< FGenericApplicationMessageHandler > MessageHandler;
};