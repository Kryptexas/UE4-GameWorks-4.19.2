// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VREditorInteractor.h"
#include "EngineUtils.h"
#include "ViewportWorldInteraction.h"

#include "VREditorMode.h"
#include "VREditorFloatingText.h"

#include "VREditorButton.h"
#include "VREditorFloatingUI.h"
#include "VREditorDockableWindow.h"

#define LOCTEXT_NAMESPACE "VREditor"

UVREditorInteractor::UVREditorInteractor( const FObjectInitializer& Initializer ) :
	Super( Initializer ),
	VRMode( nullptr ),
	bIsModifierPressed( false ),
	SelectAndMoveTriggerValue( 0.0f ),
	bHasUIInFront( false ),
	bHasUIOnForearm( false ),
	bIsClickingOnUI( false ),
	bIsRightClickingOnUI( false ),
	bIsHoveringOverUI( false ),
	UIScrollVelocity( 0.0f ),
	LastClickReleaseTime( 0.0f ),
	bWantHelpLabels( false ),
	HelpLabelShowOrHideStartTime( FTimespan::MinValue() )
{

}

UVREditorInteractor::~UVREditorInteractor()
{
	Shutdown();
}

void UVREditorInteractor::Init( UVREditorMode* InVRMode )
{
	VRMode = InVRMode;
	KeyToActionMap.Reset();
}

void UVREditorInteractor::Shutdown()
{
	for ( auto& KeyAndValue : HelpLabels )
	{
		AFloatingText* FloatingText = KeyAndValue.Value;
		UViewportWorldInteraction::DestroyTransientActor( GetVRMode().GetWorld(), FloatingText );
	}

	HelpLabels.Empty();	

	VRMode = nullptr;

	Super::Shutdown();
}

void UVREditorInteractor::Tick( const float DeltaTime )
{
	Super::Tick( DeltaTime );
}

FHitResult UVREditorInteractor::GetHitResultFromLaserPointer( TArray<AActor*>* OptionalListOfIgnoredActors /*= nullptr*/, const bool bIgnoreGizmos /*= false*/,
	TArray<UClass*>* ObjectsInFrontOfGizmo /*= nullptr */, const bool bEvenIfBlocked /*= false */, const float LaserLengthOverride /*= 0.0f */ )
{
	static TArray<AActor*> IgnoredActors;
	IgnoredActors.Reset();
	if ( OptionalListOfIgnoredActors == nullptr )
	{
		OptionalListOfIgnoredActors = &IgnoredActors;
	}

	OptionalListOfIgnoredActors->Add( GetVRMode().GetAvatarMeshActor() );
	
	// Ignore UI widgets too
	if ( GetDraggingMode() == EViewportInteractionDraggingMode::TransformablesAtLaserImpact )
	{
		for( TActorIterator<AVREditorFloatingUI> UIActorIt( WorldInteraction->GetWorld() ); UIActorIt; ++UIActorIt )
		{
			OptionalListOfIgnoredActors->Add( *UIActorIt );
		}
	}

	static TArray<UClass*> PriorityOverGizmoObjects;
	PriorityOverGizmoObjects.Reset();
	if ( ObjectsInFrontOfGizmo == nullptr )
	{
		ObjectsInFrontOfGizmo = &PriorityOverGizmoObjects;
	}

	ObjectsInFrontOfGizmo->Add( AVREditorButton::StaticClass() );
	ObjectsInFrontOfGizmo->Add( AVREditorDockableWindow::StaticClass() );
	ObjectsInFrontOfGizmo->Add( AVREditorFloatingUI::StaticClass() );

	return UViewportInteractor::GetHitResultFromLaserPointer( OptionalListOfIgnoredActors, bIgnoreGizmos, ObjectsInFrontOfGizmo, bEvenIfBlocked, LaserLengthOverride );
}

void UVREditorInteractor::ResetHoverState( const float DeltaTime )
{
	bIsHoveringOverUI = false;
}	

void UVREditorInteractor::OnStartDragging( const FVector& HitLocation, const bool bIsPlacingNewObjects )
{
	// If the user is holding down the modifier key, go ahead and duplicate the selection first.  Don't do this if we're
	// placing objects right now though.
	if ( bIsModifierPressed && !bIsPlacingNewObjects )
	{
		WorldInteraction->Duplicate();
	}
}

float UVREditorInteractor::GetSlideDelta()
{
	return 0.0f;
}

bool UVREditorInteractor::IsHoveringOverUI() const
{
	return bIsHoveringOverUI;
}

void UVREditorInteractor::SetHasUIInFront( const bool bInHasUIInFront )
{
	bHasUIInFront = bInHasUIInFront;
}

bool UVREditorInteractor::HasUIInFront() const
{
	return bHasUIInFront;
}

void UVREditorInteractor::SetHasUIOnForearm( const bool bInHasUIOnForearm )
{
	bHasUIOnForearm = bInHasUIOnForearm;
}

bool UVREditorInteractor::HasUIOnForearm() const
{
	return bHasUIOnForearm;
}

UWidgetComponent* UVREditorInteractor::GetHoveringOverWidgetComponent() const
{
	return InteractorData.HoveringOverWidgetComponent;
}

void UVREditorInteractor::SetHoveringOverWidgetComponent( UWidgetComponent* NewHoveringOverWidgetComponent )
{
	InteractorData.HoveringOverWidgetComponent = NewHoveringOverWidgetComponent;
}

bool UVREditorInteractor::IsModifierPressed() const
{
	return bIsModifierPressed;
}

void UVREditorInteractor::SetIsClickingOnUI( const bool bInIsClickingOnUI )
{
	bIsClickingOnUI = bInIsClickingOnUI;
}

bool UVREditorInteractor::IsClickingOnUI() const
{
	return bIsClickingOnUI;
}

void UVREditorInteractor::SetIsHoveringOverUI( const bool bInIsHoveringOverUI )
{
	bIsHoveringOverUI = bInIsHoveringOverUI;
}

void UVREditorInteractor::SetIsRightClickingOnUI( const bool bInIsRightClickingOnUI )
{
	bIsRightClickingOnUI = bInIsRightClickingOnUI;
}

bool UVREditorInteractor::IsRightClickingOnUI() const
{
	return bIsRightClickingOnUI;
}

void UVREditorInteractor::SetLastClickReleaseTime( const double InLastClickReleaseTime )
{
	LastClickReleaseTime = InLastClickReleaseTime;
}

double UVREditorInteractor::GetLastClickReleaseTime() const
{
	return LastClickReleaseTime;
}

void UVREditorInteractor::SetUIScrollVelocity( const float InUIScrollVelocity )
{
	UIScrollVelocity = InUIScrollVelocity;
}

float UVREditorInteractor::GetUIScrollVelocity() const
{
	return UIScrollVelocity;
}

float UVREditorInteractor::GetSelectAndMoveTriggerValue() const
{
	return SelectAndMoveTriggerValue;
}

bool UVREditorInteractor::GetIsLaserBlocked() const
{
	return bHasUIInFront;
}

#undef LOCTEXT_NAMESPACE
