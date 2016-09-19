#include "VREditorModule.h"
#include "VREditorModeManager.h"
#include "VREditorMode.h"
#include "VREditorMode.h"

#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "HeadMountedDisplay.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"	// For EHMDWornState::Type
#include "EditorWorldManager.h"

FVREditorModeManager::FVREditorModeManager() :
	CurrentVREditorMode( nullptr ),
	PreviousVREditorMode( nullptr ),
	bEnableVRRequest( false ),
	bPlayStartedFromVREditor( false ),
	LastWorldToMeters( 100 ),
	HMDWornState( EHMDWornState::Unknown ),
	TimeSinceHMDChecked( 0.0f )
{
	// Tell the level editor we want to be notified when selection changes
	FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>( "LevelEditor" );
	LevelEditor.OnMapChanged().AddRaw( this, &FVREditorModeManager::OnMapChanged );
}

FVREditorModeManager::~FVREditorModeManager()
{
	FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>( "LevelEditor" );
	if( LevelEditor != nullptr )
	{
		LevelEditor->OnMapChanged().RemoveAll( this );
	}

	CurrentVREditorMode = nullptr;
	PreviousVREditorMode = nullptr;
}

void FVREditorModeManager::Tick( const float DeltaTime )
{
	//@todo vreditor: Make the timer a configurable variable and/or change the polling system to an event-based one.
	TimeSinceHMDChecked += DeltaTime;

	// You can only auto-enter VR if the setting is enabled. Other criteria are that the VR Editor is enabled in experimental settings, that you are not in PIE, and that the editor is foreground.
	bool bCanAutoEnterVR = ( GetDefault<UEditorExperimentalSettings>()->bEnableAutoVREditMode ) && ( GetDefault<UEditorExperimentalSettings>()->bEnableVREditing ) && !( GEditor->PlayWorld ) && FPlatformProcess::IsThisApplicationForeground();

	if( GEngine != nullptr && GEngine->HMDDevice.IsValid() )
	{
		// Only check whether you are wearing the HMD every second, if you are allowed to auto-enter VR, and if your HMD state has changed since the last check. 
		if( ( TimeSinceHMDChecked >= 1.0f ) && bCanAutoEnterVR && ( HMDWornState != GEngine->HMDDevice->GetHMDWornState() ) )
		{
			TimeSinceHMDChecked = 0.0f;
			HMDWornState = GEngine->HMDDevice->GetHMDWornState();
			if( HMDWornState == EHMDWornState::Worn )
			{
				EnableVREditor( true, false );
			}
			else if( HMDWornState == EHMDWornState::NotWorn )
			{
				EnableVREditor( false, false );
			}
		}
	}

	if( IsVREditorActive() )
	{
		UVREditorMode* VREditorMode = CurrentVREditorMode;
		if( VREditorMode->WantsToExitMode() )
		{
			CloseVREditor();

			// Start the session if that was the reason to stop the VR Editor mode
			if( VREditorMode->GetExitType() == EVREditorExitType::PIE_VR  || 
				VREditorMode->GetExitType() == EVREditorExitType::SIE_VR )
			{
				const bool bHMDIsReady = ( GEngine && GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHMDConnected() );

				const FVector* StartLoc = NULL;
				const FRotator* StartRot = NULL;

				if( VREditorMode->GetExitType() == EVREditorExitType::PIE_VR )
				{
					GEditor->RequestPlaySession( true, NULL, false, StartLoc, StartRot, -1, false, bHMDIsReady, false );
				}
				else if( VREditorMode->GetExitType() == EVREditorExitType::SIE_VR )
				{
					//@todo VREditor: Where request should be to start the VR Editor in simulate mode
					//TSharedPtr<ILevelViewport> LevelViewport = MakeShareable( &VREditorMode->GetLevelViewportPossessedForVR() ); //@todo VREditor: Must be above CloseVREditor
					//GEditor->RequestPlaySession( true, LevelViewport, true /*bSimulateInEditor*/, StartLoc, StartRot, -1, false, bHMDIsReady, false );
				}
				bPlayStartedFromVREditor = true;
			}
		}
	}
	// Only check for input if we started this play session from the VR Editor
	else if( bPlayStartedFromVREditor && GEditor->PlayWorld && !bEnableVRRequest )
	{
		// Shutdown PIE if we came from the VR Editor and we are not already requesting to start the VR Editor and when any of the players is holding down the required input
		const float ShutDownInputKeyTime = 1.0f;
		TArray<APlayerController*> PlayerControllers;
		GEngine->GetAllLocalPlayerControllers(PlayerControllers);
		for(APlayerController* PlayerController : PlayerControllers)
		{
			if((PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Left_Grip1 ) > ShutDownInputKeyTime || PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Left_Grip2 ) > ShutDownInputKeyTime ) &&
				(PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Right_Grip1 ) > ShutDownInputKeyTime || PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Right_Grip2 ) > ShutDownInputKeyTime ) &&
				PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Right_Trigger ) > ShutDownInputKeyTime &&
				PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Left_Trigger ) > ShutDownInputKeyTime)
			{
				GEditor->RequestEndPlayMap();
				bEnableVRRequest = true;
				break;
			}
		}
	}
	else
	{
		// Start the VR Editor mode
		if( bEnableVRRequest )
		{
			EnableVREditor( true, false );
			bEnableVRRequest = false;
		}
	}
}

