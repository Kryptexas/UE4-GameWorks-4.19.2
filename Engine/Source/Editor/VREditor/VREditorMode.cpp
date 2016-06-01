// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorMode.h"
#include "VREditorUISystem.h"
#include "VREditorWorldInteraction.h"
#include "VREditorTransformGizmo.h"
#include "VREditorFloatingText.h"
#include "VREditorFloatingUI.h"

#include "CameraController.h"
#include "DynamicMeshBuilder.h"
#include "Features/IModularFeatures.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "IMotionController.h"
#include "MotionControllerComponent.h"
#include "EngineAnalytics.h"
#include "IHeadMountedDisplay.h"

#include "Editor/LevelEditor/Public/LevelEditorActions.h"

#define LOCTEXT_NAMESPACE "VREditorMode"

namespace VREd
{
	static FAutoConsoleVariable LaserPointerMaxLength( TEXT( "VREd.LaserPointerMaxLength" ), 10000.0f, TEXT( "Maximum length of the laser pointer line" ) );
	static FAutoConsoleVariable LaserPointerRadius( TEXT( "VREd.LaserPointerRadius" ), 0.5f, TEXT( "Radius of the laser pointer line" ) );
	static FAutoConsoleVariable LaserPointerHoverBallRadius( TEXT( "VREd.LaserPointerHoverBallRadius" ), 1.5f, TEXT( "Radius of the visual cue for a hovered object along the laser pointer ray" ) );
	static FAutoConsoleVariable LaserPointerLightPullBackDistance( TEXT( "VREd.LaserPointerLightPullBackDistance" ), 2.5f, TEXT( "How far to pull back our little hover light from the impact surface" ) );
	static FAutoConsoleVariable LaserPointerLightRadius( TEXT( "VREd.LaserPointLightRadius" ), 20.0f, TEXT( "How big our hover light is" ) );
	static FAutoConsoleVariable OculusLaserPointerRotationOffset( TEXT( "VREd.OculusLaserPointerRotationOffset" ), 0.0f, TEXT( "How much to rotate the laser pointer (pitch) relative to the forward vector of the controller (Oculus)" ) );
	static FAutoConsoleVariable ViveLaserPointerRotationOffset( TEXT( "VREd.ViveLaserPointerRotationOffset" ), /* -57.8f */ 0.0f, TEXT( "How much to rotate the laser pointer (pitch) relative to the forward vector of the controller (Vive)" ) );
	static FAutoConsoleVariable OculusLaserPointerStartOffset( TEXT( "VREd.OculusLaserPointerStartOffset" ), 2.8f, TEXT( "How far to offset the start of the laser pointer to avoid overlapping the hand mesh geometry (Oculus)" ) );
	static FAutoConsoleVariable ViveLaserPointerStartOffset( TEXT( "VREd.ViveLaserPointerStartOffset" ), 1.25f /* 8.5f */, TEXT( "How far to offset the start of the laser pointer to avoid overlapping the hand mesh geometry (Vive)" ) );

	static FAutoConsoleVariable UseMouseAsHandInForcedVRMode( TEXT( "VREd.UseMouseAsHandInForcedVRMode" ), 1, TEXT( "When in forced VR mode, enabling this setting uses the mouse cursor as a virtual hand instead of motion controllers" ) );

	static FAutoConsoleVariable HelpLabelFadeDuration( TEXT( "VREd.HelpLabelFadeDuration" ), 0.4f, TEXT( "Duration to fade controller help labels in and out" ) );
	static FAutoConsoleVariable HelpLabelFadeDistance( TEXT( "VREd.HelpLabelFadeDistance" ), 30.0f, TEXT( "Distance at which controller help labels should appear (in cm)" ) );

	static FAutoConsoleVariable GridMovementTolerance( TEXT( "VREd.GridMovementTolerance" ), 0.1f, TEXT( "Tolerance for movement when the grid must disappear" ) );
	static FAutoConsoleVariable GridScaleMultiplier( TEXT( "VREd.GridScaleMultiplier" ), 35.0f, TEXT( "Scale of the grid" ) );
	static FAutoConsoleVariable GridFadeMultiplier( TEXT( "VREd.GridFadeMultiplier" ), 3.0f, TEXT( "Grid fade in/out speed, in 'fades per second'" ) );
	static FAutoConsoleVariable GridFadeStartVelocity( TEXT( "VREd.GridFadeStartVelocity" ), 10.f, TEXT( "Grid fade duration" ) );
	static FAutoConsoleVariable GridMaxOpacity( TEXT( "VREd.GridMaxFade" ), 0.8f, TEXT( "Grid maximum opacity" ) );
	static FAutoConsoleVariable GridHeightOffset( TEXT( "VREd.GridHeightOffset" ), 0.0f, TEXT( "Height offset for the world movement grid.  Useful when tracking space is not properly calibrated" ) );

	static FAutoConsoleVariable TriggerFullyPressedThreshold( TEXT( "VREd.TriggerFullyPressedThreshold" ), 0.95f, TEXT( "Minimum trigger threshold before we consider the trigger 'fully pressed'" ) );
	static FAutoConsoleVariable TriggerFullyPressedReleaseThreshold( TEXT( "VREd.TriggerFullyPressedReleaseThreshold" ), 0.8f, TEXT( "After fully pressing the trigger, if the axis falls below this threshold we no longer consider it fully pressed" ) );
	static FAutoConsoleVariable TriggerLightlyPressedThreshold( TEXT( "VREd.TriggerLightlyPressedThreshold" ), 0.03f, TEXT( "Minimum trigger threshold before we consider the trigger at least 'lightly pressed'" ) );
	static FAutoConsoleVariable TriggerLightlyPressedLockTime( TEXT( "VREd.TriggerLightlyPressedLockTime" ), 0.15f, TEXT( "If the trigger remains lightly pressed for longer than this, we'll continue to treat it as a light press in some cases" ) );
	static FAutoConsoleVariable TriggerDeadZone( TEXT( "VREd.TriggerDeadZone" ), 0.01f, TEXT( "Trigger dead zone.  The trigger must be fully released before we'll trigger a new 'light press'" ) );

	static FAutoConsoleVariable WorldMovementFogOpacity( TEXT( "VREd.WorldMovementFogOpacity" ), 0.8f, TEXT( "How opaque the fog should be at the 'end distance' (0.0 - 1.0)" ) );
	static FAutoConsoleVariable WorldMovementFogStartDistance( TEXT( "VREd.WorldMovementFogStartDistance" ), 300.0f, TEXT( "How far away fog will start rendering while in world movement mode" ) );
	static FAutoConsoleVariable WorldMovementFogEndDistance( TEXT( "VREd.WorldMovementFogEndDistance" ), 800.0f, TEXT( "How far away fog will finish rendering while in world movement mode" ) );
	static FAutoConsoleVariable WorldMovementFogSkyboxDistance( TEXT( "VREd.WorldMovementFogSkyboxDistance" ), 20000.0f, TEXT( "Anything further than this distance will be completed fogged and not visible" ) );

	static FAutoConsoleVariable ScaleProgressBarLength( TEXT( "VREd.ScaleProgressBarLength" ), 50.0f, TEXT( "Length of the progressbar that appears when scaling" ) );
	static FAutoConsoleVariable ScaleProgressBarRadius( TEXT( "VREd.ScaleProgressBarRadius" ), 1.0f, TEXT( "Radius of the progressbar that appears when scaling" ) );

	static FAutoConsoleVariable MinHapticTimeForRift( TEXT( "VREd.MinHapticTimeForRift" ), 0.005f, TEXT( "How long to play haptic effects on the Rift" ) );
	static FAutoConsoleVariable SleepForRiftHaptics( TEXT( "VREd.SleepForRiftHaptics" ), 1, TEXT( "When enabled, we'll sleep the game thread mid-frame to wait for haptic effects to finish.  This can be devasting to performance!" ) );

	static FAutoConsoleVariable InvertTrackpadVertical( TEXT( "VREd.InvertTrackpadVertical" ), 1, TEXT( "Toggles inverting the touch pad vertical axis" ) );
	static FAutoConsoleVariable ForceOculusMirrorMode( TEXT( "VREd.ForceOculusMirrorMode" ), 3, TEXT( "Which Oculus display mirroring mode to use (see MirrorWindowModeType in OculusRiftHMD.h)" ) );

	static FAutoConsoleVariable ShowMovementGrid( TEXT( "VREd.ShowMovementGrid" ), 1, TEXT( "Showing the ground movement grid" ) );
	static FAutoConsoleVariable ShowWorldMovementPostProcess( TEXT( "VREd.ShowWorldMovementPostProcess" ), 1, TEXT( "Showing the movement post processing" ) );

	static FAutoConsoleVariable ScaleMax( TEXT( "VREd.ScaleMax" ), 6000.0f, TEXT( "Maximum world scale in centimeters" ) );
	static FAutoConsoleVariable ScaleMin( TEXT( "VREd.ScaleMin" ), 10.0f, TEXT( "Minimum world scale in centimeters" ) );
}

FEditorModeID FVREditorMode::VREditorModeID( "VREditor" );
bool FVREditorMode::bActuallyUsingVR = true;

// @todo vreditor: Hacky that we have to import these this way. (Plugin has them in a .cpp, not exported)
namespace SteamVRControllerKeyNames
{
	static const FGamepadKeyNames::Type Touch0( "Steam_Touch_0" );
	static const FGamepadKeyNames::Type Touch1( "Steam_Touch_1" );
}


namespace VREditorKeyNames
{
	// @todo vreditor input: Ideally these would not be needed, but SteamVR fires off it's "trigger pressed" event
	// well before the trigger is fully down (*click*)
	static const FName MotionController_Left_FullyPressedTriggerAxis( "MotionController_Left_FullyPressedTriggerAxis" );
	static const FName MotionController_Right_FullyPressedTriggerAxis( "MotionController_Right_FullyPressedTriggerAxis" );

	static const FName MotionController_Left_LightlyPressedTriggerAxis( "MotionController_Left_LightlyPressedTriggerAxis" );
	static const FName MotionController_Right_LightlyPressedTriggerAxis( "MotionController_Right_LightlyPressedTriggerAxis" );
}


FVREditorMode::FVREditorMode()
	: bWantsToExitMode( false ),
	  bIsFullyInitialized( false ),
	  AppTimeModeEntered( FTimespan::Zero() ),
	  AvatarMeshActor( nullptr ),
	  HeadMeshComponent( nullptr ),
	  WorldMovementGridMeshComponent( nullptr ),
	  WorldMovementGridMID( nullptr ),
	  WorldMovementGridOpacity( 0.0f ),	// NOTE: Intentionally not quite zero so that we update the MIDs on the first frame
	  bIsDrawingWorldMovementPostProcess( false ),
	  WorldMovementPostProcessMaterial( nullptr ),
	  SnapGridActor( nullptr ),
	  SnapGridMeshComponent( nullptr ),
	  SnapGridMID( nullptr ),
	  ScaleProgressMeshComponent( nullptr ),
	  CurrentScaleProgressMeshComponent( nullptr ),
	  UserScaleIndicatorText( nullptr ),
	  PostProcessComponent( nullptr ),
	  MotionControllerID( 0 ),	// @todo vreditor minor: We only support a single controller, and we assume the first controller are the motion controls
	  LastFrameNumberInputWasPolled( 0 ),
	  UISystem(),
	  WorldInteraction(),
	  CurrentGizmoType( EGizmoHandleTypes::All ),
	  bFirstTick( true )
{
	FEditorDelegates::MapChange.AddRaw( this, &FVREditorMode::OnMapChange );
	FEditorDelegates::BeginPIE.AddRaw( this, &FVREditorMode::OnBeginPIE );
	FEditorDelegates::EndPIE.AddRaw( this, &FVREditorMode::OnEndPIE );
	FEditorDelegates::OnSwitchBeginPIEAndSIE.AddRaw( this, &FVREditorMode::OnSwitchBetweenPIEAndSIE );
}


FVREditorMode::~FVREditorMode()
{
	FEditorDelegates::MapChange.RemoveAll( this );
	FEditorDelegates::BeginPIE.RemoveAll( this );
	FEditorDelegates::EndPIE.RemoveAll( this );
	FEditorDelegates::OnSwitchBeginPIEAndSIE.RemoveAll( this );
}


