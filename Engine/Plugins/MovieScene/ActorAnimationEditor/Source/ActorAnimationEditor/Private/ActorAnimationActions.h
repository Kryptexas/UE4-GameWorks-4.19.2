// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"


/**
 * Implements actions for UActorAnimation assets.
 */
class FActorAnimationActions
	: public FAssetTypeActions_Base
{
public:
	
	// IAssetTypeActions interface

	virtual uint32 GetCategories() override;
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual bool ShouldForceWorldCentric() override;
};
