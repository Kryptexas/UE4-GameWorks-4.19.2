// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


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
	FEdMode();
	virtual ~FEdMode();

	virtual bool MouseEnter( FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y );

	virtual bool MouseLeave( FLevelEditorViewportClient* ViewportClient,FViewport* Viewport );

	virtual bool MouseMove(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y);

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
	virtual bool CapturedMouseMove( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY );

	virtual bool InputKey(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,FKey Key,EInputEvent Event);
	virtual bool InputAxis(FLevelEditorViewportClient* InViewportClient,FViewport* Viewport,int32 ControllerId,FKey Key,float Delta,float DeltaTime);
	virtual bool InputDelta(FLevelEditorViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);
	virtual bool StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport);
	virtual bool EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport);
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

	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime);

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

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE {}

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

	// Allows modes to preview selection change events, and potentially make themselves active or inactive
	// Note: This method is called on *all* modes, not just ones that are currently active, and it is not currently called if matinee is active.
	// @param ItemUndergoingChange	Either an actor being selected or deselected, or a selection set being modified (typically emptied)
	virtual void PeekAtSelectionChangedEvent(UObject* ItemUndergoingChange) {}

	virtual bool HandleClick(FLevelEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click);

	/** Handling SelectActor */
	virtual bool Select( AActor* InActor, bool bInSelected ) { return 0; }

	/** Check to see if an actor can be selected in this mode - no side effects */
	virtual bool IsSelectionAllowed( AActor* InActor, bool bInSelection ) const { return true; }

	/** Returns the editor mode. */
	FEditorModeID GetID() const { return ID; }

	/** Returns the name of this mode (already localized) */
	const FText& GetName() const
	{
		return Name;
	}

	/** Returns the icon for the mode */
	FSlateIcon GetIcon() const
	{
		return IconBrush.IsSet() ? IconBrush : FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.EditorModes");
	}

	/** Returns true if the editor mode should be explicit and visible in any menu/toolbar for modes */
	bool IsVisible() const { return bVisible; }

	/** Returns the priority of this editor mode */
	int32 GetPriorityOrder() const { return PriorityOrder; }

	friend class FEditorModeTools;

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
	virtual void DrawHUD(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas);
	//@}

	/**
	 * Called when attempting to duplicate the selected actors by alt+dragging,
	 * return true to prevent normal duplication.
	 */
	virtual bool HandleDragDuplicate() { return false; }

	/**
	 * Called when the mode wants to draw brackets around selected objects
	 */
	virtual void DrawBrackets( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas );

	/** True if this mode uses a toolkit mode (eventually they all should) */
	virtual bool UsesToolkits() const;

	UWorld* GetWorld() const
	{
		return GWorld;
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
	/** Returns true if this property can support creating a widget in the editor */
	static bool CanCreateWidgetForProperty(UProperty* InProp);
	/** See if we should create a widget for the supplied property when selecting an actor instance */
	static bool ShouldCreateWidgetForProperty(UProperty* InProp);

protected:
	/** Name for the editor to display. */
	FText Name;

	/** The current axis that is being dragged on the widget. */
	EAxisList::Type CurrentWidgetAxis;

	/** Optional array of tools for this mode. */
	TArray<FModeTool*> Tools;

	/** The tool that is currently active within this mode. */
	FModeTool* CurrentTool;

	/** The mode ID. */
	FEditorModeID ID;

	/** The mode icon. */
	FSlateIcon IconBrush;

	/** Whether or not the mode should be visible in the mode menu. */
	bool bVisible;

	/** The priority of this mode which will determine its default order and shift+X command assignment */
	int32 PriorityOrder;

	/** Editor Mode Toolkit that is associated with this toolkit mode */
	TSharedPtr<class FModeToolkit> Toolkit;

	// Property Widgets

	/** Structure that holds info about our optional property widget */
	struct FPropertyWidgetInfo
	{
		FString PropertyName;
		int32 PropertyIndex;
		FName PropertyValidationName;
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
	virtual bool UsesPropertyWidgets() const OVERRIDE { return true; }
	// End of FEdMode interface
};



/**
 * A helper class to store the state of the various editor modes.
 */
class FEditorModeTools : public FGCObject
{
public:
	FEditorModeTools();
	virtual ~FEditorModeTools();

	void Init();

	void Shutdown();

	/**
	 * Registers an editor mode.  Typically called from a module's StartupModule() routine.
	 *
	 * @param ModeToRegister	The editor mode instance to register
	 */
	UNREALED_API void RegisterMode(TSharedRef<FEdMode> ModeToRegister);

	/**
	 * Unregisters an editor mode.  Typically called from a module's ShutdownModule() routine.
	 * Note: will exit the edit mode if it is currently active.
	 *
	 * @param ModeToUnregister	The editor mode instance to unregister
	 */
	UNREALED_API void UnregisterMode(TSharedRef<FEdMode> ModeToUnregister);

	UNREALED_API void RegisterCompatibleModes( const FEditorModeID& Mode, const FEditorModeID& CompatibleMode );

	UNREALED_API void UnregisterCompatibleModes( const FEditorModeID& Mode, const FEditorModeID& CompatibleMode );

	/**
	 * Activates an editor mode. Shuts down all other active modes which cannot run with the passed in mode.
	 * 
	 * @param InID		The ID of the editor mode to activate.
	 * @param bToggle	true if the passed in editor mode should be toggled off if it is already active.
	 */
	UNREALED_API void ActivateMode( FEditorModeID InID, bool bToggle = false );

	/**
	 * Deactivates an editor mode. 
	 * 
	 * @param InID		The ID of the editor mode to deactivate.
	 */
	UNREALED_API void DeactivateMode( FEditorModeID InID );

	/**
	 * Deactivates all modes, note some modes can never be deactivated.
	 */
	UNREALED_API void DeactivateAllModes();

	/** 
	 * Returns the editor mode specified by the passed in ID
	 */
	UNREALED_API FEdMode* FindMode( FEditorModeID InID );

	/**
	 * Returns true if the current mode is not the specified ModeID.  Also optionally warns the user.
	 *
	 * @param	ModeID			The editor mode to query.
	 * @param	ErrorMsg		If specified, inform the user the reason why this is a problem
	 * @param	bNotifyUser		If true, display the error as a notification, instead of a dialog
	 * @return					true if the current mode is not the specified mode.
	 */
	UNREALED_API bool EnsureNotInMode(FEditorModeID ModeID, const FText& ErrorMsg = FText::GetEmpty(), bool bNotifyUser = false) const;

	UNREALED_API FMatrix GetCustomDrawingCoordinateSystem();
	FMatrix GetCustomInputCoordinateSystem();
	
	/** 
	 * Returns true if the passed in editor mode is active 
	 */
	UNREALED_API bool IsModeActive( FEditorModeID InID ) const;

	/**
	 * Returns a pointer to an active mode specified by the passed in ID
	 * If the editor mode is not active, NULL is returned
	 */
	UNREALED_API FEdMode* GetActiveMode( FEditorModeID InID );
	UNREALED_API const FEdMode* GetActiveMode( FEditorModeID InID ) const;

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
	UNREALED_API void GetActiveModes( TArray<FEdMode*>& OutActiveModes );

	/** 
	 * Returns an array of all modes.
	 */
	UNREALED_API void GetModes( TArray<FEdMode*>& OutModes );

	void SetShowWidget( bool InShowWidget )	{ bShowWidget = InShowWidget; }
	bool GetShowWidget() const				{ return bShowWidget; }

	void CycleWidgetMode (void);

	/** Allow translate/rotate widget */
	bool GetAllowTranslateRotateZWidget()                 { return GEditor->GetEditorUserSettings().bAllowTranslateRotateZWidget; }
	void SetAllowTranslateRotateZWidget (const bool bInOnOff) { GEditor->AccessEditorUserSettings().bAllowTranslateRotateZWidget = bInOnOff; }

	/**Save Widget Settings to Ini file*/
	void SaveWidgetSettings();
	/**Load Widget Settings from Ini file*/
	void LoadWidgetSettings();

	/** Gets the widget axis to be drawn */
	EAxisList::Type GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const;

	/** Mouse tracking interface.  Passes tracking messages to all active modes */
	bool StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport);
	bool EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport);
	bool IsTracking() const { return bIsTracking; }

	/** Notifies all active modes that a map change has occured */
	UNREALED_API void MapChangeNotify();

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
	bool HandleClick(FLevelEditorViewportClient* InViewportClient,  HHitProxy *HitProxy, const FViewportClick &Click );

	/** true if the passed in brush actor should be drawn in wireframe */	
	bool ShouldDrawBrushWireframe( AActor* InActor ) const;

	/** true if brush vertices should be drawn */
	bool ShouldDrawBrushVertices() const;

	/** Ticks all active modes */
	void Tick( FLevelEditorViewportClient* ViewportClient, float DeltaTime );

	/** Notifies all active modes of any change in mouse movement */
	bool InputDelta( FLevelEditorViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale );

	/** Notifies all active modes of captured mouse movement */	
	bool CapturedMouseMove( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY );

	/** Notifies all active modes of keyboard input */
	bool InputKey( FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);

	/** Notifies all active modes of axis movement */
	bool InputAxis( FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime);

	bool MouseEnter( FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, int32 X, int32 Y );
	
	bool MouseLeave( FLevelEditorViewportClient* InViewportClient, FViewport* Viewport );

	/** Notifies all active modes that the mouse has moved */
	bool MouseMove( FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, int32 X, int32 Y );

	/** Draws all active modes */	
	void DrawActiveModes( const FSceneView* InView, FPrimitiveDrawInterface* PDI );

	/** Renders all active modes */
	UNREALED_API void Render( const FSceneView* InView, FViewport* Viewport, FPrimitiveDrawInterface* PDI );

	/** Draws the HUD for all active modes */
	void DrawHUD( FLevelEditorViewportClient* InViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas );

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
	UNREALED_API FVector GetWidgetLocation() const;

	/**
	 * Changes the current widget mode.
	 */
	UNREALED_API void SetWidgetMode( FWidget::EWidgetMode InWidgetMode );

	/**
	 * Allows you to temporarily override the widget mode.  Call this function again
	 * with WM_None to turn off the override.
	 */
	void SetWidgetModeOverride( FWidget::EWidgetMode InWidgetMode );

	/**
	 * Retrieves the current widget mode, taking overrides into account.
	 */
	UNREALED_API FWidget::EWidgetMode GetWidgetMode() const;

	/**
	 * Sets editor wide usage of replace respecting scale
	 */
	void SetReplaceRespectsScale(bool bInReplaceRespectsScale) { GEditor->AccessEditorUserSettings().bReplaceRespectsScale = bInReplaceRespectsScale; }
	/**
	 * Gets the current state of editor wide usage of replace respecting scale
	 * @ return - If true, replace respects scale
	 */
	bool GetReplaceRespectsScale(void) const { return GEditor->GetEditorUserSettings().bReplaceRespectsScale; }

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
	 * Sets clicking BSP surface selecting brush
	 */
	void SetClickBSPSelectsBrush(bool bInClickBSPSelectsBrush) { GEditor->AccessEditorUserSettings().bClickBSPSelectsBrush = bInClickBSPSelectsBrush; }
	/**
	 * Gets the current state of clicking BSP surface selecting brush
	 * @ return - If true, the clicking BSP surface selects brush
	 */
	bool GetClickBSPSelectsBrush(void) const { return GEditor->GetEditorUserSettings().bClickBSPSelectsBrush; }
	/**
	 * Sets BSP auto updating
	 */
	void SetBSPAutoUpdate(bool bInBSPAutoUpdate) { GEditor->AccessEditorUserSettings().bBSPAutoUpdate = bInBSPAutoUpdate; }
	/**
	 * Gets the current state of BSP auto-updating
	 * @ return - If true, BSP auto-updating is enabled
	 */
	bool GetBSPAutoUpdate(void) const { return GEditor->GetEditorUserSettings().bBSPAutoUpdate; }
	/**
	 * Sets Navigation auto updating
	 */
	void SetNavigationAutoUpdate(bool bInNavigationAutoUpdate) { GEditor->AccessEditorUserSettings().bNavigationAutoUpdate = bInNavigationAutoUpdate; }
	/**
	 * Gets the current state of Navigation auto-updating
	 * @ return - If true, Navigation auto-updating is enabled
	 */
	bool GetNavigationAutoUpdate(void) const { return GEditor->GetEditorUserSettings().bNavigationAutoUpdate; }

	/**
	 * Sets a bookmark in the levelinfo file, allocating it if necessary.
	 * 
	 * @param InIndex			Index of the bookmark to set.
	 * @param InViewportClient	Level editor viewport client used to get pointer the world that the bookmark is for.
	 */
	UNREALED_API void SetBookmark( uint32 InIndex, FLevelEditorViewportClient* InViewportClient );

	/**
	 * Checks to see if a bookmark exists at a given index
	 * 
	 * @param InIndex			Index of the bookmark to set.
	 * @param InViewportClient	Level editor viewport client used to get pointer the world to check for the bookmark in.
	 */
	UNREALED_API bool CheckBookmark( uint32 InIndex, FLevelEditorViewportClient* InViewportClient );

	/**
	 * Retrieves a bookmark from the list.
	 * 
	 * @param InIndex			Index of the bookmark to set.
	 * @param					Set to true to restore the visibility of any streaming levels.
	 * @param InViewportClient	Level editor viewport client used to get pointer the world that the bookmark is relevant to.
	 */
	UNREALED_API void JumpToBookmark( uint32 InIndex, bool bShouldRestoreLevelVisibility, FLevelEditorViewportClient* InViewportClient );

	/**
	 * Clears a bookmark from the list.
	 * 
	 * @param InIndex			Index of the bookmark to clear.
	 * @param InViewportClient	Level editor viewport client used to get pointer the world that the bookmark is relevant to.
	 */
	UNREALED_API void ClearBookmark( uint32 InIndex, FLevelEditorViewportClient* InViewportClient );

	/**
	 * Clears all book marks
	 * 
	 * @param InViewportClient	Level editor viewport client used to get pointer the world to clear the bookmarks from.
	 */
	UNREALED_API void ClearAllBookmarks( FLevelEditorViewportClient* InViewportClient );

	// FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;
	// End of FGCObject interface

	/**
	 * Loads the state that was saved in the INI file
	 */
	void LoadConfig(void);

	/**
	 * Saves the current state to the INI file
	 */
	UNREALED_API void SaveConfig(void);

	/** 
	 * Returns a list of active modes that are incompatible with the passed in mode.
	 * 
	 * @param InID 				The mode to check for incompatibilites.
	 * @param IncompatibleModes	The list of incompatible modes.
	 */
	void GetIncompatibleActiveModes( FEditorModeID InID, TArray<FEdMode*>& IncompatibleModes );

	/** 
	 * Sets the pivot locations
	 * 
	 * @param Location 		The location to set
	 * @param bIncGridBase	Whether or not to also set the GridBase
	 */
	UNREALED_API void SetPivotLocation( const FVector& Location, const bool bIncGridBase );

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
	void BroadcastWidgetModeChanged(FWidget::EWidgetMode InWidgetMode) { WidgetModeChangedEvent.Broadcast( InWidgetMode ); }

	DECLARE_EVENT( FEditorModeTools, FRegisteredModesChangedEvent );
	FRegisteredModesChangedEvent& OnRegisteredModesChanged() { return RegisteredModesChanged; }

	/**	Broadcasts the registered modes changed event */
	void BroadcastRegisteredModesChanged() { RegisteredModesChanged.Broadcast( ); }

	/**
	 * Returns the current CoordSystem
	 * 
	 * @param bGetRawValue true when you want the actual value of CoordSystem, not the value modified by the state.
	 */
	UNREALED_API ECoordSystem GetCoordSystem(bool bGetRawValue = false);

	/** Sets the current CoordSystem */
	UNREALED_API void SetCoordSystem(ECoordSystem NewCoordSystem);

	/** Sets the hide viewport UI state */
	void SetHideViewportUI( bool bInHideViewportUI ) { bHideViewportUI = bInHideViewportUI; }

	/** Is the viewport UI hidden? */
	bool IsViewportUIHidden() const { return bHideViewportUI; }

	bool PivotShown, Snapping, SnappedActor;
	FVector CachedLocation, PivotLocation, SnappedLocation, GridBase;

	/** The angle for the translate rotate widget */
	float TranslateRotateXAxisAngle;

	/** Draws in the top level corner of all FLevelEditorViewportClient windows (can be used to relay info to the user). */
	FString InfoString;

