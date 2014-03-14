// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "DragAndDrop.h"
#include "Editor/GraphEditor/Public/GraphEditorDragDropAction.h"

/**
 * Class for rendering previews of material expressions in the material editor's linked object viewport.
 */
class FMatExpressionPreview : public FMaterial, public FMaterialRenderProxy
{
public:
	FMatExpressionPreview()
	: FMaterial()
	{
		// Register this FMaterial derivative with AddEditorLoadedMaterialResource since it does not have a corresponding UMaterialInterface
		FMaterial::AddEditorLoadedMaterialResource(this);
		SetQualityLevelProperties(EMaterialQualityLevel::High,false,GRHIFeatureLevel);
	}

	FMatExpressionPreview(UMaterialExpression* InExpression)
	: FMaterial()
	, Expression(InExpression)
	{
		FMaterial::AddEditorLoadedMaterialResource(this);
		FPlatformMisc::CreateGuid(Id);

		check(InExpression->Material && InExpression->Material->Expressions.Contains(InExpression));
		InExpression->Material->AppendReferencedTextures(ReferencedTextures);
		SetQualityLevelProperties(EMaterialQualityLevel::High,false,GRHIFeatureLevel);
	}

	~FMatExpressionPreview()
	{
		ReleaseResource();
	}

	void AddReferencedObjects( FReferenceCollector& Collector )
	{
		for (int32 TextureIndex = 0; TextureIndex < ReferencedTextures.Num(); TextureIndex++)
		{
			Collector.AddReferencedObject(ReferencedTextures[TextureIndex]);
		}
	}

	/**
	 * Should the shader for this material with the given platform, shader type and vertex 
	 * factory type combination be compiled
	 *
	 * @param Platform		The platform currently being compiled for
	 * @param ShaderType	Which shader is being compiled
	 * @param VertexFactory	Which vertex factory is being compiled (can be NULL)
	 *
	 * @return true if the shader should be compiled
	 */
	virtual bool ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const;

	virtual const TArray<UTexture*>& GetReferencedTextures() const OVERRIDE
	{
		return ReferencedTextures;
	}

	////////////////
	// FMaterialRenderProxy interface.

	virtual const FMaterial* GetMaterial(ERHIFeatureLevel::Type FeatureLevel) const
	{
		if(GetRenderingThreadShaderMap())
		{
			return this;
		}
		else
		{
			return UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false)->GetMaterial(FeatureLevel);
		}
	}

	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		if (Expression.IsValid() && Expression->Material)
		{
			return Expression->Material->GetRenderProxy(0)->GetVectorValue(ParameterName, OutValue, Context);
		}
		return false;
	}

	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const
	{
		if (Expression.IsValid() && Expression->Material)
		{
			return Expression->Material->GetRenderProxy(0)->GetScalarValue(ParameterName, OutValue, Context);
		}
		return false;
	}

	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const
	{
		if (Expression.IsValid() && Expression->Material)
		{
			return Expression->Material->GetRenderProxy(0)->GetTextureValue(ParameterName, OutValue, Context);
		}
		return false;
	}
	
	// Material properties.
	/** Entry point for compiling a specific material property.  This must call SetMaterialProperty. */
	virtual int32 CompileProperty(EMaterialProperty Property,EShaderFrequency InShaderFrequency,FMaterialCompiler* Compiler) const;

	virtual int32 GetMaterialDomain() const OVERRIDE { return MD_Surface; }
	virtual FString GetMaterialUsageDescription() const { return FString::Printf(TEXT("FMatExpressionPreview %s"), Expression.IsValid() ? *Expression->GetName() : TEXT("NULL")); }
	virtual bool IsTwoSided() const { return false; }
	virtual bool IsLightFunction() const { return false; }
	virtual bool IsUsedWithDeferredDecal() const { return false; }
	virtual bool IsSpecialEngineMaterial() const { return false; }
	virtual bool IsWireframe() const { return false; }
	virtual bool IsMasked() const { return false; }
	virtual enum EBlendMode GetBlendMode() const { return BLEND_Opaque; }
	virtual enum EMaterialLightingModel GetLightingModel() const { return MLM_Unlit; }
	virtual float GetOpacityMaskClipValue() const { return 0.5f; }
	virtual FString GetFriendlyName() const { return FString::Printf(TEXT("FMatExpressionPreview %s"), Expression.IsValid() ? *Expression->GetName() : TEXT("NULL")); }
	/**
	 * Should shaders compiled for this material be saved to disk?
	 */
	virtual bool IsPersistent() const { return false; }
	virtual FGuid GetMaterialId() const OVERRIDE { return Id; }
	const UMaterialExpression* GetExpression() const
	{
		return Expression.Get();
	}

	virtual void NotifyCompilationFinished() OVERRIDE;

	friend FArchive& operator<< ( FArchive& Ar, FMatExpressionPreview& V )
	{
		return Ar << V.Expression;
	}

