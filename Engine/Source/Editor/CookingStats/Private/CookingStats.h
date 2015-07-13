// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


class FCookingStats : public ICookingStats
{
public:

	FCookingStats();
	virtual ~FCookingStats();


	/**
	* AddRunTag
	* Add a tag and associate it with this run of the editor 
	* this is for storing global information from this run
	*
	* @param Tag tag which we want to keep
	* @param Value for this tag FString() if we just want to store the tag
	*/
	virtual void AddRunTag(const FName& Tag, const FString& Value) override;

	/**
	* AddTag
	* add a tag to a key, the tags will be saved out with the key
	* tags can be timing information or anything
	*
	* @param Key key to add the tag to
	* @param Tag tag to add to the key
	*/
	virtual void AddTag(const FName& Key, const FName& Tag) override;

	/**
	* AddTag
	* add a tag to a key, the tags will be saved out with the key
	* tags can be timing information or anything
	*
	* @param Key key to add the tag to
	* @param Tag tag to add to the key
	*/
	virtual void AddTagValue(const FName& Key, const FName& Tag, const FString& Value) override;

	/**
	* SaveStatsAsCSV
	* Save stats in a CSV file
	*
	* @param Filename file name to save comma delimited stats to
	* @return true if succeeded
	*/
	virtual bool SaveStatsAsCSV(const FString& Filename) override;

private:
	FName RunGuid; // unique guid for this run of the editor

	struct FTag
	{
		FName Name;
		FString Value;
	};
	TMap< FName, TArray<FTag> > KeyTags;
	TArray<FTag> GlobalTags; // tags to add to every key in this runthrough
	FCriticalSection SyncObject;
};