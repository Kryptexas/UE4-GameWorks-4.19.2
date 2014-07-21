// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


struct FActorPositionTraceResult
{
	/** Enum representing the state of this result */
	enum ResultState
	{
		/** The trace found a valid hit target */
		HitSuccess,
		/** The trace found no valid targets, so chose a default position */
		Default,
		/** The trace failed entirely */
		Failed,
	};

	/** Constructor */
	FActorPositionTraceResult() : State(Failed), Location(0.f), SurfaceNormal(0.f) {}

	/** The state of this result */
	ResultState	State;

	/** The location of the preferred trace hit */
	FVector		Location;

	/** The surface normal of the trace hit */
	FVector		SurfaceNormal;

	/** Pointer to the actor that was hit, if any. nullptr otherwise */
	TWeakObjectPtr<AActor>	HitActor;
};

struct FActorPositioning
{
	/** Trace the specified world to find a position to snap actors to
	 *
	 *	@param	Cursor			The cursor position and direction to trace at
	 *	@param	View			The scene view that we are tracing on
	 *	@param	IgnoreActors	Optional array of actors to exclude from the trace
	 *
	 *	@return	Result structure containing the location and normal of a trace hit, or a default position in front of the camera on failure
	*/
	static UNREALED_API FActorPositionTraceResult TraceWorldForPositionWithDefault(const FViewportCursorLocation& Cursor, const FSceneView& View, const TArray<AActor*>* IgnoreActors = nullptr);
	
	/** Trace the specified world to find a position to snap actors to
	 *
	 *	@param	Cursor			The cursor position and direction to trace at
	 *	@param	View			The scene view that we are tracing on
	 *	@param	IgnoreActors	Optional array of actors to exclude from the trace
	 *
	 *	@return	Result structure containing the location and normal of a trace hit, or empty on failure
	*/
	static UNREALED_API FActorPositionTraceResult TraceWorldForPosition(const FViewportCursorLocation& Cursor, const FSceneView& View, const TArray<AActor*>* IgnoreActors = nullptr);

	/** Trace the specified world to find a position to snap actors to
	 *
	 *	@param	World			The world to trace
	 *	@param	InSceneView		The scene view that we are tracing on
	 *	@param	RayStart		The start of the ray in world space
	 *	@param	RayEnd			The end of the ray in world space
	 *	@param	IgnoreActors	Optional array of actors to exclude from the trace
	 *
	 *	@return	Result structure containing the location and normal of a trace hit, or empty on failure
	*/
	static UNREALED_API FActorPositionTraceResult TraceWorldForPosition(const UWorld& InWorld, const FSceneView& InSceneView, const FVector& RayStart, const FVector& RayEnd, const TArray<AActor*>* IgnoreActors = nullptr);

	/** Get a transform that should be used to spawn the specified actor using the global editor click location and plane
	 *
	 *	@param	Actor			The actor to spawn
	 *	@param	bSnap			Whether to perform snapping on the placement transform or not (defaults to true)
	 *
	 *	@return	The expected transform for the specified actor
	*/
	static UNREALED_API FTransform GetCurrentViewportPlacementTransform(const AActor& Actor, bool bSnap = true);

	/** Get a default actor position in front of the camera
	 *
	 *	@param	InActor				The actor
	 *	@param	InCameraOrigin		The location of the camera
	 *	@param	InCameraDirection	The view direction of the camera
	 *
	 *	@return	The expected transform for the specified actor
	*/
	static UNREALED_API FVector GetActorPositionInFrontOfCamera(const AActor& InActor, const FVector& InCameraOrigin, const FVector& InCameraDirection);
	
	/**
	 *	Get the snapped position and rotation transform for an actor to snap it to the specified surface. This will potentially align the actor to the specified surface normal, and offset it based on factory settings
	 *
	 *	@param	InFactory				The factory used to spawn the actor. Factories define how actors are aligned and offset.
	 *	@param	InSurfaceLocation		The location on the surface to transform to
	 *	@param	InSurfaceNormal			The surface normal of the spawn position
	 *	@param	InPlacementExtent		(optional) Optional actor placement extent
	 *	@param	InStartTransform		(optional) Optional initial transform. This will result in a transformation from InStartTransform, to the surface transform
	 *
	 *	@return	The expected transform for the specified actor
	 */
	static UNREALED_API FTransform GetSnappedSurfaceAlignedTransform(FLevelEditorViewportClient* InViewportClient, const UActorFactory* InFactory, FVector InSurfaceLocation, const FVector& InSurfaceNormal, const FVector& InPlacementExtent = FVector(0.f), const FTransform& InStartTransform = FTransform::Identity);

	/**
	 *	Get the position and rotation transform for an actor to snap it to the specified surface. This will potentially align the actor to the specified surface normal, and offset it.
	 *
	 *	@param	InFactory				The factory used to spawn the actor. Factories define how actors are aligned and offset.
	 *	@param	InSurfaceLocation		The location on the surface to transform to
	 *	@param	InSurfaceNormal			The surface normal of the spawn position
	 *	@param	InPlacementExtent		(optional) Optional actor placement extent
	 *	@param	InStartTransform		(optional) Optional initial transform. This will result in a transformation from InStartTransform, to the surface transform
	 *
	 *	@return	The expected transform for the specified actor
	 */
	static UNREALED_API FTransform GetSurfaceAlignedTransform(const UActorFactory* InFactory, const FVector& InSurfaceLocation, const FVector& InSurfaceNormal, const FVector& InPlacementExtent = FVector(0.f), const FTransform& InStartTransform = FTransform::Identity);
};