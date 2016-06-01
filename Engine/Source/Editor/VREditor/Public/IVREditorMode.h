// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


namespace VREditorConstants
{
	/** The number of virtual hands we support.  Always two. */
	const int32 NumVirtualHands = 2;

	/** Index of the left hand */
	const int32 LeftHandIndex = 0;

	/** Index of the right hand */
	const int32 RightHandIndex = 1;
};


/**
 * Types of actions that can be performed with VR controller devices
 */
enum class EVRActionType
{
	NoAction,
	Undo,
	Redo,
	Touch,
	Modifier,
	WorldMovement,
	SelectAndMove,
	SelectAndMove_LightlyPressed,
	Delete,
	ConfirmRadialSelection,
	TrackpadPositionX,
	TrackpadPositionY,
	TriggerAxis,
	// ...
	TotalActionTypes
};


/**
 * Represents a generic action bound to a specific VR "hand"
 */
struct FVRAction
{
	FVRAction( EVRActionType InActionType, int32 InHandIndex )
		: ActionType( InActionType )
		, HandIndex( InHandIndex )
	{}

	EVRActionType ActionType;
	int32 HandIndex;
};


/**
 * VR Editor Mode public interface
 */
class IVREditorMode : public FEdMode
{

public:

	/**
	 * Gets the start and end point of the laser pointer for the specified hand
	 *
	 * @param HandIndex				Index of the hand to use
	 * @param LasertPointerStart	(Out) The start location of the laser pointer in world space
	 * @param LasertPointerEnd		(Out) The end location of the laser pointer in world space
	 * @param bEvenIfUIIsInFront	If true, returns a laser pointer even if the hand has UI in front of it (defaults to false)
	 * @param LaserLengthOverride	If zero the default laser length (VREdMode::GetLaserLength) is used
	 *
	 * @return	True if we have motion controller data for this hand and could return a valid result
	 */
	virtual bool GetLaserPointer( const int32 HandIndex, FVector& LaserPointerStart, FVector& LaserPointerEnd, const bool bEvenIfUIIsInFront = false, const float LaserLengthOverride = 0.0f ) const = 0;

	/**
	 * Gets our avatar's mesh actor
	 *
	 * @return	The mesh actor
	 */
	virtual class AActor* GetAvatarMeshActor() = 0;

	/**
	 * Checks to see if input is captured for the specified action
	 *
	 * @param	VRAction	The VR action that might be captured (the action type, and hand)
	 *
	 * @return	True if input is currently captured for this action
	 */
	virtual bool IsInputCaptured( const FVRAction VRAction ) const = 0;

	/** Event that's triggered when the user presses a button on their motion controller device.  Check bIsInputCaptured first, 
	    to see whether you should actually handle input.  Set bIsInputCaptured to true if you want to capture input, or clear it
	    when finished capturing, if you were the captor.  Set bWasHandled to true if you reacted to the event.  Remember to check
		bWasHandled yourself first, as there may be sibling subscribers that handle the event before you! */
	DECLARE_EVENT_FiveParams( IVREditorMode, FOnVRAction, class FEditorViewportClient& /* ViewportClient */, const FVRAction /* VRAction */, const EInputEvent /* Event */, bool& /* bIsInputCaptured */, bool& /* bWasHandled */ );
	virtual FOnVRAction& OnVRAction() = 0;

	/** Event that fires every frame to update hover state based on what's under the cursor.  Set bWasHandled to true if you detect 
	    something to hover, and set HoverImpactPoint to the world space location you hit.  Remember to check bWasHandled yourself 
		first, as there may be sibling subscribers that handle the event before you! */
	DECLARE_EVENT_FiveParams( IVREditorMode, FOnVRHoverUpdate, class FEditorViewportClient& /* ViewportClient */, const int32 /* HandIndex */, FVector& /* HoverImpactPoint */, bool& /* bIsHoveringOverUI */, bool& /* bWasHandled */ );
	virtual FOnVRHoverUpdate& OnVRHoverUpdate() = 0;
};

