// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"

/**
 * StaticMesh Editor class
 */
class FStaticMeshEditor : public IStaticMeshEditor, public FGCObject, public FEditorUndoClient, public FNotifyHook
{
public:
	FStaticMeshEditor()
		: StaticMesh( NULL )
	{}

	~FStaticMeshEditor();

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	/**
	 * Edits the specified static mesh object
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit			The static mesh to edit
	 */
	void InitStaticMeshEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UStaticMesh* ObjectToEdit );

	/** Creates details for a static mesh */
	TSharedRef<class IDetailCustomization> MakeStaticMeshDetails();

	// FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;
	// End of FGCObject interface

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;

	/** @return the documentation location for this editor */
	virtual FString GetDocumentationLink() const OVERRIDE
	{
		return FString(TEXT("Engine/Content/Types/StaticMeshes/Editor"));
	}

	/** @return Returns the color and opacity to use for the color that appears behind the tab text for this toolkit's tab in world-centric mode. */
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	/** IStaticMeshEditor interface */
	virtual UStaticMesh* GetStaticMesh() { return StaticMesh; }
	virtual UStaticMeshComponent* GetStaticMeshComponent() const OVERRIDE;

	virtual UStaticMeshSocket* GetSelectedSocket() const OVERRIDE ;
	virtual void SetSelectedSocket(UStaticMeshSocket* InSelectedSocket) OVERRIDE;
	virtual void DuplicateSelectedSocket() OVERRIDE;
	virtual void RequestRenameSelectedSocket() OVERRIDE;

	virtual int32 GetNumTriangles( int32 LODLevel = 0 ) const OVERRIDE;
	virtual int32 GetNumVertices( int32 LODLevel = 0 ) const OVERRIDE;
	virtual int32 GetNumUVChannels( int32 LODLevel = 0 ) const OVERRIDE;

	virtual int32 GetCurrentUVChannel() OVERRIDE;
	virtual int32 GetCurrentLODLevel() OVERRIDE;
	virtual int32 GetCurrentLODIndex() OVERRIDE;

	virtual void RefreshTool() OVERRIDE;
	virtual void RefreshViewport() OVERRIDE;

	/** This is called when Apply is pressed in the dialog. Does the actual processing. */
	virtual void DoDecomp(int32 InMaxHullCount, int32 InMaxHullVerts) OVERRIDE;

	virtual TSet< int32 >& GetSelectedEdges() OVERRIDE;
	// End of IStaticMeshEditor
	
	/** Extends the toolbar menu to include static mesh editor options */
	void ExtendMenu();

	/** Registers a delegate to be called after an Undo operation */
	virtual void RegisterOnPostUndo( const FOnPostUndo& Delegate ) OVERRIDE;

	/** Unregisters a delegate to be called after an Undo operation */
	virtual void UnregisterOnPostUndo( SWidget* Widget ) OVERRIDE;
	
	/** From FNotifyHook */
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged ) OVERRIDE;
	
	/** Get the names of the LOD for menus */
	TArray< TSharedPtr< FString > >& GetLODLevels() { return LODLevels; }
	const TArray< TSharedPtr< FString > >& GetLODLevels() const { return LODLevels; }