void FVREditorMode::Enter()
{
	InputProcessor = MakeShareable(new FVREditorInputProcessor(this));
	FSlateApplication::Get().SetInputPreProcessor(true, InputProcessor);

	// @todo vreditor urgent: Turn on global editor hacks for VR Editor mode
	GEnableVREditorHacks = true;

	bIsFullyInitialized = false;
	bWantsToExitMode = false;

	AppTimeModeEntered = FTimespan::FromSeconds( FApp::GetCurrentTime() );

	WorldMovementGridOpacity = 0.0f;
	bIsDrawingWorldMovementPostProcess = false;
	LastFrameNumberInputWasPolled = 0;

	// Take note of VREditor activation
	if( FEngineAnalytics::IsAvailable() )
	{
		FEngineAnalytics::GetProvider().RecordEvent( TEXT( "Editor.Usage.EnterVREditorMode" ) );
	}

	// Fully reset all hand state.  We don't want anything carrying over from the previous session.
	for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
	{
		FVirtualHand& Hand = VirtualHands[ HandIndex ];
		Hand = FVirtualHand();
	}

	// Call parent implementation
	FEdMode::Enter();

	{
		KeyToActionMap.Reset();

		if( !bActuallyUsingVR && VREd::UseMouseAsHandInForcedVRMode->GetInt() != 0 )
		{
			// For mouse control, when in forced VR mode
			KeyToActionMap.Add( EKeys::LeftMouseButton, FVRAction( EVRActionType::SelectAndMove, VREditorConstants::LeftHandIndex ) );
			KeyToActionMap.Add( EKeys::MiddleMouseButton, FVRAction( EVRActionType::SelectAndMove_LightlyPressed, VREditorConstants::LeftHandIndex ) );
			//			KeyToActionMap.Add( EKeys::RightMouseButton, FVRAction( EVRActionType::WorldMovement, VREditorConstants::LeftHandIndex ) );
//			KeyToActionMap.Add( EKeys::MiddleMouseButton, FVRAction( EVRActionType::Selection, VREditorConstants::LeftHandIndex ) );
			KeyToActionMap.Add( EKeys::LeftControl, FVRAction( EVRActionType::Modifier, VREditorConstants::LeftHandIndex ) );
		}

		// Motion controllers
		KeyToActionMap.Add( EKeys::MotionController_Left_Grip1, FVRAction( EVRActionType::WorldMovement, VREditorConstants::LeftHandIndex ) );
		KeyToActionMap.Add( EKeys::MotionController_Right_Grip1, FVRAction( EVRActionType::WorldMovement, VREditorConstants::RightHandIndex ) );
		if( GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR )
		{
			KeyToActionMap.Add( EKeys::MotionController_Left_Shoulder /* Red face button */, FVRAction( EVRActionType::Modifier, VREditorConstants::LeftHandIndex ) );
			KeyToActionMap.Add( EKeys::MotionController_Right_Shoulder /* Red face button */, FVRAction( EVRActionType::Modifier, VREditorConstants::RightHandIndex ) );
		}
		else
		{
			KeyToActionMap.Add( EKeys::MotionController_Left_FaceButton1, FVRAction( EVRActionType::Modifier, VREditorConstants::LeftHandIndex ) );
			KeyToActionMap.Add( EKeys::MotionController_Right_FaceButton1, FVRAction( EVRActionType::Modifier, VREditorConstants::RightHandIndex ) );
		}

// 		KeyToActionMap.Add( EKeys::MotionController_Left_Trigger, FVRAction( EVRActionType::SelectAndMove, VREditorConstants::LeftHandIndex ) );
// 		KeyToActionMap.Add( EKeys::MotionController_Right_Trigger, FVRAction( EVRActionType::SelectAndMove, VREditorConstants::RightHandIndex ) );
		KeyToActionMap.Add( VREditorKeyNames::MotionController_Left_FullyPressedTriggerAxis, FVRAction( EVRActionType::SelectAndMove, VREditorConstants::LeftHandIndex ) );
		KeyToActionMap.Add( VREditorKeyNames::MotionController_Right_FullyPressedTriggerAxis, FVRAction( EVRActionType::SelectAndMove, VREditorConstants::RightHandIndex ) );
		KeyToActionMap.Add( VREditorKeyNames::MotionController_Left_LightlyPressedTriggerAxis, FVRAction( EVRActionType::SelectAndMove_LightlyPressed, VREditorConstants::LeftHandIndex ) );
		KeyToActionMap.Add( VREditorKeyNames::MotionController_Right_LightlyPressedTriggerAxis, FVRAction( EVRActionType::SelectAndMove_LightlyPressed, VREditorConstants::RightHandIndex ) );

		KeyToActionMap.Add( EKeys::MotionController_Left_Thumbstick, FVRAction( EVRActionType::ConfirmRadialSelection, VREditorConstants::LeftHandIndex ) );
 		KeyToActionMap.Add( EKeys::MotionController_Right_Thumbstick, FVRAction( EVRActionType::ConfirmRadialSelection, VREditorConstants::RightHandIndex ) );

// 		KeyToActionMap.Add( EKeys::MotionController_Left_FaceButton2 /* Touchpad right */, FVRAction( EVRActionType::Redo, VREditorConstants::LeftHandIndex ) );
// 		KeyToActionMap.Add( EKeys::MotionController_Right_FaceButton2 /* Touchpad right */, FVRAction( EVRActionType::Redo, VREditorConstants::RightHandIndex ) );
// 		KeyToActionMap.Add( EKeys::MotionController_Left_FaceButton3 /* Touchpad down */, FVRAction( EVRActionType::Delete, VREditorConstants::LeftHandIndex ) );
// 		KeyToActionMap.Add( EKeys::MotionController_Right_FaceButton3 /* Touchpad down */, FVRAction( EVRActionType::Delete, VREditorConstants::RightHandIndex ) );
// 		KeyToActionMap.Add( EKeys::MotionController_Left_FaceButton4 /* Touchpad left */, FVRAction( EVRActionType::Undo, VREditorConstants::LeftHandIndex ) );
// 		KeyToActionMap.Add( EKeys::MotionController_Right_FaceButton4 /* Touchpad left */, FVRAction( EVRActionType::Undo, VREditorConstants::RightHandIndex ) );
		KeyToActionMap.Add( SteamVRControllerKeyNames::Touch0, FVRAction( EVRActionType::Touch, VREditorConstants::LeftHandIndex ) );
 		KeyToActionMap.Add( SteamVRControllerKeyNames::Touch1, FVRAction( EVRActionType::Touch, VREditorConstants::RightHandIndex ) );
		KeyToActionMap.Add( EKeys::MotionController_Left_TriggerAxis, FVRAction( EVRActionType::TriggerAxis, VREditorConstants::LeftHandIndex ) );
		KeyToActionMap.Add( EKeys::MotionController_Right_TriggerAxis, FVRAction( EVRActionType::TriggerAxis, VREditorConstants::RightHandIndex ) );
		KeyToActionMap.Add( EKeys::MotionController_Left_Thumbstick_X, FVRAction( EVRActionType::TrackpadPositionX, VREditorConstants::LeftHandIndex ) );
		KeyToActionMap.Add( EKeys::MotionController_Right_Thumbstick_X, FVRAction( EVRActionType::TrackpadPositionX, VREditorConstants::RightHandIndex ) );
		KeyToActionMap.Add( EKeys::MotionController_Left_Thumbstick_Y, FVRAction( EVRActionType::TrackpadPositionY, VREditorConstants::LeftHandIndex ) );
		KeyToActionMap.Add( EKeys::MotionController_Right_Thumbstick_Y, FVRAction( EVRActionType::TrackpadPositionY, VREditorConstants::RightHandIndex ) );
	}

	// Setting up colors
	Colors.SetNumZeroed( (int32)EColors::TotalCount );
	{
		Colors[ (int32)EColors::DefaultColor ] = FLinearColor::Red;	
		Colors[ (int32)EColors::SelectionColor ] = FLinearColor::Green;
		Colors[ (int32)EColors::TeleportColor ] = FLinearColor( 1.0f, 0.0f, 0.75f, 1.0f );
		Colors[ (int32)EColors::WorldDraggingColor_OneHand ] = FLinearColor::Yellow;
		Colors[ (int32)EColors::WorldDraggingColor_TwoHands ] = FLinearColor( 0.05f, 0.05f, 0.4f, 1.0f );
		Colors[ (int32)EColors::RedGizmoColor ] = FLinearColor( 0.4f, 0.05f, 0.05f, 1.0f );
		Colors[ (int32)EColors::GreenGizmoColor ] = FLinearColor( 0.05f, 0.4f, 0.05f, 1.0f );
		Colors[ (int32)EColors::BlueGizmoColor ] = FLinearColor( 0.05f, 0.05f, 0.4f, 1.0f );
		Colors[ (int32)EColors::WhiteGizmoColor ] = FLinearColor( 0.7f, 0.7f, 0.7f, 1.0f );
		Colors[ (int32)EColors::HoverGizmoColor ] = FLinearColor( FLinearColor::Yellow );
		Colors[ (int32)EColors::DraggingGizmoColor ] = FLinearColor( FLinearColor::White );
		Colors[ (int32)EColors::UISelectionBarColor ] = FLinearColor( 0.025f, 0.025f, 0.025f, 1.0f );
		Colors[ (int32)EColors::UISelectionBarHoverColor ] = FLinearColor( 0.1f, 0.1f, 0.1f, 1.0f );
		Colors[ (int32)EColors::UICloseButtonColor ] = FLinearColor( 0.1f, 0.1f, 0.1f, 1.0f );
		Colors[ (int32)EColors::UICloseButtonHoverColor ] = FLinearColor( 1.0f, 1.0f, 1.0f, 1.0f );
	}

	// @todo vreditor: We need to make sure the user can never switch to orthographic mode, or activate settings that
	// would disrupt the user's ability to view the VR scene.
		
	// @todo vreditor: Don't bother drawing toolbars in VR, or other things that won't matter in VR

	{
		const TSharedRef< ILevelEditor >& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" ).GetFirstLevelEditor().ToSharedRef();

		bool bSummonNewWindow = true;

		// Do we have an active perspective viewport that is valid for VR?  If so, go ahead and use that.
		TSharedPtr<SLevelViewport> ExistingActiveLevelViewport;
		{
			TSharedPtr<ILevelViewport> ActiveLevelViewport = LevelEditor->GetActiveViewportInterface();
			if( ActiveLevelViewport.IsValid() )
			{
				ExistingActiveLevelViewport = StaticCastSharedRef< SLevelViewport >( ActiveLevelViewport->AsWidget() );

				// Use the currently active window instead
				bSummonNewWindow = false;
			}
		}

		TSharedPtr< SLevelViewport > VREditorLevelViewport;
		if( bSummonNewWindow )
		{
			// @todo vreditor: The resolution we set here doesn't matter, as HMDs will draw at their native resolution
			// no matter what.  We should probably allow the window to be freely resizable by the user
			// @todo vreditor: Should save and restore window position and size settings
			FVector2D WindowSize;
			{
				IHeadMountedDisplay::MonitorInfo HMDMonitorInfo;
				if( bActuallyUsingVR && GEngine->HMDDevice->GetHMDMonitorInfo( HMDMonitorInfo ) )
				{
					WindowSize = FVector2D( HMDMonitorInfo.ResolutionX, HMDMonitorInfo.ResolutionY );
				}
				else
				{
					// @todo vreditor: Hard-coded failsafe window size
					WindowSize = FVector2D( 1920.0f, 1080.0f );
				}
			}

			// @todo vreditor: Use SLevelEditor::GetTableTitle() for the VR window title (needs dynamic update)
			const FText VREditorWindowTitle = NSLOCTEXT( "VREditor", "VRWindowTitle", "Unreal Editor VR" );

			TSharedRef< SWindow > VREditorWindow = SNew( SWindow )
				.Title( VREditorWindowTitle )
				.ClientSize( WindowSize )
				.AutoCenter( EAutoCenter::PreferredWorkArea )
				.UseOSWindowBorder( true )	// @todo vreditor: Allow window to be freely resized?  Shouldn't really hurt anything.  We should save position/size too.
				.SizingRule( ESizingRule::UserSized );
			this->VREditorWindowWeakPtr = VREditorWindow;

			VREditorLevelViewport =
				SNew( SLevelViewport )
				.ViewportType( LVT_Perspective ) // Perspective
				.Realtime( true )
				//				.ParentLayout( AsShared() )	// @todo vreditor: We don't have one and we probably don't need one, right?  Make sure a null parent layout is handled properly everywhere.
				.ParentLevelEditor( LevelEditor )
				//				.ConfigKey( BottomLeftKey )	// @todo vreditor: This is for saving/loading layout.  We would need this in order to remember viewport settings like show flags, etc.
				.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() );

			// Allow the editor to keep track of this editor viewport.  Because it's not inside of a normal tab, 
			// we need to explicitly tell the level editor about it
			LevelEditor->AddStandaloneLevelViewport( VREditorLevelViewport.ToSharedRef() );

			VREditorWindow->SetContent( VREditorLevelViewport.ToSharedRef() );

			// NOTE: We're intentionally not adding this window natively parented to the main frame window, because we don't want it
			// to minimize/restore when the main frame is minimized/restored
			FSlateApplication::Get().AddWindow( VREditorWindow );

			VREditorWindow->SetOnWindowClosed( FOnWindowClosed::CreateRaw( this, &FVREditorMode::OnVREditorWindowClosed ) );

			VREditorWindow->BringToFront();	// @todo vreditor: Not sure if this is needed, especially if we decide the window should be hidden (copied this from PIE code)
		}
		else
		{
			VREditorLevelViewport = ExistingActiveLevelViewport;

			if( bActuallyUsingVR )
			{
				// Switch to immersive mode
				const bool bWantImmersive = true;
				const bool bAllowAnimation = false;
				ExistingActiveLevelViewport->MakeImmersive( bWantImmersive, bAllowAnimation );
			}
		}
		this->VREditorLevelViewportWeakPtr = VREditorLevelViewport;

		{
			FLevelEditorViewportClient& VRViewportClient = VREditorLevelViewport->GetLevelViewportClient();
			FEditorViewportClient& VREditorViewportClient = VRViewportClient;


			// Make sure we are in perspective mode
			// @todo vreditor: We should never allow ortho switching while in VR
			SavedEditorState.ViewportType = VREditorViewportClient.GetViewportType();
			VREditorViewportClient.SetViewportType( LVT_Perspective );

			// Set the initial camera location
			// @todo vreditor: This should instead be calculated using the currently active perspective camera's
			// location and orientation, compensating for the current HMD offset from the tracking space origin.
			// Perhaps, we also want to teleport the original viewport's camera back when we exit this mode, too!
			// @todo vreditor: Should save and restore camera position and any other settings we change (viewport type, pitch locking, etc.)
			SavedEditorState.ViewLocation = VRViewportClient.GetViewLocation();
			SavedEditorState.ViewRotation = VRViewportClient.GetViewRotation();

			// Don't allow the tracking space to pitch up or down.  People hate that in VR.
			// @todo vreditor: This doesn't seem to prevent people from pitching the camera with RMB drag
			SavedEditorState.bLockedPitch = VRViewportClient.GetCameraController()->GetConfig().bLockedPitch;
			if( bActuallyUsingVR )
			{
				VRViewportClient.GetCameraController()->AccessConfig().bLockedPitch = true;
			}

			// Set "game mode" to be enabled, to get better performance.  Also hit proxies won't work in VR, anyway
			SavedEditorState.bGameView = VREditorViewportClient.IsInGameView();
			VREditorViewportClient.SetGameView( true );

			SavedEditorState.bRealTime = VREditorViewportClient.IsRealtime();
			VREditorViewportClient.SetRealtime( true );

			SavedEditorState.ShowFlags = VREditorViewportClient.EngineShowFlags;

			// Never show the traditional Unreal transform widget.  It doesn't work in VR because we don't have hit proxies.
			VREditorViewportClient.EngineShowFlags.SetModeWidgets( false );

			// Make sure the mode widgets don't come back when users click on things
			VRViewportClient.bAlwaysShowModeWidgetAfterSelectionChanges = false;

			// Force tiny near clip plane distance, because user can scale themselves to be very small.
			// @todo vreditor: Make this automatically change based on WorldToMetersScale?
			SavedEditorState.NearClipPlane = GNearClippingPlane;
			GNearClippingPlane = 1.0f;	// Normally defaults to 10cm (NOTE: If we go too small, skyboxes become affected)

			SavedEditorState.bOnScreenMessages = GAreScreenMessagesEnabled;
			GAreScreenMessagesEnabled = false;

			if( bActuallyUsingVR )
			{
				SavedEditorState.TrackingOrigin = GEngine->HMDDevice->GetTrackingOrigin();
				GEngine->HMDDevice->SetTrackingOrigin( EHMDTrackingOrigin::Floor );
			}

			// Make the new viewport the active level editing viewport right away
			GCurrentLevelEditingViewportClient = &VRViewportClient;
		}

		VREditorLevelViewport->EnableStereoRendering( bActuallyUsingVR );
		VREditorLevelViewport->SetRenderDirectlyToWindow( bActuallyUsingVR );

		if( bActuallyUsingVR )
		{
			GEngine->HMDDevice->EnableStereo( true );

			// @todo vreditor: Force single eye, undistorted mirror for demos
			const bool bIsVREditorDemo = FParse::Param( FCommandLine::Get(), TEXT( "VREditorDemo" ) );	// @todo vreditor: Remove this when no longer needed (console variable, too!)
			if( bIsVREditorDemo && GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )
			{
				// If we're using an Oculus Rift, go ahead and set the mirror mode to a single undistorted eye
				GEngine->DeferredCommands.Add( FString::Printf( TEXT( "HMD MIRROR MODE %i" ), VREd::ForceOculusMirrorMode->GetInt() ) );
			}
		}

		if( bActuallyUsingVR )
		{
			// Tell Slate to require a larger pixel distance threshold before the drag starts.  This is important for things 
			// like Content Browser drag and drop.
			SavedEditorState.DragTriggerDistance = FSlateApplication::Get().GetDragTriggerDistance();
			FSlateApplication::Get().SetDragTriggerDistance( 100.0f );	// @todo vreditor tweak
		}
	}

	// Load our post process material for the world movement grid
	{
		UMaterial* Material = LoadObject<UMaterial>( nullptr, TEXT( "/Engine/VREditor/WorldMovementGrid/GridPostProcess" ) );
		check( Material != nullptr );
		WorldMovementPostProcessMaterial = UMaterialInstanceDynamic::Create( Material, GetTransientPackage() );
		check( WorldMovementPostProcessMaterial != nullptr );
	}


	UISystem.Reset( new FVREditorUISystem( *this ) );

	WorldInteraction.Reset( new FVREditorWorldInteraction( *this ) );

	//// If we're not actually in VR mode, and instead we've forced it, we should spawn some UI at the Origin to help test
	//if ( !bActuallyUsingVR )
	//{
	//	const int32 HandIndex = 1;
	//	const bool bShouldShow = true;

	//	FVirtualHand& Hand = VirtualHands[HandIndex];
	//	Hand.Transform = FTransform(FVector(0, 0, 0));

	//	UISystem->ShowEditorUIPanel(FVREditorUISystem::EEditorUIPanel::ContentBrowser, HandIndex, bShouldShow);
	//}

	if ( bActuallyUsingVR )
	{
		 SetWorldToMetersScale( 100.0f );
	}

	bFirstTick = true;
	bIsFullyInitialized = true;
}


void FVREditorMode::Exit()
{
	bIsFullyInitialized = false;

	FSlateApplication::Get().SetInputPreProcessor(false);

	// Kill subsystems
	{
		WorldInteraction.Reset();
		UISystem.Reset();
	}

	if( WorldMovementPostProcessMaterial != nullptr )
	{
		WorldMovementPostProcessMaterial->MarkPendingKill();
		WorldMovementPostProcessMaterial = nullptr;
	}

	{
		DestroyTransientActor( AvatarMeshActor );
		AvatarMeshActor = nullptr;
		HeadMeshComponent = nullptr;
		WorldMovementGridMeshComponent = nullptr;
		WorldMovementGridMID = nullptr;
		DestroyTransientActor( SnapGridActor );
		SnapGridActor = nullptr;
		SnapGridMeshComponent = nullptr;
		SnapGridMID = nullptr;
		PostProcessComponent = nullptr;
		ScaleProgressMeshComponent = nullptr;
		CurrentScaleProgressMeshComponent = nullptr;
		UserScaleIndicatorText = nullptr;

		for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
		{
			FVirtualHand& Hand = VirtualHands[ HandIndex ];

			Hand.MotionControllerComponent = nullptr;
			Hand.HandMeshComponent = nullptr;
			Hand.HoverMeshComponent = nullptr;
			Hand.HoverPointLightComponent = nullptr;
			Hand.LaserPointerMeshComponent = nullptr;
			Hand.LaserPointerMID = nullptr;
			Hand.TranslucentLaserPointerMID = nullptr;

			for( auto& KeyAndValue : Hand.HelpLabels )
			{
				AFloatingText* FloatingText = KeyAndValue.Value;
				DestroyTransientActor( FloatingText );
			}
			Hand.HelpLabels.Reset();
		}
	}

	{
		if( GEngine->HMDDevice.IsValid() )
		{
			// @todo vreditor switch: We don't want to do this if a VR PIE session is somehow active.  Is that even possible while immersive?
			GEngine->HMDDevice->EnableStereo( false );
		}

		if( bActuallyUsingVR )
		{ 
			// Restore Slate drag trigger distance
			FSlateApplication::Get().SetDragTriggerDistance( SavedEditorState.DragTriggerDistance );
		}

		TSharedPtr<SLevelViewport> VREditorLevelViewport( VREditorLevelViewportWeakPtr.Pin() );
		if( VREditorLevelViewport.IsValid() )
		{
			VREditorLevelViewport->EnableStereoRendering( false );
			VREditorLevelViewport->SetRenderDirectlyToWindow( false );

			{
				FLevelEditorViewportClient& VRViewportClient = VREditorLevelViewport->GetLevelViewportClient();
				FEditorViewportClient& VREditorViewportClient = VRViewportClient;

				// Restore settings that we changed on the viewport
				VREditorViewportClient.SetViewportType( SavedEditorState.ViewportType );
				VRViewportClient.GetCameraController()->AccessConfig().bLockedPitch = SavedEditorState.bLockedPitch;
				VRViewportClient.bAlwaysShowModeWidgetAfterSelectionChanges = SavedEditorState.bAlwaysShowModeWidgetAfterSelectionChanges;
				VRViewportClient.EngineShowFlags = SavedEditorState.ShowFlags;
				VRViewportClient.SetGameView( SavedEditorState.bGameView );
				VRViewportClient.SetViewLocation( GetHeadTransform().GetLocation() ); // Use SavedEditorState.ViewLocation to go back to start location when entering VR Editor Mode
				
				FRotator HeadRotationNoRoll = GetHeadTransform().GetRotation().Rotator();
				HeadRotationNoRoll.Roll = 0.0f;
				VRViewportClient.SetViewRotation(HeadRotationNoRoll); // Use SavedEditorState.ViewRotation to go back to start rot
				
				VRViewportClient.SetRealtime( SavedEditorState.bRealTime );

				GNearClippingPlane = SavedEditorState.NearClipPlane;
				GAreScreenMessagesEnabled = SavedEditorState.bOnScreenMessages;

				if( bActuallyUsingVR )
				{
					GEngine->HMDDevice->SetTrackingOrigin( SavedEditorState.TrackingOrigin );
				}
			}

			if( bActuallyUsingVR )
			{
				// Leave immersive mode
				const bool bWantImmersive = false;
				const bool bAllowAnimation = false;
				VREditorLevelViewport->MakeImmersive( bWantImmersive, bAllowAnimation );
			}

			VREditorLevelViewportWeakPtr.Reset();
		}

		// Kill the VR editor window
		TSharedPtr<SWindow> VREditorWindow( VREditorWindowWeakPtr.Pin() );
		if( VREditorWindow.IsValid() )
		{
			VREditorWindow->RequestDestroyWindow();
			VREditorWindow.Reset();
		}
	}

	// Make sure we are no longer overriding WorldToMetersScale every frame
	SetWorldToMetersScale( 0.0f );

	// Call parent implementation
	FEdMode::Exit();

	// @todo vreditor urgent: Disable global editor hacks for VR Editor mode
	GEnableVREditorHacks = false;
}


