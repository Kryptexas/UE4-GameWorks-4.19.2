// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EditorWidgetsPrivatePCH.h"
#include "SCreateNewAssetFromFactory.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"

UObject* SCreateNewAssetFromFactory::Create(TWeakObjectPtr<UFactory> FactoryPtr)
{
	if (!FactoryPtr.IsValid())
	{
		return nullptr;
	}

	FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
	return AssetToolsModule.Get().CreateAsset(FactoryPtr->GetSupportedClass(), FactoryPtr.Get());
}

