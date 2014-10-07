// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagsModule.h"
#include "GameplayEffectTypes.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FGameplayEffectContext
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FGameplayEffectContext::AddInstigator(class AActor *InInstigator, class AActor *InEffectCauser)
{
	Instigator = InInstigator;
	EffectCauser = InEffectCauser;
	InstigatorAbilitySystemComponent = NULL;

	// Cache off his AbilitySystemComponent.
	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Instigator);
	if (AbilitySystemInterface)
	{
		InstigatorAbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
	}
}

void FGameplayEffectContext::AddHitResult(const FHitResult InHitResult)
{
	check(!HitResult.IsValid());
	HitResult = TSharedPtr<FHitResult>(new FHitResult(InHitResult));
}

bool FGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Instigator;
	Ar << EffectCauser;

	bool HasHitResults = HitResult.IsValid();
	Ar << HasHitResults;
	if (Ar.IsLoading())
	{
		if (HasHitResults)
		{
			if (!HitResult.IsValid())
			{
				HitResult = TSharedPtr<FHitResult>(new FHitResult());
			}
		}
		AddInstigator(Instigator, EffectCauser); // Just to initialize InstigatorAbilitySystemComponent
	}

	if (HasHitResults == 1)
	{
		HitResult->NetSerialize(Ar, Map, bOutSuccess);
	}

	Ar << bHasWorldOrigin;
	Ar << WorldOrigin;

	bOutSuccess = true;
	return true;
}

bool FGameplayEffectContext::IsLocallyControlled() const
{
	APawn* Pawn = Cast<APawn>(Instigator);
	if (!Pawn)
	{
		Pawn = Cast<APawn>(EffectCauser);
	}
	if (Pawn)
	{
		return Pawn->IsLocallyControlled();
	}
	return false;
}

void FGameplayEffectContext::AddOrigin(FVector InOrigin)
{
	bHasWorldOrigin = true;
	WorldOrigin = InOrigin;
}

bool FGameplayEffectContextHandle::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	UScriptStruct* ScriptStruct = Data.IsValid() ? Data->GetScriptStruct() : NULL;
	Ar << ScriptStruct;

	if (ScriptStruct)
	{
		if (Ar.IsLoading())
		{
			// For now, just always reset/reallocate the data when loading.
			// Longer term if we want to generalize this and use it for property replication, we should support
			// only reallocating when necessary
			check(!Data.IsValid());

			FGameplayEffectContext * NewData = (FGameplayEffectContext*)FMemory::Malloc(ScriptStruct->GetCppStructOps()->GetSize());
			ScriptStruct->InitializeScriptStruct(NewData);

			Data = TSharedPtr<FGameplayEffectContext>(NewData);
		}

		void* ContainerPtr = Data.Get();

		if (ScriptStruct->StructFlags & STRUCT_NetSerializeNative)
		{
			ScriptStruct->GetCppStructOps()->NetSerialize(Ar, Map, bOutSuccess, Data.Get());
		}
		else
		{
			// This won't work since UStructProperty::NetSerializeItem is deprecrated.
			//	1) we have to manually crawl through the topmost struct's fields since we don't have a UStructProperty for it (just the UScriptProperty)
			//	2) if there are any UStructProperties in the topmost struct's fields, we will assert in UStructProperty::NetSerializeItem.

			ABILITY_LOG(Fatal, TEXT("FGameplayEffectContextHandle::NetSerialize called on data struct %s without a native NetSerialize"), *ScriptStruct->GetName());

			for (TFieldIterator<UProperty> It(ScriptStruct); It; ++It)
			{
				if (It->PropertyFlags & CPF_RepSkip)
				{
					continue;
				}

				void * PropertyData = It->ContainerPtrToValuePtr<void*>(ContainerPtr);

				It->NetSerializeItem(Ar, Map, PropertyData);
			}
		}
	}

	bOutSuccess = true;
	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	Misc
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

