// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MaterialEditor.h"
#include "LinkedObjEditor.h"
#include "LinkedObjDrawUtils.h"




// Forward declarations.
class FMaterialEditorPreviewVC;
class FScopedTransaction;
class UMaterial;
class UMaterialEditorMeshComponent;
class UMaterialEditorOptions;
class UMaterialExpression;
struct FLinkedObjDrawInfo;







/**
 * The material editor canvas.
 */
class SMaterialEditorCanvas : public SCompoundWidget, public FNotifyHook, public FLinkedObjEdNotifyInterface, public FGCObject, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS( SMaterialEditorCanvas ){}
		/** Parent material editor*/
		SLATE_ARGUMENT(TWeakPtr<FMaterialEditor>, MaterialEditor)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	
	/** The expression currently being previewed.  This is NULL when not in expression preview mode. */
	UMaterialExpression* PreviewExpression;
	
protected:
	// SWidget interface
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) OVERRIDE;
	// End of SWidget interface
	
public:
	SMaterialEditorCanvas();

	virtual ~SMaterialEditorCanvas();
	
	/** Callback for when the canvas search next and previous are used */
	void OnSearch(SSearchBox::SearchDirection Direction);
	
	/** Callback for when the canvas search box is changed */
	void OnSearchChanged( const FString& InFilterText );

	/** The canvas was added to the OwnerTab */
	void OnAddedToTab( const TSharedRef<SDockTab>& OwnerTab );
	
	/**
	 * Creates a new material expression of the specified class.  Will automatically connect output 0
	 * of the new expression to an input tab, if one was clicked on.
	 *
	 * @param	NewExpressionClass		The type of material expression to add.  Must be a child of UMaterialExpression.
	 * @param	bAutoConnect			If true, connect the new material expression to the most recently selected connector, if possible.
	 * @param	bAutoSelect				If true, deselect all expressions and select the newly created one.
	 * @param	NodePos					Position of the new node.
	 */
	UMaterialExpression* CreateNewMaterialExpression(UClass* NewExpressionClass, bool bAutoConnect, bool bAutoSelect, bool bAutoAssignResource, const FIntPoint& NodePos);
	
	/** Sets the expression to be previewed. */
	void SetPreviewExpression(UMaterialExpression* NewPreviewExpression);
	
	/**
	 * Refreshes all material expression previews, regardless of whether or not realtime previews are enabled.
	 */
	void ForceRefreshExpressionPreviews();

	/**
	 * Updates the editor's property window to contain material expression properties if any are selected.
	 * Otherwise, properties for the material are displayed.
	 */
	void UpdatePropertyWindow();
	
	/**
	 * Connects the specified input expression to the specified output expression.
	 */
	static void ConnectExpressions(FExpressionInput& Input, const FExpressionOutput& Output, int32 OutputIndex, UMaterialExpression* Expression);
	
	/**
	 * Connects the MaterialInputIndex'th material input to the MaterialExpressionOutputIndex'th material expression output.
	 */
	static void ConnectMaterialToMaterialExpression(UMaterial* Material,
													int32 MaterialInputIndex,
													UMaterialExpression* MaterialExpression,
													int32 MaterialExpressionOutputIndex,
													bool bUnusedConnectionsHidden);

	/**
	 * Draws messages on the specified viewport and canvas.
	 */
	void DrawMessages( FViewport* Viewport, FCanvas* Canvas );

	/** Centers camera around the home node */
	void RecenterCamera(bool bCenterY = true);

