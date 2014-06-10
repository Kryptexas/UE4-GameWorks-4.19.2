// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsModulePrivatePCH.h"

UBlueprintGameplayTagLibrary::UBlueprintGameplayTagLibrary(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
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
