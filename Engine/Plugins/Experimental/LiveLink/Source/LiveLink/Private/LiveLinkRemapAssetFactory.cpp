// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkRemapAssetFactory.h"
#include "LiveLinkRemapAsset.h"

#define LOCTEXT_NAMESPACE "LiveLinkRemapAssetFactory"

ULiveLinkRemapAssetFactory::ULiveLinkRemapAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = ULiveLinkRemapAsset::StaticClass();
}

UObject* ULiveLinkRemapAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	ULiveLinkRemapAsset* NewRemapAsset = NewObject<ULiveLinkRemapAsset>(InParent, Name, Flags | RF_Transactional);

	return NewRemapAsset;
}

bool ULiveLinkRemapAssetFactory::ShouldShowInNewMenu() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
