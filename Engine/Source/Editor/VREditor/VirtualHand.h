// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VREditorWorldInteractionTypes.h"



/** Represents a single virtual hand */
struct FVirtualHand
{

	//
	// Positional data
	//

	/** Your hand in the virtual world in world space, usually driven by VR motion controllers */
	FTransform Transform;

	/** Your hand transform, in the local tracking space */
	FTransform RoomSpaceTransform;

	/** Hand transform in world space from the previous frame */
	FTransform LastTransform;

	/** Room space hand transform from the previous frame */
	FTransform LastRoomSpaceTransform;


	//
	// Motion controllers
	//

	/** True if this hand has a motion controller (or both!) */
	bool bHaveMotionController;


	//
	// Graphics
	//
	
	/** Motion controller component which handles late-frame transform updates of all parented sub-components */
	class UMotionControllerComponent* MotionControllerComponent;

	/** Mesh for this hand */
	class UStaticMeshComponent* HandMeshComponent;

	/** Mesh for this hand's laser pointer */
	class UStaticMeshComponent* LaserPointerMeshComponent;

	/** MID for laser pointer material (opaque parts) */
	class UMaterialInstanceDynamic* LaserPointerMID;

	/** MID for laser pointer material (translucent parts) */
	class UMaterialInstanceDynamic* TranslucentLaserPointerMID;

	/** Hover impact indicator mesh */
	class UStaticMeshComponent* HoverMeshComponent;

	/** Hover point light */
	class UPointLightComponent* HoverPointLightComponent;

	/** MID for hand mesh */
	class UMaterialInstanceDynamic* HandMeshMID;

	//
	// Hover feedback
	//

	/** Lights up when our laser pointer is close enough to something to click */
	bool bIsHovering;

	/** True if we're hovering over UI right now.  When hovering over UI, we don't bother drawing a see-thru laser pointer */
	bool bIsHoveringOverUI;

	/** The widget component we are currently hovering over */
	class UWidgetComponent* HoveringOverWidgetComponent;

	/** Position the laser pointer impacted an interactive object at (UI, meshes, etc.) */
	FVector HoverLocation;


	//
	// Trigger axis state
	//

	/** True if trigger is fully pressed right now (or within some small threshold) */
	bool bIsTriggerFullyPressed;

	/** True if the trigger is currently pulled far enough that we consider it in a "half pressed" state */
	bool bIsTriggerAtLeastLightlyPressed;

	/** True if we allow locking of the lightly pressed state for the current event.  This can be useful to
	    turn off in cases where you always want a full press to override the light press, even if it was
		a very slow press */
	bool bAllowTriggerLightPressLocking;

	/** Real time that the trigger became lightly pressed.  If the trigger remains lightly pressed for a reasonably 
	    long duration before being pressed fully, we'll continue to treat it as a light press in some cases */
	double RealTimeTriggerWasLightlyPressed;

	/** True if trigger has been fully released since the last press */
	bool bHasTriggerBeenReleasedSinceLastPress;


	//
	// Trackpad support
	//

	/** True if the trackpad is actively being touched */
	bool bIsTouchingTrackpad;

	/** Position of the touched trackpad */
	FVector2D TrackpadPosition;

	/** Last position of the touched trackpad */
	FVector2D LastTrackpadPosition;

	/** True if we have a valid trackpad position (for each axis) */
	bool bIsTrackpadPositionValid[2];

	/** Real time that the last trackpad position was last updated.  Used to filter out stale previous data. */
	FTimespan LastTrackpadPositionUpdateTime;


	//
	// General input
	//

	/** Is the Modifier button held down? */
	bool bIsModifierPressed;

	/** True if this hand is "captured" for each possible action type, meaning that only the active captor should 
	    handle input events until it is no longer captured.  It's the captors responsibility to set this using 
		OnVRAction(), or clear it when finished with capturing. */
	bool IsInputCaptured[ (int32)EVRActionType::TotalActionTypes ];

