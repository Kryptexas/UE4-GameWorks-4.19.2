// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VREditorWorldInteractionTypes.generated.h"


/** Methods of dragging objects around in VR */
UENUM()
enum class EVREditorDraggingMode : uint8
{
	/** Not dragging right now with this hand */
	Nothing,

	/** Dragging actors around using the transform gizmo */
	ActorsWithGizmo,

	/** Actors locked to the impact point under the laser */
	ActorsAtLaserImpact,

	/** We're grabbing an object (or the world) that was already grabbed by the other hand */
	AssistingDrag,

	/** Freely moving, rotating and scaling objects with one or two hands */
	ActorsFreely,

	/** Moving the world itself around (actually, moving the camera in such a way that it feels like you're moving the world) */
	World,

	/** Dragging a material to place it on an object under the laser */
	Material,

	/** Moving a dockable window */
	DockableWindow,
};


/**
 * Things the transform gizmo can do to objects
 */
UENUM()
enum class ETransformGizmoInteractionType : uint8
{
	/** No interaction */
	None,

	/** Translate the object(s), either in any direction or confined to one axis.  No rotation. */
	Translate,

	/** Translate the object, constrained to a plane.  No rotation. */
	TranslateOnPlane,

	/** Stretch the object non-uniformly while simultaneously repositioning it so account for it's new size */
	StretchAndReposition,

	/** Rotate the object(s) around a specific axis relative to the gizmo's pivot, translating as needed */
	Rotate,

	/** Uniform scale the object(s) */
	UniformScale,
};


/* Directions that a transform handle can face along any given axis */
enum class ETransformGizmoHandleDirection
{
	Negative,
	Center,
	Positive,
};


/** Placement of a handle in pivot space */
struct FTransformGizmoHandlePlacement
{
	/* Handle direction in X, Y and Z */
	ETransformGizmoHandleDirection Axes[ 3 ];


	/** Finds the center handle count and facing axis index.  The center handle count is simply the number
	of axes where the handle would be centered on the bounds along that axis.  The facing axis index is
	the index (0-2) of the axis where the handle would be facing, or INDEX_NONE for corners or edges.
	The center axis index is valid for edges, and defines the axis perpendicular to that edge direction,
	or INDEX_NONE if it's not an edge */
	void GetCenterHandleCountAndFacingAxisIndex( int32& OutCenterHandleCount, int32& OutFacingAxisIndex, int32& OutCenterAxisIndex ) const;
};


