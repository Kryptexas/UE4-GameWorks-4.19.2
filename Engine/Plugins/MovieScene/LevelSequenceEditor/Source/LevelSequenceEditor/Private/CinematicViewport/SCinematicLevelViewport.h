// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LevelEditorViewport.h"
#include "SEditorViewport.h"


class FLevelSequenceEditorToolkit;
class ISequencer;
class SCinematicTransportRange;
class FLevelViewportLayout;
struct FTypeInterfaceProxy;


/** Overridden level viewport client for this viewport */
struct FCinematicViewportClient : FLevelEditorViewportClient
{
	FCinematicViewportClient();
	virtual bool IsLevelEditorClient() const override { return true; }
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad=false) override;
};


/** struct containing UI data - populated once per frame */
struct FUIData
{
	/** The name of the current shot */
	FText ShotName;
	/** The name of the current shot's lens */
	FText Lens;
	/** The name of the current shot's filmback */
	FText Filmback;
	/** The text that represents the current frame */
	FText Frame;
	/** The text that represents the master start frame */
	FText MasterStartText;
	/** The text that represents the master end frame */
	FText MasterEndText;
};


/** Enum specifying different camera modes */
enum class ECinematicCameraMode
{
	FreeCamera,
	Auto,
	SpecificCamera
};


class SCinematicLevelViewport : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SCinematicLevelViewport) : _bShowMaximize(true) {}
		
		/** The unique name of this viewport inside its parent layout */
		SLATE_ARGUMENT(FName, LayoutName)
		/** Name of the viewport layout we should revert to at the user's request */
		SLATE_ARGUMENT(FName, RevertToLayoutName)
		/** Ptr to this viewport's parent layout */
		SLATE_ARGUMENT(TSharedPtr<FLevelViewportLayout>, ParentLayout)
		/** Whether to show the maximize button or not */
		SLATE_ARGUMENT(bool, bShowMaximize)
	SLATE_END_ARGS()

	/** Access this viewport's viewport client */
	TSharedPtr<FCinematicViewportClient> GetViewportClient() const;

	/** Construct this widget */
	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
private:

	/** Set up this viewport to operate on the specified toolkit */
	void Setup(FLevelSequenceEditorToolkit& NewToolkit);

	/** Clean up this viewport after its toolkit has been closed */
	void CleanUp();

	/** Called when a level sequence editor toolkit has been opened */
	void OnEditorOpened(FLevelSequenceEditorToolkit& Toolkit);
	
	/** Called when the level sequence editor toolkit we were observing has been closed */
	void OnEditorClosed();

	void BindCommands();

private:

	/** Get the sequencer ptr from our current toolkit, if set */
	ISequencer* GetSequencer() const;

	/** Called when our sequencer wants to switch cameras */
	void OnUpdateCameraCut(UObject* CameraObject);

	/** Set the camera to the specified mode (optionally binding to a specific camera) */
	void SetCameraMode(ECinematicCameraMode InCameraMode, TOptional<FGuid> InSpecificCameraGuid = TOptional<FGuid>());

	/** Make the currently available camera menu */
	TSharedRef<SWidget> MakeCameraMenu();

private:

	/** Toggle this viewport's maximized state */
	FReply OnToggleMaximize();

	/** Test whether this viewport is maximized or not */
	bool IsMaximized() const;

	/** Toggle this viewport's immersive state */
	void OnToggleImmersive();

	/** Test whether this viewport is immersive or not */
	bool IsImmersive() const;

	/** Called when we want to revert to the equivalent non-cinematic viewport */
	FReply OnRevertLayout();

private:

	/** Called once per frame to update the available cameras in the scene */
	void UpdateActiveCameras();

	/** Cache the desired viewport size based on the filled, allotted geometry */
	void CacheDesiredViewportSize(const FGeometry& AllottedGeometry);

	/** Get the desired width and height of the viewport */
	FOptionalSize GetDesiredViewportWidth() const;
	FOptionalSize GetDesiredViewportHeight() const;

	/** Get the image for the maximize button */
	const FSlateBrush* GetMaximizeImage() const;

	int32 GetVisibleWidgetIndex() const;
	EVisibility GetControlsVisibility() const;

private:

	TOptional<float> GetMinTime() const;

	TOptional<float> GetMaxTime() const;

	void OnTimeCommitted(float Value, ETextCommit::Type);

	void SetTime(float Value);

	float GetTime() const;

	FReply SetPlaybackStart();

	FReply SetPlaybackEnd();

private:
	
	/** Widget where the scene viewport is drawn in */
	TSharedPtr<SEditorViewport> ViewportWidget;

	/** The toolkit we're currently editing */
	TWeakPtr<FLevelSequenceEditorToolkit> CurrentToolkit;

	/** Commandlist used in the viewport (Maps commands to viewport specific actions) */
	TSharedPtr<FUICommandList> CommandList;

	/** Slot for transport controls */
	TSharedPtr<SBox> TransportControls;

	/** Transport range widget */
	TSharedPtr<SCinematicTransportRange> TransportRange;
	
	/** Cached UI data */
	FUIData UIData;

	/** Delegate binding handle for ISequencer::OnCameraCut */
	FDelegateHandle OnCameraCutHandle;

	/** The current camera mode */
	ECinematicCameraMode CameraMode;

	/** Guid of the camera in the UMovieScene to look through. Stored as a GUID rather than a ptr to ensure spawnable cameras work. */
	TOptional<FGuid> LockedCameraGuid;

	/** Cached desired size of the viewport */
	FVector2D DesiredViewportSize;
	
	/** Viewport controls widget that sits just below, and matches the width, of the viewport */
	TSharedPtr<SWidget> ViewportControls;

	/** Weak ptr to our parent layout */
	TWeakPtr<FLevelViewportLayout> ParentLayout;

	/** Name of the viewport in the parent layout */
	FName LayoutName;
	/** Name of the viewport layout we should revert to at the user's request */
	FName RevertToLayoutName;

	/** The time spin box */
	TSharedPtr<FTypeInterfaceProxy> TypeInterfaceProxy;

	/** Cached active cameras */
	struct FActiveCamera
	{
		FActiveCamera(FString InDisplayName, AActor* InCameraActor)
			: DisplayName(InDisplayName), CameraActor(InCameraActor)
		{}

		FString DisplayName;
		TWeakObjectPtr<AActor> CameraActor;
	};
	TMap<FGuid, FActiveCamera> Cameras;
};