// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UPaperSpriteAtlas

UPaperSpriteAtlas::UPaperSpriteAtlas(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, MaxWidth(2048)
	, MaxHeight(2048)
{
}

void UPaperSpriteAtlas::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	Super::GetAssetRegistryTags(OutTags);

#if WITH_EDITORONLY_DATA
	OutTags.Add(FAssetRegistryTag(GET_MEMBER_NAME_CHECKED(UPaperSpriteAtlas, AtlasDescription), AtlasDescription, FAssetRegistryTag::TT_Hidden));
#endif
}

void UPaperSpriteAtlas::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	FPlatformMisc::CreateGuid(AtlasGUID);
}

void UPaperSpriteAtlas::PostInitProperties()
{
	Super::PostInitProperties();
	FPlatformMisc::CreateGuid(AtlasGUID);
}