void FVREditorMode::SpawnAvatarMeshActor()
{
	// Setup our avatar
	if( AvatarMeshActor == nullptr )
	{
		{
			const bool bWithSceneComponent = true;
			AvatarMeshActor = SpawnTransientSceneActor( AActor::StaticClass(), TEXT( "AvatarMesh" ), bWithSceneComponent );
		}

		// Give us a head mesh
		{
			HeadMeshComponent = NewObject<UStaticMeshComponent>( AvatarMeshActor );
			AvatarMeshActor->AddOwnedComponent( HeadMeshComponent );
			HeadMeshComponent->SetupAttachment( AvatarMeshActor->GetRootComponent() );
			HeadMeshComponent->RegisterComponent();

			// @todo vreditor: This needs to adapt based on the device you're using
			UStaticMesh* HeadMesh = LoadObject<UStaticMesh>( nullptr, TEXT( "/Engine/VREditor/Devices/Generic/GenericHMD" ) );
			check( HeadMesh != nullptr );

			HeadMeshComponent->SetStaticMesh( HeadMesh );
			HeadMeshComponent->SetMobility( EComponentMobility::Movable );
			HeadMeshComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );
		}

		for( int32 HandIndex = 0; HandIndex < ARRAY_COUNT( VirtualHands ); ++HandIndex )
		{
			FVirtualHand& Hand = VirtualHands[ HandIndex ];

			// Setup a motion controller component.  This allows us to take advantage of late frame updates, so
			// our motion controllers won't lag behind the HMD
			{
				Hand.MotionControllerComponent = NewObject<UMotionControllerComponent>( AvatarMeshActor );
				AvatarMeshActor->AddOwnedComponent( Hand.MotionControllerComponent );
				Hand.MotionControllerComponent->SetupAttachment( AvatarMeshActor->GetRootComponent() );
				Hand.MotionControllerComponent->RegisterComponent();

				Hand.MotionControllerComponent->SetMobility( EComponentMobility::Movable );
				Hand.MotionControllerComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );

				Hand.MotionControllerComponent->Hand = ( HandIndex == VREditorConstants::LeftHandIndex ) ? EControllerHand::Left : EControllerHand::Right;

				Hand.MotionControllerComponent->bDisableLowLatencyUpdate = false;
			}

			// Hand mesh
			{
				Hand.HandMeshComponent = NewObject<UStaticMeshComponent>( AvatarMeshActor );
				AvatarMeshActor->AddOwnedComponent( Hand.HandMeshComponent );
				Hand.HandMeshComponent->SetupAttachment( Hand.MotionControllerComponent );
				Hand.HandMeshComponent->RegisterComponent();

				// @todo vreditor extensibility: We need this to be able to be overridden externally, or simply based on the HMD name (but allowing external folders)
				FString MeshName;
				FString MaterialName;
				if( GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR )
				{
					MeshName = TEXT( "/Engine/VREditor/Devices/Vive/VivePreControllerMesh" );
					MaterialName = TEXT( "/Engine/VREditor/Devices/Vive/VivePreControllerMaterial_Inst" );
				}
				else
				{
					MeshName = TEXT( "/Engine/VREditor/Devices/Oculus/OculusControllerMesh" );
					MaterialName = TEXT( "/Engine/VREditor/Devices/Oculus/OculusControllerMaterial_Inst" );
				}

				UStaticMesh* HandMesh = LoadObject<UStaticMesh>( nullptr, *MeshName );
				check( HandMesh != nullptr );

				Hand.HandMeshComponent->SetStaticMesh( HandMesh );
				Hand.HandMeshComponent->SetMobility( EComponentMobility::Movable );
				Hand.HandMeshComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );

				UMaterialInstance* HandMeshMaterialInst = LoadObject<UMaterialInstance>( nullptr, *MaterialName );
				check( HandMeshMaterialInst != nullptr );
				Hand.HandMeshMID = UMaterialInstanceDynamic::Create( HandMeshMaterialInst, GetTransientPackage() );
				check( Hand.HandMeshMID != nullptr );
				Hand.HandMeshComponent->SetMaterial( 0, Hand.HandMeshMID );
			}

			// Laser pointer
			{
				Hand.LaserPointerMeshComponent = NewObject<UStaticMeshComponent>( AvatarMeshActor );
				AvatarMeshActor->AddOwnedComponent( Hand.LaserPointerMeshComponent );
				Hand.LaserPointerMeshComponent->SetupAttachment( Hand.MotionControllerComponent );
				Hand.LaserPointerMeshComponent->RegisterComponent();

				UStaticMesh* LaserPointerMesh = LoadObject<UStaticMesh>( nullptr, TEXT( "/Engine/VREditor/LaserPointer/LaserPointerMesh" ) );
				check( LaserPointerMesh != nullptr );

				Hand.LaserPointerMeshComponent->SetStaticMesh( LaserPointerMesh );
				Hand.LaserPointerMeshComponent->SetMobility( EComponentMobility::Movable );
				Hand.LaserPointerMeshComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );

				UMaterialInstance* LaserPointerMaterialInst = LoadObject<UMaterialInstance>( nullptr, TEXT( "/Engine/VREditor/LaserPointer/LaserPointerMaterialInst" ) );
				check( LaserPointerMaterialInst != nullptr );
				Hand.LaserPointerMID = UMaterialInstanceDynamic::Create( LaserPointerMaterialInst, GetTransientPackage() );
				check( Hand.LaserPointerMID != nullptr );
				Hand.LaserPointerMeshComponent->SetMaterial( 0, Hand.LaserPointerMID );

				UMaterialInstance* TranslucentLaserPointerMaterialInst = LoadObject<UMaterialInstance>( nullptr, TEXT( "/Engine/VREditor/LaserPointer/TranslucentLaserPointerMaterialInst" ) );
				check( TranslucentLaserPointerMaterialInst != nullptr );
				Hand.TranslucentLaserPointerMID = UMaterialInstanceDynamic::Create( TranslucentLaserPointerMaterialInst, GetTransientPackage() );
				check( Hand.TranslucentLaserPointerMID != nullptr );
				Hand.LaserPointerMeshComponent->SetMaterial( 1, Hand.TranslucentLaserPointerMID );
			}

			// Hover cue for laser pointer
			{
				Hand.HoverMeshComponent = NewObject<UStaticMeshComponent>( AvatarMeshActor );
				AvatarMeshActor->AddOwnedComponent( Hand.HoverMeshComponent );
				Hand.HoverMeshComponent->SetupAttachment( AvatarMeshActor->GetRootComponent() );
				Hand.HoverMeshComponent->RegisterComponent();

				UStaticMesh* HoverMesh = LoadObject<UStaticMesh>( nullptr, TEXT( "/Engine/VREditor/LaserPointer/HoverMesh" ) );
				check( HoverMesh != nullptr );
				Hand.HoverMeshComponent->SetStaticMesh( HoverMesh );
				Hand.HoverMeshComponent->SetMobility( EComponentMobility::Movable );
				Hand.HoverMeshComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );

				// Add a light!
				{
					Hand.HoverPointLightComponent = NewObject<UPointLightComponent>( AvatarMeshActor );
					AvatarMeshActor->AddOwnedComponent( Hand.HoverPointLightComponent );
					Hand.HoverPointLightComponent->SetupAttachment( Hand.HoverMeshComponent );
					Hand.HoverPointLightComponent->RegisterComponent();

					Hand.HoverPointLightComponent->SetLightColor( FLinearColor::Red );
//					Hand.HoverPointLightComponent->SetLightColor( FLinearColor( 0.0f, 1.0f, 0.2f, 1.0f ) );
					Hand.HoverPointLightComponent->SetIntensity( 30.0f );	// @todo vreditor tweak
					Hand.HoverPointLightComponent->SetMobility( EComponentMobility::Movable );
					Hand.HoverPointLightComponent->SetAttenuationRadius( VREd::LaserPointerLightRadius->GetFloat() );
					Hand.HoverPointLightComponent->bUseInverseSquaredFalloff = false;
					Hand.HoverPointLightComponent->SetCastShadows( false );
				}
			}
		}

		// World movement grid mesh
		{
			WorldMovementGridMeshComponent = NewObject<UStaticMeshComponent>( AvatarMeshActor );
			AvatarMeshActor->AddOwnedComponent( WorldMovementGridMeshComponent );
			WorldMovementGridMeshComponent->SetupAttachment( AvatarMeshActor->GetRootComponent() );
			WorldMovementGridMeshComponent->RegisterComponent();

			UStaticMesh* GridMesh = LoadObject<UStaticMesh>(nullptr, TEXT( "/Engine/VREditor/WorldMovementGrid/PlaneMesh" ) );
			check( GridMesh != nullptr );
			WorldMovementGridMeshComponent->SetStaticMesh( GridMesh );
			WorldMovementGridMeshComponent->SetMobility( EComponentMobility::Movable );
			WorldMovementGridMeshComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );

			UMaterialInterface* GridMaterial = LoadObject<UMaterialInterface>( nullptr, TEXT( "/Engine/VREditor/WorldMovementGrid/GridMaterial" ) );
			check( GridMaterial != nullptr );
			
			WorldMovementGridMID = UMaterialInstanceDynamic::Create( GridMaterial, GetTransientPackage() );
			check( WorldMovementGridMID != nullptr );
			WorldMovementGridMeshComponent->SetMaterial( 0, WorldMovementGridMID );

			// The grid starts off hidden
			WorldMovementGridMeshComponent->SetVisibility( false );
		}

		// Snap grid mesh
		{
			const bool bWithSceneComponent = false;
			SnapGridActor = SpawnTransientSceneActor<AActor>( TEXT( "SnapGrid" ), bWithSceneComponent );

			SnapGridMeshComponent = NewObject<UStaticMeshComponent>( SnapGridActor );
			SnapGridActor->AddOwnedComponent( SnapGridMeshComponent );
			SnapGridActor->SetRootComponent( SnapGridMeshComponent );
			SnapGridMeshComponent->RegisterComponent();

			UStaticMesh* GridMesh = LoadObject<UStaticMesh>( nullptr, TEXT( "/Engine/VREditor/SnapGrid/SnapGridPlaneMesh" ) );
			check( GridMesh != nullptr );
			SnapGridMeshComponent->SetStaticMesh( GridMesh );
			SnapGridMeshComponent->SetMobility( EComponentMobility::Movable );
			SnapGridMeshComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );

			UMaterialInterface* GridMaterial = LoadObject<UMaterialInterface>( nullptr, TEXT( "/Engine/VREditor/SnapGrid/SnapGridMaterial" ) );
			check( GridMaterial != nullptr );

			SnapGridMID = UMaterialInstanceDynamic::Create( GridMaterial, GetTransientPackage() );
			check( SnapGridMID != nullptr );
			SnapGridMeshComponent->SetMaterial( 0, SnapGridMID );

			// The grid starts off hidden
			SnapGridMeshComponent->SetVisibility( false );
		}

		{
			{
				UMaterialInterface* UserScaleIndicatorMaterial = LoadObject<UMaterialInterface>( nullptr, TEXT( "/Engine/VREditor/LaserPointer/LaserPointerMaterialInst" ) );
				check( UserScaleIndicatorMaterial != nullptr );

				UMaterialInstance* TranslucentUserScaleIndicatorMaterial = LoadObject<UMaterialInstance>( nullptr, TEXT( "/Engine/VREditor/LaserPointer/TranslucentLaserPointerMaterialInst" ) );
				check( TranslucentUserScaleIndicatorMaterial != nullptr );

				UStaticMesh* ScaleLineMesh = LoadObject<UStaticMesh>( nullptr, TEXT( "/Engine/VREditor/LaserPointer/LaserPointerMesh" ) );
				check( ScaleLineMesh != nullptr );

				// Creating the background bar progress of the scale 
				{
					ScaleProgressMeshComponent = NewObject<UStaticMeshComponent>( AvatarMeshActor );
					AvatarMeshActor->AddOwnedComponent( ScaleProgressMeshComponent );
					ScaleProgressMeshComponent->SetupAttachment( AvatarMeshActor->GetRootComponent() );
					ScaleProgressMeshComponent->RegisterComponent();
				
					ScaleProgressMeshComponent->SetStaticMesh( ScaleLineMesh );
					ScaleProgressMeshComponent->SetMobility( EComponentMobility::Movable );
					ScaleProgressMeshComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );

					UMaterialInstanceDynamic* UserScaleMID = UMaterialInstanceDynamic::Create( UserScaleIndicatorMaterial, GetTransientPackage() );
					check( UserScaleMID != nullptr );
					ScaleProgressMeshComponent->SetMaterial( 0, UserScaleMID );

					UMaterialInstanceDynamic* TranslucentUserScaleMID = UMaterialInstanceDynamic::Create( TranslucentUserScaleIndicatorMaterial, GetTransientPackage() );
					check( TranslucentUserScaleMID != nullptr );
					ScaleProgressMeshComponent->SetMaterial( 1, TranslucentUserScaleMID );

					static FName StaticLaserColorName( "LaserColor" );
					const FLinearColor Color = GetColor( EColors::WorldDraggingColor_TwoHands );
					UserScaleMID->SetVectorParameterValue( StaticLaserColorName, Color );
					TranslucentUserScaleMID->SetVectorParameterValue( StaticLaserColorName, Color );

					// The user scale indicator starts invisible
					ScaleProgressMeshComponent->SetVisibility( false );
				}


				// Creating the current progress of the scale 
				{
					CurrentScaleProgressMeshComponent = NewObject<UStaticMeshComponent>( AvatarMeshActor );
					AvatarMeshActor->AddOwnedComponent( CurrentScaleProgressMeshComponent );
					CurrentScaleProgressMeshComponent->SetupAttachment( AvatarMeshActor->GetRootComponent() );
					CurrentScaleProgressMeshComponent->RegisterComponent();

					CurrentScaleProgressMeshComponent->SetStaticMesh( ScaleLineMesh );
					CurrentScaleProgressMeshComponent->SetMobility( EComponentMobility::Movable );
					CurrentScaleProgressMeshComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );

					UMaterialInstanceDynamic* UserScaleMID = UMaterialInstanceDynamic::Create( UserScaleIndicatorMaterial, GetTransientPackage() );
					check( UserScaleMID != nullptr );
					CurrentScaleProgressMeshComponent->SetMaterial( 0, UserScaleMID );

					UMaterialInstanceDynamic* TranslucentUserScaleMID = UMaterialInstanceDynamic::Create( TranslucentUserScaleIndicatorMaterial, GetTransientPackage() );
					check( TranslucentUserScaleMID != nullptr );
					CurrentScaleProgressMeshComponent->SetMaterial( 1, TranslucentUserScaleMID );

					static FName StaticLaserColorName( "LaserColor" );
					const FLinearColor Color = GetColor( EColors::GreenGizmoColor );
					UserScaleMID->SetVectorParameterValue( StaticLaserColorName, Color );
					TranslucentUserScaleMID->SetVectorParameterValue( StaticLaserColorName, Color );

					// The user scale indicator starts invisible
					CurrentScaleProgressMeshComponent->SetVisibility( false );
				}
			}

			// Creating the text for scaling
			{
				UFont* TextFont = LoadObject<UFont>( nullptr, TEXT( "/Engine/VREditor/Fonts/VRText_RobotoLarge" ) );
				check( TextFont != nullptr );

				UMaterialInterface* UserScaleIndicatorMaterial = LoadObject<UMaterialInterface>( nullptr, TEXT( "/Engine/VREditor/Fonts/VRTextMaterial" ) );
				check( UserScaleIndicatorMaterial != nullptr );

				UserScaleIndicatorText = NewObject<UTextRenderComponent>( AvatarMeshActor );
				AvatarMeshActor->AddOwnedComponent( UserScaleIndicatorText );
				UserScaleIndicatorText->SetupAttachment( AvatarMeshActor->GetRootComponent() );
				UserScaleIndicatorText->RegisterComponent();

				UserScaleIndicatorText->SetMobility( EComponentMobility::Movable );
				UserScaleIndicatorText->SetCollisionEnabled( ECollisionEnabled::NoCollision );
				UserScaleIndicatorText->SetCollisionProfileName( UCollisionProfile::NoCollision_ProfileName );

				UserScaleIndicatorText->bGenerateOverlapEvents = false;
				UserScaleIndicatorText->SetCanEverAffectNavigation( false );
				UserScaleIndicatorText->bCastDynamicShadow = false;
				UserScaleIndicatorText->bCastStaticShadow = false;
				UserScaleIndicatorText->bAffectDistanceFieldLighting = false;
				UserScaleIndicatorText->bAffectDynamicIndirectLighting = false;

				// Use a custom font.  The text will be visible up close.
				UserScaleIndicatorText->SetFont( TextFont );
				UserScaleIndicatorText->SetWorldSize( 8.0f );
				UserScaleIndicatorText->SetTextMaterial( UserScaleIndicatorMaterial );
				UserScaleIndicatorText->SetTextRenderColor( GetColor( EColors::WorldDraggingColor_TwoHands ).ToFColor( false ) );

				// Center the text horizontally
				UserScaleIndicatorText->SetHorizontalAlignment( EHTA_Center );
				UserScaleIndicatorText->SetVisibility( false );
			}
		}

		// Post processing
		{
			PostProcessComponent = NewObject<UPostProcessComponent>( AvatarMeshActor );
			AvatarMeshActor->AddOwnedComponent( PostProcessComponent );
			PostProcessComponent->SetupAttachment( AvatarMeshActor->GetRootComponent() );
			PostProcessComponent->RegisterComponent();

			// Unlimited size
			PostProcessComponent->bEnabled = VREd::ShowWorldMovementPostProcess->GetInt() == 1 ? true : false;
			PostProcessComponent->bUnbound = true;
		}
	}
}


