// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorPivotTransformGizmo.h"
#include "VREditorTranslationGizmoHandle.h"
#include "VREditorRotationGizmoHandle.h"
#include "VREditorPlaneTranslationGizmoHandle.h"
#include "VREditorStretchGizmoHandle.h"
#include "VREditorUniformScaleGizmoHandle.h"
#include "VREditorMode.h"

namespace VREd //@todo VREditor: Duplicates of TransformGizmo
{
	// @todo vreditor tweak: Tweak out console variables
	static FAutoConsoleVariable PivotGizmoDistanceScaleFactor( TEXT( "VREd.PivotGizmoDistanceScaleFactor" ), 0.003f, TEXT( "How much the gizmo handles should increase in size with distance from the camera, to make it easier to select"));
	static FAutoConsoleVariable PivotGizmoTranslationPivotOffsetX( TEXT("VREd.PivotGizmoTranslationPivotOffsetX" ), 20.0f, TEXT( "How much the translation cylinder is offsetted from the pivot" ) );
	static FAutoConsoleVariable PivotGizmoScalePivotOffsetX( TEXT( "VREd.PivotGizmoScalePivotOffsetX" ), 120.0f, TEXT( "How much the non-uniform scale is offsetted from the pivot" ) );
	static FAutoConsoleVariable PivotGizmoPlaneTranslationPivotOffsetYZ(TEXT("VREd.PivotGizmoPlaneTranslationPivotOffsetYZ" ), 40.0f, TEXT( "How much the plane translation is offsetted from the pivot" ) );
	static FAutoConsoleVariable PivotGizmoTranslationScaleMultiply( TEXT( "VREd.PivotGizmoTranslationScaleMultiply" ), 2.0f, TEXT( "Multiplies translation handles scale" ) );
	static FAutoConsoleVariable PivotGizmoTranslationHoverScaleMultiply( TEXT( "VREd.PivotGizmoTranslationHoverScaleMultiply" ), 2.5f, TEXT( "Multiplies translation handles hover scale" ) );
}

APivotTransformGizmo::APivotTransformGizmo() :
	Super()
{
	UniformScaleGizmoHandleGroup = CreateDefaultSubobject<UVREditorUniformScaleGizmoHandleGroup>( TEXT( "UniformScaleHandles" ), true );
	UniformScaleGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	UniformScaleGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	UniformScaleGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( UniformScaleGizmoHandleGroup );

	TranslationGizmoHandleGroup = CreateDefaultSubobject<UVREditorPivotTranslationGizmoHandleGroup>(TEXT("TranslationHandles"), true);
	TranslationGizmoHandleGroup->SetTranslucentGizmoMaterial(TranslucentGizmoMaterial);
	TranslationGizmoHandleGroup->SetGizmoMaterial(GizmoMaterial);
	TranslationGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add(TranslationGizmoHandleGroup);

	ScaleGizmoHandleGroup = CreateDefaultSubobject<UVREditorPivotScaleGizmoHandleGroup>( TEXT( "ScaleHandles" ), true );
	ScaleGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	ScaleGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	ScaleGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( ScaleGizmoHandleGroup );

	PlaneTranslationGizmoHandleGroup = CreateDefaultSubobject<UVREditorPivotPlaneTranslationGizmoHandleGroup>( TEXT( "PlaneTranslationHandles" ), true );
	PlaneTranslationGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	PlaneTranslationGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	PlaneTranslationGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( PlaneTranslationGizmoHandleGroup );

	RotationGizmoHandleGroup = CreateDefaultSubobject<UVREditorPivotRotationGizmoHandleGroup>( TEXT( "RotationHandles" ), true );
	RotationGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	RotationGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	RotationGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( RotationGizmoHandleGroup );

	StretchGizmoHandleGroup = CreateDefaultSubobject<UVREditorStretchGizmoHandleGroup>( TEXT( "StretchHandles" ), true );
	StretchGizmoHandleGroup->SetTranslucentGizmoMaterial( TranslucentGizmoMaterial );
	StretchGizmoHandleGroup->SetGizmoMaterial( GizmoMaterial );
	StretchGizmoHandleGroup->SetShowOnUniversalGizmo( false );
	StretchGizmoHandleGroup->SetupAttachment( SceneComponent );
	AllHandleGroups.Add( StretchGizmoHandleGroup );

	// There may already be some objects selected as we switch into VR mode, so we'll pretend that just happened so
	// that we can make sure all transitions complete properly
	OnNewObjectsSelected();
}

