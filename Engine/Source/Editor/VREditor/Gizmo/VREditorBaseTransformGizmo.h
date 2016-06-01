// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VREditorWorldInteractionTypes.h"
#include "GameFramework/Actor.h"
#include "VREditorBaseTransformGizmo.generated.h"


UENUM()
enum class EGizmoHandleTypes : uint8
{
	All = 0,
	Translate = 1,
	Rotate = 2,
	Scale = 3
};


/**
 * Base class for transform gizmo
 */
UCLASS( Abstract )
class ABaseTransformGizmo : public AActor
{
	GENERATED_BODY()

public:
	
	/** Default constructor that sets up CDO properties */
	ABaseTransformGizmo();

	/** Call this when new objects become selected.  This triggers an animation transition. */
	void OnNewObjectsSelected();

	/** Called by the world interaction system when one of our components is dragged by the user to find out
	    what type of interaction to do.  If null is passed in then we'll treat it as dragging the whole object 
		(rather than a specific axis/handle) */
	ETransformGizmoInteractionType GetInteractionType( UActorComponent* DraggedComponent, TOptional<FTransformGizmoHandlePlacement>& OutHandlePlacement );

	/** Updates the animation with the currenttime and selectedtime */
	float GetAnimationAlpha();

	/** Sets the owner */
	void SetOwnerMode( class FVREditorMode* InOwner );

	/** Gets the owner */
	class FVREditorMode* GetOwnerMode() const;

protected:

	/** Makes up a name string for a handle */
	FString MakeHandleName( const FTransformGizmoHandlePlacement HandlePlacement ) const;

	/** Static: Given a bounding box and information about which edge to query, returns the vertex positions of that edge */
	static void GetBoundingBoxEdge( const FBox& Box, const int32 AxisIndex, const int32 EdgeIndex, FVector& OutVertex0, FVector& OutVertex1 );

	/** Updates the visibility of all the handles */
	void UpdateHandleVisibility( const EGizmoHandleTypes GizmoType, const ECoordSystem GizmoCoordinateSpace, bool bAllHandlesVisible, UActorComponent* DraggingHandle );

protected:
	
	/** Real time that the gizmo was last attached to a selected set of objects.  This is used for animation transitions */
	FTimespan SelectedAtTime;
	
	/** Scene component root of this actor */
	UPROPERTY()
	USceneComponent* SceneComponent;

	/** Gizmo material (opaque) */
	UPROPERTY()
	UMaterialInterface* GizmoMaterial;

	/** Gizmo material (translucent) */
	UPROPERTY()
	UMaterialInterface* TranslucentGizmoMaterial;

	/** All gizmo components */
	UPROPERTY()
	TArray< class UVREditorGizmoHandleGroup* > AllHandleGroups;

	/** Owning object */
	class FVREditorMode* Owner;
};