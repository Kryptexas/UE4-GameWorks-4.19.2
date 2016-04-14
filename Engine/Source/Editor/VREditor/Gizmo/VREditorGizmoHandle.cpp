// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorGizmoHandle.h"
#include "VREditorTransformGizmo.h"
#include "VREditorMode.h"

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
	UpdateHoverAnimation( DraggingHandle, HoveringOverHandles, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );
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

void UVREditorGizmoHandleGroup::UpdateHandleColor( const int32 AxisIndex, FVREditorGizmoHandle& Handle, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles )
{
	UStaticMeshComponent* HandleMesh = Handle.HandleMesh;

	if ( !HandleMesh->GetMaterial( 0 )->IsA( UMaterialInstanceDynamic::StaticClass() ) )
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create( GizmoMaterial, this );
		HandleMesh->SetMaterial( 0, MID );
	}
	if ( !HandleMesh->GetMaterial( 1 )->IsA( UMaterialInstanceDynamic::StaticClass() ) )
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create( TranslucentGizmoMaterial, this );
		HandleMesh->SetMaterial( 1, MID );
	}
	UMaterialInstanceDynamic* MID0 = CastChecked<UMaterialInstanceDynamic>( HandleMesh->GetMaterial( 0 ) );
	UMaterialInstanceDynamic* MID1 = CastChecked<UMaterialInstanceDynamic>( HandleMesh->GetMaterial( 1 ) );

	ABaseTransformGizmo* GizmoActor = CastChecked<ABaseTransformGizmo>( GetOwner() );
	if (GizmoActor)
	{
		FVREditorMode* Mode = GizmoActor->GetOwnerMode();
		if (Mode)
		{
			FLinearColor HandleColor = Mode->GetColor( FVREditorMode::EColors::WhiteGizmoColor );
			if (HandleMesh == DraggingHandle)
			{
				HandleColor = Mode->GetColor( FVREditorMode::EColors::DraggingGizmoColor );
			}
			else if (AxisIndex != INDEX_NONE)
			{
				switch (AxisIndex)
				{
				case 0:
					HandleColor = Mode->GetColor( FVREditorMode::EColors::RedGizmoColor );
					break;

				case 1:
					HandleColor = Mode->GetColor( FVREditorMode::EColors::GreenGizmoColor );
					break;

				case 2:
					HandleColor = Mode->GetColor( FVREditorMode::EColors::BlueGizmoColor );
					break;
				}

				if (HoveringOverHandles.Contains( HandleMesh ))
				{
					HandleColor = FLinearColor::LerpUsingHSV( HandleColor, Mode->GetColor( FVREditorMode::EColors::HoverGizmoColor ), Handle.HoverAlpha );
				}
			}

			static FName StaticHandleColorParameter( "Color" );
			MID0->SetVectorParameterValue( StaticHandleColorParameter, HandleColor );
			MID1->SetVectorParameterValue( StaticHandleColorParameter, HandleColor );
		}
	}
}

class UStaticMeshComponent* UVREditorGizmoHandleGroup::CreateMeshHandle( class UStaticMesh* HandleMesh, const FString& ComponentName )
{
	const bool bAllowGizmoLighting = false;	// @todo vreditor: Not sure if we want this for gizmos or not yet.  Needs feedback.  Also they're translucent right now.

	UStaticMeshComponent* HandleComponent = CreateDefaultSubobject<UStaticMeshComponent>( *ComponentName );
	check( HandleComponent != nullptr );

	HandleComponent->SetStaticMesh( HandleMesh );
	HandleComponent->SetMobility( EComponentMobility::Movable );
	HandleComponent->SetupAttachment( this );

	HandleComponent->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
	HandleComponent->SetCollisionResponseToAllChannels( ECR_Ignore );
	HandleComponent->SetCollisionResponseToChannel( COLLISION_GIZMO, ECollisionResponse::ECR_Block );
	HandleComponent->SetCollisionObjectType( COLLISION_GIZMO );

	HandleComponent->bGenerateOverlapEvents = false;
	HandleComponent->SetCanEverAffectNavigation( false );
	HandleComponent->bCastDynamicShadow = bAllowGizmoLighting;
	HandleComponent->bCastStaticShadow = false;
	HandleComponent->bAffectDistanceFieldLighting = bAllowGizmoLighting;
	HandleComponent->bAffectDynamicIndirectLighting = bAllowGizmoLighting;

	return HandleComponent;
}

UStaticMeshComponent* UVREditorGizmoHandleGroup::CreateAndAddMeshHandle( UStaticMesh* HandleMesh, const FString& ComponentName, const FTransformGizmoHandlePlacement& HandlePlacement )
{
	UStaticMeshComponent* HandleComponent = CreateMeshHandle( HandleMesh, ComponentName );
	AddMeshToHandles( HandleComponent, HandlePlacement );
	return HandleComponent;
}

void UVREditorGizmoHandleGroup::AddMeshToHandles( UStaticMeshComponent* HandleMeshComponent, const FTransformGizmoHandlePlacement& HandlePlacement )
{
	int32 HandleIndex = MakeHandleIndex( HandlePlacement );
	if (Handles.Num() < (HandleIndex + 1))
	{
		Handles.SetNumZeroed( HandleIndex + 1 );
	}
	Handles[HandleIndex].HandleMesh = HandleMeshComponent;
}

FTransformGizmoHandlePlacement UVREditorGizmoHandleGroup::GetHandlePlacement( const int32 X, const int32 Y, const int32 Z ) const
{
	FTransformGizmoHandlePlacement HandlePlacement;
	HandlePlacement.Axes[0] = (ETransformGizmoHandleDirection)X;
	HandlePlacement.Axes[1] = (ETransformGizmoHandleDirection)Y;
	HandlePlacement.Axes[2] = (ETransformGizmoHandleDirection)Z;

	return HandlePlacement;
}

void UVREditorGizmoHandleGroup::UpdateHoverAnimation( UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	for (FVREditorGizmoHandle& Handle : Handles)
	{
		const bool bIsHoveringOverHandle = HoveringOverHandles.Contains( Handle.HandleMesh ) || (DraggingHandle != nullptr && DraggingHandle == Handle.HandleMesh);

		if (bIsHoveringOverHandle)
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
