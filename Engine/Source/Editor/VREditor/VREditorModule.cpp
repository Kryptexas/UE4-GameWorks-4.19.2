// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "IVREditorModule.h"
#include "EditorModeRegistry.h"
#include "VREditorMode.h"
#include "LevelEditor.h"
#include "HeadMountedDisplay.h"

class FVREditorModule : public IVREditorModule, public FTickableEditorObject
{
public:
	FVREditorModule() :
		bEnableVRRequest( false ),
		bPlayStartedFromVREditor( false ),
		LastWorldToMeters( 100 ),
		SavedWorldToMeters( 0 )
	{
	}

	// FModuleInterface overrides
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual void PostLoadCallback() override;
	virtual bool SupportsDynamicReloading() override
	{
		return true;
	}

	// IVREditorModule overrides
	virtual FEditorModeID GetVREditorModeID() const override
	{
		return FVREditorMode::GetVREditorModeID();
	}
	virtual bool IsVREditorEnabled() const override;
	virtual bool IsVREditorAvailable() const override;
	virtual void EnableVREditor( const bool bEnable, const bool bForceWithoutHMD ) override;

	// FTickableEditorObject overrides
	virtual void Tick( float DeltaTime ) override;
	virtual bool IsTickable() const override
	{
		return true;
	}
	virtual TStatId GetStatId() const override
	{
		return TStatId();
	}


	static void ToggleForceVRMode();

private:


	/** Sets the world settings directly and prevents the last GNewWorldToMetersScale from the VREditor to reset it by setting it 0 */
	void SetDirectWorldToMeters( const float NewWorldToMeters );

	/** If the VR Editor mode needs to be enabled next tick */
	bool bEnableVRRequest;

	/** If PIE session started from within the VR Editor, so we can go back to the VR Editor when closing PIE with the motion controllers */
	bool bPlayStartedFromVREditor;

	/** WorldToMeters from last VR Editor session */
	float LastWorldToMeters;

	/** Saved last world to meters scale from last VR Editor session so we can restore it when entering the VR Editor when PIE starter from VR Editor */
	float SavedWorldToMeters;
};

namespace VREd
{
	static FAutoConsoleCommand ForceVRMode( TEXT( "VREd.ForceVRMode" ), TEXT( "Toggles VREditorMode, even if not in immersive VR" ), FConsoleCommandDelegate::CreateStatic( &FVREditorModule::ToggleForceVRMode ) );
}


void FVREditorModule::StartupModule()
{
	FEditorModeRegistry::Get().RegisterMode<FVREditorMode>( FVREditorMode::GetVREditorModeID(), NSLOCTEXT( "VREditor", "ModeName", "VR" ) );
}


void FVREditorModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode( FVREditorMode::GetVREditorModeID() );
}


void FVREditorModule::PostLoadCallback()
{
	GLevelEditorModeTools().ActivateDefaultMode();
}


bool FVREditorModule::IsVREditorEnabled() const
{
	return GLevelEditorModeTools().IsModeActive( FVREditorMode::GetVREditorModeID() );
}


bool FVREditorModule::IsVREditorAvailable() const
{
	const bool bHasHMDDevice = GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHMDEnabled();
	return bHasHMDDevice;
}


