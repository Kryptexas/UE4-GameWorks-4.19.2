// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Gizmo/VIPivotTransformGizmo.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/WorldSettings.h"
#include "VIStretchGizmoHandle.h"
#include "VIUniformScaleGizmoHandle.h"
#include "ViewportWorldInteraction.h"

namespace VREd //@todo VREditor: Duplicates of TransformGizmo
{
	// @todo vreditor tweak: Tweak out console variables
	static FAutoConsoleVariable PivotGizmoMinDistanceForScaling( TEXT( "VI.PivotGizmoMinDistanceForScaling" ), 0.0f, TEXT( "How far away the camera needs to be from an object before we'll start scaling it based on distance"));
	static FAutoConsoleVariable PivotGizmoDistanceScaleFactor( TEXT( "VI.PivotGizmoDistanceScaleFactor" ), 0.003f, TEXT( "How much the gizmo handles should increase in size with distance from the camera, to make it easier to select" ) );
	static FAutoConsoleVariable PivotGizmoTranslationPivotOffsetX( TEXT("VI.PivotGizmoTranslationPivotOffsetX" ), 30.0f, TEXT( "How much the translation cylinder is offsetted from the pivot" ) );
	static FAutoConsoleVariable PivotGizmoScalePivotOffsetX( TEXT( "VI.PivotGizmoScalePivotOffsetX" ), 120.0f, TEXT( "How much the non-uniform scale is offsetted from the pivot" ) );
	static FAutoConsoleVariable PivotGizmoPlaneTranslationPivotOffsetYZ(TEXT("VI.PivotGizmoPlaneTranslationPivotOffsetYZ" ), 40.0f, TEXT( "How much the plane translation is offsetted from the pivot" ) );
	static FAutoConsoleVariable PivotGizmoTranslationScaleMultiply( TEXT( "VI.PivotGizmoTranslationScaleMultiply" ), 2.0f, TEXT( "Multiplies translation handles scale" ) );
	static FAutoConsoleVariable PivotGizmoTranslationHoverScaleMultiply( TEXT( "VI.PivotGizmoTranslationHoverScaleMultiply" ), 0.75f, TEXT( "Multiplies translation handles hover scale" ) );
}

APivotTransformGizmo::APivotTransformGizmo() :
	Super()
{
	UniformScaleGizmoHandleGroup = CreateDefaultSubobject<UUniformScaleGizmoHandleGroup>( TEXT( "UniformScaleHandles" ), true );
	UniformScaleGizmoHandleGroup->SetOwningTransformGizmo(this);
	UniformScaleGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	UniformScaleGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	UniformScaleGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( UniformScaleGizmoHandleGroup );

	TranslationGizmoHandleGroup = CreateDefaultSubobject<UPivotTranslationGizmoHandleGroup>(TEXT("TranslationHandles"), true);
	TranslationGizmoHandleGroup->SetOwningTransformGizmo(this);
	TranslationGizmoHandleGroup->SetTranslucentGizmoMaterial(TranslucentGizmoMaterial);
	TranslationGizmoHandleGroup->SetGizmoMaterial(GizmoMaterial);
	TranslationGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add(TranslationGizmoHandleGroup);

	ScaleGizmoHandleGroup = CreateDefaultSubobject<UPivotScaleGizmoHandleGroup>( TEXT( "ScaleHandles" ), true );
	ScaleGizmoHandleGroup->SetOwningTransformGizmo(this);
	ScaleGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	ScaleGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	ScaleGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( ScaleGizmoHandleGroup );

	PlaneTranslationGizmoHandleGroup = CreateDefaultSubobject<UPivotPlaneTranslationGizmoHandleGroup>( TEXT( "PlaneTranslationHandles" ), true );
	PlaneTranslationGizmoHandleGroup->SetOwningTransformGizmo(this);
	PlaneTranslationGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	PlaneTranslationGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	PlaneTranslationGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( PlaneTranslationGizmoHandleGroup );

	RotationGizmoHandleGroup = CreateDefaultSubobject<UPivotRotationGizmoHandleGroup>( TEXT( "RotationHandles" ), true );
	RotationGizmoHandleGroup->SetOwningTransformGizmo(this);
	RotationGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	RotationGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	RotationGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( RotationGizmoHandleGroup );

	StretchGizmoHandleGroup = CreateDefaultSubobject<UStretchGizmoHandleGroup>( TEXT( "StretchHandles" ), true );
	StretchGizmoHandleGroup->SetOwningTransformGizmo(this);
	StretchGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	StretchGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	StretchGizmoHandleGroup->SetShowOnUniversalGizmo( false );
	StretchGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( StretchGizmoHandleGroup );

	// There may already be some objects selected as we switch into VR mode, so we'll pretend that just happened so
	// that we can make sure all transitions complete properly
	OnNewObjectsSelected();
}

