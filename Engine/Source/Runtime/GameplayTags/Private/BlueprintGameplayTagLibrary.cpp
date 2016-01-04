// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsModulePrivatePCH.h"

UBlueprintGameplayTagLibrary::UBlueprintGameplayTagLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UBlueprintGameplayTagLibrary::DoGameplayTagsMatch(const FGameplayTag& TagOne, const FGameplayTag& TagTwo, TEnumAsByte<EGameplayTagMatchType::Type> TagOneMatchType, TEnumAsByte<EGameplayTagMatchType::Type> TagTwoMatchType)
{
	return TagOne.Matches(TagOneMatchType, TagTwo, TagTwoMatchType);
}

int32 UBlueprintGameplayTagLibrary::GetNumGameplayTagsInContainer(const FGameplayTagContainer& TagContainer)
{
	return TagContainer.Num();
}

bool UBlueprintGameplayTagLibrary::DoesContainerHaveTag(const FGameplayTagContainer& TagContainer, TEnumAsByte<EGameplayTagMatchType::Type> ContainerTagsMatchType, const FGameplayTag& Tag, TEnumAsByte<EGameplayTagMatchType::Type> TagMatchType)
{
	return (TagContainer.HasTag(Tag, ContainerTagsMatchType, TagMatchType));
}

bool UBlueprintGameplayTagLibrary::DoesContainerMatchAnyTagsInContainer(const FGameplayTagContainer& TagContainer, const FGameplayTagContainer& OtherContainer, bool bCountEmptyAsMatch)
{
	return TagContainer.MatchesAny(OtherContainer, bCountEmptyAsMatch);
}

bool UBlueprintGameplayTagLibrary::DoesContainerMatchAllTagsInContainer(const FGameplayTagContainer& TagContainer, const FGameplayTagContainer& OtherContainer, bool bCountEmptyAsMatch)
{
	return TagContainer.MatchesAll(OtherContainer, bCountEmptyAsMatch);
}

bool UBlueprintGameplayTagLibrary::DoesContainerMatchTagQuery(const FGameplayTagContainer& TagContainer, const FGameplayTagQuery& TagQuery)
{
	return TagQuery.Matches(TagContainer);
}

FGameplayTag UBlueprintGameplayTagLibrary::MakeLiteralGameplayTag(FGameplayTag Value)
{
	return Value;
}

FGameplayTagQuery UBlueprintGameplayTagLibrary::MakeGameplayTagQuery(FGameplayTagQuery TagQuery)
{
	return TagQuery;
}

bool UBlueprintGameplayTagLibrary::HasAllMatchingGameplayTags(TScriptInterface<IGameplayTagAssetInterface> TagContainerInterface, const FGameplayTagContainer& OtherContainer, bool bCountEmptyAsMatch)
{
	if (TagContainerInterface.GetInterface() == NULL)
	{
		if (bCountEmptyAsMatch)
		{
			return (OtherContainer.Num() == 0);
		}
		return false;
	}

	return TagContainerInterface->HasAllMatchingGameplayTags(OtherContainer, bCountEmptyAsMatch);
}

bool UBlueprintGameplayTagLibrary::DoesTagAssetInterfaceHaveTag(TScriptInterface<IGameplayTagAssetInterface> TagContainerInterface, TEnumAsByte<EGameplayTagMatchType::Type> ContainerTagsMatchType, const FGameplayTag& Tag, TEnumAsByte<EGameplayTagMatchType::Type> TagMatchType)
{
	if (TagContainerInterface.GetInterface() == NULL)
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	TagContainerInterface->GetOwnedGameplayTags(OwnedTags);
	return (OwnedTags.HasTag(Tag, ContainerTagsMatchType, TagMatchType));
}

bool UBlueprintGameplayTagLibrary::AppendGameplayTagContainers(const FGameplayTagContainer& InTagContainer, UPARAM(ref) FGameplayTagContainer& InOutTagContainer)
{
	InOutTagContainer.AppendTags(InTagContainer);

	return true;
}

bool UBlueprintGameplayTagLibrary::NotEqual_TagTag(FGameplayTag A, FString B)
{
	return A.ToString() != B;
}

bool UBlueprintGameplayTagLibrary::NotEqual_TagContainerTagContainer(FGameplayTagContainer A, FString B)
{
	FGameplayTagContainer TagContainer;

	// Convert string to Tag Container before compare
	FString TagString = B;
	if (TagString.StartsWith(TEXT("(")) && TagString.EndsWith(TEXT(")")))
	{
		TagString = TagString.LeftChop(1);
		TagString = TagString.RightChop(1);

		TagString.Split("=", NULL, &TagString);

		TagString = TagString.LeftChop(1);
		TagString = TagString.RightChop(1);

		FString ReadTag;
		FString Remainder;

		while (TagString.Split(TEXT(","), &ReadTag, &Remainder))
		{
			ReadTag.Split("=", NULL, &ReadTag);
			if (ReadTag.EndsWith(TEXT(")")))
			{
				ReadTag = ReadTag.LeftChop(1);
				if (ReadTag.StartsWith(TEXT("\"")) && ReadTag.EndsWith(TEXT("\"")))
				{
					ReadTag = ReadTag.LeftChop(1);
					ReadTag = ReadTag.RightChop(1);
				}
			}
			TagString = Remainder;
			FGameplayTag Tag = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTag(FName(*ReadTag));
			TagContainer.AddTag(Tag);
		}
		if (Remainder.IsEmpty())
		{
			Remainder = TagString;
		}
		if (!Remainder.IsEmpty())
		{
			Remainder.Split("=", NULL, &Remainder);
			if (Remainder.EndsWith(TEXT(")")))
			{
				Remainder = Remainder.LeftChop(1);
				if (Remainder.StartsWith(TEXT("\"")) && Remainder.EndsWith(TEXT("\"")))
				{
					Remainder = Remainder.LeftChop(1);
					Remainder = Remainder.RightChop(1);
				}
			}
			FGameplayTag Tag = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTag(FName(*Remainder));
			TagContainer.AddTag(Tag);
		}
	}

	return A != TagContainer;
}
