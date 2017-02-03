// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Widgets/SOverlay.h"
#include "VREditorUISystem.generated.h"

class AVREditorDockableWindow;
class AVREditorFloatingUI;
class FProxyTabmanager;
class SColorPicker;
class SBorder;
class SButton;
class FMultiBox;
class SMultiBoxWidget;
class SWidget;
class UViewportInteractor;
class UVREditorInteractor;


/** Stores the animation playback state of a VR UI element */
enum class EVREditorAnimationState : uint8
{
	None,
	Forward,
	Backward
};

/** Structure to keep track of all relevant interaction and animation elements of a VR Button */
struct FVRButton
{
	/** Pointer to button */
	TSharedPtr<SButton> Button;

	/** Pointer to button border */
	TSharedPtr<SBorder> ButtonBorder;

	/** Animation playback state of the button */
	EVREditorAnimationState AnimationDirection;

	/** Current scale of the button element */
	float CurrentScale;

	/** Minimum scale of the button element */
	float MinScale;

	/** Maximum scale of the button element */
	float MaxScale;

	/** Rate at which the button changes scale. Currently the same for scaling up and scaling down. */
	float ScaleRate;

	FVRButton()
		: AnimationDirection(EVREditorAnimationState::None),
		CurrentScale(1.0f),
		MinScale(1.0f),
		MaxScale(1.10f),
		ScaleRate(2.0f)
		{}

	FVRButton(TSharedPtr<SButton> InButton, TSharedPtr<SBorder> InButtonBorder,
		EVREditorAnimationState InAnimationDirection = EVREditorAnimationState::None, float InCurrentScale = 1.0f, float InMinScale = 1.0f, float InMaxScale = 1.25f, float InScaleRate = 2.0f)
		: Button(InButton),
		ButtonBorder(InButtonBorder),
		AnimationDirection(InAnimationDirection),
		CurrentScale(InCurrentScale),
		MinScale(InMinScale),
		MaxScale(InMaxScale),
		ScaleRate(InScaleRate)
		{}

};


/**
 * VR Editor user interface manager
 */