UPostProcessComponent* FVREditorMode::GetPostProcessComponent()
{
	SpawnAvatarMeshActor();
	check( PostProcessComponent != nullptr );
	return PostProcessComponent;
}


void FVREditorMode::OnVREditorWindowClosed( const TSharedRef<SWindow>& ClosedWindow )
{
	StartExitingVRMode();
}


void FVREditorMode::Tick( FEditorViewportClient* ViewportClient, float DeltaTime )
{
	// Call parent implementation
	FEdMode::Tick( ViewportClient, DeltaTime );

	// Only if this is our VR viewport. Remember, editor modes get a chance to tick and receive input for each active viewport.
	if( ViewportClient != GetLevelViewportPossessedForVR().GetViewportClient().Get() )
	{
		return;
	}

	
	// Cancel any haptic effects that have been playing too long
	StopOldHapticEffects();

	// Get the latest controller data, and fill in our VirtualHands with fresh transforms
	PollInputIfNeeded();
	

	// Update hover.  Note that hover can also be updated when ticking our sub-systems below.
	{
		for( int32 HandIndex = 0; HandIndex < ARRAY_COUNT( VirtualHands ); ++HandIndex )
		{
			FVirtualHand& Hand = VirtualHands[ HandIndex ];
			Hand.bIsHovering = false;
			Hand.bIsHoveringOverUI = false;

			// Extensibility: Allow other modes to get a shot at this event before we handle it
			bool bHandled = false;
			FVector HoverImpactPoint = FVector::ZeroVector;
			bool bIsHoveringOverUI = false;
			OnVRHoverUpdateEvent.Broadcast( *ViewportClient, HandIndex, /* In/Out */ HoverImpactPoint, /* In/Out */ bIsHoveringOverUI, /* In/Out */ bHandled );

			if( bHandled )
			{
				Hand.bIsHovering = true;
				Hand.bIsHoveringOverUI = bIsHoveringOverUI;
				Hand.HoverLocation = HoverImpactPoint;
			}
		}
	}

	WorldInteraction->Tick( ViewportClient, DeltaTime );

	UISystem->Tick( ViewportClient, DeltaTime );

	// Update avatar meshes
	{
		if( AvatarMeshActor == nullptr )
		{
			SpawnAvatarMeshActor();
		}

		// Move our avatar mesh along with the room.  We need our hand components to remain the same coordinate space as the 
		{
			const FTransform RoomTransform = GetRoomTransform();
			AvatarMeshActor->SetActorTransform( RoomTransform );
		}

		// Our head will lock to the HMD position
		{
			FTransform RoomSpaceTransformWithWorldToMetersScaling = GetRoomSpaceHeadTransform();
			RoomSpaceTransformWithWorldToMetersScaling.SetScale3D( RoomSpaceTransformWithWorldToMetersScaling.GetScale3D() * FVector( GetWorldScaleFactor() ) );

			// @todo vreditor urgent: Head disabled until we can fix late frame update issue
			if( false && bActuallyUsingVR && GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHeadTrackingAllowed() )
			{
				HeadMeshComponent->SetVisibility( true );

				// Apply late frame update to the head mesh
				HeadMeshComponent->ResetRelativeTransform();
				const FTransform ParentToWorld = HeadMeshComponent->GetComponentToWorld();
				GEngine->HMDDevice->SetupLateUpdate( ParentToWorld, HeadMeshComponent );
				HeadMeshComponent->SetRelativeTransform( RoomSpaceTransformWithWorldToMetersScaling );
			}
			else
			{
				HeadMeshComponent->SetVisibility( false );
			}
		}

		// Scale the grid so that it stays the same size in the tracking space
		WorldMovementGridMeshComponent->SetRelativeScale3D( FVector( GetWorldScaleFactor() ) * VREd::GridScaleMultiplier->GetFloat() );

		WorldMovementGridMeshComponent->SetRelativeLocation( FVector( GetWorldScaleFactor() ) * FVector( 0.0f, 0.0f, VREd::GridHeightOffset->GetFloat() ) );

		for( int32 HandIndex = 0; HandIndex < ARRAY_COUNT( VirtualHands ); ++HandIndex )
		{
			FVirtualHand& Hand = VirtualHands[ HandIndex ];

			// @todo vreditor: Manually ticking motion controller components
			Hand.MotionControllerComponent->TickComponent( DeltaTime, ELevelTick::LEVELTICK_PauseTick, nullptr );

			// The hands need to stay the same size relative to our tracking space, so we inverse compensate for world to meters scale here
			// NOTE: We don't need to set the hand mesh location and rotation, as the MotionControllerComponent does that itself
			if( HandIndex == VREditorConstants::RightHandIndex && 
				GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )	// Oculus has asymmetrical controllers, so we mirror the mesh horizontally
			{
				Hand.HandMeshComponent->SetRelativeScale3D( FVector( GetWorldScaleFactor(), -GetWorldScaleFactor(), GetWorldScaleFactor() ) );
			}
			else
			{
				Hand.HandMeshComponent->SetRelativeScale3D( FVector( GetWorldScaleFactor() ) );
			}
			
			// We don't bother drawing hand meshes if we're in "forced VR mode" (because they'll just be on top of the camera).
			// Also, don't bother drawing hands if we're not currently tracking them.
			if( bActuallyUsingVR && Hand.bHaveMotionController )
			{
				Hand.HandMeshComponent->SetVisibility( true );
			}
			else
			{
				Hand.HandMeshComponent->SetVisibility( false );
			}

			FVector LaserPointerStart, LaserPointerEnd;
			if( GetLaserPointer( HandIndex, /* Out */ LaserPointerStart, /* Out */ LaserPointerEnd ) )
			{
				// Only show the laser if we're actually in VR
				Hand.LaserPointerMeshComponent->SetVisibility( bActuallyUsingVR );


			    // NOTE: We don't need to set the laser pointer location and rotation, as the MotionControllerComponent will do
			    // that later in the frame.  

				// If we're actively dragging something around, then we'll crop the laser length to the hover impact
				// point.  Otherwise, we always want the laser to protrude through hovered objects, so that you can
				// interact with translucent gizmo handles that are occluded by geometry
				FVector LaserPointerImpactPoint = LaserPointerEnd;
				if( Hand.bIsHovering && ( Hand.DraggingMode != EVREditorDraggingMode::Nothing || Hand.bIsHoveringOverUI ) )
				{
					LaserPointerImpactPoint = Hand.HoverLocation;
				}

				// Apply rotation offset to the laser direction
				const float LaserPointerRotationOffset = GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift ? VREd::OculusLaserPointerRotationOffset->GetFloat() : VREd::ViveLaserPointerRotationOffset->GetFloat();
				Hand.LaserPointerMeshComponent->SetRelativeRotation( FRotator( LaserPointerRotationOffset, 0.0f, 0.0f ) );

				const FVector LaserPointerDirection = ( LaserPointerImpactPoint - LaserPointerStart ).GetSafeNormal();

				// Offset the beginning of the laser pointer a bit, so that it doesn't overlap the hand mesh
				const float LaserPointerStartOffset =
					GetWorldScaleFactor() *
					( GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift ? VREd::OculusLaserPointerStartOffset->GetFloat() : VREd::ViveLaserPointerStartOffset->GetFloat() );

				const float LaserPointerLength = FMath::Max( 0.000001f, ( LaserPointerImpactPoint - LaserPointerStart ).Size() - LaserPointerStartOffset );

				Hand.LaserPointerMeshComponent->SetRelativeLocation( FRotator( LaserPointerRotationOffset, 0.0f, 0.0f ).RotateVector( FVector( LaserPointerStartOffset, 0.0f, 0.0f ) ) );

				// The laser pointer needs to stay the same size relative to our tracking space, so we inverse compensate for world to meters scale here
				float LaserPointerRadius = VREd::LaserPointerRadius->GetFloat() * GetWorldScaleFactor();
				float HoverMeshRadius = VREd::LaserPointerHoverBallRadius->GetFloat() * GetWorldScaleFactor();

				// If we're hovering over something really close to the camera, go ahead and shrink the effect
				// @todo vreditor: Can we make this actually just sized based on distance automatically?  The beam and impact point are basically a cone.
				if( Hand.bIsHoveringOverUI )
				{
					LaserPointerRadius *= 0.35f;	// @todo vreditor tweak
					HoverMeshRadius *= 0.35f;	// @todo vreditor tweak
				}

				Hand.LaserPointerMeshComponent->SetRelativeScale3D( FVector( LaserPointerLength, LaserPointerRadius * 2.0f, LaserPointerRadius * 2.0f ) );

				if( Hand.bIsHovering )
				{
					// The hover effect needs to stay the same size relative to our tracking space, so we inverse compensate for world to meters scale here
					Hand.HoverMeshComponent->SetRelativeScale3D( FVector( HoverMeshRadius * 2.0f ) );
					Hand.HoverMeshComponent->SetVisibility( true );
					Hand.HoverMeshComponent->SetWorldLocation( Hand.HoverLocation );

					// Show the light too, unless it's on top of UI.  It looks too distracting on top of UI.
					Hand.HoverPointLightComponent->SetVisibility( !Hand.bIsHoveringOverUI );

					// Update radius for world scaling
					Hand.HoverPointLightComponent->SetAttenuationRadius( VREd::LaserPointerLightRadius->GetFloat() * GetWorldScaleFactor() );


					// Pull hover light back a bit from the end of the ray
					const float PullBackAmount = VREd::LaserPointerLightPullBackDistance->GetFloat() * GetWorldScaleFactor();
					Hand.HoverPointLightComponent->SetWorldLocation( Hand.HoverLocation - PullBackAmount * LaserPointerDirection );
				}
				else
				{
					Hand.HoverMeshComponent->SetVisibility( false );
					Hand.HoverPointLightComponent->SetVisibility( false );
				}


				// Update laser pointer materials
				{
					// @todo vreditor: Hook up variety of colors and "crawling"
					const bool CrawlFade = 1.0f;
					const float CrawlSpeed = 10.0f;

					SetLaserVisuals( HandIndex, GetColor( EColors::DefaultColor ) );

					static FName StaticLengthParameterName( "Length" );
					Hand.LaserPointerMID->SetScalarParameterValue( StaticLengthParameterName, LaserPointerLength );
					Hand.TranslucentLaserPointerMID->SetScalarParameterValue( StaticLengthParameterName, LaserPointerLength );
				}
			}
			else
			{
				Hand.LaserPointerMeshComponent->SetVisibility( false );
				Hand.HoverMeshComponent->SetVisibility( false );
				Hand.HoverPointLightComponent->SetVisibility( false );
			}
		}

	
		// Update the user scale indicator
		{
			FVirtualHand LeftHand = GetVirtualHand( VREditorConstants::LeftHandIndex );
			FVirtualHand RightHand = GetVirtualHand( VREditorConstants::RightHandIndex );
			if ( ( LeftHand.DraggingMode == EVREditorDraggingMode::World && RightHand.DraggingMode == EVREditorDraggingMode::AssistingDrag ) ||
				 ( LeftHand.DraggingMode == EVREditorDraggingMode::AssistingDrag && RightHand.DraggingMode == EVREditorDraggingMode::World ) )
			{
				// Setting all components to be visible
				CurrentScaleProgressMeshComponent->SetVisibility( true );
				ScaleProgressMeshComponent->SetVisibility( true );
				UserScaleIndicatorText->SetVisibility( true );

				// Here we calculate the distance, direction and center of the two hands
				const FVector ScaleIndicatorStartPosition = LeftHand.Transform.GetLocation();
				const FVector ScaleIndicatorEndPosition = RightHand.Transform.GetLocation();
				const FVector ScaleIndicatorDirection = (ScaleIndicatorEndPosition - ScaleIndicatorStartPosition).GetSafeNormal();

				const float ScaleIndicatorLength = FMath::Max( 0.000001f, (ScaleIndicatorEndPosition - ScaleIndicatorStartPosition).Size() );
				const float Radius = ( VREd::ScaleProgressBarRadius->GetFloat() * GetWorldScaleFactor() );
				const float ProgressbarLength = VREd::ScaleProgressBarLength->GetFloat();
				const float Scale = GetWorldScaleFactor();
				 
				// Add an offset to the center, we don't want the hands to clip through the hand meshes
				FVector MiddleLocation = ScaleIndicatorEndPosition - ( ScaleIndicatorLength * 0.5f ) * ScaleIndicatorDirection;
				MiddleLocation.Z += Scale * 5.f;
				
				// Setting the transform for the scale progressbar
				{
					const FVector ProgressbarStart = MiddleLocation - ((ProgressbarLength * 0.5f) * Scale) * ScaleIndicatorDirection;

					ScaleProgressMeshComponent->SetWorldTransform( FTransform( ScaleIndicatorDirection.ToOrientationRotator(),
						ProgressbarStart,
						 FVector( ProgressbarLength * Scale, Radius, Radius )
						) );
				}

				// Setting the transform for the current scale progressbar from the center
				{
					const float CurrentProgressScale = (Scale * Scale) * (ProgressbarLength / (VREd::ScaleMax->GetFloat() / 100));
					const FVector CurrentProgressStart = MiddleLocation - (CurrentProgressScale * 0.5f) * ScaleIndicatorDirection;
					
					CurrentScaleProgressMeshComponent->SetWorldTransform( FTransform (  ScaleIndicatorDirection.ToOrientationRotator(),
							CurrentProgressStart,
							FVector( CurrentProgressScale, Radius * 2, Radius * 2 )
						) );
				}

				//Setting the transform for the scale text
				{
					MiddleLocation.Z += Scale * 5.0f;
					UserScaleIndicatorText->SetWorldTransform( FTransform( ( GetHeadTransform().GetLocation() - MiddleLocation ).ToOrientationRotator(),
						MiddleLocation,
						GetRoomSpaceHeadTransform().GetScale3D()  * Scale
						) );
					
					FNumberFormattingOptions NumberFormat;
					NumberFormat.MinimumIntegralDigits = 1;
					NumberFormat.MaximumIntegralDigits = 10000;
					NumberFormat.MinimumFractionalDigits = 1;
					NumberFormat.MaximumFractionalDigits = 1;
					UserScaleIndicatorText->SetText( FText::AsNumber( Scale, &NumberFormat ) );
				}
			}
			else
			{
				//Setting all components invisible
				CurrentScaleProgressMeshComponent->SetVisibility( false );
				ScaleProgressMeshComponent->SetVisibility( false );
				UserScaleIndicatorText->SetVisibility( false );
			}
		}
	}

	// Updating laser colors for both hands
	{
		for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
		{
			FLinearColor ResultColor = GetColor( EColors::DefaultColor );
			float CrawlSpeed = 0.0f;
			float CrawlFade = 0.0f;

			const FVirtualHand& Hand = GetVirtualHand( HandIndex );
			const FVirtualHand& OtherHand = GetOtherHand( HandIndex );

			const bool bIsDraggingWorldWithTwoHands =
				( Hand.DraggingMode == EVREditorDraggingMode::AssistingDrag && OtherHand.DraggingMode == EVREditorDraggingMode::World ) ||
				( Hand.DraggingMode == EVREditorDraggingMode::World && OtherHand.DraggingMode == EVREditorDraggingMode::AssistingDrag );

			if( bIsDraggingWorldWithTwoHands )
			{
				ResultColor = GetColor( EColors::WorldDraggingColor_TwoHands );
			}
			else if( Hand.DraggingMode == EVREditorDraggingMode::World )
			{
				if( WorldInteraction->IsTeleporting() )
				{
					ResultColor = GetColor( EColors::TeleportColor );
				}
				else
				{
					// We can teleport in this mode, so animate the laser a bit
					CrawlFade = 1.0f;
					CrawlSpeed = 5.0f;
					ResultColor = GetColor( EColors::WorldDraggingColor_OneHand );
				}
			}
			else if( Hand.DraggingMode == EVREditorDraggingMode::ActorsAtLaserImpact ||
				Hand.DraggingMode == EVREditorDraggingMode::Material ||
				Hand.DraggingMode == EVREditorDraggingMode::ActorsFreely ||
				Hand.DraggingMode == EVREditorDraggingMode::ActorsWithGizmo ||
				Hand.DraggingMode == EVREditorDraggingMode::AssistingDrag || 
				( GetUISystem().IsDraggingDockUI() && HandIndex == GetUISystem().GetDraggingDockUIHandIndex() ) )
			{
				ResultColor = GetColor( EColors::SelectionColor );
			}

			SetLaserVisuals( HandIndex, ResultColor, CrawlFade, CrawlSpeed );
		}
	}

	// Updating the opacity and visibility of the grid according to the controllers
	{
		if( VREd::ShowMovementGrid->GetInt() == 1)
		{
			// Show the grid full opacity when dragging or scaling
			float GoalOpacity = 0.f;
			if ( GetVirtualHand( VREditorConstants::LeftHandIndex ).DraggingMode == EVREditorDraggingMode::World || GetVirtualHand( VREditorConstants::RightHandIndex ).DraggingMode == EVREditorDraggingMode::World )
			{
				GoalOpacity = 1.0f;
			}
			else if ( ( GetVirtualHand( VREditorConstants::LeftHandIndex ).LastDraggingMode == EVREditorDraggingMode::World && !GetVirtualHand( VREditorConstants::LeftHandIndex ).DragTranslationVelocity.IsNearlyZero( VREd::GridMovementTolerance->GetFloat() ) ) )
			{
				GoalOpacity = FMath::Clamp( GetVirtualHand( VREditorConstants::LeftHandIndex ).DragTranslationVelocity.Size() / VREd::GridFadeStartVelocity->GetFloat(), 0.0f, 1.0f );
			}
			else if( ( GetVirtualHand( VREditorConstants::RightHandIndex ).LastDraggingMode == EVREditorDraggingMode::World && !GetVirtualHand( VREditorConstants::RightHandIndex ).DragTranslationVelocity.IsNearlyZero( VREd::GridMovementTolerance->GetFloat() ) ) )
			{
				GoalOpacity = FMath::Clamp( GetVirtualHand( VREditorConstants::RightHandIndex ).DragTranslationVelocity.Size() / VREd::GridFadeStartVelocity->GetFloat(), 0.0f, 1.0f );
			}

			// Check the current opacity and add or subtract to reach the goal
			float NewOpacity = WorldMovementGridOpacity;
			if( NewOpacity < GoalOpacity )
			{
				NewOpacity = FMath::Min( GoalOpacity, NewOpacity + DeltaTime * VREd::GridFadeMultiplier->GetFloat() );
			}
			else if( NewOpacity > GoalOpacity )
			{
				NewOpacity = FMath::Max( GoalOpacity, NewOpacity - DeltaTime * VREd::GridFadeMultiplier->GetFloat() );
			}

			// Only update when the opacity is different
			if( !FMath::IsNearlyEqual( WorldMovementGridOpacity, GoalOpacity ) )
			{
				WorldMovementGridOpacity = NewOpacity;

				// See if the opacity is almost zero and if the goal opacity is lower than the new opacity set it to zero if so. Otherwise it will flicker
				if( WorldMovementGridOpacity < SMALL_NUMBER )
				{
					WorldMovementGridOpacity = 0.f;
					WorldMovementGridMeshComponent->SetVisibility( false );
				}
				else
				{
					WorldMovementGridMeshComponent->SetVisibility( true );
				}

				static const FName OpacityName( "OpacityParam" );
				WorldMovementGridMID->SetScalarParameterValue( OpacityName, WorldMovementGridOpacity * VREd::GridMaxOpacity->GetFloat() );
			}
		}
		else
		{
			WorldMovementGridMeshComponent->SetVisibility( false );
		}
	}


	// Apply a post process effect while the user is moving the world around.  The effect "greys out" any pixels
	// that are not nearby, and completely hides the skybox and other very distant objects.  This is to help
	// prevent simulation sickness when moving/rotating/scaling the entire world around them.
	{
		PostProcessComponent->bEnabled = VREd::ShowWorldMovementPostProcess->GetInt() == 1 ? true : false;

		// Make sure our world size is reflected in the post process material
		static FName WorldScaleFactorParameterName( "WorldScaleFactor" );
		WorldMovementPostProcessMaterial->SetScalarParameterValue( WorldScaleFactorParameterName, GetWorldScaleFactor() );

		static FName RoomOriginParameterName( "RoomOrigin" );
		WorldMovementPostProcessMaterial->SetVectorParameterValue( RoomOriginParameterName, GetRoomTransform().GetLocation() );

		static FName FogColorParameterName( "FogColor" );
		// WorldMovementPostProcessMaterial->SetVectorParameterValue( FogColorParameterName, FLinearColor::Black );

		static FName StartDistanceParameterName( "StartDistance" );
		WorldMovementPostProcessMaterial->SetScalarParameterValue( StartDistanceParameterName, VREd::WorldMovementFogStartDistance->GetFloat() );

		static FName EndDistanceParameterName( "EndDistance" );
		WorldMovementPostProcessMaterial->SetScalarParameterValue( EndDistanceParameterName, VREd::WorldMovementFogEndDistance->GetFloat() );

		static FName FogOpacityParameterName( "FogOpacity" );
		WorldMovementPostProcessMaterial->SetScalarParameterValue( FogOpacityParameterName, VREd::WorldMovementFogOpacity->GetFloat() );

		static FName SkyboxDistanceParameterName( "SkyboxDistance" );
		WorldMovementPostProcessMaterial->SetScalarParameterValue( SkyboxDistanceParameterName, VREd::WorldMovementFogSkyboxDistance->GetFloat() );

		const bool bShouldDrawWorldMovementPostProcess = WorldMovementGridOpacity > KINDA_SMALL_NUMBER;
		if( bShouldDrawWorldMovementPostProcess != bIsDrawingWorldMovementPostProcess )
		{
			UPostProcessComponent* PostProcess = GetPostProcessComponent();
			if( bShouldDrawWorldMovementPostProcess )
			{
				PostProcess->AddOrUpdateBlendable( WorldMovementPostProcessMaterial );
				bIsDrawingWorldMovementPostProcess = true;
			}
			else
			{
				bIsDrawingWorldMovementPostProcess = false;
				PostProcess->Settings.RemoveBlendable( WorldMovementPostProcessMaterial );
			}
		}

		if( bIsDrawingWorldMovementPostProcess )
		{
			static FName OpacityParameterName( "Opacity" );
			WorldMovementPostProcessMaterial->SetScalarParameterValue( OpacityParameterName, FMath::Clamp( WorldMovementGridOpacity, 0.0f, 1.0f ) );
		}
	}

	// Setting the initial position and rotation based on the editor viewport when going into VR mode
	if ( bActuallyUsingVR && bFirstTick )
	{
		const FTransform RoomToWorld = GetRoomTransform();
		const FTransform WorldToRoom = RoomToWorld.Inverse();
		const FTransform ViewportToWorld = FTransform(SavedEditorState.ViewRotation, SavedEditorState.ViewLocation);
		const FTransform ViewportToRoom = ViewportToWorld * WorldToRoom;

		FTransform ViewportToRoomYaw = ViewportToRoom;
		ViewportToRoomYaw.SetRotation(FQuat(FRotator(0.0f, ViewportToRoomYaw.GetRotation().Rotator().Yaw, 0.0f)));

		FTransform HeadToRoomYaw = GetRoomSpaceHeadTransform();
		HeadToRoomYaw.SetRotation(FQuat(FRotator(0.0f, HeadToRoomYaw.GetRotation().Rotator().Yaw, 0.0f)));

		FTransform RoomToWorldYaw = RoomToWorld;
		RoomToWorldYaw.SetRotation(FQuat(FRotator(0.0f, RoomToWorldYaw.GetRotation().Rotator().Yaw, 0.0f)));

		FTransform ResultToWorld = ( HeadToRoomYaw.Inverse() * ViewportToRoomYaw ) * RoomToWorldYaw;
		SetRoomTransform(ResultToWorld);
	}


	// Update floating help text
	UpdateHelpLabels();

	StopOldHapticEffects();

	bFirstTick = false;
}


