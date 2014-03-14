// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "StreamingLevelModel.h"

/** The non-UI solution specific presentation logic for a LevelsView */
class FStreamingLevelCollectionModel 
	: public FLevelCollectionModel
	, public FEditorUndoClient
{

public:
	
	/** FStreamingLevelCollectionModel destructor */
	virtual ~FStreamingLevelCollectionModel();

	/**  
	 *	Factory method which creates a new FStreamingLevelCollectionModel object
	 *
	 *	@param	InEditor		The UEditorEngine to use
	 */
	static TSharedRef<FStreamingLevelCollectionModel> Create(const TWeakObjectPtr<UEditorEngine>& InEditor)
	{
		TSharedRef<FStreamingLevelCollectionModel> LevelCollectionModel(new FStreamingLevelCollectionModel(InEditor));
		LevelCollectionModel->Initialize();
		return LevelCollectionModel;
	}

	virtual bool SupportsGridView() const OVERRIDE { return false; };

public:
	/** FLevelCollection interface */
	virtual void UnloadLevels(const FLevelModelList& InLevelList) OVERRIDE;
	virtual TSharedPtr<FLevelDragDropOp> CreateDragDropOp() const OVERRIDE;
	virtual void BuildHierarchyMenu(FMenuBuilder& InMenuBuilder) const OVERRIDE;
	virtual void CustomizeFileMainMenu(FMenuBuilder& InMenuBuilder) const OVERRIDE;
	virtual void RegisterDetailsCustomization(class FPropertyEditorModule& PropertyModule, TSharedPtr<class IDetailsView> InDetailsView)  OVERRIDE;
	virtual void UnregisterDetailsCustomization(class FPropertyEditorModule& PropertyModule, TSharedPtr<class IDetailsView> InDetailsView) OVERRIDE;
	

private:
	virtual void Initialize() OVERRIDE;
	virtual void BindCommands() OVERRIDE;
	virtual void OnLevelsCollectionChanged() OVERRIDE;
	/** FLevelCollection interface end */
	
public:
	/** @return Whether the current selection can be shifted */
	bool CanShiftSelection();

	/** Moves the level selection up or down in the list; used for re-ordering */
	void ShiftSelection( bool bUp );

	/** @return Any selected ULevel objects in the LevelsView that are NULL */
	const FLevelModelList& GetInvalidSelectedLevels() const;

private:
	/**  
	 *	FStreamingLevelCollectionModel Constructor
	 *
	 *	@param	InWorldLevels	The Level management logic object
	 *	@param	InEditor		The UEditorEngine to use
	 */
	FStreamingLevelCollectionModel( const TWeakObjectPtr< UEditorEngine >& InEditor );
	
	/** Refreshes the sort index on all viewmodels that contain ULevels */
	void RefreshSortIndexes();
	
	/**	Sorts the filtered Levels list */
	void SortFilteredLevels();

	// Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) OVERRIDE { UpdateAllLevels(); }
	virtual void PostRedo(bool bSuccess) OVERRIDE { PostUndo(bSuccess); }
	// End of FEditorUndoClient

private:
	/** Adds an existing level; prompts for path */
	void FixupInvalidReference_Executed();

	/** Removes selected levels from world */
	void RemoveInvalidSelectedLevels_Executed();
	
	/** Creates a new empty Level; prompts for level save location */
	void CreateEmptyLevel_Executed();

	/** Calls AddExistingLevel which adds an existing level; prompts for path */
	void AddExistingLevel_Executed();
	
	/** Adds an existing level; prompts for path, Returns true if a level is selected */
	bool AddExistingLevel();

	/** Add Selected Actors to New Level; prompts for level save location */
	void AddSelectedActorsToNewLevel_Executed();

	/** Merges selected levels into a new level; prompts for level save location */
	void MergeSelectedLevels_Executed();

	/** Changes the streaming method for new or added levels. */
	void SetAddedLevelStreamingClass_Executed(UClass* InClass);

	/** Checks if the passed in streaming method is checked */
	bool IsNewStreamingMethodChecked(UClass* InClass) const;

	/** Checks if the passed in streaming method is the current streaming method */
	bool IsStreamingMethodChecked(UClass* InClass) const;

	/** Changes the streaming method for the selected levels. */
	void SetStreamingLevelsClass_Executed(UClass* InClass);

	/** Selects the streaming volumes associated with the selected levels */
	void SelectStreamingVolumes_Executed();

	/**  */
	void FillSetStreamingMethodMenu(class FMenuBuilder& MenuBuilder);
	
	/**  */
	void FillDefaultStreamingMethodMenu(class FMenuBuilder& MenuBuilder);
	
private:
	/** Currently selected NULL Levels */
	FLevelModelList		InvalidSelectedLevels;

	/** The current class to set new or added levels streaming method to. */
	UClass*				AddedLevelStreamingClass;
};