void APivotTransformGizmo::UpdateGizmo( const EGizmoHandleTypes GizmoType, const ECoordSystem GizmoCoordinateSpace, const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, const float ScaleMultiplier, bool bAllHandlesVisible, 
	UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, const float GizmoHoverScale, const float GizmoHoverAnimationDuration )
{
	const float WorldScaleFactor = GetWorld()->GetWorldSettings()->WorldToMeters / 100.0f;

	// Position the gizmo at the location of the first selected actor
	const bool bSweep = false;
	this->SetActorTransform( LocalToWorld, bSweep );


	// Increase scale with distance, to make gizmo handles easier to click on
	const float WorldSpaceDistanceToToPivot = FMath::Max( VREd::PivotGizmoMinDistanceForScaling->GetFloat(), FMath::Sqrt( FVector::DistSquared( GetActorLocation(), ViewLocation ) ) );

	// @todo gizmo: Scale causes bounds-based features to look wrong
	const float GizmoScale( ScaleMultiplier * ( ( WorldSpaceDistanceToToPivot / WorldScaleFactor ) * VREd::PivotGizmoDistanceScaleFactor->GetFloat() ) * WorldScaleFactor );

	// Update animation
	float AnimationAlpha = GetAnimationAlpha();

	// Update all the handles
	bool bIsHoveringOrDraggingScaleGizmo = false;
	for ( UGizmoHandleGroup* HandleGroup : AllHandleGroups )
	{
		if ( HandleGroup != nullptr)
		{
			if ( DraggingHandle == nullptr || HandleGroup == StretchGizmoHandleGroup || HandleGroup == RotationGizmoHandleGroup)
			{
				bool bIsHoveringOrDraggingThisHandleGroup = false;
				HandleGroup->UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, DraggingHandle,
					HoveringOverHandles, AnimationAlpha, GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, /* Out */ bIsHoveringOrDraggingThisHandleGroup );
			}

			if (HandleGroup != RotationGizmoHandleGroup)
			{
				HandleGroup->UpdateVisibilityAndCollision(GizmoType, GizmoCoordinateSpace, bAllHandlesVisible, DraggingHandle);
			}
		}
	}
}

/************************************************************************/
/* Translation                                                          */
/************************************************************************/
UPivotTranslationGizmoHandleGroup::UPivotTranslationGizmoHandleGroup() :
	Super()
{
	UStaticMesh* TranslationHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TranslateArrowHandle" ) );
		TranslationHandleMesh = ObjectFinder.Object;
		check( TranslationHandleMesh != nullptr );
	}

	CreateHandles( TranslationHandleMesh, FString( "PivotTranslationHandle" ) );
}


void UPivotTranslationGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, class UActorComponent* DraggingHandle, 
	const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, DraggingHandle, HoveringOverHandles, AnimationAlpha,
		GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );
	
	const float MultipliedGizmoScale = GizmoScale * VREd::PivotGizmoTranslationScaleMultiply->GetFloat();
	const float MultipliedGizmoHoverScale = GizmoHoverScale * VREd::PivotGizmoTranslationHoverScaleMultiply->GetFloat();

	UpdateHandlesRelativeTransformOnAxis( FTransform( FVector( VREd::PivotGizmoTranslationPivotOffsetX->GetFloat(), 0, 0 ) ), AnimationAlpha, MultipliedGizmoScale, MultipliedGizmoHoverScale, ViewLocation, DraggingHandle, HoveringOverHandles );
}