bool FVREditorMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	// Only if this is our VR viewport. Remember, editor modes get a chance to tick and receive input for each active viewport.
	if ( ViewportClient != GetLevelViewportPossessedForVR().GetViewportClient().Get() )
	{
		return InputKey(GetLevelViewportPossessedForVR().GetViewportClient().Get(), Viewport, Key, Event);
	}

	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

bool FVREditorMode::HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	bool bHandled = false;

	FVRAction* KnownAction = KeyToActionMap.Find( Key );
	if( KnownAction != nullptr )
	{
		FVRAction VRAction( KnownAction->ActionType, GetHandIndexFromKey( Key ) );
		if( VRAction.HandIndex != INDEX_NONE )
		{
			FVirtualHand& Hand = VirtualHands[ VRAction.HandIndex ];

			bool bShouldAbsorbEvent = false;

			// If "SelectAndMove" was pressed, we need to make sure that "SelectAndMove_LightlyPressed" is no longer
			// in effect before we start handling "SelectAndMove".
			if( VRAction.ActionType == EVRActionType::SelectAndMove )
			{
				if( Hand.bIsTriggerAtLeastLightlyPressed )
				{
					if( Event == IE_Pressed )
					{
						// How long since the trigger was lightly pressed?
						const float TimeSinceLightlyPressed = (float)( FPlatformTime::Seconds() - Hand.RealTimeTriggerWasLightlyPressed );
						if( !Hand.bIsClickingOnUI &&	// @todo vreditor: UI clicks happen with light presses; we never want to convert to a hard press!
							Hand.DraggingMode != EVREditorDraggingMode::Material &&	// @todo vreditor: Material dragging happens with light presses, don't convert to a hard press!
							Hand.DraggingMode != EVREditorDraggingMode::ActorsAtLaserImpact &&	// @todo vreditor: Actor dragging happens with light presses, don't convert to a hard press!
							( !Hand.bAllowTriggerLightPressLocking || TimeSinceLightlyPressed < VREd::TriggerLightlyPressedLockTime->GetFloat() ) )
						{
							Hand.bIsTriggerAtLeastLightlyPressed = false;
							Hand.bHasTriggerBeenReleasedSinceLastPress = false;

							// Synthesize an input key for releasing the light press
							// NOTE: Intentionally re-entrant call here.
							const EInputEvent InputEvent = IE_Released;
							const bool bWasLightReleaseHandled = HandleInputKey(ViewportClient, Viewport, VRAction.HandIndex == VREditorConstants::LeftHandIndex ? VREditorKeyNames::MotionController_Left_LightlyPressedTriggerAxis : VREditorKeyNames::MotionController_Right_LightlyPressedTriggerAxis, InputEvent);
						}
						else
						{
							// The button was held in a "lightly pressed" state for a long enough time that we should just continue to
							// treat it like a light press.  This makes it much easier to hold it in this state when you need to!
							bShouldAbsorbEvent = true;
						}
					}
					else
					{
						// Absorb the release of the SelectAndMove event if we are still in a lightly pressed state
						bShouldAbsorbEvent = true;
					}
				}
			}

			if( bShouldAbsorbEvent )
			{
				bHandled = true;
			}
			else
			{
				// Update touch state
				if( VRAction.ActionType == EVRActionType::Touch )
				{
					if( Event == IE_Pressed )
					{
						Hand.bIsTouchingTrackpad = true;
					}
					else if( Event == IE_Released )
					{
						Hand.bIsTouchingTrackpad = false;
						Hand.bIsTrackpadPositionValid[ 0 ] = false;
						Hand.bIsTrackpadPositionValid[ 1 ] = false;
					}
				}

				// Update modifier state
				if( VRAction.ActionType == EVRActionType::Modifier )
				{
					bHandled = true;
					if( Event == IE_Pressed )
					{
						Hand.bIsModifierPressed = true;
					}
					else if( Event == IE_Released )
					{
						Hand.bIsModifierPressed = false;
					}
				}

				// Undo/redo
				if( VRAction.ActionType == EVRActionType::Undo )
				{
					if( Event == IE_Pressed || Event == IE_Repeat )
					{
						Undo();
					}
					bHandled = true;
				}
				else if( VRAction.ActionType == EVRActionType::Redo )
				{
					if( Event == IE_Pressed || Event == IE_Repeat )
					{
						Redo();
					}
					bHandled = true;
				}

				if( !bHandled )
				{
					// Extensibility: Allow other modes to get a shot at this event before we handle it
					OnVRActionEvent.Broadcast( *ViewportClient, VRAction, Event, Hand.IsInputCaptured[ (int32)VRAction.ActionType ], /* In/Out */ bHandled );
				}

				if( !bHandled )
				{
					// If "select and move" was pressed but not handled, go ahead and deselect everything
					if( VRAction.ActionType == EVRActionType::SelectAndMove && Event == IE_Pressed )
					{
						GEditor->SelectNone( true, true );
					}
				}

				// Always handle mouse button presses when forcing VR mode, so that the editor doesn't select non-interactable actors
				// on mouse down, etc.
				if( !bActuallyUsingVR && VREd::UseMouseAsHandInForcedVRMode->GetInt() != 0 )
				{
					if( Key == EKeys::LeftMouseButton )
					{
						bHandled = true;
					}
				}
			}

			ApplyButtonPressColors( VRAction, Event );
		}
	}

	if( Key == EKeys::Escape )	// @todo vreditor: Should be a bindable action, not hard coded
	{
		// User hit escape, so bail out of VR mode
		StartExitingVRMode();

		bHandled = true;
	}

	StopOldHapticEffects();

	return bHandled;
}


bool FVREditorMode::InputAxis(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime)
{
	// Only if this is our VR viewport. Remember, editor modes get a chance to tick and receive input for each active viewport.
	if ( ViewportClient != GetLevelViewportPossessedForVR().GetViewportClient().Get() )
	{
		return InputAxis(GetLevelViewportPossessedForVR().GetViewportClient().Get(), Viewport, ControllerId, Key, Delta, DeltaTime);
	}

	return FEdMode::InputAxis(ViewportClient, Viewport, ControllerId, Key, Delta, DeltaTime);
}

bool FVREditorMode::HandleInputAxis(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime)
{
	bool bHandled = false;

	FVRAction* KnownAction = KeyToActionMap.Find( Key );
	if( KnownAction != nullptr )	// Ignore key repeats
	{
		FVRAction VRAction( KnownAction->ActionType, GetHandIndexFromKey( Key ) );
		FVirtualHand& Hand = VirtualHands[ VRAction.HandIndex ];

		if( VRAction.HandIndex != INDEX_NONE )
		{
			if( VRAction.ActionType == EVRActionType::TriggerAxis )
			{
				// Synthesize "lightly pressed" events for the trigger
				{
					const bool bIsFullPressAlreadyCapturing = Hand.IsInputCaptured[ (int32)EVRActionType::SelectAndMove ];
					if( !bIsFullPressAlreadyCapturing &&		// Don't fire if we're already capturing input for a full press
						!Hand.bIsTriggerAtLeastLightlyPressed &&	// Don't fire unless we are already pressed
						Hand.bHasTriggerBeenReleasedSinceLastPress &&	// Only if we've been fully released since the last time we fired
						Delta >= VREd::TriggerLightlyPressedThreshold->GetFloat() )
					{
						Hand.bIsTriggerAtLeastLightlyPressed = true;
						Hand.bAllowTriggerLightPressLocking = true;
						Hand.RealTimeTriggerWasLightlyPressed = FPlatformTime::Seconds();
						Hand.bHasTriggerBeenReleasedSinceLastPress = false;

						// Synthesize an input key for this light press
						const EInputEvent InputEvent = IE_Pressed;
						const bool bWasLightPressHandled = HandleInputKey(ViewportClient, Viewport, VRAction.HandIndex == VREditorConstants::LeftHandIndex ? VREditorKeyNames::MotionController_Left_LightlyPressedTriggerAxis : VREditorKeyNames::MotionController_Right_LightlyPressedTriggerAxis, InputEvent);
					}
					else if( Hand.bIsTriggerAtLeastLightlyPressed && Delta < VREd::TriggerLightlyPressedThreshold->GetFloat() )
					{
						Hand.bIsTriggerAtLeastLightlyPressed = false;

						// Synthesize an input key for this light press
						const EInputEvent InputEvent = IE_Released;
						const bool bWasLightReleaseHandled = HandleInputKey(ViewportClient, Viewport, VRAction.HandIndex == VREditorConstants::LeftHandIndex ? VREditorKeyNames::MotionController_Left_LightlyPressedTriggerAxis : VREditorKeyNames::MotionController_Right_LightlyPressedTriggerAxis, InputEvent);
					}
				}

				if( Delta < VREd::TriggerDeadZone->GetFloat() )
				{
					Hand.bHasTriggerBeenReleasedSinceLastPress = true;
				}

				// Synthesize "fully pressed" events for the trigger
				{
					if( !Hand.bIsTriggerFullyPressed &&	// Don't fire unless we are already pressed
						Delta >= VREd::TriggerFullyPressedThreshold->GetFloat() )
					{
						Hand.bIsTriggerFullyPressed = true;

						// Synthesize an input key for this full press
						const EInputEvent InputEvent = IE_Pressed;
						HandleInputKey(ViewportClient, Viewport, VRAction.HandIndex == VREditorConstants::LeftHandIndex ? VREditorKeyNames::MotionController_Left_FullyPressedTriggerAxis : VREditorKeyNames::MotionController_Right_FullyPressedTriggerAxis, InputEvent);
					}
					else if( Hand.bIsTriggerFullyPressed && Delta < VREd::TriggerFullyPressedReleaseThreshold->GetFloat() )
					{
						Hand.bIsTriggerFullyPressed = false;

						// Synthesize an input key for this full press
						const EInputEvent InputEvent = IE_Released;
						HandleInputKey(ViewportClient, Viewport, VRAction.HandIndex == VREditorConstants::LeftHandIndex ? VREditorKeyNames::MotionController_Left_FullyPressedTriggerAxis : VREditorKeyNames::MotionController_Right_FullyPressedTriggerAxis, InputEvent);
					}
				}
			}

			if( VRAction.ActionType == EVRActionType::TrackpadPositionX )
			{
				Hand.LastTrackpadPosition.X = Hand.bIsTrackpadPositionValid[0] ? Hand.TrackpadPosition.X : Delta;
				Hand.LastTrackpadPositionUpdateTime = FTimespan::FromSeconds( FPlatformTime::Seconds() );
				Hand.TrackpadPosition.X = Delta;
				Hand.bIsTrackpadPositionValid[0] = true;
			}

			if( VRAction.ActionType == EVRActionType::TrackpadPositionY )
			{
				if( VREd::InvertTrackpadVertical->GetInt() != 0 )
				{
					Delta = -Delta;	// Y axis is inverted from HMD
				}

				Hand.LastTrackpadPosition.Y = Hand.bIsTrackpadPositionValid[1] ? Hand.TrackpadPosition.Y : Delta;
				Hand.LastTrackpadPositionUpdateTime = FTimespan::FromSeconds( FPlatformTime::Seconds() );
				Hand.TrackpadPosition.Y = Delta;
				Hand.bIsTrackpadPositionValid[1] = true;
			}
		}
	}

	// ...
	StopOldHapticEffects();

	return bHandled;
}