	/** True if we're currently holding the 'SelectAndMove' button down after clicking on an actor */
	TWeakObjectPtr<UActorComponent> ClickingOnComponent;

	/** True if we're currently holding the 'SelectAndMove' button down after clicking on UI */
	bool bIsClickingOnUI;

	/** When bIsClickingOnUI is true, this will be true if we're "right clicking".  That is, the Modifier key was held down at the time that the user clicked */
	bool bIsRightClickingOnUI;

	/** Last real time that we released the 'SelectAndMove' button on UI.  This is used to detect double-clicks. */
	double LastClickReleaseTime;

	/** The last real time we played a haptic effect.  This is used to turn off haptic effects shortly after they're triggered. */
	double LastHapticTime;



	//
	// UI
	//
	
	/** True if a floating UI panel is attached to the front of the hand, and we shouldn't bother drawing a laser
	    pointer or enabling certain other features */
	bool bHasUIInFront;

	/** True if a floating UI panel is attached to our forearm, so we shouldn't bother drawing help labels */
	bool bHasUIOnForearm;

	/** Inertial scrolling -- how fast to scroll the mousewheel over UI */
	float UIScrollVelocity;


	//
	// Object/world movement
	// 

	/** What we're currently dragging with this hand, if anything */
	EVREditorDraggingMode DraggingMode;

	/** What we were doing last.  Used for inertial movement. */
	EVREditorDraggingMode LastDraggingMode;

	/** True if this is the first update since we started dragging */
	bool bIsFirstDragUpdate;

	/** True if we were assisting the other hand's drag the last time we did anything.  This is used for inertial movement */
	bool bWasAssistingDrag;

	/** Length of the ray that's dragging */
	float DragRayLength;

	/** Laser pointer start location the last frame */
	FVector LastLaserPointerStart;

	/** Location that we dragged to last frame (end point of the ray) */
	FVector LastDragToLocation;

	/** Laser pointer impact location at the drag start */
	FVector LaserPointerImpactAtDragStart;

	/** How fast to move selected objects every frame for inertial translation */
	FVector DragTranslationVelocity;

	/** How fast to adjust ray length every frame for inertial ray length changes */
	float DragRayLengthVelocity;

	/** While dragging, true if we're dragging at least one simulated object that we're driving the velocities of.  When this
	    is true, our default inertia system is disabled and we rely on the physics engine to take care of inertia */
	bool bIsDrivingVelocityOfSimulatedTransformables;


	//
	// Transform gizmo interaction
	//

	/** Where the gizmo was placed at the beginning of the current interaction */
	FTransform GizmoStartTransform;

	/** Our gizmo bounds at the start of the interaction, in actor local space. */
	FBox GizmoStartLocalBounds;

	/** For a single axis drag, this is the cached local offset where the laser pointer ray intersected the axis line on the first frame of the drag */
	FVector GizmoSpaceFirstDragUpdateOffsetAlongAxis;

	/** When dragging with an axis/plane constraint applied, this is the difference between the actual "delta from start" and the constrained "delta from start".
		This is used when the user releases the object and inertia kicks in */
	FVector GizmoSpaceDragDeltaFromStartOffset;

	/** The gizmo interaction we're doing with this hand */
	ETransformGizmoInteractionType TransformGizmoInteractionType;

	/** Which handle on the gizmo we're interacting with, if any */
	TOptional<FTransformGizmoHandlePlacement> OptionalHandlePlacement;

	/** The gizmo component we're dragging right now */
	TWeakObjectPtr<class UActorComponent> DraggingTransformGizmoComponent;

	/** Gizmo component that we're hovering over, or nullptr if not hovering over any */
	TWeakObjectPtr<class UActorComponent> HoveringOverTransformGizmoComponent;

	/** Gizmo handle that we hovered over last (used only for avoiding spamming of hover haptics!) */
	TWeakObjectPtr<class UActorComponent> HoverHapticCheckLastHoveredGizmoComponent;

