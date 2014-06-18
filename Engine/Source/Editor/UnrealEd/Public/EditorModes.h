// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorModeRegistry.h"

/** Outcomes when determining whether it's possible to perform an action on the edit modes*/
namespace EEditAction
{
	enum Type
	{
		/** Can't process this action */
		Skip		= 0,
		/** Can process this action */
		Process,
		/** Stop evaluating other modes (early out) */
		Halt,
	};
};

// Builtin editor mode constants
struct UNREALED_API FBuiltinEditorModes
{
public:
	/** Gameplay, editor disabled. */
	const static FEditorModeID EM_None;

	/** Camera movement, actor placement. */
	const static FEditorModeID EM_Default;

	/** Placement mode */
	const static FEditorModeID EM_Placement;

	/** Bsp mode */
	const static FEditorModeID EM_Bsp;

	/** Geometry editing mode. */
	const static FEditorModeID EM_Geometry;

	/** Interpolation editing. */
	const static FEditorModeID EM_InterpEdit;

	/** Texture alignment via the widget. */
	const static FEditorModeID EM_Texture;

	/** Mesh paint tool */
	const static FEditorModeID EM_MeshPaint;

	/** Landscape editing */
	const static FEditorModeID EM_Landscape;

	/** Foliage painting */
	const static FEditorModeID EM_Foliage;

	/** Level editing mode */
	const static FEditorModeID EM_Level;

	/** Streaming level editing mode */
	const static FEditorModeID EM_StreamingLevel;

	/** Physics manipulation mode ( available only when simulating in viewport )*/
	const static FEditorModeID EM_Physics;

	/** Actor picker mode, used to interactively pick actors in the viewport */
	const static FEditorModeID EM_ActorPicker;

private:
	FBuiltinEditorModes() {}
};

/** Material proxy wrapper that can be created on the game thread and passed on to the render thread. */
class UNREALED_API FDynamicColoredMaterialRenderProxy : public FDynamicPrimitiveResource, public FColoredMaterialRenderProxy
{
public:
	/** Initialization constructor. */
	FDynamicColoredMaterialRenderProxy(const FMaterialRenderProxy* InParent,const FLinearColor& InColor)
	:	FColoredMaterialRenderProxy(InParent,InColor)
	{
	}
	virtual ~FDynamicColoredMaterialRenderProxy()
	{
	}

	// FDynamicPrimitiveResource interface.
	virtual void InitPrimitiveResource()
	{
	}
	virtual void ReleasePrimitiveResource()
	{
		delete this;
	}
};


/**
 * Base class for all editor modes.
 */
class UNREALED_API FEdMode : public TSharedFromThis<FEdMode>, public FGCObject, public FEditorCommonDrawHelper
{
public:
	/** Friends so it can access mode's internals on construction */
	friend class FEditorModeRegistry;

	FEdMode();
	virtual ~FEdMode();

	virtual void Initialize() {}

	virtual bool MouseEnter( FEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y );

	virtual bool MouseLeave( FEditorViewportClient* ViewportClient,FViewport* Viewport );

	virtual bool MouseMove(FEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y);

	virtual bool ReceivedFocus(FEditorViewportClient* ViewportClient,FViewport* Viewport);

	virtual bool LostFocus(FEditorViewportClient* ViewportClient,FViewport* Viewport);

	/**
	 * Called when the mouse is moved while a window input capture is in effect
	 *
	 * @param	InViewportClient	Level editor viewport client that captured the mouse input
	 * @param	InViewport			Viewport that captured the mouse input
	 * @param	InMouseX			New mouse cursor X coordinate
	 * @param	InMouseY			New mouse cursor Y coordinate
	 *
	 * @return	true if input was handled
	 */
	virtual bool CapturedMouseMove( FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY );

