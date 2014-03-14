// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
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
class ENGINE_API UBlackboardData : public UDataAsset
{
	GENERATED_UCLASS_BODY()

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

	/** If you want some keys always present in BB assets make it happen in this function */
	virtual void UpdatePersistentKeys();
};
