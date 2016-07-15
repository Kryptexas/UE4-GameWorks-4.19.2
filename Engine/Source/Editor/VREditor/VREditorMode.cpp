// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorMode.h"
#include "VREditorUISystem.h"
#include "VREditorWorldInteraction.h"
#include "VREditorTransformGizmo.h"
#include "VREditorFloatingText.h"
#include "VREditorFloatingUI.h"
#include "Teleporter/VREditorTeleporter.h"


#include "CameraController.h"
#include "DynamicMeshBuilder.h"
#include "Features/IModularFeatures.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "IMotionController.h"
#include "MotionControllerComponent.h"
#include "EngineAnalytics.h"
#include "IHeadMountedDisplay.h"
#include "EngineAnalytics.h"
#include "IAnalyticsProvider.h"

#include "Editor/LevelEditor/Public/LevelEditorActions.h"
#include "Editor/ViewportInteraction/Public/ViewportInteraction.h"
#include "VREditorInteractor.h"
#include "MouseCursorInteractor.h"
#include "VREditorMotionControllerInteractor.h"

#include "Interactables/VREditorButton.h"

#define LOCTEXT_NAMESPACE "VREditorMode"

namespace VREd
{
	static FAutoConsoleVariable UseMouseAsHandInForcedVRMode( TEXT( "VREd.UseMouseAsHandInForcedVRMode" ), 1, TEXT( "When in forced VR mode, enabling this setting uses the mouse cursor as a virtual hand instead of motion controllers" ) );

	static FAutoConsoleVariable GridMovementTolerance( TEXT( "VREd.GridMovementTolerance" ), 0.1f, TEXT( "Tolerance for movement when the grid must disappear" ) );
	static FAutoConsoleVariable GridScaleMultiplier( TEXT( "VREd.GridScaleMultiplier" ), 35.0f, TEXT( "Scale of the grid" ) );
	static FAutoConsoleVariable GridFadeMultiplier( TEXT( "VREd.GridFadeMultiplier" ), 3.0f, TEXT( "Grid fade in/out speed, in 'fades per second'" ) );
	static FAutoConsoleVariable GridFadeStartVelocity( TEXT( "VREd.GridFadeStartVelocity" ), 10.f, TEXT( "Grid fade duration" ) );
	static FAutoConsoleVariable GridMaxOpacity( TEXT( "VREd.GridMaxFade" ), 0.8f, TEXT( "Grid maximum opacity" ) );
	static FAutoConsoleVariable GridHeightOffset( TEXT( "VREd.GridHeightOffset" ), 0.0f, TEXT( "Height offset for the world movement grid.  Useful when tracking space is not properly calibrated" ) );

	static FAutoConsoleVariable WorldMovementFogOpacity( TEXT( "VREd.WorldMovementFogOpacity" ), 0.8f, TEXT( "How opaque the fog should be at the 'end distance' (0.0 - 1.0)" ) );
	static FAutoConsoleVariable WorldMovementFogStartDistance( TEXT( "VREd.WorldMovementFogStartDistance" ), 300.0f, TEXT( "How far away fog will start rendering while in world movement mode" ) );
	static FAutoConsoleVariable WorldMovementFogEndDistance( TEXT( "VREd.WorldMovementFogEndDistance" ), 800.0f, TEXT( "How far away fog will finish rendering while in world movement mode" ) );
	static FAutoConsoleVariable WorldMovementFogSkyboxDistance( TEXT( "VREd.WorldMovementFogSkyboxDistance" ), 20000.0f, TEXT( "Anything further than this distance will be completed fogged and not visible" ) );

	static FAutoConsoleVariable ScaleProgressBarLength( TEXT( "VREd.ScaleProgressBarLength" ), 50.0f, TEXT( "Length of the progressbar that appears when scaling" ) );
	static FAutoConsoleVariable ScaleProgressBarRadius( TEXT( "VREd.ScaleProgressBarRadius" ), 1.0f, TEXT( "Radius of the progressbar that appears when scaling" ) );