	virtual bool InputKey(FEditorViewportClient* ViewportClient,FViewport* Viewport,FKey Key,EInputEvent Event);
	virtual bool InputAxis(FEditorViewportClient* InViewportClient,FViewport* Viewport,int32 ControllerId,FKey Key,float Delta,float DeltaTime);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport);
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport);
	// Added for handling EDIT Command...
	virtual EEditAction::Type GetActionEditDuplicate() { return EEditAction::Skip; }
	virtual EEditAction::Type GetActionEditDelete() { return EEditAction::Skip; }
	virtual EEditAction::Type GetActionEditCut() { return EEditAction::Skip; }
	virtual EEditAction::Type GetActionEditCopy() { return EEditAction::Skip; }
	virtual EEditAction::Type GetActionEditPaste() { return EEditAction::Skip; }
	virtual bool ProcessEditDuplicate() { return false; }
	virtual bool ProcessEditDelete() { return false; }
	virtual bool ProcessEditCut() { return false; }
	virtual bool ProcessEditCopy() { return false; }
	virtual bool ProcessEditPaste() { return false; }

	virtual void Tick(FEditorViewportClient* ViewportClient,float DeltaTime);

	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const { return false; }
	virtual void ActorMoveNotify() {}
	virtual void ActorsDuplicatedNotify(TArray<AActor*>& PreDuplicateSelection, TArray<AActor*>& PostDuplicateSelection, bool bOffsetLocations) {}
	virtual void ActorSelectionChangeNotify();
	virtual void ActorPropChangeNotify() {}
	virtual void MapChangeNotify() {}
	virtual bool ShowModeWidgets() const { return 1; }

	/** If the EdMode is handling InputDelta (i.e., returning true from it), this allows a mode to indicated whether or not the Widget should also move. */
	virtual bool AllowWidgetMove() { return true; }
	
	/** If the Edmode is handling its own mouse deltas, it can disable the MouseDeltaTacker */
	virtual bool DisallowMouseDeltaTracking() const { return false; }

	/**
	 * Get a cursor to override the default with, if any.
	 * @return true if the cursor was overriden.
	 */
	virtual bool GetCursor(EMouseCursor::Type& OutCursor) const { return false; }

	virtual bool ShouldDrawBrushWireframe( AActor* InActor ) const { return true; }
	virtual bool GetCustomDrawingCoordinateSystem( FMatrix& InMatrix, void* InData ) { return 0; }
	virtual bool GetCustomInputCoordinateSystem( FMatrix& InMatrix, void* InData ) { return 0; }

	/** If Rotation Snap should be enabled for this mode*/ 
	virtual bool IsSnapRotationEnabled();

	/** If this mode should override the snap rotation
	* @param	Rotation		The Rotation Override
	*
	* @return					True if you have overridden the value
	*/
	virtual bool SnapRotatorToGridOverride(FRotator& Rotation){ return false; };

	/**
	 * Allows each mode to customize the axis pieces of the widget they want drawn.
	 *
	 * @param	InwidgetMode	The current widget mode
	 *
	 * @return					A bitfield comprised of AXIS_* values
	 */
	virtual EAxisList::Type GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const;

	/**
	 * Allows each mode/tool to determine a good location for the widget to be drawn at.
	 */
	virtual FVector GetWidgetLocation() const;

	/**
	 * Lets the mode determine if it wants to draw the widget or not.
	 */
	virtual bool ShouldDrawWidget() const;
	virtual void UpdateInternalData() {}

	virtual FVector GetWidgetNormalFromCurrentAxis( void* InData );

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override {}

	virtual void Enter();
	virtual void Exit();
	virtual UTexture2D* GetVertexTexture() { return GEngine->DefaultBSPVertexTexture; }
	
	/**
	 * Lets each tool determine if it wants to use the editor widget or not.  If the tool doesn't want to use it,
	 * it will be fed raw mouse delta information (not snapped or altered in any way).
	 */
	virtual bool UsesTransformWidget() const;

	/**
	 * Lets each mode selectively exclude certain widget types.
	 */
	virtual bool UsesTransformWidget(FWidget::EWidgetMode CheckMode) const;

	virtual void PostUndo() {}

	/**
	 * Lets each mode/tool handle box selection in its own way.
	 *
	 * @param	InBox	The selection box to use, in worldspace coordinates.
	 * @return		true if something was selected/deselected, false otherwise.
	 */
	bool BoxSelect( FBox& InBox, bool InSelect = true );

	/**
	 * Lets each mode/tool handle frustum selection in its own way.
	 *
	 * @param	InFrustum	The selection box to use, in worldspace coordinates.
	 * @return	true if something was selected/deselected, false otherwise.
	 */
	bool FrustumSelect( const FConvexVolume& InFrustum, bool InSelect = true );

	void SelectNone();
	virtual void SelectionChanged() {}

	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click);

	/** Handling SelectActor */
	virtual bool Select( AActor* InActor, bool bInSelected ) { return 0; }

	/** Check to see if an actor can be selected in this mode - no side effects */
	virtual bool IsSelectionAllowed( AActor* InActor, bool bInSelection ) const { return true; }

	/** Returns the editor mode identifier. */
	FEditorModeID GetID() const { return Info.ID; }

	/** Returns the editor mode information. */
	const FEditorModeInfo& GetModeInfo() const { return Info; }

	// Tools

	void SetCurrentTool( EModeTools InID );
	void SetCurrentTool( FModeTool* InModeTool );
	FModeTool* FindTool( EModeTools InID );

	const TArray<FModeTool*>& GetTools() const		{ return Tools; }

	virtual void CurrentToolChanged() {}

	/** Returns the current tool. */
	//@{
	FModeTool* GetCurrentTool()				{ return CurrentTool; }
	const FModeTool* GetCurrentTool() const	{ return CurrentTool; }
	//@}

	/** @name Current widget axis. */
	//@{
	void SetCurrentWidgetAxis(EAxisList::Type InAxis)		{ CurrentWidgetAxis = InAxis; }
	EAxisList::Type GetCurrentWidgetAxis() const			{ return CurrentWidgetAxis; }
	//@}

	/** @name Rendering */
	//@{
	/** Draws translucent polygons on brushes and volumes. */
	virtual void Render(const FSceneView* View,FViewport* Viewport,FPrimitiveDrawInterface* PDI);
	//void DrawGridSection(int32 ViewportLocX,int32 ViewportGridY,FVector* A,FVector* B,float* AX,float* BX,int32 Axis,int32 AlphaCase,FSceneView* View,FPrimitiveDrawInterface* PDI);

	/** Overlays the editor hud (brushes, drag tools, static mesh vertices, etc*. */
	virtual void DrawHUD(FEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas);
	//@}

	/**
	 * Called when attempting to duplicate the selected actors by alt+dragging,
	 * return true to prevent normal duplication.
	 */
	virtual bool HandleDragDuplicate() { return false; }

	/**
	 * Called when the mode wants to draw brackets around selected objects
	 */
	virtual void DrawBrackets( FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas );

	/** True if this mode uses a toolkit mode (eventually they all should) */
	virtual bool UsesToolkits() const;

	UWorld* GetWorld() const
	{
		return GEditor->GetEditorWorldContext().World();
	}

	// Property Widgets

	/**
	 * Lets each mode selectively enable widgets for editing properties tagged with 'Show 3D Widget' metadata.
	 */
	virtual bool UsesPropertyWidgets() const { return false; }

	// @TODO: Find a better home for these?
	/** Value of UPROPERTY can be edited with a widget in the editor (translation, rotation) */
	static const FName MD_MakeEditWidget;
	/** Specifies a function used for validation of the current value of a property.  The function returns a string that is empty if the value is valid, or contains an error description if the value is invalid */
	static const FName MD_ValidateWidgetUsing;
	/** Returns true if this structure can support creating a widget in the editor */
	static bool CanCreateWidgetForStructure(const UStruct* InPropStruct);
	/** Returns true if this property can support creating a widget in the editor */
	static bool CanCreateWidgetForProperty(UProperty* InProp);
	/** See if we should create a widget for the supplied property when selecting an actor instance */
	static bool ShouldCreateWidgetForProperty(UProperty* InProp);