private:
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_SocketManager(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Collision(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_GenerateUniqueUVs(const FSpawnTabArgs& Args);

private:
	/** Binds commands associated with the Static Mesh Editor. */
	void BindCommands();
	
	/** Builds the Static Mesh Editor toolbar. */
	void ExtendToolBar();

	/** Builds the sub-tools that are a part of the static mesh editor. */
	void BuildSubTools();

	/** 
	* Updates NumTriangles, NumVertices and NumUVChannels for the given LOD 
	*/
	void UpdateLODStats(int32 CurrentLOD);
	
	/** A general callback for the combo boxes in the Static Mesh Editor to force a viewport refresh when a selection changes. */
	void ComboBoxSelectionChanged( TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo );

	/** A callback for when the LOD is selected, forces an update to retrieve UV channels, triangles, vertices among other things. Refreshes the viewport. */
	void LODLevelsSelectionChanged( TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo );

	/**
	 *	Sets the editor's current mesh and refreshes various settings to correspond with the new data.
	 *
	 *	@param	InStaticMesh		The static mesh to use for the editor.
	 */
	void SetEditorMesh(UStaticMesh* InStaticMesh);

	/** Helper function for generating K-DOP collision geometry. */
	void GenerateKDop(const FVector* Directions, uint32 NumDirections);

	/** Callback for creating sphere collision. */
	void OnCollisionSphere();

	/** Event for exporting the light map channel of a static mesh to an intermediate file for Max/Maya */
	void OnExportLightmapMesh( bool IsFBX );

	/** Event for importing the light map channel to a static mesh from an intermediate file from Max/Maya */
	void OnImportLightmapMesh( bool IsFBX );

	/*
	* Quick and dirty way of creating box vertices from a box collision representation
	* Grossly inefficient, but not time critical
	* @param BoxElem - Box collision to get the vertices for
	* @param Verts - output listing of vertex data from the box collision
	* @param Scale - scale to create this box at
	*/
	void CreateBoxVertsFromBoxCollision(const struct FKBoxElem& BoxElem, TArray<FVector>& Verts, float Scale);

	/** Converts the collision data for the static mesh */
	void OnConvertBoxToConvexCollision(void);

	/** Copy the collision data from the selected static mesh in Content Browser*/
	void OnCopyCollisionFromSelectedStaticMesh(void);

	/** Clears the collision data for the static mesh */
	void OnRemoveCollision(void);

	/** Change the mesh the editor is viewing. */
	void OnChangeMesh();

	/** Rebuilds the LOD combo list and sets it to "auto", a safe LOD level. */
	void RegenerateLODComboList();

	/** Rebuilds the UV Channel combo list and attempts to set it to the same channel. */
	void RegenerateUVChannelComboList();

	/** Delete the currently selected sockets */
	void DeleteSelectedSockets();

	/** Whether we currently have selected sockets */
	bool HasSelectedSockets() const;

	/** Handler for when FindInExplorer is selected */
	void ExecuteFindInExplorer();

	/** Returns true to allow execution of source file commands */
	bool CanExecuteSourceCommands() const;

	/** Callback when an object is reimported, handles steps needed to keep the editor up-to-date. */
	void OnObjectReimported(UObject* InObject);

	/** Opens the convex decomposition tab. */
	void OnConvexDecomposition();

	// Begin FAssetEditorToolkit interface.
	virtual bool OnRequestClose() OVERRIDE;
	// End FAssetEditorToolkit interface.

	// Begin FEditorUndoClient Interface
	virtual void PostUndo( bool bSuccess ) OVERRIDE;
	virtual void PostRedo( bool bSuccess ) OVERRIDE;
	// End of FEditorUndoClient

	/** Undo Action**/
	void UndoAction();

	/** Redo Action **/
	void RedoAction();

	/** Called when socket selection changes */
	void OnSocketSelectionChanged();

	/** Callback when an object has been reimported, and whether it worked */
	void OnPostReimport(UObject* InObject, bool bSuccess);

private:
	/** List of open tool panels; used to ensure only one exists at any one time */
	TMap< FName, TWeakPtr<class SDockableTab> > SpawnedToolPanels;

	/** Preview Viewport widget */
	TSharedPtr<class SStaticMeshEditorViewport> Viewport;

	/** Property View */
	TSharedPtr<class IDetailsView> StaticMeshDetailsView;

	/** Socket Manager widget. */
	TSharedPtr< class ISocketManager> SocketManager;

	/** Convex Decomposition widget */
	TSharedPtr< class SConvexDecomposition> ConvexDecomposition;

	/** Generate Unique UVs widget. */
	TSharedPtr< class SGenerateUniqueUVs> GenerateUniqueUVs;

	/** Widget for displaying the available UV Channels. */
	TSharedPtr< class STextComboBox > UVChannelCombo;

	/** List of available UV Channels. */
	TArray< TSharedPtr< FString > > UVChannels;

	/** Widget for displaying the available LOD. */
	TSharedPtr< class STextComboBox > LODLevelCombo;

	/** Static mesh editor detail customization */
	TWeakPtr<class FStaticMeshDetails> StaticMeshDetails;

	/** Named list of LODs for use in menus */
	TArray< TSharedPtr< FString > > LODLevels;

	/** The currently viewed Static Mesh. */
	UStaticMesh* StaticMesh;

	/** The number of triangles associated with the static mesh LOD. */
	TArray<int32> NumTriangles;

	/** The number of vertices associated with the static mesh LOD. */
	TArray<int32> NumVertices;

	/** The number of used UV channels. */
	TArray<int32> NumUVChannels;

	/** The number of LOD levels. */
	int32 NumLODLevels;

	/** Delegates called after an undo operation for child widgets to refresh */
	FOnPostUndoMulticaster OnPostUndo;	

	/**	The tab ids for all the tabs used */
	static const FName ViewportTabId;
	static const FName PropertiesTabId;
	static const FName SocketManagerTabId;
	static const FName CollisionTabId;
	static const FName GenerateUniqueUVsTabId;
};