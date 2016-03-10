// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VREditorWorldInteractionTypes.h"


/**
 * VR Editor interaction with the 3D world
 */
class FVREditorWorldInteraction : public FGCObject
{
public:

	/** Default constructor for FVREditorWorldInteraction, which sets up safe defaults */
	FVREditorWorldInteraction( class FVREditorMode& InitOwner );

	/** Clean up before destruction */
	virtual ~FVREditorWorldInteraction();

	// FGCObject overrides
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;

	/** Called by VREditorMode to update us every frame */
	void Tick( class FEditorViewportClient* ViewportClient, const float DeltaTime );

	/** Called by VREditorMode to draw debug graphics every frame */
	void Render( const class FSceneView* SceneView, class FViewport* Viewport, class FPrimitiveDrawInterface* PDI );

	/** Gets the transform gizmo actor, or returns null if we currently don't have one */
	class ATransformGizmo* GetTransformGizmoActor() const
	{
		return TransformGizmoActor;
	}

	/** Deletes whatever is selected right now. */
	void DeleteSelectedObjects();

	/** Called by our owner right before a map is loaded or switching between Simulate and normal Editor, so we can clean
	    up our spawned actors */
	void CleanUpActorsBeforeMapChangeOrSimulate();

	/** Returns true if we're actively teleporting */
	bool IsTeleporting() const
	{
		return bIsTeleporting;
	}

	/** Creates a Transformable for each selected actor, wiping out the existing transformables */
	void SetupTransformablesForSelectedActors();

	/** Gets the Y delta of the trackpad */
	float GetTrackpadSlideDelta( const int32 HandIndex );

protected:

	/** Called when the user presses a button on their motion controller device */
	void OnVRAction( FEditorViewportClient& ViewportClient, const FVRAction VRAction, const EInputEvent Event, bool& bIsInputCaptured, bool& bOutWasHandled );

	/** Called every frame to update hover state */
	void OnVRHoverUpdate( FEditorViewportClient& ViewportClient, const int32 HandIndex, FVector& HoverImpactPoint, bool& bIsHoveringOverUI, bool& bOutWasHandled );

	/** Called when the world is "dropped" with a hand */
	void StopGrabbingWorld( struct FVirtualHand& VirtualHand );

	/** Called when selection changes in the level */
	void OnActorSelectionChanged( UObject* ChangedObject );

	/** Checks to see if we're allowed to interact with the specified component */
	bool IsInteractableComponent( const UActorComponent* Component ) const;

	/** Called by the world interaction system when one of our components is dragged by the user.  If null is
	    passed in then we'll treat it as dragging the whole object (rather than a specific axis/handle) */
	void UpdateDragging( 
		const float DeltaTime, 
		bool& bIsFirstDragUpdate, 
		const EVREditorDraggingMode DraggingMode, 
		const ETransformGizmoInteractionType InteractionType, 
		const bool bWithTwoHands, 
		class FEditorViewportClient& ViewportClient, 
		const TOptional<FTransformGizmoHandlePlacement> OptionalHandlePlacement, 
		const FVector& DragDelta, 
		const FVector& OtherHandDragDelta, 
		const FVector& DraggedTo, 
		const FVector& OtherHandDraggedTo, 
		const FVector& DragDeltaFromStart, 
		const FVector& OtherHandDragDeltaFromStart, 
		const FVector& LaserPointerStart, 
		const FVector& LaserPointerDirection, 
		const bool bIsLaserPointerValid, 
		const FTransform& GizmoStartTransform, 
		const FBox& GizmoStartLocalBounds, 
		FVector& GizmoSpaceFirstDragUpdateOffsetAlongAxis,
		FVector& DragDeltaFromStartOffset,
		bool& bIsDrivingVelocityOfSimulatedTransformables,
		FVector& OutUnsnappedDraggedTo );

	/** Given a drag delta from a starting point, contrains that delta based on a gizmo handle axis */
	FVector ComputeConstrainedDragDeltaFromStart( const bool bIsFirstDragUpdate, const ETransformGizmoInteractionType InteractionType, const TOptional<FTransformGizmoHandlePlacement> OptionalHandlePlacement, const FVector& DragDeltaFromStart, const FVector& LaserPointerStart, const FVector& LaserPointerDirection, const bool bIsLaserPointerValid, const FTransform& GizmoStartTransform, FVector& GizmoSpaceFirstDragUpdateOffsetAlongAxis, FVector& DragDeltaFromStartOffset ) const;