	static FAutoConsoleVariable ForceOculusMirrorMode( TEXT( "VREd.ForceOculusMirrorMode" ), 3, TEXT( "Which Oculus display mirroring mode to use (see MirrorWindowModeType in OculusRiftHMD.h)" ) );

	static FAutoConsoleVariable ShowMovementGrid( TEXT( "VREd.ShowMovementGrid" ), 1, TEXT( "Showing the ground movement grid" ) );
	static FAutoConsoleVariable ShowWorldMovementPostProcess( TEXT( "VREd.ShowWorldMovementPostProcess" ), 1, TEXT( "Showing the movement post processing" ) );
}

FEditorModeID FVREditorMode::VREditorModeID( "VREditor" );
bool FVREditorMode::bActuallyUsingVR = true;

// @todo vreditor: Hacky that we have to import these this way. (Plugin has them in a .cpp, not exported)

FVREditorMode::FVREditorMode()
	: bWantsToExitMode( false ),
	  bIsFullyInitialized( false ),
	  AppTimeModeEntered( FTimespan::Zero() ),
	  AvatarMeshActor( nullptr ),
	  HeadMeshComponent( nullptr ),
      FlashlightComponent( nullptr ),
	  bIsFlashlightOn( false ),
	  WorldMovementGridMeshComponent( nullptr ),
	  WorldMovementGridMID( nullptr ),
	  WorldMovementGridOpacity( 0.0f ),	// NOTE: Intentionally not quite zero so that we update the MIDs on the first frame
	  bIsDrawingWorldMovementPostProcess( false ),
	  WorldMovementPostProcessMaterial( nullptr ),
	  ScaleProgressMeshComponent( nullptr ),
	  CurrentScaleProgressMeshComponent( nullptr ),
	  UserScaleIndicatorText( nullptr ),
	  PostProcessComponent( nullptr ),
	  MotionControllerID( 0 ),	// @todo vreditor minor: We only support a single controller, and we assume the first controller are the motion controls
	  UISystem( nullptr ),
	  TeleporterSystem( nullptr ),
	  WorldInteraction( nullptr ),
	  MouseCursorInteractor( nullptr ),
	  LeftHandInteractor( nullptr ),
	  RightHandInteractor( nullptr ),
	  bFirstTick( true ),
	  bWasInWorldSpaceBeforeScaleMode( false )
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
	// @todo vreditor urgent: Turn on global editor hacks for VR Editor mode
	GEnableVREditorHacks = true;

	bIsFullyInitialized = false;
	bWantsToExitMode = false;

	AppTimeModeEntered = FTimespan::FromSeconds( FApp::GetCurrentTime() );

	WorldMovementGridOpacity = 0.0f;
	bIsDrawingWorldMovementPostProcess = false;

	// Take note of VREditor activation
	if( FEngineAnalytics::IsAvailable() )
	{
		FEngineAnalytics::GetProvider().RecordEvent( TEXT( "Editor.Usage.EnterVREditorMode" ) );
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

			// Enable selection outline right away
			VREditorViewportClient.EngineShowFlags.SetSelection(true);
			VREditorViewportClient.EngineShowFlags.SetSelectionOutline(true);
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

	// Setup sub systems
	{
		// Setup world interaction
		WorldInteraction = NewObject<UVREditorWorldInteraction>();
		WorldInteraction->SetOwner( this );
		WorldInteraction->Init( VREditorLevelViewportWeakPtr.Pin()->GetViewportClient().Get() );

		// Motion controllers
		{
			LeftHandInteractor = NewObject<UVREditorMotionControllerInteractor>( WorldInteraction );
			LeftHandInteractor->SetControllerHandSide( EControllerHand::Left );
			LeftHandInteractor->Init( this );
			WorldInteraction->AddInteractor( LeftHandInteractor );

			RightHandInteractor = NewObject<UVREditorMotionControllerInteractor>( WorldInteraction );
			RightHandInteractor->SetControllerHandSide( EControllerHand::Right );
			RightHandInteractor->Init( this );
			WorldInteraction->AddInteractor( RightHandInteractor );

			WorldInteraction->PairInteractors( LeftHandInteractor, RightHandInteractor );
		}

		if( !bActuallyUsingVR )
		{
			// Register an interactor for the mouse cursor
			MouseCursorInteractor = NewObject<UMouseCursorInteractor>( WorldInteraction );
			MouseCursorInteractor->Init();
			WorldInteraction->AddInteractor( MouseCursorInteractor );
		}

		// Setup the UI system
		UISystem = NewObject<UVREditorUISystem>();
		UISystem->SetOwner( this );
		UISystem->Init();

		// Setup teleporter
		TeleporterSystem = NewObject<UVREditorTeleporter>();
		TeleporterSystem->Init( this );

	}

	// Call parent implementation
	FEdMode::Enter();

	bFirstTick = true;
	bIsFullyInitialized = true;
}


