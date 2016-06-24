// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ViewportInteractionModule.h"
#include "ViewportInteractorMotionController.h"
#include "ViewportInteractorData.h"
#include "ViewportWorldInteraction.h"

#include "IMotionController.h"
#include "IHeadMountedDisplay.h"
#include "MotionControllerComponent.h"
#include "Features/IModularFeatures.h"

namespace VI
{
	//Laser
	static FAutoConsoleVariable OculusLaserPointerRotationOffset( TEXT( "VI.OculusLaserPointerRotationOffset" ), 0.0f, TEXT( "How much to rotate the laser pointer (pitch) relative to the forward vector of the controller (Oculus)" ) );
	static FAutoConsoleVariable ViveLaserPointerRotationOffset( TEXT( "VI.ViveLaserPointerRotationOffset" ), /* -57.8f */ 0.0f, TEXT( "How much to rotate the laser pointer (pitch) relative to the forward vector of the controller (Vive)" ) );
	static FAutoConsoleVariable OculusLaserPointerStartOffset( TEXT( "VI.OculusLaserPointerStartOffset" ), 2.8f, TEXT( "How far to offset the start of the laser pointer to avoid overlapping the hand mesh geometry (Oculus)" ) );
	static FAutoConsoleVariable ViveLaserPointerStartOffset( TEXT( "VI.ViveLaserPointerStartOffset" ), 1.25f /* 8.5f */, TEXT( "How far to offset the start of the laser pointer to avoid overlapping the hand mesh geometry (Vive)" ) );
	
	//Trigger
	static FAutoConsoleVariable TriggerLightlyPressedThreshold( TEXT( "VI.TriggerLightlyPressedThreshold" ), 0.03f, TEXT( "Minimum trigger threshold before we consider the trigger at least 'lightly pressed'" ) );
	static FAutoConsoleVariable TriggerDeadZone( TEXT( "VI.TriggerDeadZone" ), 0.01f, TEXT( "Trigger dead zone.  The trigger must be fully released before we'll trigger a new 'light press'" ) );
	static FAutoConsoleVariable TriggerFullyPressedThreshold( TEXT( "VI.TriggerFullyPressedThreshold" ), 0.95f, TEXT( "Minimum trigger threshold before we consider the trigger 'fully pressed'" ) );
	static FAutoConsoleVariable TriggerFullyPressedReleaseThreshold( TEXT( "VI.TriggerFullyPressedReleaseThreshold" ), 0.8f, TEXT( "After fully pressing the trigger, if the axis falls below this threshold we no longer consider it fully pressed" ) );

	// Haptic feedback
	static FAutoConsoleVariable SleepForRiftHaptics( TEXT( "VI.SleepForRiftHaptics" ), 1, TEXT( "When enabled, we'll sleep the game thread mid-frame to wait for haptic effects to finish.  This can be devasting to performance!" ) );
	static FAutoConsoleVariable MinHapticTimeForRift( TEXT( "VREd.MinHapticTimeForRift" ), 0.005f, TEXT( "How long to play haptic effects on the Rift" ) );
}

const FName UViewportInteractorMotionController::TrackpadPositionX = FName( "TrackpadPositionX" );
const FName UViewportInteractorMotionController::TrackpadPositionY = FName( "TrackpadPositionY" );
const FName UViewportInteractorMotionController::TriggerAxis = FName( "TriggerAxis" );
const FName UViewportInteractorMotionController::MotionController_Left_FullyPressedTriggerAxis = FName( "MotionController_Left_FullyPressedTriggerAxis" );
const FName UViewportInteractorMotionController::MotionController_Right_FullyPressedTriggerAxis = FName( "MotionController_Right_FullyPressedTriggerAxis" );
const FName UViewportInteractorMotionController::MotionController_Left_LightlyPressedTriggerAxis = FName( "MotionController_Left_LightlyPressedTriggerAxis" );
const FName UViewportInteractorMotionController::MotionController_Right_LightlyPressedTriggerAxis = FName( "MotionController_Right_LightlyPressedTriggerAxis" );