ETransformGizmoInteractionType UPivotTranslationGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::Translate;
}

EGizmoHandleTypes UPivotTranslationGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Translate;
}

/************************************************************************/
/* Scale	                                                            */
/************************************************************************/
UPivotScaleGizmoHandleGroup::UPivotScaleGizmoHandleGroup() :
	Super()
{
	UStaticMesh* ScaleHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/UniformScaleHandle" ) );
		ScaleHandleMesh = ObjectFinder.Object;
		check( ScaleHandleMesh != nullptr );
	}

	CreateHandles( ScaleHandleMesh, FString( "PivotScaleHandle" ) );	
}

void UPivotScaleGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, class UActorComponent* DraggingHandle, 
	const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, DraggingHandle, HoveringOverHandles, AnimationAlpha, 
		GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );

	UpdateHandlesRelativeTransformOnAxis( FTransform( FVector( VREd::PivotGizmoScalePivotOffsetX->GetFloat(), 0, 0 ) ), AnimationAlpha, GizmoScale, GizmoHoverScale, ViewLocation, DraggingHandle, HoveringOverHandles );
}

ETransformGizmoInteractionType UPivotScaleGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::Scale;
}

EGizmoHandleTypes UPivotScaleGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Scale;
}

bool UPivotScaleGizmoHandleGroup::SupportsWorldCoordinateSpace() const
{
	return false;
}

/************************************************************************/
/* Plane Translation	                                                */
/************************************************************************/
UPivotPlaneTranslationGizmoHandleGroup::UPivotPlaneTranslationGizmoHandleGroup() :
	Super()
{
	UStaticMesh* TranslationHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/PlaneTranslationHandle" ) );
		TranslationHandleMesh = ObjectFinder.Object;
		check( TranslationHandleMesh != nullptr );
	}

	CreateHandles( TranslationHandleMesh, FString( "PlaneTranslationHandle" ) );
}

void UPivotPlaneTranslationGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, class UActorComponent* DraggingHandle,
	const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, DraggingHandle, HoveringOverHandles, AnimationAlpha,
		GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );

	UpdateHandlesRelativeTransformOnAxis( FTransform( FVector( 0, VREd::PivotGizmoPlaneTranslationPivotOffsetYZ->GetFloat(), VREd::PivotGizmoPlaneTranslationPivotOffsetYZ->GetFloat() ) ), 
		AnimationAlpha, GizmoScale, GizmoHoverScale, ViewLocation, DraggingHandle, HoveringOverHandles );
}

ETransformGizmoInteractionType UPivotPlaneTranslationGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::TranslateOnPlane;
}

EGizmoHandleTypes UPivotPlaneTranslationGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Translate;
}