private:
	/** The parent tab where this viewport resides */
	TWeakPtr<SDockTab> ParentTab;

	/** Builds the context menu widget for canvas when clicking on empty space */
	TSharedRef<SWidget> BuildMenuWidgetEmpty();
	
	/** Builds the context menu widget for canvas when clicking on a node */
	TSharedRef<SWidget> BuildMenuWidgetObjects();

	/** Builds the context menu widget for canvas when clicking on a connector */
	TSharedRef<SWidget> BuildMenuWidgetConnector();
	
	/** Flags the material as dirty */
	void SetMaterialDirty();
	
	/**
	 * Refreshes the viewport containing the material expression graph.
	 */
	void RefreshExpressionViewport();
	
	/**
	 * Refreshes material expression previews.  Refreshes all previews if bAlwaysRefreshAllPreviews is true.
	 * Otherwise, refreshes only those previews that have a bRealtimePreview of true.
	 */
	void RefreshExpressionPreviews();

	/**
	 * Refreshes the preview for the specified material expression.  Does nothing if the specified expression
	 * has a bRealtimePreview of false.
	 *
	 * @param	MaterialExpression		The material expression to update.
	 */
	void RefreshExpressionPreview(UMaterialExpression* MaterialExpression, bool bRecompile);

	/**
	 * Run the HLSL material translator and refreshes the View Source window.
	 */
	void RefreshSourceWindowMaterial();

	/**
	 * Draws the root node -- that is, the node corresponding to the final material.
	 */
	void DrawMaterialRoot(bool bIsSelected, FCanvas* Canvas);

	/**
	 * Render connections between the material's inputs and the material expression outputs connected to them.
	 */
	void DrawMaterialRootConnections(FCanvas* Canvas);

	/**
	 * Draws the specified material expression node.
	 */
	void DrawMaterialExpression(UMaterialExpression* MaterialExpression, bool bExpressionSelected, FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo);

	/**
	 * Render connectors between this material expression's inputs and the material expression outputs connected to them.
	 */
	void DrawMaterialExpressionConnections(UMaterialExpression* MaterialExpression, FCanvas* Canvas);

	/**
	 * Draws comments for the specified material expression node.
	 */
	void DrawMaterialExpressionComments(UMaterialExpression* MaterialExpression, FCanvas* Canvas);

	/** Draws tooltips for material expressions. */
	void DrawMaterialExpressionToolTips(const TArray<FLinkedObjDrawInfo>& LinkedObjDrawInfos, FCanvas* Canvas);

	/**
	 * Draws UMaterialExpressionComment nodes.
	 */
	void DrawCommentExpressions(FCanvas* Canvas);

	/**
	 * Highlights any active logic connectors in the specified linked object.
	 */
	void HighlightActiveConnectors(FLinkedObjDrawInfo &ObjInfo);

	/** @return Returns whether the specified connector is currently highlighted or not. */
	bool IsConnectorHighlighted(UObject* Object, uint8 ConnType, int32 ConnIndex);

	/** @return Returns whether the specified connector is currently marked or not. */
	bool IsConnectorMarked(UObject* Object, uint8 ConnType, int32 ConnIndex);

	// FLinkedObjViewportClient interface

	virtual void DrawObjects(FViewport* Viewport, FCanvas* Canvas);

	/**
	 * Called when the user right-clicks on an empty region of the expression viewport.
	 */
	virtual void OpenNewObjectMenu();

	/**
	 * Called when the user right-clicks on an object in the viewport.
	 */
	virtual void OpenObjectOptionsMenu();

	/**
	 * Called when the user right-clicks on an object connector in the viewport.
	 */
	virtual void OpenConnectorOptionsMenu();
	
	/** Updates the material being previewed in the viewport */
	void UpdatePreviewMaterial();

	/**
	 * Deselects all selected material expressions and comments.
	 */
	virtual void EmptySelection();


	/**
	 * Add the specified object to the list of selected objects
	 */
	virtual void AddToSelection(UObject* Obj);

	/**
	 * Remove the specified object from the list of selected objects.
	 */
	virtual void RemoveFromSelection(UObject* Obj);

	/**
	 * Checks whether the specified object is currently selected.
	 *
	 * @return	true if the specified object is currently selected
	 */
	virtual bool IsInSelection(UObject* Obj) const;

	/**
	 * Returns the number of selected objects
	 */
	virtual int32 GetNumSelected() const;

	/**
	 * Called when the user clicks on an unselected link connector.
	 *
	 * @param	Connector	the link connector that was clicked on
	 */
	virtual void SetSelectedConnector(FLinkedObjectConnector& Connector);

	/**
	 * Gets the position of the selected connector.
	 *
	 * @return	an FIntPoint that represents the position of the selected connector, or (0,0)
	 *			if no connectors are currently selected
	 */
	virtual FIntPoint GetSelectedConnLocation(FCanvas* Canvas);

	/**
	 * Gets the EConnectorHitProxyType for the currently selected connector
	 *
	 * @return	the type for the currently selected connector, or 0 if no connector is selected
	 */
	virtual int32 GetSelectedConnectorType();

	/**
	 * Makes a connection between connectors.
	 */
	virtual void MakeConnectionToConnector(FLinkedObjectConnector& Connector);

	/** Makes a connection between the currently selected connector and the passed in end point. */
	void MakeConnectionToConnector(TWeakObjectPtr<UObject> EndObject, int32 EndConnType, int32 EndConnIndex);

	/**
	 * Called when the user releases the mouse over a link connector and is holding the ALT key.
	 * Commonly used as a shortcut to breaking connections.
	 *
	 * @param	Connector	The connector that was ALT+clicked upon.
	 */
	virtual void AltClickConnector(FLinkedObjectConnector& Connector);

	/**
	 * Called when the user double-clicks an object in the viewport
	 *
	 * @param	Obj		the object that was double-clicked on
	 */
	virtual void DoubleClickedObject(UObject* Obj);

	/**
	 * Called when double-clicking a connector.
	 * Used to pan the connection's link into view
	 *
	 * @param	Connector	The connector that was double-clicked
	 */
	virtual void DoubleClickedConnector(FLinkedObjectConnector& Connector);

	/**
	 * Called when the user performs a draw operation while objects are selected.
	 *
	 * @param	DeltaX	the X value to move the objects
	 * @param	DeltaY	the Y value to move the objects
	 */
	virtual void MoveSelectedObjects(int32 DeltaX, int32 DeltaY);

	virtual bool EdHandleKeyInput(FViewport* Viewport, FKey Key, EInputEvent Event) OVERRIDE;

	/**
	 *	Handling for dragging on 'special' hit proxies. Should only have 1 object selected when this is being called. 
	 *	In this case used for handling resizing handles on Comment objects. 
	 *
	 *	@param	DeltaX			Horizontal drag distance this frame (in pixels)
	 *	@param	DeltaY			Vertical drag distance this frame (in pixels)
	 *	@param	SpecialIndex	Used to indicate different classes of action. @see HLinkedObjProxySpecial.
	 */
	virtual void SpecialDrag( int32 DeltaX, int32 DeltaY, int32 NewX, int32 NewY, int32 SpecialIndex );

	/**
	 * Called once the user begins a drag operation.  Transactions expression, comment position.
	 */
	virtual void BeginTransactionOnSelected();

	/**
	 * Called when the user releases the mouse button while a drag operation is active.
	 */
	virtual void EndTransactionOnSelected();

	// FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	// FNotifyHook interface
	virtual void NotifyPreChange(UProperty* PropertyAboutToChange);
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged);

	// Accessors for the material or material function stored in the parent
	UMaterial* GetMaterial();
	UMaterialFunction* GetMaterialFunction();

	/** Pointer back to the material editor tool that owns us */
	TWeakPtr<FMaterialEditor> MaterialEditorPtr;
	
	/** The set of selected material expressions. */
	TArray<UMaterialExpression*> SelectedExpressions;

	/** The set of selected comments. */
	TArray<UMaterialExpressionComment*> SelectedComments;

	/** Flag which determines if you have right clicked on the home node */
	bool bHomeNodeSelected;

	/** Object containing selected connector. */
	TWeakObjectPtr<UObject> ConnObj;

	/** Type of selected connector. */
	int32 ConnType;

	/** Index of selected connector. */
	int32 ConnIndex;

	/** The viewport client for the material editor's linked object editor. */
	TSharedPtr<FLinkedObjViewportClient> LinkedObjVC;

	/** Slate viewport for rendering and I/O */
	TSharedPtr<class FSceneViewport> Viewport;

	/** Viewport widget*/
	TSharedPtr<SViewport> ViewportWidget;

	UTexture2D* NodeCollapsedTexture;
	UTexture2D* NodeExpandedTexture;
	UTexture2D* NodeExpandedTextureRealtimePreview;

	static const int32 NumFunctionOutputsSupported;

	class FMaterialNodeDrawInfo
	{
	public:
		int32 DrawWidth;
		TArray<int32> LeftYs;
		TArray<int32> RightYs;

		FMaterialNodeDrawInfo()
			:	DrawWidth( 0 )
		{}
		FMaterialNodeDrawInfo(int32 InOutputY)
			:	DrawWidth( 0 )
		{
			// Initialize the "output" connectors array with five entries, for material expression.
			LeftYs.Add( InOutputY );
			LeftYs.Add( InOutputY );
			LeftYs.Add( InOutputY );
			LeftYs.Add( InOutputY );
			LeftYs.Add( InOutputY );
		}
	};

	/** Draw information for the material itself. */
	FMaterialNodeDrawInfo							MaterialDrawInfo;

	/** Draw information for material expressions. */
	TMap<UObject*,FMaterialNodeDrawInfo>			MaterialNodeDrawInfo;

	/** If true, always refresh all expression previews.  This overrides UMaterialExpression::bRealtimePreview. */
	bool											bAlwaysRefreshAllPreviews;

	/** Material expression previews. */
	TIndirectArray<class FMatExpressionPreview>		ExpressionPreviews;

	/** If true, don't render connectors that are not connected to anything. */
	bool											bHideUnusedConnectors;

	/** If true, show material stats like number of shader instructions. */
	bool											bShowStats;

	/** If true, show material stats and errors for mobile. */
	bool											bShowMobileStats;

	/** If false, sort material expressions using categorized menus. */
	bool	bUseUnsortedMenus;

	/** Current search query */
	FString SearchQuery;

	/** Array of expressions matching the current search terms. */
	TArray<UMaterialExpression*> SearchResults;

	/** Index in the above array of the currently selected search result */
	int32 SelectedSearchResult;

	/** For double-clicking connectors */
	UObject* DblClickObject;
	int32 DblClickConnType;
	int32 DblClickConnIndex;

	/** For shift-clicking connectors */
	UObject* MarkedObject;
	int32 MarkedConnType;
	int32 MarkedConnIndex;

	/**
	 * A human-readable name - material expression input pair.
	 */
	class FMaterialInputInfo
	{
	public:
		FMaterialInputInfo()
		{}
		FMaterialInputInfo(const TCHAR* InName, FExpressionInput* InInput, EMaterialProperty InProperty)
			:	Name( InName )
			,	Input( InInput )
			,	Property( InProperty )
		{}
		FString Name;
		FExpressionInput* Input;
		EMaterialProperty Property;
	};

	TArray<FMaterialInputInfo> MaterialInputs;

	/** The current transaction. */
	FScopedTransaction*								ScopedTransaction;

	/** Expressions being moved as a result of lying withing a comment expression. */
	TArray<UMaterialExpression*> ExpressionsLinkedToComments;
	
	/** Reference to owner of the current popup */
	TSharedPtr<class SWindow> NameEntryPopupWindow;

	/**
	 * @param		InMaterial	The material to query.
	 * @param		ConnType	Type of the connection (LOC_OUTPUT).
	 * @param		ConnIndex	Index of the connection to query
	 * @return					A viewport location for the connection.
	 */
	FIntPoint GetMaterialConnectionLocation(UMaterial* InMaterial, int32 ConnType, int32 ConnIndex);

	/**
	 * @param		InMaterialExpression	The material expression to query.
	 * @param		ConnType				Type of the connection (LOC_INPUT or LOC_OUTPUT).
	 * @param		ConnIndex				Index of the connection to query
	 * @return								A viewport location for the connection.
	 */
	FIntPoint GetExpressionConnectionLocation(UMaterialExpression* InMaterialExpression, int32 ConnType, int32 ConnIndex);

	/**
	 * Returns the drawinfo object for the specified expression, creating it if one does not currently exist.
	 */
	FMaterialNodeDrawInfo& GetDrawInfo(UMaterialExpression* MaterialExpression);

	/**
	 * Returns the expression preview for the specified material expression.
	 */
	FMatExpressionPreview* GetExpressionPreview(UMaterialExpression* MaterialExpression, bool& bNewlyCreated);

	/**
	 * Disconnects and removes the given expressions and comments.
	 */
	void DeleteObjects(const TArray<UMaterialExpression*>& Expressions, const TArray<UMaterialExpressionComment*>& Comments);

	/**
	 * Disconnects and removes the selected material expressions.
	 */
	void DeleteSelectedObjects();

	/**
	 * Duplicates the selected material expressions.  Preserves internal references.
	 */
	void DuplicateSelectedObjects();

	/**
	 * Pastes into this material from the editor's material copy buffer.
	 *
	 * @param	PasteLocation	If non-NULL locate the upper-left corner of the new nodes' bound at this location.
	 */
	void PasteExpressionsIntoMaterial(const FIntPoint* PasteLocation);

	/**
	 * Deletes any disconnected material expressions.
	 */
	void CleanUnusedExpressions();

	/** 
	 * Computes a bounding box for the selected material expressions.  Output is not sensible if no expressions are selected.
	 */
	FIntRect GetBoundingBoxOfSelectedExpressions();

	/** 
	 * Computes a bounding box for the specified set of material expressions.  Output is not sensible if the set is empty.
	 */
	FIntRect GetBoundingBoxOfExpressions(const TArray<UMaterialExpression*>& Expressions);

	/**
	 * Creates a new material expression comment frame.
	 */
	void CreateNewCommentExpression();
	/** Callback for committing to a new comment */
	void NewCommentCommitted(const FText& CommentText, ETextCommit::Type CommitInfo);

	void DrawMaterialExpressionLinkedObj(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const TArray<FString>& Captions, const TCHAR* Comment, const FColor& BorderColor, const FColor& TitleBkgColor, UMaterialExpression* MaterialExpression, bool bRenderPreview);

	/**
	 * Breaks the link currently indicated by ConnObj/ConnType/ConnIndex.
	 */
	void BreakConnLink(bool bOnlyIfMouseOver);

	/** Updates the marked connector based on what's currently under the mouse, and potentially makes a new connection. */
	void UpdateMarkedConnector();

	/**
	 * Breaks all connections to/from selected material expressions.
	 */
	void BreakAllConnectionsToSelectedExpressions();
	
	/** Removes the selected expression from the favorites list. */
	void RemoveSelectedExpressionFromFavorites();
	/** Adds the selected expression to the favorites list. */
	void AddSelectedExpressionToFavorites();

	/**
	 * Connects an expression output to the specified material input slot.
	 *
	 * @param	MaterialInputIndex		Index to the material input (Diffuse=0, Emissive=1, etc).
	 */
	void OnConnectToMaterial(int32 MaterialInputIndex);

	/**
	 * Uses the global Undo transactor to reverse changes, update viewports etc.
	 */
	void Undo();

	/**
	 * Uses the global Undo transactor to redo changes, update viewports etc.
	 */
	void Redo();

	/**
	  * Updates the SearchResults array based on the search query
	  *
	  * @param	bQueryChanged		Indicates whether the update is because of a query change or a potential material content change.
	  */
	void UpdateSearch( bool bQueryChanged );

	/**
	  * Moves the shows the selected result in the viewport
	  */
	void ShowSearchResult();

	/**
	* PanLocationOnscreen: pan the viewport if necessary to make the particular location visible
	*
	* @param	X, Y		Coordinates of the location we want onscreen
	* @param	Border		Minimum distance we want the location from the edge of the screen.
	*/
	void PanLocationOnscreen( int32 X, int32 Y, int32 Border );

	/**
	* IsActiveMaterialInput: get whether a particular input should be considered "active"
	* based on whether it is relevant to the current material.
	* e.g. SubsurfaceXXX inputs are only active when EnableSubsurfaceScattering is true
	*
	* @param	InputInfo		The input to check.
	* @return	true if the input is active and should be shown normally, false if the input should be drawn to indicate that it is inactive.
	*/
	bool IsActiveMaterialInput(const FMaterialInputInfo& InputInfo);

	/**
	* IsVisibleMaterialInput: get whether a particular input should be visible
	*
	* @param	InputInfo		The input to check.
	* @return	true if the input is visible and should be shown, false if the input should be hidden.
	*/
	bool IsVisibleMaterialInput(const FMaterialInputInfo& InputInfo);

	/** Appends entries to the specified submenu for creating new material expressions */
	void NewShaderExpressionsSubMenu(FMenuBuilder& MenuBuilder, TArray<struct FMaterialExpression>* MaterialExpressions);

	/**
	 * Appends entries to the specified menu for creating new material expressions.
	 *
	 * @param	MenuBuilder		The MenuBuilder to add items to
	 */
	void NewShaderExpressionsMenu(FMenuBuilder& MenuBuilder);
	
	/** Pointer to the object that the current color picker is working on. Can be NULL and stale. */
	TWeakObjectPtr<UObject> ColorPickerObject;

	/** Called before the color picker commits a change. */
	void PreColorPickerCommit(FLinearColor LinearColor);

	/** Called whenever the color picker is used and accepted. */
	void OnColorPickerCommitted(FLinearColor LinearColor);
	
	/** Determines if this canvas is visible to the user or not */
	bool IsVisible() const;

	/** Callback to create a new material expression */
	void OnNewMaterialExpression(UClass* MaterialExpression);

	// Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) OVERRIDE;
	virtual void PostRedo(bool bSuccess) OVERRIDE { PostUndo(bSuccess); }
	// End of FEditorUndoClient

public:
	/**
	 * Binds our UI commands to delegates
	 */ 
	void BindCommands();

	void OnCameraHome();
	void OnCleanUnusedExpressions();
	void OnShowConnectors();
	bool IsOnShowConnectorsChecked() const;
	void ToggleRealTimeExpressions();
	bool IsToggleRealTimeExpressionsChecked() const;
	void OnAlwaysRefreshAllPreviews();
	bool IsOnAlwaysRefreshAllPreviews() const;
	void ToggleStats();
	bool IsToggleStatsChecked() const;
	void ToggleMobileStats();
	bool IsToggleMobileStatsChecked() const;
	void OnNewComment();
	void OnPasteHere();
	void OnUseCurrentTexture();
	void OnConvertObjects();
	void OnConvertTextures();
	void OnPreviewNode();
	void OnToggleRealtimePreview();
	void OnBreakAllLinks();
	void OnDuplicateObjects();
	void OnDeleteObjects();
	void OnSelectDownsteamNodes();
	void OnSelectUpsteamNodes();
	void OnRemoveFromFavorites();
	void OnAddToFavorites();
	void OnBreakLink();
	
	void OnConnectToFunctionOutput(int32 MaterialInputIndex);
};
