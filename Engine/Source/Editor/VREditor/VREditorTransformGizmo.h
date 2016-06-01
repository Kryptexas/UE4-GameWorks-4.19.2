// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Gizmo/VREditorBaseTransformGizmo.h"
#include "VREditorTransformGizmo.generated.h"

/**
 * Displays measurements along the bounds of selected objects
 */
USTRUCT()
struct FTransformGizmoMeasurement
{
	GENERATED_BODY()

	/** The text that displays the actual measurement and units */
	UPROPERTY()
	class UTextRenderComponent* MeasurementText;
};


/**
 * A transform gizmo that allows you to interact with selected objects by moving, scaling and rotating.
 */
UCLASS()
class ATransformGizmo : public ABaseTransformGizmo
{
	GENERATED_BODY()

public:

	/** Default constructor that sets up CDO properties */
	ATransformGizmo();

	/** Called by the world interaction system after we've been spawned into the world, to allow
	    us to create components and set everything up nicely for the selected objects that we'll be
		used to manipulate */
	void UpdateGizmo( const EGizmoHandleTypes GizmoType, const ECoordSystem GizmoCoordinateSpace, const FTransform& LocalToWorld, const FBox& LocalBounds, const FVector ViewLocation, bool bAllHandlesVisible, class UActorComponent* DraggingHandle, const TArray< UActorComponent* >& HoveringOverHandles, const float GizmoHoverScale, const float GizmoHoverAnimationDuration );

private:

	/** Measurements, one for each axis */
	UPROPERTY()
	FTransformGizmoMeasurement Measurements[ 3 ];

	UPROPERTY()
	class UVREditorTranslationGizmoHandleGroup* TranslationGizmoHandleGroup;

	UPROPERTY()
	class UVREditorPlaneTranslationGizmoHandleGroup* PlaneTranslationGizmoHandleGroup;

	UPROPERTY()
	class UVREditorRotationGizmoHandleGroup* RotationGizmoHandleGroup;

	UPROPERTY()
	class UVREditorStretchGizmoHandleGroup* StretchGizmoHandleGroup;

	UPROPERTY()
	class UVREditorUniformScaleGizmoHandleGroup* UniformScaleGizmoHandleGroup;
};