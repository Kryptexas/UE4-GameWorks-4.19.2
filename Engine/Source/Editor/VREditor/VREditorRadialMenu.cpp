// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorRadialMenu.h"
#include "VREditorRadialMenuItem.h"
#include "VREditorFloatingUI.h"
#include "VREditorUISystem.h"
#include "VREditorMode.h"
#include "VREditorWorldInteraction.h"
#include "VREditorTransformGizmo.h"
#include "VirtualHand.h"

#define LOCTEXT_NAMESPACE "VREditorRadialMenu"

namespace VREd
{
	static FAutoConsoleVariable RadialMenuSelectionRadius( TEXT( "VREd.RadialMenuSelectionRadius" ), 0.2f, TEXT( "The radius that is used for selecting buttons on the radial menu" ) );
}

UVREditorRadialMenu::UVREditorRadialMenu( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer ),
	TopItem( nullptr ),
	TopRightItem(nullptr),
	RightItem(nullptr),
	BottomRightItem(nullptr),
	LeftBottomItem(nullptr),
	LeftItem(nullptr),
	TopLeftItem(nullptr),
	CurrentItem(nullptr),
	PreviousItem(nullptr)
{
}


void UVREditorRadialMenu::Update( const FVirtualHand& Hand )
{
	TrackpadPosition = FVector2D( Hand.TrackpadPosition.X, -Hand.TrackpadPosition.Y );
	
	if( !GetOwner()->bHidden )
	{
		// Check if position is from the center
		if( !IsInMenuRadius() )
		{
			float Angle = FRotator::NormalizeAxis( FMath::RadiansToDegrees(FMath::Atan2( Hand.TrackpadPosition.X,  Hand.TrackpadPosition.Y ) ) );
			TrackpadAngle = Angle;
			if( Angle < 0)
			{
				Angle = 360 + Angle;
			}

			// @todo vreditor : really hardcoded! This should be calculated from the amount of widgets in the current group
			if( Angle > 337.5f && Angle <= 360|| Angle > 0 && Angle <= 22.5f )
			{
				CurrentItem = TopItem;
			}
			else if( Angle > 22.5f && Angle <= 67.5f)
			{
				CurrentItem = TopRightItem;
			}
			else if (Angle > 67.5f && Angle <= 112.5f)
			{
				CurrentItem = RightItem;
			}
			else if (Angle > 112.5f && Angle <= 157.5f)
			{
				CurrentItem = BottomRightItem;
			}
			else if (Angle > 202.5f && Angle <= 247.5f)
			{
				CurrentItem = LeftBottomItem;
			}
			else if (Angle > 247.5f && Angle <= 292.5f)
			{
				CurrentItem = LeftItem;
			}
			else if (Angle > 292.5f && Angle <= 337.5f)
			{
				CurrentItem = TopLeftItem;
			}

			// Update the visuals
			if (CurrentItem != PreviousItem)
			{
				if( CurrentItem )
				{
					CurrentItem->OnEnterHover();
				}

				if ( PreviousItem )
				{
					PreviousItem->OnLeaveHover();
				}
			}
		
			PreviousItem = CurrentItem;
		}
		else
		{
			if (CurrentItem)
			{
				CurrentItem->OnLeaveHover();
				CurrentItem = nullptr;
			}
		}
	}
}

