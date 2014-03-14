// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"

class FTileSetAssetTypeActions : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions interface
	virtual FText GetName() const OVERRIDE;
	virtual FColor GetTypeColor() const OVERRIDE;
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE;
	// End of IAssetTypeActions interface
};