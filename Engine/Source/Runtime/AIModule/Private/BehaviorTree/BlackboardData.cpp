// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/BlackboardData.h"

UBlackboardData::FKeyUpdate UBlackboardData::OnUpdateKeys;

static void UpdatePersistentKeys(class UBlackboardData* Asset)
{
	UBlackboardKeyType_Object* SelfKeyType = Asset->UpdatePersistentKey<UBlackboardKeyType_Object>(FBlackboard::KeySelf);
	if (SelfKeyType)
	{
		SelfKeyType->BaseClass = AActor::StaticClass();
	}
}

UBlackboardData::UBlackboardData(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		OnUpdateKeys.AddStatic(&UpdatePersistentKeys);
	}
}

FBlackboard::FKey UBlackboardData::GetKeyID(const FName& KeyName) const
{
	return InternalGetKeyID(KeyName, CheckParentKeys);
}

FName UBlackboardData::GetKeyName(FBlackboard::FKey KeyID) const
{
	const FBlackboardEntry* KeyEntry = GetKey(KeyID);
	return KeyEntry ? KeyEntry->EntryName : NAME_None;
}

TSubclassOf<UBlackboardKeyType> UBlackboardData::GetKeyType(FBlackboard::FKey KeyID) const
{
	const FBlackboardEntry* KeyEntry = GetKey(KeyID);
	return KeyEntry ? KeyEntry->KeyType->GetClass() : NULL;
}

const FBlackboardEntry* UBlackboardData::GetKey(FBlackboard::FKey KeyID) const
{
	if (KeyID != FBlackboard::InvalidKey)
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

FBlackboard::FKey UBlackboardData::InternalGetKeyID(const FName& KeyName, EKeyLookupMode LookupMode) const
{
	for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); KeyIndex++)
	{
		if (Keys[KeyIndex].EntryName == KeyName)
		{
			return KeyIndex + FirstKeyID;
		}
	}

	return Parent && (LookupMode == CheckParentKeys) ? Parent->InternalGetKeyID(KeyName, LookupMode) : FBlackboard::InvalidKey;
}

bool UBlackboardData::IsValid() const
{
	if (Parent)
	{
		for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); KeyIndex++)
		{
			const FBlackboard::FKey KeyID = Parent->InternalGetKeyID(Keys[KeyIndex].EntryName, CheckParentKeys);
			if (KeyID != FBlackboard::InvalidKey)
			{
				UE_LOG(LogBehaviorTree, Warning, TEXT("Blackboard asset (%s) has duplicated key (%s) in parent chain!"),
					*GetName(), *Keys[KeyIndex].EntryName.ToString());

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
	for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); KeyIndex++)
	{
		if (Keys[KeyIndex].EntryName == KeyName)
		{
			return true;
		}
	}

	for (int32 KeyIndex = 0; KeyIndex < ParentKeys.Num(); KeyIndex++)
	{
		if (ParentKeys[KeyIndex].EntryName == KeyName)
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
		for (int32 KeyIndex = 0; KeyIndex < It->Keys.Num(); KeyIndex++)
		{
			const bool bAlreadyExist = ContainsKeyName(It->Keys[KeyIndex].EntryName, Keys, ParentKeys);
			if (!bAlreadyExist)
			{
				ParentKeys.Add(It->Keys[KeyIndex]);
			}
		}
	}
#endif // WITH_EDITORONLY_DATA

	FirstKeyID = Parent ? Parent->GetNumKeys() : 0;
	OnUpdateKeys.Broadcast(this);
}