UCLASS()
class UVREditorUISystem : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	enum class EEditorUIPanel
	{
		ContentBrowser,
		WorldOutliner,
		ActorDetails,
		Modes,
		Tutorial,
		AssetEditor,
		WorldSettings,
		ColorPicker,
		// ...

		TotalCount,
	};

	/** Shuts down the UISystem whenever the mode is exited */
	void Shutdown();

	/** Initializes default values for the UISystem and creates the UI panels */
	void Init();

	/** Called by VREditorMode to update us every frame */
	void Tick( class FEditorViewportClient* ViewportClient, const float DeltaTime );

	/** Called by VREditorMode to draw debug graphics every frame */
	void Render( const class FSceneView* SceneView, class FViewport* Viewport, class FPrimitiveDrawInterface* PDI );

	/** Sets the owner of this system */
	void SetOwner( class UVREditorMode* NewOwner )
	{
		VRMode = NewOwner;
	}

	/** Gets the owner of this system */
	class UVREditorMode& GetOwner()
	{
		return *VRMode;
	}

	/** Gets the owner of this system (const) */
	const class UVREditorMode& GetOwner() const
	{
		return *VRMode;
	}

	/** Called by our owner right before a map is loaded or switching between Simulate and normal Editor, so we can clean
	    up our spawned actors */
	void CleanUpActorsBeforeMapChangeOrSimulate();

	/** Searches to see if the specified widget component is from our editor UI */
	bool IsWidgetAnEditorUIWidget( const class UActorComponent* WidgetComponent ) const;

	/** Returns true if the specified editor UI panel is currently visible */
	bool IsShowingEditorUIPanel( const EEditorUIPanel EditorUIPanel ) const;

	/** Sets whether the specified editor UI panel should be visible.  Any other UI floating off this hand will be dismissed when showing it. */
	void ShowEditorUIPanel( const class UWidgetComponent* WidgetComponent, UVREditorInteractor* Interactor, const bool bShouldShow, const bool OnHand = false, const bool bRefreshQuickMenu = true, const bool bPlaySound = true );
	void ShowEditorUIPanel( const EEditorUIPanel EditorUIPanel, UVREditorInteractor* Interactor, const bool bShouldShow, const bool OnHand = false, const bool bRefreshQuickMenu = true, const bool bPlaySound = true );
	void ShowEditorUIPanel( class AVREditorFloatingUI* Panel, UVREditorInteractor* Interactor, const bool bShouldShow, const bool OnHand = false, const bool bRefreshQuickMenu = true, const bool bPlaySound = true );

	/** Returns true if the radial menu is visible on this hand */
	bool IsShowingRadialMenu(const UVREditorInteractor* Interactor ) const;

	/** Tries to spawn the radial menu (if the specified hand isn't doing anything else) */
	void TryToSpawnRadialMenu( UVREditorInteractor* Interactor, const bool bForceOverUI );

	/** Hides the radial menu if the specified hand is showing it */
	void HideRadialMenu( UVREditorInteractor* Interactor );

	/** Start dragging a dock window on the hand */
	void StartDraggingDockUI( class AVREditorDockableWindow* InitDraggingDockUI, UVREditorInteractor* Interactor, const float DockSelectDistance );

	/** Makes up a transform for a dockable UI when dragging it with a laser at the specified distance from the laser origin */
	FTransform MakeDockableUITransformOnLaser( AVREditorDockableWindow* InitDraggingDockUI, UVREditorInteractor* Interactor, const float DockSelectDistance ) const;

	/** Makes up a transform for a dockable UI when dragging it that includes the original offset from the laser's impact point */
	FTransform MakeDockableUITransform( AVREditorDockableWindow* InitDraggingDockUI, UVREditorInteractor* Interactor, const float DockSelectDistance );
	
	/** Returns the current dragged dock window, nullptr if none */
	class AVREditorDockableWindow* GetDraggingDockUI() const;

	/** Resets all values to stop dragging the current dock window */
	void StopDraggingDockUI( UVREditorInteractor* VREditorInteractor );

	/** Are we currently dragging a dock window */
	bool IsDraggingDockUI();	

	bool IsInteractorDraggingDockUI( const UVREditorInteractor* Interactor ) const;

	/** Hides and unhides all the editor UI panels */
	void TogglePanelsVisibility();

	/** Get the maximum dock window size */
	float GetMaxDockWindowSize() const;

	/** Get the minimum dock window size */
	float GetMinDockWindowSize() const;

	/** Toggles the visibility of the panel, if the panel is in room space it will be hidden and docked to nothing */
	void TogglePanelVisibility( const EEditorUIPanel EditorUIPanel );

	/** Returns the radial widget so other classes, like the interactors, can access its functionality */
	const TSharedPtr<SWidget>& GetRadialWidget();

	/** 
	 * Finds a widget with a given name inside the Content argument 
	 * @param Content The widget to begin searching in
	 * @param The FName of the widget type to find. 
	 */
	static const TSharedRef<SWidget>& FindWidgetOfType(const TSharedRef<SWidget>& Content, FName WidgetType);


protected:

	/** Called when the user presses a button on their motion controller device */
	void OnVRAction( class FEditorViewportClient& ViewportClient, UViewportInteractor* Interactor,
		const struct FViewportActionKeyInput& Action, bool& bOutIsInputCaptured, bool& bWasHandled );

	/** Called every frame to update hover state */
	void OnVRHoverUpdate( class FEditorViewportClient& ViewportClient, UViewportInteractor* Interactor, FVector& HoverImpactPoint, bool& bWasHandled );

	/** Testing Slate UI */
	void CreateUIs();

	/**
	 * Called when the injected proxy tab manager reports a new tab has been launched, 
	 * signaling an asset editor has been launched, but really it could be any major global tab.
	 */
	void OnProxyTabLaunched(TSharedPtr<SDockTab> NewTab);

	/** Called when the injected proxy tab manager reports needing to draw attention to a tab. */
	void OnAttentionDrawnToTab(TSharedPtr<SDockTab> NewTab);

	/** Show the asset editor if it's not currently visible. */
	void ShowAssetEditor();

	/** Called when an asset editor opens an asset while in VR Editor Mode. */
	void OnAssetEditorOpened(UObject* Asset);

	/** Sets the main windows to their default transform */
	void SetDefaultWindowLayout();


	/** Creates a VR-specific color picker. Gets bound to SColorPicker's creation override delegate */
	void CreateVRColorPicker(const TSharedRef<SColorPicker>& ColorPicker);

	/** Hides the VR-specific color picker. Gets bound to SColorPicker's destruction override delegate */
	void DestroyVRColorPicker();

	/**  Makes a uniform grid widget from the menu information contained in a MultiBox and MultiBoxWidget */
	void MakeUniformGridMenu( const TSharedRef<FMultiBox>& MultiBox, const TSharedRef<SMultiBoxWidget>& MultiBoxWidget, int32 Columns);
	/**  Makes a radial box widget from the menu information contained in a MultiBox and MultiBoxWidget */
	void MakeRadialBoxMenu(const TSharedRef<FMultiBox>& MultiBox, const TSharedRef<SMultiBoxWidget>& MultiBoxWidget, float RadiusRatioOverride);

	/** Adds a hoverable button of a given type to an overlay, using menu data from a BlockWidget */
	TSharedRef<SWidget> AddHoverableButton(TSharedRef<SWidget>& BlockWidget, FName ButtonType, TSharedRef<SOverlay>& TestOverlay);
	/** Sets the text wrap size of the text block element nested in a BlockWidget */
	TSharedRef<SWidget> SetButtonTextWrap(TSharedRef<SWidget>& BlockWidget, float WrapSize);
	
	/** Builds the quick menu Slate widget */
	TSharedRef<SWidget> BuildQuickMenuWidget();
	/** Builds the radial menu Slate widget */
	TSharedRef<SWidget> BuildRadialMenuWidget();
	/** Builds the numpad Slate widget */
	TSharedRef<SWidget> BuildNumPadWidget();
	/** Swaps the content of the radial menu between the radial menu and the numpad */
	void SwapRadialMenu();

	/** Called when a laser or simulated mouse hover enters a button */
	void OnHoverBeginEffect(TSharedRef<SButton> Button);
	/** Called when a laser or simulated mouse hover leaves a button */
	void OnHoverEndEffect(TSharedRef<SButton> Button);