void UVREditorRadialMenu::SetButtons(UVREditorRadialMenuItem* InitTopItem, UVREditorRadialMenuItem* InitTopRightItem, UVREditorRadialMenuItem* InitRightItem, UVREditorRadialMenuItem* InitBottomRightItem, UVREditorRadialMenuItem* InitLeftBottomItem, UVREditorRadialMenuItem* InitLeftItem, UVREditorRadialMenuItem* InitTopLeftItem)
{
	TopItem = InitTopItem;
	TopItem->OnPressedDelegate.AddUObject( this, &UVREditorRadialMenu::OnGizmoCycle );
	UpdateGizmoTypeLabel();

	TopRightItem = InitTopRightItem;
	TopRightItem->OnPressedDelegate.AddUObject(this, &UVREditorRadialMenu::OnCopyButtonClicked);
	TopRightItem->SetLabel(FString("Copy"));

	RightItem = InitRightItem;
	RightItem->OnPressedDelegate.AddUObject(this, &UVREditorRadialMenu::OnPasteButtonClicked);
	RightItem->SetLabel(FString("Paste"));

	BottomRightItem = InitBottomRightItem;
	BottomRightItem->OnPressedDelegate.AddUObject(this, &UVREditorRadialMenu::OnRedoButtonClicked);
	BottomRightItem->SetLabel(FString("Redo"));

	LeftBottomItem = InitLeftBottomItem;
	LeftBottomItem->OnPressedDelegate.AddUObject(this, &UVREditorRadialMenu::OnUndoButtonClicked);
	LeftBottomItem->SetLabel(FString("Undo"));

	LeftItem = InitLeftItem;
	LeftItem->OnPressedDelegate.AddUObject(this, &UVREditorRadialMenu::OnDuplicateButtonClicked);
	LeftItem->SetLabel(FString("Duplicate"));

	TopLeftItem = InitTopLeftItem;
	TopLeftItem->OnPressedDelegate.AddUObject(this, &UVREditorRadialMenu::OnDeleteButtonClicked);
	TopLeftItem->SetLabel(FString("Delete"));
}

void UVREditorRadialMenu::ResetItem()
{
	if (PreviousItem)
	{
		PreviousItem->OnLeaveHover();
	}

	if ( CurrentItem )
	{
		CurrentItem->OnLeaveHover();
	}
}

void UVREditorRadialMenu::SelectCurrentItem( const FVirtualHand& Hand )
{
	if ( IsInMenuRadius() )
	{
		GetOwner()->GetOwner().TogglePanelsVisibility();
	}
	else if(CurrentItem)
	{
		CurrentItem->OnPressed();
	}
}

bool UVREditorRadialMenu::IsInMenuRadius() const
{
	return FVector2D::Distance( FVector2D::ZeroVector, TrackpadPosition ) < VREd::RadialMenuSelectionRadius->GetFloat();
}

void UVREditorRadialMenu::UpdateGizmoTypeLabel()
{
	FString Result;

	switch ( GetOwner()->GetOwner().GetOwner().GetCurrentGizmoType() )
	{
	case EGizmoHandleTypes::All:
		Result = LOCTEXT( "AllGizmoType", "Universal Gizmo" ).ToString();
		break;
	case EGizmoHandleTypes::Translate:
		Result = LOCTEXT( "TranslateGizmoType", "Translate Gizmo" ).ToString();
		break;
	case EGizmoHandleTypes::Rotate:
		Result = LOCTEXT( "RotateGizmoType", "Rotate Gizmo" ).ToString();
		break;
	case EGizmoHandleTypes::Scale:
		Result = LOCTEXT( "ScaleGizmoType", "Scale Gizmo" ).ToString();
		break;
	default:
		break;
	}

	TopItem->SetLabel( Result );
}

void UVREditorRadialMenu::OnGizmoCycle()
{
	GetOwner()->GetOwner().GetOwner().CycleTransformGizmoHandleType();
	UpdateGizmoTypeLabel();
}

void UVREditorRadialMenu::OnDuplicateButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Duplicate();
}


void UVREditorRadialMenu::OnDeleteButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().GetWorldInteraction().DeleteSelectedObjects();
}


void UVREditorRadialMenu::OnUndoButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Undo();
}


void UVREditorRadialMenu::OnRedoButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Redo();
}


void UVREditorRadialMenu::OnCopyButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Copy();
}


void UVREditorRadialMenu::OnPasteButtonClicked()
{
	GetOwner()->GetOwner().GetOwner().Paste();
}

#undef LOCTEXT_NAMESPACE
