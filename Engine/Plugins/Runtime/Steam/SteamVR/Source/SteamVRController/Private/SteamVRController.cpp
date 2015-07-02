// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SteamVRControllerPrivatePCH.h"


DEFINE_LOG_CATEGORY_STATIC(LogSteamVRController, Log, All);

/** Controller axis mappings. @todo steamvr: should enumerate rather than hard code */
#define TOUCHPAD_AXIS	0
#define TRIGGER_AXIS	1

//
// Gamepad thresholds
//
#define TOUCHPAD_DEADZONE  0.0f

namespace SteamVRControllerKeyNames
{
	const FGamepadKeyNames::Type Touch0("Steam_Touch_0");
	const FGamepadKeyNames::Type Touch1("Steam_Touch_1");
	const FGamepadKeyNames::Type BackLeft("Steam_Back_Left");
	const FGamepadKeyNames::Type BackRight("Steam_Back_Right");
}

class FSteamVRController : public IInputDevice
{

public:

	/** The maximum number of Unreal controllers.  Each Unreal controller represents a pair of motion controller devices */
	static const int32 MaxUnrealControllers = MAX_STEAMVR_CONTROLLER_PAIRS;

	/** Total number of motion controllers we'll support */
	static const int32 MaxControllers = MaxUnrealControllers * 2;

	/** Left and right hands for Steam VR motion controller devices */
	enum class EControllerHand
	{
		Left,
		Right,

		// ...
		TotalHandCount
	};


	/**
	 * Buttons on the SteamVR controller
	 */
	struct ESteamVRControllerButton
	{
		enum Type
		{
			System,
			ApplicationMenu,
			TouchPadPress,
			TouchPadTouch,
			TriggerPress,
			Grip,
			TouchPadUp,
			TouchPadDown,
			TouchPadLeft,
			TouchPadRight,

			/** Max number of controller buttons.  Must be < 256 */
			TotalButtonCount
		};
	};


