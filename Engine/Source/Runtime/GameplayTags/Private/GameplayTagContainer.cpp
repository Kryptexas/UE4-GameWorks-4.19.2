// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsModulePrivatePCH.h"

FGameplayTagContainer::FGameplayTagContainer()
{}

FGameplayTagContainer::FGameplayTagContainer(FGameplayTagContainer const& Other)
{
	*this = Other;
}

FGameplayTagContainer& FGameplayTagContainer::operator=(FGameplayTagContainer const& Other)
{
	// Guard against self-assignment
	if (this == &Other)
	{
		return *this;
	}
	GameplayTags.Empty(Other.GameplayTags.Num());
	GameplayTags.Append(Other.GameplayTags);

	return *this;
}

bool FGameplayTagContainer::operator==(FGameplayTagContainer const& Other) const
{
	if (GameplayTags.Num() != Other.GameplayTags.Num())
	{
		return false;
	}
	return Filter(Other, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::Explicit).Num() == this->Num();
}

bool FGameplayTagContainer::operator!=(FGameplayTagContainer const& Other) const
{
	if (GameplayTags.Num() != Other.GameplayTags.Num())
	{
		return true;
	}
	return Filter(Other, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::Explicit).Num() != this->Num();
}

bool FGameplayTagContainer::HasTag(FGameplayTag const& TagToCheck, TEnumAsByte<EGameplayTagMatchType::Type> TagMatchType, TEnumAsByte<EGameplayTagMatchType::Type> TagToCheckMatchType) const
{
	UGameplayTagsManager& TagManager = IGameplayTagsModule::Get().GetGameplayTagsManager();
	for (TArray<FGameplayTag>::TConstIterator It(this->GameplayTags); It; ++It)
	{
		if (TagManager.GameplayTagsMatch(*It, TagMatchType, TagToCheck, TagToCheckMatchType) == true)
		{
			return true;
		}
	}
	return false;
}

FGameplayTagContainer FGameplayTagContainer::GetGameplayTagParents() const
{
	FGameplayTagContainer ResultContainer;
	ResultContainer.AppendTags(*this);

	for (TArray<FGameplayTag>::TConstIterator TagIterator(GameplayTags); TagIterator; ++TagIterator)
	{
		FGameplayTagContainer ParentTags = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTagParents(*TagIterator);
		ResultContainer.AppendTags(ParentTags);
	}

	return ResultContainer;
}

FGameplayTagContainer FGameplayTagContainer::Filter(const FGameplayTagContainer& OtherContainer, TEnumAsByte<EGameplayTagMatchType::Type> TagMatchType, TEnumAsByte<EGameplayTagMatchType::Type> OtherTagMatchType) const
{
	FGameplayTagContainer ResultContainer;
	UGameplayTagsManager& TagManager = IGameplayTagsModule::Get().GetGameplayTagsManager();

	
	for (TArray<FGameplayTag>::TConstIterator OtherIt(OtherContainer.GameplayTags); OtherIt; ++OtherIt)
	{
		for (TArray<FGameplayTag>::TConstIterator It(this->GameplayTags); It; ++It)
		{
			if (TagManager.GameplayTagsMatch(*It, TagMatchType, *OtherIt, OtherTagMatchType) == true)
			{
				ResultContainer.AddTag(*It);
			}
		}
	}

	return ResultContainer;
}

bool FGameplayTagContainer::DoesTagContainerMatch(const FGameplayTagContainer& OtherContainer, TEnumAsByte<EGameplayTagMatchType::Type> TagMatchType, TEnumAsByte<EGameplayTagMatchType::Type> OtherTagMatchType, TEnumAsByte<EGameplayContainerMatchType::Type> ContainerMatchType) const
{
	UGameplayTagsManager& TagManager = IGameplayTagsModule::Get().GetGameplayTagsManager();

	for (TArray<FGameplayTag>::TConstIterator OtherIt(OtherContainer.GameplayTags); OtherIt; ++OtherIt)
	{
		bool bTagFound = false;

		for (TArray<FGameplayTag>::TConstIterator It(this->GameplayTags); It; ++It)
		{
			if (TagManager.GameplayTagsMatch(*It, TagMatchType, *OtherIt, OtherTagMatchType) == true)
			{
				if (ContainerMatchType == EGameplayContainerMatchType::Any)
				{
					return true;
				}

				bTagFound = true;

				// we only need one match per tag in OtherContainer, so don't bother looking for more
				break;
			}
		}

		if (ContainerMatchType == EGameplayContainerMatchType::All && bTagFound == false)
		{
			return false;
		}
	}

	// if we've reached this far then either we are looking for any match and didn't find one (return false) or we're looking for all matches and didn't miss one (return true).
	check(ContainerMatchType == EGameplayContainerMatchType::All || ContainerMatchType == EGameplayContainerMatchType::Any);
	return ContainerMatchType == EGameplayContainerMatchType::All;
}


