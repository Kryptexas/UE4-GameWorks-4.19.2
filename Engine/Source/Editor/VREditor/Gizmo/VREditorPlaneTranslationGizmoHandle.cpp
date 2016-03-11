// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorPlaneTranslationGizmoHandle.h"
#include "VREditorTransformGizmo.h"
#include "UnitConversion.h"

UVREditorPlaneTranslationGizmoHandleGroup::UVREditorPlaneTranslationGizmoHandleGroup()
	: Super()
{
	UStaticMesh* PlaneTranslationHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/PlaneTranslationHandle" ) );
		PlaneTranslationHandleMesh = ObjectFinder.Object;
		check( PlaneTranslationHandleMesh != nullptr );
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

					// Plane translation only happens on the center of an axis.  And we only bother drawing one for the "positive" direction.
					if (CenterHandleCount == 2 && HandlePlacement.Axes[FacingAxisIndex] == ETransformGizmoHandleDirection::Positive)
					{
						// Plane translation handle
						{
							FString ComponentName = HandleName + TEXT( "PlaneTranslationHandle" );

							UStaticMeshComponent* PlaneTranslationHandle = CreateDefaultSubobject<UStaticMeshComponent>( *ComponentName );
							check( PlaneTranslationHandle != nullptr );

							PlaneTranslationHandle->SetStaticMesh( PlaneTranslationHandleMesh );
							PlaneTranslationHandle->SetMobility( EComponentMobility::Movable );
							PlaneTranslationHandle->AttachTo( this );

							PlaneTranslationHandle->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
							PlaneTranslationHandle->SetCollisionResponseToAllChannels( ECR_Ignore );
							PlaneTranslationHandle->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );

							PlaneTranslationHandle->bGenerateOverlapEvents = false;
							PlaneTranslationHandle->SetCanEverAffectNavigation( false );
							PlaneTranslationHandle->bCastDynamicShadow = bAllowGizmoLighting;
							PlaneTranslationHandle->bCastStaticShadow = false;
							PlaneTranslationHandle->bAffectDistanceFieldLighting = bAllowGizmoLighting;
							PlaneTranslationHandle->bAffectDynamicIndirectLighting = bAllowGizmoLighting;

							int32 HandleIndex = MakeHandleIndex( HandlePlacement );
							if (Handles.Num() < (HandleIndex + 1))
							{
								Handles.SetNumZeroed( HandleIndex + 1 );
							}
							Handles[HandleIndex].HandleMesh = PlaneTranslationHandle;
						}
					}
				}
			}
		}
	}
}

void UVREditorPlaneTranslationGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, bAllHandlesVisible, DraggingHandle, HoveringOverHandles, AnimationAlpha, GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );

	for (int32 HandleIndex = 0; HandleIndex < Handles.Num(); ++HandleIndex)
	{
		FVREditorGizmoHandle& Handle = Handles[ HandleIndex ];
		UStaticMeshComponent* PlaneTranslationHandle = Handle.HandleMesh;
		if (PlaneTranslationHandle != nullptr)	// Can be null if no handle for this specific placement
		{
			const FTransformGizmoHandlePlacement HandlePlacement = MakeHandlePlacementForIndex( HandleIndex );

			int32 CenterHandleCount, FacingAxisIndex, CenterAxisIndex;
			HandlePlacement.GetCenterHandleCountAndFacingAxisIndex( /* Out */ CenterHandleCount, /* Out */ FacingAxisIndex, /* Out */ CenterAxisIndex );
			check( CenterHandleCount == 2 );

			const FVector GizmoSpaceFacingAxisVector = GetAxisVector( FacingAxisIndex, HandlePlacement.Axes[FacingAxisIndex] );
			const FQuat GizmoSpaceOrientation = GizmoSpaceFacingAxisVector.ToOrientationQuat();

			FVector HandleRelativeLocation = LocalBounds.GetCenter();

			float GizmoHandleScale = GizmoScale;

			// Push toward the viewer (along our facing axis) a bit
			const float GizmoSpaceOffsetAlongFacingAxis =
				GizmoHandleScale *
				(10.0f +
				(1.0f - AnimationAlpha) * 6.0f);	// @todo vreditor tweak: Animation offset

			// Make the handle bigger while hovered (but don't affect the offset -- we want it to scale about it's origin)
			GizmoHandleScale *= FMath::Lerp( 1.0f, GizmoHoverScale, Handle.HoverAlpha );

			const FVector WorldSpacePositiveOffsetLocation = LocalToWorld.TransformPosition( HandleRelativeLocation + GizmoSpaceOffsetAlongFacingAxis * GizmoSpaceFacingAxisVector );
			const FVector WorldSpaceNegativeOffsetLocation = LocalToWorld.TransformPosition( HandleRelativeLocation - GizmoSpaceOffsetAlongFacingAxis * GizmoSpaceFacingAxisVector );
			const bool bIsPositiveTowardViewer =
				FVector::DistSquared( WorldSpacePositiveOffsetLocation, ViewLocation ) <
				FVector::DistSquared( WorldSpaceNegativeOffsetLocation, ViewLocation );

			if (bIsPositiveTowardViewer)
			{
				// Back it up
				HandleRelativeLocation -= GizmoSpaceOffsetAlongFacingAxis * GizmoSpaceFacingAxisVector;
			}
			else
			{
				// Push it forward
				HandleRelativeLocation += GizmoSpaceOffsetAlongFacingAxis * GizmoSpaceFacingAxisVector;
			}

			PlaneTranslationHandle->SetRelativeLocation( HandleRelativeLocation );

			PlaneTranslationHandle->SetRelativeRotation( GizmoSpaceOrientation );

			PlaneTranslationHandle->SetRelativeScale3D( FVector( GizmoHandleScale ) );

			// Update material
			{
				if (!PlaneTranslationHandle->GetMaterial( 0 )->IsA( UMaterialInstanceDynamic::StaticClass() ))
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create( GizmoMaterial, this );
					PlaneTranslationHandle->SetMaterial( 0, MID );
				}
				if (!PlaneTranslationHandle->GetMaterial( 1 )->IsA( UMaterialInstanceDynamic::StaticClass() ))
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create( TranslucentGizmoMaterial, this );
					PlaneTranslationHandle->SetMaterial( 1, MID );
				}
				UMaterialInstanceDynamic* MID0 = CastChecked<UMaterialInstanceDynamic>( PlaneTranslationHandle->GetMaterial( 0 ) );
				UMaterialInstanceDynamic* MID1 = CastChecked<UMaterialInstanceDynamic>( PlaneTranslationHandle->GetMaterial( 1 ) );

				FLinearColor HandleColor = FLinearColor::White;
				if (PlaneTranslationHandle == DraggingHandle)
				{
					HandleColor = VREd::GizmoColor::DraggingGizmoColor;
				}
				else if (FacingAxisIndex != INDEX_NONE)
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

					if( HoveringOverHandles.Contains( PlaneTranslationHandle ) )
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

ETransformGizmoInteractionType UVREditorPlaneTranslationGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::TranslateOnPlane;
}

EGizmoHandleTypes UVREditorPlaneTranslationGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Translate;
}