protected:
	/** 
	 * Delegate handlers
	 **/
	void OnEditorSelectionChanged(UObject* NewSelection);
	void OnEditorSelectNone();

	/** A mapping of editor modes that can be active at the same time */
	TMultiMap<FEditorModeID, FEditorModeID> ModeCompatabilityMap;

	/** A list of all available editor modes. */
	TArray< TSharedPtr<FEdMode> > Modes;

	/** The editor modes currently in use. */
	TArray<FEdMode*> ActiveModes;

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

	/**	Broadcasts the EditorModeChanged event */
	void BroadcastEditorModeChanged(FEdMode* Mode, bool IsEnteringMode) { EditorModeChangedEvent.Broadcast( Mode, IsEnteringMode ); }

	friend class FEdMode;

private:

	/** The coordinate system the widget is operating within. */
	ECoordSystem CoordSystem;

	/** Multicast delegate that is broadcast when a mode is entered or exited */
	FEditorModeChangedEvent EditorModeChangedEvent;

	/** Multicast delegate that is broadcast when a widget mode is changed */
	FWidgetModeChangedEvent WidgetModeChangedEvent;

	FRegisteredModesChangedEvent RegisteredModesChanged;

	/** Flag set between calls to StartTracking() and EndTracking() */
	bool bIsTracking;
};