private:
	TWeakObjectPtr<UMaterialExpression> Expression;
	TArray<UTexture*> ReferencedTextures;
	FGuid Id;
};

/** Wrapper for each material expression, including a trimmed name */
struct FMaterialExpression
{
	FString Name;
	UClass* MaterialClass;

	friend bool operator==(const FMaterialExpression& A,const FMaterialExpression& B)
	{
		return A.MaterialClass == B.MaterialClass;
	}
};

/** Static array of categorized material expression classes. */
struct FCategorizedMaterialExpressionNode
{
	FString	CategoryName;
	TArray<FMaterialExpression> MaterialExpressions;
};

/** Used in the material function library to store function information. */
struct FFunctionEntryInfo
{
	FString Name;
	FString Path;
	FString ToolTip;

	FFunctionEntryInfo(){}
	FFunctionEntryInfo(const FString& InPath, const FString& InToolTip)
	{
		Path = InPath;
		ToolTip = InToolTip;

		// Extract the object name from the path
		FString FunctionName = Path;
		int32 PeriodIndex = Path.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);

		if (PeriodIndex != INDEX_NONE)
		{
			FunctionName = Path.Right(Path.Len() - PeriodIndex - 1);
		}
		Name = FunctionName;
	}

	friend bool operator==(const FFunctionEntryInfo& A,const FFunctionEntryInfo& B)
	{
		return A.Path == B.Path;
	}
};

/** Used in the material function library to store category information. */
struct FCategorizedMaterialFunctions
{
	FString	CategoryName;
	TArray<FFunctionEntryInfo> FunctionInfos;

	FCategorizedMaterialFunctions(){}
	FCategorizedMaterialFunctions(const FString& InCategoryName)
	{
		CategoryName = InCategoryName;
	}
};

/** Used to display material information, compile errors etc. */
struct FMaterialInfo
{
	FString Text;
	FLinearColor Color;

	FMaterialInfo(const FString& InText, const FLinearColor& InColor)
		: Text(InText)
		, Color(InColor)
	{}
};


typedef TSharedPtr<class FMaterialItem> FMaterialItemPtr;
typedef STreeView<FMaterialItemPtr> SMaterialTreeView;



/**
 * Material Editor class
 */
class FMaterialEditor : public IMaterialEditor, public FGCObject, public FTickableGameObject, public FEditorUndoClient, public FNotifyHook
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