public:

	/** Request that this mode be deleted at the next convenient opportunity (FEditorModeTools::Tick) */
	void RequestDeletion() { bPendingDeletion = true; }
	
	/** returns true if this mode is to be deleted at the next convenient opportunity (FEditorModeTools::Tick) */
	bool IsPendingDeletion() const { return bPendingDeletion; }

private:
	/** true if this mode is pending removal from its owner */
	bool bPendingDeletion;

	/** Called whenever a mode type is unregistered */
	void OnModeUnregistered(FEditorModeID ModeID);

protected:
	/** The current axis that is being dragged on the widget. */
	EAxisList::Type CurrentWidgetAxis;

	/** Optional array of tools for this mode. */
	TArray<FModeTool*> Tools;

	/** The tool that is currently active within this mode. */
	FModeTool* CurrentTool;

	/** Information pertaining to this mode. Assigned by FEditorModeRegistry. */
	FEditorModeInfo Info;

	/** Editor Mode Toolkit that is associated with this toolkit mode */
	TSharedPtr<class FModeToolkit> Toolkit;

	/** Pointer back to the mode tools that we are registered with */
	FEditorModeTools* Owner;

	// Property Widgets

	/** Structure that holds info about our optional property widget */
	struct FPropertyWidgetInfo
	{
		FString PropertyName;
		int32 PropertyIndex;
		FName PropertyValidationName;
		FString DisplayName;
		bool bIsTransform;

		FPropertyWidgetInfo()
			: PropertyIndex(INDEX_NONE)
			, bIsTransform(false)
		{
		}
	};

	/**
	 * Returns the first selected Actor, or NULL if there is no selection.
	 */
	AActor* GetFirstSelectedActorInstance() const;

	/**
	 * Gets an array of property widget info structures for the given struct/class type for the given container.
	 *
	 * @param InStruct The type of structure/class to access widget info structures for.
	 * @param InContainer The container of the given type.
	 * @param OutInfos An array of widget info structures (output).
	 * @param PropertyNamePrefix Optional prefix to use to filter the output.
	 */
	void GetPropertyWidgetInfos(const UStruct* InStruct, const void* InContainer, TArray<FPropertyWidgetInfo>& OutInfos, FString PropertyNamePrefix = TEXT("")) const;

	/** Name of the property currently being edited */
	FString EditedPropertyName;

	/** If the property being edited is an array property, this is the index of the element we're currently dealing with */
	int32 EditedPropertyIndex;

	/** Indicates  */
	bool bEditedPropertyIsTransform;
};