/************************************************************************/
/* Rotation																*/
/************************************************************************/
UPivotRotationGizmoHandleGroup::UPivotRotationGizmoHandleGroup() :
	Super()
{
	UStaticMesh* QuarterRotationHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder(TEXT("/Engine/VREditor/TransformGizmo/RotationHandleQuarter"));
		QuarterRotationHandleMesh = ObjectFinder.Object;
		check(QuarterRotationHandleMesh != nullptr);
	}

	CreateHandles(QuarterRotationHandleMesh, FString("RotationHandle"));

	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder(TEXT("/Engine/VREditor/TransformGizmo/RotationHandleFull"));
		UStaticMesh* FullRotationHandleMesh = ObjectFinder.Object;
		check(FullRotationHandleMesh != nullptr);

		FullRotationHandleMeshComponent = CreateMeshHandle(FullRotationHandleMesh, FString("FullRotationHandle"));
		FullRotationHandleMeshComponent->SetVisibility(false);
		FullRotationHandleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void UPivotRotationGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, class UActorComponent* DraggingHandle,
	const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation
	Super::UpdateGizmoHandleGroup(LocalToWorld, LocalBounds, ViewLocation, DraggingHandle, HoveringOverHandles, AnimationAlpha,
		GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup);

	bool bShowFullRotationDragHandle = false;
	const bool bShowRotationHandles = DraggingHandle == nullptr ? true : false;
	const ECoordSystem CoordSystem = OwningTransformGizmoActor->GetOwnerWorldInteraction()->GetTransformGizmoCoordinateSpace();

	for (int32 HandleIndex = 0; HandleIndex < Handles.Num(); ++HandleIndex)
	{
		FGizmoHandle& Handle = Handles[HandleIndex];
		UStaticMeshComponent* GizmoHandleMeshComponent = Handle.HandleMesh;
		if (GizmoHandleMeshComponent != nullptr)	// Can be null if no handle for this specific placement
		{
			GizmoHandleMeshComponent->SetVisibility(bShowRotationHandles);

			int32 CenterHandleCount, FacingAxisIndex, CenterAxisIndex;
			const FTransformGizmoHandlePlacement HandlePlacement = MakeHandlePlacementForIndex(HandleIndex);
			HandlePlacement.GetCenterHandleCountAndFacingAxisIndex(/* Out */ CenterHandleCount, /* Out */ FacingAxisIndex, /* Out */ CenterAxisIndex);

			if (DraggingHandle != nullptr && DraggingHandle == GizmoHandleMeshComponent)
			{
				bShowFullRotationDragHandle = true;
				if (!StartActorRotation.IsSet())
				{
					StartActorRotation = OwningTransformGizmoActor->GetTransform().GetRotation();
				}

				if (CoordSystem == ECoordSystem::COORD_Local)
				{
					OwningTransformGizmoActor->SetActorRotation(StartActorRotation.GetValue());
				}

				const FVector GizmoSpaceFacingAxisVector = GetAxisVector(FacingAxisIndex, HandlePlacement.Axes[FacingAxisIndex]);
				FullRotationHandleMeshComponent->SetRelativeTransform(FTransform(GizmoSpaceFacingAxisVector.ToOrientationQuat(), FVector::ZeroVector, FVector(GizmoScale * AnimationAlpha)));
			}
			else if(bShowRotationHandles)
			{
				int32 UpAxisIndex = 0;
				int32 RightAxisIndex = 0;
				FRotator Rotation;

				if (FacingAxisIndex == 0)
				{
					// X, up = Z, right = Y
					UpAxisIndex = 2;
					RightAxisIndex = 1;
					Rotation = FRotator::ZeroRotator;
				}
				else if (FacingAxisIndex == 1)
				{
					// Y, up = Z, right = X
					UpAxisIndex = 2;
					RightAxisIndex = 0;
					Rotation = FRotator(0, -90, 0);

				}
				else if (FacingAxisIndex == 2)
				{
					// Z, up = Y, right = X
					UpAxisIndex = 0;
					RightAxisIndex = 1;
					Rotation = FRotator(-90, 0, 0);
				}

				// Check on which side we are relative to the gizmo
				const FVector GizmoSpaceViewLocation = GetOwner()->GetTransform().InverseTransformPosition(ViewLocation);
				if (GizmoSpaceViewLocation[UpAxisIndex] < 0 && GizmoSpaceViewLocation[RightAxisIndex] < 0)
				{
					Rotation.Roll = 180;
				}
				else if (GizmoSpaceViewLocation[UpAxisIndex] < 0)
				{
					Rotation.Roll = 90;
				}
				else if (GizmoSpaceViewLocation[RightAxisIndex] < 0)
				{
					Rotation.Roll = -90;
				}

				// Set the final transform
				GizmoHandleMeshComponent->SetRelativeTransform(FTransform(Rotation, FVector::ZeroVector, FVector(GizmoScale * AnimationAlpha )));

				// Update material
				UpdateHandleColor(FacingAxisIndex, Handle, DraggingHandle, HoveringOverHandles);
			}
		}
	}

	// Show or hide the visuals for full rotation and set the collision accordingly
	FullRotationHandleMeshComponent->SetCollisionEnabled(bShowFullRotationDragHandle == true ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	FullRotationHandleMeshComponent->SetVisibility(bShowFullRotationDragHandle);

	if (!bShowFullRotationDragHandle)
	{
		StartActorRotation.Reset();
	}
}

ETransformGizmoInteractionType UPivotRotationGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::RotateOnAngle;
}

EGizmoHandleTypes UPivotRotationGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Rotate;
}