public:
	/** Initializes the editor to use a material. Should be the first thing called. */
	void InitEditorForMaterial(UMaterial* InMaterial);

	/** Initializes the editor to use a material function. Should be the first thing called. */
	void InitEditorForMaterialFunction(UMaterialFunction* InMaterialFunction);

	/**
	 * Edits the specified material object
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit			The material or material function to edit
	 */
	void InitMaterialEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit );

	/** Constructor */
	FMaterialEditor();

	virtual ~FMaterialEditor();
	
	/** FGCObject interface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;

	/** @return the documentation location for this editor */
	virtual FString GetDocumentationLink() const OVERRIDE
	{
		return FString(TEXT("Engine/Rendering/Materials"));
	}

	/** @return Returns the color and opacity to use for the color that appears behind the tab text for this toolkit's tab in world-centric mode. */
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	/** The material instance applied to the preview mesh. */
	virtual UMaterialInterface* GetMaterialInterface() const OVERRIDE;
	
	/** Called by our list to generate a widget that represents the specified item at the specified column in the tree */
	TSharedRef< SWidget > GenerateWidgetForItemAndColumn( FMaterialItemPtr Item, const FName ColumnID ) const;
	
	/**
	 * Draws material info strings such as instruction count and current errors onto the canvas.
	 */
	static void DrawMaterialInfoStrings(
		FCanvas* Canvas,
		const UMaterial* Material,
		const FMaterialResource* MaterialResource,
		const TArray<FString>& CompileErrors,
		int32 &DrawPositionY,
		bool bDrawInstructions);
	
	/**
	 * Draws messages on the specified viewport and canvas.
	 */
	virtual void DrawMessages( FViewport* Viewport, FCanvas* Canvas );

	/**
	 * Recenter the editor to either the material inputs or the first material function output
	 */
	void RecenterEditor();

	/** Passes instructions to the preview viewport */
	bool SetPreviewMesh(UStaticMesh* InStaticMesh, USkeletalMesh* InSkeletalMesh);
	bool SetPreviewMesh(const TCHAR* InMeshName);
	void SetPreviewMaterial(UMaterialInterface* InMaterialInterface);
	
	/**
	 * Refreshes the viewport containing the preview mesh.
	 */
	void RefreshPreviewViewport();
	
	/** Regenerates the code view widget with new text */
	void RegenerateCodeView();
	
	/**
	 * Recompiles the material used in the preview window.
	 */
	void UpdatePreviewMaterial();

	/**
	 * Updates the original material with the changes made in the editor
	 */
	void UpdateOriginalMaterial();

	/**
	 * Updates list of Material Info used to show stats
	 */
	void UpdateMaterialInfoList();

	/**
	 * Updates flags on the Material Nodes to avoid expensive look up calls when rendering
	 */
	void UpdateGraphNodeStates();
	
	// Widget Accessors
	TSharedRef<class IDetailsView> GetDetailView() const {return MaterialDetailsView.ToSharedRef();}
	
	// FTickableGameObject interface
	virtual void Tick(float DeltaTime) OVERRIDE;

	virtual bool IsTickable() const OVERRIDE
	{
		return true;
	}

	virtual bool IsTickableWhenPaused() const OVERRIDE
	{
		return true;
	}
	virtual TStatId GetStatId() const OVERRIDE;

	/** Pushes the PreviewMesh assigned the the material instance to the thumbnail info */
	static void UpdateThumbnailInfoPreviewMesh(UMaterialInterface* MatInterface);

	/** Checks whether we are using the new Graph Editor */
	bool UsingNewGraphEditor() const
	{
		return bUseNewGraphEditor;
	}

	/** Callback for when the canvas search next and previous are used */
	void OnSearch(SSearchBox::SearchDirection Direction);

	/** Sets the expression to be previewed. */
	void SetPreviewExpression(UMaterialExpression* NewPreviewExpression);

	// IMaterial Editor Interface
	virtual UMaterialExpression* CreateNewMaterialExpression(UClass* NewExpressionClass, const FVector2D& NodePos, bool bAutoSelect, bool bAutoAssignResource) OVERRIDE;
	virtual UMaterialExpressionComment* CreateNewMaterialExpressionComment(const FVector2D& NodePos) OVERRIDE;
	virtual void ForceRefreshExpressionPreviews() OVERRIDE;
	virtual void AddToSelection(UMaterialExpression* Expression) OVERRIDE;
	virtual void DeleteSelectedNodes() OVERRIDE;
	virtual FString GetOriginalObjectName() const OVERRIDE;
	virtual void UpdateMaterialAfterGraphChange() OVERRIDE;
	virtual bool CanPasteNodes() const OVERRIDE;
	virtual void PasteNodesHere(const FVector2D& Location) OVERRIDE;
	virtual int32 GetNumberOfSelectedNodes() const OVERRIDE;
	virtual FMaterialRenderProxy* GetExpressionPreview(UMaterialExpression* InExpression) OVERRIDE;
	virtual void UpdateSearch( bool bQueryChanged ) OVERRIDE;
	