/*------------------------------------------------------------------------------
	Default.
------------------------------------------------------------------------------*/

/**
 * The default editing mode.  User can work with BSP and the builder brush. Vector and array properties are also visually editable.
 */
class FEdModeDefault : public FEdMode
{
public:
	FEdModeDefault();

	// FEdMode interface
	virtual bool UsesPropertyWidgets() const override { return true; }
	// End of FEdMode interface
};


/**
 * A helper class to store the state of the various editor modes.
 */
class UNREALED_API FEditorModeTools : public FGCObject
{
public:
	FEditorModeTools();
	virtual ~FEditorModeTools();

	/**
	 * Set the default editor mode for these tools
	 */
	void SetDefaultMode ( FEditorModeID DefaultID );

	/**
	 * Activates the default mode defined by this class
	 */
	void ActivateDefaultMode();

	/** 
	 * Returns true if the default editor mode is active 
	 */
	bool IsDefaultModeActive() const;

	/**
	 * Activates an editor mode. Shuts down all other active modes which cannot run with the passed in mode.
	 * 
	 * @param InID		The ID of the editor mode to activate.
	 * @param bToggle	true if the passed in editor mode should be toggled off if it is already active.
	 */
	void ActivateMode( FEditorModeID InID, bool bToggle = false );

	/**
	 * Deactivates an editor mode. 
	 * 
	 * @param InID		The ID of the editor mode to deactivate.
	 */
	void DeactivateMode(FEditorModeID InID);

	/**
	 * Deactivate the mode and entirely purge it from memory. Used when a mode type is unregistered
	 */
	void DestroyMode(FEditorModeID InID);

protected:
	
	/** Deactivates the editor mode at the specified index */
	void DeactivateModeAtIndex( int32 InIndex );
		
public:

	/**
	 * Deactivates all modes, note some modes can never be deactivated.
	 */
	void DeactivateAllModes();

	/** 
	 * Returns the editor mode specified by the passed in ID
	 */
	FEdMode* FindMode( FEditorModeID InID );