void APivotTransformGizmo::UpdateGizmo( const EGizmoHandleTypes GizmoType, const ECoordSystem GizmoCoordinateSpace, const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, 
	UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, const float GizmoHoverScale, const float GizmoHoverAnimationDuration )
{
	const float WorldScaleFactor = GetWorld()->GetWorldSettings()->WorldToMeters / 100.0f;

	// Position the gizmo at the location of the first selected actor
	const bool bSweep = false;
	this->SetActorTransform( LocalToWorld, bSweep );

	// Increase scale with distance, to make gizmo handles easier to click on
	// @todo vreditor: Should probably be a curve, not linear
	// @todo vreditor: Should take FOV into account (especially in non-stereo/HMD mode)
	const float WorldSpaceDistanceToToPivot = FMath::Sqrt( FVector::DistSquared( GetActorLocation(), ViewLocation ) );
	const float GizmoScale( ( ( WorldSpaceDistanceToToPivot / WorldScaleFactor ) * VREd::PivotGizmoDistanceScaleFactor->GetFloat() ) * WorldScaleFactor );

	// Update animation
	float AnimationAlpha = GetAnimationAlpha();

	// Update all the handles
	bool bIsHoveringOrDraggingScaleGizmo = false;
	for ( UVREditorGizmoHandleGroup* HandleGroup : AllHandleGroups )
	{
		if ( HandleGroup != nullptr && 
		   ( DraggingHandle == nullptr || HandleGroup == StretchGizmoHandleGroup ) )
		{
			bool bIsHoveringOrDraggingThisHandleGroup = false;
			HandleGroup->UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, bAllHandlesVisible, DraggingHandle,
				HoveringOverHandles, AnimationAlpha, GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, /* Out */ bIsHoveringOrDraggingThisHandleGroup );
		}
	}

	UpdateHandleVisibility( GizmoType, GizmoCoordinateSpace, bAllHandlesVisible, DraggingHandle );
}

/************************************************************************/
/* Translation                                                          */
/************************************************************************/
UVREditorPivotTranslationGizmoHandleGroup::UVREditorPivotTranslationGizmoHandleGroup() :
	Super()
{
	UStaticMesh* TranslationHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TranslateHandleLong" ) );
		TranslationHandleMesh = ObjectFinder.Object;
		check( TranslationHandleMesh != nullptr );
	}

	CreateHandles( TranslationHandleMesh, FString( "PivotTranslationHandle" ) );
}


void UVREditorPivotTranslationGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, 
	const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, bAllHandlesVisible, DraggingHandle, HoveringOverHandles, AnimationAlpha,
		GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );
	
	const float MultipliedGizmoScale = GizmoScale * VREd::PivotGizmoTranslationScaleMultiply->GetFloat();
	const float MultipliedGizmoHoverScale = GizmoHoverScale * VREd::PivotGizmoTranslationHoverScaleMultiply->GetFloat();

	UpdateHandlesRelativeTransformOnAxis( FTransform( FVector( VREd::PivotGizmoTranslationPivotOffsetX->GetFloat(), 0, 0 ) ), AnimationAlpha, MultipliedGizmoScale, MultipliedGizmoHoverScale, ViewLocation, DraggingHandle, HoveringOverHandles );
}

ETransformGizmoInteractionType UVREditorPivotTranslationGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::Translate;
}

EGizmoHandleTypes UVREditorPivotTranslationGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Translate;
}

