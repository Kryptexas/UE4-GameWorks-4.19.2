// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ISteamVRControllerPlugin.h"
#include "ISteamVRPlugin.h"
#include "IInputDevice.h"
#include "IHapticDevice.h"
#include "IMotionController.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "GenericPlatform/IInputInterface.h"
#include "../../SteamVR/Private/SteamVRHMD.h"

#if PLATFORM_WINDOWS
#include "WindowsHWrapper.h"
#endif

#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
#include "openvr.h"
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS

DEFINE_LOG_CATEGORY_STATIC(LogSteamVRController, Log, All);

/** Total number of controllers in a set */
#define CONTROLLERS_PER_PLAYER	2

#define MAX_TRACKED_DEVICES (int32)EControllerHand::Special_9 - (int32)EControllerHand::Left + 1

/** Player that generic trackers will be assigned to */
#define GENERIC_TRACKER_PLAYER_NUM 0

/** Controller axis mappings. @todo steamvr: should enumerate rather than hard code */
#define TOUCHPAD_AXIS	0
#define TRIGGER_AXIS	1
#define DOT_45DEG		0.7071f

//
// Gamepad thresholds
//
#define TOUCHPAD_DEADZONE  0.0f

// Controls whether or not we need to swap the input routing for the hands, for debugging
static TAutoConsoleVariable<int32> CVarSwapHands(
	TEXT("vr.SwapMotionControllerInput"),
	0,
	TEXT("This command allows you to swap the button / axis input handedness for the input controller, for debugging purposes.\n")
	TEXT(" 0: don't swap (default)\n")
	TEXT(" 1: swap left and right buttons"),
	ECVF_Cheat);

namespace SteamVRControllerKeyNames
{
	const FGamepadKeyNames::Type Touch0("Steam_Touch_0");
	const FGamepadKeyNames::Type Touch1("Steam_Touch_1");
	const FGamepadKeyNames::Type GenericGrip("Steam_Generic_Grip");
	const FGamepadKeyNames::Type GenericTrigger("Steam_Generic_Trigger");
	const FGamepadKeyNames::Type GenericTouchpad("Steam_Generic_Touchpad");
	const FGamepadKeyNames::Type GenericMenu("Steam_Generic_Menu");
	const FGamepadKeyNames::Type GenericSystem("Steam_Generic_System");
}



class FSteamVRController : public IInputDevice, public IMotionController, public IHapticDevice
{
#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	FSteamVRHMD* GetSteamVRHMD() const
	{
		if (GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR))
		{
			return static_cast<FSteamVRHMD*>(GEngine->HMDDevice.Get());
		}

		return nullptr;
	}
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS

