// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VREditorBaseTransformGizmo.h"
#include "VREditorGizmoHandle.h"
#include "VREditorPivotTransformGizmo.generated.h"

/**
 * A transform gizmo on the pivot that allows you to interact with selected objects by moving, scaling and rotating.
 */
UCLASS()
class APivotTransformGizmo : public ABaseTransformGizmo
{
	GENERATED_BODY()

public:

	/** Default constructor that sets up CDO properties */
	APivotTransformGizmo();

	/** Called by the world interaction system after we've been spawned into the world, to allow
	    us to create components and set everything up nicely for the selected objects that we'll be
		used to manipulate */
	virtual void UpdateGizmo( const EGizmoHandleTypes GizmoType, const ECoordSystem GizmoCoordinateSpace, const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, 
		bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, const float GizmoHoverScale, const float GizmoHoverAnimationDuration ) override;

private:
	/** Uniform scale handle group component */
	UPROPERTY()
	class UVREditorUniformScaleGizmoHandleGroup* UniformScaleGizmoHandleGroup;

	/** Translation handle group component */
	UPROPERTY()
	class UVREditorPivotTranslationGizmoHandleGroup* TranslationGizmoHandleGroup;
	
	/** Scale handle group component */
	UPROPERTY()
	class UVREditorPivotScaleGizmoHandleGroup* ScaleGizmoHandleGroup;
	
	/** Plane translation handle group component */
	UPROPERTY()
	class UVREditorPivotPlaneTranslationGizmoHandleGroup* PlaneTranslationGizmoHandleGroup;

	/** Rotation handle group component */
	UPROPERTY()
	class UVREditorPivotRotationGizmoHandleGroup* RotationGizmoHandleGroup;

	/** Stretch handle group component */
	UPROPERTY()
	class UVREditorStretchGizmoHandleGroup* StretchGizmoHandleGroup;
};

/**
 * Axis Gizmo handle for translating
 */
UCLASS()
class UVREditorPivotTranslationGizmoHandleGroup : public UVREditorAxisGizmoHandleGroup
{
	GENERATED_BODY()

public:

	/** Default constructor that sets up CDO properties */
	UVREditorPivotTranslationGizmoHandleGroup();

	/** Updates the gizmo handles */
	virtual void UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, 
		float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup ) override;

	/** Gets the InteractionType for this Gizmo handle */
	virtual ETransformGizmoInteractionType GetInteractionType() const override;

	/** Gets the GizmoType for this Gizmo handle */
	virtual EGizmoHandleTypes GetHandleType() const override;
};

/**
 * Axis Gizmo handle for scaling
 */
UCLASS()
class UVREditorPivotScaleGizmoHandleGroup : public UVREditorAxisGizmoHandleGroup
{
	GENERATED_BODY()

public:

	/** Default constructor that sets up CDO properties */
	UVREditorPivotScaleGizmoHandleGroup();


	/** Updates the gizmo handles */
	virtual void UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds,  const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, 
		float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup ) override;

	/** Gets the InteractionType for this Gizmo handle */
	virtual ETransformGizmoInteractionType GetInteractionType() const override;

	/** Gets the GizmoType for this Gizmo handle */
	virtual EGizmoHandleTypes GetHandleType() const override;

	/** Returns true if this type of handle is allowed with world space gizmos */
	virtual bool SupportsWorldCoordinateSpace() const override;
};

/**
 * Axis Gizmo handle for plane translation
 */
UCLASS()
class UVREditorPivotPlaneTranslationGizmoHandleGroup : public UVREditorAxisGizmoHandleGroup
{
	GENERATED_BODY()

public:

	/** Default constructor that sets up CDO properties */
	UVREditorPivotPlaneTranslationGizmoHandleGroup();


	/** Updates the gizmo handles */
	virtual void UpdateGizmoHandleGroup( const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, 
		float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup ) override;

	/** Gets the InteractionType for this Gizmo handle */
	virtual ETransformGizmoInteractionType GetInteractionType() const override;

	/** Gets the GizmoType for this Gizmo handle */
	virtual EGizmoHandleTypes GetHandleType() const override;
};


/**
 * Axis Gizmo handle for rotation
 */
UCLASS()
class UVREditorPivotRotationGizmoHandleGroup : public UVREditorAxisGizmoHandleGroup
{
	GENERATED_BODY()

public:

	/** Default constructor that sets up CDO properties */
	UVREditorPivotRotationGizmoHandleGroup();

	/** Updates the gizmo handles */
	virtual void UpdateGizmoHandleGroup(const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, 
		float AnimationAlpha, float GizmoScale, const float GizmoHoverScale, const float GizmoHoverAnimationDuration, bool& bOutIsHoveringOrDraggingThisHandleGroup ) override;

	/** Gets the InteractionType for this Gizmo handle */
	virtual ETransformGizmoInteractionType GetInteractionType() const override;

	/** Gets the GizmoType for this Gizmo handle */
	virtual EGizmoHandleTypes GetHandleType() const override;
};