	/** Last real time that we played hover haptic feedback for a gizmo handle.  This is just to avoid spamming haptic effects every frame due to motion jitter */
	FTimespan LastGizmoHandleHapticTime;


	//
	// Help
	//

	/** True if we want help labels to be visible right now, otherwise false */
	bool bWantHelpLabels;

	/** Help labels for buttons on the motion controllers */
	TMap< FKey, class AFloatingText* > HelpLabels;

	/** Time that we either started showing or hiding help labels (for fade transitions) */
	FTimespan HelpLabelShowOrHideStartTime;

	/** The current component hovered by the laser pointer of this hand */
	TWeakObjectPtr<class UActorComponent> HoveredActorComponent;

	/** Default constructor for FVirtualHand that initializes safe defaults */
	FVirtualHand()
	{
		bHaveMotionController = false;

		MotionControllerComponent = nullptr;
		HandMeshComponent = nullptr;
		LaserPointerMeshComponent = nullptr;
		LaserPointerMID = nullptr;
		TranslucentLaserPointerMID = nullptr;
		HoverMeshComponent = nullptr;
		HoverPointLightComponent = nullptr;

		bIsHovering = false;
		bIsHoveringOverUI = false;
		HoveringOverWidgetComponent = nullptr;
		HoverLocation = FVector::ZeroVector;

		TrackpadPosition = FVector2D::ZeroVector;
		LastTrackpadPosition = FVector2D::ZeroVector;
		bIsTrackpadPositionValid[ 0 ] = false;
		bIsTrackpadPositionValid[ 1 ] = false;
		LastTrackpadPositionUpdateTime = FTimespan::MinValue();

		bIsTriggerFullyPressed = false;
		bIsTriggerAtLeastLightlyPressed = false;
		bAllowTriggerLightPressLocking = true;
		RealTimeTriggerWasLightlyPressed = 0.0;
		bHasTriggerBeenReleasedSinceLastPress = true;

		bIsTouchingTrackpad = false;
		bIsModifierPressed = false;
		for( int32 ActionTypeIndex = 0; ActionTypeIndex < (int32)EVRActionType::TotalActionTypes; ++ActionTypeIndex )
		{
			IsInputCaptured[ ActionTypeIndex ] = false;
		}
		ClickingOnComponent = nullptr;
		bIsClickingOnUI = false;
		bIsRightClickingOnUI = false;
		LastClickReleaseTime = 0.0;
		LastHapticTime = 0.0;
		
		bHasUIInFront = false;
		bHasUIOnForearm = false;
		UIScrollVelocity = 0.0f;

		DraggingMode = EVREditorDraggingMode::Nothing;
		LastDraggingMode = EVREditorDraggingMode::Nothing;
		bWasAssistingDrag = false;
		bIsFirstDragUpdate = false;
		DragRayLength = 0.0f;
		LastLaserPointerStart = FVector::ZeroVector;
		LastDragToLocation = FVector::ZeroVector;
		LaserPointerImpactAtDragStart = FVector::ZeroVector;
		DragTranslationVelocity = FVector::ZeroVector;
		DragRayLengthVelocity = 0.0f;
		bIsDrivingVelocityOfSimulatedTransformables = false;

		GizmoStartTransform = FTransform::Identity;
		GizmoStartLocalBounds = FBox( 0 );
		GizmoSpaceFirstDragUpdateOffsetAlongAxis = FVector::ZeroVector;
		GizmoSpaceDragDeltaFromStartOffset = FVector::ZeroVector;
		TransformGizmoInteractionType = ETransformGizmoInteractionType::None;
		OptionalHandlePlacement.Reset();
		DraggingTransformGizmoComponent = nullptr;
		HoveringOverTransformGizmoComponent = nullptr;
		HoverHapticCheckLastHoveredGizmoComponent = nullptr;
		LastGizmoHandleHapticTime = FTimespan::Zero();

		bWantHelpLabels = false;
		HelpLabelShowOrHideStartTime = FTimespan::MinValue();

		HoveredActorComponent = nullptr;
	}
};


