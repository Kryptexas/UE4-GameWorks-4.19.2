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
	FVREditorModule()
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
	virtual void EnableVREditor( const bool bEnable ) override;

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

	void EnableVREditor( const bool bEnable, const bool bForceWithoutHMD );
	void SetupMotionControls();
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
		}
	}
}


void FVREditorModule::EnableVREditor( const bool bEnable )
{
	const bool bForceWithoutHMD = false;
	EnableVREditor( bEnable, bForceWithoutHMD );
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
			EnableVREditor( false );
		}
	}
}


IMPLEMENT_MODULE( FVREditorModule, VREditor )