bool FVREditorMode::IsCompatibleWith(FEditorModeID OtherModeID) const
{
	// We are compatible with all other modes!
	return true;
}


void FVREditorMode::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject( AvatarMeshActor );
	Collector.AddReferencedObject( HeadMeshComponent );
	Collector.AddReferencedObject( WorldMovementGridMeshComponent );
	Collector.AddReferencedObject( SnapGridActor );
	Collector.AddReferencedObject( SnapGridMeshComponent );
	Collector.AddReferencedObject( SnapGridMID );
	Collector.AddReferencedObject( PostProcessComponent );

	for( FVirtualHand& Hand : VirtualHands )
	{
		Collector.AddReferencedObject( Hand.MotionControllerComponent );
		Collector.AddReferencedObject( Hand.HandMeshComponent );
		Collector.AddReferencedObject( Hand.HandMeshMID );
		Collector.AddReferencedObject( Hand.LaserPointerMeshComponent );
		Collector.AddReferencedObject( Hand.LaserPointerMID );
		Collector.AddReferencedObject( Hand.TranslucentLaserPointerMID );
		Collector.AddReferencedObject( Hand.HoverMeshComponent );
		Collector.AddReferencedObject( Hand.HoverPointLightComponent );

		Collector.AddReferencedObjects( Hand.HelpLabels );

		Collector.AddReferencedObject( Hand.HoveringOverWidgetComponent );
	}

	Collector.AddReferencedObject( WorldMovementGridMID );
	Collector.AddReferencedObject( WorldMovementPostProcessMaterial );
	Collector.AddReferencedObject( ScaleProgressMeshComponent );
	Collector.AddReferencedObject( CurrentScaleProgressMeshComponent );
	Collector.AddReferencedObject( UserScaleIndicatorText );
}


void FVREditorMode::Render( const FSceneView* SceneView, FViewport* Viewport, FPrimitiveDrawInterface* PDI )
{
	StopOldHapticEffects();

	FEdMode::Render( SceneView, Viewport, PDI );

	if( bIsFullyInitialized )
	{
		// Let our subsystems render, too
		UISystem->Render( SceneView, Viewport, PDI );
		WorldInteraction->Render( SceneView, Viewport, PDI );
	}
}


float FVREditorMode::GetLaserPointerMaxLength() const
{
	return VREd::LaserPointerMaxLength->GetFloat() * GetWorldScaleFactor();
}


FTransform FVREditorMode::GetRoomTransform() const
{
	const FLevelEditorViewportClient& ViewportClient = GetLevelViewportPossessedForVR().GetLevelViewportClient();
	const FTransform RoomTransform( 
		ViewportClient.GetViewRotation().Quaternion(), 
		ViewportClient.GetViewLocation(),
		FVector( 1.0f ) );
	return RoomTransform;
}


void FVREditorMode::SetRoomTransform( const FTransform& NewRoomTransform )
{
	FLevelEditorViewportClient& ViewportClient = GetLevelViewportPossessedForVR().GetLevelViewportClient();
	ViewportClient.SetViewLocation( NewRoomTransform.GetLocation() );
	ViewportClient.SetViewRotation( NewRoomTransform.GetRotation().Rotator() );

	// Forcibly dirty the viewport camera location
	const bool bDollyCamera = false;
	ViewportClient.MoveViewportCamera( FVector::ZeroVector, FRotator::ZeroRotator, bDollyCamera );
}


bool FVREditorMode::GetHandTransformAndForwardVector( int32 HandIndex, FTransform& OutHandTransform, FVector& OutForwardVector ) const
{
	FVREditorMode* MutableThis = const_cast<FVREditorMode*>( this );
	MutableThis->PollInputIfNeeded();

	const FVirtualHand& Hand = VirtualHands[ HandIndex ];
	if( Hand.bHaveMotionController )
	{
		OutHandTransform = Hand.Transform;

		const float LaserPointerRotationOffset = GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift ? VREd::OculusLaserPointerRotationOffset->GetFloat() : VREd::ViveLaserPointerRotationOffset->GetFloat();
		OutForwardVector = OutHandTransform.GetRotation().RotateVector( FRotator( LaserPointerRotationOffset, 0.0f, 0.0f ).RotateVector( FVector( 1.0f, 0.0f, 0.0f ) ) );

		return true;
	}

	return false;
}


float FVREditorMode::GetMaxScale()
{
	return VREd::ScaleMax->GetFloat();
}

float FVREditorMode::GetMinScale()
{
	return VREd::ScaleMin->GetFloat();
}

void FVREditorMode::SetWorldToMetersScale( const float NewWorldToMetersScale )
{
	// @todo vreditor: This is bad because we're clobbering the world settings which will be saved with the map.  Instead we need to 
	// be able to apply an override before the scene view gets it

	ENGINE_API extern float GNewWorldToMetersScale;
	GNewWorldToMetersScale = NewWorldToMetersScale;
}

bool FVREditorMode::GetLaserPointer(const int32 HandIndex, FVector& LaserPointerStart, FVector& LaserPointerEnd, const bool bEvenIfUIIsInFront, const float LaserLengthOverride) const
{
	const FVirtualHand& Hand = VirtualHands[ HandIndex ];

	// If we're currently grabbing the world with both hands, then the laser pointer is not available
	if (Hand.bHaveMotionController &&
		!( Hand.DraggingMode == EVREditorDraggingMode::World && GetOtherHand( HandIndex ).DraggingMode == EVREditorDraggingMode::AssistingDrag ) &&
		!( Hand.DraggingMode == EVREditorDraggingMode::AssistingDrag && GetOtherHand( HandIndex ).DraggingMode == EVREditorDraggingMode::World ) )  
	{
		// If we have UI attached to us, don't allow a laser pointer
		if( bEvenIfUIIsInFront || !Hand.bHasUIInFront )
		{
			FTransform HandTransform;
			FVector HandForwardVector;
			if( GetHandTransformAndForwardVector( HandIndex, HandTransform, HandForwardVector ) )
			{
				LaserPointerStart = HandTransform.GetLocation();

				float LaserLength = 0.0f;
				if( LaserLengthOverride == 0.0f )
				{
					LaserLength = GetLaserPointerMaxLength();
				}
				else
				{
					LaserLength = LaserLengthOverride;
				}

				LaserPointerEnd = HandTransform.GetLocation() + HandForwardVector * LaserLength;

				return true;
			}
		}
	}

	return false;
}


bool FVREditorMode::IsInputCaptured( const FVRAction VRAction ) const
{
	check( VRAction.HandIndex >= 0 && VRAction.HandIndex < VREditorConstants::NumVirtualHands );
	check( (int32)VRAction.ActionType >= 0 && VRAction.ActionType < EVRActionType::TotalActionTypes );
	return VirtualHands[ VRAction.HandIndex ].IsInputCaptured[ (int32)VRAction.ActionType ];
}


FHitResult FVREditorMode::GetHitResultFromLaserPointer( int32 HandIndex, TArray<AActor*>* OptionalListOfIgnoredActors, const bool bIgnoreGizmos, const bool bEvenIfUIIsInFront, const float LaserLengthOverride )
{
	FHitResult BestHitResult;
	bool bBestHitResultSoFarIsUI = false;

	FVector LaserPointerStart, LaserPointerEnd;
	if( GetLaserPointer( HandIndex, LaserPointerStart, LaserPointerEnd, bEvenIfUIIsInFront, LaserLengthOverride ) )
	{
		// Twice twice.  Once for editor gizmos which are "on top" and always take precedence, then a second time
		// for all of the scene objects

		for( int32 PassIndex = bIgnoreGizmos ? 1 : 0; PassIndex < 2; ++PassIndex )
		{
			const bool bOnlyEditorGizmos = ( PassIndex == 0 );

			const bool bTraceComplex = true;
			FCollisionQueryParams TraceParams( NAME_None, bTraceComplex, nullptr );
			
			const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam;

			const ECollisionChannel CollisionChannel = bOnlyEditorGizmos ? COLLISION_GIZMO : ECC_Visibility;

			// Don't trace against our own head/hand meshes
			TraceParams.AddIgnoredActor( AvatarMeshActor );

			// Don't trace against our snap grid
			TraceParams.AddIgnoredActor( SnapGridActor );

			if( OptionalListOfIgnoredActors != nullptr )
			{
				TraceParams.AddIgnoredActors( *OptionalListOfIgnoredActors );
			}

			bool bHit = false;
			FHitResult HitResult;
			if( bOnlyEditorGizmos )
			{
				bHit = GetWorld()->LineTraceSingleByChannel( HitResult, LaserPointerStart, LaserPointerEnd, CollisionChannel, TraceParams, ResponseParam );
			}
			else
			{
				FCollisionObjectQueryParams EverythingButGizmos( FCollisionObjectQueryParams::AllObjects );
				EverythingButGizmos.RemoveObjectTypesToQuery( COLLISION_GIZMO );
				bHit = GetWorld()->LineTraceSingleByObjectType( HitResult, LaserPointerStart, LaserPointerEnd, EverythingButGizmos, TraceParams );
			}
			if( bHit )
			{
				// Is this better than what we have already?  Always prefer transform gizmos even if they were not using
				// COLLISION_GIZMO (some gizmo handles are opaque and use ECC_Visibility as their collision channel)
				// NOTE: We're treating components of floating UI actors as "gizmos" for the purpose of hit testing as long as the component is not the actual UI widget component
				const bool bHitResultIsUI = UISystem->IsWidgetAnEditorUIWidget( HitResult.GetComponent() );
				const bool bHitResultIsGizmo = HitResult.GetActor() != nullptr && ( ( HitResult.GetActor() == WorldInteraction->GetTransformGizmoActor() ) || ( !bHitResultIsUI && HitResult.GetActor()->IsA( AVREditorFloatingUI::StaticClass() ) ) );
				const bool bBestHitResultSoFarIsGizmo = BestHitResult.GetActor() != nullptr && ( ( BestHitResult.GetActor() == WorldInteraction->GetTransformGizmoActor() ) || ( !bBestHitResultSoFarIsUI && BestHitResult.GetActor()->IsA( AVREditorFloatingUI::StaticClass() ) ) );
				if( BestHitResult.GetActor() == nullptr ||	// Don't have anything yet?
					( bHitResultIsUI && bBestHitResultSoFarIsGizmo ) ||
					( bHitResultIsGizmo && !bBestHitResultSoFarIsUI ) )	// Always prefer gizmos, unless clicking on UI
				{
					BestHitResult = HitResult;
					bBestHitResultSoFarIsUI = bHitResultIsUI;
				}
			}
		}
	}

	return BestHitResult;
}


int32 FVREditorMode::GetHandIndexFromKey( const FKey& InKey ) const
{
	const FVRAction* Action = KeyToActionMap.Find( InKey );
	return Action ? Action->HandIndex : INDEX_NONE;
}


void FVREditorMode::PollInputIfNeeded()
{
	if( LastFrameNumberInputWasPolled != GFrameNumber )
	{
		PollInputFromMotionControllers();
		LastFrameNumberInputWasPolled = GFrameNumber;
	}
}


void FVREditorMode::PollInputFromMotionControllers()
{
	const FTransform RoomTransform = GetRoomTransform();
	for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
	{
		FVirtualHand& Hand = VirtualHands[ HandIndex ];

		Hand.bHaveMotionController = false;

		Hand.LastTransform = Hand.Transform;
		Hand.LastRoomSpaceTransform = Hand.RoomSpaceTransform;

		// If we're in forced VR mode and were asked to use the mouse instead of motion controllers, go
		// ahead and poll the mouse
		if( !bActuallyUsingVR && VREd::UseMouseAsHandInForcedVRMode->GetInt() != 0 )
		{
			// The mouse will always be our left hand
			if( HandIndex == VREditorConstants::LeftHandIndex )
			{
				Hand.bHaveMotionController = true;


				FLevelEditorViewportClient& ViewportClient = this->VREditorLevelViewportWeakPtr.Pin()->GetLevelViewportClient();
				FViewport* Viewport = ViewportClient.Viewport;
				if( Viewport->GetSizeXY().GetMin() > 0 )
				{
					FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues(
						Viewport,
						ViewportClient.GetScene(),
						ViewportClient.EngineShowFlags )
						.SetRealtimeUpdate( ViewportClient.IsRealtime() ) );
					FSceneView* SceneView = ViewportClient.CalcSceneView( &ViewFamily );

					const FIntPoint MousePosition( Viewport->GetMouseX(), Viewport->GetMouseY() );

					{
						const FViewportCursorLocation MouseViewportRay( SceneView, &ViewportClient, MousePosition.X, MousePosition.Y );
						
						const FVector WorldSpaceLocation = MouseViewportRay.GetOrigin();
						const FQuat WorldSpaceOrientation = MouseViewportRay.GetDirection().ToOrientationQuat();
						const FTransform HandToWorldTransform( WorldSpaceOrientation, WorldSpaceLocation, FVector( 1.0f ) );
						
						const FTransform WorldToRoomTransform = RoomTransform.Inverse();
						
						Hand.RoomSpaceTransform = HandToWorldTransform * WorldToRoomTransform;
					}
				}

				Hand.Transform = Hand.RoomSpaceTransform * RoomTransform;
			}
			else
			{
				// Skip the right hand when using the mouse
				continue;
			}
		}


		// Generic motion controllers
		if( !Hand.bHaveMotionController )
		{
			TArray<IMotionController*> MotionControllers = IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>( IMotionController::GetModularFeatureName() );
			for( auto MotionController : MotionControllers )
			{
				if( MotionController != nullptr && !Hand.bHaveMotionController )
				{
					FVector Location = FVector::ZeroVector;
					FRotator Rotation = FRotator::ZeroRotator;

					if( MotionController->GetControllerOrientationAndPosition( MotionControllerID, HandIndex == VREditorConstants::LeftHandIndex ? EControllerHand::Left : EControllerHand::Right, /* Out */ Rotation, /* Out */ Location ) )
					{
						Hand.bHaveMotionController = true;

						Hand.RoomSpaceTransform = FTransform(
							Rotation.Quaternion(),
							Location,
							FVector( 1.0f ) );

						Hand.Transform = Hand.RoomSpaceTransform * RoomTransform;
					}
				}
			}
		}
	}
}


