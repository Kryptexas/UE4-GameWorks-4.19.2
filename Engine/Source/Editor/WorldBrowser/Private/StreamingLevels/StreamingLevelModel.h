// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

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
	virtual bool HasValidPackage() const override;
	virtual UObject* GetNodeObject() override;
	virtual ULevel* GetLevelObject() const override;
	virtual FName GetAssetName() const override;
	virtual FName GetLongPackageName() const override;
	virtual void Update() override;
	virtual void OnDrop(const TSharedPtr<FLevelDragDropOp>& Op) override;
	virtual bool IsGoodToDrop(const TSharedPtr<FLevelDragDropOp>& Op) const override;
	virtual UClass* GetStreamingClass() const override;
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
	virtual void PostUndo(bool bSuccess) override { Update(); }
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
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