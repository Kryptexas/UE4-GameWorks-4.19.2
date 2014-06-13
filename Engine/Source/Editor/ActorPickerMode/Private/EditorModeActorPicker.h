// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace EPickState
{
	enum Type
	{
		NotOverViewport,
		OverViewport,
		OverIncompatibleActor,
		OverActor,
	};
}

/**
 * Editor mode used to interactively pick actors in the level editor viewports.
 */
class FEdModeActorPicker : public FEdMode
{
public:
	FEdModeActorPicker();
	
	/** Register the picker editor mode */
	static void RegisterEditMode();

	/** Unregister the picker editor mode */
	static void UnregisterEditMode();

	/** Begin FEdMode interface */
	virtual void Tick(FLevelEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual bool MouseEnter(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport,int32 x, int32 y) override;
	virtual bool MouseLeave(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool MouseMove(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;
	virtual bool LostFocus(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool InputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual bool GetCursor(EMouseCursor::Type& OutCursor) const override;
	virtual bool UsesToolkits() const override;
	/** End FEdMode interface */

	/** Start up the mode, reset state etc. */
	void BeginMode(FOnGetAllowedClasses InOnGetAllowedClasses, FOnShouldFilterActor InOnShouldFilterActor, FOnActorSelected InOnActorSelected );

	/** End the mode */
	void EndMode();

	/** Delegate used to display information about picking near the cursor */
	FString GetCursorDecoratorText() const;

	bool IsActorValid(const AActor *const Actor) const;

	/** Flag for display state */
	TWeakObjectPtr<AActor> HoveredActor;

	/** Flag for display state */
	EPickState::Type PickState;

	/** The window that owns the decorator widget */
	TSharedPtr<SWindow> CursorDecoratorWindow;

	/** Delegates used to pick actors */
	FOnActorSelected OnActorSelected;
	FOnGetAllowedClasses OnGetAllowedClasses;
	FOnShouldFilterActor OnShouldFilterActor;
};