public:

	/** The maximum number of Unreal controllers.  Each Unreal controller represents a pair of motion controller devices */
	static const int32 MaxUnrealControllers = MAX_STEAMVR_CONTROLLER_PAIRS;

	/** Total number of motion controllers we'll support */
	static const int32 MaxControllers = MaxUnrealControllers * CONTROLLERS_PER_PLAYER;

	/** The maximum number of Special hand designations available to use for generic trackers 
	 *  Casting enums directly, so if the input model changes, this won't silently be invalid
	 */
	static const int32 MaxSpecialDesignations = (int32)EControllerHand::Special_9 - (int32)EControllerHand::Special_1 + 1;

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
#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
		FMemory::Memzero(ControllerStates, sizeof(ControllerStates));

		for (int32 i=0; i < vr::k_unMaxTrackedDeviceCount; ++i)
		{
			DeviceToControllerMap[i] = INDEX_NONE;
		}

		for (int32 UnrealControllerIndex = 0; UnrealControllerIndex < MaxUnrealControllers; ++UnrealControllerIndex)
		{
			for (int32 HandIndex = 0; HandIndex < vr::k_unMaxTrackedDeviceCount; ++HandIndex)
			{
				UnrealControllerIdAndHandToDeviceIdMap[UnrealControllerIndex][HandIndex] = INDEX_NONE;
			}
		}

		for (int32& Count : UnrealControllerHandUsageCount)
		{
			Count = 0;
		}

		NumControllersMapped = 0;
		NumTrackersMapped = 0;

		InitialButtonRepeatDelay = 0.2f;
		ButtonRepeatDelay = 0.1f;

		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::System ] = FGamepadKeyNames::SpecialLeft;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::ApplicationMenu ] = FGamepadKeyNames::MotionController_Left_Shoulder;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadPress ] = FGamepadKeyNames::MotionController_Left_Thumbstick;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadTouch ] = SteamVRControllerKeyNames::Touch0;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TriggerPress ] = FGamepadKeyNames::MotionController_Left_Trigger;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::Grip ] = FGamepadKeyNames::MotionController_Left_Grip1;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadUp ] = FGamepadKeyNames::MotionController_Left_FaceButton1;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadDown ] = FGamepadKeyNames::MotionController_Left_FaceButton3;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadLeft ] = FGamepadKeyNames::MotionController_Left_FaceButton4;
		Buttons[ (int32)EControllerHand::Left ][ ESteamVRControllerButton::TouchPadRight ] = FGamepadKeyNames::MotionController_Left_FaceButton2;

		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::System ] = FGamepadKeyNames::SpecialRight;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::ApplicationMenu ] = FGamepadKeyNames::MotionController_Right_Shoulder;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadPress ] = FGamepadKeyNames::MotionController_Right_Thumbstick;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadTouch ] = SteamVRControllerKeyNames::Touch1;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TriggerPress ] = FGamepadKeyNames::MotionController_Right_Trigger;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::Grip ] = FGamepadKeyNames::MotionController_Right_Grip1;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadUp ] = FGamepadKeyNames::MotionController_Right_FaceButton1;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadDown ] = FGamepadKeyNames::MotionController_Right_FaceButton3;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadLeft ] = FGamepadKeyNames::MotionController_Right_FaceButton4;
		Buttons[ (int32)EControllerHand::Right ][ ESteamVRControllerButton::TouchPadRight ] = FGamepadKeyNames::MotionController_Right_FaceButton2;

		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::System ] = SteamVRControllerKeyNames::GenericSystem;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::ApplicationMenu ] = SteamVRControllerKeyNames::GenericMenu;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::TouchPadPress ] = SteamVRControllerKeyNames::GenericTouchpad;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::TouchPadTouch ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::TriggerPress ] = SteamVRControllerKeyNames::GenericTrigger;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::Grip ] = SteamVRControllerKeyNames::GenericGrip;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::TouchPadUp ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::TouchPadDown ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::TouchPadLeft ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::Pad ][ ESteamVRControllerButton::TouchPadRight ] = FGamepadKeyNames::Invalid;

		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::System ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::ApplicationMenu ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::TouchPadPress ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::TouchPadTouch ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::TriggerPress ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::Grip ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::TouchPadUp ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::TouchPadDown ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::TouchPadLeft ] = FGamepadKeyNames::Invalid;
		Buttons[ (int32)EControllerHand::ExternalCamera ][ ESteamVRControllerButton::TouchPadRight ] = FGamepadKeyNames::Invalid;

		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::System] = SteamVRControllerKeyNames::GenericSystem;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::ApplicationMenu] = SteamVRControllerKeyNames::GenericMenu;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::TouchPadPress] = FGamepadKeyNames::Invalid;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::TouchPadTouch] = FGamepadKeyNames::Invalid;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::TriggerPress] = SteamVRControllerKeyNames::GenericTrigger;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::Grip] = SteamVRControllerKeyNames::GenericGrip;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::TouchPadUp] = FGamepadKeyNames::Invalid;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::TouchPadDown] = FGamepadKeyNames::Invalid;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::TouchPadLeft] = FGamepadKeyNames::Invalid;
		Buttons[(int32)EControllerHand::Gun][ESteamVRControllerButton::TouchPadRight] = FGamepadKeyNames::Invalid;

		for (int32 SpecialIndex = (int32)EControllerHand::Special_1; SpecialIndex <= (int32)EControllerHand::Special_9; ++SpecialIndex)
		{
			Buttons[SpecialIndex][ESteamVRControllerButton::System] = SteamVRControllerKeyNames::GenericSystem;
			Buttons[SpecialIndex][ESteamVRControllerButton::ApplicationMenu] = SteamVRControllerKeyNames::GenericMenu;
			Buttons[SpecialIndex][ESteamVRControllerButton::TouchPadPress] = SteamVRControllerKeyNames::GenericTouchpad;
			Buttons[SpecialIndex][ESteamVRControllerButton::TouchPadTouch] = FGamepadKeyNames::Invalid;
			Buttons[SpecialIndex][ESteamVRControllerButton::TriggerPress] = SteamVRControllerKeyNames::GenericTrigger;
			Buttons[SpecialIndex][ESteamVRControllerButton::Grip] = SteamVRControllerKeyNames::GenericGrip;
			Buttons[SpecialIndex][ESteamVRControllerButton::TouchPadUp] = FGamepadKeyNames::Invalid;
			Buttons[SpecialIndex][ESteamVRControllerButton::TouchPadDown] = FGamepadKeyNames::Invalid;
			Buttons[SpecialIndex][ESteamVRControllerButton::TouchPadLeft] = FGamepadKeyNames::Invalid;
			Buttons[SpecialIndex][ESteamVRControllerButton::TouchPadRight] = FGamepadKeyNames::Invalid;
		}

		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	}

	virtual ~FSteamVRController()
	{
#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	}

	virtual void Tick( float DeltaTime ) override
	{
	}

	virtual void SendControllerEvents() override
	{
#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
		vr::VRControllerState_t VRControllerState;

		vr::IVRSystem* VRSystem = GetVRSystem();

		if (VRSystem != nullptr)
		{
			const double CurrentTime = FPlatformTime::Seconds();

			bool bNewDeviceConnected = false;

			for (uint32 DeviceIndex=0; DeviceIndex < vr::k_unMaxTrackedDeviceCount; ++DeviceIndex)
			{
				// see what kind of hardware this is
				vr::ETrackedDeviceClass DeviceClass = VRSystem->GetTrackedDeviceClass(DeviceIndex);

				// skip non-controllers
				if (DeviceClass != vr::TrackedDeviceClass_Controller && DeviceClass != vr::TrackedDeviceClass_GenericTracker)
				{
					continue;
				}

				// update the mappings if this is a new device
				if (DeviceToControllerMap[DeviceIndex] == INDEX_NONE )
				{
					// skip unregistered devices that are not connected.
					// This lets connected devices claim their preferred hands.
					if (!VRSystem->IsTrackedDeviceConnected(DeviceIndex))
					{
						continue;
					}

					// don't map too many controllers
					// @todo: this may need to be looked at, vive tracker documentation states that up to 11 trackers and 2 controllers can be connected
					if (NumControllersMapped >= MaxControllers)
					{
						UE_LOG(LogSteamVRController, Warning, TEXT("Found more controllers than we support (%i vs %i)!  Probably need to fix this."), NumControllersMapped + 1, MaxControllers);
						continue;
					}

					// if this device is a controller
					if (DeviceClass == vr::TrackedDeviceClass_Controller)
					{
						// Decide which hand to associate this controller with
						EControllerHand ChosenHand = EControllerHand::Special_9;
						{
							const vr::ETrackedControllerRole Role = VRSystem->GetControllerRoleForTrackedDeviceIndex(DeviceIndex);
							UE_LOG(LogSteamVRController, Log, TEXT("Controller role for device %i is %i (invalid=0, left=1, right=2)."), DeviceIndex, (int32)Role);

							// if we already have one hand we have to put the controller on the other hand
							if (UnrealControllerHandUsageCount[(int32)EControllerHand::Right] != 0 && UnrealControllerHandUsageCount[(int32)EControllerHand::Left] == 0)
							{
								UE_LOG(LogSteamVRController, Log, TEXT("Putting controller %i on the left hand because there is already a controller for the right."), DeviceIndex);
								if (Role == vr::TrackedControllerRole_RightHand)
								{
									UE_LOG(LogSteamVRController, Warning, TEXT("We are ignoring the steam api controller role for device %i, because we have already used that spot."), DeviceIndex);
								}
								ChosenHand = EControllerHand::Left;
							}
							else if (UnrealControllerHandUsageCount[(int32)EControllerHand::Left] != 0 && UnrealControllerHandUsageCount[(int32)EControllerHand::Right] == 0)
							{
								UE_LOG(LogSteamVRController, Log, TEXT("Putting controller %i on the right hand because there is already a controller for the left."), DeviceIndex);
								if (Role == vr::TrackedControllerRole_LeftHand)
								{
									UE_LOG(LogSteamVRController, Warning, TEXT("We are ignoring the steam api controller role for device %i, because we have already used that spot."), DeviceIndex);
								}
								ChosenHand = EControllerHand::Right;
							}
							else
							{
								// Either both controller hands are unused or both are used.

								// Try to give the controller to the role it prefers.
								switch (Role)
								{
								case vr::TrackedControllerRole_LeftHand:
									UE_LOG(LogSteamVRController, Warning, TEXT("Controller device %i is being assigned to its prefered role 'left'."), DeviceIndex);
									ChosenHand = EControllerHand::Left;
									break;
								case vr::TrackedControllerRole_RightHand:
									UE_LOG(LogSteamVRController, Warning, TEXT("Controller device %i is being assigned to its prefered role 'right'."), DeviceIndex);
									ChosenHand = EControllerHand::Right;
									break;
								case vr::TrackedControllerRole_Invalid:
								{
									const uint32 LeftDeviceIndex = VRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
									const uint32 RightDeviceIndex = VRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
									if (RightDeviceIndex == vr::k_unTrackedDeviceIndexInvalid)
									{
										UE_LOG(LogSteamVRController, Warning, TEXT("Controller device %i has no role set.  There is no 'right' controller according to steamvr, so we are pickign that."), DeviceIndex);
										ChosenHand = EControllerHand::Right;
									}
									else if (LeftDeviceIndex == vr::k_unTrackedDeviceIndexInvalid)
									{
										UE_LOG(LogSteamVRController, Warning, TEXT("Controller device %i has no role set.  There is no 'left' controller according to steamvr, so we are pickign that."), DeviceIndex);
										ChosenHand = EControllerHand::Left;
									}
									else
									{
										UE_LOG(LogSteamVRController, Warning, TEXT("Controller device %i has no role set.  We could not find an unused role, so picking Right.  This controller may not function correctly."), DeviceIndex);
										ChosenHand = EControllerHand::Right;
									}
								}
								break;
								default:
									UE_LOG(LogSteamVRController, Warning, TEXT("Controller device with unknown role %i encountered.  Ignoring it."), (int32)Role);
									continue;
								}
							}
						}

						// determine which player controller to assign the device to
						int32 ControllerIndex = FMath::FloorToInt(NumControllersMapped / CONTROLLERS_PER_PLAYER);

						UE_LOG(LogSteamVRController, Verbose, TEXT("Controller device %i is being assigned unreal hand %i (left=0, right=1), for player %i."), DeviceIndex, (int32)ChosenHand, ControllerIndex);
						ControllerStates[DeviceIndex].Hand = ChosenHand;
						UnrealControllerHandUsageCount[(int32)ChosenHand] += 1;

						DeviceToControllerMap[DeviceIndex] = ControllerIndex;

						++NumControllersMapped;

						// a new valid controller was mapped
						bNewDeviceConnected = true;

						UnrealControllerIdAndHandToDeviceIdMap[DeviceToControllerMap[DeviceIndex]][(int32)ControllerStates[DeviceIndex].Hand] = DeviceIndex;
					}
					// or if the device is a tracker
					else if (DeviceClass == vr::TrackedDeviceClass_GenericTracker)
					{
						// check to see if there are any Special designations left, skip mapping it if there are not
						if (NumTrackersMapped >= MaxSpecialDesignations)
						{
							// go ahead and increment, so we can display a little more info in the log
							++NumTrackersMapped;
							UE_LOG(LogSteamVRController, Warning, TEXT("Unable to map VR tracker (#%i) to Special hand designation!"), NumTrackersMapped);
							continue;
						}

						// add the tracker to player 0
						DeviceToControllerMap[DeviceIndex] = GENERIC_TRACKER_PLAYER_NUM;

						// select next special #
						switch (NumTrackersMapped)
						{
						case 0:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_1;
							break;
						case 1:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_2;
							break;
						case 2:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_3;
							break;
						case 3:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_4;
							break;
						case 4:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_5;
							break;
						case 5:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_6;
							break;
						case 6:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_7;
							break;
						case 7:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_8;
							break;
						case 8:
							ControllerStates[DeviceIndex].Hand = EControllerHand::Special_9;
							break;
						default:
							// initial mapping verification above should catch any erroneous NumTrackersMapped
							check(false);
							break;
						}

						++NumTrackersMapped;
						UE_LOG(LogSteamVRController, Log, TEXT("Tracker device %i is being assigned unreal hand: Special %i, for player %i"), DeviceIndex, NumTrackersMapped, GENERIC_TRACKER_PLAYER_NUM);

						// a new valid tracker was mapped
						bNewDeviceConnected = true;

						UnrealControllerIdAndHandToDeviceIdMap[DeviceToControllerMap[DeviceIndex]][(int32)ControllerStates[DeviceIndex].Hand] = DeviceIndex;
					}

					
				}

				// get the controller index for this device
				int32 ControllerIndex = DeviceToControllerMap[DeviceIndex];
				FControllerState& ControllerState = ControllerStates[ DeviceIndex ];
				EControllerHand HandToUse = ControllerState.Hand;

				// see if this is a hand specific controller
				if (HandToUse == EControllerHand::Left || HandToUse == EControllerHand::Right)
				{
					// check to see if we need to swap input hands for debugging
					static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.SwapMotionControllerInput"));
					bool bSwapHandInput = (CVar->GetValueOnGameThread() != 0) ? true : false;
					if (bSwapHandInput)
					{
						HandToUse = (HandToUse == EControllerHand::Left) ? EControllerHand::Right : EControllerHand::Left;
					}
				}

				if (VRSystem->GetControllerState(DeviceIndex, &VRControllerState, sizeof(vr::VRControllerState_t)))
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
						if (!CurrentStates[ ESteamVRControllerButton::TouchPadTouch ])
						{
 							VRControllerState.rAxis[TOUCHPAD_AXIS].y = 0.0f;
 							VRControllerState.rAxis[TOUCHPAD_AXIS].x = 0.0f;
						}

						// D-pad emulation
						const FVector2D TouchDir = FVector2D(VRControllerState.rAxis[TOUCHPAD_AXIS].x, VRControllerState.rAxis[TOUCHPAD_AXIS].y).GetSafeNormal();
						const FVector2D UpDir(0.f, 1.f);
						const FVector2D RightDir(1.f, 0.f);

						const float VerticalDot = TouchDir | UpDir;
						const float RightDot = TouchDir | RightDir;

						const bool bPressed = !TouchDir.IsNearlyZero() && CurrentStates[ ESteamVRControllerButton::TouchPadPress ];
						
						CurrentStates[ ESteamVRControllerButton::TouchPadUp ]		= bPressed && (VerticalDot >= DOT_45DEG);
						CurrentStates[ ESteamVRControllerButton::TouchPadDown ]		= bPressed && (VerticalDot <= -DOT_45DEG);
						CurrentStates[ ESteamVRControllerButton::TouchPadLeft ]		= bPressed && (RightDot <= -DOT_45DEG);
						CurrentStates[ ESteamVRControllerButton::TouchPadRight ]	= bPressed && (RightDot >= DOT_45DEG);

						if ( ControllerState.TouchPadXAnalog != VRControllerState.rAxis[TOUCHPAD_AXIS].x)
						{
							const FGamepadKeyNames::Type AxisButton = (HandToUse == EControllerHand::Left) ? FGamepadKeyNames::MotionController_Left_Thumbstick_X : FGamepadKeyNames::MotionController_Right_Thumbstick_X;
							MessageHandler->OnControllerAnalog(AxisButton, ControllerIndex, VRControllerState.rAxis[TOUCHPAD_AXIS].x);
							ControllerState.TouchPadXAnalog = VRControllerState.rAxis[TOUCHPAD_AXIS].x;
						}

						if ( ControllerState.TouchPadYAnalog != VRControllerState.rAxis[TOUCHPAD_AXIS].y)
						{
							const FGamepadKeyNames::Type AxisButton = (HandToUse == EControllerHand::Left) ? FGamepadKeyNames::MotionController_Left_Thumbstick_Y : FGamepadKeyNames::MotionController_Right_Thumbstick_Y;
							// Invert the y to match UE4 convention
							const float Value = -VRControllerState.rAxis[TOUCHPAD_AXIS].y;
							MessageHandler->OnControllerAnalog(AxisButton, ControllerIndex, Value);
							ControllerState.TouchPadYAnalog = Value;
						}

						if ( ControllerState.TriggerAnalog != VRControllerState.rAxis[TRIGGER_AXIS].x)
						{
							const FGamepadKeyNames::Type AxisButton = (HandToUse == EControllerHand::Left) ? FGamepadKeyNames::MotionController_Left_TriggerAxis : FGamepadKeyNames::MotionController_Right_TriggerAxis;
							MessageHandler->OnControllerAnalog(AxisButton, ControllerIndex, VRControllerState.rAxis[TRIGGER_AXIS].x);
							ControllerState.TriggerAnalog = VRControllerState.rAxis[TRIGGER_AXIS].x;
						}

						// For each button check against the previous state and send the correct message if any
						for (int32 ButtonIndex = 0; ButtonIndex < ESteamVRControllerButton::TotalButtonCount; ++ButtonIndex)
						{
							if (CurrentStates[ButtonIndex] != ControllerState.ButtonStates[ButtonIndex])
							{
								if (CurrentStates[ButtonIndex])
								{
									MessageHandler->OnControllerButtonPressed( Buttons[ (int32)HandToUse ][ ButtonIndex ], ControllerIndex, false );
								}
								else
								{
									MessageHandler->OnControllerButtonReleased( Buttons[ (int32)HandToUse ][ ButtonIndex ], ControllerIndex, false );
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
						MessageHandler->OnControllerButtonPressed( Buttons[ (int32)HandToUse ][ ButtonIndex ], ControllerIndex, true );

						// set the button's NextRepeatTime to the ButtonRepeatDelay
						ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + ButtonRepeatDelay;
					}
				}
			}

			// if a new device was connected, update the SteamVR plugin with the new mapping
			if (bNewDeviceConnected == true)
			{
				SteamVRPlugin->SetUnrealControllerIdAndHandToDeviceIdMap(UnrealControllerIdAndHandToDeviceIdMap);
			}
		}
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	}

#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	int32 UnrealControllerIdToControllerIndex( const int32 UnrealControllerId, const EControllerHand Hand ) const
	{
		return UnrealControllerIdAndHandToDeviceIdMap[UnrealControllerId][(int32)Hand];
	}
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	
	void SetChannelValue(int32 UnrealControllerId, FForceFeedbackChannelType ChannelType, float Value) override
	{
#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
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
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	}

	void SetChannelValues(int32 UnrealControllerId, const FForceFeedbackValues& Values) override
	{
#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
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
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	}

	virtual IHapticDevice* GetHapticDevice() override
	{
		return this;
	}	

	virtual void SetHapticFeedbackValues(int32 UnrealControllerId, int32 Hand, const FHapticFeedbackValues& Values) override
	{
#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
		if (Hand != (int32)EControllerHand::Left && Hand != (int32)EControllerHand::Right)
		{
			return;
		}

		const int32 ControllerIndex = UnrealControllerIdToControllerIndex(UnrealControllerId, (EControllerHand)Hand);
		if (ControllerIndex >= 0 && ControllerIndex < MaxControllers)
		{
			FControllerState& ControllerState = ControllerStates[ControllerIndex];
			ControllerState.ForceFeedbackValue = (Values.Frequency > 0.0f) ? Values.Amplitude : 0.0f;

			UpdateVibration(ControllerIndex);
		}
#endif
	}

	virtual void GetHapticFrequencyRange(float& MinFrequency, float& MaxFrequency) const override
	{
		MinFrequency = 0.0f;
		MaxFrequency = 1.0f;
	}
	
	virtual float GetHapticAmplitudeScale() const override
	{
		return 1.0f;
	}

#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
	void UpdateVibration( const int32 ControllerIndex )
	{

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
			VRSystem->TriggerHapticPulse(ControllerIndex, TOUCHPAD_AXIS, LeftIntensity);
		}
	}
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS

	virtual void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler ) override
	{
		MessageHandler = InMessageHandler;
	}

	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override
	{
		return false;
	}

	virtual FName GetMotionControllerDeviceTypeName() const override
	{
		const static FName DefaultName(TEXT("SteamVRController"));
		return DefaultName;
	}

	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const
	{
		bool RetVal = false;

#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
 		FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
 		if (SteamVRHMD)
 		{
 			FQuat DeviceOrientation = FQuat::Identity;
			// Steam handles WorldToMetersScale when it reads the controller posrot, so we do not need to use it again here.  Debugging found that they are the same.
 			RetVal = SteamVRHMD->GetControllerHandPositionAndOrientation(ControllerIndex, DeviceHand, OutPosition, DeviceOrientation);
 			OutOrientation = DeviceOrientation.Rotator();
 		}
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS

		return RetVal;
	}

	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const
	{
		ETrackingStatus TrackingStatus = ETrackingStatus::NotTracked;

#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS
		FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
 		if (SteamVRHMD)
 		{
			TrackingStatus = SteamVRHMD->GetControllerTrackingStatus(ControllerIndex, DeviceHand);
 		}
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS

		return TrackingStatus;
	}