	/**
	 * Returns true if the current mode is not the specified ModeID.  Also optionally warns the user.
	 *
	 * @param	ModeID			The editor mode to query.
	 * @param	ErrorMsg		If specified, inform the user the reason why this is a problem
	 * @param	bNotifyUser		If true, display the error as a notification, instead of a dialog
	 * @return					true if the current mode is not the specified mode.
	 */
	bool EnsureNotInMode(FEditorModeID ModeID, const FText& ErrorMsg = FText::GetEmpty(), bool bNotifyUser = false) const;

	FMatrix GetCustomDrawingCoordinateSystem();
	FMatrix GetCustomInputCoordinateSystem();
	
	/** 
	 * Returns true if the passed in editor mode is active 
	 */
	bool IsModeActive( FEditorModeID InID ) const;

	/**
	 * Returns a pointer to an active mode specified by the passed in ID
	 * If the editor mode is not active, NULL is returned
	 */
	FEdMode* GetActiveMode( FEditorModeID InID );
	const FEdMode* GetActiveMode( FEditorModeID InID ) const;

	template <typename SpecificModeType>
	SpecificModeType* GetActiveModeTyped( FEditorModeID InID )
	{
		return (SpecificModeType*)GetActiveMode(InID);
	}

	template <typename SpecificModeType>
	const SpecificModeType* GetActiveModeTyped( FEditorModeID InID ) const
	{
		return (SpecificModeType*)GetActiveMode(InID);
	}

	/**
	 * Returns the active tool of the passed in editor mode.
	 * If the passed in editor mode is not active or the mode has no active tool, NULL is returned
	 */
	const FModeTool* GetActiveTool( FEditorModeID InID ) const;

	/** 
	 * Returns an array of all active modes
	 */
	void GetActiveModes( TArray<FEdMode*>& OutActiveModes );

	void SetShowWidget( bool InShowWidget )	{ bShowWidget = InShowWidget; }
	bool GetShowWidget() const				{ return bShowWidget; }

	void CycleWidgetMode (void);

	/**Save Widget Settings to Ini file*/
	void SaveWidgetSettings();
	/**Load Widget Settings from Ini file*/
	void LoadWidgetSettings();

	/** Gets the widget axis to be drawn */
	EAxisList::Type GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const;