	/** Refreshes the transform gizmo to match the currently selected actor set */
	void RefreshTransformGizmo( const bool bNewObjectsSelected, bool bAllHandlesVisible, class UActorComponent* SingleVisibleHandle, const TArray< UActorComponent* >& HoveringOverHandles );

	/** Starts dragging selected actors around, changing selection if needed.  Called when clicking in the world, or when placing new objects */
	void StartDraggingActors( FVRAction VRAction, UActorComponent* ClickedComponent, const FVector HitLocation, const bool bIsPlacingActors );

	/** Starts dragging a material, allowing the user to drop it on an object in the scene to place it */
	void StartDraggingMaterialOrTexture( FVRAction VRAction, const FVector HitLocation, UObject* MaterialOrTextureAsset );

	/** Tries to place whatever material or texture that's being dragged on the object under the hand's laser pointer */
	void PlaceDraggedMaterialOrTexture( const int32 HandIndex );

	/** Called to finish a drag action with the specified hand */
	void StopDragging( const int32 HandIndex );

	/** True if we're actively interacting with the specified actor */
	bool IsTransformingActor( AActor* Actor ) const;

	/** Called when FEditorDelegates::OnAssetDragStarted is broadcast */
	void OnAssetDragStartedFromContentBrowser( const TArray<FAssetData>& DraggedAssets, class UActorFactory* FactoryToUse );

	/** Figures out where to place an object when tracing it against the scene using a laser pointer */
	bool FindPlacementPointUnderLaser( const int32 HandIndex, FVector& OutHitLocation );
	
	/** Start teleporting, does a ray trace with the hand passed and calculates the locations for lerp movement in Teleport */
	void StartTeleport( const int32 HandIndex );

	/** Actually teleport to a position */
	void Teleport( const float DeltaTime );

	/** Checks to see if smooth snapping is currently enabled */
	bool IsSmoothSnappingEnabled() const;


protected:

	/** Owning object */
	class FVREditorMode& Owner;


	//
	// Undo/redo
	//

	/** Manages saving undo for selected actors while we're dragging them around */
	FTrackingTransaction TrackingTransaction;


	//
	// Object movement/placement
	//

	/** All of the objects we're currently interacting with, such as selected actors */
	TArray< TUniquePtr< class FVREditorTransformable > > Transformables;

	/** True if our transformables are currently interpolating to their target location */
	bool bIsInterpolatingTransformablesFromSnapshotTransform;

	/** Time that we started interpolating at */
	FTimespan TransformablesInterpolationStartTime;

	/** Duration to interpolate transformables over */
	float TransformablesInterpolationDuration;

	/** The material or texture asset we're dragging to place on an object */
	UObject* PlacingMaterialOrTextureAsset;

	//
	// World movement
	//

	/** The world to meters scale before we changed it (last frame's world to meters scale) */
	float LastWorldToMetersScale;


	//
	// Transform gizmo
	//

	/** Transform gizmo actor, for manipulating selected actor(s) */
	class ATransformGizmo* TransformGizmoActor;

	/** Current gizmo bounds in local space.  Updated every frame.  This is not the same as the gizmo actor's bounds. */
	FBox GizmoLocalBounds;

	/** True if we've dragged objects with either hand since the last time we selected something */
	bool bDraggedSinceLastSelection;

	/** Gizmo start transform of the last drag we did with either hand.  Only valid if bDraggedSinceLastSelection is true */
	FTransform LastDragGizmoStartTransform;



	//
	// Hover state
	//

	/** Set of objects that are currently hovered over */
	TSet<FViewportHoverTarget> HoveredObjects;

	//
	// Teleport
	//

	/** If we are currently teleporting */
	bool bIsTeleporting;

	/** The current lerp of the teleport between the TeleportStartLocation and the TeleportGoalLocation */
	float TeleportLerpAlpha;

	/** Set on the current Roomspace location in the world in StartTeleport before doing the actual teleporting */
	FVector TeleportStartLocation;

	/** The calculated goal location in StartTeleport to move the Roomspace to */
	FVector TeleportGoalLocation;

	/** So we can teleport on a light press and make sure a second teleport by a fullpress doesn't happen */
	bool bJustTeleported;

	//
	// Sound
	//

	/** Teleport sound */
	class USoundCue* TeleportSound;

	/** Sound for dropping materials and textures */
	class USoundCue* DropMaterialOrMaterialSound;

	/** The distance when starting dragging docks */
	float DockSelectDistance;

	/** The UI used to drag an asset into the level */
	class UWidgetComponent* FloatingUIAssetDraggedFrom;
};

