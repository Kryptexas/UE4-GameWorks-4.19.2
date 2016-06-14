// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/ViewportInteraction/Public/ViewportInteractorMotionController.h"
#include "VREditorInteractor.generated.h"

/**
 * VREditor default interactor
 */
UCLASS()
class VREDITOR_API UVREditorInteractor : public UViewportInteractorMotionController
{
	GENERATED_UCLASS_BODY()

public:

	virtual ~UVREditorInteractor();

	/** Initialize default values */
	void Init( const EControllerHand InHandSide, class FVREditorMode* InVRMode );

	/** Sets up all componentes defined in UViewportInteractorMotionController with the passed Actor as owner */
	void SetupComponent( AActor* OwningActor );

	/** Gets the owner of this system */
	class FVREditorMode& GetVRMode()
	{
		return *VRMode;
	}

	/** Gets the owner of this system (const) */
	const class FVREditorMode& GetVRMode() const
	{
		return *VRMode;
	}

	// ViewportInteractorInterface overrides
	virtual void Shutdown() override;
	virtual void Tick( const float DeltaTime ) override;
	virtual FHitResult GetHitResultFromLaserPointer( TArray<AActor*>* OptionalListOfIgnoredActors = nullptr, const bool bIgnoreGizmos = false,
		TArray<UClass*>* ObjectsInFrontOfGizmo = nullptr, const bool bEvenIfBlocked = false, const float LaserLengthOverride = 0.0f ) override;
	virtual void ResetHoverState( const float DeltaTime ) override;
	virtual void CalculateDragRay() override;
	virtual void OnStartDragging( UActorComponent* ClickedComponent, const FVector& HitLocation, const bool bIsPlacingActors ) override;

	//
	// Getters and setters
	//

	/** Gets the Y delta of the trackpad */
	float GetTrackpadSlideDelta();

	/** Gets if*/
	bool IsHoveringOverUI() const;
	
	/** Sets if there is a docked window on this interactor */
	void SetHasUIInFront( const bool bInHasUIInFront );
	
	/** Check if there is any docked window on this interactor */
	bool HasUIInFront() const;

	/** Sets if the quick menu is on this interactor */
	void SetHasUIOnForearm( const bool bInHasUIOnForearm );
	
	/** Check if the quick menu is on this interactor */
	bool HasUIOnForearm() const;

	/** Check if the touchpad is currently touched */
	bool IsTouchingTrackpad() const;

	/** Get the current position of the trackpad or analog stick */
	FVector2D GetTrackpadPosition() const;

	/** Get the last position of the trackpad or analog stick */
	FVector2D GetLastTrackpadPosition() const;

	/** Gets the current hovered widget component if any */
	UWidgetComponent* GetHoveringOverWidgetComponent() const;

	/** Sets the current hovered widget component */
	void SetHoveringOverWidgetComponent( UWidgetComponent* NewHoveringOverWidgetComponent );
	
	/** If the trackpad values are valid */
	bool IsTrackpadPositionValid( const int32 AxisIndex ) const;

	/** Get when the last time the trackpad position was updated */
	FTimespan& GetLastTrackpadPositionUpdateTime();

	/** If the modifier key is currently pressed */
	bool IsModifierPressed() const;

	/** Sets if the interactor is clicking on any UI */
	void SetIsClickingOnUI( const bool bInIsClickingOnUI );
	
	/** Gets if the interactor is clicking on any UI */
	bool IsClickingOnUI() const;

	/** Sets if the interactor is hovering over any UI */
	void SetIsHoveringOverUI( const bool bInIsHoveringOverUI );

	/** Sets if the interactor is right  hover over any UI */
	void SetIsRightClickingOnUI( const bool bInIsRightClickingOnUI );

	/** Gets if the interactor is right clicking on UI */
	bool IsRightClickingOnUI() const;

	/** Gets if the interactor is right clicking on UI */
	void SetLastClickReleaseTime( const double InLastClickReleaseTime );

	/** Gets last time the interactor released */
	double GetLastClickReleaseTime() const;

	/** Sets the UI scroll velocity */
	void SetUIScrollVelocity( const float InUIScrollVelocity );

	/** Gets the UI scroll velocity */
	float GetUIScrollVelocity() const;
	
	/* ViewportInteractor overrides, checks if the laser is blocked by UI */
	virtual bool GetIsLaserBlocked() override;

protected:

	// ViewportInteractor overrides
	virtual void HandleInputKey( FViewportActionKeyInput& Action, const FKey Key, const EInputEvent Event, bool& bOutWasHandled ) override;
	virtual void HandleInputAxis( FViewportActionKeyInput& Action, const FKey Key, const float Delta, const float DeltaTime, bool& bOutWasHandled ) override;

private:

	/** Changes the color of the buttons on the handmesh */
	void ApplyButtonPressColors( const FViewportActionKeyInput& Action );

	/** Set the visuals for a button on the motion controller */
	void SetMotionControllerButtonPressedVisuals( const EInputEvent Event, const FName& ParameterName, const float PressStrength );
	
	/** Pops up some help text labels for the controller in the specified hand, or hides it, if requested */
	void ShowHelpForHand( const bool bShowIt );
	
	/** Called every frame to update the position of any floating help labels */
	void UpdateHelpLabels();

	/** Given a mesh and a key name, tries to find a socket on the mesh that matches a supported key */
	UStaticMeshSocket* FindMeshSocketForKey( UStaticMesh* StaticMesh, const FKey Key );

	/** Sets the visuals of the LaserPointer */
	void SetLaserVisuals( const FLinearColor& NewColor, const float CrawlFade, const float CrawlSpeed );

	/** Updates the radial menu */
	void UpdateRadialMenuInput( const float DeltaTime );

private:
	
	/** The mode that created this interactor */
	class FVREditorMode* VRMode;

	//
	// UI
	//

	/** True if a floating UI panel is attached to the front of the hand, and we shouldn't bother drawing a laser
	pointer or enabling certain other features */
	bool bHasUIInFront;

	/** True if a floating UI panel is attached to our forearm, so we shouldn't bother drawing help labels */
	bool bHasUIOnForearm;

	/** True if we're currently holding the 'SelectAndMove' button down after clicking on UI */
	bool bIsClickingOnUI;

	/** When bIsClickingOnUI is true, this will be true if we're "right clicking".  That is, the Modifier key was held down at the time that the user clicked */
	bool bIsRightClickingOnUI;

	 /** True if we're hovering over UI right now.  When hovering over UI, we don't bother drawing a see-thru laser pointer */
	bool bIsHoveringOverUI;
	
	/** Inertial scrolling -- how fast to scroll the mousewheel over UI */
	float UIScrollVelocity;	

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
	bool bIsTrackpadPositionValid[ 2 ];

	/** Real time that the last trackpad position was last updated.  Used to filter out stale previous data. */
	FTimespan LastTrackpadPositionUpdateTime;

	//
	// General input @todo: VREditor: Should this be general (non-UI) in interactordata ?
	//

	/** Is the Modifier button held down? */
	bool bIsModifierPressed;

	/** Last real time that we released the 'SelectAndMove' button on UI.  This is used to detect double-clicks. */
	double LastClickReleaseTime;
	
	//
	// Help
	//

	/** True if we want help labels to be visible right now, otherwise false */
	bool bWantHelpLabels;

	/** Help labels for buttons on the motion controllers */
	TMap< FKey, class AFloatingText* > HelpLabels;

	/** Time that we either started showing or hiding help labels (for fade transitions) */
	FTimespan HelpLabelShowOrHideStartTime;
};