AActor* FVREditorMode::SpawnTransientSceneActor( TSubclassOf<AActor> ActorClass, const FString& ActorName, const bool bWithSceneComponent ) const
{
	const bool bWasWorldPackageDirty = GetWorld()->GetOutermost()->IsDirty();

	// @todo vreditor: Needs respawn if world changes (map load, etc.)  Will that always restart the editor mode anyway?
	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.Name = MakeUniqueObjectName( GetWorld(), ActorClass, *ActorName );	// @todo vreditor: Without this, SpawnActor() can return us an existing PendingKill actor of the same name!  WTF?
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActorSpawnParameters.ObjectFlags = EObjectFlags::RF_Transient;

	check( ActorClass != nullptr );
	AActor* NewActor = GetWorld()->SpawnActor< AActor >( ActorClass, ActorSpawnParameters );
	NewActor->SetActorLabel( ActorName );

	if( bWithSceneComponent )
	{
		// Give the new actor a root scene component, so we can attach multiple sibling components to it
		USceneComponent* SceneComponent = NewObject<USceneComponent>( NewActor );
		NewActor->AddOwnedComponent( SceneComponent );
		NewActor->SetRootComponent( SceneComponent );
		SceneComponent->RegisterComponent();
	}

	// Don't dirty the level file after spawning a transient actor
	if( !bWasWorldPackageDirty )
	{
		GetWorld()->GetOutermost()->SetDirtyFlag( false );
	}

	return NewActor;
}


void FVREditorMode::DestroyTransientActor( AActor* Actor ) const
{
	if( Actor != nullptr )
	{
		const bool bWasWorldPackageDirty = GetWorld()->GetOutermost()->IsDirty();

		const bool bNetForce = false;
		const bool bShouldModifyLevel = false;	// Don't modify level for transient actor destruction
		GetWorld()->DestroyActor( Actor, bNetForce, bShouldModifyLevel );
		Actor = nullptr;

		// Don't dirty the level file after destroying a transient actor
		if( !bWasWorldPackageDirty )
		{
			GetWorld()->GetOutermost()->SetDirtyFlag( false );
		}
	}
}


FTransform FVREditorMode::GetRoomSpaceHeadTransform() const
{
	FTransform HeadTransform = FTransform::Identity;
	if( bActuallyUsingVR && GEngine->HMDDevice.IsValid() )
	{
		FQuat RoomSpaceHeadOrientation;
		FVector RoomSpaceHeadLocation;
		GEngine->HMDDevice->GetCurrentOrientationAndPosition( /* Out */ RoomSpaceHeadOrientation, /* Out */ RoomSpaceHeadLocation );

		HeadTransform = FTransform(
			RoomSpaceHeadOrientation,
			RoomSpaceHeadLocation,
			FVector( 1.0f ) );
	}

	return HeadTransform;
}


FTransform FVREditorMode::GetHeadTransform() const
{
	return GetRoomSpaceHeadTransform() * GetRoomTransform();
}


void FVREditorMode::PlayHapticEffect( const float LeftStrength, const float RightStrength )
{
	IInputInterface* InputInterface = FSlateApplication::Get().GetInputInterface();
	if( InputInterface )
	{
		const double CurrentTime = FPlatformTime::Seconds();

		// If we're dealing with an Oculus Rift, we have to setup haptic feedback directly.  Otherwise we can use our
		// generic force feedback system
		if( GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )
		{
			// Haptics are a little strong on Oculus Touch, so we scale them down a bit
			const float HapticScaleForRift = 0.8f;

			// Left hand
			{
				FHapticFeedbackValues HapticFeedbackValues;
				HapticFeedbackValues.Amplitude = LeftStrength * HapticScaleForRift;
				HapticFeedbackValues.Frequency = 0.5f;
				InputInterface->SetHapticFeedbackValues( MotionControllerID, (int32)EControllerHand::Left, HapticFeedbackValues );

				VirtualHands[ VREditorConstants::LeftHandIndex ].LastHapticTime = CurrentTime;
			}

			// Right hand
			{
				FHapticFeedbackValues HapticFeedbackValues;
				HapticFeedbackValues.Amplitude = RightStrength * HapticScaleForRift;
				HapticFeedbackValues.Frequency = 0.5f;
				InputInterface->SetHapticFeedbackValues( MotionControllerID, (int32)EControllerHand::Right, HapticFeedbackValues );

				VirtualHands[ VREditorConstants::RightHandIndex ].LastHapticTime = CurrentTime;
			}
		}
		else
		{
			FForceFeedbackValues ForceFeedbackValues;
			ForceFeedbackValues.LeftLarge = LeftStrength;
			ForceFeedbackValues.RightLarge = RightStrength;

			// @todo vreditor: If an Xbox controller is plugged in, this causes both the motion controllers and the Xbox controller to vibrate!
			InputInterface->SetForceFeedbackChannelValues( GetMotionControllerID(), ForceFeedbackValues );

			if( ForceFeedbackValues.LeftLarge > KINDA_SMALL_NUMBER )
			{
				VirtualHands[ VREditorConstants::LeftHandIndex ].LastHapticTime = CurrentTime;
			}

			if( ForceFeedbackValues.RightLarge > KINDA_SMALL_NUMBER )
			{
				VirtualHands[ VREditorConstants::RightHandIndex ].LastHapticTime = CurrentTime;
			}
		}
	}

	// @todo vreditor: We'll stop haptics right away because if the frame hitches, the controller will be left vibrating
	StopOldHapticEffects();
}


void FVREditorMode::StopOldHapticEffects()
{
	const float MinHapticTime = VREd::MinHapticTimeForRift->GetFloat();

	IInputInterface* InputInterface = FSlateApplication::Get().GetInputInterface();
	if( InputInterface )
	{
		// If we're dealing with an Oculus Rift, we have to setup haptic feedback directly.  Otherwise we can use our
		// generic force feedback system
		if( GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )
		{
			bool bWaitingForMoreHaptics = false;

			if( VirtualHands[ VREditorConstants::LeftHandIndex ].LastHapticTime != 0.0 ||
				VirtualHands[ VREditorConstants::RightHandIndex ].LastHapticTime != 0.0 )
			{
				do
				{
					bWaitingForMoreHaptics = false;

					const double CurrentTime = FPlatformTime::Seconds();

					// Left hand
					if( CurrentTime - VirtualHands[ VREditorConstants::LeftHandIndex ].LastHapticTime >= MinHapticTime )
					{
						FHapticFeedbackValues HapticFeedbackValues;
						HapticFeedbackValues.Amplitude = 0.0f;
						HapticFeedbackValues.Frequency = 0.0f;
						InputInterface->SetHapticFeedbackValues( MotionControllerID, (int32)EControllerHand::Left, HapticFeedbackValues );

						VirtualHands[ VREditorConstants::LeftHandIndex ].LastHapticTime = 0.0;
					}
					else if( VirtualHands[ VREditorConstants::LeftHandIndex ].LastHapticTime != 0.0 )
					{
						bWaitingForMoreHaptics = true;
					}

					// Right hand
					if( CurrentTime - VirtualHands[ VREditorConstants::RightHandIndex ].LastHapticTime >= MinHapticTime )
					{
						FHapticFeedbackValues HapticFeedbackValues;
						HapticFeedbackValues.Amplitude = 0.0f;
						HapticFeedbackValues.Frequency = 0.0f;
						InputInterface->SetHapticFeedbackValues( MotionControllerID, (int32)EControllerHand::Right, HapticFeedbackValues );

						VirtualHands[ VREditorConstants::RightHandIndex ].LastHapticTime = 0.0;
					}
					else if( VirtualHands[ VREditorConstants::RightHandIndex ].LastHapticTime != 0.0 )
					{
						bWaitingForMoreHaptics = true;
					}

					if( bWaitingForMoreHaptics && VREd::SleepForRiftHaptics->GetInt() != 0 )
					{
						FPlatformProcess::Sleep( 0 );
					}
				}
				// @todo vreditor urgent: This is needed so that haptics don't play too long.  Our Oculus code doesn't currently support 
				// multi-threading, so we need to delay the main thread to make sure we stop it before it rumbles for more than an instant!
				while( bWaitingForMoreHaptics && VREd::SleepForRiftHaptics->GetInt() != 0 );
			}
		}
		else
		{
			// @todo vreditor: Do we need to cancel haptics for non-Rift devices?  Doesn't seem like it
		}
	}
}

void FVREditorMode::ShowHelpForHand( const int32 HandIndex, const bool bShowIt )
{
	FVirtualHand& Hand = VirtualHands[ HandIndex ];

	if( bShowIt != Hand.bWantHelpLabels )
	{
		Hand.bWantHelpLabels = bShowIt;

		const FTimespan CurrentTime = FTimespan::FromSeconds( FApp::GetCurrentTime() );
		const FTimespan TimeSinceStartedFadingOut = CurrentTime - Hand.HelpLabelShowOrHideStartTime;
		const FTimespan HelpLabelFadeDuration = FTimespan::FromSeconds( VREd::HelpLabelFadeDuration->GetFloat() );

		// If we were already fading, account for that here
		if( TimeSinceStartedFadingOut < HelpLabelFadeDuration )
		{
			// We were already fading, so we'll reverse the time value so it feels continuous
			Hand.HelpLabelShowOrHideStartTime = CurrentTime - ( HelpLabelFadeDuration - TimeSinceStartedFadingOut );
		}
		else
		{
			Hand.HelpLabelShowOrHideStartTime = FTimespan::FromSeconds( FApp::GetCurrentTime() );
		}

		if( bShowIt && Hand.HelpLabels.Num() == 0 )
		{
			for( const auto& KeyToAction : KeyToActionMap )
			{
				const FKey Key = KeyToAction.Key;
				const FVRAction& VRAction = KeyToAction.Value;

				if( HandIndex == VRAction.HandIndex )
				{
					UStaticMeshSocket* Socket = FindMeshSocketForKey( Hand.HandMeshComponent->StaticMesh, Key );
					if( Socket != nullptr )
					{
						FText LabelText;
						FString ComponentName;

						switch( VRAction.ActionType )
						{
							case EVRActionType::Modifier:
								LabelText = LOCTEXT( "ModifierHelp", "Modifier" );
								ComponentName = TEXT( "ModifierHelp" );
								break;

							case EVRActionType::WorldMovement:
								LabelText = LOCTEXT( "WorldMovementHelp", "Move World" );
								ComponentName = TEXT( "WorldMovementHelp" );
								break;

							case EVRActionType::SelectAndMove:
								LabelText = LOCTEXT( "SelectAndMoveHelp", "Select & Move" );
								ComponentName = TEXT( "SelectAndMoveHelp" );
								break;

							case EVRActionType::SelectAndMove_LightlyPressed:
								LabelText = LOCTEXT( "SelectAndMove_LightlyPressedHelp", "Select & Move" );
								ComponentName = TEXT( "SelectAndMove_LightlyPressedHelp" );
								break;

							case EVRActionType::Touch:
								LabelText = LOCTEXT( "TouchHelp", "Slide" );
								ComponentName = TEXT( "TouchHelp" );
								break;

							case EVRActionType::Undo:
								LabelText = LOCTEXT( "UndoHelp", "Undo" );
								ComponentName = TEXT( "UndoHelp" );
								break;

							case EVRActionType::Redo:
								LabelText = LOCTEXT( "RedoHelp", "Redo" );
								ComponentName = TEXT( "RedoHelp" );
								break;

							case EVRActionType::Delete:
								LabelText = LOCTEXT( "DeleteHelp", "Delete" );
								ComponentName = TEXT( "DeleteHelp" );
								break;

							case EVRActionType::ConfirmRadialSelection:
								LabelText = LOCTEXT( "ConfirmRadialSelectionHelp", "Radial Menu" );
								ComponentName = TEXT( "ConfirmRadialSelectionHelp" );
								break;

							default:
								// If this goes off, a new EVRActionType was added and we need to update the above switch statement
								check( 0 );
								break;
						}
					
						const bool bWithSceneComponent = false;	// Nope, we'll spawn our own inside AFloatingText
						AFloatingText* FloatingText = SpawnTransientSceneActor<AFloatingText>( ComponentName, bWithSceneComponent );
						FloatingText->SetText( LabelText );

						Hand.HelpLabels.Add( Key, FloatingText );
					}
				}
			}
		}
	}
}


void FVREditorMode::UpdateHelpLabels()
{
	const FTimespan HelpLabelFadeDuration = FTimespan::FromSeconds( VREd::HelpLabelFadeDuration->GetFloat() );

	const FTransform HeadTransform = GetHeadTransform();
	for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
	{
		FVirtualHand& Hand = VirtualHands[ HandIndex ];

		// Only show help labels if the hand is pretty close to the face
		const float DistanceToHead = ( Hand.Transform.GetLocation() - HeadTransform.GetLocation() ).Size();
		const float MinDistanceToHeadForHelp = VREd::HelpLabelFadeDistance->GetFloat() * GetWorldScaleFactor();	// (in cm)
		bool bShowHelp = DistanceToHead <= MinDistanceToHeadForHelp;

		// Don't show help if a UI is summoned on that hand
		if( Hand.bHasUIInFront || Hand.bHasUIOnForearm || UISystem->IsShowingRadialMenu( HandIndex ) )
		{
			bShowHelp = false;
		}

		ShowHelpForHand( HandIndex, bShowHelp );

		// Have the labels finished fading out?  If so, we'll kill their actors!
		const FTimespan CurrentTime = FTimespan::FromSeconds( FApp::GetCurrentTime() );
		const FTimespan TimeSinceStartedFadingOut = CurrentTime - Hand.HelpLabelShowOrHideStartTime;
		if( !Hand.bWantHelpLabels && ( TimeSinceStartedFadingOut > HelpLabelFadeDuration ) )
		{
			// Get rid of help text
			for( auto& KeyAndValue : Hand.HelpLabels )
			{
				AFloatingText* FloatingText = KeyAndValue.Value;
				DestroyTransientActor( FloatingText );
			}
			Hand.HelpLabels.Reset();
		}
		else
		{
			// Update fading state
			float FadeAlpha = FMath::Clamp( (float)TimeSinceStartedFadingOut.GetTotalSeconds() / (float)HelpLabelFadeDuration.GetTotalSeconds(), 0.0f, 1.0f );
			if( !Hand.bWantHelpLabels )
			{
				FadeAlpha = 1.0f - FadeAlpha;
			}

			// Exponential falloff, so the fade is really obvious (gamma/HDR)
			FadeAlpha = FMath::Pow( FadeAlpha, 3.0f );

			for( auto& KeyAndValue : Hand.HelpLabels )
			{
				const FKey Key = KeyAndValue.Key;
				AFloatingText* FloatingText = KeyAndValue.Value;

				UStaticMeshSocket* Socket = FindMeshSocketForKey( Hand.HandMeshComponent->StaticMesh, Key );
				check( Socket != nullptr );
				FTransform SocketRelativeTransform( Socket->RelativeRotation, Socket->RelativeLocation, Socket->RelativeScale );

				// Oculus has asymmetrical controllers, so we the sock transform horizontally
				if( HandIndex == VREditorConstants::RightHandIndex &&
					GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )
				{
					const FVector Scale3D = SocketRelativeTransform.GetLocation();
					SocketRelativeTransform.SetLocation( FVector( Scale3D.X, -Scale3D.Y, Scale3D.Z ) );
				}

				// Make sure the labels stay the same size even when the world is scaled
				FTransform HandTransformWithWorldToMetersScaling = Hand.Transform;
				HandTransformWithWorldToMetersScaling.SetScale3D( HandTransformWithWorldToMetersScaling.GetScale3D() * FVector( GetWorldScaleFactor() ) );


				// Position right on top of the controller itself
				FTransform FloatingTextTransform = SocketRelativeTransform * HandTransformWithWorldToMetersScaling;
				FloatingText->SetActorTransform( FloatingTextTransform );

				// Orientate it toward the viewer
				FloatingText->Update( HeadTransform.GetLocation() );

				// Update fade state
				FloatingText->SetOpacity( FadeAlpha );
			}
		}
	}
}


UStaticMeshSocket* FVREditorMode::FindMeshSocketForKey( UStaticMesh* StaticMesh, const FKey Key )
{
	// @todo vreditor: Hard coded mapping of socket names (e.g. "Shoulder") to expected names of sockets in the static mesh
	FName SocketName = NAME_None;
	if( Key == EKeys::MotionController_Left_Shoulder || Key == EKeys::MotionController_Right_Shoulder )
	{
		static FName ShoulderSocketName( "Shoulder" );
		SocketName = ShoulderSocketName;
	}
	else if( Key == EKeys::MotionController_Left_Trigger || Key == EKeys::MotionController_Right_Trigger ||
		     Key == VREditorKeyNames::MotionController_Left_FullyPressedTriggerAxis || Key == VREditorKeyNames::MotionController_Right_FullyPressedTriggerAxis ||
			 Key == VREditorKeyNames::MotionController_Left_LightlyPressedTriggerAxis || Key == VREditorKeyNames::MotionController_Right_LightlyPressedTriggerAxis )
	{
		static FName TriggerSocketName( "Trigger" );
		SocketName = TriggerSocketName;
	}
	else if( Key == EKeys::MotionController_Left_Grip1 || Key == EKeys::MotionController_Right_Grip1 )
	{
		static FName GripSocketName( "Grip" );
		SocketName = GripSocketName;
	}
	else if( Key == EKeys::MotionController_Left_Thumbstick || Key == EKeys::MotionController_Right_Thumbstick )
	{
		static FName ThumbstickSocketName( "Thumbstick" );
		SocketName = ThumbstickSocketName;
	}
	else if( Key == SteamVRControllerKeyNames::Touch0 || Key == SteamVRControllerKeyNames::Touch1 )
	{
		static FName TouchSocketName( "Touch" );
		SocketName = TouchSocketName;
	}					   
	else if( Key == EKeys::MotionController_Left_Thumbstick_Down || Key == EKeys::MotionController_Right_Thumbstick_Down )
	{
		static FName DownSocketName( "Down" );
		SocketName = DownSocketName;
	}
	else if( Key == EKeys::MotionController_Left_Thumbstick_Up || Key == EKeys::MotionController_Right_Thumbstick_Up )
	{
		static FName UpSocketName( "Up" );
		SocketName = UpSocketName;
	}
	else if( Key == EKeys::MotionController_Left_Thumbstick_Left || Key == EKeys::MotionController_Right_Thumbstick_Left )
	{
		static FName LeftSocketName( "Left" );
		SocketName = LeftSocketName;
	}
	else if( Key == EKeys::MotionController_Left_Thumbstick_Right || Key == EKeys::MotionController_Right_Thumbstick_Right )
	{
		static FName RightSocketName( "Right" );
		SocketName = RightSocketName;
	}
	else if( Key == EKeys::MotionController_Left_FaceButton1 || Key == EKeys::MotionController_Right_FaceButton1 )
	{
		static FName FaceButton1SocketName( "FaceButton1" );
		SocketName = FaceButton1SocketName;
	}
	else if( Key == EKeys::MotionController_Left_FaceButton2 || Key == EKeys::MotionController_Right_FaceButton2 )
	{
		static FName FaceButton2SocketName( "FaceButton2" );
		SocketName = FaceButton2SocketName;
	}
	else if( Key == EKeys::MotionController_Left_FaceButton3 || Key == EKeys::MotionController_Right_FaceButton3 )
	{
		static FName FaceButton3SocketName( "FaceButton3" );
		SocketName = FaceButton3SocketName;
	}
	else if( Key == EKeys::MotionController_Left_FaceButton4 || Key == EKeys::MotionController_Right_FaceButton4 )
	{
		static FName FaceButton4SocketName( "FaceButton4" );
		SocketName = FaceButton4SocketName;
	}
	else
	{
		// Not a key that we care about
	}

	if( SocketName != NAME_None )
	{
		UStaticMeshSocket* Socket = StaticMesh->FindSocket( SocketName );
		if( Socket != nullptr )
		{
			return Socket;
		}
	}

	return nullptr;
};