void FVREditorModule::EnableVREditor( const bool bEnable, const bool bForceWithoutHMD )
{
	// @todo vreditor: Need to be careful about Play in VR interfering with this.  While in Editor VR, PIE should
	// work differently (in VR play experience).  And when outside of Editor VR, this feature should be disabled in the UI

	TSharedPtr< ILevelEditor > LevelEditor;
	{
		FLevelEditorModule* LevelEditorModulePtr = FModuleManager::GetModulePtr<FLevelEditorModule>( "LevelEditor" );
		if( LevelEditorModulePtr != nullptr )
		{
			// NOTE: Can be nullptr, especially during shutdown!
			LevelEditor = LevelEditorModulePtr->GetFirstLevelEditor();
		}
	}

	if( bEnable != IsVREditorEnabled() )
	{
		if( bEnable && ( IsVREditorAvailable() || bForceWithoutHMD ) && LevelEditor.IsValid() )
		{
			FVREditorMode::SetActuallyUsingVR( !bForceWithoutHMD );

			SavedWorldToMeters = GWorld->GetWorldSettings()->WorldToMeters;

			// Set the WorldToMeters scale when we stopped playing PIE that was started by the VR Editor to that VR Editor sessions latest WorldToMeters
			if ( bPlayStartedFromVREditor )
			{
				SetDirectWorldToMeters( LastWorldToMeters );
				bPlayStartedFromVREditor = false;
			}
		
			// Activate the mode right away.  We expect it to stay active forever!
			GLevelEditorModeTools().AddDefaultMode( FVREditorMode::GetVREditorModeID() );
			GLevelEditorModeTools().ActivateMode( FVREditorMode::GetVREditorModeID() );

		}
		else if( !bEnable )
		{ 
			if( LevelEditor.IsValid() )
			{
				// Shut off VREditor mode
				GLevelEditorModeTools().RemoveDefaultMode( FVREditorMode::GetVREditorModeID() );
				GLevelEditorModeTools().DeactivateMode( FVREditorMode::GetVREditorModeID() );
			}

			SetDirectWorldToMeters( SavedWorldToMeters );
			SavedWorldToMeters = 0.0f;
		}
	}
}


void FVREditorModule::SetDirectWorldToMeters( const float NewWorldToMeters )
{
	GWorld->GetWorldSettings()->WorldToMeters = NewWorldToMeters;
	ENGINE_API extern float GNewWorldToMetersScale;
	GNewWorldToMetersScale = 0.0f;
}

void FVREditorModule::ToggleForceVRMode()
{
	const bool bForceWithoutHMD = true;
	FVREditorModule& Self = FModuleManager::GetModuleChecked< FVREditorModule >( TEXT( "VREditor" ) );
	Self.EnableVREditor( !Self.IsVREditorEnabled(), bForceWithoutHMD );
}


void FVREditorModule::Tick( float DeltaTime )
{
	if( IsVREditorEnabled() )
	{
		FVREditorMode* VREditorMode = static_cast<FVREditorMode*>( GLevelEditorModeTools().FindMode( FVREditorMode::GetVREditorModeID() ) );
		check( VREditorMode != nullptr );

		if( VREditorMode->WantsToExitMode() )
		{
			//Store the current WorldToMeters before exiting the mode
			LastWorldToMeters = GWorld->GetWorldSettings()->WorldToMeters;

			EnableVREditor( false, false );

			// Start the session if that was the reason to stop the VR Editor mode
			if ( VREditorMode->GetExitType() == EVREditorExitType::PIE_VR )
			{
				const bool bHMDIsReady = ( GEngine && GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHMDConnected() );

				const FVector* StartLoc = NULL;
				const FRotator* StartRot = NULL;
				GEditor->RequestPlaySession( true, NULL, false, StartLoc, StartRot, -1, false, bHMDIsReady, false );
				bPlayStartedFromVREditor = true;
			}
		}
	}
	// Only check for input if we started this play session from the VR Editor
	else if( bPlayStartedFromVREditor && GEditor->PlayWorld && !bEnableVRRequest )
	{
		// Shutdown PIE if we came from the VR Editor and we are not already requesting to start the VR Editor and when any of the players is holding down the required input
		const float ShutDownInputKeyTime = 0.5f;
		TArray<APlayerController*> PlayerControllers;
		GEngine->GetAllLocalPlayerControllers( PlayerControllers );
		for ( APlayerController* PlayerController : PlayerControllers )
		{
			if ( ( PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Left_Grip1 ) > ShutDownInputKeyTime || PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Left_Grip2 ) > ShutDownInputKeyTime ) &&
				( PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Right_Grip1 ) > ShutDownInputKeyTime || PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Right_Grip2 ) > ShutDownInputKeyTime ) &&
				PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Right_Trigger ) > ShutDownInputKeyTime &&
				PlayerController->GetInputKeyTimeDown( EKeys::MotionController_Left_Trigger ) > ShutDownInputKeyTime )
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
		if ( bEnableVRRequest )
		{
			EnableVREditor( true, false );
			bEnableVRRequest = false;
		}
	}
}


IMPLEMENT_MODULE( FVREditorModule, VREditor )