	/** Mouse tracking interface.  Passes tracking messages to all active modes */
	bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport);
	bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport);
	bool IsTracking() const { return bIsTracking; }

	/** Notifies all active modes that a map change has occured */
	void MapChangeNotify();

	/** Notifies all active modes to empty their selections */
	void SelectNone();

	/** Notifies all active modes of box selection attempts */
	bool BoxSelect( FBox& InBox, bool InSelect );

	/** Notifies all active modes of frustum selection attempts */
	bool FrustumSelect( const FConvexVolume& InFrustum, bool InSelect );

	/** true if any active mode uses a transform widget */
	bool UsesTransformWidget() const;

	/** true if any active mode uses the passed in transform widget */
	bool UsesTransformWidget( FWidget::EWidgetMode CheckMode ) const;

	/** Sets the current widget axis */
	void SetCurrentWidgetAxis( EAxisList::Type NewAxis );

	/** Notifies all active modes of mouse click messages. */
	bool HandleClick(FEditorViewportClient* InViewportClient,  HHitProxy *HitProxy, const FViewportClick &Click );

	/** true if the passed in brush actor should be drawn in wireframe */	
	bool ShouldDrawBrushWireframe( AActor* InActor ) const;

	/** true if brush vertices should be drawn */
	bool ShouldDrawBrushVertices() const;

	/** Ticks all active modes */
	void Tick( FEditorViewportClient* ViewportClient, float DeltaTime );

	/** Notifies all active modes of any change in mouse movement */
	bool InputDelta( FEditorViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale );

	/** Notifies all active modes of captured mouse movement */	
	bool CapturedMouseMove( FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY );

	/** Notifies all active modes of keyboard input */
	bool InputKey( FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);

	/** Notifies all active modes of axis movement */
	bool InputAxis( FEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime);

	bool MouseEnter( FEditorViewportClient* InViewportClient, FViewport* Viewport, int32 X, int32 Y );
	
	bool MouseLeave( FEditorViewportClient* InViewportClient, FViewport* Viewport );

	/** Notifies all active modes that the mouse has moved */
	bool MouseMove( FEditorViewportClient* InViewportClient, FViewport* Viewport, int32 X, int32 Y );

	/** Notifies all active modes that a viewport has received focus */
	bool ReceivedFocus( FEditorViewportClient* InViewportClient, FViewport* Viewport );

	/** Notifies all active modes that a viewport has lost focus */
	bool LostFocus( FEditorViewportClient* InViewportClient, FViewport* Viewport );

	/** Draws all active modes */	
	void DrawActiveModes( const FSceneView* InView, FPrimitiveDrawInterface* PDI );

	/** Renders all active modes */
	void Render( const FSceneView* InView, FViewport* Viewport, FPrimitiveDrawInterface* PDI );

	/** Draws the HUD for all active modes */
	void DrawHUD( FEditorViewportClient* InViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas );

	/** Calls PostUndo on all active components */
	void PostUndo();

	/** True if we should allow widget move */
	bool AllowWidgetMove() const;

	/** True if we should disallow mouse delta tracking. */
	bool DisallowMouseDeltaTracking() const;

	/** Get a cursor to override the default with, if any */
	bool GetCursor(EMouseCursor::Type& OutCursor) const;

	/**
	 * Returns a good location to draw the widget at.
	 */
	FVector GetWidgetLocation() const;

	/**
	 * Changes the current widget mode.
	 */
	void SetWidgetMode( FWidget::EWidgetMode InWidgetMode );

	/**
	 * Allows you to temporarily override the widget mode.  Call this function again
	 * with WM_None to turn off the override.
	 */
	void SetWidgetModeOverride( FWidget::EWidgetMode InWidgetMode );

	/**
	 * Retrieves the current widget mode, taking overrides into account.
	 */
	FWidget::EWidgetMode GetWidgetMode() const;

	/**
	 * Gets the current state of script editor usage of show friendly names
	 * @ return - If true, replace variable names with sanatized ones
	 */
	bool GetShowFriendlyVariableNames() const;

	/**
	 * Sets Interp Editor inversion of panning on drag
	 */
	void SetInterpPanInvert(bool bInInterpPanInverted) { bInterpPanInverted = bInInterpPanInverted; }

	/**
	 * Gets the current state of the interp editor panning
	 * @ return - If true, the interp editor left-right panning is inverted
	 */
	bool GetInterpPanInvert(void) const { return bInterpPanInverted; }

	/**
	 * Sets a bookmark in the levelinfo file, allocating it if necessary.
	 * 
	 * @param InIndex			Index of the bookmark to set.
	 * @param InViewportClient	Level editor viewport client used to get pointer the world that the bookmark is for.
	 */
	void SetBookmark( uint32 InIndex, FEditorViewportClient* InViewportClient );

	/**
	 * Checks to see if a bookmark exists at a given index
	 * 
	 * @param InIndex			Index of the bookmark to set.
	 * @param InViewportClient	Level editor viewport client used to get pointer the world to check for the bookmark in.
	 */
	bool CheckBookmark( uint32 InIndex, FEditorViewportClient* InViewportClient );

	/**
	 * Retrieves a bookmark from the list.
	 * 
	 * @param InIndex			Index of the bookmark to set.
	 * @param					Set to true to restore the visibility of any streaming levels.
	 * @param InViewportClient	Level editor viewport client used to get pointer the world that the bookmark is relevant to.
	 */
	void JumpToBookmark( uint32 InIndex, bool bShouldRestoreLevelVisibility, FEditorViewportClient* InViewportClient );

	/**
	 * Clears a bookmark from the list.
	 * 
	 * @param InIndex			Index of the bookmark to clear.
	 * @param InViewportClient	Level editor viewport client used to get pointer the world that the bookmark is relevant to.
	 */
	void ClearBookmark( uint32 InIndex, FEditorViewportClient* InViewportClient );

	/**
	 * Clears all book marks
	 * 
	 * @param InViewportClient	Level editor viewport client used to get pointer the world to clear the bookmarks from.
	 */
	void ClearAllBookmarks( FEditorViewportClient* InViewportClient );

	// FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	// End of FGCObject interface

	/**
	 * Loads the state that was saved in the INI file
	 */
	void LoadConfig(void);

	/**
	 * Saves the current state to the INI file
	 */
	void SaveConfig(void);

	/** 
	 * Sets the pivot locations
	 * 
	 * @param Location 		The location to set
	 * @param bIncGridBase	Whether or not to also set the GridBase
	 */
	void SetPivotLocation( const FVector& Location, const bool bIncGridBase );

	/**
	 * Multicast delegate for OnModeEntered and OnModeExited callbacks.
	 *
	 * First parameter:  The editor mode that was changed
	 * Second parameter:  True if entering the mode, or false if exiting the mode
	 */
	DECLARE_EVENT_TwoParams( FEditorModeTools, FEditorModeChangedEvent, FEdMode*, bool );
	FEditorModeChangedEvent& OnEditorModeChanged() { return EditorModeChangedEvent; }

	/** delegate type for triggering when widget mode changed */
	DECLARE_EVENT_OneParam( FEditorModeTools, FWidgetModeChangedEvent, FWidget::EWidgetMode );
	FWidgetModeChangedEvent& OnWidgetModeChanged() { return WidgetModeChangedEvent; }

	/**	Broadcasts the WidgetModeChanged event */
	void BroadcastWidgetModeChanged(FWidget::EWidgetMode InWidgetMode) { WidgetModeChangedEvent.Broadcast(InWidgetMode); }

	/**	Broadcasts the EditorModeChanged event */
	void BroadcastEditorModeChanged(FEdMode* Mode, bool IsEnteringMode) { EditorModeChangedEvent.Broadcast(Mode, IsEnteringMode); }

	/**
	 * Returns the current CoordSystem
	 * 
	 * @param bGetRawValue true when you want the actual value of CoordSystem, not the value modified by the state.
	 */
	ECoordSystem GetCoordSystem(bool bGetRawValue = false);

	/** Sets the current CoordSystem */
	void SetCoordSystem(ECoordSystem NewCoordSystem);

	/** Sets the hide viewport UI state */
	void SetHideViewportUI( bool bInHideViewportUI ) { bHideViewportUI = bInHideViewportUI; }

	/** Is the viewport UI hidden? */
	bool IsViewportUIHidden() const { return bHideViewportUI; }

	bool PivotShown, Snapping, SnappedActor;
	FVector CachedLocation, PivotLocation, SnappedLocation, GridBase;

	/** The angle for the translate rotate widget */
	float TranslateRotateXAxisAngle;

	/** Draws in the top level corner of all FEditorViewportClient windows (can be used to relay info to the user). */
	FString InfoString;