public:
	/** Set to true when modifications have been made to the material */
	bool bMaterialDirty;

	/** Set to true if stats should be displayed from the preview material. */
	bool bStatsFromPreviewMaterial;

	/** The material applied to the preview mesh. */
	UMaterial* Material;
	
	/** The source material being edited by this material editor. Only will be updated when Material's settings are copied over this material */
	UMaterial* OriginalMaterial;
	
	/** The material applied to the preview mesh when previewing an expression. */
	UMaterial* ExpressionPreviewMaterial;

	/** The expression currently being previewed.  This is NULL when not in expression preview mode. */
	UMaterialExpression* PreviewExpression;

	/** 
	 * Material function being edited.  
	 * If this is non-NULL, a function is being edited and Material is being used to preview it.
	 */
	UMaterialFunction* MaterialFunction;
	
	/** The original material or material function being edited by this material editor.. */
	UObject* OriginalMaterialObject;

	/** Configuration class used to store editor settings across sessions. */
	UMaterialEditorOptions* EditorOptions;
	
protected:
	/** Called when "Save" is clicked for this asset */
	virtual void SaveAsset_Execute() OVERRIDE;
	
	/** Called when this toolkit would close */
	virtual bool OnRequestClose() OVERRIDE;

	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	/**
	 * Called when a node is double clicked
	 *
	 * @param	Node	The Node that was clicked
	 */
	void OnNodeDoubleClicked(class UEdGraphNode* Node);

	/**
	 * Called when a node's title is committed for a rename
	 *
	 * @param	NewText				New title text
	 * @param	CommitInfo			How text was committed
	 * @param	NodeBeingChanged	The node being changed
	 */
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

	/**
	 * Handles spawning a graph node in the current graph using the passed in gesture
	 *
	 * @param	InGesture	Gesture that was just performed
	 * @param	InPosition	Current cursor position
	 * @param	InGraph		Graph that gesture was performed in
	 *
	 * @return	FReply	Whether gesture was handled
	 */
	FReply OnSpawnGraphNodeByShortcut(FInputGesture InGesture, const FVector2D& InPosition, UEdGraph* InGraph);

	/** Select every node in the graph */
	void SelectAllNodes();
	/** Whether we can select every node */
	bool CanSelectAllNodes() const;

	/** Delete an array of Material Graph Nodes and their corresponding expressions/comments */
	void DeleteNodes(const TArray<class UEdGraphNode*>& NodesToDelete);
	/** Whether we are able to delete the currently selected nodes */
	bool CanDeleteNodes() const;
	/** Delete only the currently selected nodes that can be duplicated */
	void DeleteSelectedDuplicatableNodes();

	/** Copy the currently selected nodes */
	void CopySelectedNodes();
	/** Whether we are able to copy the currently selected nodes */
	bool CanCopyNodes() const;

	/** Paste the contents of the clipboard */
	void PasteNodes();

	/** Cut the currently selected nodes */
	void CutSelectedNodes();
	/** Whether we are able to cut the currently selected nodes */
	bool CanCutNodes() const;

	/** Duplicate the currently selected nodes */
	void DuplicateNodes();
	/** Whether we are able to duplicate the currently selected nodes */
	bool CanDuplicateNodes() const;

	/** Called to undo the last action */
	void UndoGraphAction();

	/** Called to redo the last undone action */
	void RedoGraphAction();

private:
	/** Builds the toolbar widget for the material editor */
	void ExtendToolbar();

	/** Allows editor to veto the setting of a preview mesh */
	virtual bool ApproveSetPreviewMesh(UStaticMesh* InStaticMesh, USkeletalMesh* InSkeletalMesh) OVERRIDE;

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();
	
	/** Collects all groups for all material expressions */
	void GetAllMaterialExpressionGroups(TArray<FString>* OutGroups);

public:

private:
	/** Populates the expressions tree view with the appropriate items, based on filters present */
	void PopulateMaterialExpressionsTree();
	/** Populates the functions tree view with the appropriate items, based on filters present */
	void PopulateMaterialFunctionsTree();

	/** Called when the graph search box changes text */
	void OnGraphSearchChanged( const FText& InFilterText );

	/** Called when the graph search text is committed */
	void OnGraphSearchCommitted(const FText& NewTypeInValue, ETextCommit::Type CommitInfo);

	/** Called by STreeView to generate a table row for the specified item */
	TSharedRef< ITableRow > OnGenerateRowForTree( FMaterialItemPtr Item, const TSharedRef< STableViewBase >& OwnerTable );

	/** Called by STreeView to get child items for the specified parent item */
	void OnGetChildrenForTree( FMaterialItemPtr Parent, TArray<FMaterialItemPtr>& OutChildren );
	
	/** Called by STreeView when the user double-clicks on an item in the tree */
	void OnTreeDoubleClick( FMaterialItemPtr TreeItem );

	/** Gets the text in the expressions search filter */
	FText GetExpressionsFilterText() const;
	/** Gets the text in the functions search filter */
	FText GetFunctionsFilterText() const;
	
	/**
	 * Called by the editable text control when the filter text is changed by the user
	 *
	 * @param	InFilterText	The new text
	 */
	void OnExpressionsFilterTextChanged( const FText& InFilterText );
	void OnFunctionsFilterTextChanged( const FText& InFilterText );
	
	/**
	 * Load editor settings from disk (docking state, window pos/size, option state, etc).
	 */
	void LoadEditorSettings();

	/**
	 * Saves editor settings to disk (docking state, window pos/size, option state, etc).
	 */
	void SaveEditorSettings();

	/** Gets the text in the code view widget */
	FString GetCodeViewText() const;

	/** Copies all the HLSL Code View code to the clipboard */
	FReply CopyCodeViewTextToClipboard();

	/**
	* Rebuilds dependant Material Instance Editors
	* @param		MatInst	Material Instance to search dependent editors and force refresh of them.
	*/
	void RebuildMaterialInstanceEditors(UMaterialInstance * MatInst);
	
	/**
	 * Binds our UI commands to delegates
	 */
	void BindCommands();
	
	/** Command for the apply button */
	void OnApply();
	bool OnApplyEnabled() const;
	/** Command for the camera home button */
	void OnCameraHome();
	/** Command for the show unused connectors button */
	void OnShowConnectors();
	bool IsOnShowConnectorsChecked() const;
	/** Command for the Toggle Real Time button */
	void ToggleRealTimeExpressions();
	bool IsToggleRealTimeExpressionsChecked() const;
	/** Command for the Refresh all previews button */
	void OnAlwaysRefreshAllPreviews();
	bool IsOnAlwaysRefreshAllPreviews() const;
	/** Command for the stats button */
	void ToggleStats();
	bool IsToggleStatsChecked() const;
	/** Mobile stats button. */
	void ToggleMobileStats();
	bool IsToggleMobileStatsChecked() const;
	/** Command for using currently selected texture */
	void OnUseCurrentTexture();
	/** Command for converting nodes to objects */
	void OnConvertObjects();
	/** Command for converting nodes to textures */
	void OnConvertTextures();
	/** Command for previewing a selected node */
	void OnPreviewNode();
	/** Command for toggling real time preview of selected node */
	void OnToggleRealtimePreview();
	/** Command to select nodes downstream of selected node */
	void OnSelectDownsteamNodes();
	/** Command to select nodes upstream of selected node */
	void OnSelectUpsteamNodes();
	/** Command to force a refresh of all previews (triggered by space bar) */
	void OnForceRefreshPreviews();
	/* Create comment node on graph */
	void OnCreateComment();

	/** Callback from the Asset Registry when an asset is renamed. */
	void RenameAssetFromRegistry(const FAssetData& InAddedAssetData, const FString& InNewName);

	/** Callback to tell the Material Editor that the Material List needs to be refreshed. */
	void FunctionMaterialListNeedsRefresh()
	{
		bFunctionListNeedsRefresh = true;
	}

	/** Callback to tell the Material Editor that a materials usage flags have been changed */
	void OnMaterialUsageFlagsChanged(class UMaterial* MaterialThatChanged, int32 FlagThatChanged);

	// FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) OVERRIDE;
	virtual void PostRedo(bool bSuccess) OVERRIDE { PostUndo(bSuccess); }

	// FNotifyHook interface
	virtual void NotifyPreChange(UProperty* PropertyAboutToChange) OVERRIDE;
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged) OVERRIDE;

	/** Flags the material as dirty */
	void SetMaterialDirty() {bMaterialDirty = true;}

	/** Toggles the collapsed flag of a Material Expression and updates preview */
	void ToggleCollapsed(UMaterialExpression* MaterialExpression);

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
	 * Returns the expression preview for the specified material expression.
	 */
	FMatExpressionPreview* GetExpressionPreview(UMaterialExpression* MaterialExpression, bool& bNewlyCreated);

	/**
	  * Moves the shows the selected result in the viewport
	  */
	void ShowSearchResult();

	/** Pointer to the object that the current color picker is working on. Can be NULL and stale. */
	TWeakObjectPtr<UObject> ColorPickerObject;

	/** Called before the color picker commits a change. */
	void PreColorPickerCommit(FLinearColor LinearColor);

	/** Called whenever the color picker is used and accepted. */
	void OnColorPickerCommitted(FLinearColor LinearColor);

	/** Create new graph editor widget */
	TSharedRef<class SGraphEditor> CreateGraphEditorWidget();

	/**
	 * Deletes any disconnected material expressions.
	 */
	void CleanUnusedExpressions();

	/**
	 * Displays a warning message to the user if the expressions to remove would cause any issues
	 *
	 * @param NodesToRemove The expression nodes we wish to remove
	 *
	 * @return Whether the user agrees to remove these expressions
	 */
	bool CheckExpressionRemovalWarnings(const TArray<UEdGraphNode*>& NodesToRemove);

	/** Removes the selected expression from the favorites list. */
	void RemoveSelectedExpressionFromFavorites();
	/** Adds the selected expression to the favorites list. */
	void AddSelectedExpressionToFavorites();

	/**
	 * Flip the X coordinates of a material's expressions when loading and saving -
	 * Remove this once new material editor is active and materials are saved inverted.
	 *
	 * @param	Expressions	Array of material expressions
	 * @param	Comments	Array of material expression comments
	 */
	void FlipExpressionPositions(const TArray<UMaterialExpression*>& Expressions, const TArray<UMaterialExpressionComment*>& Comments, UMaterial* Material = NULL);