void FVREditorMode::Exit()
{
	bIsFullyInitialized = false;

	FSlateApplication::Get().SetInputPreProcessor(false);

	if( WorldMovementPostProcessMaterial != nullptr )
	{
		WorldMovementPostProcessMaterial->MarkPendingKill();
		WorldMovementPostProcessMaterial = nullptr;
	}

	check( WorldInteraction->GetViewportClient() != nullptr );

	{
		DestroyTransientActor( AvatarMeshActor );
		AvatarMeshActor = nullptr;
		
		HeadMeshComponent = nullptr;
		FlashlightComponent = nullptr;
		WorldMovementGridMeshComponent = nullptr;
		WorldMovementGridMID = nullptr;
		PostProcessComponent = nullptr;
		ScaleProgressMeshComponent = nullptr;
		CurrentScaleProgressMeshComponent = nullptr;
		UserScaleIndicatorText = nullptr;
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
				VRViewportClient.SetViewLocation( GetHeadTransform().GetLocation() );
				
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

	// Call parent implementation
	FEdMode::Exit();

	// Kill subsystems
	if ( UISystem != nullptr )
	{
		UISystem->Shutdown();
		UISystem->MarkPendingKill();
		UISystem = nullptr;
	}

	if ( TeleporterSystem != nullptr )
	{
		TeleporterSystem->Shutdown();
		TeleporterSystem->MarkPendingKill();
		TeleporterSystem = nullptr;
	}

	if ( WorldInteraction != nullptr )
	{
		WorldInteraction->Shutdown();
		WorldInteraction->MarkPendingKill();
		WorldInteraction = nullptr;
	}

	// @todo vreditor urgent: Disable global editor hacks for VR Editor mode
	GEnableVREditorHacks = false;
}

void FVREditorMode::StartExitingVRMode( const EVREditorExitType InExitType /*= EVREditorExitType::To_Editor */ )
{
	ExitType = InExitType;
	bWantsToExitMode = true;
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

		//@todo VREditor: Hardcoded interactors
		LeftHandInteractor->SetupComponent( AvatarMeshActor );
		RightHandInteractor->SetupComponent( AvatarMeshActor );
		
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
	if ( ViewportClient != GetLevelViewportPossessedForVR().GetViewportClient().Get() || bWantsToExitMode )
	{
		return;
	}

	//Setting the initial position and rotation based on the editor viewport when going into VR mode
	if ( bFirstTick )
	{
		const FTransform RoomToWorld = GetRoomTransform();
		const FTransform WorldToRoom = RoomToWorld.Inverse();
		FTransform ViewportToWorld = FTransform( SavedEditorState.ViewRotation, SavedEditorState.ViewLocation );
		FTransform ViewportToRoom = ( ViewportToWorld * WorldToRoom );

		FTransform ViewportToRoomYaw = ViewportToRoom;
		ViewportToRoomYaw.SetRotation( FQuat( FRotator( 0.0f, ViewportToRoomYaw.GetRotation().Rotator().Yaw, 0.0f ) ) );

		FTransform HeadToRoomYaw = GetRoomSpaceHeadTransform();
		HeadToRoomYaw.SetRotation( FQuat( FRotator( 0.0f, HeadToRoomYaw.GetRotation().Rotator().Yaw, 0.0f ) ) );

		FTransform RoomToWorldYaw = RoomToWorld;
		RoomToWorldYaw.SetRotation( FQuat( FRotator( 0.0f, RoomToWorldYaw.GetRotation().Rotator().Yaw, 0.0f ) ) );

		FTransform ResultToWorld = ( HeadToRoomYaw.Inverse() * ViewportToRoomYaw ) * RoomToWorldYaw;
		SetRoomTransform( ResultToWorld );
	}

	if ( AvatarMeshActor == nullptr )
	{
		SpawnAvatarMeshActor();
	}

	TickHandle.Broadcast( DeltaTime );

	WorldInteraction->Tick( ViewportClient, DeltaTime );

	UISystem->Tick( ViewportClient, DeltaTime );

	// Update avatar meshes
	{
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

		// Update the user scale indicator //@todo
		{
			if ( ( LeftHandInteractor->GetDraggingMode() == EViewportInteractionDraggingMode::World && RightHandInteractor->GetDraggingMode() == EViewportInteractionDraggingMode::AssistingDrag ) ||
				 ( LeftHandInteractor->GetDraggingMode() == EViewportInteractionDraggingMode::AssistingDrag && RightHandInteractor->GetDraggingMode() == EViewportInteractionDraggingMode::World ) )
			{
				// Setting all components to be visible
				CurrentScaleProgressMeshComponent->SetVisibility( true );
				ScaleProgressMeshComponent->SetVisibility( true );
				UserScaleIndicatorText->SetVisibility( true );

				// Here we calculate the distance, direction and center of the two hands
				const FVector ScaleIndicatorStartPosition = LeftHandInteractor->GetTransform().GetLocation();
				const FVector ScaleIndicatorEndPosition = RightHandInteractor->GetTransform().GetLocation();
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
					const float CurrentProgressScale = (Scale * Scale) * (ProgressbarLength / ( WorldInteraction->GetMaxScale() / 100));
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

	// Updating the scale and intensity of the flashlight according to the world scale
	if (FlashlightComponent)
	{
		float CurrentFalloffExponent = FlashlightComponent->LightFalloffExponent;
		//@todo vreditor tweak
		float UpdatedFalloffExponent = FMath::Clamp(CurrentFalloffExponent / GetWorldScaleFactor(), 2.0f, 16.0f);
		FlashlightComponent->SetLightFalloffExponent(UpdatedFalloffExponent);
	}

	// Updating the opacity and visibility of the grid according to the controllers //@todo
	{
		if( VREd::ShowMovementGrid->GetInt() == 1)
		{
			// Show the grid full opacity when dragging or scaling
			float GoalOpacity = 0.f;
			if ( LeftHandInteractor->GetDraggingMode() == EViewportInteractionDraggingMode::World || RightHandInteractor->GetDraggingMode() == EViewportInteractionDraggingMode::World )
			{
				GoalOpacity = 1.0f;
			}
			else if ( ( LeftHandInteractor->GetLastDraggingMode() == EViewportInteractionDraggingMode::World && !LeftHandInteractor->GetDragTranslationVelocity().IsNearlyZero( VREd::GridMovementTolerance->GetFloat() ) ) )
			{
				GoalOpacity = FMath::Clamp( LeftHandInteractor->GetDragTranslationVelocity().Size() / VREd::GridFadeStartVelocity->GetFloat(), 0.0f, 1.0f );
			}
			else if( ( RightHandInteractor->GetLastDraggingMode() == EViewportInteractionDraggingMode::World && !RightHandInteractor->GetDragTranslationVelocity().IsNearlyZero( VREd::GridMovementTolerance->GetFloat() ) ) )
			{
				GoalOpacity = FMath::Clamp( RightHandInteractor->GetDragTranslationVelocity().Size() / VREd::GridFadeStartVelocity->GetFloat(), 0.0f, 1.0f );
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

	if ( Key == EKeys::Escape )
	{
		// User hit escape, so bail out of VR mode
		StartExitingVRMode();
	}
	else if( Key.IsMouseButton() )	// Input preprocessor cannot handle mouse buttons, so we'll route those the normal way
	{
		return WorldInteraction->HandleInputKey( Key, Event );
	}

	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
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

bool FVREditorMode::IsCompatibleWith(FEditorModeID OtherModeID) const
{
	// We are compatible with all other modes!
	return true;
}


void FVREditorMode::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject( AvatarMeshActor );
	Collector.AddReferencedObject( HeadMeshComponent );
	Collector.AddReferencedObject( FlashlightComponent );
	Collector.AddReferencedObject( WorldMovementGridMeshComponent );
	Collector.AddReferencedObject( PostProcessComponent );
	Collector.AddReferencedObject( WorldMovementGridMID );
	Collector.AddReferencedObject( WorldMovementPostProcessMaterial );
	Collector.AddReferencedObject( ScaleProgressMeshComponent );
	Collector.AddReferencedObject( CurrentScaleProgressMeshComponent );
	Collector.AddReferencedObject( UserScaleIndicatorText );	
	Collector.AddReferencedObject( UISystem );
	Collector.AddReferencedObject( WorldInteraction );
	Collector.AddReferencedObject( TeleporterSystem );
	Collector.AddReferencedObject( MouseCursorInteractor );
	Collector.AddReferencedObject( LeftHandInteractor );
	Collector.AddReferencedObject( RightHandInteractor );
}


void FVREditorMode::Render( const FSceneView* SceneView, FViewport* Viewport, FPrimitiveDrawInterface* PDI )
{
	//StopOldHapticEffects(); //@todo vreditor

	FEdMode::Render( SceneView, Viewport, PDI );

	if( bIsFullyInitialized )
	{
		// Let our subsystems render, too
		UISystem->Render( SceneView, Viewport, PDI );
	}
}

/************************************************************************/
/* IVREditorMode interface                                              */
/************************************************************************/

AActor* FVREditorMode::GetAvatarMeshActor()
{
	return AvatarMeshActor;
}

UWorld* FVREditorMode::GetWorld() const
{
	return WorldInteraction->GetViewportWorld();
}

FTransform FVREditorMode::GetRoomTransform() const
{
	return WorldInteraction->GetRoomTransform();
}

void FVREditorMode::SetRoomTransform( const FTransform& NewRoomTransform )
{
	WorldInteraction->SetRoomTransform( NewRoomTransform );
}

FTransform FVREditorMode::GetRoomSpaceHeadTransform() const
{
	return WorldInteraction->GetRoomSpaceHeadTransform();
}

FTransform FVREditorMode::GetHeadTransform() const
{
	return WorldInteraction->GetHeadTransform();
}

const UViewportWorldInteraction& FVREditorMode::GetWorldInteraction() const
{
	return *WorldInteraction;
}

UViewportWorldInteraction& FVREditorMode::GetWorldInteraction()
{
	return *WorldInteraction;
}

bool FVREditorMode::IsFullyInitialized() const
{
	return bIsFullyInitialized;
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

const SLevelViewport& FVREditorMode::GetLevelViewportPossessedForVR() const
{
	return *VREditorLevelViewportWeakPtr.Pin();
}

SLevelViewport& FVREditorMode::GetLevelViewportPossessedForVR()
{
	return *VREditorLevelViewportWeakPtr.Pin();
}


float FVREditorMode::GetWorldScaleFactor() const
{
	return WorldInteraction->GetViewportWorld()->GetWorldSettings()->WorldToMeters / 100.0f;
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
	FlashlightComponent = nullptr;

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

	PostProcessComponent = nullptr;
	ScaleProgressMeshComponent = nullptr;
	CurrentScaleProgressMeshComponent = nullptr;
	UserScaleIndicatorText = nullptr;

	if( UISystem != nullptr )
	{
		UISystem->CleanUpActorsBeforeMapChangeOrSimulate();
	}

	if( WorldInteraction != nullptr )
	{
		WorldInteraction->Shutdown();
	}
}

void FVREditorMode::ToggleFlashlight( UVREditorInteractor* Interactor )
{
	UVREditorMotionControllerInteractor* MotionControllerInteractor = Cast<UVREditorMotionControllerInteractor>( Interactor );
	if ( MotionControllerInteractor )
	{
		if ( FlashlightComponent == nullptr )
		{
			FlashlightComponent = NewObject<USpotLightComponent>( AvatarMeshActor );
			AvatarMeshActor->AddOwnedComponent( FlashlightComponent );
			FlashlightComponent->RegisterComponent();
			FlashlightComponent->SetMobility( EComponentMobility::Movable );
			FlashlightComponent->SetCastShadows( false );
			FlashlightComponent->bUseInverseSquaredFalloff = false;
			//@todo vreditor tweak
			FlashlightComponent->SetLightFalloffExponent( 8.0f );
			FlashlightComponent->SetIntensity( 20.0f );
			FlashlightComponent->SetOuterConeAngle( 25.0f );
			FlashlightComponent->SetInnerConeAngle( 25.0f );

		}

		const FAttachmentTransformRules AttachmentTransformRules = FAttachmentTransformRules( EAttachmentRule::KeepRelative, true );
		FlashlightComponent->AttachToComponent( MotionControllerInteractor->GetMotionControllerComponent(), AttachmentTransformRules );
		bIsFlashlightOn = !bIsFlashlightOn;
		FlashlightComponent->SetVisibility( bIsFlashlightOn );
	}
}

void FVREditorMode::CycleTransformGizmoHandleType()
{
	EGizmoHandleTypes NewGizmoType = (EGizmoHandleTypes)( (uint8)WorldInteraction->GetCurrentGizmoType() + 1 );
	
	if( NewGizmoType > EGizmoHandleTypes::Scale )
	{
		NewGizmoType = EGizmoHandleTypes::All;
	}

	// Set coordinate system to local if the next gizmo will be for non-uniform scaling 
	if ( NewGizmoType == EGizmoHandleTypes::Scale )
	{
		const ECoordSystem CurrentCoordSystem = WorldInteraction->GetTransformGizmoCoordinateSpace();
		if ( CurrentCoordSystem == COORD_World )
		{
			GLevelEditorModeTools().SetCoordSystem( COORD_Local );
			// Remember if coordinate system was in world space before scaling
			bWasInWorldSpaceBeforeScaleMode = true;
		}
		else if ( CurrentCoordSystem == COORD_Local )
		{
			bWasInWorldSpaceBeforeScaleMode = false;
		}
	} 
	else if ( WorldInteraction->GetCurrentGizmoType() == EGizmoHandleTypes::Scale && bWasInWorldSpaceBeforeScaleMode )
	{
		// Set the coordinate system to world space if the coordinate system was world before scaling
		WorldInteraction->SetTransformGizmoCoordinateSpace( COORD_World );
	}
	
	WorldInteraction->SetGizmoHandleType( NewGizmoType );
}

EHMDDeviceType::Type FVREditorMode::GetHMDDeviceType() const
{
	return GEngine->HMDDevice.IsValid() ? GEngine->HMDDevice->GetHMDDeviceType() : EHMDDeviceType::DT_SteamVR;
}

FLinearColor FVREditorMode::GetColor( const EColors Color ) const
{
	return Colors[ (int32)Color ];
}

bool FVREditorMode::IsHandAimingTowardsCapsule( UViewportInteractor* Interactor, const FTransform& CapsuleTransform, FVector CapsuleStart, const FVector CapsuleEnd, const float CapsuleRadius, const float MinDistanceToCapsule, const FVector CapsuleFrontDirection, const float MinDotForAimingAtCapsule ) const
{
	bool bIsAimingTowards = false;
	const float WorldScaleFactor = GetWorldScaleFactor();

	FVector LaserPointerStart, LaserPointerEnd;
	if( Interactor->GetLaserPointer( /* Out */ LaserPointerStart, /* Out */ LaserPointerEnd ) )
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

UVREditorInteractor* FVREditorMode::GetHandInteractor( const EControllerHand ControllerHand ) const 
{
	UVREditorInteractor* ResultInteractor = ControllerHand == EControllerHand::Left ? LeftHandInteractor : RightHandInteractor;
	check( ResultInteractor != nullptr );
	return ResultInteractor;
}

void FVREditorMode::StopOldHapticEffects()
{
	LeftHandInteractor->StopOldHapticEffects();
	RightHandInteractor->StopOldHapticEffects();
}

#undef LOCTEXT_NAMESPACE