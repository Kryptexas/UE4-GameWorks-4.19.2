// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardData::UBlackboardData(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

uint8 UBlackboardData::GetKeyID(const FName& KeyName) const
{
	return InternalGetKeyID(KeyName, CheckParentKeys);
}

FName UBlackboardData::GetKeyName(uint8 KeyID) const
{
	const FBlackboardEntry* KeyEntry = GetKey(KeyID);
	return KeyEntry ? KeyEntry->EntryName : NAME_None;
}

TSubclassOf<UBlackboardKeyType> UBlackboardData::GetKeyType(uint8 KeyID) const
{
	const FBlackboardEntry* KeyEntry = GetKey(KeyID);
	return KeyEntry ? KeyEntry->KeyType->GetClass() : NULL;
}

const FBlackboardEntry* UBlackboardData::GetKey(uint8 KeyID) const
{
	if (KeyID != InvalidKeyID)
	{
		if (KeyID >= FirstKeyID)
		{
			return &Keys[KeyID - FirstKeyID];
		}
		else if (Parent)
		{
			return Parent->GetKey(KeyID);
		}
	}

	return NULL;
}

int32 UBlackboardData::GetNumKeys() const
{
	return FirstKeyID + Keys.Num();
}

uint8 UBlackboardData::InternalGetKeyID(const FName& KeyName, EKeyLookupMode LookupMode) const
{
	for (int32 i = 0; i < Keys.Num(); i++)
	{
		if (Keys[i].EntryName == KeyName)
		{
			return i + FirstKeyID;
		}
	}

	return Parent && (LookupMode == CheckParentKeys) ? Parent->InternalGetKeyID(KeyName, LookupMode) : InvalidKeyID;
}

bool UBlackboardData::IsValid() const
{
	if (Parent)
	{
		for (int32 i = 0; i < Keys.Num(); i++)
		{
			const uint8 KeyID = Parent->InternalGetKeyID(Keys[i].EntryName, CheckParentKeys);
			if (KeyID != InvalidKeyID)
			{
				UE_LOG(LogBehaviorTree, Warning, TEXT("Blackboard asset (%s) has duplicated key (%s) in parent chain!"),
					*GetName(), *Keys[i].EntryName.ToString());

				return false;
			}
		}
	}

	return true;
}

void UBlackboardData::PostLoad()
{
	Super::PostLoad();

	UpdateParentKeys();
}

#if WITH_EDITOR

void UBlackboardData::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UBlackboardData, Parent))
	{
		UpdateParentKeys();
	}
}

#endif // WITH_EDITOR

static bool ContainsKeyName(FName KeyName, const TArray<FBlackboardEntry>& Keys, const TArray<FBlackboardEntry>& ParentKeys)
{
	for (int32 i = 0; i < Keys.Num(); i++)
	{
		if (Keys[i].EntryName == KeyName)
		{
			return true;
		}
	}

	for (int32 i = 0; i < ParentKeys.Num(); i++)
	{
		if (ParentKeys[i].EntryName == KeyName)
		{
			return true;
		}
	}

	return false;
}

void UBlackboardData::UpdateParentKeys()
{
	if (Parent == this)
	{
		Parent = NULL;
	}

#if WITH_EDITORONLY_DATA
	ParentKeys.Reset();

	for (UBlackboardData* It = Parent; It; It = It->Parent)
	{
		for (int32 i = 0; i < It->Keys.Num(); i++)
		{
			const bool bAlreadyExist = ContainsKeyName(It->Keys[i].EntryName, Keys, ParentKeys);
			if (!bAlreadyExist)
			{
				ParentKeys.Add(It->Keys[i]);
			}
		}
	}

	UpdatePersistentKeys();
#endif // WITH_EDITORONLY_DATA

	FirstKeyID = Parent ? Parent->GetNumKeys() : 0;
}

void UBlackboardData::UpdatePersistentKeys()
{
	// first, naive implementation. If we wanted to have more persistent keys write a proper implementation
	static const FName& SelfKeyName = TEXT("SelfActor");

	const uint8 KeyID = InternalGetKeyID(SelfKeyName, DontCheckParentKeys);
	
	if (KeyID == InvalidKeyID && Parent == NULL)
	{
		FBlackboardEntry Entry;
		Entry.EntryName = SelfKeyName;

		UBlackboardKeyType_Object* KeyType = ConstructObject<UBlackboardKeyType_Object>(UBlackboardKeyType_Object::StaticClass(), this);
		KeyType->BaseClass = AActor::StaticClass();
		Entry.KeyType = KeyType;		

		Keys.Add(Entry);
		MarkPackageDirty();
	}
	else if (KeyID != InvalidKeyID && Parent != NULL)
	{
		const int32 KeyIndex = KeyID - FirstKeyID;
		Keys.RemoveAt(KeyIndex);
		MarkPackageDirty();
	}
}