private:
	TSharedRef<SDockTab> SpawnTab_Preview(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_GraphCanvas(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_MaterialProperties(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_MaterialExpressions(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_MaterialFunctions(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_HLSLCode(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Palette(const FSpawnTabArgs& Args);
private:
	/** List of open tool panels; used to ensure only one exists at any one time */
	TMap< FName, TWeakPtr<class SDockableTab> > SpawnedToolPanels;

	/** Property View */
	TSharedPtr<class IDetailsView> MaterialDetailsView;

	/** Graph Canvas Widget */
	TSharedPtr<class SMaterialEditorCanvas> GraphCanvas;

	/** New Graph Editor */
	TSharedPtr<class SGraphEditor> GraphEditor;

	/** Preview Viewport widget */
	TSharedPtr<class SMaterialEditorViewport> Viewport;

	/** Widget to hold utility components for the HLSL Code View */
	 TSharedPtr<SWidget> CodeViewUtility;

	/** Widget for the HLSL Code View */
	TSharedPtr<class SScrollBox> CodeView;
	/** Cached Code for the widget */
	FString HLSLCode;

	/** Palette of Material Expressions and functions */
	TSharedPtr<class SMaterialPalette> Palette;

	/** True if the Material Editor's function list needs to refresh. */
	bool bFunctionListNeedsRefresh;

	/** True if we are using the new EdGraph based editor */
	bool bUseNewGraphEditor;

	/** Array of all expressions and functions gathered in the tree views currently (filtered) */
	TArray<FMaterialItemPtr> MaterialExpressions;
	TArray<FMaterialItemPtr> MaterialFunctions;
	
	/** Tree Views for Expressions and functions */
	TSharedPtr<SMaterialTreeView> MaterialExpressionsView;
	TSharedPtr<SMaterialTreeView> MaterialFunctionsLibraryView;

	/* Widgets containing the filtering text box */
	TSharedPtr< SSearchBox > ExpressionsFilterTextBox;
	TSharedPtr< SSearchBox > FunctionsFilterTextBox;

	/** Current search query */
	FString SearchQuery;

	/** Array of expressions matching the current search terms. */
	TArray<UMaterialExpression*> SearchResults;

	/** Index in the above array of the currently selected search result */
	int32 SelectedSearchResult;

	/** The current transaction. */
	FScopedTransaction* ScopedTransaction;

	/** If true, always refresh all expression previews.  This overrides UMaterialExpression::bRealtimePreview. */
	bool bAlwaysRefreshAllPreviews;

	/** Material expression previews. */
	TIndirectArray<class FMatExpressionPreview>		ExpressionPreviews;

	/** Information about material to show when stats are enabled */
	TArray<TSharedPtr<FMaterialInfo>> MaterialInfoList;

	/** If true, don't render connectors that are not connected to anything. */
	bool bHideUnusedConnectors;

	/** Just storing this choice for now, not sure what difference it will make to Graph Editor */
	bool bIsRealtime;

	/** If true, show material stats like number of shader instructions. */
	bool bShowStats;

	/** If true, show material stats and errors for mobile. */
	bool bShowMobileStats;

	/** Command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	/**	The tab ids for the material editor */
	static const FName PreviewTabId;		
	static const FName GraphCanvasTabId;	
	static const FName PropertiesTabId;	
	static const FName ExpressionsTabId;	
	static const FName FunctionsTabId;	
	static const FName HLSLCodeTabId;	
	static const FName PaletteTabId;
};



class FExpressionDragDropOp : public FDragDropOperation
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FExpressionDragDropOp"); return Type;}

	FMaterialExpression* MaterialExpression;
	FFunctionEntryInfo* FunctionEntryInfo;
	
	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		return SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			.Content()
			[
				SNew(STextBlock) 
				.Text(MaterialExpression ? MaterialExpression->Name : FunctionEntryInfo->Name)
			];
	}

	static TSharedRef<FExpressionDragDropOp> New(FMaterialExpression* InMaterialExpression, FFunctionEntryInfo* InFunctionEntryInfo)
	{
		TSharedRef<FExpressionDragDropOp> Operation = MakeShareable(new FExpressionDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FExpressionDragDropOp>(Operation);
		Operation->MaterialExpression = InMaterialExpression;
		Operation->FunctionEntryInfo = InFunctionEntryInfo;
		Operation->Construct();
		return Operation;
	}
};

class FGraphEditorExpressionDragDropAction : public FGraphEditorDragDropAction
{
public:
	FMaterialExpression* MaterialExpression;
	FFunctionEntryInfo* FunctionEntryInfo;

	// GetTypeId is the parent: FGraphEditorDragDropAction

	// FGraphEditorDragDropAction Interface
	virtual FReply DroppedOnPanel(const TSharedRef< class SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;

	static TSharedRef<FGraphEditorExpressionDragDropAction> New(FMaterialExpression* InMaterialExpression, FFunctionEntryInfo* InFunctionEntryInfo)
	{
		TSharedRef<FGraphEditorExpressionDragDropAction> Operation = MakeShareable(new FGraphEditorExpressionDragDropAction);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FGraphEditorExpressionDragDropAction>(Operation);
		Operation->MaterialExpression = InMaterialExpression;
		Operation->FunctionEntryInfo = InFunctionEntryInfo;
		Operation->Construct();
		return Operation;
	}
};