protected:

	/** Owning object */
	UPROPERTY()
	class UVREditorMode* VRMode;

	/** All of the floating UIs.  These may or may not be visible (spawned) */
	UPROPERTY()
	TArray< class AVREditorFloatingUI* > FloatingUIs;

	/** Our Quick Menu UI */
	UPROPERTY()
	AVREditorFloatingUI* QuickMenuUI;

	/** Editor UI panels */
	UPROPERTY()
	TArray<AVREditorFloatingUI*> EditorUIPanels;

	/** The Radial Menu UI */
	UPROPERTY()
	AVREditorFloatingUI* QuickRadialMenu;

	/** The time since the radial menu was updated */
	float RadialMenuHideDelayTime;

	/** Tutorial menu widget class */
	UClass* TutorialWidgetClass;

	/** Pointer to the radial menu widget */
	TSharedPtr<SWidget> RadialWidget;

	/** True if the radial menu was visible when the content was swapped */
	bool bRadialMenuVisibleAtSwap;

	/** True if the radial menu is currently displaying the numpad */
	bool bRadialMenuIsNumpad;
	
	//
	// Dragging UI
	//

	/** Interactor that is dragging the UI */
	class UVREditorInteractor* InteractorDraggingUI;

	/** Offset transform from room-relative transform to the object where we picked it up at */
	FTransform DraggingUIOffsetTransform;

	/** The current UI that is being dragged */
	UPROPERTY()
	class AVREditorDockableWindow* DraggingUI;

	/** The color picker dockable window */
	UPROPERTY()
	class AVREditorDockableWindow* ColorPickerUI;

	//
	// Tab Manager UI
	//

	/** Allows us to steal the global tabs and show them in the world. */
	TSharedPtr<FProxyTabmanager> ProxyTabManager;

	/** Set to true when we need to refocus the viewport. */
	bool bRefocusViewport;


	//
	// Sounds
	//

	/** Start dragging UI sound */
	UPROPERTY()
	class USoundCue* StartDragUISound;

	/** Stop dragging UI sound */
	UPROPERTY()
	class USoundCue* StopDragUISound;

	/** Hide UI sound */
	UPROPERTY()
	class USoundCue* HideUISound;

	/** Show UI sound */
	UPROPERTY()
	class USoundCue* ShowUISound;

	/** Button press sound */
	UPROPERTY()
	class USoundCue* ButtonPressSound;

	/** If the current dragged dock passed a certain distance if dragged from a hand */
	bool bDraggedDockFromHandPassedThreshold;

	/** The last dragged hover location by the laser */
	FVector LastDraggingHoverLocation;

	/** Default transforms */
	TArray<FTransform> DefaultWindowTransforms;

	/** All buttons created for the radial and quick menus */
	TArray<FVRButton> VRButtons;

	/** If this is the first time using TogglePanelsVisibility */
	bool bSetDefaultLayout;
};

