// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorTranslationGizmoHandle.h"
#include "VREditorTransformGizmo.h"
#include "UnitConversion.h"

UVREditorTranslationGizmoHandleGroup::UVREditorTranslationGizmoHandleGroup() 
	: Super()
{
	UStaticMesh* TranslationHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TranslationHandle" ) );
		TranslationHandleMesh = ObjectFinder.Object;
		check( TranslationHandleMesh != nullptr );
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

					// Don't bother with translation handles on the corners.  They don't make too much sense.
					if (CenterHandleCount == 2)
					{
						// Translation handle
						{
							FString ComponentName = HandleName + TEXT( "TranslationHandle" );

							UStaticMeshComponent* TranslationHandle = CreateDefaultSubobject<UStaticMeshComponent>( *ComponentName );
							check( TranslationHandle != nullptr );

							TranslationHandle->SetStaticMesh( TranslationHandleMesh );
							TranslationHandle->SetMobility( EComponentMobility::Movable );
							TranslationHandle->AttachTo( this );

							// @todo vreditor: We added a new engine collision channel "ECC_EditorGizmo" so that we could trace only
							// against gizmos and nothing else.  This allows you to "click thru" solid objects to hit a ghosted gizmo.
							// However, we need to look out for backwards compatibility issues with adding a new engine collision
							// channel.  When I tested this with an existing project, all of the engine collision channels had been
							// duplicated into the game's DefaultEngine.ini and were not updated to filter out the ECC_EditorGizmo
							// like the stock collision channels that I updated in BaseEngine.ini when adding this feature.

							TranslationHandle->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
							TranslationHandle->SetCollisionResponseToAllChannels( ECR_Ignore );
							TranslationHandle->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );

							TranslationHandle->bGenerateOverlapEvents = false;
							TranslationHandle->SetCanEverAffectNavigation( false );
							TranslationHandle->bCastDynamicShadow = bAllowGizmoLighting;
							TranslationHandle->bCastStaticShadow = false;
							TranslationHandle->bAffectDistanceFieldLighting = bAllowGizmoLighting;
							TranslationHandle->bAffectDynamicIndirectLighting = bAllowGizmoLighting;

							int32 HandleIndex = MakeHandleIndex( HandlePlacement );
							if (Handles.Num() < (HandleIndex + 1))
							{
								Handles.SetNumZeroed( HandleIndex + 1 );
							}
							Handles[HandleIndex].HandleMesh = TranslationHandle;
						}
					}
				}
			}
		}
	}
}

void UVREditorTranslationGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, bAllHandlesVisible, DraggingHandle, HoveringOverHandles, AnimationAlpha, GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );

	// Place the translation handles
	for (int32 HandleIndex = 0; HandleIndex < Handles.Num(); ++HandleIndex)
	{
		FVREditorGizmoHandle& Handle = Handles[ HandleIndex ];

		UStaticMeshComponent* TranslationHandle = Handle.HandleMesh;
		if (TranslationHandle != nullptr)	// Can be null if no handle for this specific placement
		{
			const FTransformGizmoHandlePlacement HandlePlacement = MakeHandlePlacementForIndex( HandleIndex );

			float GizmoHandleScale = GizmoScale;
			const float OffsetFromSide = GizmoHandleScale *
				(10.0f +	// @todo vreditor tweak: Hard coded handle offset from side of primitive
				(1.0f - AnimationAlpha) * 4.0f);	// @todo vreditor tweak: Animation offset

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

			TranslationHandle->SetRelativeLocation( HandleRelativeLocation );

			int32 CenterHandleCount, FacingAxisIndex, CenterAxisIndex;
			HandlePlacement.GetCenterHandleCountAndFacingAxisIndex( /* Out */ CenterHandleCount, /* Out */ FacingAxisIndex, /* Out */ CenterAxisIndex );
			check( CenterHandleCount == 2 );

			const FQuat GizmoSpaceOrientation = GetAxisVector( FacingAxisIndex, HandlePlacement.Axes[FacingAxisIndex] ).ToOrientationQuat();
			TranslationHandle->SetRelativeRotation( GizmoSpaceOrientation );

			TranslationHandle->SetRelativeScale3D( FVector( GizmoHandleScale ) );

			// Update material
			{
				if (!TranslationHandle->GetMaterial( 0 )->IsA( UMaterialInstanceDynamic::StaticClass() ))
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create( GizmoMaterial, this );
					TranslationHandle->SetMaterial( 0, MID );
				}
				if (!TranslationHandle->GetMaterial( 1 )->IsA( UMaterialInstanceDynamic::StaticClass() ))
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create( TranslucentGizmoMaterial, this );
					TranslationHandle->SetMaterial( 1, MID );
				}
				UMaterialInstanceDynamic* MID0 = CastChecked<UMaterialInstanceDynamic>( TranslationHandle->GetMaterial( 0 ) );
				UMaterialInstanceDynamic* MID1 = CastChecked<UMaterialInstanceDynamic>( TranslationHandle->GetMaterial( 1 ) );

				FLinearColor HandleColor = FLinearColor::White;
				if (TranslationHandle == DraggingHandle)
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

					if( HoveringOverHandles.Contains( TranslationHandle ) )
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

ETransformGizmoInteractionType UVREditorTranslationGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::Translate;
}

EGizmoHandleTypes UVREditorTranslationGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Translate;
}