UViewportInteractorMotionController::UViewportInteractorMotionController( const FObjectInitializer& Initializer ) :
	UViewportInteractor( Initializer ),
	MotionControllerComponent( nullptr ),
	HandMeshComponent( nullptr ),
	LaserPointerMeshComponent( nullptr ),
	LaserPointerMID( nullptr ),
	TranslucentLaserPointerMID( nullptr ),
	HoverMeshComponent( nullptr ),
	HoverPointLightComponent( nullptr ),
	HandMeshMID( nullptr ),
	ControllerHandSide( EControllerHand::Pad ),
	bHaveMotionController( false ),
	bIsTriggerFullyPressed( false ),
	bIsTriggerAtLeastLightlyPressed( false ),
	RealTimeTriggerWasLightlyPressed( 0.0f ),
	bHasTriggerBeenReleasedSinceLastPress( true ),
	SelectAndMoveTriggerValue( 0.0f )
{
}

UViewportInteractorMotionController::~UViewportInteractorMotionController()
{
	Shutdown();
}

void UViewportInteractorMotionController::Shutdown()
{
	Super::Shutdown();

	MotionControllerComponent = nullptr;
	HandMeshComponent = nullptr;
	LaserPointerMeshComponent = nullptr;
	LaserPointerMID = nullptr;
	TranslucentLaserPointerMID = nullptr;
	HoverMeshComponent = nullptr;
	HoverPointLightComponent = nullptr;
	HandMeshComponent = nullptr;
}

void UViewportInteractorMotionController::Tick( const float DeltaTime )
{
	Super::Tick( DeltaTime );

	{
		const float WorldScaleFactor = WorldInteraction->GetWorldScaleFactor();

		// @todo vreditor: Manually ticking motion controller components
		MotionControllerComponent->TickComponent( DeltaTime, ELevelTick::LEVELTICK_PauseTick, nullptr );

		// The hands need to stay the same size relative to our tracking space, so we inverse compensate for world to meters scale here
		// NOTE: We don't need to set the hand mesh location and rotation, as the MotionControllerComponent does that itself
		if ( ControllerHandSide == EControllerHand::Right &&
			GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )	// Oculus has asymmetrical controllers, so we mirror the mesh horizontally
		{
			HandMeshComponent->SetRelativeScale3D( FVector( WorldScaleFactor, -WorldScaleFactor, WorldScaleFactor ) );
		}
		else
		{
			HandMeshComponent->SetRelativeScale3D( FVector( WorldScaleFactor ) );
		}
	}
}

EHMDDeviceType::Type UViewportInteractorMotionController::GetHMDDeviceType() const
{
	return GEngine->HMDDevice.IsValid() ? GEngine->HMDDevice->GetHMDDeviceType() : EHMDDeviceType::DT_SteamVR; //@todo: ViewportInteraction, assumption that it's steamvr ??
}

void UViewportInteractorMotionController::HandleInputAxis( FViewportActionKeyInput& Action, const FKey Key, const float Delta, const float DeltaTime, bool& bOutWasHandled )
{
	if ( Action.ActionType == TriggerAxis )
	{
		// Synthesize "lightly pressed" events for the trigger
		{
			// Store latest trigger value amount
			SelectAndMoveTriggerValue = Delta;

			FViewportActionKeyInput* OptionalAction = GetActionWithName( ViewportWorldActionTypes::SelectAndMove );
			const bool bIsFullPressAlreadyCapturing = OptionalAction && OptionalAction->bIsInputCaptured;
				
			if ( !bIsFullPressAlreadyCapturing &&		// Don't fire if we're already capturing input for a full press
				!bIsTriggerAtLeastLightlyPressed &&	// Don't fire unless we are already pressed
				bHasTriggerBeenReleasedSinceLastPress &&	// Only if we've been fully released since the last time we fired
				Delta >= VI::TriggerLightlyPressedThreshold->GetFloat() )
			{
				bIsTriggerAtLeastLightlyPressed = true;
				SetAllowTriggerLightPressLocking( true );
				RealTimeTriggerWasLightlyPressed = FPlatformTime::Seconds();
				bHasTriggerBeenReleasedSinceLastPress = false;

				// Synthesize an input key for this light press
				const EInputEvent InputEvent = IE_Pressed;
				const bool bWasLightPressHandled = UViewportInteractor::HandleInputKey( ControllerHandSide == EControllerHand::Left ? MotionController_Left_LightlyPressedTriggerAxis : MotionController_Right_LightlyPressedTriggerAxis, InputEvent );
			}
			else if ( bIsTriggerAtLeastLightlyPressed && Delta < VI::TriggerLightlyPressedThreshold->GetFloat() )
			{
				bIsTriggerAtLeastLightlyPressed = false;

				// Synthesize an input key for this light press
				const EInputEvent InputEvent = IE_Released;
				const bool bWasLightReleaseHandled = UViewportInteractor::HandleInputKey( ControllerHandSide == EControllerHand::Left ? MotionController_Left_LightlyPressedTriggerAxis : MotionController_Right_LightlyPressedTriggerAxis, InputEvent );
			}
		}

		if ( Delta < VI::TriggerDeadZone->GetFloat() )
		{
			bHasTriggerBeenReleasedSinceLastPress = true;
		}

		// Synthesize "fully pressed" events for the trigger
		{
			if ( !bIsTriggerFullyPressed &&	// Don't fire unless we are already pressed
				Delta >= VI::TriggerFullyPressedThreshold->GetFloat() )
			{
				bIsTriggerFullyPressed = true;

				// Synthesize an input key for this full press
				const EInputEvent InputEvent = IE_Pressed;
				UViewportInteractor::HandleInputKey( ControllerHandSide == EControllerHand::Left ? MotionController_Left_FullyPressedTriggerAxis : MotionController_Right_FullyPressedTriggerAxis, InputEvent );
			}
			else if ( bIsTriggerFullyPressed && Delta < VI::TriggerFullyPressedReleaseThreshold->GetFloat() )
			{
				bIsTriggerFullyPressed = false;

				// Synthesize an input key for this full press
				const EInputEvent InputEvent = IE_Released;
				UViewportInteractor::HandleInputKey( ControllerHandSide == EControllerHand::Left ? MotionController_Left_FullyPressedTriggerAxis : MotionController_Right_FullyPressedTriggerAxis, InputEvent );
			}
		}
	}
}