void FVREditorModeManager::EnableVREditor( const bool bEnable, const bool bForceWithoutHMD )
{
	// Don't do anything when the current VR Editor is already in the requested state
	if( bEnable != IsVREditorActive() )
	{
		if( bEnable && ( IsVREditorAvailable() || bForceWithoutHMD ) )
		{
			StartVREditorMode( bForceWithoutHMD );
		}
		else if( !bEnable )
		{
			CloseVREditor();
		}
	}
}

bool FVREditorModeManager::IsVREditorActive() const
{
	return CurrentVREditorMode != nullptr && CurrentVREditorMode->IsActive();
}


bool FVREditorModeManager::IsVREditorAvailable() const
{
	const bool bHasHMDDevice = GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHMDEnabled();
	return bHasHMDDevice;
}


void FVREditorModeManager::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject( CurrentVREditorMode );
}

void FVREditorModeManager::StartVREditorMode( const bool bForceWithoutHMD )
{
	// Set the WorldToMeters scale when we stopped playing PIE that was started by the VR Editor to that VR Editor sessions latest WorldToMeters
	if( bPlayStartedFromVREditor )
	{
		SetDirectWorldToMeters( LastWorldToMeters );
		bPlayStartedFromVREditor = false;
	}

	TSharedPtr<FEditorWorldWrapper> EditorWorld = GEditor->GetEditorWorldManager()->GetEditorWorldWrapper( GWorld );
	UVREditorMode* ModeFromWorld = EditorWorld->GetVREditorMode();

	CurrentVREditorMode = ModeFromWorld;
	CurrentVREditorMode->SetActuallyUsingVR( !bForceWithoutHMD );
	CurrentVREditorMode->Enter();
}

void FVREditorModeManager::CloseVREditor()
{
	//Store the current WorldToMeters before exiting the mode
	LastWorldToMeters = GWorld->GetWorldSettings()->WorldToMeters;

	if( CurrentVREditorMode != nullptr )
	{
		CurrentVREditorMode->Exit();
		PreviousVREditorMode = CurrentVREditorMode;
		CurrentVREditorMode = nullptr;
	}
}

void FVREditorModeManager::SetDirectWorldToMeters( const float NewWorldToMeters )
{
	GWorld->GetWorldSettings()->WorldToMeters = NewWorldToMeters; //@todo VREditor: Do not use GWorld
	ENGINE_API extern float GNewWorldToMetersScale;
	GNewWorldToMetersScale = 0.0f;
}

void FVREditorModeManager::OnMapChanged( UWorld* World, EMapChangeType MapChangeType )
{
	if( CurrentVREditorMode && CurrentVREditorMode->IsActive() )
	{
		CloseVREditor();
		bEnableVRRequest = true;
	}
	CurrentVREditorMode = nullptr;
}