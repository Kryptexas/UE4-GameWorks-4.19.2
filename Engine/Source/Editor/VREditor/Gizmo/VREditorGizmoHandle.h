// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "VREditorWorldInteractionTypes.h"
#include "VREditorGizmoHandle.generated.h"

namespace VREd
{
	namespace GizmoColor
	{
		const FLinearColor RedGizmoColor( 0.4f, 0.05f, 0.05f, 1.0f );
		const FLinearColor GreenGizmoColor( 0.05f, 0.4f, 0.05f, 1.0f );
		const FLinearColor BlueGizmoColor( 0.05f, 0.05f, 0.4f, 1.0f );
		const FLinearColor WhiteGizmoColor( 0.7f, 0.7f, 0.7f, 1.0f );

		// @todo vreditor: Use configured editor selection color instead
		const FLinearColor HoverGizmoColor( FLinearColor::Yellow );
		const FLinearColor DraggingGizmoColor( FLinearColor::White );
	}
}

enum class EGizmoHandleTypes : uint8;

USTRUCT()
struct FVREditorGizmoHandle
{
	GENERATED_BODY()

	/** Static mesh for this handle */
	class UStaticMeshComponent* HandleMesh;

	/** Scalar that will advance toward 1.0 over time as we hover over the gizmo handle */
	float HoverAlpha;
};


/**
 * Base class for gizmo handles
 */
UCLASS( ABSTRACT )
class UVREditorGizmoHandleGroup : public USceneComponent
{
	GENERATED_BODY()

public:

	/** Default constructor that sets up CDO properties */
	UVREditorGizmoHandleGroup();
	
	/** Given the unique index, makes a handle */
	FTransformGizmoHandlePlacement MakeHandlePlacementForIndex( const int32 HandleIndex ) const;

	/** Makes a unique index for a handle */
	int32 MakeHandleIndex( const FTransformGizmoHandlePlacement HandlePlacement ) const;

	/** Makes up a name string for a handle */
	FString MakeHandleName( const FTransformGizmoHandlePlacement HandlePlacement ) const;

	/** Static: Given an axis (0-2) and a facing direction, returns the vector normal for that axis */
	static FVector GetAxisVector( const int32 AxisIndex, const ETransformGizmoHandleDirection HandleDirection );

	/** Updates the Gizmo handles, needs to be implemented by derived classes */
	virtual void UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup );

	/** Gets the InteractionType and the HandlePlacement for this Gizmo handle */
	virtual void GetHandleIndexInteractionType( const int32 HandleIndex, ETransformGizmoInteractionType& OutInteractionType, TOptional<FTransformGizmoHandlePlacement>& OutHandlePlacement );

	/** Gets the Gizmo InteractionType, needs to be implemented by derived classes */
	virtual ETransformGizmoInteractionType GetInteractionType() const;

	/** Finds the index of DraggedMesh in HandleMeshes */
	virtual int32 GetDraggedHandleIndex( class UStaticMeshComponent* DraggedMesh );

	/** Sets the Gizmo material */
	void SetGizmoMaterial( UMaterialInterface* Material );
	
	/** Sets the translucent Gizmo material */
	void SetTranslucentGizmoMaterial( UMaterialInterface* Material );

	/** Gets all the handles */
	TArray< FVREditorGizmoHandle >& GetHandles();

	/** Gets the GizmoType for this Gizmo handle */
	virtual EGizmoHandleTypes GetHandleType() const;

	/** Returns true if this type of handle is allowed with world space gizmos */
	virtual bool SupportsWorldCoordinateSpace() const
	{
		return true;
	}

protected:

	/** Gizmo material (opaque) */
	UPROPERTY()
	UMaterialInterface* GizmoMaterial;

	/** Gizmo material (translucent) */
	UPROPERTY()
	UMaterialInterface* TranslucentGizmoMaterial;
	
	/** All the StaticMeshes for this handle type */
	UPROPERTY()
	TArray< FVREditorGizmoHandle > Handles;
};