void UViewportInteractorMotionController::PollInput()
{
	bHaveMotionController = false;
	InteractorData.LastTransform = InteractorData.Transform;
	InteractorData.LastRoomSpaceTransform = InteractorData.RoomSpaceTransform;

	// Generic motion controllers
	TArray<IMotionController*> MotionControllers = IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>( IMotionController::GetModularFeatureName() );
	for ( auto MotionController : MotionControllers )	// @todo viewportinteraction: Needs support for multiple pairs of motion controllers
	{
		if ( MotionController != nullptr && !bHaveMotionController )
		{
			FVector Location = FVector::ZeroVector;
			FRotator Rotation = FRotator::ZeroRotator;

			if ( MotionController->GetControllerOrientationAndPosition( WorldInteraction->GetMotionControllerID(), ControllerHandSide, /* Out */ Rotation, /* Out */ Location ) )
			{
				bHaveMotionController = true;
				InteractorData.RoomSpaceTransform = FTransform( Rotation.Quaternion(), Location, FVector( 1.0f ) );
				InteractorData.Transform = InteractorData.RoomSpaceTransform * WorldInteraction->GetRoomTransform();
			}
		}
	}
}

bool UViewportInteractorMotionController::IsTriggerAtLeastLightlyPressed() const
{
	return bIsTriggerAtLeastLightlyPressed;
}

double UViewportInteractorMotionController::GetRealTimeTriggerWasLightlyPressed() const
{
	return RealTimeTriggerWasLightlyPressed;
}

void UViewportInteractorMotionController::SetTriggerAtLeastLightlyPressed( const bool bInTriggerAtLeastLightlyPressed )
{
	bIsTriggerAtLeastLightlyPressed = bInTriggerAtLeastLightlyPressed;
}

void UViewportInteractorMotionController::SetTriggerBeenReleasedSinceLastPress( const bool bInTriggerBeenReleasedSinceLastPress )
{
	bHasTriggerBeenReleasedSinceLastPress = bInTriggerBeenReleasedSinceLastPress;
}