protected:
	/** 
	 * Delegate handlers
	 **/
	void OnEditorSelectionChanged(UObject* NewSelection);
	void OnEditorSelectNone();

	/** The mode ID to use as a default */
	FEditorModeID DefaultID;

	/** A list of active editor modes. */
	TArray< TSharedPtr<FEdMode> > Modes;

	/** A list of previously active editor modes that we will potentially recycle */
	TMap< FEditorModeID, TSharedPtr<FEdMode> > RecycledModes;

	/** The mode that the editor viewport widget is in. */
	FWidget::EWidgetMode WidgetMode;

	/** If the widget mode is being overridden, this will be != WM_None. */
	FWidget::EWidgetMode OverrideWidgetMode;

	/** If 1, draw the widget and let the user interact with it. */
	bool bShowWidget;

	/** If true, panning in the Interp Editor is inverted. If false, panning is normal*/
	bool bInterpPanInverted;

	/** if true, the viewports will hide all UI overlays */
	bool bHideViewportUI;

private:

	/** The coordinate system the widget is operating within. */
	ECoordSystem CoordSystem;

	/** Multicast delegate that is broadcast when a mode is entered or exited */
	FEditorModeChangedEvent EditorModeChangedEvent;

	/** Multicast delegate that is broadcast when a widget mode is changed */
	FWidgetModeChangedEvent WidgetModeChangedEvent;

	/** Flag set between calls to StartTracking() and EndTracking() */
	bool bIsTracking;
};
