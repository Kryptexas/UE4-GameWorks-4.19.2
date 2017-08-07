// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"

/**
 * Implements actions for ULiveLinkRemapAsset assets.
 */
class FLiveLinkRemapAssetActions
	: public FAssetTypeActions_Base
{
public:

	/**
	 * Creates and initializes a new remap.
	 */
	FLiveLinkRemapAssetActions();

public:
	
	// Begin IAssetTypeActions interface
	virtual uint32 GetCategories() override;
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual bool CanLocalize() const { return false; }
	// End IAssetTypeActions interface
};
