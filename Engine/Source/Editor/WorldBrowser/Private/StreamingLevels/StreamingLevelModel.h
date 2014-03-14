// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

namespace ELevelsAction
{
	enum Type
	{
		/**	The specified ChangedLevel is a newly created ULevel, if ChangedLevel is invalid then multiple Levels were added */	
		Add,

		/**	
		 *	The specified ChangedLevel was just modified, if ChangedLevel is invalid then multiple Levels were modified. 
		 *  ChangedProperty specifies what field on the ULevel was changed, if NAME_None then multiple fields were changed 
		 */
		Modify,

		/**	A ULevel was deleted */
		Delete,

		/**	The specified ChangedLevel was just renamed */
		Rename,

		/**	A large amount of changes have occurred to a number of Levels. A full rebind will be required. */
		Reset,
	};
}

class FStreamingLevelCollectionModel;

/**
 * The non-UI solution specific presentation logic for a single streaming level
 */
class FStreamingLevelModel 
	: public FLevelModel
	, public FEditorUndoClient	
{

public:
	/**
	 *	FStreamingLevelModel Constructor
	 *
	 *	@param	InEditor			The UEditorEngine to use
	 *	@param	InWorldData			Level collection owning this model
	 *	@param	InLevelStreaming	Streaming object this model should represent
	 */
	FStreamingLevelModel(const TWeakObjectPtr<UEditorEngine>& InEditor, 
							FStreamingLevelCollectionModel& InWorldData, 
							class ULevelStreaming* InLevelStreaming);
	~FStreamingLevelModel();


public:
	// FLevelModel interface
	virtual bool HasValidPackage() const OVERRIDE;
	virtual UObject* GetNodeObject() OVERRIDE;
	virtual ULevel* GetLevelObject() const OVERRIDE;
	virtual FName GetAssetName() const OVERRIDE;
	virtual FName GetLongPackageName() const OVERRIDE;
	virtual void Update() OVERRIDE;
	virtual void OnDrop(const TSharedPtr<FLevelDragDropOp>& Op) OVERRIDE;
	virtual bool IsGoodToDrop(const TSharedPtr<FLevelDragDropOp>& Op) const OVERRIDE;
	virtual void GetGridItemTooltipFields(TArray< TPair<TAttribute<FText>, TAttribute<FText>> >& CustomFields) const OVERRIDE;
	// FLevelModel interface end
		
	/** @return the StreamingLevelIndex */
	int GetStreamingLevelIndex() { return StreamingLevelIndex; }

	/** Refreshes the StreamingLevelIndex by checking or its position in its OwningWorld's Level array */
	void RefreshStreamingLevelIndex();

	/** @return The ULevelStreaming this viewmodel contains*/
	const TWeakObjectPtr< ULevelStreaming > GetLevelStreaming();

	/** Sets the Level's streaming class */
	void SetStreamingClass( UClass *LevelStreamingClass );
		
private:
	/** Updates cached value of package file availability */
	void UpdatePackageFileAvailability();

	// Begin FEditorUndoClient
	virtual void PostUndo(bool bSuccess) OVERRIDE { Update(); }
	virtual void PostRedo(bool bSuccess) OVERRIDE { PostUndo(bSuccess); }
	// End of FEditorUndoClient

private:
	/** The Actor stats of the Level */
	TArray< FLayerActorStats > ActorStats;
	
	/** The LevelStreaming this object represents */
	TWeakObjectPtr< ULevelStreaming > LevelStreaming;

	/** The index of this level within its parent UWorld's StreamingLevels Array;  Used for manual sorting */
	int StreamingLevelIndex;

	/** Whether underlying streaming level object has a valid package name */
	bool bHasValidPackageName;
};