FString EGameplayModOpToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayModOp"));
	return e->GetEnum(Type).ToString();
}

FString EGameplayModToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayMod"));
	return e->GetEnum(Type).ToString();
}

FString EGameplayModEffectToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayModEffect"));
	return e->GetEnum(Type).ToString();
}

FString EGameplayEffectCopyPolicyToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayEffectCopyPolicy"));
	return e->GetEnum(Type).ToString();
}

FString EGameplayEffectStackingPolicyToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayEffectStackingPolicy"));
	return e->GetEnum(Type).ToString();
}

bool FGameplayTagCountContainer::HasMatchingGameplayTag(FGameplayTag TagToCheck, EGameplayTagMatchType::Type TagMatchType) const
{
	if (TagMatchType == EGameplayTagMatchType::Explicit)
	{
		// Search for TagToCheck
		const int32* Count = GameplayTagCountMap.Find(TagToCheck);
		if (Count && *Count > 0)
		{
			return true;
		}
	}
	else if (TagMatchType == EGameplayTagMatchType::IncludeParentTags)
	{
		// Search for TagToCheck or any of its parent tags
		FGameplayTagContainer TagAndParentsContainer = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTagParents(TagToCheck);
		for (auto TagIt = TagAndParentsContainer.CreateConstIterator(); TagIt; ++TagIt)
		{
			const int32* Count = GameplayTagCountMap.Find(*TagIt);
			if (Count && *Count > 0)
			{
				return true;
			}
		}
	}

	return false;
}

bool FGameplayTagCountContainer::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer, EGameplayTagMatchType::Type TagMatchType, bool bCountEmptyAsMatch) const
{
	if (TagContainer.Num() == 0)
		return bCountEmptyAsMatch;

	for (auto It = TagContainer.CreateConstIterator(); It; ++It)
	{
		if (!HasMatchingGameplayTag(*It, TagMatchType))
		{
			return false;
		}
	}

	return true;
}


bool FGameplayTagCountContainer::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer, EGameplayTagMatchType::Type TagMatchType, bool bCountEmptyAsMatch) const
{
	if (TagContainer.Num() == 0)
		return bCountEmptyAsMatch;

	for (auto It = TagContainer.CreateConstIterator(); It; ++It)
	{
		if (HasMatchingGameplayTag(*It, TagMatchType))
		{
			return true;
		}
	}

	return false;
}

void FGameplayTagCountContainer::UpdateTagMap(const FGameplayTag& Tag, int32 CountDelta)
{
	// Update count of Tag
	int32& Count = GameplayTagCountMap.FindOrAdd(Tag);

	bool WasZero = Count == 0;
	Count = FMath::Max(Count + CountDelta, 0);
		
	// If we went from 0->1 or 1->0
	if (WasZero || Count == 0)
	{
		FOnGameplayEffectTagCountChanged *Delegate = GameplayTagEventMap.Find(Tag);
		if (Delegate)
		{
			Delegate->Broadcast(Tag, Count);
		}
	}
}

void FGameplayTagCountContainer::UpdateTagMap(const FGameplayTagContainer& Container, int32 CountDelta)
{
	for (auto TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
	{
		const FGameplayTag& BaseTag = *TagIt;
		if (TagContainerType == EGameplayTagMatchType::Explicit)
		{
			// Update count of BaseTag
			UpdateTagMap(BaseTag, CountDelta);
		}
		else if (TagContainerType == EGameplayTagMatchType::IncludeParentTags)
		{
			// Update count of BaseTag and all of its parent tags
			FGameplayTagContainer TagAndParentsContainer = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTagParents(BaseTag);
			for (auto ParentTagIt = TagAndParentsContainer.CreateConstIterator(); ParentTagIt; ++ParentTagIt)
			{
				UpdateTagMap(*ParentTagIt, CountDelta);
			}
		}
	}
}