/************************************************************************/
/* Scale	                                                            */
/************************************************************************/
UVREditorPivotScaleGizmoHandleGroup::UVREditorPivotScaleGizmoHandleGroup() :
	Super()
{
	UStaticMesh* TranslationHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/UniformScaleHandle" ) );
		TranslationHandleMesh = ObjectFinder.Object;
		check( TranslationHandleMesh != nullptr );
	}

	CreateHandles( TranslationHandleMesh, FString( "PivotScaleHandle" ) );	
}

void UVREditorPivotScaleGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, 
	const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, bAllHandlesVisible, DraggingHandle, HoveringOverHandles, AnimationAlpha, 
		GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );

	UpdateHandlesRelativeTransformOnAxis( FTransform( FVector( VREd::PivotGizmoScalePivotOffsetX->GetFloat(), 0, 0 ) ), AnimationAlpha, GizmoScale, GizmoHoverScale, ViewLocation, DraggingHandle, HoveringOverHandles );
}

ETransformGizmoInteractionType UVREditorPivotScaleGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::Scale;
}

EGizmoHandleTypes UVREditorPivotScaleGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Scale;
}

bool UVREditorPivotScaleGizmoHandleGroup::SupportsWorldCoordinateSpace() const
{
	return false;
}

/************************************************************************/
/* Plane Translation	                                                */
/************************************************************************/
UVREditorPivotPlaneTranslationGizmoHandleGroup::UVREditorPivotPlaneTranslationGizmoHandleGroup() :
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

void UVREditorPivotPlaneTranslationGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle,
	const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup( LocalToWorld, LocalBounds, ViewLocation, bAllHandlesVisible, DraggingHandle, HoveringOverHandles, AnimationAlpha,
		GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );

	UpdateHandlesRelativeTransformOnAxis( FTransform( FVector( 0, VREd::PivotGizmoPlaneTranslationPivotOffsetYZ->GetFloat(), VREd::PivotGizmoPlaneTranslationPivotOffsetYZ->GetFloat() ) ), 
		AnimationAlpha, GizmoScale, GizmoHoverScale, ViewLocation, DraggingHandle, HoveringOverHandles );
}

ETransformGizmoInteractionType UVREditorPivotPlaneTranslationGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::TranslateOnPlane;
}

EGizmoHandleTypes UVREditorPivotPlaneTranslationGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Translate;
}

/************************************************************************/
/* Rotation																*/
/************************************************************************/
UVREditorPivotRotationGizmoHandleGroup::UVREditorPivotRotationGizmoHandleGroup() :
	Super()
{
	UStaticMesh* TranslationHandleMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder(TEXT("/Engine/VREditor/TransformGizmo/RotationHandleFull"));
		TranslationHandleMesh = ObjectFinder.Object;
		check(TranslationHandleMesh != nullptr);
	}

	CreateHandles(TranslationHandleMesh, FString("RotationHandle"));
}

void UVREditorPivotRotationGizmoHandleGroup::UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle,
	const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup )
{
	// Call parent implementation (updates hover animation)
	Super::UpdateGizmoHandleGroup(LocalToWorld, LocalBounds, ViewLocation, bAllHandlesVisible, DraggingHandle, HoveringOverHandles, AnimationAlpha,
		GizmoScale, GizmoHoverScale, GizmoHoverAnimationDuration, bOutIsHoveringOrDraggingThisHandleGroup );
	
	UpdateHandlesRelativeTransformOnAxis( FTransform( FVector( 0, 0, 0 ) ), AnimationAlpha, GizmoScale, GizmoScale, ViewLocation, DraggingHandle, HoveringOverHandles );
}

ETransformGizmoInteractionType UVREditorPivotRotationGizmoHandleGroup::GetInteractionType() const
{
	return ETransformGizmoInteractionType::RotateOnAngle;
}

EGizmoHandleTypes UVREditorPivotRotationGizmoHandleGroup::GetHandleType() const
{
	return EGizmoHandleTypes::Rotate;
}