	FSteamVRController(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
		: MessageHandler(InMessageHandler),
		  SteamVRPlugin(nullptr)
	{
		FMemory::Memzero(ControllerStates, sizeof(ControllerStates));
		for( int32 ControllerPairIndex = 0; ControllerPairIndex < MaxUnrealControllers; ++ControllerPairIndex )
		{
			for( int32 HandIndex = 0; HandIndex < (int32)EControllerHand::TotalHandCount; ++HandIndex )
			{
				ControllerStates[ UnrealControllerIdToControllerIndex( ControllerPairIndex, (EControllerHand)HandIndex ) ].Hand = (EControllerHand)HandIndex;
			}
		}

		for (int32 i=0; i < vr::k_unMaxTrackedDeviceCount; ++i)
		{
			DeviceToControllerMap[i] = INDEX_NONE;
		}

		for (int32 i=0; i < MaxControllers; ++i)
		{
			ControllerToDeviceMap[i] = INDEX_NONE;
		}

		NumControllersMapped = 0;

		InitialButtonRepeatDelay = 0.2f;
		ButtonRepeatDelay = 0.1f;

		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::System ] = FGamepadKeyNames::SpecialLeft;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::ApplicationMenu ] = FGamepadKeyNames::LeftShoulder;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadPress ] = FGamepadKeyNames::LeftThumb;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadTouch ] = SteamVRControllerKeyNames::Touch0;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TriggerPress ] = FGamepadKeyNames::LeftTriggerThreshold;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::Grip ] = SteamVRControllerKeyNames::BackLeft;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadUp ] = FGamepadKeyNames::LeftStickUp;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadDown ] = FGamepadKeyNames::LeftStickDown;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadLeft ] = FGamepadKeyNames::LeftStickLeft;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadRight ] = FGamepadKeyNames::LeftStickRight;

		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::System ] = FGamepadKeyNames::SpecialRight;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::ApplicationMenu ] = FGamepadKeyNames::RightShoulder;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadPress ] = FGamepadKeyNames::RightThumb;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadTouch ] = SteamVRControllerKeyNames::Touch1;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TriggerPress ] = FGamepadKeyNames::RightTriggerThreshold;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::Grip ] = SteamVRControllerKeyNames::BackRight;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadUp ] = FGamepadKeyNames::RightStickUp;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadDown ] = FGamepadKeyNames::RightStickDown;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadLeft ] = FGamepadKeyNames::RightStickLeft;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadRight ] = FGamepadKeyNames::RightStickRight;
	}

	virtual ~FSteamVRController()
	{
	}

	virtual void Tick( float DeltaTime ) override
	{
	}

	virtual void SendControllerEvents() override
	{
		vr::VRControllerState_t VRControllerState;

		vr::IVRSystem* VRSystem = GetVRSystem();

		if (VRSystem != nullptr)
		{
			const double CurrentTime = FPlatformTime::Seconds();

			for (uint32 DeviceIndex=0; DeviceIndex < vr::k_unMaxTrackedDeviceCount; ++DeviceIndex)
			{
				// skip non-controllers
				if (VRSystem->GetTrackedDeviceClass(DeviceIndex) != vr::TrackedDeviceClass_Controller)
				{
					continue;
				}

				// update the mappings if this is a new device
				if (DeviceToControllerMap[DeviceIndex] == INDEX_NONE )
				{
					// don't map too many controllers
					if (NumControllersMapped >= MaxControllers)
					{
						continue;
					}

					DeviceToControllerMap[DeviceIndex] = NumControllersMapped;
					ControllerToDeviceMap[NumControllersMapped] = DeviceIndex;
					++NumControllersMapped;

					// update the SteamVR plugin with the new mapping
					{
						int32 UnrealControllerIdAndHandToDeviceIdMap[ MaxUnrealControllers ][ (int32)EControllerHand::TotalHandCount ];
						for( int32 UnrealControllerIndex = 0; UnrealControllerIndex < MaxUnrealControllers; ++UnrealControllerIndex )
						{
							for( int32 HandIndex = 0; HandIndex < (int32)EControllerHand::TotalHandCount; ++HandIndex )
							{
								UnrealControllerIdAndHandToDeviceIdMap[ UnrealControllerIndex ][ HandIndex ] = ControllerToDeviceMap[ UnrealControllerIdToControllerIndex( UnrealControllerIndex, (EControllerHand)HandIndex ) ];
							}
						}
						SteamVRPlugin->SetUnrealControllerIdAndHandToDeviceIdMap( UnrealControllerIdAndHandToDeviceIdMap );
					}
				}

				// get the controller index for this device
				int32 ControllerIndex = DeviceToControllerMap[DeviceIndex];
				FControllerState& ControllerState = ControllerStates[ ControllerIndex ];

				if (VRSystem->GetControllerState(DeviceIndex, &VRControllerState))
				{
					if (VRControllerState.unPacketNum != ControllerState.PacketNum )
					{
						bool CurrentStates[ ESteamVRControllerButton::TotalButtonCount ] = {0};

						// Get the current state of all buttons
						CurrentStates[ ESteamVRControllerButton::System ] = !!(VRControllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_System));
						CurrentStates[ ESteamVRControllerButton::ApplicationMenu ] = !!(VRControllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu));
						CurrentStates[ ESteamVRControllerButton::TouchPadPress ] = !!(VRControllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));
						CurrentStates[ ESteamVRControllerButton::TouchPadTouch ] = !!( VRControllerState.ulButtonTouched & vr::ButtonMaskFromId( vr::k_EButton_SteamVR_Touchpad ) );
						CurrentStates[ ESteamVRControllerButton::TriggerPress ] = !!(VRControllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger));
						CurrentStates[ ESteamVRControllerButton::Grip ] = !!(VRControllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip));

						// If the touchpad isn't currently pressed or touched, zero put both of the axes
						if (!CurrentStates[ ESteamVRControllerButton::TouchPadTouch ] ||	// @todo steamvr: This should actually be a '&&', not an '||'.  For now we require button to be pressed to have non-zeroed data.  This is hack to workaround faulty hardware that reports the touchpad as pressed all of the time
							!CurrentStates[ ESteamVRControllerButton::TouchPadPress ] )
						{
							VRControllerState.rAxis[TOUCHPAD_AXIS].y = 0.0f;
							VRControllerState.rAxis[TOUCHPAD_AXIS].x = 0.0f;
						}

						CurrentStates[ ESteamVRControllerButton::TouchPadUp ] = !!(VRControllerState.rAxis[TOUCHPAD_AXIS].y > TOUCHPAD_DEADZONE);
						CurrentStates[ ESteamVRControllerButton::TouchPadDown ] = !!(VRControllerState.rAxis[TOUCHPAD_AXIS].y < -TOUCHPAD_DEADZONE);
						CurrentStates[ ESteamVRControllerButton::TouchPadLeft ] = !!(VRControllerState.rAxis[TOUCHPAD_AXIS].x < -TOUCHPAD_DEADZONE);
						CurrentStates[ ESteamVRControllerButton::TouchPadRight ] = !!(VRControllerState.rAxis[TOUCHPAD_AXIS].x > TOUCHPAD_DEADZONE);

						if ( ControllerState.TouchPadXAnalog != VRControllerState.rAxis[TOUCHPAD_AXIS].x)
						{
							MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftAnalogX, ControllerIndex, VRControllerState.rAxis[TOUCHPAD_AXIS].x);
							ControllerState.TouchPadXAnalog = VRControllerState.rAxis[TOUCHPAD_AXIS].x;
						}

						if ( ControllerState.TouchPadYAnalog != VRControllerState.rAxis[TOUCHPAD_AXIS].y)
						{
							MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftAnalogY, ControllerIndex, VRControllerState.rAxis[TOUCHPAD_AXIS].y);
							ControllerState.TouchPadYAnalog = VRControllerState.rAxis[TOUCHPAD_AXIS].y;
						}

						if ( ControllerState.TriggerAnalog != VRControllerState.rAxis[TRIGGER_AXIS].x)
						{
							MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftTriggerAnalog, ControllerIndex, VRControllerState.rAxis[TRIGGER_AXIS].x);
							ControllerState.TriggerAnalog = VRControllerState.rAxis[TRIGGER_AXIS].x;
						}

						// For each button check against the previous state and send the correct message if any
						for (int32 ButtonIndex = 0; ButtonIndex < ESteamVRControllerButton::TotalButtonCount; ++ButtonIndex)
						{
							if (CurrentStates[ButtonIndex] != ControllerState.ButtonStates[ButtonIndex])
							{
								if (CurrentStates[ButtonIndex])
								{
									MessageHandler->OnControllerButtonPressed( Buttons[ (int32)ControllerState.Hand ][ ButtonIndex ], ControllerIndex, false );
								}
								else
								{
									MessageHandler->OnControllerButtonReleased( Buttons[ (int32)ControllerState.Hand ][ ButtonIndex ], ControllerIndex, false );
								}

								if (CurrentStates[ButtonIndex] != 0)
								{
									// this button was pressed - set the button's NextRepeatTime to the InitialButtonRepeatDelay
									ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + InitialButtonRepeatDelay;
								}
							}

							// Update the state for next time
							ControllerState.ButtonStates[ButtonIndex] = CurrentStates[ButtonIndex];
						}

						ControllerState.PacketNum = VRControllerState.unPacketNum;
					}
				}

				for (int32 ButtonIndex = 0; ButtonIndex < ESteamVRControllerButton::TotalButtonCount; ++ButtonIndex)
				{
					if ( ControllerState.ButtonStates[ButtonIndex] != 0 && ControllerState.NextRepeatTime[ButtonIndex] <= CurrentTime)
					{
						MessageHandler->OnControllerButtonPressed( Buttons[ (int32)ControllerState.Hand ][ ButtonIndex ], ControllerIndex, true );

						// set the button's NextRepeatTime to the ButtonRepeatDelay
						ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + ButtonRepeatDelay;
					}
				}
			}
		}
	}


	int32 UnrealControllerIdToControllerIndex( const int32 UnrealControllerId, const EControllerHand Hand ) const
	{
		return UnrealControllerId * (int32)EControllerHand::TotalHandCount + (int32)Hand;
	}

	
	void SetChannelValue(int32 UnrealControllerId, FForceFeedbackChannelType ChannelType, float Value) override
	{
		// Skip unless this is the left or right large channel, which we consider to be the only SteamVRController feedback channel
		if( ChannelType != FForceFeedbackChannelType::LEFT_LARGE && ChannelType != FForceFeedbackChannelType::RIGHT_LARGE )
		{
			return;
		}

		const EControllerHand Hand = ( ChannelType == FForceFeedbackChannelType::LEFT_LARGE ) ? EControllerHand::Left : EControllerHand::Right;
		const int32 ControllerIndex = UnrealControllerIdToControllerIndex( UnrealControllerId, Hand );

		if ((ControllerIndex >= 0) && ( ControllerIndex < MaxControllers))
		{
			FControllerState& ControllerState = ControllerStates[ ControllerIndex ];

			ControllerState.ForceFeedbackValue = Value;

			UpdateVibration( ControllerIndex );
		}
	}

	void SetChannelValues(int32 UnrealControllerId, const FForceFeedbackValues& Values) override
	{
		const int32 LeftControllerIndex = UnrealControllerIdToControllerIndex( UnrealControllerId, EControllerHand::Left );
		if ((LeftControllerIndex >= 0) && ( LeftControllerIndex < MaxControllers))
		{
			FControllerState& ControllerState = ControllerStates[ LeftControllerIndex ];
			ControllerState.ForceFeedbackValue = Values.LeftLarge;

			UpdateVibration( LeftControllerIndex );
		}

		const int32 RightControllerIndex = UnrealControllerIdToControllerIndex( UnrealControllerId, EControllerHand::Right );
		if( ( RightControllerIndex >= 0 ) && ( RightControllerIndex < MaxControllers ) )
		{
			FControllerState& ControllerState = ControllerStates[ RightControllerIndex ];
			ControllerState.ForceFeedbackValue = Values.RightLarge;

			UpdateVibration( RightControllerIndex );
		}
	}

	void UpdateVibration( const int32 ControllerIndex )
	{
		// make sure there is a valid device for this controller
		int32 DeviceIndex = ControllerToDeviceMap[ ControllerIndex ];
		if (DeviceIndex < 0)
		{
			return;
		}

		const FControllerState& ControllerState = ControllerStates[ ControllerIndex ];
		vr::IVRSystem* VRSystem = GetVRSystem();

		if (VRSystem == nullptr)
		{
			return;
		}

		// Map the float values from [0,1] to be more reasonable values for the SteamController.  The docs say that [100,2000] are reasonable values
 		const float LeftIntensity = FMath::Clamp(ControllerState.ForceFeedbackValue * 2000.f, 0.f, 2000.f);
		if (LeftIntensity > 0.f)
		{
			VRSystem->TriggerHapticPulse(DeviceIndex, TOUCHPAD_AXIS, LeftIntensity);
		}
	}

	virtual void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler ) override
	{
		MessageHandler = InMessageHandler;
	}

	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override
	{
		return false;
	}

