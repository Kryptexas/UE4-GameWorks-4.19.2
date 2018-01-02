// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ContentSourceProviderManager.h"

void FContentSourceProviderManager::RegisterContentSourceProvider(TSharedRef<IContentSourceProvider> ContentSourceProvider)
{
	ContentSourceProviders.Add(ContentSourceProvider);
}

const TArray<TSharedRef<IContentSourceProvider>>* FContentSourceProviderManager::GetContentSourceProviders()
{
	return &ContentSourceProviders;
}
