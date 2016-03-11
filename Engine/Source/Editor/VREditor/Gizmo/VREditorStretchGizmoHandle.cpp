// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorStretchGizmoHandle.h"
#include "UnitConversion.h"
#include "VREditorTransformGizmo.h"

UVREditorStretchGizmoHandleGroup::UVREditorStretchGizmoHandleGroup()
	: Super()
{
	UStaticMesh* StretchingHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/StretchingHandle" ) );
		StretchingHandleMesh = ObjectFinder.Object;
		check( StretchingHandleMesh != nullptr );
	}

	UStaticMesh* BoundingBoxCornerMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/BoundingBoxCorner" ) );
		BoundingBoxCornerMesh = ObjectFinder.Object;
		check( BoundingBoxCornerMesh != nullptr );
	}

	UStaticMesh* BoundingBoxEdgeMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/BoundingBoxEdge" ) );
		BoundingBoxEdgeMesh = ObjectFinder.Object;
		check( BoundingBoxEdgeMesh != nullptr );
	}

	const bool bAllowGizmoLighting = false;	// @todo vreditor: Not sure if we want this for gizmos or not yet.  Needs feedback.  Also they're translucent right now.

	for (int32 X = 0; X < 3; ++X)
	{
		for (int32 Y = 0; Y < 3; ++Y)
		{
			for (int32 Z = 0; Z < 3; ++Z)
			{
				FTransformGizmoHandlePlacement HandlePlacement;
				HandlePlacement.Axes[0] = (ETransformGizmoHandleDirection)X;
				HandlePlacement.Axes[1] = (ETransformGizmoHandleDirection)Y;
				HandlePlacement.Axes[2] = (ETransformGizmoHandleDirection)Z;

				int32 CenterHandleCount, FacingAxisIndex, CenterAxisIndex;
				HandlePlacement.GetCenterHandleCountAndFacingAxisIndex( /* Out */ CenterHandleCount, /* Out */ FacingAxisIndex, /* Out */ CenterAxisIndex );

				// Don't allow translation/stretching/rotation from the origin
				if (CenterHandleCount < 3)
				{
					const FString HandleName = MakeHandleName( HandlePlacement );

					// Stretching handle
					if (CenterHandleCount != 1)	// @todo vreditor: Remove this line if we want to re-enable support for edge stretching handles (rather than only corners).  We disabled this because they sort of got in the way of the rotation gizmo, and weren't very popular to use.
					{
						FString ComponentName = HandleName + TEXT( "StretchingHandle" );

						UStaticMeshComponent* StretchingHandle = CreateDefaultSubobject<UStaticMeshComponent>( *ComponentName );
						check( StretchingHandle != nullptr );

						if (CenterHandleCount == 0)	// Corner?
						{
							StretchingHandle->SetStaticMesh( BoundingBoxCornerMesh );
						}
						else if (CenterHandleCount == 1)	// Edge?
						{
							StretchingHandle->SetStaticMesh( BoundingBoxEdgeMesh );
						}
						else  // Face
						{
							StretchingHandle->SetStaticMesh( StretchingHandleMesh );
						}
						StretchingHandle->SetMobility( EComponentMobility::Movable );
						StretchingHandle->AttachTo( this );

						StretchingHandle->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
						StretchingHandle->SetCollisionResponseToAllChannels( ECR_Ignore );
						StretchingHandle->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );

						StretchingHandle->bGenerateOverlapEvents = false;
						StretchingHandle->SetCanEverAffectNavigation( false );
						StretchingHandle->bCastDynamicShadow = bAllowGizmoLighting;
						StretchingHandle->bCastStaticShadow = false;
						StretchingHandle->bAffectDistanceFieldLighting = bAllowGizmoLighting;
						StretchingHandle->bAffectDynamicIndirectLighting = bAllowGizmoLighting;

						int32 HandleIndex = MakeHandleIndex( HandlePlacement );
						if (Handles.Num() < (HandleIndex + 1))
						{
							Handles.SetNumZeroed( HandleIndex + 1 );
						}
						Handles[HandleIndex].HandleMesh = StretchingHandle;
					}
				}
			}
		}
	}
}

void UVREditorStretchGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, bAllHandlesVisible, DraggingHandle, HoveringOverHandles, AnimationAlpha, GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );

	for (int32 HandleIndex = 0; HandleIndex < Handles.Num(); ++HandleIndex)
	{
		FVREditorGizmoHandle& Handle = Handles[ HandleIndex ];

		UStaticMeshComponent* StretchingHandle = Handle.HandleMesh;
		if (StretchingHandle != nullptr)	// Can be null if no handle for this specific placement
		{
			const FTransformGizmoHandlePlacement HandlePlacement = MakeHandlePlacementForIndex( HandleIndex );

			float GizmoHandleScale = GizmoScale;

			const float OffsetFromSide = GizmoHandleScale *
				(0.0f +	// @todo vreditor tweak: Hard coded handle offset from side of primitive
				(1.0f - AnimationAlpha) * 10.0f);	// @todo vreditor tweak: Animation offset

			// Make the handle bigger while hovered (but don't affect the offset -- we want it to scale about it's origin)
			GizmoHandleScale *= FMath::Lerp( 1.0f, GizmoHoverScale, Handle.HoverAlpha );

			FVector HandleRelativeLocation = FVector::ZeroVector;
			for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
			{
				if (HandlePlacement.Axes[AxisIndex] == ETransformGizmoHandleDirection::Negative)	// Negative direction
				{
					HandleRelativeLocation[AxisIndex] = LocalBounds.Min[AxisIndex] - OffsetFromSide;
				}
				else if (HandlePlacement.Axes[AxisIndex] == ETransformGizmoHandleDirection::Positive)	// Positive direction
				{
					HandleRelativeLocation[AxisIndex] = LocalBounds.Max[AxisIndex] + OffsetFromSide;
				}
				else // ETransformGizmoHandleDirection::Center
				{
					HandleRelativeLocation[AxisIndex] = LocalBounds.GetCenter()[AxisIndex];
				}
			}

			StretchingHandle->SetRelativeLocation( HandleRelativeLocation );

			int32 CenterHandleCount, FacingAxisIndex, CenterAxisIndex;
			HandlePlacement.GetCenterHandleCountAndFacingAxisIndex( /* Out */ CenterHandleCount, /* Out */ FacingAxisIndex, /* Out */ CenterAxisIndex );

			FRotator Rotator = FRotator::ZeroRotator;
			{
				// Back bottom left
				if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[1] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[2] == ETransformGizmoHandleDirection::Negative)
				{
					Rotator.Yaw = 0.0f;
					Rotator.Pitch = 0.0f;
				}

				// Back bottom right
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[1] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[2] == ETransformGizmoHandleDirection::Negative)
				{
					Rotator.Yaw = -90.0f;
					Rotator.Pitch = 0.0f;
				}

				// Back top left
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[1] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[2] == ETransformGizmoHandleDirection::Positive)
				{
					Rotator.Yaw = 0.0f;
					Rotator.Pitch = -90.0f;
				}

				// Back top right
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[1] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[2] == ETransformGizmoHandleDirection::Positive)
				{
					Rotator.Yaw = -90.0f;
					Rotator.Pitch = -90.0f;
				}

				// Front bottom left
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[1] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[2] == ETransformGizmoHandleDirection::Negative)
				{
					Rotator.Yaw = 0.0f;
					Rotator.Pitch = 90.0f;
				}

				// Front bottom right
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[1] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[2] == ETransformGizmoHandleDirection::Negative)
				{
					Rotator.Yaw = 90.0f;
					Rotator.Pitch = 90.0f;
				}

				// Front top left
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[1] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[2] == ETransformGizmoHandleDirection::Positive)
				{
					Rotator.Yaw = 0.0f;
					Rotator.Pitch = -180.0f;
				}

				// Front top right
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[1] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[2] == ETransformGizmoHandleDirection::Positive)
				{
					Rotator.Yaw = 180.0f;
					Rotator.Pitch = -90.0f;
				}

				// Back left/right edge
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[1] != ETransformGizmoHandleDirection::Center)
				{
					Rotator.Yaw = 0.0f;
					Rotator.Pitch = 90.0f;
				}

				// Back bottom/top edge
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Negative && HandlePlacement.Axes[2] != ETransformGizmoHandleDirection::Center)
				{
					Rotator.Yaw = 90.0f;
					Rotator.Pitch = 0.0f;
				}

				// Front left/right edge
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[1] != ETransformGizmoHandleDirection::Center)
				{
					Rotator.Yaw = 0.0f;
					Rotator.Pitch = 90.0f;
				}

				// Front bottom/top edge
				else if (HandlePlacement.Axes[0] == ETransformGizmoHandleDirection::Positive && HandlePlacement.Axes[2] != ETransformGizmoHandleDirection::Center)
				{
					Rotator.Yaw = 90.0f;
					Rotator.Pitch = 0.0f;
				}

				else
				{
					// Facing out from center of a face
					if (CenterHandleCount == 2)
					{
						const FQuat GizmoSpaceOrientation = GetAxisVector( FacingAxisIndex, HandlePlacement.Axes[FacingAxisIndex] ).ToOrientationQuat();
						Rotator = GizmoSpaceOrientation.Rotator();
					}

					else
					{
						// One of the left/right bottom or top edges
					}
				}
			}

			StretchingHandle->SetRelativeRotation( Rotator );

			StretchingHandle->SetRelativeScale3D( FVector( GizmoHandleScale ) );

			// Update material
			{
				if (!StretchingHandle->GetMaterial( 0 )->IsA( UMaterialInstanceDynamic::StaticClass() ))
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create( GizmoMaterial, this );
					StretchingHandle->SetMaterial( 0, MID );
				}
				if (!StretchingHandle->GetMaterial( 1 )->IsA( UMaterialInstanceDynamic::StaticClass() ))
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create( TranslucentGizmoMaterial, this );
					StretchingHandle->SetMaterial( 1, MID );
				}
				UMaterialInstanceDynamic* MID0 = CastChecked<UMaterialInstanceDynamic>( StretchingHandle->GetMaterial( 0 ) );
				UMaterialInstanceDynamic* MID1 = CastChecked<UMaterialInstanceDynamic>( StretchingHandle->GetMaterial( 1 ) );

				FLinearColor HandleColor = FLinearColor::White;
				if (StretchingHandle == DraggingHandle)
				{
					HandleColor = VREd::GizmoColor::DraggingGizmoColor;
				}
				else 
				{
					if (FacingAxisIndex != INDEX_NONE)
					{
						switch (FacingAxisIndex)
						{
						case 0:
							HandleColor = VREd::GizmoColor::RedGizmoColor;
							break;

						case 1:
							HandleColor = VREd::GizmoColor::GreenGizmoColor;
							break;

						case 2:
							HandleColor = VREd::GizmoColor::BlueGizmoColor;
							break;
						}
					}

					if( HoveringOverHandles.Contains( StretchingHandle ) )
					{
						HandleColor = FLinearColor::LerpUsingHSV( HandleColor, VREd::GizmoColor::HoverGizmoColor, Handle.HoverAlpha );
					}
				}
				MID0->SetVectorParameterValue( "Color", HandleColor );
				MID1->SetVectorParameterValue( "Color", HandleColor );
			}
		}
	}
}

ETransformGizmoInteractionType UVREditorStretchGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::StretchAndReposition;
}

EGizmoHandleTypes UVREditorStretchGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Scale;
}

bool UVREditorStretchGizmoHandleGroup::SupportsWorldCoordinateSpace() const
{
	// Stretching only works with local space gizmos
	return false;
}
