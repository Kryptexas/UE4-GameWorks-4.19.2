// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorQuickMenu.h"
#include "VREditorUISystem.h"
#include "VREditorMode.h"
#include "VREditorWorldInteraction.h"
#include "VREditorFloatingUI.h"
#include "VREditorTransformGizmo.h"
#include "SLevelViewport.h"	// For Simulate toggle


#define LOCTEXT_NAMESPACE "VREditorQuickMenu"

UVREditorQuickMenu::UVREditorQuickMenu( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}


void UVREditorQuickMenu::OnExitVRButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().StartExitingVRMode();
}


void UVREditorQuickMenu::OnDeleteButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().GetWorldInteraction().DeleteSelectedObjects();
}


void UVREditorQuickMenu::OnUndoButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Undo();
}


void UVREditorQuickMenu::OnRedoButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Redo();
}


void UVREditorQuickMenu::OnCopyButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Copy();
}


void UVREditorQuickMenu::OnPasteButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Paste();
}


void UVREditorQuickMenu::OnTranslationSnapButtonClicked( const bool bIsChecked )
{
	GUnrealEd->Exec( GetWorld(), *FString::Printf( TEXT( "MODE GRID=%d" ), bIsChecked ? 1 : 0 ) );
}


bool UVREditorQuickMenu::IsTranslationSnapEnabled() const
{
	return GetDefault<ULevelEditorViewportSettings>()->GridEnabled;
}


void UVREditorQuickMenu::OnTranslationSnapSizeButtonClicked()
{
	const TArray<float>& GridSizes = GEditor->GetCurrentPositionGridArray();
	const int32 CurrentGridSize = GetDefault<ULevelEditorViewportSettings>()->CurrentPosGridSize;

	int32 NewGridSize = CurrentGridSize + 1;
	if( NewGridSize >= GridSizes.Num() )
	{
		NewGridSize = 0;
	}

	GEditor->SetGridSize( NewGridSize );
}


FText UVREditorQuickMenu::GetTranslationSnapSizeText() const
{
	return FText::AsNumber( GEditor->GetGridSize() );
}


void UVREditorQuickMenu::OnRotationSnapSizeButtonClicked()
{
	const TArray<float>& GridSizes = GEditor->GetCurrentRotationGridArray();
	const int32 CurrentGridSize = GetDefault<ULevelEditorViewportSettings>()->CurrentRotGridSize;
	const ERotationGridMode CurrentGridMode = GetDefault<ULevelEditorViewportSettings>()->CurrentRotGridMode;

	int32 NewGridSize = CurrentGridSize + 1;
	if( NewGridSize >= GridSizes.Num() )
	{
		NewGridSize = 0;
	}

	GEditor->SetRotGridSize( NewGridSize, CurrentGridMode );
}


FText UVREditorQuickMenu::GetRotationSnapSizeText() const
{
	return FText::AsNumber( GEditor->GetRotGridSize().Yaw );
}


void UVREditorQuickMenu::OnScaleSnapSizeButtonClicked()
{
	const ULevelEditorViewportSettings* ViewportSettings = GetMutableDefault<ULevelEditorViewportSettings>();
	const int32 NumGridSizes = ViewportSettings->ScalingGridSizes.Num();

	const int32 CurrentGridSize = GetDefault<ULevelEditorViewportSettings>()->CurrentScalingGridSize;

	int32 NewGridSize = CurrentGridSize + 1;
	if( NewGridSize >= NumGridSizes )
	{
		NewGridSize = 0;
	}

	GEditor->SetScaleGridSize( NewGridSize );
}


FText UVREditorQuickMenu::GetScaleSnapSizeText() const
{
	return FText::AsNumber( GEditor->GetScaleGridSize() );
}


void UVREditorQuickMenu::OnRotationSnapButtonClicked( const bool bIsChecked )
{
	GUnrealEd->Exec( GetWorld(), *FString::Printf( TEXT( "MODE ROTGRID=%d" ), bIsChecked ? 1 : 0 ) );
}


bool UVREditorQuickMenu::IsRotationSnapEnabled() const
{
	return GetDefault<ULevelEditorViewportSettings>()->RotGridEnabled;
}


void UVREditorQuickMenu::OnScaleSnapButtonClicked( const bool bIsChecked )
{
	GUnrealEd->Exec( GetWorld(), *FString::Printf( TEXT( "MODE SCALEGRID=%d" ), bIsChecked ? 1 : 0 ) );
}


bool UVREditorQuickMenu::IsScaleSnapEnabled() const
{
	return GetDefault<ULevelEditorViewportSettings>()->SnapScaleEnabled;
}


void UVREditorQuickMenu::OnGizmoCoordinateSystemButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().CycleTransformGizmoCoordinateSpace();
}


FText UVREditorQuickMenu::GetGizmoCoordinateSystemText() const
{
	const ECoordSystem CurrentCoordSystem = GetOwner()->GetOwner().GetOwner().GetTransformGizmoCoordinateSpace();
	return CurrentCoordSystem == COORD_World ? LOCTEXT( "WorldCoordinateSystem", "World Space" ) : LOCTEXT( "LocalCoordinateSystem", "Local Space" );
}


void UVREditorQuickMenu::OnGizmoModeButtonClicked()
{
	const EGizmoHandleTypes CurrentGizmoMode = GetOwner()->GetOwner().GetOwner().GetCurrentGizmoType();
	GetOwner()->GetOwner().GetOwner().CycleTransformGizmoHandleType();
}