bool FGameplayTagContainer::MatchesAll(FGameplayTagContainer const& Other, bool bCountEmptyAsMatch) const
{
	if (Other.Num() == 0)
	{
		return bCountEmptyAsMatch;
	}

	return DoesTagContainerMatch(Other, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit, EGameplayContainerMatchType::All);
}

bool FGameplayTagContainer::MatchesAny(FGameplayTagContainer const& Other, bool bCountEmptyAsMatch) const
{
	if (Other.Num() == 0)
	{
		return bCountEmptyAsMatch;
	}

	return DoesTagContainerMatch(Other, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit, EGameplayContainerMatchType::Any);
}

void FGameplayTagContainer::AppendTags(FGameplayTagContainer const& Other)
{
	//add all the tags
	for(TArray<FGameplayTag>::TConstIterator It(Other.GameplayTags); It; ++It)
	{
		AddTag(*It);
	}
}

void FGameplayTagContainer::AddTag(const FGameplayTag& TagToAdd)
{
	if (TagToAdd.IsValid())
	{
		// Don't want duplicate tags
		GameplayTags.AddUnique(TagToAdd);
	}
}

void FGameplayTagContainer::RemoveTag(FGameplayTag TagToRemove)
{
	GameplayTags.Remove(TagToRemove);
}

void FGameplayTagContainer::RemoveAllTags()
{
	GameplayTags.Empty();
}

bool FGameplayTagContainer::Serialize(FArchive& Ar)
{
	const bool bOldTagVer = Ar.UE4Ver() < VER_UE4_GAMEPLAY_TAG_CONTAINER_TAG_TYPE_CHANGE;
	
	if (bOldTagVer)
	{
		Ar << Tags_DEPRECATED;
	}
	else
	{
		Ar << GameplayTags;
	}
	
	if (Ar.IsLoading())
	{
		// Regardless of version, want loading to have a chance to handle redirects
		RedirectTags();

		// If loading old version, add old tags to the new gameplay tags array so they can be saved out with the new version
		if (bOldTagVer)
		{
			UGameplayTagsManager& TagManager = IGameplayTagsModule::Get().GetGameplayTagsManager();
			for (auto It = Tags_DEPRECATED.CreateConstIterator(); It; ++It)
			{
				FGameplayTag Tag = TagManager.RequestGameplayTag(*It);
				TagManager.AddLeafTagToContainer(*this, Tag);
			}
		}
	}

	return true;
}

void FGameplayTagContainer::RedirectTags()
{
	FConfigSection* PackageRedirects = GConfig->GetSectionPrivate( TEXT("/Script/Engine.Engine"), false, true, GEngineIni );
	UGameplayTagsManager& TagManager = IGameplayTagsModule::Get().GetGameplayTagsManager();
	
	for (FConfigSection::TIterator It(*PackageRedirects); It; ++It)
	{
		if (It.Key() == TEXT("GameplayTagRedirects"))
		{
			FName OldTagName = NAME_None;
			FName NewTagName;

			FParse::Value( *It.Value(), TEXT("OldTagName="), OldTagName );
			FParse::Value( *It.Value(), TEXT("NewTagName="), NewTagName );

			int32 TagIndex = 0;
			if (Tags_DEPRECATED.Find(OldTagName, TagIndex))
			{
				Tags_DEPRECATED.RemoveAt(TagIndex);
				Tags_DEPRECATED.AddUnique(NewTagName);
			}

			FGameplayTag OldTag = TagManager.RequestGameplayTag(OldTagName);
			FGameplayTag NewTag = TagManager.RequestGameplayTag(NewTagName);

			if (HasTag(OldTag, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::Explicit))
			{
				RemoveTag(OldTag);
				AddTag(NewTag);
			}
		}
	}
}

int32 FGameplayTagContainer::Num() const
{
	return GameplayTags.Num();
}

FString FGameplayTagContainer::ToString() const
{
	FString RetString = TEXT("(GameplayTags=(");
	for (int i = 0; i < GameplayTags.Num(); ++i)
	{
		RetString += TEXT("(TagName=\"");
		RetString += GameplayTags[i].ToString();
		RetString += TEXT("\")");
		if (i < GameplayTags.Num() - 1)
		{
			RetString += TEXT(",");
		}
	}
	RetString += TEXT("))");
	return RetString;
}

FGameplayTag::FGameplayTag()
	: TagName(NAME_None)
{
}

bool FGameplayTag::Matches(TEnumAsByte<EGameplayTagMatchType::Type> MatchTypeOne, const FGameplayTag& Other, TEnumAsByte<EGameplayTagMatchType::Type> MatchTypeTwo) const
{
	return IGameplayTagsModule::Get().GetGameplayTagsManager().GameplayTagsMatch(*this, MatchTypeOne, Other, MatchTypeTwo);
}

bool FGameplayTag::IsValid() const
{
	return (TagName != NAME_None);
}

FGameplayTag::FGameplayTag(FName Name)
	: TagName(Name)
{
	check(IGameplayTagsModule::Get().GetGameplayTagsManager().ValidateTagCreation(Name));
}