void UViewportInteractorMotionController::PlayHapticEffect( const float Strength )
{
	IInputInterface* InputInterface = FSlateApplication::Get().GetInputInterface();
	if ( InputInterface )
	{
		const double CurrentTime = FPlatformTime::Seconds();

		// If we're dealing with an Oculus Rift, we have to setup haptic feedback directly.  Otherwise we can use our
		// generic force feedback system
		if ( GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )
		{
			// Haptics are a little strong on Oculus Touch, so we scale them down a bit
			const float HapticScaleForRift = 0.8f;

			FHapticFeedbackValues HapticFeedbackValues;
			HapticFeedbackValues.Amplitude = Strength * HapticScaleForRift;
			HapticFeedbackValues.Frequency = 0.5f;
			InputInterface->SetHapticFeedbackValues( WorldInteraction->GetMotionControllerID(), ( int32 )ControllerHandSide, HapticFeedbackValues );

			LastHapticTime = CurrentTime;
		}
		else
		{
			//@todo viewportinteration
			FForceFeedbackValues ForceFeedbackValues;
			ForceFeedbackValues.LeftLarge = ControllerHandSide == EControllerHand::Left ? Strength : 0;
			ForceFeedbackValues.RightLarge = ControllerHandSide == EControllerHand::Right ? Strength : 0;

			// @todo vreditor: If an Xbox controller is plugged in, this causes both the motion controllers and the Xbox controller to vibrate!
			InputInterface->SetForceFeedbackChannelValues( WorldInteraction->GetMotionControllerID(), ForceFeedbackValues );

			if ( ForceFeedbackValues.LeftLarge > KINDA_SMALL_NUMBER )
			{
				LastHapticTime = CurrentTime;
			}

			if ( ForceFeedbackValues.RightLarge > KINDA_SMALL_NUMBER )
			{
				LastHapticTime = CurrentTime;
			}
		}
	}

	// @todo vreditor: We'll stop haptics right away because if the frame hitches, the controller will be left vibrating
	StopOldHapticEffects();
}

void UViewportInteractorMotionController::StopOldHapticEffects()
{
	const float MinHapticTime = VI::MinHapticTimeForRift->GetFloat();

	IInputInterface* InputInterface = FSlateApplication::Get().GetInputInterface();
	if ( InputInterface )
	{
		// If we're dealing with an Oculus Rift, we have to setup haptic feedback directly.  Otherwise we can use our
		// generic force feedback system
		if ( GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )
		{
			bool bWaitingForMoreHaptics = false;

			if ( LastHapticTime != 0.0f )
			{
				do
				{
					bWaitingForMoreHaptics = false;

					const double CurrentTime = FPlatformTime::Seconds();

					// Left hand
					if ( CurrentTime - LastHapticTime >= MinHapticTime )
					{
						FHapticFeedbackValues HapticFeedbackValues;
						HapticFeedbackValues.Amplitude = 0.0f;
						HapticFeedbackValues.Frequency = 0.0f;
						InputInterface->SetHapticFeedbackValues( WorldInteraction->GetMotionControllerID(), ( int32 ) ControllerHandSide, HapticFeedbackValues );

						LastHapticTime = 0.0;
					}
					else if ( LastHapticTime != 0.0 )
					{
						bWaitingForMoreHaptics = true;
					}

					if ( bWaitingForMoreHaptics && VI::SleepForRiftHaptics->GetInt() != 0 )
					{
						FPlatformProcess::Sleep( 0 );
					}
				}
				// @todo vreditor urgent: This is needed so that haptics don't play too long.  Our Oculus code doesn't currently support 
				// multi-threading, so we need to delay the main thread to make sure we stop it before it rumbles for more than an instant!
				while ( bWaitingForMoreHaptics && VI::SleepForRiftHaptics->GetInt() != 0 );
			}
		}
		else
		{
			// @todo vreditor: Do we need to cancel haptics for non-Rift devices?  Doesn't seem like it
		}
	}
}

bool UViewportInteractorMotionController::GetTransformAndForwardVector( FTransform& OutHandTransform, FVector& OutForwardVector )
{
	if ( bHaveMotionController )
	{
		OutHandTransform = InteractorData.Transform;

		const float LaserPointerRotationOffset = GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift ? VI::OculusLaserPointerRotationOffset->GetFloat() : VI::ViveLaserPointerRotationOffset->GetFloat();
		OutForwardVector = OutHandTransform.GetRotation().RotateVector( FRotator( LaserPointerRotationOffset, 0.0f, 0.0f ).RotateVector( FVector( 1.0f, 0.0f, 0.0f ) ) );

		return true;
	}

	return false;
}

EControllerHand UViewportInteractorMotionController::GetControllerSide() const
{
	return ControllerHandSide;
}

UMotionControllerComponent* UViewportInteractorMotionController::GetMotionControllerComponent() const
{
	return MotionControllerComponent;
}

float UViewportInteractorMotionController::GetSelectAndMoveTriggerValue() const
{
	return SelectAndMoveTriggerValue;
}