const SLevelViewport& FVREditorMode::GetLevelViewportPossessedForVR() const
{
	return *VREditorLevelViewportWeakPtr.Pin();
}


SLevelViewport& FVREditorMode::GetLevelViewportPossessedForVR()
{
	return *VREditorLevelViewportWeakPtr.Pin();
}


void FVREditorMode::OnMapChange( uint32 MapChangeFlags )
{
	CleanUpActorsBeforeMapChangeOrSimulate();
}


void FVREditorMode::OnBeginPIE( const bool bIsSimulatingInEditor )
{
	CleanUpActorsBeforeMapChangeOrSimulate();
}


void FVREditorMode::OnEndPIE( const bool bIsSimulatingInEditor )
{
	CleanUpActorsBeforeMapChangeOrSimulate();
}


void FVREditorMode::OnSwitchBetweenPIEAndSIE( const bool bIsSimulatingInEditor )
{
	CleanUpActorsBeforeMapChangeOrSimulate();
}


void FVREditorMode::CleanUpActorsBeforeMapChangeOrSimulate()
{
	// NOTE: This will be called even when this mode is not currently active!

	DestroyTransientActor( AvatarMeshActor );
	AvatarMeshActor = nullptr;
	HeadMeshComponent = nullptr;
	WorldMovementGridMeshComponent = nullptr;

	if( WorldMovementGridMID != nullptr )
	{
		WorldMovementGridMID->MarkPendingKill();
		WorldMovementGridMID = nullptr;
	}

	if( WorldMovementPostProcessMaterial != nullptr )
	{
		WorldMovementPostProcessMaterial->MarkPendingKill();
		WorldMovementPostProcessMaterial = nullptr;
	}

	DestroyTransientActor( SnapGridActor );
	SnapGridActor = nullptr;
	SnapGridMeshComponent = nullptr;

	if( SnapGridMID != nullptr )
	{
		SnapGridMID->MarkPendingKill();
		SnapGridMID = nullptr;
	}

	PostProcessComponent = nullptr;
	ScaleProgressMeshComponent = nullptr;
	CurrentScaleProgressMeshComponent = nullptr;
	UserScaleIndicatorText = nullptr;

	for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
	{
		FVirtualHand& Hand = VirtualHands[ HandIndex ];

		Hand.MotionControllerComponent = nullptr;
		Hand.HandMeshComponent = nullptr;
		Hand.HoverMeshComponent = nullptr;
		Hand.HoverPointLightComponent = nullptr;
		Hand.LaserPointerMeshComponent = nullptr;
		Hand.LaserPointerMID = nullptr;
		Hand.TranslucentLaserPointerMID = nullptr;

		for( auto& KeyAndValue : Hand.HelpLabels )
		{
			AFloatingText* FloatingText = KeyAndValue.Value;
			DestroyTransientActor( FloatingText );
		}
		Hand.HelpLabels.Reset();
	}

	if( UISystem != nullptr )
	{
		UISystem->CleanUpActorsBeforeMapChangeOrSimulate();
	}

	if( WorldInteraction != nullptr )
	{
		WorldInteraction->CleanUpActorsBeforeMapChangeOrSimulate();
	}
}


void FVREditorMode::Undo()
{
	GUnrealEd->Exec( GetWorld(), TEXT( "TRANSACTION UNDO" ) );
}


void FVREditorMode::Redo()
{
	GUnrealEd->Exec( GetWorld(), TEXT( "TRANSACTION REDO" ) );
}


void FVREditorMode::Copy()
{
	// @todo vreditor: Needs CanExecute()  (see LevelEditorActions.cpp)
	GUnrealEd->Exec( GetWorld(), TEXT( "EDIT COPY" ) );
}


void FVREditorMode::Paste()
{
	// @todo vreditor: Needs CanExecute()  (see LevelEditorActions.cpp)
	// @todo vreditor: Needs "paste here" style pasting (TO=HERE), but with ray
	GUnrealEd->Exec( GetWorld(), TEXT( "EDIT PASTE" ) );
}


void FVREditorMode::Duplicate()
{
	ABrush::SetSuppressBSPRegeneration( true );
	GEditor->edactDuplicateSelected( GetWorld()->GetCurrentLevel(), false );
	ABrush::SetSuppressBSPRegeneration( false );
}


void FVREditorMode::SnapSelectedActorsToGround()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT( "LevelEditor" ) );
	const FLevelEditorCommands& Commands = LevelEditorModule.GetLevelEditorCommands();
	const TSharedPtr< FUICommandList >& CommandList = GetLevelViewportPossessedForVR().GetParentLevelEditor().Pin()->GetLevelEditorActions();

	CommandList->ExecuteAction( Commands.SnapBottomCenterBoundsToFloor.ToSharedRef() );
	
	// @todo vreditor: This should not be needed after to allow transformables to stop animating the transforms of actors after they come to rest
	WorldInteraction->SetupTransformablesForSelectedActors();
}

void FVREditorMode::SetLaserVisuals( const int32 HandIndex, const FLinearColor& NewColor, const float CrawlFade, const float CrawlSpeed )
{
	FVirtualHand Hand = GetVirtualHand( HandIndex );

	static FName StaticLaserColorParameterName( "LaserColor" );
	Hand.LaserPointerMID->SetVectorParameterValue( StaticLaserColorParameterName, NewColor );
	Hand.TranslucentLaserPointerMID->SetVectorParameterValue( StaticLaserColorParameterName, NewColor );

	static FName StaticCrawlParameterName( "Crawl" );
	Hand.LaserPointerMID->SetScalarParameterValue( StaticCrawlParameterName, CrawlFade );
	Hand.TranslucentLaserPointerMID->SetScalarParameterValue( StaticCrawlParameterName, CrawlFade );

	static FName StaticCrawlSpeedParameterName( "CrawlSpeed" );
	Hand.LaserPointerMID->SetScalarParameterValue( StaticCrawlSpeedParameterName, CrawlSpeed );
	Hand.TranslucentLaserPointerMID->SetScalarParameterValue( StaticCrawlSpeedParameterName, CrawlSpeed );

	static FName StaticHandTrimColorParameter( "TrimGlowColor" );
	Hand.HandMeshMID->SetVectorParameterValue( StaticHandTrimColorParameter, NewColor );

	Hand.HoverPointLightComponent->SetLightColor( NewColor );
}


void FVREditorMode::ApplyVelocityDamping( FVector& Velocity, const bool bVelocitySensitive )
{
	const float InertialMovementZeroEpsilon = 0.01f;	// @todo vreditor tweak
	if( !Velocity.IsNearlyZero( InertialMovementZeroEpsilon ) )
	{
		// Apply damping
		if( bVelocitySensitive )
		{
			const float DampenMultiplierAtLowSpeeds = 0.94f;	// @todo vreditor tweak
			const float DampenMultiplierAtHighSpeeds = 0.99f;	// @todo vreditor tweak
			const float SpeedForMinimalDamping = 2.5f * GetWorldScaleFactor();	// cm/frame	// @todo vreditor tweak
			const float SpeedBasedDampeningScalar = FMath::Clamp( Velocity.Size(), 0.0f, SpeedForMinimalDamping ) / SpeedForMinimalDamping;	// @todo vreditor: Probably needs a curve applied to this to compensate for our framerate insensitivity
			Velocity = Velocity * FMath::Lerp( DampenMultiplierAtLowSpeeds, DampenMultiplierAtHighSpeeds, SpeedBasedDampeningScalar );	// @todo vreditor: Frame rate sensitive damping.  Make use of delta time!
		}
		else
		{
			Velocity = Velocity * 0.95f;
		}
	}

	if( Velocity.IsNearlyZero( InertialMovementZeroEpsilon ) )
	{
		Velocity = FVector::ZeroVector;
	}
}


void FVREditorMode::ApplyButtonPressColors( FVRAction VRAction, EInputEvent Event )
{
	SpawnAvatarMeshActor();
	
	FVirtualHand Hand = GetVirtualHand( VRAction.HandIndex );

	const float PressStrength = 10.0f;

	//Trigger
	if ( VRAction.ActionType == EVRActionType::SelectAndMove || VRAction.ActionType == EVRActionType::SelectAndMove_LightlyPressed )
	{
		static FName StaticTriggerParameter( "B1" );

		if( Event == IE_Pressed)
		{
			Hand.HandMeshMID->SetScalarParameterValue( StaticTriggerParameter, PressStrength );
		}
		else if ( Event == IE_Released )
		{
			Hand.HandMeshMID->SetScalarParameterValue( StaticTriggerParameter, 0.0f );
		}
	}

	//Shoulder button
	if ( VRAction.ActionType == EVRActionType::WorldMovement )
	{
		static FName StaticShoulderParameter( "B2" );

		if (Event == IE_Pressed)
		{
			Hand.HandMeshMID->SetScalarParameterValue( StaticShoulderParameter, PressStrength );
		}
		else if( Event == IE_Released )
		{
			Hand.HandMeshMID->SetScalarParameterValue( StaticShoulderParameter, 0.0f );
		}
	}

	//Trackpad
	if( VRAction.ActionType == EVRActionType::ConfirmRadialSelection )
	{
		static FName StaticTrackpadParameter( "B3" );

		if( Event == IE_Pressed )
		{
			Hand.HandMeshMID->SetScalarParameterValue( StaticTrackpadParameter, PressStrength );
		}
		else if( Event == IE_Released )
		{
			Hand.HandMeshMID->SetScalarParameterValue( StaticTrackpadParameter, 0.0f );
		}
	}

	if( VRAction.ActionType == EVRActionType::Modifier )
	{
		static FName StaticModifierParameter( "B4" );

		if( Event == IE_Pressed )
		{
			Hand.HandMeshMID->SetScalarParameterValue( StaticModifierParameter, PressStrength );
		}
		else if( Event == IE_Released )
		{
			Hand.HandMeshMID->SetScalarParameterValue( StaticModifierParameter, 0.0f );
		}
	}
}

void FVREditorMode::CycleTransformGizmoHandleType()
{
	EGizmoHandleTypes NewGizmoType = (EGizmoHandleTypes)( (uint8)CurrentGizmoType + 1 );
	if( NewGizmoType > EGizmoHandleTypes::Scale )
	{
		NewGizmoType = EGizmoHandleTypes::All;
	}

	CurrentGizmoType = NewGizmoType;
}


EGizmoHandleTypes FVREditorMode::GetCurrentGizmoType() const
{
	return CurrentGizmoType;
}


void FVREditorMode::CycleTransformGizmoCoordinateSpace()
{
	const ECoordSystem CurrentCoordSystem = GLevelEditorModeTools().GetCoordSystem();
	GLevelEditorModeTools().SetCoordSystem( CurrentCoordSystem == COORD_World ? COORD_Local : COORD_World );
}


ECoordSystem FVREditorMode::GetTransformGizmoCoordinateSpace() const
{
	const ECoordSystem CurrentCoordSystem = GLevelEditorModeTools().GetCoordSystem();
	return CurrentCoordSystem;
}


EHMDDeviceType::Type FVREditorMode::GetHMDDeviceType() const
{
	return GEngine->HMDDevice.IsValid() ? GEngine->HMDDevice->GetHMDDeviceType() : EHMDDeviceType::DT_SteamVR;
}

FLinearColor FVREditorMode::GetColor( const EColors Color ) const
{
	return Colors[ (int32)Color ];
}


bool FVREditorMode::IsHandAimingTowardsCapsule( const int32 HandIndex, const FTransform& CapsuleTransform, FVector CapsuleStart, const FVector CapsuleEnd, const float CapsuleRadius, const float MinDistanceToCapsule, const FVector CapsuleFrontDirection, const float MinDotForAimingAtCapsule ) const
{
	bool bIsAimingTowards = false;
	const float WorldScaleFactor = GetWorldScaleFactor();

	FVector LaserPointerStart, LaserPointerEnd;
	if( GetLaserPointer( HandIndex, /* Out */ LaserPointerStart, /* Out */ LaserPointerEnd ) )
	{
		const FVector LaserPointerStartInCapsuleSpace = CapsuleTransform.InverseTransformPosition( LaserPointerStart );
		const FVector LaserPointerEndInCapsuleSpace = CapsuleTransform.InverseTransformPosition( LaserPointerEnd );

		FVector ClosestPointOnLaserPointer, ClosestPointOnUICapsule;
		FMath::SegmentDistToSegment(
			LaserPointerStartInCapsuleSpace, LaserPointerEndInCapsuleSpace,
			CapsuleStart, CapsuleEnd,
			/* Out */ ClosestPointOnLaserPointer,
			/* Out */ ClosestPointOnUICapsule );

		const bool bIsClosestPointInsideCapsule = ( ClosestPointOnLaserPointer - ClosestPointOnUICapsule ).Size() <= CapsuleRadius;

		const FVector TowardLaserPointerVector = ( ClosestPointOnLaserPointer - ClosestPointOnUICapsule ).GetSafeNormal();

		// Apply capsule radius
		ClosestPointOnUICapsule += TowardLaserPointerVector * CapsuleRadius;

		if( false )	// @todo vreditor debug
		{
			const float RenderCapsuleLength = ( CapsuleEnd - CapsuleStart ).Size() + CapsuleRadius * 2.0f;
			// @todo vreditor:  This capsule draws with the wrong orientation
			if( false )
			{
				DrawDebugCapsule( GetWorld(), CapsuleTransform.TransformPosition( CapsuleStart + ( CapsuleEnd - CapsuleStart ) * 0.5f ), RenderCapsuleLength * 0.5f, CapsuleRadius, CapsuleTransform.GetRotation() * FRotator( 90.0f, 0, 0 ).Quaternion(), FColor::Green, false, 0.0f );
			}
			DrawDebugLine( GetWorld(), CapsuleTransform.TransformPosition( ClosestPointOnLaserPointer ), CapsuleTransform.TransformPosition( ClosestPointOnUICapsule ), FColor::Green, false, 0.0f );
			DrawDebugSphere( GetWorld(), CapsuleTransform.TransformPosition( ClosestPointOnLaserPointer ), 1.5f * WorldScaleFactor, 32, FColor::Red, false, 0.0f );
			DrawDebugSphere( GetWorld(), CapsuleTransform.TransformPosition( ClosestPointOnUICapsule ), 1.5f * WorldScaleFactor, 32, FColor::Green, false, 0.0f );
		}

		// If we're really close to the capsule
		if( bIsClosestPointInsideCapsule ||
			( ClosestPointOnUICapsule - ClosestPointOnLaserPointer ).Size() <= MinDistanceToCapsule )
		{
			const FVector LaserPointerDirectionInCapsuleSpace = ( LaserPointerEndInCapsuleSpace - LaserPointerStartInCapsuleSpace ).GetSafeNormal();

			if( false )	// @todo vreditor debug
			{
				DrawDebugLine( GetWorld(), CapsuleTransform.TransformPosition( FVector::ZeroVector ), CapsuleTransform.TransformPosition( CapsuleFrontDirection * 5.0f ), FColor::Yellow, false, 0.0f );
				DrawDebugLine( GetWorld(), CapsuleTransform.TransformPosition( FVector::ZeroVector ), CapsuleTransform.TransformPosition( -LaserPointerDirectionInCapsuleSpace * 5.0f ), FColor::Purple, false, 0.0f );
			}

			const float Dot = FVector::DotProduct( CapsuleFrontDirection, -LaserPointerDirectionInCapsuleSpace );
			if( Dot >= MinDotForAimingAtCapsule )
			{
				bIsAimingTowards = true;
			}
		}
	}

	return bIsAimingTowards;
}

#undef LOCTEXT_NAMESPACE