#if STEAMVRCONTROLLER_SUPPORTED_PLATFORMS

	virtual bool IsGamepadAttached() const override
	{
		FSteamVRHMD* SteamVRSystem = GetSteamVRHMD();

		if (SteamVRSystem != nullptr)
		{
			// Check if at least one motion controller is tracked
			// Only need to check for at least one player (player index 0)
			int32 PlayerIndex = 0;
			ETrackingStatus LeftHandTrackingStatus = GetControllerTrackingStatus(PlayerIndex, EControllerHand::Left);
			ETrackingStatus RightHandTrackingStatus = GetControllerTrackingStatus(PlayerIndex, EControllerHand::Right);

			return LeftHandTrackingStatus == ETrackingStatus::Tracked || RightHandTrackingStatus == ETrackingStatus::Tracked;
		}

		return false;
	}

private:

	inline vr::IVRSystem* GetVRSystem()
	{
		if (SteamVRPlugin == nullptr)
		{
			SteamVRPlugin = &FModuleManager::LoadModuleChecked<ISteamVRPlugin>(TEXT("SteamVR"));
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
	int32 NumTrackersMapped;
	int32 DeviceToControllerMap[ vr::k_unMaxTrackedDeviceCount ];
	int32 UnrealControllerIdAndHandToDeviceIdMap[ MaxUnrealControllers ][ vr::k_unMaxTrackedDeviceCount ];
	int32 UnrealControllerHandUsageCount[CONTROLLERS_PER_PLAYER];

	/** Controller states */
	FControllerState ControllerStates[ MaxControllers ];

	/** Delay before sending a repeat message after a button was first pressed */
	float InitialButtonRepeatDelay;

	/** Delay before sending a repeat message after a button has been pressed for a while */
	float ButtonRepeatDelay;

	/** Mapping of controller buttons */
	FGamepadKeyNames::Type Buttons[ MAX_TRACKED_DEVICES ][ ESteamVRControllerButton::TotalButtonCount ];

	/** weak pointer to the IVRSystem owned by the HMD module */
	TWeakPtr<vr::IVRSystem> HMDVRSystem;
#endif // STEAMVRCONTROLLER_SUPPORTED_PLATFORMS

	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;

	/** the SteamVR plugin module */
	ISteamVRPlugin* SteamVRPlugin;
};


class FSteamVRControllerPlugin : public ISteamVRControllerPlugin
{
	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		return TSharedPtr< class IInputDevice >(new FSteamVRController(InMessageHandler));

	}
};

IMPLEMENT_MODULE( FSteamVRControllerPlugin, SteamVRController)
