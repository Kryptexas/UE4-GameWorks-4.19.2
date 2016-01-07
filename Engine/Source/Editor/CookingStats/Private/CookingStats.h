// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


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
	* AddTag
	* add a float tag to a key, these tags will be saved out with the key
	* this is an optimization for time tags so we don't have to process them as strings
	*
	* @param Key key to add the tag to
	* @param Tag tag to add to the key
	*/
	virtual void AddTagValue(const FName& Key, const FName& Tag, const float Value) override;

	/**
	* AddTag
	* add a bool tag to a key, these tags will be saved out with the key
	* this is an optimization for time tags so we don't have to process them as strings
	*
	* @param Key key to add the tag to
	* @param Tag tag to add to the key
	*/
	virtual void AddTagValue(const FName& Key, const FName& Tag, const bool Value) override;


	/**
	* AddTag
	* add a int32 tag to a key, these tags will be saved out with the key
	* this is an optimization for time tags so we don't have to process them as strings
	*
	* @param Key key to add the tag to
	* @param Tag tag to add to the key
	*/
	virtual void AddTagValue(const FName& Key, const FName& Tag, const int32 Value) override;


	/**
	* GetTagValue
	* retrieve a previously added tag value
	*
	* @param Key to use to get the tag value
	* @param Tag to use to get the tag value
	* @param Value the value which will be returned fomr that tag key pair
	* @return if the key + tag exists will retrun true false otherwise
	*/
	virtual bool GetTagValue(const FName& Key, const FName& Tag, FString& Value) const override;

	/**
	* SaveStatsAsCSV
	* Save stats in a CSV file
	*
	* @param Filename file name to save comma delimited stats to
	* @return true if succeeded
	*/
	virtual bool SaveStatsAsCSV(const FString& Filename) const override;

private:
	FName RunGuid; // unique guid for this run of the editor
	struct FTag
	{
	private:
		enum ETagType
		{
			TT_String,
			TT_Float,
			TT_Int,
			TT_Bool,
		} Type;
		FString String;
		union
		{
			float Float;
			bool Bool;
			int32 Int;
		};

	public:
		FTag(const FString& InString) : Type(TT_String), String(InString) { }
		FTag(float InFloat) : Type(TT_Float), Float(InFloat) { }
		FTag(bool InBool) : Type(TT_Bool), Bool(InBool) { }
		FTag(int32 InInt) : Type(TT_Int), Int(InInt) { }

		FString ToString() const
		{
			switch (Type)
			{
			case TT_String:
				return String;
			case TT_Float:
				return FString::Printf(TEXT("%f"), Float);
			case TT_Int:
				return FString::Printf(TEXT("%d"), Int);
			case TT_Bool:
				return Bool ? TEXT("true") : TEXT("false");
			default:
				return FString();
			}
		}
		bool IsEmpty() const
		{
			if (TT_String)
			{
				return String.IsEmpty();
			}
			return false;
		}
	};
	TMap< FName, TMap<FName, FTag> > KeyTags;

	TMap<FName, FString> GlobalTags; // tags to add to every key in this runthrough
	mutable FCriticalSection SyncObject;
};