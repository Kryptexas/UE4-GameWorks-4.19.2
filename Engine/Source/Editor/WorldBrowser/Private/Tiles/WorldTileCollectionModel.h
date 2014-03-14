// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "WorldTileModel.h"

/** The non-UI solution specific presentation logic for a world composition */
class FWorldTileCollectionModel
	: public FLevelCollectionModel 
	, public FEditorUndoClient
{

public:
	enum EWorldDirections
	{
		XNegative,
		YNegative,
		XPositive,
		YPositive
	};
	
	/** FWorldTileCollectionModel destructor */
	virtual ~FWorldTileCollectionModel();

	/**  
	 *	Factory method which creates a new FWorldTileCollectionModel object
	 *
	 *	@param	InEditor		The UEditorEngine to use
	 */
	static TSharedRef<FWorldTileCollectionModel> Create(const TWeakObjectPtr<UEditorEngine>& InEditor)
	{
		TSharedRef<FWorldTileCollectionModel> LevelCollectionModel(new FWorldTileCollectionModel(InEditor));
		LevelCollectionModel->Initialize();
		return LevelCollectionModel;
	}
public:
	/** FLevelCollection interface */
	virtual void UnloadLevels(const FLevelModelList& InLevelList) OVERRIDE;
	virtual void TranslateLevels(const FLevelModelList& InList, FVector2D InAbsoluteDelta, bool bSnapDelta = true)  OVERRIDE;
	virtual FVector2D SnapTranslationDelta(const FLevelModelList& InList, FVector2D InAbsoluteDelta, float SnappingDistance) OVERRIDE;
	virtual TSharedPtr<FLevelDragDropOp> CreateDragDropOp() const OVERRIDE;
	virtual bool PassesAllFilters(TSharedPtr<FLevelModel> InLevelModel) const OVERRIDE;
	virtual void BuildGridMenu(FMenuBuilder& InMenuBuilder) const OVERRIDE;
	virtual void BuildHierarchyMenu(FMenuBuilder& InMenuBuilder) const OVERRIDE;
	virtual void CustomizeFileMainMenu(FMenuBuilder& InMenuBuilder) const OVERRIDE;
	virtual FVector GetObserverPosition() const OVERRIDE;
	virtual bool CompareLevelsZOrder(TSharedPtr<FLevelModel> InA, TSharedPtr<FLevelModel> InB) const OVERRIDE;
	virtual void RegisterDetailsCustomization(class FPropertyEditorModule& PropertyModule, TSharedPtr<class IDetailsView> InDetailsView)  OVERRIDE;
	virtual void UnregisterDetailsCustomization(class FPropertyEditorModule& PropertyModule, TSharedPtr<class IDetailsView> InDetailsView) OVERRIDE;
	/** FLevelCollection interface end */

private:
	/** FTickableEditorObject interface */
	void Tick( float DeltaTime ) OVERRIDE;
	/** FTickableEditorObject interface end */

	/** FLevelCollection interface */
	virtual void Initialize() OVERRIDE;
	virtual void BindCommands() OVERRIDE;
	virtual void OnLevelsCollectionChanged() OVERRIDE;
	virtual void OnLevelsSelectionChanged() OVERRIDE;
	virtual void OnLevelsHierarchyChanged() OVERRIDE;
	virtual void OnPreLoadLevels(const FLevelModelList& InList) OVERRIDE;
	virtual void OnPreShowLevels(const FLevelModelList& InList) OVERRIDE;
	/** FLevelCollection interface end */
	
public:
	/** @return	Whether world browser has world root opened */
	bool HasWorldRoot() const;

	/** Returns TileModel which is used as root for all tiles */
	TSharedPtr<FWorldTileModel> GetWorldRootModel();

	/** Removes selection from levels which belongs to provided Layer */
	void DeselectLevels(const FWorldTileLayer& InLayer);

	/** @return	whether at least one layer is selected */
	bool AreAnyLayersSelected() const;

	/** Hide a levels from the editor and move them to original position 
	 *	Similar to unloading level, but does not removes it from the memory
	 */
	void ShelveLevels(const FWorldTileModelList& InLevels);

	/** Show a levels in the editor and place them to actual world position */
	void UnshelveLevels(const FWorldTileModelList& InLevels);

	/** Toggles levels always loaded flag*/
	void ToggleAlwaysLoaded(const FLevelModelList& InLevels);

	/** Whether specified list of levels has at least one landscape level */
	bool HasLandscapeLevel(const FWorldTileModelList& InLevels) const;
	
	/** Creates a new empty level 
	 *	@return LevelModel of a new empty level
	 */
	TSharedPtr<FLevelModel> CreateNewEmptyLevel();

	/** Returns all layers found in the world */
	TArray<FWorldTileLayer>& GetLayers();

	/** Adds unique runtime layer to the world */
	void AddLayer(const FWorldTileLayer& InLayer);
	
	/** Adds unique managed layer to the world */
	void AddManagedLayer(const FWorldTileLayer& InLayer);

	/** Sets provided layer as selected */
	void SetSelectedLayer(const FWorldTileLayer& InLayer); 
	
	/** Sets provided layers list as selected */
	void SetSelectedLayers(const TArray<FWorldTileLayer>& InLayers); 
	
	/** Toggles provided layer selection */
	void ToggleLayerSelection(const FWorldTileLayer& InLayer);
	
	/** Return whether provides layer is selected or not */
	bool IsLayerSelected(const FWorldTileLayer& InLayer);

	/* Notification that level object was loaded from disk */
	void OnLevelLoadedFromDisk(TSharedPtr<FWorldTileModel> InLevel);

	/**	Notification that "view point" for streaming levels visibility preview was changed */
	void UpdateStreamingPreview(FVector2D InPreviewLocation, bool bEnabled);

	/** Returns list of visible streaming levels for curent preview location */
	TArray<FName>& GetPreviewStreamingLevels();
		
	/** Calculates snapped moving delta based on specified landscape tile */
	FVector2D SnapTranslationDeltaLandscape(const TSharedPtr<FWorldTileModel>& LandscapeTile, 
											FVector2D InAbsoluteDelta, 
											float SnappingDistance);
	
	// Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) OVERRIDE;
	virtual void PostRedo(bool bSuccess) OVERRIDE { PostUndo(bSuccess); }
	// End of FEditorUndoClient

	enum FocusStrategy 
	{
		OriginAtCenter,			// Unconditionally move current world origin to specified area center
		EnsureEditable,			// May move world origin such that specified area become editable
		EnsureEditableCentered  // May move world origin such that specified area become editable and centered in world
	};
	
	/** 
	 *  Tell the browser that user is focusing on this area in world  
	 *  This may cause world origin shifting and subsequent shelving/unshelving operations
	 */
	void Focus(FBox InArea, FocusStrategy InStaragegy);

private:
	FWorldTileCollectionModel( const TWeakObjectPtr< UEditorEngine >& InEditor );
	
	/** Setups parent->child links between tiles */
	void SetupParentChildLinks();

	/** Called before saving world into package file */
	void OnPreSaveWorld(uint32 SaveFlags, class UWorld* World);
	
	/** Called right after world was saved into package file */
	void OnPostSaveWorld(uint32 SaveFlags, class UWorld* World, bool bSuccess);

	/** Called when world has new current level */
	void OnNewCurrentLevel();

	/** @return shared pointer to added level */
	TSharedPtr<FWorldTileModel> AddLevelFromTile(int32 TileIdx);

	/** @return shared pointer to FStreamingLevelModel associated with provided ULevel object*/
	TSharedPtr<FWorldTileModel> GetCorrespondingModel(ULevel* InLevel);

	/** Moves world origin closer to levels which are going to be loaded 
	 *  and unloads levels which are far enough from new world origin
	 */
	void PrepareToLoadLevels(FWorldTileModelList& InLevels);
		
	/** Delegate callback: the world origin is going to be moved. */
	void PreWorldOriginOffset(UWorld* InWorld, const FIntPoint& InSrcOrigin, const FIntPoint& InDstOrigin);
	
	/** Delegate callback: the world origin has been moved. */
	void PostWorldOriginOffset(UWorld* InWorld, const FIntPoint& InSrcOrigin, const FIntPoint& InDstOrigin);

	/** Update list layers */
	void PopulateLayersList();

	/** Scrolls world origin to specified position */
	void MoveWorldOrigin(const FIntPoint& InOrigin);

	/** Adds a loaded level to the world and makes it visible if possible 
	 *	@return Whether level was added to the world
	 */
	bool AddLevelToTheWorld(const TSharedPtr<FWorldTileModel>& InLevel);
	
	/** Builds Layers menu */
	void BuildLayersMenu(FMenuBuilder& InMenuBuilder) const;
	
	/** Builds adjacent landscape menu */
	void BuildAdjacentLandscapeMenu(FMenuBuilder& InMenuBuilder) const;

private:
	/** Creates a new empty Level; prompts for level save location */
	void CreateEmptyLevel_Executed();

	/** Moves world origin to selected level position */
	void MoveWorldOrigin_Executed();
	
	/** Reset world origin offset */
	void ResetWorldOrigin_Executed();

	/** Reset world origin offset */
	void ResetLevelOrigin_Executed();

	/** Toggles always loaded flag for selected levels */
	void ToggleAlwaysLoaded_Executed();
		
	/** Clear parent links fro selected levels */
	void ClearParentLink_Executed();

	/** 
	 * Creates a new Level with landscape proxy in it
	 * @param InWhere  Defines on which side of currently selected landscape level
	 */
	void AddLandscapeProxy_Executed(EWorldDirections InWhere);
	
	/** @return whether it is possible to add a level with landscape proxy at specified location */	
	bool CanAddLandscapeProxy(EWorldDirections InWhere) const;

public:
	/** Whether Editor has support for generating LOD levels */	
	bool HasGenerateLODLevelSupport() const;
	
	/** 
	 * Generates LOD level from a specified level. Level has to be loaded.
	 * Currently all static meshes will be merged into one proxy mesh using Simplygon ProxyLOD
	 * Landscape actors will be converted into static meshes and simplified using mesh reduction interface
	 */	
	bool GenerateLODLevel(TSharedPtr<FLevelModel> InLevelModel, int32 TargetLODIndex);

	/** Assign selected levels to current layer */
	void AssignSelectedLevelsToLayer_Executed(FWorldTileLayer InLayer);

private:
	/** List of tiles currently not affected by user selection set */
	FLevelModelList						StaticTileList;	

	/** Streamed in tiles which have to be added to the world*/
	TArray<TWeakPtr<FWorldTileModel>>	PendingVisibilityTiles;

	/** Cached streaming tiles which are potentially visible from specified view point*/
	TArray<FName>						PreviewVisibleTiles;
	
	/** View point location for calculating potentially visible streaming tiles*/
	FVector								PreviewLocation;

	/** All layers */
	TArray<FWorldTileLayer>				AllLayers;

	/** All layers currently created by the user*/
	TArray<FWorldTileLayer>				ManagedLayers;

	/** All selected layers */
	TArray<FWorldTileLayer>				SelectedLayers;

	// Is in process of saving a level
	bool								bIsSavingLevel;
	
	// Whether Editor has support for mesh proxy
	bool								bMeshProxyAvailable;
};
