// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "VREditorFloatingUI.generated.h"


/**
 * Represents an interactive floating UI panel in the VR Editor
 */
UCLASS()
class AVREditorFloatingUI : public AActor
{
	GENERATED_BODY()

public:

	/** Default constructor which sets up safe defaults */
	AVREditorFloatingUI();

	/** Possible UI attachment points */
	enum class EDockedTo
	{
		Nothing,
		LeftHand,
		RightHand,
		LeftArm,
		RightArm,
		Room,
		Custom,
		Dragging,
	};

	/** Creates a FVREditorFloatingUI using a Slate widget, and sets up safe defaults */
	void SetSlateWidget( class FVREditorUISystem& InitOwner, const TSharedRef<SWidget>& InitSlateWidget, const FIntPoint InitResolution, const float InitScale, const EDockedTo InitDockedTo );

	/** Creates a FVREditorFloatingUI using a UMG user widget, and sets up safe defaults */
	void SetUMGWidget( class FVREditorUISystem& InitOwner, class TSubclassOf<class UVREditorBaseUserWidget> InitUserWidgetClass, const FIntPoint InitResolution, const float InitScale, const EDockedTo InitDockedTo );

	/** @return Returns true if the UI is visible (or wants to be visible -- it might be transitioning */
	bool IsUIVisible() const
	{
		return bShouldBeVisible.GetValue();
	}

	/** Shows or hides the UI (also enables collision, and performs a transition effect) */
	void ShowUI( const bool bShow, const bool bAllowFading = true, const float InitFadeDelay = 0.0f );

	/** Returns what we're docked to, if anything */
	EDockedTo GetDockedTo() const
	{
		return DockedTo;
	}

	/** Returns what we were previously docked to */
	EDockedTo GetPreviouslyDockedTo() const
	{
		return PreviousDockedTo;
	}

	/** Sets what this UI is docked to */
	void SetDockedTo( const EDockedTo NewDockedTo );

	/** Returns the widget component for this UI, or nullptr if not spawned right now */
	class UVREditorWidgetComponent* GetWidgetComponent()
	{
		return WidgetComponent;
	}

	/** Returns the actual size of the UI in either axis, after scaling has been applied.  Does not take into animation or world scaling */
	FVector2D GetSize() const;

	/** Gets the scale */
	float GetScale() const;

	/** Sets a new size for the UI */
	void SetScale( const float NewSize );

	/** AActor overrides */
	virtual void Destroyed() override;

	/** Called by the UI system to tick this UI every frame.  We can't use AActor::Tick() because this actor can exist in 
	    the editor's world, which never ticks. */
	virtual void TickManually( float DeltaTime );

	/** Sets the transform for this UI */
	void SetTransform( const FTransform& Transform );

	/** Returns the owner of this object */
	FVREditorUISystem& GetOwner()
	{
		check( Owner != nullptr );
		return *Owner;
	}

	/** Returns the owner of this object (const) */
	const FVREditorUISystem& GetOwner() const
	{
		check( Owner != nullptr );
		return *Owner;
	}

	/** Returns casted userwidget */
	template<class T>
	T* GetUserWidget() const
	{
		return CastChecked<T>( UserWidget );
	}

	/** Sets the relative offset of the UI */
	void SetRelativeOffset( const FVector& InRelativeOffset );

	/** Sets the local rotation of the UI */
	void SetLocalRotation( const FRotator& InRelativeRotation );

	/** Sets if the collision is on when showing the UI*/
	void SetCollisionOnShow( const bool bInCollisionOnShow );

	/** Gets the initial size of this UI */
	float GetInitialScale() const;

	/** Get a relative transform from the hand */
	FTransform MakeUITransformLockedToHand( const int32 HandIndex, const bool bOnArm, const FVector& InRelativeOffset, const FRotator& InLocalRotation );

	/** Start moving towards a transform */
	void MoveTo( const FTransform& ResultTransform, const float TotalMoveToTime, const EDockedTo ResultDock );

	/** Abort moving to transform and move instant to MoveToTransform */
	void StopMoveTo();

protected:

	/** Returns a scale to use for this widget that takes into account animation */
	FVector CalculateAnimatedScale() const;

	/** Called when DockTo state is custom, needs to be implemented by derived classes */
	virtual FTransform MakeCustomUITransform() { return FTransform(); };

	/** Called to finish setting everything up, after a widget has been assigned */
	virtual void SetupWidgetComponent();

private:

	/** Called after spawning, and every Tick, to update opacity of the widget */
	void UpdateFadingState( const float DeltaTime );

	/** Called every tick to keep the UI position up to date */
	void UpdateTransformIfDocked();

	/** Given a hand to lock to, returns a transform to place UI at that hand's location and orientation */
	FTransform MakeUITransformLockedToHand( const int32 HandIndex, const bool bOnArm);

	/** Create a room transform with the RelativeOffset and LocalRotation */
	FTransform MakeUITransformLockedToRoom();

	/** Updates the lerp movement */
	void TickMoveTo( const float DeltaTime );

public:

	/** Local rotation of the UI */
	FRotator LocalRotation;

	/** Relative offset of the UI */
	FVector RelativeOffset;

protected:
	/** Slate widget we're drawing, or null if we're drawing a UMG user widget */
	TSharedPtr<SWidget> SlateWidget;
	
	/** UMG user widget we're drawing, or nullptr if we're drawing a Slate widget */
	UPROPERTY()
	UVREditorBaseUserWidget* UserWidget;
	
	/** When in a spawned state, this is the widget component to represent the widget */
	UPROPERTY()
	class UVREditorWidgetComponent* WidgetComponent;
	
	/** How big the UI should be */
	float Scale;

	/** Resolution we should draw this UI at, regardless of scale */
	FIntPoint Resolution;

private:

	/** Owning object */
	class FVREditorUISystem* Owner;

	/** What the UI is attached to */
	EDockedTo DockedTo;

	/** What was the UI previously attached to */
	EDockedTo PreviousDockedTo;

	/** Class of the UMG widget we're spawning */
	UPROPERTY()
	UClass* UserWidgetClass;

	/** True if the UI wants to be visible, or false if it wants to be hidden.  Remember, we might still be visually 
	    transitioning between states */
	TOptional<bool> bShouldBeVisible;

	/** Fade alpha, for visibility transitions */
	float FadeAlpha;

	/** Collision on when showing UI */
	bool bCollisionOnShowUI;

	/** Delay to fading in or out. Set on ShowUI and cleared on finish of fade in/out */
	float FadeDelay;

	/** The starting scale of this UI */
	float InitialScale;

	/** If the UI is currently moving */
	bool bIsMoving;

	/** The end transform to move to */
	FTransform MoveToTransform;

	/** The actor transform when started moving */
	FTransform StartMoveToTransform;

	/** Current lerp alpha */
	float MoveToAlpha;

	/** Total time to move to the end transform */
	float MoveToTime;

	/** Dock state to switch to when finished moving */
	EDockedTo MoveToResultDock;
};

