// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorGizmoHandle.h"
#include "VREditorTransformGizmo.h"

UVREditorGizmoHandleGroup::UVREditorGizmoHandleGroup()
	: Super()
{

}

FTransformGizmoHandlePlacement UVREditorGizmoHandleGroup::MakeHandlePlacementForIndex( const int32 HandleIndex ) const
{
	FTransformGizmoHandlePlacement HandlePlacement;
	HandlePlacement.Axes[0] = (ETransformGizmoHandleDirection)(HandleIndex / 9);
	HandlePlacement.Axes[1] = (ETransformGizmoHandleDirection)((HandleIndex % 9) / 3);
	HandlePlacement.Axes[2] = (ETransformGizmoHandleDirection)((HandleIndex % 9) % 3);
	//	GWarn->Logf( TEXT( "%i = HandlePlacment[ %i %i %i ]" ), HandleIndex, (int32)HandlePlacement.Axes[ 0 ], (int32)HandlePlacement.Axes[ 1 ], (int32)HandlePlacement.Axes[ 2 ] );
	return HandlePlacement;
}

int32 UVREditorGizmoHandleGroup::MakeHandleIndex( const FTransformGizmoHandlePlacement HandlePlacement ) const
{
	const int32 HandleIndex = (int32)HandlePlacement.Axes[0] * 9 + (int32)HandlePlacement.Axes[1] * 3 + (int32)HandlePlacement.Axes[2];
	//	GWarn->Logf( TEXT( "HandlePlacment[ %i %i %i ] = %i" ), (int32)HandlePlacement.Axes[ 0 ], (int32)HandlePlacement.Axes[ 1 ], (int32)HandlePlacement.Axes[ 2 ], HandleIndex );
	return HandleIndex;
}

FString UVREditorGizmoHandleGroup::MakeHandleName( const FTransformGizmoHandlePlacement HandlePlacement ) const
{
	FString HandleName;
	int32 CenteredAxisCount = 0;
	for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
	{
		const ETransformGizmoHandleDirection HandleDirection = HandlePlacement.Axes[AxisIndex];

		if (HandleDirection == ETransformGizmoHandleDirection::Center)
		{
			++CenteredAxisCount;
		}
		else
		{
			switch (AxisIndex)
			{
			case 0:
				HandleName += (HandleDirection == ETransformGizmoHandleDirection::Negative) ? TEXT( "Back" ) : TEXT( "Front" );
				break;

			case 1:
				HandleName += (HandleDirection == ETransformGizmoHandleDirection::Negative) ? TEXT( "Left" ) : TEXT( "Right" );
				break;

			case 2:
				HandleName += (HandleDirection == ETransformGizmoHandleDirection::Negative) ? TEXT( "Bottom" ) : TEXT( "Top" );
				break;
			}
		}
	}

	if (CenteredAxisCount == 2)
	{
		HandleName += TEXT( "Center" );
	}
	else if (CenteredAxisCount == 3)
	{
		HandleName = TEXT( "Origin" );
	}

	return HandleName;
}

FVector UVREditorGizmoHandleGroup::GetAxisVector( const int32 AxisIndex, const ETransformGizmoHandleDirection HandleDirection )
{
	FVector AxisVector;

	if (HandleDirection == ETransformGizmoHandleDirection::Center)
	{
		AxisVector = FVector::ZeroVector;
	}
	else
	{
		const bool bIsFacingPositiveDirection = HandleDirection == ETransformGizmoHandleDirection::Positive;
		switch (AxisIndex)
		{
		case 0:
			AxisVector = (bIsFacingPositiveDirection ? FVector::ForwardVector : -FVector::ForwardVector);
			break;

		case 1:
			AxisVector = (bIsFacingPositiveDirection ? FVector::RightVector : -FVector::RightVector);
			break;

		case 2:
			AxisVector = (bIsFacingPositiveDirection ? FVector::UpVector : -FVector::UpVector);
			break;
		}
	}

	return AxisVector;
}

void UVREditorGizmoHandleGroup::GetHandleIndexInteractionType( const int32 HandleIndex, ETransformGizmoInteractionType& OutInteractionType, TOptional<FTransformGizmoHandlePlacement>& OutHandlePlacement )
{
	OutHandlePlacement = MakeHandlePlacementForIndex( HandleIndex );
	OutInteractionType = GetInteractionType();
}

ETransformGizmoInteractionType UVREditorGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::Translate;
}

void UVREditorGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	bOutIsHoveringOrDraggingThisHandleGroup = false;

	// Update hover animation
	for( FVREditorGizmoHandle& Handle : Handles )
	{
		const bool bIsHoveringOverHandle = HoveringOverHandles.Contains( Handle.HandleMesh ) || ( DraggingHandle != nullptr && DraggingHandle == Handle.HandleMesh );

		if( bIsHoveringOverHandle )
		{
			Handle.HoverAlpha += GetWorld()->GetDeltaSeconds() / GizmoHoverAnimationDuration;
			bOutIsHoveringOrDraggingThisHandleGroup = true;
		}
		else
		{
			Handle.HoverAlpha -= GetWorld()->GetDeltaSeconds() / GizmoHoverAnimationDuration;
		}
		Handle.HoverAlpha = FMath::Clamp( Handle.HoverAlpha, 0.0f, 1.0f );
	}
}


int32 UVREditorGizmoHandleGroup::GetDraggedHandleIndex( class UStaticMeshComponent* DraggedMesh )
{
	for( int32 HandleIndex = 0; HandleIndex < Handles.Num(); ++HandleIndex )
	{
		if( Handles[ HandleIndex ].HandleMesh == DraggedMesh )
		{
			return HandleIndex;
		}
	}
	return INDEX_NONE;
}

void UVREditorGizmoHandleGroup::SetGizmoMaterial( UMaterialInterface* Material )
{
	GizmoMaterial = Material;
}

void UVREditorGizmoHandleGroup::SetTranslucentGizmoMaterial( UMaterialInterface* Material )
{
	TranslucentGizmoMaterial = Material;
}

TArray<FVREditorGizmoHandle>& UVREditorGizmoHandleGroup::GetHandles()
{
	return Handles;
}

EGizmoHandleTypes UVREditorGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::All;
}