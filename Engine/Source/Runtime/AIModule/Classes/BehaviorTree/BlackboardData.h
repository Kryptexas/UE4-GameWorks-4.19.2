// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Engine/DataAsset.h"
#include "BlackboardData.generated.h"

/** blackboard entry definition */
USTRUCT()
struct FBlackboardEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Blackboard)
	FName EntryName;

	/** key type and additional properties */
	UPROPERTY(EditAnywhere, EditInline, Category=Blackboard)
	UBlackboardKeyType* KeyType;
};

UCLASS()
class AIMODULE_API UBlackboardData : public UDataAsset
{
	GENERATED_UCLASS_BODY()
	DECLARE_MULTICAST_DELEGATE_OneParam(FKeyUpdate, UBlackboardData* /*asset*/);

	enum 
	{
		InvalidKeyID = 0xFF
	};

	/** parent blackboard (keys can be overridden) */
	UPROPERTY(EditAnywhere, Category=Parent)
	class UBlackboardData* Parent;

#if WITH_EDITORONLY_DATA
	/** all keys inherited from parent chain */
	UPROPERTY(VisibleDefaultsOnly, Transient, Category=Parent)
	TArray<FBlackboardEntry> ParentKeys;
#endif

	/** blackboard keys */
	UPROPERTY(EditAnywhere, EditInLine, Category=Blackboard)
	TArray<FBlackboardEntry> Keys;

	/** @return key ID from name */
	uint8 GetKeyID(const FName& KeyName) const;

	/** @return name of key */
	FName GetKeyName(uint8 KeyID) const;

	/** @return class of value for given key */
	TSubclassOf<UBlackboardKeyType> GetKeyType(uint8 KeyID) const;

	/** @return number of defined keys, including parent chain */
	int32 GetNumKeys() const;

	FORCEINLINE int32 GetFirstKeyID() const { return FirstKeyID; }

	/** @return key data */
	const FBlackboardEntry* GetKey(uint8 KeyID) const;

	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif

	/** @return true if blackboard keys are not conflicting with parent key chain */
	bool IsValid() const;

	/** updates persistent key with given name, depending on currently defined entries and parent chain
	 *  @return key type of newly created entry for further setup
	 */
	template<class T>
	T* UpdatePersistentKey(const FName& KeyName)
	{
		T* CreatedKeyType = NULL;

		const uint8 KeyID = InternalGetKeyID(KeyName, DontCheckParentKeys);
		if (KeyID == InvalidKeyID && Parent == NULL)
		{
			FBlackboardEntry Entry;
			Entry.EntryName = KeyName;

			CreatedKeyType = ConstructObject<T>(T::StaticClass(), this);
			Entry.KeyType = CreatedKeyType;		

			Keys.Add(Entry);
			MarkPackageDirty();
		}
		else if (KeyID != InvalidKeyID && Parent != NULL)
		{
			const int32 KeyIndex = KeyID - FirstKeyID;
			Keys.RemoveAt(KeyIndex);
			MarkPackageDirty();
		}

		return CreatedKeyType;
	}

	/** delegate called for every loaded blackboard asset
	 *  meant for adding game specific persistent keys */
	static FKeyUpdate OnUpdateKeys;

protected:

	enum EKeyLookupMode
	{
		CheckParentKeys,
		DontCheckParentKeys,
	};

	/** @return first ID for keys of this asset (parent keys goes first) */
	int32 FirstKeyID;

	/** @return key ID from name */
	uint8 InternalGetKeyID(const FName& KeyName, EKeyLookupMode LookupMode) const;

	/** updates parent key cache for editor */
	void UpdateParentKeys();
};