FText UVREditorQuickMenu::GetGizmoModeText() const
{
	const EGizmoHandleTypes CurrentGizmoType = GetOwner()->GetOwner().GetOwner().GetCurrentGizmoType();
	FText GizmoTypeText;
	switch( CurrentGizmoType )
	{
		case EGizmoHandleTypes::All:
			GizmoTypeText = LOCTEXT( "AllGizmoType", "Universal" );
			break;

		case EGizmoHandleTypes::Translate:
			GizmoTypeText = LOCTEXT( "TranslateGizmoType", "Translate" );
			break;

		case EGizmoHandleTypes::Rotate:
			GizmoTypeText = LOCTEXT( "RotationGizmoType", "Rotation" );
			break;

		case EGizmoHandleTypes::Scale:
			GizmoTypeText = LOCTEXT( "ScaleGizmoType", "Scale" );
			break;

		default:
			check( 0 );	// Unrecognized type
			break;
	}

	return GizmoTypeText;
}


void UVREditorQuickMenu::OnSimulateButtonClicked( const bool bIsChecked )
{
	if( bIsChecked != GEditor->bIsSimulatingInEditor )
	{
		if( GEditor->PlayWorld == nullptr && !GEditor->bIsSimulatingInEditor )
		{
			const bool bSimulateInEditor = false;
			const bool bAtPlayerStart = true;

			// If there is an active level view port, play the game in it.
			TSharedRef<ILevelViewport> LevelViewport = StaticCastSharedRef<SLevelViewport>( GetOwner()->GetOwner().GetOwner().GetLevelViewportPossessedForVR().AsShared() );
			GUnrealEd->RequestPlaySession(
				bAtPlayerStart,
				LevelViewport,
				bSimulateInEditor );
		}
		else if( GEditor->bIsSimulatingInEditor )
		{
			GEditor->RequestEndPlayMap();
		}
		else if( GUnrealEd->PlayWorld != nullptr )
		{
			// There is already a play world active which means simulate in editor is happening
			// Toggle to pie 
			GUnrealEd->RequestToggleBetweenPIEandSIE();
		}
	}
}


bool UVREditorQuickMenu::IsSimulatingEnabled() const
{
	return GEditor->bIsSimulateInEditorQueued || GEditor->bIsSimulatingInEditor;
}


void UVREditorQuickMenu::OnContentBrowserButtonClicked( const bool bIsChecked )
{
	GetOwner()->GetOwner().TogglePanelVisibility( FVREditorUISystem::EEditorUIPanel::ContentBrowser );
}


bool UVREditorQuickMenu::IsContentBrowserVisible() const
{
	return ( GetOwner()->GetOwner().IsShowingEditorUIPanel( FVREditorUISystem::EEditorUIPanel::ContentBrowser ) );
}


void UVREditorQuickMenu::OnWorldOutlinerButtonClicked( const bool bIsChecked )
{
	GetOwner()->GetOwner().TogglePanelVisibility( FVREditorUISystem::EEditorUIPanel::WorldOutliner );
}


bool UVREditorQuickMenu::IsWorldOutlinerVisible() const
{
	return ( GetOwner()->GetOwner().IsShowingEditorUIPanel( FVREditorUISystem::EEditorUIPanel::WorldOutliner ) );
}


void UVREditorQuickMenu::OnActorDetailsButtonClicked( const bool bIsChecked )
{
	GetOwner()->GetOwner().TogglePanelVisibility( FVREditorUISystem::EEditorUIPanel::ActorDetails );
}


bool UVREditorQuickMenu::IsActorDetailsVisible() const
{
	return ( GetOwner()->GetOwner().IsShowingEditorUIPanel( FVREditorUISystem::EEditorUIPanel::ActorDetails ) );
}


void UVREditorQuickMenu::OnModesButtonClicked( const bool bIsChecked )
{
	GetOwner()->GetOwner().TogglePanelVisibility( FVREditorUISystem::EEditorUIPanel::Modes );
}


bool UVREditorQuickMenu::IsModesVisible() const
{
	return ( GetOwner()->GetOwner().IsShowingEditorUIPanel( FVREditorUISystem::EEditorUIPanel::Modes ) );
}


void UVREditorQuickMenu::OnGameModeButtonClicked( const bool bIsChecked )
{
	FEditorViewportClient* ViewportClient = GetOwner()->GetOwner().GetOwner().GetLevelViewportPossessedForVR().GetViewportClient().Get();
	const bool bIsInGameView = ViewportClient->IsInGameView();
	if( bIsChecked != bIsInGameView )
	{
		ViewportClient->SetGameView( bIsChecked );
	}
}


bool UVREditorQuickMenu::IsGameModeEnabled() const
{
	const FEditorViewportClient* ViewportClient = GetOwner()->GetOwner().GetOwner().GetLevelViewportPossessedForVR().GetViewportClient().Get();
	return ViewportClient->IsInGameView();
}


void UVREditorQuickMenu::OnTutorialButtonClicked( const bool bIsChecked )
{
	GetOwner()->GetOwner().TogglePanelVisibility( FVREditorUISystem::EEditorUIPanel::Tutorial );
}

bool UVREditorQuickMenu::IsTutorialVisible() const
{
	return ( GetOwner()->GetOwner().IsShowingEditorUIPanel( FVREditorUISystem::EEditorUIPanel::Tutorial ) );
}


void UVREditorQuickMenu::OnAssetEditorButtonClicked( const bool bIsChecked )
{
	GetOwner()->GetOwner().TogglePanelVisibility( FVREditorUISystem::EEditorUIPanel::AssetEditor );
	
}

bool UVREditorQuickMenu::IsAssetEditorVisible() const
{
	return ( GetOwner()->GetOwner().IsShowingEditorUIPanel( FVREditorUISystem::EEditorUIPanel::AssetEditor ) );
}

#undef LOCTEXT_NAMESPACE