private:

	inline vr::IVRSystem* GetVRSystem()
	{
		if (SteamVRPlugin == nullptr)
		{
			SteamVRPlugin = &FModuleManager::LoadModuleChecked<ISteamVRPlugin>(TEXT("SteamVR"));
			if (SteamVRPlugin == nullptr)
			{
				return nullptr;
			}
		}

		return SteamVRPlugin->GetVRSystem();
	}



	struct FControllerState
	{
		/** Which hand this controller is representing */
		EControllerHand Hand;

		/** If packet num matches that on your prior call, then the controller state hasn't been changed since 
		  * your last call and there is no need to process it. */
		uint32 PacketNum;

		/** touchpad analog values */
		float TouchPadXAnalog;
		float TouchPadYAnalog;

		/** trigger analog value */
		float TriggerAnalog;

		/** Last frame's button states, so we only send events on edges */
		bool ButtonStates[ ESteamVRControllerButton::TotalButtonCount ];

		/** Next time a repeat event should be generated for each button */
		double NextRepeatTime[ ESteamVRControllerButton::TotalButtonCount ];

		/** Value for force feedback on this controller hand */
		float ForceFeedbackValue;
	};

	/** Mappings between tracked devices and 0 indexed controllers */
	int32 NumControllersMapped;
	int32 ControllerToDeviceMap[ MaxControllers ];
	int32 DeviceToControllerMap[vr::k_unMaxTrackedDeviceCount];

	/** Controller states */
	FControllerState ControllerStates[ MaxControllers ];

	/** Delay before sending a repeat message after a button was first pressed */
	float InitialButtonRepeatDelay;

	/** Delay before sending a repeat message after a button has been pressed for a while */
	float ButtonRepeatDelay;

	/** Mapping of controller buttons */
	FGamepadKeyNames::Type Buttons[ (int32)EControllerHand::TotalHandCount ][ ESteamVRControllerButton::TotalButtonCount ];

	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;

	/** the SteamVR plugin module */
	ISteamVRPlugin* SteamVRPlugin;

	/** weak pointer to the IVRSystem owned by the HMD module */
	TWeakPtr<vr::IVRSystem> HMDVRSystem;
};


class FSteamVRControllerPlugin : public IInputDeviceModule
{
	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		return TSharedPtr< class IInputDevice >(new FSteamVRController(InMessageHandler));

	}
};

IMPLEMENT_MODULE( FSteamVRControllerPlugin, SteamVRController)
